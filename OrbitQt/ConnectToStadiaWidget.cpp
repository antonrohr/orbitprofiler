// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ConnectToStadiaWidget.h"

#include <QMessageBox>
#include <QRadioButton>
#include <QSettings>
#include <optional>

#include "Error.h"
#include "OrbitBase/Logging.h"
#include "OrbitBase/Result.h"
#include "OrbitGgp/Client.h"
#include "OverlayWidget.h"
#include "servicedeploymanager.h"

namespace {
const QString kRememberChosenInstance{"RememberChosenInstance"};
}  // namespace

namespace orbit_qt {

using OrbitGgp::Instance;
using OrbitSshQt::ScopedConnection;

ConnectToStadiaWidget::ConnectToStadiaWidget(QWidget* parent)
    : QWidget(parent), ui_(std::make_unique<Ui::ConnectToStadiaWidget>()) {
  ui_->setupUi(this);

  parent->installEventFilter(this);

  QSettings settings;
  if (!settings.value(kRememberChosenInstance).toString().isEmpty()) {
    remembered_instance_id_ = settings.value(kRememberChosenInstance).toString();
    ui_->rememberCheckBox->setChecked(true);
  }

  ui_->instancesTableView->setModel(&instance_model_);

  QObject::connect(ui_->connectToStadiaInstanceRadioButton, &QRadioButton::clicked, this,
                   [this](bool checked) {
                     if (checked) {
                       emit Activated();
                     } else {
                       ui_->connectToStadiaInstanceRadioButton->setChecked(true);
                     }
                   });

  QObject::connect(this, &ConnectToStadiaWidget::ErrorOccurred, this,
                   [this](const QString& message) {
                     if (isVisible()) {
                       QMessageBox::critical(this, QApplication::applicationName(), message);
                     } else {
                       ERROR("%s", message.toStdString());
                     }
                   });

  QObject::connect(ui_->instancesTableView->selectionModel(), &QItemSelectionModel::currentChanged,
                   this, [this](const QModelIndex& current) {
                     if (!current.isValid()) return;

                     CHECK(current.model() == &instance_model_);
                     stadia_connection_->instance = current.data(Qt::UserRole).value<Instance>();
                     emit InstanceSelected();
                   });

  QObject::connect(ui_->rememberCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
    QSettings settings;
    if (checked) {
      CHECK(stadia_connection_->instance != std::nullopt);
      settings.setValue(kRememberChosenInstance, stadia_connection_->instance->id);
    } else {
      settings.remove(kRememberChosenInstance);
    }
  });

  SetupAndStartStateMachine();
}

bool ConnectToStadiaWidget::eventFilter(QObject* obj, QEvent* event) {
  if (obj == parent() && event->type() == QEvent::Resize) {
    ui_->contentFrame->resize(size());
  }
  return false;
}

void ConnectToStadiaWidget::SetActive(bool value) {
  ui_->contentFrame->setEnabled(value);
  ui_->connectToStadiaInstanceRadioButton->setChecked(value);
}

void ConnectToStadiaWidget::SetStadiaConnection(StadiaConnection* stadia_connection) {
  CHECK(stadia_connection != nullptr);
  stadia_connection_ = stadia_connection;
}

