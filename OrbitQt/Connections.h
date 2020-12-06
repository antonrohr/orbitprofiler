// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_CONNECTIONS_H_
#define ORBIT_QT_CONNECTIONS_H_

#include <memory>
#include <optional>
#include <utility>

#include "DeploymentConfigurations.h"
#include "OrbitBase/Logging.h"
#include "OrbitClientData/ProcessData.h"
#include "OrbitClientServices/ProcessManager.h"
#include "OrbitGgp/Instance.h"
#include "OrbitSsh/Context.h"
#include "OrbitSsh/Credentials.h"
#include "grpcpp/channel.h"
#include "servicedeploymanager.h"

namespace orbit_qt {

class SshConnectionArtifacts {
 public:
  explicit SshConnectionArtifacts(const OrbitSsh::Context* ssh_context,
                                  ServiceDeployManager::GrpcPort grpc_port,
                                  const DeploymentConfiguration* deployment_configuration)
      : ssh_context_(ssh_context),
        grpc_port_(grpc_port),
        deployment_configuration_(deployment_configuration) {
    CHECK(ssh_context != nullptr);
    CHECK(deployment_configuration != nullptr);
  }

  [[nodiscard]] const OrbitSsh::Context* GetSshContext() const { return ssh_context_; }
  [[nodiscard]] const ServiceDeployManager::GrpcPort& GetGrpcPort() const { return grpc_port_; }
  [[nodiscard]] const DeploymentConfiguration* GetDeploymentConfiguration() const {
    return deployment_configuration_;
  }

 private:
  const OrbitSsh::Context* ssh_context_;
  const ServiceDeployManager::GrpcPort grpc_port_;
  const DeploymentConfiguration* deployment_configuration_;
};

class StadiaConnection {
  friend class ConnectToStadiaWidget;

 public:
  explicit StadiaConnection(OrbitGgp::Instance&& instance,
                            std::unique_ptr<ServiceDeployManager> service_deploy_manager,
                            std::shared_ptr<grpc::Channel>&& grpc_channel)
      : instance_(std::move(instance)),
        service_deploy_manager_(std::move(service_deploy_manager)),
        grpc_channel_(std::move(grpc_channel)) {}
  [[nodiscard]] const OrbitGgp::Instance& GetInstance() const { return instance_; }
  [[nodiscard]] ServiceDeployManager* GetServiceDeployManager() const {
    return service_deploy_manager_.get();
  }
  [[nodiscard]] const std::shared_ptr<grpc::Channel>& GetGrpcChannel() { return grpc_channel_; }

 private:
  OrbitGgp::Instance instance_;
  std::unique_ptr<ServiceDeployManager> service_deploy_manager_;
  std::shared_ptr<grpc::Channel> grpc_channel_;
};

class LocalConnection {
 public:
  explicit LocalConnection(std::shared_ptr<grpc::Channel> grpc_channel)
      : grpc_channel_(std::move(grpc_channel)) {}
  [[nodiscard]] const std::shared_ptr<grpc::Channel>& GetGrpcChannel() const {
    return grpc_channel_;
  }

 private:
  std::shared_ptr<grpc::Channel> grpc_channel_;
};

}  // namespace orbit_qt

#endif  // ORBIT_QT_CONNECTIONS_H_
