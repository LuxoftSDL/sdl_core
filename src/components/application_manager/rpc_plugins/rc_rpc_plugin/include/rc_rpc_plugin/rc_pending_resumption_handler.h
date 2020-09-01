#pragma once

#include <map>
#include <queue>
#include "application_manager/event_engine/event_observer.h"
#include "application_manager/resumption/extension_pending_resumption_handler.h"
#include "application_manager/resumption/resumption_data_processor.h"
#include "application_manager/rpc_service.h"
#include "rc_rpc_plugin/interior_data_cache.h"
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
      application_manager::ApplicationManager& application_manager,
      rc_rpc_plugin::InteriorDataCache& interior_data_cache);

  void on_event(const application_manager::event_engine::Event& event) override;

  void HandleResumptionSubscriptionRequest(
      application_manager::AppExtension& extension,
      resumption::Subscriber& subscriber,
      application_manager::Application& app) override;

  void OnResumptionRevert() override;

  /**
   * @brief Creates GetInteriorVehicleData
   * subscription request
   * @param module unique identify a module
   * moduleType + moduleID
   * @param correlation_id - unique ID
   * @return subscription request in smart object representation
   */
  static smart_objects::SmartObjectSPtr CreateSubscriptionRequest(
      const ModuleUid& module, const uint32_t correlation_id);

  /**
   * @brief Retrieves function id from
   * subscription request
   * @param subscription_request a subscription request that contains the
   * function id
   * @return function id
   */
  static hmi_apis::FunctionID::eType GetFunctionId(
      const smart_objects::SmartObject& subscription_request);

  /**
   * @brief Retrieves module uid from
   * subscription request
   * @param subscription_request a subscription request that contains a unique
   * identify a module
   * @return unique identify a module
   */
  static ModuleUid GetModuleUid(
      const smart_objects::SmartObject& subscription_request);

 private:
  /**
   * @brief The PendingRequest struct contains fields, needed during
   * processing events, related to responses from HMI to each resumption request
   */
  struct PendingRequest {
    uint32_t app_id;
    smart_objects::SmartObject message;
    uint32_t correlation_id() const {
      namespace am_strings = app_mngr::strings;
      return message[am_strings::params][am_strings::correlation_id].asInt();
    }
  };

  /**
   * @brief Handles a successful response from HMI
   * @param event contains response from HMI
   * @param module_uid a unique identify a module
   */
  void HandleSuccessfulResponse(
      const application_manager::event_engine::Event& event,
      const ModuleUid& module_uid);

  /**
   * @brief Creates next GetInteriorVehicleData
   * subscription request if a previous resumption of subscriptions is not
   * successful
   * @param module a unique identify a module
   */
  void ProcessNextFreezedResumption(const ModuleUid& module);

  /**
   * @brief Notifies subscriber about resumption status
   * @param subscription_response response from HMI
   * @param correaltion_id unique ID
   */
  void RaiseEventForResponse(
      const smart_objects::SmartObject& subscription_response,
      const uint32_t correlation_id) const;

  /**
   * @brief Checks if sdl is still waiting
   * responce for specified module
   * @param module a unique identify a module
   */
  bool IsPendingForResponse(const ModuleUid& module) const;

  /**
   * @brief Checks if any app except passed is subscribed
   * to a given module
   * @param module module to check
   * @param app_id app to ignore subscription
   * @return true if any app except passed is subscribed to module, otherwise
   * false
   */
  bool IsOtherAppsSubscribed(const uint32_t app_id,
                             const ModuleUid& module) const;

  /**
   * @brief Returns pending request, which corresponds to
   * correlation ID
   * @param corr_id unique ID
   * @return optional object, which contains subscription request, or empty
   * optional, if such request wasn't found
   */
  utils::Optional<smart_objects::SmartObject> GetPendingRequest(
      const uint32_t corr_id);

  /**
   * @brief Returns ID of application, related to pending request, which
   * corresponds to correlation ID
   * @param corr_id unique ID
   * @return optional object, which contains ID of application, or empty
   * optional, if such request wasn't found
   */
  utils::Optional<uint32_t> GetPendingApp(const uint32_t corr_id);

  /**
   * @brief Removes pending request, which corresponds to
   * correlation ID
   * @param corr_id unique ID
   */
  void RemovePendingRequest(const uint32_t corr_id);

  /**
   * @brief Adds pending request, related to
   * application ID
   * @param request_so pending request message
   */
  void AddPendingRequest(const uint32_t app_id,
                         const smart_objects::SmartObject request_so);

  /**
   * @brief QueueFreezedResumptions contains subscription request,
   * which will be sent to the HMI, in the case if a previous resumption of
   * subscriptions for the same module is not successful
   */
  using QueueFreezedResumptions = std::queue<PendingRequest>;
  std::map<ModuleUid, QueueFreezedResumptions> freezed_resumptions_;

  sync_primitives::Lock pending_resumption_lock_;
  std::vector<PendingRequest> pending_requests_;

  application_manager::rpc_service::RPCService& rpc_service_;
  rc_rpc_plugin::InteriorDataCache& interior_data_cache_;
};

}  // namespace rc_rpc_plugin