void ConnectToStadiaWidget::SetupAndStartStateMachine() {
  state_machine_.setGlobalRestorePolicy(QStateMachine::RestoreProperties);

  // STATES list
  auto* s_ggp_not_available = new QState();
  state_machine_.addState(s_ggp_not_available);
  auto* s_instances_empty = new QState();
  state_machine_.addState(s_instances_empty);
  auto* s_instances_loading = new QState();
  state_machine_.addState(s_instances_loading);
  auto* s_instances_loaded = new QState();
  state_machine_.addState(s_instances_loaded);
  auto* s_instance_selected = new QState();
  state_machine_.addState(s_instance_selected);
  auto* s_waiting_for_creds = new QState();
  state_machine_.addState(s_waiting_for_creds);
  auto* s_deploying = new QState();
  state_machine_.addState(s_deploying);
  auto* s_connected = new QState();
  state_machine_.addState(s_connected);

  // PROPERTIES of states
  // STATE s_ggp_not_available
  s_ggp_not_available->assignProperty(this, "enabled", false);
  s_ggp_not_available->assignProperty(ui_->connectToStadiaInstanceRadioButton, "checked", false);
  // STATE s_instances_empty
  s_instances_empty->assignProperty(ui_->refreshButton, "enabled", true);
  // STATE s_instances_loading
  s_instances_loading->assignProperty(ui_->instancesTableOverlay, "visible", true);
  s_instances_loading->assignProperty(ui_->instancesTableOverlay, "statusMessage",
                                      "Loading instances...");
  s_instances_loading->assignProperty(ui_->instancesTableOverlay, "cancelable", false);
  // STATE s_instances_loaded
  s_instances_loaded->assignProperty(ui_->refreshButton, "enabled", true);
  // STATE s_instance_selected
  s_instance_selected->assignProperty(ui_->refreshButton, "enabled", true);
  s_instance_selected->assignProperty(ui_->connectButton, "enabled", true);
  // STATE s_waiting_for_creds
  s_waiting_for_creds->assignProperty(ui_->instancesTableOverlay, "visible", true);
  s_waiting_for_creds->assignProperty(ui_->instancesTableOverlay, "statusMessage",
                                      "Loading encryption credentials for instance...");
  s_waiting_for_creds->assignProperty(ui_->instancesTableOverlay, "cancelable", true);
  // STATE s_deploying
  s_deploying->assignProperty(ui_->instancesTableOverlay, "visible", true);
  s_deploying->assignProperty(ui_->instancesTableOverlay, "cancelable", true);
  // STATE s_connected
  s_connected->assignProperty(ui_->instancesTableOverlay, "visible", true);
  s_connected->assignProperty(ui_->instancesTableOverlay, "spinning", false);
  s_connected->assignProperty(ui_->instancesTableOverlay, "cancelable", true);
  s_connected->assignProperty(ui_->instancesTableOverlay, "buttonMessage", "Disconnect");
  s_connected->assignProperty(ui_->rememberCheckBox, "enabled", true);

  // STATE s_ggp_not_available

  // TRANSITIONS (and entered/xit events)
  // STATE s_instances_empty
  s_instances_empty->addTransition(ui_->refreshButton, &QPushButton::clicked, s_instances_loading);

  // STATE s_instances_loading
  QObject::connect(s_instances_loading, &QState::entered, this,
                   &ConnectToStadiaWidget::ReloadInstances);

  s_instances_loading->addTransition(this, &ConnectToStadiaWidget::ErrorOccurred,
                                     s_instances_empty);
  s_instances_loading->addTransition(this, &ConnectToStadiaWidget::ReceivedInstances,
                                     s_instances_loaded);

  // STATE s_instances_loaded
  s_instances_loaded->addTransition(ui_->refreshButton, &QPushButton::clicked, s_instances_loading);
  s_instances_loaded->addTransition(this, &ConnectToStadiaWidget::InstanceSelected,
                                    s_instance_selected);
  // STATE s_instance_selected
  s_instance_selected->addTransition(ui_->refreshButton, &QPushButton::clicked,
                                     s_instances_loading);
  s_instance_selected->addTransition(ui_->connectButton, &QPushButton::clicked,
                                     s_waiting_for_creds);
  s_instance_selected->addTransition(ui_->instancesTableView, &QTableView::doubleClicked,
                                     s_waiting_for_creds);
  s_instance_selected->addTransition(this, &ConnectToStadiaWidget::Connect, s_waiting_for_creds);

  // STATE s_waiting_for_creds
  QObject::connect(s_waiting_for_creds, &QState::entered, this,
                   &ConnectToStadiaWidget::CheckCredentialsAvailable);

  s_waiting_for_creds->addTransition(this, &ConnectToStadiaWidget::ReceivedSshInfo,
                                     s_waiting_for_creds);
  s_waiting_for_creds->addTransition(this, &ConnectToStadiaWidget::ReadyToDeploy, s_deploying);
  s_waiting_for_creds->addTransition(ui_->instancesTableOverlay, &OverlayWidget::Cancelled,
                                     s_instance_selected);
  s_waiting_for_creds->addTransition(this, &ConnectToStadiaWidget::ErrorOccurred,
                                     s_instances_loading);

  // STATE s_deploying
  QObject::connect(s_deploying, &QState::entered, this, &ConnectToStadiaWidget::DeployOrbitService);

  s_deploying->addTransition(this, &ConnectToStadiaWidget::ErrorOccurred, s_instance_selected);
  s_deploying->addTransition(ui_->instancesTableOverlay, &OverlayWidget::Cancelled,
                             s_instance_selected);
  s_deploying->addTransition(this, &ConnectToStadiaWidget::Connected, s_connected);

  // STATE s_connected
  QObject::connect(s_connected, &QState::entered, this, [this] {
    ui_->instancesTableOverlay->SetStatusMessage(
        QString{"Connected to %1"}.arg(stadia_connection_->instance->display_name));
  });
  QObject::connect(s_connected, &QState::exited, this, &ConnectToStadiaWidget::Disconnect);

  s_connected->addTransition(ui_->instancesTableOverlay, &OverlayWidget::Cancelled,
                             s_instance_selected);
  s_connected->addTransition(this, &ConnectToStadiaWidget::ErrorOccurred, s_instance_selected);

  // START the state machine
  auto client_result = OrbitGgp::Client::Create(this);
  if (!client_result) {
    ui_->connectToStadiaInstanceRadioButton->setToolTip(
        QString::fromStdString(client_result.error().message()));
    state_machine_.setInitialState(s_ggp_not_available);
    SetActive(false);
  } else {
    ggp_client_ = client_result.value();
    state_machine_.setInitialState(s_instances_loading);
    SetActive(true);
  }

  state_machine_.start();
}

