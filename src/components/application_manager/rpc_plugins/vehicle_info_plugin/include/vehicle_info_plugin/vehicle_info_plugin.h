/*
 Copyright (c) 2018, Ford Motor Company
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 Redistributions of source code must retain the above copyright notice, this
 list of conditions and the following disclaimer.

 Redistributions in binary form must reproduce the above copyright notice,
 this list of conditions and the following
 disclaimer in the documentation and/or other materials provided with the
 distribution.

 Neither the name of the Ford Motor Company nor the names of its contributors
 may be used to endorse or promote products derived from this software
 without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_VEHICLE_INFO_PLUGIN_INCLUDE_VEHICLE_INFO_PLUGIN_VEHICLE_INFO_PLUGIN_H
#define SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_VEHICLE_INFO_PLUGIN_INCLUDE_VEHICLE_INFO_PLUGIN_VEHICLE_INFO_PLUGIN_H

#include "application_manager/command_factory.h"
#include "application_manager/resumption/pending_resumption_handler.h"
#include "vehicle_info_plugin/custom_vehicle_data_manager.h"

namespace vehicle_info_plugin {
class VehicleInfoAppExtension;
namespace app_mngr = application_manager;
namespace plugins = application_manager::plugin_manager;

bool IsSubscribedAppExist(const std::string& ivi,
                          const app_mngr::ApplicationManager& app_manager);

class VehicleInfoPlugin : public plugins::RPCPlugin {
 public:
  VehicleInfoPlugin();

  bool Init(app_mngr::ApplicationManager& application_manager,
            app_mngr::rpc_service::RPCService& rpc_service,
            app_mngr::HMICapabilities& hmi_capabilities,
            policy::PolicyHandlerInterface& policy_handler,
            resumption::LastStateWrapperPtr last_state) OVERRIDE;

  bool IsAbleToProcess(
      const int32_t function_id,
      const app_mngr::commands::Command::CommandSource source) OVERRIDE;

  std::string PluginName() OVERRIDE;
  app_mngr::CommandFactory& GetCommandFactory() OVERRIDE;

  void OnPolicyEvent(plugins::PolicyEvent event) OVERRIDE;
  void OnApplicationEvent(plugins::ApplicationEvent event,
                          app_mngr::ApplicationSharedPtr application) OVERRIDE;

  /**
   * @brief ProcessResumptionSubscription send Subscribe vehicle data requests
   * to HMI
   * @param app application for subscription
   * @param ext application extension
   * @param subscriber callback for subscription
   */
  void ProcessResumptionSubscription(app_mngr::Application& app,
                                     VehicleInfoAppExtension& ext);

  /**
   * @brief Revert the data to the state before Resumption.
   * @param subscriptions Subscriptions to be returned
   **/
  void RevertResumption(app_mngr::Application& app,
                        const std::set<std::string>& list_of_subscriptions);

  smart_objects::SmartObjectSPtr CreateSubscriptionRequest(
      const std::set<std::string>& list_of_subscriptions);

  smart_objects::SmartObjectSPtr CreateUnsubscriptionRequest(
      const std::set<std::string>& list_of_subscriptions);

 private:
  bool IsAnyPendingSubscriptionExist(const std::string& ivi);
  void UnsubscribeFromRemovedVDItems();
  smart_objects::SmartObjectSPtr GetUnsubscribeIVIRequest(
      const std::vector<std::string>& ivi_names);
  void DeleteSubscriptions(app_mngr::ApplicationSharedPtr app);

  std::unique_ptr<app_mngr::CommandFactory> command_factory_;
  typedef std::shared_ptr<resumption::PendingResumptionHandler>
      PendingResumptionHandlerSPtr;
  app_mngr::ApplicationManager* application_manager_;
  std::unique_ptr<CustomVehicleDataManager> custom_vehicle_data_manager_;
  PendingResumptionHandlerSPtr pending_resumption_handler_;
};
}  // namespace vehicle_info_plugin

extern "C" application_manager::plugin_manager::RPCPlugin* Create(
    logger::Logger* logger_instances);
extern "C" void Delete(application_manager::plugin_manager::RPCPlugin* data);
#endif  // SRC_COMPONENTS_APPLICATION_MANAGER_RPC_PLUGINS_VEHICLE_INFO_PLUGIN_INCLUDE_VEHICLE_INFO_PLUGIN_VEHICLE_INFO_PLUGIN_H
