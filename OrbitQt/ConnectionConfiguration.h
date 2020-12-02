// Copyright (c) 2020 The Orbit Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef ORBIT_QT_CONNECTION_CONFIGURATION_H_
#define ORBIT_QT_CONNECTION_CONFIGURATION_H_

#include <stdint.h>

#include <memory>
#include <utility>

#include "DeploymentConfigurations.h"
#include "OrbitBase/Logging.h"
#include "OrbitClientData/ProcessData.h"
#include "OrbitClientServices/ProcessManager.h"
#include "OrbitGgp/Instance.h"
#include "OrbitSsh/Context.h"
#include "OrbitSsh/Credentials.h"
#include "absl/strings/str_format.h"
#include "grpcpp/channel.h"
#include "grpcpp/create_channel.h"
#include "grpcpp/security/credentials.h"
#include "grpcpp/support/channel_arguments.h"
#include "servicedeploymanager.h"

namespace orbit_qt {

struct StadiaConnection {
 public:
  explicit StadiaConnection(const OrbitSsh::Context* ssh_context,
                            const ServiceDeployManager::GrpcPort& grpc_port,
                            const DeploymentConfiguration* deployment_configuration)
      : ssh_context(ssh_context),
        grpc_port(grpc_port),
        deployment_configuration(deployment_configuration) {
    CHECK(ssh_context != nullptr);
    CHECK(deployment_configuration != nullptr);
  }
  ~StadiaConnection() {
    if (process_manager != nullptr) process_manager->Shutdown();
  }
  void CreateServiceDeployManager(OrbitSsh::Credentials credentials) {
    CHECK(service_deploy_manager == nullptr);
    service_deploy_manager = std::make_unique<ServiceDeployManager>(
        deployment_configuration, ssh_context, std::move(credentials), grpc_port);
  }

  const OrbitSsh::Context* ssh_context;
  const ServiceDeployManager::GrpcPort& grpc_port;
  const DeploymentConfiguration* deployment_configuration;

  std::unique_ptr<ServiceDeployManager> service_deploy_manager;
  std::optional<OrbitGgp::Instance> instance;
  std::shared_ptr<grpc::Channel> grpc_channel;
  std::unique_ptr<ProcessManager> process_manager;
  std::unique_ptr<ProcessData> process;
};

struct LocalConnection {
 public:
  explicit LocalConnection(const uint16_t grpc_port) : grpc_port(grpc_port) {}
  void CreateGrpcChannel() {
    CHECK(grpc_channel == nullptr);
    std::string grpc_server_address = absl::StrFormat("127.0.0.1:%d", grpc_port);
    grpc_channel = grpc::CreateCustomChannel(
        grpc_server_address, grpc::InsecureChannelCredentials(), grpc::ChannelArguments());
  }
  const uint16_t grpc_port;

  std::shared_ptr<grpc::Channel> grpc_channel;
  std::unique_ptr<ProcessManager> process_manager;
  std::unique_ptr<ProcessData> process;
};

struct NoConnection {
 public:
  explicit NoConnection() = default;

  std::filesystem::path capture_file_path;
};

using ConnectionConfiguration =
    std::variant<const StadiaConnection*, const LocalConnection*, const NoConnection*>;

}  // namespace orbit_qt

#endif  // ORBIT_QT_CONNECTION_CONFIGURATION_H_