void ConnectToStadiaWidget::ReloadInstances() {
  CHECK(ggp_client_ != nullptr);
  instance_model_.SetInstances({});

  ggp_client_->GetInstancesAsync([this](outcome::result<QVector<Instance>> instances) {
    if (!instances) {
      emit ErrorOccurred(QString("Orbit was unable to retrieve the list of available Stadia "
                                 "instances. The error message was: %1")
                             .arg(QString::fromStdString(instances.error().message())));
      return;
    }

    instance_model_.SetInstances(instances.value());
    emit ReceivedInstances();

    for (const auto& instance : instances.value()) {
      std::string instance_id = instance.id.toStdString();

      if (remembered_instance_id_ != std::nullopt) {
        std::optional<int> row =
            instance_model_.GetRowOfInstanceById(remembered_instance_id_.value());
        if (row != std::nullopt) {
          ui_->instancesTableView->selectRow(row.value());
          emit Connect();
          remembered_instance_id_ = std::nullopt;
        } else {
          ui_->rememberCheckBox->setChecked(false);
        }
      }

      if (instance_credentials_.contains(instance_id) &&
          instance_credentials_.at(instance_id).has_value()) {
        continue;
      }

      ggp_client_->GetSshInfoAsync(
          instance, [this, instance_id = std::move(instance_id)](
                        outcome::result<OrbitGgp::SshInfo> ssh_info_result) {
            if (!ssh_info_result) {
              std::string error_message = absl::StrFormat(
                  "Unable to load encryption credentials for instance with id %s: %s", instance_id,
                  ssh_info_result.error().message());
              ERROR("%s", error_message);
              instance_credentials_.emplace(instance_id, ErrorMessage(error_message));
            } else {
              LOG("Received ssh info for instance with id: %s", instance_id);

              OrbitGgp::SshInfo& ssh_info{ssh_info_result.value()};
              OrbitSsh::Credentials credentials;
              credentials.addr_and_port = {ssh_info.host.toStdString(), ssh_info.port};
              credentials.key_path = ssh_info.key_path.toStdString();
              credentials.known_hosts_path = ssh_info.known_hosts_path.toStdString();
              credentials.user = ssh_info.user.toStdString();

              instance_credentials_.emplace(instance_id, std::move(credentials));
            }

            emit ReceivedSshInfo();
          });
    }
  });
}

