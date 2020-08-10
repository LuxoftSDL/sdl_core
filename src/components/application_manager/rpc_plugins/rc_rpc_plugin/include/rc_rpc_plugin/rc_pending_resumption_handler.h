#pragma once

#include <map>
#include <queue>
#include <vector>
#include "application_manager/event_engine/event_observer.h"
#include "application_manager/resumption/extension_pending_resumption_handler.h"
#include "application_manager/resumption/resumption_data_processor.h"
#include "application_manager/rpc_service.h"
#include "rc_rpc_plugin/rc_app_extension.h"

namespace rc_rpc_plugin {

/**
 * @brief The RCPendingResumptionHandler class
 * responsibility to avoid duplication of subscription requests to HMI
 * if multiple applications are registering
 */
class RCPendingResumptionHandler
    : public resumption::ExtensionPendingResumptionHandler {
 public:
  RCPendingResumptionHandler(
      application_manager::ApplicationManager& application_manager);

  void on_event(const application_manager::event_engine::Event& event) override;

  void HandleResumptionSubscriptionRequest(
      application_manager::AppExtension& extension,
      resumption::Subscriber& subscriber,
      application_manager::Application& app) override;

  void OnResumptionRevert() override;

  static smart_objects::SmartObjectSPtr CreateSubscriptionRequest(
      const ModuleUid& module, const uint32_t correlation_id);

  static hmi_apis::FunctionID::eType GetFunctionId(
      const smart_objects::SmartObject& subscription_request);

  static ModuleUid GetModuleUid(
      const smart_objects::SmartObject& subscription_request);

 private:
  void HandleSuccessfulResponse(
      const application_manager::event_engine::Event& event,
      const ModuleUid& module_uid);

  void ProcessNextFreezedResumption(const ModuleUid& module);

  void RaiseEventForResponse(
      const smart_objects::SmartObject& subscription_response,
      const uint32_t correlation_id) const;

  bool IsPendingForResponse(const ModuleUid subscription) const;

  using QueueFreezedResumptions = std::queue<resumption::ResumptionRequest>;
  std::map<ModuleUid, QueueFreezedResumptions> freezed_resumptions_;
  std::vector<ModuleUid> subscriptions_;
  sync_primitives::Lock pending_resumption_lock_;
  std::map<int32_t, smart_objects::SmartObject> pending_requests_;
  application_manager::rpc_service::RPCService& rpc_service_;
};

}  // namespace rc_rpc_plugin
