// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_PROFILING_TARGET_DIALOG_H_
#define ORBIT_QT_PROFILING_TARGET_DIALOG_H_

#include <QDialog>
#include <QFileInfo>
#include <QSortFilterProxyModel>
#include <QStateMachine>
#include <memory>
#include <optional>
#include <variant>

#include "ConnectionConfiguration.h"
#include "DeploymentConfigurations.h"
#include "MainThreadExecutor.h"
#include "OrbitClientData/ProcessData.h"
#include "OrbitClientServices/ProcessManager.h"
#include "ProcessItemModel.h"
#include "grpcpp/channel.h"
#include "ui_ProfilingTargetDialog.h"

namespace orbit_qt {

class ProfilingTargetDialog : public QDialog {
  Q_OBJECT
 public:
  explicit ProfilingTargetDialog(ConnectionConfiguration* connection_configuration,
                                 const OrbitSsh::Context* ssh_context,
                                 const ServiceDeployManager::GrpcPort& grpc_port,
                                 const DeploymentConfiguration* deployment_configuration,
                                 MainThreadExecutor* main_thread_executor,
                                 QWidget* parent = nullptr);

 private slots:
  void SelectFile();
  void LoadStadiaProcesses();
  void ResetStadiaProcessManager();
  void ProcessSelectionChanged(const QModelIndex& current);
  void ConnectToLocal();

 signals:
  void FileSelected();
  void ProcessSelected();
  void NoProcessSelected();
  void StadiaWidgetIsConnected();
  void LocalIsConnected();

 private:
  std::unique_ptr<Ui::ProfilingTargetDialog> ui_;
  ProcessItemModel process_model_;
  QSortFilterProxyModel process_proxy_model_;

  QStateMachine state_machine_;

  MainThreadExecutor* main_thread_executor_;
  StadiaConnection stadia_connection_;
  LocalConnection local_connection_;
  NoConnection no_connection_;
  ConnectionConfiguration* connection_configuration_;

  void SetupStateMachine();
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_PROFILING_TARGET_DIALOG_H_