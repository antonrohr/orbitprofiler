// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ProfilingTargetDialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHistoryState>
#include <QItemSelectionModel>
#include <QRadioButton>
#include <QSortFilterProxyModel>
#include <QWidget>
#include <memory>
#include <optional>
#include <type_traits>
#include <variant>

#include "ConnectToStadiaWidget.h"
#include "ConnectionConfiguration.h"
#include "DeploymentConfigurations.h"
#include "MainThreadExecutor.h"
#include "OrbitBase/Logging.h"
#include "OrbitClientServices/ProcessManager.h"
#include "OverlayWidget.h"
#include "Path.h"
#include "ProcessItemModel.h"
#include "process.pb.h"

namespace {
const int kProcessesRowHeight = 19;
}

namespace orbit_qt {

using orbit_grpc_protos::ProcessInfo;

ProfilingTargetDialog::ProfilingTargetDialog(
    ConnectionConfiguration* connection_configuration, const OrbitSsh::Context* ssh_context,
    const ServiceDeployManager::GrpcPort& grpc_port,
    const DeploymentConfiguration* deployment_configuration,
    MainThreadExecutor* main_thread_executor, QWidget* parent)
    : QDialog(parent),
      ui_(std::make_unique<Ui::ProfilingTargetDialog>()),
      main_thread_executor_(main_thread_executor),
      stadia_connection_(ssh_context, grpc_port, deployment_configuration),
      connection_configuration_(connection_configuration) {
  CHECK(main_thread_executor_ != nullptr);
  CHECK(connection_configuration_ != nullptr);

  ui_->setupUi(this);

  ui_->stadiaWidget->SetStadiaConnection(&stadia_connection_);

  SetupStateMachine();

  process_proxy_model_.setSourceModel(&process_model_);
  process_proxy_model_.setSortRole(Qt::EditRole);
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

  QObject::connect(ui_->loadFromFileButton, &QPushButton::clicked, this,
                   &ProfilingTargetDialog::SelectFile);
  QObject::connect(ui_->loadCaptureRadioButton, &QRadioButton::clicked, this, [this](bool checked) {
    if (!checked) ui_->loadCaptureRadioButton->setChecked(true);
  });
  QObject::connect(ui_->processesTableView->selectionModel(), &QItemSelectionModel::currentChanged,
                   this, &ProfilingTargetDialog::ProcessSelectionChanged);
  QObject::connect(ui_->processesTableView, &QTableView::doubleClicked, this, &QDialog::accept);
  QObject::connect(ui_->confirmButton, &QPushButton::clicked, this, &QDialog::accept);
  QObject::connect(ui_->processFilterLineEdit, &QLineEdit::textChanged, &process_proxy_model_,
                   &QSortFilterProxyModel::setFilterFixedString);
}

void ProfilingTargetDialog::ProcessSelectionChanged(const QModelIndex& current) {
  std::unique_ptr<ProcessData> process;

  if (current.isValid()) {
    CHECK(current.model() == &process_proxy_model_);
    process =
        std::make_unique<ProcessData>(*current.data(Qt::UserRole).value<const ProcessInfo*>());
  }

  std::visit(
      [this, process = std::move(process)](auto&& connection) mutable {
        using Connection = std::decay_t<decltype(connection)>;
        if constexpr (std::is_same_v<Connection, const StadiaConnection*>) {
          stadia_connection_.process = std::move(process);
          if (stadia_connection_.process != nullptr) {
            emit ProcessSelected();
          } else {
            emit NoProcessSelected();
          }
        } else if constexpr (std::is_same_v<Connection, const LocalConnection*>) {
          // TODO(antonrohr) Add when local profiling is supported
          UNREACHABLE();
        } else if constexpr (std::is_same_v<Connection, const NoConnection*>) {
          UNREACHABLE();
        } else {
          UNREACHABLE();
        }
      },
      *connection_configuration_);
}

void ProfilingTargetDialog::SetupStateMachine() {
  state_machine_.setGlobalRestorePolicy(QStateMachine::RestoreProperties);

  // STATES list
  // STATE s_stadia
  auto* s_stadia = new QState();
  state_machine_.addState(s_stadia);
  auto* s_s_history = new QHistoryState(s_stadia);

  // s_stadia Child States
  auto* s_s_connecting = new QState(s_stadia);
  auto* s_s_connected = new QState(s_stadia);
  auto* s_s_processes_loaded = new QState(s_stadia);
  auto* s_s_process_selected = new QState(s_stadia);
  s_stadia->setInitialState(s_s_connecting);
  s_s_history->setDefaultState(s_s_connecting);

  // STATE s_capture
  auto* s_capture = new QState();
  state_machine_.addState(s_capture);
  auto* s_c_history = new QHistoryState(s_capture);

  // s_capture Child States
  auto* s_c_no_file_selected = new QState(s_capture);
  auto* s_c_file_selected = new QState(s_capture);
  s_capture->setInitialState(s_c_no_file_selected);
  s_c_history->setDefaultState(s_c_no_file_selected);

  // PROPERTIES of states
  // STATE s_stadia
  s_stadia->assignProperty(ui_->confirmButton, "text", "Confirm Process");
  s_stadia->assignProperty(ui_->confirmButton, "enabled", false);
  s_stadia->assignProperty(ui_->confirmButton, "toolTip",
                           "Please connect to an instance and select a process.");
  s_stadia->assignProperty(ui_->stadiaWidget, "active", true);
  s_stadia->assignProperty(ui_->loadCaptureRadioButton, "checked", false);
  QObject::connect(s_stadia, &QState::entered,
                   [this]() { *connection_configuration_ = &stadia_connection_; });

  // STATE s_capture
  s_capture->assignProperty(ui_->confirmButton, "text", "Load Capture");
  s_capture->assignProperty(ui_->stadiaWidget, "active", false);
  s_capture->assignProperty(ui_->loadCaptureRadioButton, "checked", true);
  s_capture->assignProperty(ui_->processesFrame, "enabled", false);
  s_capture->assignProperty(ui_->loadFromFileButton, "enabled", true);
  QObject::connect(s_capture, &QState::entered,
                   [this]() { *connection_configuration_ = &no_connection_; });

  // STATE s_s_connecting
  s_s_connecting->assignProperty(ui_->processesFrame, "enabled", false);

  // STATE s_s_connected
  s_s_connected->assignProperty(ui_->processesTableOverlay, "visible", true);
  s_s_connected->assignProperty(ui_->processesTableOverlay, "cancelable", false);
  s_s_connected->assignProperty(ui_->processesTableOverlay, "statusMessage",
                                "Loading processes...");

  // STATE s_s_process_selected
  s_s_process_selected->assignProperty(ui_->confirmButton, "enabled", true);

  // STATE s_c_no_file_selected
  s_c_no_file_selected->assignProperty(ui_->confirmButton, "enabled", false);
  s_c_no_file_selected->assignProperty(ui_->confirmButton, "toolTip",
                                       "Please select a capture to load");
  // STATE s_c_file_selected
  s_c_file_selected->assignProperty(ui_->confirmButton, "enabled", true);

  // TRANSITIONS (and entered/exit events)
  // STATE s_stadia
  s_stadia->addTransition(ui_->loadCaptureRadioButton, &QRadioButton::clicked, s_c_history);
  s_stadia->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Disconnected, s_s_connecting);

