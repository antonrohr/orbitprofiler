// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_CONNECT_TO_STADIA_WIDGET_H_
#define ORBIT_QT_CONNECT_TO_STADIA_WIDGET_H_

#include <QSettings>
#include <QStateMachine>
#include <QVector>
#include <QWidget>
#include <memory>
#include <optional>

#include "Connections.h"
#include "OrbitBase/Logging.h"
#include "OrbitGgp/Client.h"
#include "OrbitGgp/Instance.h"
#include "OrbitGgp/InstanceItemModel.h"
#include "OrbitGgp/SshInfo.h"
#include "servicedeploymanager.h"
#include "ui_ConnectToStadiaWidget.h"

namespace orbit_qt {

class ConnectToStadiaWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool active READ IsActive WRITE SetActive)

 public:
  explicit ConnectToStadiaWidget(QWidget* parent = nullptr);
  void SetSshArtifacts(SshConnectionArtifacts* ssh_connection_artifacts);
  void SetConnection(StadiaConnection stadia_connection);
  [[nodiscard]] std::optional<StadiaConnection> StopAndClearConnection();
  [[nodiscard]] bool IsActive() const { return ui_->contentFrame->isEnabled(); }
  [[nodiscard]] const std::shared_ptr<grpc::Channel>& GetGrpcChannel() { return grpc_channel_; }
  void Start();

 public slots:
  void SetActive(bool value);

 signals:
  void Activated();
  void Connected();
  void Disconnected();

 private slots:
  void ReloadInstances();
  void CheckCredentialsAvailable();
  void DeployOrbitService();
  void Disconnect();
  void OnConnectToStadiaRadioButtonClicked(bool checked);
  void OnErrorOccurred(const QString& message);
  void OnSelectionChanged(const QModelIndex& current);
  void OnRememberCheckBoxToggled(bool checked);

 signals:
  void ErrorOccurred(const QString& message);
  void ReceivedInstances();
  void InstanceSelected();
  void ReceivedSshInfo();
  void ReadyToDeploy();
  void Connect();

 private:
  std::unique_ptr<Ui::ConnectToStadiaWidget> ui_;
  OrbitGgp::InstanceItemModel instance_model_;
  SshConnectionArtifacts* ssh_connection_artifacts_ = nullptr;
  std::optional<OrbitGgp::Instance> selected_instance_;
  std::unique_ptr<ServiceDeployManager> service_deploy_manager_;
  std::shared_ptr<grpc::Channel> grpc_channel_;
  QPointer<OrbitGgp::Client> ggp_client_ = nullptr;
  std::optional<QString> remembered_instance_id_;

  // State Machine
  QStateMachine state_machine_;
  QState* s_instances_loading_ = nullptr;
  QState* s_connected_ = nullptr;

  absl::flat_hash_map<std::string, ErrorMessageOr<OrbitSsh::Credentials>> instance_credentials_;

  void SetupStateMachine();
  void OnInstancesLoaded(outcome::result<QVector<OrbitGgp::Instance>> instances);
  void OnSshInfoLoaded(outcome::result<OrbitGgp::SshInfo> ssh_info_result, std::string instance_id);
  void TrySelectRememberedInstance();
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_CONNECT_TO_STADIA_WIDGET_H_