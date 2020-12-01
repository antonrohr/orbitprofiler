// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_CONNECT_TO_STADIA_WIDGET_H_
#define ORBIT_QT_CONNECT_TO_STADIA_WIDGET_H_

#include <QSettings>
#include <QStateMachine>
#include <QWidget>
#include <memory>

#include "ConnectionConfiguration.h"
#include "OrbitBase/Logging.h"
#include "OrbitGgp/Client.h"
#include "OrbitGgp/InstanceItemModel.h"
#include "ui_ConnectToStadiaWidget.h"

namespace orbit_qt {

class ConnectToStadiaWidget : public QWidget {
  Q_OBJECT
  Q_PROPERTY(bool active READ IsActive WRITE SetActive)

 public:
  explicit ConnectToStadiaWidget(QWidget* parent = nullptr);
  // This needs to be called before this class can be used. (It is not part of the constructor,
  // because otherwise this class could not be instantiated from a .ui file)
  void SetStadiaConnection(StadiaConnection* stadia_connection);
  bool IsActive() { return ui_->contentFrame->isEnabled(); }

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

 signals:
  void ErrorOccurred(const QString& message);
  void ReceivedInstances();
  void InstanceSelected();
  void ReceivedSshInfo();
  void ReadyToDeploy();
  void Connect();

 protected:
  bool eventFilter(QObject* obj, QEvent* event) override;

 private:
  std::unique_ptr<Ui::ConnectToStadiaWidget> ui_;
  StadiaConnection* stadia_connection_ = nullptr;
  OrbitGgp::InstanceItemModel instance_model_;
  QStateMachine state_machine_;
  QPointer<OrbitGgp::Client> ggp_client_ = nullptr;
  std::optional<QString> remembered_instance_id_;

  absl::flat_hash_map<std::string, ErrorMessageOr<OrbitSsh::Credentials>> instance_credentials_;

  void SetupAndStartStateMachine();
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_CONNECT_TO_STADIA_WIDGET_H_