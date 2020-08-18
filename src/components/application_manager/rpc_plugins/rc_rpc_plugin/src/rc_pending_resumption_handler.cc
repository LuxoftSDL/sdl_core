#include "rc_rpc_plugin/rc_pending_resumption_handler.h"
#include "rc_rpc_plugin/rc_helpers.h"
#include "rc_rpc_plugin/rc_module_constants.h"

namespace rc_rpc_plugin {

CREATE_LOGGERPTR_GLOBAL(logger_, "RCPendingResumptionHandler")

RCPendingResumptionHandler::RCPendingResumptionHandler(
    application_manager::ApplicationManager& application_manager,
    InteriorDataCache& interior_data_cache)
    : ExtensionPendingResumptionHandler(application_manager)
    , rpc_service_(application_manager.GetRPCService())
    , interior_data_cache_(interior_data_cache) {}

void RCPendingResumptionHandler::on_event(
    const application_manager::event_engine::Event& event) {
  LOG4CXX_AUTO_TRACE(logger_);
  namespace am_strings = application_manager::strings;
  sync_primitives::AutoLock lock(pending_resumption_lock_);
  const auto cid = event.smart_object_correlation_id();
  LOG4CXX_TRACE(logger_,
                "Received event with function id: "
                    << event.id() << " and correlation id: " << cid);
  const auto& data_request = pending_requests_.find(cid);
  if (data_request == pending_requests_.end()) {
    LOG4CXX_ERROR(logger_,
                  "Not waiting for message with correlation id: " << cid);
    return;
  }

  auto current_request = data_request->second;
  pending_requests_.erase(data_request);
  auto module_uid = GetModuleUid(current_request);

  auto& response = event.smart_object();
  if (RCHelpers::IsResponseSuccessful(response)) {
    LOG4CXX_DEBUG(logger_,
                  "Resumption of subscriptions is successful"
                      << " module type: " << module_uid.first
                      << " module id: " << module_uid.second);

    if (response[am_strings::msg_params].keyExists(
            message_params::kModuleData)) {
      const auto module_data =
          response[am_strings::msg_params][message_params::kModuleData];
      const auto& data_mapping = RCHelpers::GetModuleTypeToDataMapping();
      if (module_data.keyExists(data_mapping(module_uid.first))) {
        const auto control_data = module_data[data_mapping(module_uid.first)];
        interior_data_cache_.Add(module_uid, control_data);
      }
    }

    HandleSuccessfulResponse(event, module_uid);
  } else {
    LOG4CXX_DEBUG(logger_,
                  "Resumption of subscriptions is NOT successful"
                      << " module type: " << module_uid.first
                      << " module id: " << module_uid.second);
    ProcessNextFreezedResumption(module_uid);
  }
}

void RCPendingResumptionHandler::HandleResumptionSubscriptionRequest(
    application_manager::AppExtension& extension,
    resumption::Subscriber& subscriber,
    application_manager::Application& app) {
  UNUSED(extension);
  LOG4CXX_AUTO_TRACE(logger_);
  sync_primitives::AutoLock lock(pending_resumption_lock_);
  LOG4CXX_TRACE(logger_, "app id " << app.app_id());

  auto rc_extension = RCHelpers::GetRCExtension(app);
  auto subscriptions = rc_extension->InteriorVehicleDataSubscriptions();

  std::vector<ModuleUid> already_pending;
  std::vector<ModuleUid> need_to_subscribe;
  for (auto subscription : subscriptions) {
    bool is_another_app_subscribed =
        IsAnotherAppsSubscribedOnTheSameModule(app.app_id(), subscription);
    bool is_pending_response = IsPendingForResponse(subscription);
    if (is_another_app_subscribed && !is_pending_response) {
      continue;
    } else if (is_pending_response) {
      already_pending.push_back(subscription);
    } else {
      need_to_subscribe.push_back(subscription);
    }
  }

  for (auto subscription : already_pending) {
    const auto cid = application_manager_.GetNextHMICorrelationID();
    const auto subscription_request =
        CreateSubscriptionRequest(subscription, cid);
    const auto fid = GetFunctionId(*subscription_request);
    const auto resumption_request =
        MakeResumptionRequest(cid, fid, *subscription_request);
    freezed_resumptions_[subscription].push(resumption_request);
    subscriber(app.app_id(), resumption_request);
    LOG4CXX_DEBUG(logger_,
                  "Freezed request with correlation_id: "
                      << cid << " module type: " << subscription.first
                      << " module id: " << subscription.second);
  }

  for (auto module : need_to_subscribe) {
    const auto cid = application_manager_.GetNextHMICorrelationID();
    const auto subscription_request = CreateSubscriptionRequest(module, cid);
    const auto fid = GetFunctionId(*subscription_request);
    const auto resumption_request =
        MakeResumptionRequest(cid, fid, *subscription_request);
    pending_requests_.emplace(cid, *subscription_request);
    subscribe_on_event(fid, cid);
    subscriber(app.app_id(), resumption_request);
    LOG4CXX_DEBUG(logger_,
                  "Sending request with correlation id: "
                      << cid << " module type: " << module.first
                      << " module id: " << module.second);
    application_manager_.GetRPCService().ManageHMICommand(subscription_request);
  }
}

void RCPendingResumptionHandler::OnResumptionRevert() {
  LOG4CXX_AUTO_TRACE(logger_);
}

void RCPendingResumptionHandler::HandleSuccessfulResponse(
    const application_manager::event_engine::Event& event,
    const ModuleUid& module_uid) {
  LOG4CXX_AUTO_TRACE(logger_);

  auto& response = event.smart_object();
  auto cid = event.smart_object_correlation_id();

  const auto& it_queue_freezed = freezed_resumptions_.find(module_uid);
  if (it_queue_freezed != freezed_resumptions_.end()) {
    LOG4CXX_DEBUG(logger_, "Freezed resumptions found");
    auto& queue_freezed = it_queue_freezed->second;
    while (!queue_freezed.empty()) {
      const auto& resumption_request = queue_freezed.front();
      cid = resumption_request
                .message[app_mngr::strings::params]
                        [app_mngr::strings::correlation_id]
                .asInt();
      RaiseEventForResponse(response, cid);
      queue_freezed.pop();
    }
    freezed_resumptions_.erase(it_queue_freezed);
  }
}

void RCPendingResumptionHandler::ProcessNextFreezedResumption(
    const ModuleUid& module_uid) {
  LOG4CXX_AUTO_TRACE(logger_);

  auto pop_front_freezed_resumptions = [this](const ModuleUid& module_uid) {
    const auto& it_queue_freezed = freezed_resumptions_.find(module_uid);
    if (it_queue_freezed == freezed_resumptions_.end()) {
      return std::shared_ptr<QueueFreezedResumptions::value_type>(nullptr);
    }
    auto& queue_freezed = it_queue_freezed->second;
    if (queue_freezed.empty()) {
      freezed_resumptions_.erase(it_queue_freezed);
      return std::shared_ptr<QueueFreezedResumptions::value_type>(nullptr);
    }
    auto freezed_resumption =
        std::make_shared<QueueFreezedResumptions::value_type>(
            queue_freezed.front());
    queue_freezed.pop();
    if (queue_freezed.empty()) {
      freezed_resumptions_.erase(it_queue_freezed);
    }
    return freezed_resumption;
  };

  auto freezed_resumption = pop_front_freezed_resumptions(module_uid);
  if (!freezed_resumption) {
    LOG4CXX_DEBUG(logger_, "No freezed resumptions found");
    return;
  }

  auto& resumption_request = *freezed_resumption;
  auto subscription_request =
      std::make_shared<smart_objects::SmartObject>(resumption_request.message);
  const auto fid = GetFunctionId(*subscription_request);
  const auto cid =
      resumption_request
          .message[app_mngr::strings::params][app_mngr::strings::correlation_id]
          .asInt();
  subscribe_on_event(fid, cid);
  pending_requests_.emplace(cid, *subscription_request);
  LOG4CXX_DEBUG(logger_,
                "Sending request with correlation id: "
                    << cid << " module type: " << module_uid.first
                    << " module id: " << module_uid.second);
  application_manager_.GetRPCService().ManageHMICommand(subscription_request);
}

void RCPendingResumptionHandler::RaiseEventForResponse(
    const smart_objects::SmartObject& subscription_response,
    const uint32_t correlation_id) const {
  smart_objects::SmartObject event_message = subscription_response;
  event_message[app_mngr::strings::params][app_mngr::strings::correlation_id] =
      correlation_id;

  app_mngr::event_engine::Event event(
      hmi_apis::FunctionID::RC_GetInteriorVehicleData);
  event.set_smart_object(event_message);
  event.raise(application_manager_.event_dispatcher());
}

bool RCPendingResumptionHandler::IsPendingForResponse(
    const ModuleUid& module_uid) const {
  auto is_module_exists =
      [&module_uid](const PendingRequestList::value_type& value_type) {
        return module_uid == GetModuleUid(value_type.second);
      };
  auto it = std::find_if(
      pending_requests_.begin(), pending_requests_.end(), is_module_exists);
  return it != pending_requests_.end();
}

bool RCPendingResumptionHandler::IsAnotherAppsSubscribedOnTheSameModule(
    const uint32_t app_id, const ModuleUid& module_uid) const {
  auto get_subscriptions = [](application_manager::ApplicationSharedPtr app) {
    std::set<ModuleUid> result;
    auto rc_app_extension = RCHelpers::GetRCExtension(*app);
    if (rc_app_extension) {
      result = rc_app_extension->InteriorVehicleDataSubscriptions();
    }
    return result;
  };

  auto another_app_subscribed =
      [app_id, module_uid, &get_subscriptions](
          application_manager::ApplicationSharedPtr app) {
        if (app_id == app->app_id()) {
          return false;
        }
        auto subscriptions = get_subscriptions(app);
        auto it = subscriptions.find(module_uid);
        return subscriptions.end() != it;
      };

  auto accessor = application_manager_.applications();
  auto it = std::find_if(accessor.GetData().begin(),
                         accessor.GetData().end(),
                         another_app_subscribed);
  return accessor.GetData().end() != it;
}

smart_objects::SmartObjectSPtr
RCPendingResumptionHandler::CreateSubscriptionRequest(
    const ModuleUid& module, const uint32_t correlation_id) {
  auto request = RCHelpers::CreateGetInteriorVDRequestToHMI(
      module, correlation_id, RCHelpers::GetInteriorData::SUBSCRIBE);
  return request;
}

hmi_apis::FunctionID::eType RCPendingResumptionHandler::GetFunctionId(
    const smart_objects::SmartObject& subscription_request) {
  const auto function_id = static_cast<hmi_apis::FunctionID::eType>(
      subscription_request[app_mngr::strings::params]
                          [app_mngr::strings::function_id]
                              .asInt());
  return function_id;
}

ModuleUid RCPendingResumptionHandler::GetModuleUid(
    const smart_objects::SmartObject& subscription_request) {
  const smart_objects::SmartObject& msg_params =
      subscription_request[app_mngr::strings::msg_params];
  return ModuleUid(msg_params[message_params::kModuleType].asString(),
                   msg_params[message_params::kModuleId].asString());
}

}  // namespace rc_rpc_plugin
