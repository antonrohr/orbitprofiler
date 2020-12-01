// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ProfilingTargetDialog.h"

#include <qradiobutton.h>

#include <QFileDialog>
#include <QFileInfo>
#include <QHistoryState>
#include <QItemSelectionModel>
#include <QObject>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QWidget>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

#include "ConnectToStadiaWidget.h"
#include "Connections.h"
#include "DeploymentConfigurations.h"
#include "MainThreadExecutor.h"
#include "OrbitBase/Logging.h"
#include "OrbitClientServices/ProcessManager.h"
#include "OverlayWidget.h"
#include "Path.h"
#include "ProcessItemModel.h"
#include "TargetConfiguration.h"
#include "absl/flags/flag.h"
#include "grpc/impl/codegen/grpc_types.h"
#include "process.pb.h"

ABSL_DECLARE_FLAG(bool, local);

namespace {
const int kProcessesRowHeight = 19;
const int kLocalTryConnectTimeoutMs = 1000;
}  // namespace

namespace orbit_qt {

using orbit_grpc_protos::ProcessInfo;

ProfilingTargetDialog::ProfilingTargetDialog(SshConnectionArtifacts* ssh_connection_artifacts,
                                             MainThreadExecutor* main_thread_executor,
                                             QWidget* parent)
    : QDialog(parent),
      ui_(std::make_unique<Ui::ProfilingTargetDialog>()),
      main_thread_executor_(main_thread_executor),
      local_grpc_port_(ssh_connection_artifacts->GetGrpcPort().grpc_port) {
  CHECK(main_thread_executor_ != nullptr);

  setWindowFlags(Qt::Window);

  ui_->setupUi(this);

  ui_->stadiaWidget->SetSshArtifacts(ssh_connection_artifacts);

  SetupStateMachine();

  process_proxy_model_.setSourceModel(&process_model_);
  process_proxy_model_.setSortRole(Qt::EditRole);
  process_proxy_model_.setFilterCaseSensitivity(Qt::CaseInsensitive);
  ui_->processesTableView->setModel(&process_proxy_model_);
  ui_->processesTableView->setSortingEnabled(true);
  ui_->processesTableView->sortByColumn(static_cast<int>(ProcessItemModel::Column::kCpu),
                                        Qt::DescendingOrder);

  ui_->processesTableView->horizontalHeader()->resizeSection(
      static_cast<int>(ProcessItemModel::Column::kPid), 60);
  ui_->processesTableView->horizontalHeader()->resizeSection(
      static_cast<int>(ProcessItemModel::Column::kCpu), 60);
  ui_->processesTableView->horizontalHeader()->setSectionResizeMode(
      static_cast<int>(ProcessItemModel::Column::kName), QHeaderView::Stretch);
  ui_->processesTableView->verticalHeader()->setDefaultSectionSize(kProcessesRowHeight);
  ui_->processesTableView->verticalHeader()->setVisible(false);

  if (absl::GetFlag(FLAGS_local)) {
    ui_->localFrame->setVisible(true);
  }

  QObject::connect(ui_->loadFromFileButton, &QPushButton::clicked, this,
                   &ProfilingTargetDialog::SelectFile);
  QObject::connect(ui_->loadCaptureRadioButton, &QRadioButton::clicked, this, [this](bool checked) {
    if (!checked) ui_->loadCaptureRadioButton->setChecked(true);
  });
  QObject::connect(ui_->localProfilingRadioButton, &QRadioButton::clicked, this,
                   [this](bool checked) {
                     if (!checked) ui_->localProfilingRadioButton->setChecked(true);
                   });
  QObject::connect(ui_->processesTableView->selectionModel(), &QItemSelectionModel::currentChanged,
                   this, &ProfilingTargetDialog::ProcessSelectionChanged);
  QObject::connect(ui_->processesTableView, &QTableView::doubleClicked, this, &QDialog::accept);
  QObject::connect(ui_->confirmButton, &QPushButton::clicked, this, &QDialog::accept);
  QObject::connect(ui_->processFilterLineEdit, &QLineEdit::textChanged, &process_proxy_model_,
                   &QSortFilterProxyModel::setFilterFixedString);
}

std::optional<ConnectionConfiguration> ProfilingTargetDialog::Exec(
    std::optional<ConnectionConfiguration> connection_configuration) {
  if (!connection_configuration.has_value()) {
    if (absl::GetFlag(FLAGS_local)) {
      state_machine_.setInitialState(s_local_);
    } else if (ui_->stadiaWidget->IsActive()) {
      state_machine_.setInitialState(s_stadia_);
    } else {
      state_machine_.setInitialState(s_capture_);
    }
  } else {
    std::visit(
        [this](auto&& target) {
          using Target = std::decay_t<decltype(target)>;
          if constexpr (std::is_same_v<Target, StadiaProfilingTarget>) {
            ui_->stadiaWidget->SetConnection(std::move(target.connection_));
            stadia_process_manager_ = std::move(target.process_manager_);
            process_ = std::move(target.process_);
            stadia_process_manager_->SetProcessListUpdateListener(
                [this](ProcessManager* process_manager) { OnProcessListUpdate(process_manager); });

            s_stadia_->setInitialState(s_s_connected_);
            s_s_history_->setDefaultState(s_s_connected_);
            state_machine_.setInitialState(s_stadia_);
          } else if constexpr (std::is_same_v<Target, LocalTarget>) {
            local_process_manager_ = std::move(target.process_manager_);
            process_ = std::move(target.process_);
            local_process_manager_->SetProcessListUpdateListener(
                [this](ProcessManager* process_manager) { OnProcessListUpdate(process_manager); });

            s_local_->setInitialState(s_l_connected_);
            s_l_history_->setDefaultState(s_l_connected_);
            state_machine_.setInitialState(s_local_);
          } else if constexpr (std::is_same_v<Target, FileTarget>) {
            selected_file_path_ = target.capture_file_path_;
            s_capture_->setInitialState(s_c_file_selected_);
            s_c_history_->setDefaultState(s_c_file_selected_);
            state_machine_.setInitialState(s_capture_);
            // TOOD(antonrohr) also start stadiaWidget here
            LOG("started state machine with s_s_file_selected_");
          } else {
            UNREACHABLE();
          }
        },
        connection_configuration.value());
  }

  ui_->stadiaWidget->Start();

  state_machine_.start();
  int rc = exec();
  state_machine_.stop();

  if (rc != 1) {
    // User closed dialog
    return std::nullopt;
  }

  if (stadia_process_manager_ != nullptr) {
    stadia_process_manager_->SetProcessListUpdateListener(nullptr);
  }
  if (local_process_manager_ != nullptr) {
    local_process_manager_->SetProcessListUpdateListener(nullptr);
  }

  switch (current_target_) {
    case ResultTarget::kStadia:
      return StadiaProfilingTarget(ui_->stadiaWidget->StopAndClearConnection().value(),
                                   std::move(stadia_process_manager_), std::move(process_));
    case ResultTarget::kLocal:
      return LocalTarget(LocalConnection(std::move(local_grpc_channel_)),
                         std::move(local_process_manager_), std::move(process_));
    case ResultTarget::kFile:
      return FileTarget(selected_file_path_);
  }
}

void ProfilingTargetDialog::ProcessSelectionChanged(const QModelIndex& current) {
  if (!current.isValid()) {
    process_ = nullptr;
    emit NoProcessSelected();
    return;
  }

  CHECK(current.model() == &process_proxy_model_);
  process_ = std::make_unique<ProcessData>(*current.data(Qt::UserRole).value<const ProcessInfo*>());
  emit ProcessSelected();
}

void ProfilingTargetDialog::SetupStateMachine() {
  state_machine_.setGlobalRestorePolicy(QStateMachine::RestoreProperties);

  // STATES list
  // STATE s_stadia
  s_stadia_ = new QState();
  state_machine_.addState(s_stadia_);
  s_s_history_ = new QHistoryState(s_stadia_);

  // s_stadia Child States
  auto s_s_connecting = new QState(s_stadia_);
  s_s_connected_ = new QState(s_stadia_);
  auto s_s_processes_loaded = new QState(s_stadia_);
  auto s_s_process_selected = new QState(s_stadia_);
  s_stadia_->setInitialState(s_s_connecting);
  s_s_history_->setDefaultState(s_s_connecting);

  // STATE s_capture
  s_capture_ = new QState();
  state_machine_.addState(s_capture_);
  s_c_history_ = new QHistoryState(s_capture_);

  // s_capture Child States
  auto s_c_no_file_selected = new QState(s_capture_);
  s_c_file_selected_ = new QState(s_capture_);
  s_capture_->setInitialState(s_c_no_file_selected);
  s_c_history_->setDefaultState(s_c_no_file_selected);

  // STATE s_local
  s_local_ = new QState();
  state_machine_.addState(s_local_);
  s_l_history_ = new QHistoryState(s_local_);

  // s_local Child States
  auto s_l_connecting = new QState(s_local_);
  s_l_connected_ = new QState(s_local_);
  auto s_l_processes_loaded = new QState(s_local_);
  auto s_l_process_selected = new QState(s_local_);
  s_local_->setInitialState(s_l_connecting);
  s_l_history_->setDefaultState(s_l_connecting);

  // PROPERTIES of states
  // STATE s_stadia
  s_stadia_->assignProperty(ui_->confirmButton, "text", "Confirm Process");
  s_stadia_->assignProperty(ui_->confirmButton, "enabled", false);
  s_stadia_->assignProperty(ui_->confirmButton, "toolTip",
                            "Please connect to an instance and select a process.");
  s_stadia_->assignProperty(ui_->stadiaWidget, "active", true);
  s_stadia_->assignProperty(ui_->loadCaptureRadioButton, "checked", false);
  s_stadia_->assignProperty(ui_->localProfilingRadioButton, "checked", false);

  // STATE s_capture
  s_capture_->assignProperty(ui_->confirmButton, "text", "Load Capture");
  s_capture_->assignProperty(ui_->confirmButton, "enabled", false);
  s_capture_->assignProperty(ui_->confirmButton, "toolTip", "Please select a capture to load");
  s_capture_->assignProperty(ui_->stadiaWidget, "active", false);
  s_capture_->assignProperty(ui_->loadCaptureRadioButton, "checked", true);
  s_capture_->assignProperty(ui_->processesFrame, "enabled", false);
  s_capture_->assignProperty(ui_->loadFromFileButton, "enabled", true);
  s_capture_->assignProperty(ui_->localProfilingRadioButton, "checked", false);

  // STATE s_local
  s_local_->assignProperty(ui_->confirmButton, "text", "Confirm Process");
  s_local_->assignProperty(ui_->confirmButton, "enabled", false);
  s_local_->assignProperty(
      ui_->confirmButton, "toolTip",
      "Please have a OrbitService run on the local machine and select a process.");
  s_local_->assignProperty(ui_->localProfilingRadioButton, "checked", true);
  s_local_->assignProperty(ui_->stadiaWidget, "active", false);

  // STATE s_s_connecting
  s_s_connecting->assignProperty(ui_->processesFrame, "enabled", false);

  // STATE s_s_connected
  s_s_connected_->assignProperty(ui_->processesTableOverlay, "visible", true);
  s_s_connected_->assignProperty(ui_->processesTableOverlay, "cancelable", false);
  s_s_connected_->assignProperty(ui_->processesTableOverlay, "statusMessage",
                                 "Loading processes...");

  // STATE s_s_process_selected
  s_s_process_selected->assignProperty(ui_->confirmButton, "enabled", true);

  // STATE s_c_file_selected
  s_c_file_selected_->assignProperty(ui_->confirmButton, "enabled", true);

  // STATE s_l_connecting
  s_l_connecting->assignProperty(ui_->localProfilingStatusMessage, "text", "Connecting...");

  // STATE s_l_connected
  s_l_connected_->assignProperty(ui_->localProfilingStatusMessage, "text", "Connected");
  s_l_connected_->assignProperty(ui_->processesTableOverlay, "visible", true);
  s_l_connected_->assignProperty(ui_->processesTableOverlay, "cancelable", false);
  s_l_connected_->assignProperty(ui_->processesTableOverlay, "statusMessage",
                                 "Loading processes...");

  // STATE s_l_processes_loaded
  s_l_processes_loaded->assignProperty(ui_->localProfilingStatusMessage, "text", "Connected");

  // STATE s_l_process_selected
  s_l_process_selected->assignProperty(ui_->localProfilingStatusMessage, "text", "Connected");
  s_l_process_selected->assignProperty(ui_->confirmButton, "enabled", true);

  // TRANSITIONS (and entered/exit events)
  // STATE s_stadia
  s_stadia_->addTransition(ui_->loadCaptureRadioButton, &QRadioButton::clicked, s_c_history_);
  s_stadia_->addTransition(ui_->localProfilingRadioButton, &QRadioButton::clicked, s_l_history_);
  s_stadia_->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Disconnected, s_s_connecting);
  QObject::connect(s_stadia_, &QState::entered,
                   [this]() { current_target_ = ResultTarget::kStadia; });

