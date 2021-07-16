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

#include "sdl_rpc_plugin/commands/hmi/on_exit_all_applications_notification.h"

#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <chrono>

#include "application_manager/application_manager.h"
#include "application_manager/resumption/resume_ctrl.h"
#include "application_manager/rpc_service.h"
#include "interfaces/HMI_API.h"

namespace sdl_rpc_plugin {
using namespace application_manager;

namespace commands {

SDL_CREATE_LOG_VARIABLE("Commands")

OnExitAllApplicationsNotification::OnExitAllApplicationsNotification(
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

OnExitAllApplicationsNotification::~OnExitAllApplicationsNotification() {}

void OnExitAllApplicationsNotification::Run() {
  SDL_LOG_AUTO_TRACE();

  const hmi_apis::Common_ApplicationsCloseReason::eType reason =
      static_cast<hmi_apis::Common_ApplicationsCloseReason::eType>(
          (*message_)[strings::msg_params][hmi_request::reason].asInt());
  SDL_LOG_DEBUG("Reason " << reason);

  mobile_api::AppInterfaceUnregisteredReason::eType mob_reason =
      mobile_api::AppInterfaceUnregisteredReason::INVALID_ENUM;

  switch (reason) {
    case hmi_apis::Common_ApplicationsCloseReason::IGNITION_OFF: {
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::IGNITION_OFF;
      break;
    }
    case hmi_apis::Common_ApplicationsCloseReason::MASTER_RESET: {
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::MASTER_RESET;
      break;
    }
    case hmi_apis::Common_ApplicationsCloseReason::FACTORY_DEFAULTS: {
      mob_reason = mobile_api::AppInterfaceUnregisteredReason::FACTORY_DEFAULTS;
      break;
    }
    case hmi_apis::Common_ApplicationsCloseReason::SUSPEND: {
      application_manager_.resume_controller().OnSuspend();
      SendOnSDLPersistenceComplete();
      return;
    }
    default: {
      SDL_LOG_ERROR("Unknown Application close reason" << reason);
      return;
    }
  }

  application_manager_.SetUnregisterAllApplicationsReason(mob_reason);

  if (mobile_api::AppInterfaceUnregisteredReason::MASTER_RESET == mob_reason ||
      mobile_api::AppInterfaceUnregisteredReason::FACTORY_DEFAULTS ==
          mob_reason) {
    application_manager_.HeadUnitReset(mob_reason);
  }

  SDL_INIT_FLUSH_LOGS_TIME_POINT(std::chrono::system_clock::now());
  kill(getpid(), SIGINT);
}

void OnExitAllApplicationsNotification::SendOnSDLPersistenceComplete() {
  SDL_LOG_AUTO_TRACE();

  smart_objects::SmartObjectSPtr message =
      std::make_shared<smart_objects::SmartObject>(
          smart_objects::SmartType_Map);
  (*message)[strings::params][strings::function_id] =
      hmi_apis::FunctionID::BasicCommunication_OnSDLPersistenceComplete;
  (*message)[strings::params][strings::message_type] =
      MessageType::kNotification;
  (*message)[strings::params][strings::correlation_id] =
      application_manager_.GetNextHMICorrelationID();

  rpc_service_.ManageHMICommand(message);
}

}  // namespace commands
}  // namespace sdl_rpc_plugin