  // STATE s_capture
  s_capture->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Activated, s_s_history);
  s_capture->addTransition(this, &ProfilingTargetDialog::FileSelected, s_c_file_selected);

  // STATE s_s_connecting
  s_s_connecting->addTransition(ui_->stadiaWidget, &ConnectToStadiaWidget::Connected,
                                s_s_connected);
  s_s_connecting->addTransition(this, &ProfilingTargetDialog::StadiaWidgetIsConnected,
                                s_s_connected);
  QObject::connect(s_s_connecting, &QState::entered, this,
                   &ProfilingTargetDialog::ResetStadiaProcessManager);

  // STATE s_s_connected
  s_s_connected->addTransition(this, &ProfilingTargetDialog::ProcessSelected, s_s_process_selected);
  QObject::connect(s_s_connected, &QState::entered, this,
                   &ProfilingTargetDialog::LoadStadiaProcesses);

  // STATE s_s_processes_loaded
  s_s_processes_loaded->addTransition(this, &ProfilingTargetDialog::ProcessSelected,
                                      s_s_process_selected);

  // STATE s_s_process_selected
  s_s_process_selected->addTransition(this, &ProfilingTargetDialog::NoProcessSelected,
                                      s_s_processes_loaded);

  // STATE s_c_no_file_selected
  QObject::connect(s_c_no_file_selected, &QState::entered, [this] {
    if (no_connection_.capture_file_path.empty()) SelectFile();
  });

  // -- START state machine

  if (ui_->stadiaWidget->IsActive()) {
    state_machine_.setInitialState(s_stadia);
  } else {
    state_machine_.setInitialState(s_capture);
  }
  state_machine_.start();
}

void ProfilingTargetDialog::ResetStadiaProcessManager() {
  process_model_.Clear();
  stadia_connection_.process = nullptr;
  if (stadia_connection_.process_manager != nullptr) {
    stadia_connection_.process_manager->Shutdown();
    stadia_connection_.process_manager = nullptr;
  }
  if (stadia_connection_.grpc_channel != nullptr) {
    emit StadiaWidgetIsConnected();
  }
}

void ProfilingTargetDialog::LoadStadiaProcesses() {
  CHECK(stadia_connection_.grpc_channel != nullptr);

  if (stadia_connection_.process_manager != nullptr) {
    if (ui_->processesTableView->selectionModel()->hasSelection()) {
      emit ProcessSelected();
    }
    return;
  }

  stadia_connection_.process_manager =
      ProcessManager::Create(stadia_connection_.grpc_channel, absl::Milliseconds(1000));
  stadia_connection_.process_manager->SetProcessListUpdateListener(
      [this](ProcessManager* process_manager) {
        main_thread_executor_->Schedule([this, process_manager]() {
          bool had_processes_before = process_model_.HasProcesses();
          process_model_.SetProcesses(process_manager->GetProcessList());
          if (!had_processes_before) return;
          ui_->processesTableView->setEnabled(true);
          if (!ui_->processesTableView->selectionModel()->hasSelection()) {
            ui_->processesTableView->selectRow(0);
          }
        });
      });
}

void ProfilingTargetDialog::SelectFile() {
  const QString file = QFileDialog::getOpenFileName(
      this, "Open Capture...", QString::fromStdString(Path::CreateOrGetCaptureDir()), "*.orbit");
  if (!file.isEmpty()) {
    std::filesystem::path file_path = std::filesystem::path(file.toStdString());
    no_connection_.capture_file_path = file_path;

    ui_->chosen_file_label->setText(QString::fromStdString(file_path.filename().string()));
    emit FileSelected();
  }
}

}  // namespace orbit_qt