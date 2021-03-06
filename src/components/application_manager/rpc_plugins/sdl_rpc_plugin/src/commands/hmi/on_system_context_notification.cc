/*
 * Copyright (c) 2018, Ford Motor Company
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following
 * disclaimer in the documentation and/or other materials provided with the
 * distribution.
 *
 * Neither the name of the Ford Motor Company nor the names of its contributors
 * may be used to endorse or promote products derived from this software
 * without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sdl_rpc_plugin/commands/hmi/on_system_context_notification.h"

#include "application_manager/application_impl.h"
#include "application_manager/message_helper.h"
#include "application_manager/state_controller.h"

namespace sdl_rpc_plugin {
using namespace application_manager;
namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

OnSystemContextNotification::OnSystemContextNotification(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handle)
    : NotificationFromHMI(message,
                          application_manager,
                          rpc_service,
                          hmi_capabilities,
                          policy_handle) {}

OnSystemContextNotification::~OnSystemContextNotification() {}

void OnSystemContextNotification::Run() {
  SDL_LOG_AUTO_TRACE();

  mobile_api::SystemContext::eType system_context =
      static_cast<mobile_api::SystemContext::eType>(
          (*message_)[strings::msg_params][hmi_notification::system_context]
              .asInt());

  WindowID window_id = mobile_apis::PredefinedWindows::DEFAULT_WINDOW;
  if ((*message_)[strings::msg_params].keyExists(strings::window_id)) {
    window_id = (*message_)[strings::msg_params][strings::window_id].asInt();
  }

  ApplicationSharedPtr app;
  if (helpers::
          Compare<mobile_api::SystemContext::eType, helpers::EQ, helpers::ONE>(
              system_context,
              mobile_api::SystemContext::SYSCTXT_VRSESSION,
              mobile_api::SystemContext::SYSCTXT_MENU,
              mobile_api::SystemContext::SYSCTXT_HMI_OBSCURED)) {
    app = application_manager_.active_application();
  }

  if (mobile_apis::PredefinedWindows::DEFAULT_WINDOW != window_id ||
      helpers::Compare<mobile_api::SystemContext::eType,
                       helpers::EQ,
                       helpers::ONE>(system_context,
                                     mobile_api::SystemContext::SYSCTXT_ALERT,
                                     mobile_api::SystemContext::SYSCTXT_MAIN)) {
    if ((*message_)[strings::msg_params].keyExists(strings::app_id)) {
      app = application_manager_.application(
          (*message_)[strings::msg_params][strings::app_id].asUInt());
    }
  }

  if (app && mobile_api::SystemContext::INVALID_ENUM != system_context) {
    application_manager_.state_controller().SetRegularState(
        app, window_id, system_context);
  } else {
    SDL_LOG_ERROR("Application does not exist");
  }
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
