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

#include "sdl_rpc_plugin/commands/hmi/vr_is_ready_request.h"
#include "application_manager/rpc_service.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOGGERPTR("VRIsReadyRequest")

VRIsReadyRequest::VRIsReadyRequest(
    const application_manager::commands::MessageSharedPtr& message,
    ApplicationManager& application_manager,
    rpc_service::RPCService& rpc_service,
    HMICapabilities& hmi_capabilities,
    policy::PolicyHandlerInterface& policy_handle)
    : RequestToHMI(message,
                   application_manager,
                   rpc_service,
                   hmi_capabilities,
                   policy_handle)
    , EventObserver(application_manager.event_dispatcher()) {}

VRIsReadyRequest::~VRIsReadyRequest() {}

void VRIsReadyRequest::Run() {
  SDL_AUTO_TRACE();
  subscribe_on_event(hmi_apis::FunctionID::VR_IsReady, correlation_id());
  SendRequest();
}

void VRIsReadyRequest::on_event(const event_engine::Event& event) {
  SDL_AUTO_TRACE();
  const smart_objects::SmartObject& message = event.smart_object();
  switch (event.id()) {
    case hmi_apis::FunctionID::VR_IsReady: {
      SDL_DEBUG("Received VR_IsReady event");
      unsubscribe_from_event(hmi_apis::FunctionID::VR_IsReady);
      const bool is_available = app_mngr::commands::ChangeInterfaceState(
          application_manager_, message, HmiInterfaces::HMI_INTERFACE_VR);

      HMICapabilities& hmi_capabilities = hmi_capabilities_;
      hmi_capabilities.set_is_vr_cooperating(is_available);
      if (!app_mngr::commands::CheckAvailabilityHMIInterfaces(
              application_manager_, HmiInterfaces::HMI_INTERFACE_VR)) {
        UpdateRequiredInterfaceCapabilitiesRequests(hmi_interface::vr);
        SDL_INFO("HmiInterfaces::HMI_INTERFACE_VR isn't available");
        return;
      }

      RequestInterfaceCapabilities(hmi_interface::vr);
      break;
    }
    default: {
      SDL_ERROR("Received unknown event" << event.id());
      return;
    }
  }
}

void VRIsReadyRequest::onTimeOut() {
  // Note(dtrunov): According to new requirment APPLINK-27956
  RequestInterfaceCapabilities(hmi_interface::vr);
}

}  // namespace commands

}  // namespace sdl_rpc_plugin
