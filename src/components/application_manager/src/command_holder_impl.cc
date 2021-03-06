/*
 * Copyright (c) 2017, Ford Motor Company
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

#include "application_manager/command_holder_impl.h"
#include "application_manager/application_manager.h"
#include "application_manager/commands/command.h"

namespace application_manager {
SDL_CREATE_LOG_VARIABLE("ApplicationManager")

CommandHolderImpl::CommandHolderImpl(ApplicationManager& app_manager)
    : app_manager_(app_manager) {}

void CommandHolderImpl::Suspend(
    ApplicationSharedPtr application,
    CommandType type,
    commands::Command::CommandSource source,
    std::shared_ptr<smart_objects::SmartObject> command) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(application);
  SDL_LOG_DEBUG("Suspending command(s) for application: "
                << application->policy_app_id());

  AppCommandInfo info = {command, source};

  sync_primitives::AutoLock lock(commands_lock_);
  if (CommandType::kHmiCommand == type) {
    app_hmi_commands_[application].push_back(info);
    SDL_LOG_DEBUG("Suspended HMI command(s): " << app_hmi_commands_.size());
  } else {
    app_mobile_commands_[application].push_back(info);
    SDL_LOG_DEBUG(
        "Suspended mobile command(s): " << app_mobile_commands_.size());
  }
}

void CommandHolderImpl::Resume(ApplicationSharedPtr application,
                               CommandType type) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(application);
  SDL_LOG_DEBUG(
      "Resuming command(s) for application: " << application->policy_app_id());
  if (CommandType::kHmiCommand == type) {
    ResumeHmiCommand(application);
  } else {
    ResumeMobileCommand(application);
  }
}

void CommandHolderImpl::Clear(ApplicationSharedPtr application) {
  SDL_LOG_AUTO_TRACE();
  DCHECK_OR_RETURN_VOID(application);
  SDL_LOG_DEBUG(
      "Clearing command(s) for application: " << application->policy_app_id());
  sync_primitives::AutoLock lock(commands_lock_);
  auto app_hmi_commands = app_hmi_commands_.find(application);
  if (app_hmi_commands_.end() != app_hmi_commands) {
    SDL_LOG_DEBUG(
        "Clearing HMI command(s): " << app_hmi_commands->second.size());
    app_hmi_commands_.erase(app_hmi_commands);
  }

  auto app_mobile_commands = app_mobile_commands_.find(application);
  if (app_mobile_commands_.end() != app_mobile_commands) {
    SDL_LOG_DEBUG(
        "Clearing mobile command(s): " << app_mobile_commands->second.size());
    app_mobile_commands_.erase(app_mobile_commands);
  }
}

void CommandHolderImpl::ResumeHmiCommand(ApplicationSharedPtr application) {
  DCHECK_OR_RETURN_VOID(application);
  sync_primitives::AutoLock lock(commands_lock_);
  auto app_commands = app_hmi_commands_.find(application);
  if (app_hmi_commands_.end() == app_commands) {
    return;
  }

  SDL_LOG_DEBUG("Resuming HMI command(s): " << app_hmi_commands_.size());

  for (auto cmd : app_commands->second) {
    (*cmd.command_ptr_)[strings::msg_params][strings::app_id] =
        application->hmi_app_id();
    app_manager_.GetRPCService().ManageHMICommand(cmd.command_ptr_,
                                                  cmd.command_source_);
  }

  app_hmi_commands_.erase(app_commands);
}

void CommandHolderImpl::ResumeMobileCommand(ApplicationSharedPtr application) {
  DCHECK_OR_RETURN_VOID(application);
  sync_primitives::AutoLock lock(commands_lock_);
  auto app_commands = app_mobile_commands_.find(application);
  if (app_mobile_commands_.end() == app_commands) {
    return;
  }

  SDL_LOG_DEBUG("Resuming mobile command(s): " << app_mobile_commands_.size());

  for (auto cmd : app_commands->second) {
    (*cmd.command_ptr_)[strings::params][strings::connection_key] =
        application->app_id();
    app_manager_.GetRPCService().ManageMobileCommand(cmd.command_ptr_,
                                                     cmd.command_source_);
  }

  app_mobile_commands_.erase(app_commands);
}
}  // namespace application_manager