  // STATE s_capture
  s_capture_->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Activated, s_s_history_);
  s_capture_->addTransition(ui_->localProfilingRadioButton, &QRadioButton::clicked, s_l_history_);
  s_capture_->addTransition(this, &ProfilingTargetDialog::FileSelected, s_c_file_selected_);
  QObject::connect(s_capture_, &QState::entered,
                   [this]() { current_target_ = ResultTarget::kFile; });

  // STATE s_local
  s_local_->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Activated, s_s_history_);
  s_local_->addTransition(ui_->loadCaptureRadioButton, &QRadioButton::clicked, s_c_history_);
  QObject::connect(s_local_, &QState::entered, [this] { current_target_ = ResultTarget::kLocal; });

  // STATE s_s_connecting
  s_s_connecting->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Connected,
                                s_s_connected_);
  s_s_connecting->addTransition(this, &ProfilingTargetDialog::StadiaIsConnected, s_s_connected_);
  QObject::connect(s_s_connecting, &QState::entered, this, [this]() {
    ResetStadiaProcessManager();
    if (ui_->stadiaWidget->GetGrpcChannel() != nullptr) {
      emit StadiaIsConnected();
    }
  });

  // STATE s_s_connected
  s_s_connected_->addTransition(this, &ProfilingTargetDialog::ProcessSelected,
                                s_s_process_selected);
  s_s_connected_->addTransition(this, &ProfilingTargetDialog::NoProcessSelected,
                                s_s_processes_loaded);
  QObject::connect(s_s_connected_, &QState::entered, this,
                   &ProfilingTargetDialog::LoadStadiaProcesses);

  // STATE s_s_processes_loaded
  s_s_processes_loaded->addTransition(this, &ProfilingTargetDialog::ProcessSelected,
                                      s_s_process_selected);

  // STATE s_s_process_selected
  s_s_process_selected->addTransition(this, &ProfilingTargetDialog::NoProcessSelected,
                                      s_s_processes_loaded);

  // STATE s_c_no_file_selected
  QObject::connect(s_c_no_file_selected, &QState::entered, [this] {
    if (selected_file_path_.empty()) SelectFile();
  });

  // STATE s_c_file_selected
  QObject::connect(s_c_file_selected_, &QState::entered, [this] {
    ui_->selectedFileLabel->setText(
        QString::fromStdString(selected_file_path_.filename().string()));
  });

  // STATE s_l_connecting
  s_l_connecting->addTransition(this, &ProfilingTargetDialog::LocalIsConnected, s_l_connected_);
  QObject::connect(s_l_connecting, &QState::entered, this, [this]() {
    ResetLocalProcessManager();
    ConnectToLocal();
  });

  // STATE s_l_connected
  s_l_connected_->addTransition(this, &ProfilingTargetDialog::ProcessSelected,
                                s_l_process_selected);
  s_l_connected_->addTransition(this, &ProfilingTargetDialog::NoProcessSelected,
                                s_l_processes_loaded);
  QObject::connect(s_l_connected_, &QState::entered, this, &ProfilingTargetDialog::ConnectToLocal);

  // STATE s_l_processes_loaded
  s_l_processes_loaded->addTransition(this, &ProfilingTargetDialog::ProcessSelected,
                                      s_l_process_selected);

  // STATE s_l_process_selected
  s_l_process_selected->addTransition(this, &ProfilingTargetDialog::NoProcessSelected,
                                      s_l_processes_loaded);

  // -- Set initial state

  if (ui_->stadiaWidget->IsActive()) {
    state_machine_.setInitialState(s_stadia_);
  } else {
    state_machine_.setInitialState(s_capture_);
  }
}

