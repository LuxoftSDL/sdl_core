/*
 * Copyright (c) 2020, Ford Motor Company
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

#ifndef SRC_COMPONENTS_UTILS_INCLUDE_UTILS_LOGGER_LOGGER_IMPL_H_
#define SRC_COMPONENTS_UTILS_INCLUDE_UTILS_LOGGER_LOGGER_IMPL_H_

#include "utils/ilogger.h"

#include <assert.h>
#include <functional>
#include <iostream>

#include "utils/logger_status.h"
#include "utils/threads/message_loop_thread.h"

namespace logger {

typedef std::queue<SDLLogMessage> LogMessageQueue;

typedef threads::MessageLoopThread<LogMessageQueue>
    LogMessageLoopThreadTemplate;

template <typename T>
using LoggerPtr = std::unique_ptr<T, std::function<void(T*)> >;

class LogMessageLoopThread : public LogMessageLoopThreadTemplate,
                             public LogMessageLoopThreadTemplate::Handler {
 public:
  LogMessageLoopThread(std::function<void(SDLLogMessage)> handler)
      : LogMessageLoopThreadTemplate("Logger", this), blocking_call_(handler) {}

  void Push(const SDLLogMessage& message) {
    this->PostMessage(message);
  }

  void Handle(const SDLLogMessage message) {
    blocking_call_(message);
  }

  ~LogMessageLoopThread() {
    // we'll have to drop messages
    // while deleting logger thread
    logger::logger_status = logger::DeletingLoggerThread;
    LogMessageLoopThreadTemplate::Shutdown();
  }

 private:
  std::function<void(SDLLogMessage)> blocking_call_;
};

template <class ThirdPartyLogger>
class LoggerImp : public Logger<ThirdPartyLogger> {
 public:
  LoggerImp() : impl_(nullptr), logs_enabled_(false) {}

  void Init(ThirdPartyLogger* impl) override {
    assert(impl_ == nullptr);
    Enable();
    impl_ = impl;
    impl_->Init();

    auto deinit_logger = [](LogMessageLoopThread* logMsgThread) {
      delete logMsgThread;
      logger::logger_status = logger::LoggerThreadNotCreated;
    };

    if (!loop_thread_) {
      loop_thread_ = LoggerPtr<LogMessageLoopThread>(
          new LogMessageLoopThread(
              [this](SDLLogMessage message) { impl_->PushLog(message); }),
          deinit_logger);
    }
    logger::logger_status = logger::LoggerThreadCreated;
  }

  void DeInit() override {
    Disable();
    if (logger::logger_status == logger::LoggerThreadCreated)
      Flush();
    loop_thread_.reset();
    impl_->DeInit();
    logger::logger_status = logger::LoggerThreadNotCreated;
  }

  void Enable() {
    logs_enabled_ = true;
  }

  bool Enabled() {
    return logs_enabled_;
  }

  void Disable() {
    logs_enabled_ = false;
  }

  void Flush() override {
    logger::LoggerStatus old_status = logger::logger_status;
    logger::logger_status = logger::DeletingLoggerThread;
    loop_thread_->WaitDumpQueue();
    logger::logger_status = old_status;
  }

  bool IsEnabledFor(const std::string& logger, LogLevel log_level) override {
    return impl_->IsEnabledFor(logger, log_level);
  }

  void PushLog(const SDLLogMessage& log_message) override {
    if (loop_thread_)
      loop_thread_->Push(log_message);
    // Optional to use blocking call :
    // impl_->PushLog(log_message);
  }

  ThirdPartyLogger* impl_;
  LoggerPtr<LogMessageLoopThread> loop_thread_;
  std::atomic_bool logs_enabled_;
};

template <typename ThirdPartyLogger>
Logger<ThirdPartyLogger>& Logger<ThirdPartyLogger>::instance() {
  static LoggerImp<ThirdPartyLogger> inst;
  return inst;
}

}  // namespace logger

#endif  // SRC_COMPONENTS_UTILS_INCLUDE_UTILS_LOGGER_LOGGER_IMPL_H_
