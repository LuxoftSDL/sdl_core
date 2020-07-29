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

#ifndef SRC_COMPONENTS_INCLUDE_UTILS_LOGGER_H_
#define SRC_COMPONENTS_INCLUDE_UTILS_LOGGER_H_

#ifdef ENABLE_LOG
#include <string>
#include <sstream>

#include "utils/ilogger.h"
#include "utils/logger_status.h"
#include "utils/auto_trace.h"

// Redefince for each paticular logger implementation
namespace logger {
class Log4CXXLogger;
}
typedef logger::Log4CXXLogger ExternalLogger;

#define SDL_CREATE_LOGGERPTR(logger_name)  \
  static std::string logger_(logger_name);

// Logger deinitilization function and macro, need to stop log4cxx writing
// without this deinitilization log4cxx threads continue using some instances
// destroyed by exit()
#define SDL_DEINIT_LOGGER() \
  logger::Logger<ExternalLogger>::instance().DeInit();

// Logger thread deinitilization macro that need to stop the thread of handling
// messages for the log4cxx
#define SDL_DELETE_THREAD_LOGGER(logger_var) \
  logger::Logger<ExternalLogger>::instance().DeInit();

// special macros to dump logs from queue
// it's need, for example, when crash happend
#define SDL_FLUSH_LOGGER() \
  logger::Logger<ExternalLogger>::instance().Flush();

#define SDL_IS_TRACE_ENABLED(logger)                       \
  logger::Logger<ExternalLogger>::instance().IsEnabledFor( \
      logger_, logger::LogLevel::TRACE_LEVEL)

#define LOG_WITH_LEVEL(logLevel, logEvent)                             \
  do {                                                                 \
    if (logger::Logger<ExternalLogger>::instance().Enabled()) {        \
      if (logger::logger_status != logger::DeletingLoggerThread) {     \
        if (logger::Logger<ExternalLogger>::instance().IsEnabledFor(   \
                logger_, logLevel)) {                                  \
          std::stringstream accumulator;                               \
          accumulator << logEvent;                                     \
          logger::SDLLogMessage message{                               \
              logger_,                                                 \
              logLevel,                                                \
              accumulator.str(),                                       \
              std::chrono::high_resolution_clock::now(),               \
              logger::SDLLocationInfo{                                 \
                  __FILE__, __PRETTY_FUNCTION__, __LINE__},            \
              std::this_thread::get_id()};                             \
          logger::Logger<ExternalLogger>::instance().PushLog(message); \
        }                                                              \
      }                                                                \
    }                                                                  \
  } while (false)

#define SDL_TRACE(loggerPtr, logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::TRACE_LEVEL, logEvent)

#define SDL_AUTO_TRACE_WITH_NAME_SPECIFIED(auto_trace) \
  logger::AutoTrace auto_trace(                                   \
      logger_,                                                    \
      logger::SDLLocationInfo{__FILE__, __PRETTY_FUNCTION__, __LINE__})
#define SDL_AUTO_TRACE() \
  SDL_AUTO_TRACE_WITH_NAME_SPECIFIED(SDL_local_auto_trace_object)

#define SDL_DEBUG(loggerPtr, logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::DEBUG_LEVEL, logEvent)

#define SDL_INFO(loggerPtr, logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::INFO_LEVEL, logEvent)

#define SDL_WARN(loggerPtr, logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::WARNIGN_LEVEL, logEvent)

#define SDL_ERROR(loggerPtr, logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::ERROR_LEVEL, logEvent)

#define SDL_FATAL(loggerPtr, logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::FATAL_LEVEL, logEvent)

#define SDL_ERROR_WITH_ERRNO(loggerPtr, message)                           \
  SDL_ERROR(loggerPtr, message << ", error code " << errno << " (" << strerror(errno) \
                    << ")")

#define SDL_WARN_WITH_ERRNO(loggerPtr, message)                           \
  SDL_WARN(loggerPtr, message << ", error code " << errno << " (" << strerror(errno) \
                   << ")")

#else  // ENABLE_LOG is OFF

#define SDL_CREATE_LOGGERPTR(logger_name)

#define INIT_LOGGER(file_name, logs_enabled)

#define SDL_DEINIT_LOGGER() ()

#define DELETE_THREAD_LOGGER(logger_var)

#define SDL_SDL_FLUSH_LOGGER() ()

#define SDL_IS_TRACE_ENABLED(logger) false

#define SDL_TRACE(x, y)

#define LOG4CXX_AUTO_TRACE_WITH_NAME_SPECIFIED(auto_trace)
#define LOG4CXX_AUTO_TRACE()

#define SDL_DEBUG(x, y)

#define SDL_INFO(x, y)

#define SDL_WARN(x, y)

#define SDL_ERROR(x, y)

#define SDL_ERROR_WITH_ERRNO(x, y)

#define SDL_WARN_WITH_ERRNO(x, y)

#define SDL_FATAL(x, y)

#endif  // ENABLE_LOG

#endif  // SRC_COMPONENTS_INCLUDE_UTILS_LOGGER_H_
