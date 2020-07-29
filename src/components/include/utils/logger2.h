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
#include <log4cxx/logger.h>
#include <string>
#include "utils/ilogger.h"
#include "utils/logger_status.h"

// Redefince for each paticular logger implementation
namespace logger {
class Log4CXXLogger;
}
typedef logger::Log4CXXLogger ExternalLogger;

#define SDL_CREATE_LOGGERPTR(logger_name)    \
  namespace {                                \
    static std::string logger_(logger_name); \
  }


// Logger deinitilization function and macro, need to stop log4cxx writing
// without this deinitilization log4cxx threads continue using some instances
// destroyed by exit()
#define SDL_DEINIT_LOGGER() logger::Logger<ExternalLogger>::instance().DeInit();

// Logger thread deinitilization macro that need to stop the thread of handling
// messages for the log4cxx
#define SDL_DELETE_THREAD_LOGGER(logger_var) \
  logger::Logger<ExternalLogger>::instance().DeInit();

// special macros to dump logs from queue
// it's need, for example, when crash happend
#define SDL_FLUSH_LOGGER() logger::Logger<ExternalLogger>::instance().Flush();

#define SDL_IS_TRACE_ENABLED(logger) \
  logger::Logger<ExternalLogger>::instance().IsEnabledFor( \
                    logger_, logger::LogLevel::TRACE_LEVEL)

#define LOG_WITH_LEVEL(logLevel, logEvent)                                                \
  do {                                                                                    \
    if (logger::Logger<ExternalLogger>::instance().Enabled()) {                           \
      if (logger::logger_status != logger::DeletingLoggerThread) {                        \
        if (logger::Logger<ExternalLogger>::instance().IsEnabledFor(logger_, logLevel)) { \
          logger::SDLLogMessage message{                                                  \
              logger_,                                                                    \
              logLevel,                                                                   \
              logEvent,                                                                   \
              std::chrono::high_resolution_clock::now(),                                  \
              logger::SDLLocationInfo{__FILE__, __PRETTY_FUNCTION__, __LINE__},           \
              std::this_thread::get_id()};                                                \
          logger::Logger<ExternalLogger>::instance().PushLog(message);                    \
        }                                                                                 \
      }                                                                                   \
    }                                                                                     \
  } while (false)

#define SDL_TRACE(logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::TRACE_LEVEL, logEvent)

#define SDL_AUTO_TRACE_WITH_NAME_SPECIFIED(loggerPtr, auto_trace) \
  logger::AutoTrace auto_trace(logger_, logger::SDLLocationInfo{__FILE__, __PRETTY_FUNCTION__, __LINE__})
#define SDL_AUTO_TRACE(loggerPtr) \
  LOG4CXX_AUTO_TRACE_WITH_NAME_SPECIFIED(logger_, SDL_local_auto_trace_object)


#define SDL_DEBUG(logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::DEBUG_LEVEL, logEvent)

#define SDL_INFO(logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::INFO_LEVEL, logEvent)

#define SDL_WARN(logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::WARN_LEVEL, logEvent)

#define SDL_ERROR(logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::ERROR_LEVEL, logEvent)

#define SDL_FATAL(logEvent) \
  LOG_WITH_LEVEL(logger::LogLevel::FATAL_LEVEL, logEvent)

#define SDL_ERROR_WITH_ERRNO(message) \
  SDL_ERROR(message << ", error code " << errno << " (" << strerror(errno) << ")")

#define SDL_WARN_WITH_ERRNO(message) \
  LOG4CXX_WARN(message << ", error code " << errno << " (" << strerror(errno) << ")")

#else  // ENABLE_LOG is OFF

#define SDL_CREATE_LOGGERPTR(logger_name)

#define INIT_LOGGER(file_name, logs_enabled)

#define DEINIT_LOGGER()

#define DELETE_THREAD_LOGGER(logger_var)

#define SDL_FLUSH_LOGGER()

#define SDL_IS_TRACE_ENABLED(logger) false

#define SDL_TRACE(x)

#define LOG4CXX_AUTO_TRACE_WITH_NAME_SPECIFIED(loggerPtr, auto_trace)
#define LOG4CXX_AUTO_TRACE(loggerPtr)

#define SDL_DEBUG(x)

#define SDL_INFO(x)

#define SDL_WARN(x)

#define SDL_ERROR(x)

#define SDL_ERROR_WITH_ERRNO(x)

#define SDL_WARN_WITH_ERRNO(x)

#define SDL_FATAL(x)

#endif  // ENABLE_LOG

#endif  // SRC_COMPONENTS_INCLUDE_UTILS_LOGGER_H_