void ProfilingTargetDialog::ResetStadiaProcessManager() {
  process_model_.Clear();
  process_ = nullptr;

  if (stadia_process_manager_ != nullptr) {
    stadia_process_manager_->Shutdown();
    stadia_process_manager_ = nullptr;
  }
}

void ProfilingTargetDialog::ResetLocalProcessManager() {
  process_model_.Clear();
  process_ = nullptr;

  if (local_process_manager_ != nullptr) {
    local_process_manager_->Shutdown();
    local_process_manager_ = nullptr;
  }
}

void ProfilingTargetDialog::LoadStadiaProcesses() {
  process_model_.Clear();
  const std::shared_ptr<grpc::Channel>& grpc_channel = ui_->stadiaWidget->GetGrpcChannel();
  CHECK(grpc_channel != nullptr);

  if (stadia_process_manager_ != nullptr) {
    if (ui_->processesTableView->selectionModel()->hasSelection()) {
      emit ProcessSelected();
    }
    return;
  }

  stadia_process_manager_ = ProcessManager::Create(grpc_channel, absl::Milliseconds(1000));
  stadia_process_manager_->SetProcessListUpdateListener(
      [this](ProcessManager* process_manager) { OnProcessListUpdate(process_manager); });
}

void ProfilingTargetDialog::SelectFile() {
  const QString file = QFileDialog::getOpenFileName(
      this, "Open Capture...", QString::fromStdString(Path::CreateOrGetCaptureDir()), "*.orbit");
  if (!file.isEmpty()) {
    selected_file_path_ = std::filesystem::path(file.toStdString());

    emit FileSelected();
  }
}

