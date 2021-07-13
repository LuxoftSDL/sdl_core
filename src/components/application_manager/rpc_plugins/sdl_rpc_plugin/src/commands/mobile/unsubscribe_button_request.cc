/*
 Copyright (c) 2021, Ford Motor Company
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

#include "sdl_rpc_plugin/commands/mobile/unsubscribe_button_request.h"

#include "application_manager/application_impl.h"
#include "application_manager/message_helper.h"
#include "utils/helpers.h"
#include "utils/semantic_version.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

namespace str = strings;

UnsubscribeButtonRequest::UnsubscribeButtonRequest(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    app_mngr::rpc_service::RPCService& rpc_service,
    app_mngr::HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handler)
    : CommandRequestImpl(message,
                         application_manager,
                         rpc_service,
                         hmi_capabilities,
                         policy_handler) {}

UnsubscribeButtonRequest::~UnsubscribeButtonRequest() {}

void UnsubscribeButtonRequest::Run() {
  SDL_LOG_AUTO_TRACE();

  ApplicationSharedPtr app = application_manager_.application(connection_key());

  if (!app) {
    SDL_LOG_ERROR("APPLICATION_NOT_REGISTERED");
    SendResponse(false, mobile_apis::Result::APPLICATION_NOT_REGISTERED);
    return;
  }

  mobile_apis::ButtonName::eType btn_id =
      static_cast<mobile_apis::ButtonName::eType>(
          (*message_)[str::msg_params][str::button_name].asInt());

  if (app->msg_version() < utils::rpc_version_5 &&
      btn_id == mobile_apis::ButtonName::OK && app->is_media_application()) {
    bool ok_supported = CheckHMICapabilities(mobile_apis::ButtonName::OK);
    bool play_pause_supported =
        CheckHMICapabilities(mobile_apis::ButtonName::PLAY_PAUSE);
    if (play_pause_supported) {
      SDL_LOG_DEBUG("Converting Legacy OK button to PLAY_PAUSE");
      btn_id = mobile_apis::ButtonName::PLAY_PAUSE;
      (*message_)[str::msg_params][str::button_name] = btn_id;
    } else if (!ok_supported) {
      SDL_LOG_ERROR("OK button isn't allowed by HMI capabilities");
      SendResponse(false, mobile_apis::Result::UNSUPPORTED_RESOURCE);
    }
  } else if (!CheckHMICapabilities(btn_id)) {
    SDL_LOG_ERROR("Button " << btn_id << " isn't allowed by HMI capabilities");
    SendResponse(false, mobile_apis::Result::UNSUPPORTED_RESOURCE);
    return;
  }

  if (!app->IsSubscribedToButton(btn_id)) {
    SDL_LOG_ERROR("App is not subscribed to button " << btn_id);
    SendResponse(false, mobile_apis::Result::IGNORED);
    return;
  }

  (*message_)[str::msg_params][str::app_id] = app->app_id();
  StartAwaitForInterface(HmiInterfaces::HMI_INTERFACE_Buttons);
  SendHMIRequest(hmi_apis::FunctionID::Buttons_UnsubscribeButton,
                 &(*message_)[app_mngr::strings::msg_params],
                 true);
}

void UnsubscribeButtonRequest::on_event(const event_engine::Event& event) {
  SDL_LOG_AUTO_TRACE();
  using namespace helpers;

  const smart_objects::SmartObject& message = event.smart_object();

  if (hmi_apis::FunctionID::Buttons_UnsubscribeButton != event.id()) {
    SDL_LOG_ERROR("Received unknown event.");
    return;
  }
  EndAwaitForInterface(HmiInterfaces::HMI_INTERFACE_Buttons);
  ApplicationSharedPtr app =
      application_manager_.application(CommandRequestImpl::connection_key());

  if (!app) {
    SDL_LOG_ERROR("NULL pointer.");
    return;
  }

  hmi_apis::Common_Result::eType hmi_result =
      static_cast<hmi_apis::Common_Result::eType>(
          message[strings::params][hmi_response::code].asInt());
  std::string response_info;
  GetInfo(message, response_info);
  const bool result = PrepareResultForMobileResponse(
      hmi_result, HmiInterfaces::HMI_INTERFACE_Buttons);

  mobile_apis::Result::eType result_code =
      MessageHelper::HMIToMobileResult(hmi_result);

  SendResponse(result,
               result_code,
               response_info.empty() ? nullptr : response_info.c_str(),
               &(message[strings::msg_params]));
}

bool UnsubscribeButtonRequest::Init() {
  hash_update_mode_ = HashUpdateMode::kDoHashUpdate;
  return true;
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
