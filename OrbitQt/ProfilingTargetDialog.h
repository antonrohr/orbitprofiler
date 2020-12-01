// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_PROFILING_TARGET_DIALOG_H_
#define ORBIT_QT_PROFILING_TARGET_DIALOG_H_

#include <bits/stdint-uintn.h>

#include <QDialog>
#include <QFileInfo>
#include <QHistoryState>
#include <QSortFilterProxyModel>
#include <QStateMachine>
#include <memory>
#include <optional>
#include <variant>

#include "Connections.h"
#include "DeploymentConfigurations.h"
#include "MainThreadExecutor.h"
#include "OrbitClientData/ProcessData.h"
#include "OrbitClientServices/ProcessManager.h"
#include "ProcessItemModel.h"
#include "TargetConfiguration.h"
#include "grpcpp/channel.h"
#include "ui_ProfilingTargetDialog.h"

namespace orbit_qt {

class ProfilingTargetDialog : public QDialog {
  Q_OBJECT

 public:
  explicit ProfilingTargetDialog(SshConnectionArtifacts* ssh_connection_artifacts,
                                 MainThreadExecutor* main_thread_executor,
                                 QWidget* parent = nullptr);
  std::optional<ConnectionConfiguration> Exec(
      std::optional<ConnectionConfiguration> connection_configuration);
 private slots:
  void SelectFile();
  void LoadStadiaProcesses();
  void ResetStadiaProcessManager();
  void ResetLocalProcessManager();
  void ProcessSelectionChanged(const QModelIndex& current);
  void ConnectToLocal();

 signals:
  void FileSelected();
  void ProcessSelected();
  void NoProcessSelected();
  void StadiaIsConnected();
  void LocalIsConnected();

 private:
  enum class ResultTarget { kStadia, kLocal, kFile };

  std::unique_ptr<Ui::ProfilingTargetDialog> ui_;

  ResultTarget current_target_;

  ProcessItemModel process_model_;
  QSortFilterProxyModel process_proxy_model_;

  MainThreadExecutor* main_thread_executor_;

  std::unique_ptr<ProcessData> process_;

  std::unique_ptr<ProcessManager> stadia_process_manager_;

  std::unique_ptr<ProcessManager> local_process_manager_;
  std::shared_ptr<grpc::Channel> local_grpc_channel_;
  uint16_t local_grpc_port_;

  std::filesystem::path selected_file_path_;

  QStateMachine state_machine_;
  QState* s_stadia_ = nullptr;
  QHistoryState* s_s_history_ = nullptr;
  QState* s_s_connected_ = nullptr;
  QState* s_capture_ = nullptr;
  QHistoryState* s_c_history_ = nullptr;
  QState* s_c_file_selected_ = nullptr;
  QState* s_local_ = nullptr;
  QHistoryState* s_l_history_ = nullptr;
  QState* s_l_connected_ = nullptr;

  void SetupStateMachine();
  void TrySelectProcess(const ProcessData& process);
  void OnProcessListUpdate(ProcessManager* process_manager);
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_PROFILING_TARGET_DIALOG_H_