void ConnectToStadiaWidget::CheckCredentialsAvailable() {
  CHECK(stadia_connection_->instance);

  const std::string& instance_id = stadia_connection_->instance->id.toStdString();

  if (!instance_credentials_.contains(instance_id)) return;

  if (instance_credentials_.at(instance_id).has_error()) {
    emit ErrorOccurred(
        QString::fromStdString(instance_credentials_.at(instance_id).error().message()));
    return;
  }

  emit ReadyToDeploy();
}

void ConnectToStadiaWidget::DeployOrbitService() {
  CHECK(stadia_connection_->service_deploy_manager == nullptr);
  CHECK(stadia_connection_->instance);
  const std::string instance_id = stadia_connection_->instance->id.toStdString();
  CHECK(instance_credentials_.contains(instance_id));
  CHECK(instance_credentials_.at(instance_id).has_value());

  const OrbitSsh::Credentials& credentials{instance_credentials_.at(instance_id).value()};

  stadia_connection_->CreateServiceDeployManager(credentials);

  ScopedConnection label_connection{
      QObject::connect(stadia_connection_->service_deploy_manager.get(),
                       &orbit_qt::ServiceDeployManager::statusMessage, ui_->instancesTableOverlay,
                       &OverlayWidget::SetStatusMessage)};
  ScopedConnection cancel_connection{QObject::connect(
      ui_->instancesTableOverlay, &OverlayWidget::Cancelled,
      stadia_connection_->service_deploy_manager.get(), &orbit_qt::ServiceDeployManager::Cancel)};

  const auto deployment_result = stadia_connection_->service_deploy_manager->Exec();
  if (!deployment_result) {
    Disconnect();
    if (deployment_result.error() ==
        make_error_code(orbit_qt::Error::kUserCanceledServiceDeployment)) {
      return;
    }
    emit ErrorOccurred(QString("Orbit was unable to successfully connect to the Instance. The "
                               "error message was: %1")
                           .arg(QString::fromStdString(deployment_result.error().message())));
    return;
  }

  QObject::connect(
      stadia_connection_->service_deploy_manager.get(), &ServiceDeployManager::socketErrorOccurred,
      this, [this](std::error_code error) {
        emit ErrorOccurred(QString("The connection to instance %1 failed with error: %2")
                               .arg(stadia_connection_->instance->display_name)
                               .arg(QString::fromStdString(error.message())));
      });

  LOG("Deployment successful, grpc_port: %d", deployment_result.value().grpc_port);
  CHECK(stadia_connection_->grpc_channel == nullptr);
  std::string grpc_server_address =
      absl::StrFormat("127.0.0.1:%d", deployment_result.value().grpc_port);
  LOG("Starting gRPC channel to: %s", grpc_server_address);
  stadia_connection_->grpc_channel = grpc::CreateCustomChannel(
      grpc_server_address, grpc::InsecureChannelCredentials(), grpc::ChannelArguments());
  CHECK(stadia_connection_->grpc_channel != nullptr);

  emit Connected();
}

void ConnectToStadiaWidget::Disconnect() {
  stadia_connection_->grpc_channel = nullptr;

  // TODO(antonrohr) currently does not work
  // if (stadia_connection_->service_deploy_manager != nullptr) {
  //   stadia_connection_->service_deploy_manager->Shutdown();
  // }
  stadia_connection_->service_deploy_manager = nullptr;

  ui_->rememberCheckBox->setChecked(false);

  emit Disconnected();
}

}  // namespace orbit_qt