void ProfilingTargetDialog::TrySelectProcess(const ProcessData& process) {
  QModelIndexList matches = process_proxy_model_.match(
      process_proxy_model_.index(0, static_cast<int>(ProcessItemModel::Column::kName)),
      Qt::DisplayRole, QVariant::fromValue(QString::fromStdString(process.name())));

  if (matches.isEmpty()) return;

  ui_->processesTableView->selectionModel()->setCurrentIndex(
      matches[0], {QItemSelectionModel::SelectCurrent, QItemSelectionModel::Rows});
}

void ProfilingTargetDialog::OnProcessListUpdate(ProcessManager* process_manager) {
  main_thread_executor_->Schedule([this, process_manager]() {
    bool had_processes_before = process_model_.HasProcesses();
    process_model_.SetProcesses(process_manager->GetProcessList());
    if (!had_processes_before) return;

    if (ui_->processesTableView->selectionModel()->hasSelection()) return;

    if (process_ != nullptr) {
      TrySelectProcess(*process_.get());
    }

    if (ui_->processesTableView->selectionModel()->hasSelection()) return;

    ui_->processesTableView->selectRow(0);
  });
}

void ProfilingTargetDialog::ConnectToLocal() {
  process_model_.Clear();
  if (local_grpc_channel_ == nullptr) {
    local_grpc_channel_ =
        grpc::CreateCustomChannel(absl::StrFormat("127.0.0.1:%d", local_grpc_port_),
                                  grpc::InsecureChannelCredentials(), grpc::ChannelArguments());
  }

  if (local_grpc_channel_->GetState(true) != GRPC_CHANNEL_READY) {
    LOG("Local grpc connection not ready, Trying to connect to local again in %d ms.",
        kLocalTryConnectTimeoutMs);
    QTimer::singleShot(kLocalTryConnectTimeoutMs, this, &ProfilingTargetDialog::ConnectToLocal);
    return;
  }

  emit LocalIsConnected();

  process_model_.Clear();

  if (local_process_manager_ != nullptr) {
    if (ui_->processesTableView->selectionModel()->hasSelection()) {
      emit ProcessSelected();
    }
    return;
  }

  local_process_manager_ = ProcessManager::Create(local_grpc_channel_, absl::Milliseconds(1000));
  local_process_manager_->SetProcessListUpdateListener(
      [this](ProcessManager* process_manager) { OnProcessListUpdate(process_manager); });
}

}  // namespace orbit_qt