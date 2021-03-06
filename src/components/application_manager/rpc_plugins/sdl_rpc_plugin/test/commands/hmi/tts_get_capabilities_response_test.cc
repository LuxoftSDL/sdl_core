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

#include <string>

#include "application_manager/commands/commands_test.h"
#include "application_manager/mock_hmi_capabilities.h"
#include "gtest/gtest.h"
#include "hmi/tts_get_capabilities_response.h"
#include "smart_objects/smart_object.h"

namespace test {
namespace components {
namespace commands_test {
namespace hmi_commands_test {
namespace tts_get_capabilities_response {

using application_manager::commands::MessageSharedPtr;
using sdl_rpc_plugin::commands::TTSGetCapabilitiesResponse;
using test::components::application_manager_test::MockHMICapabilities;

using testing::_;

namespace strings = ::application_manager::strings;
namespace hmi_response = ::application_manager::hmi_response;
namespace hmi_interface = ::application_manager::hmi_interface;

namespace {
const std::string kText{"TEXT"};
const hmi_apis::Common_Result::eType kSuccess =
    hmi_apis::Common_Result::SUCCESS;
}  // namespace

class TTSGetCapabilitiesResponseTest
    : public CommandsTest<CommandsTestMocks::kIsNice> {};

TEST_F(TTSGetCapabilitiesResponseTest, Run_BothExist_SUCCESS) {
  MessageSharedPtr msg = CreateMessage();
  (*msg)[strings::params][hmi_response::code] = kSuccess;
  (*msg)[strings::msg_params][hmi_response::speech_capabilities] = kText;
  (*msg)[strings::msg_params][hmi_response::prerecorded_speech_capabilities] =
      kText;

  EXPECT_CALL(mock_hmi_capabilities_,
              set_speech_capabilities(SmartObject(kText)));
  EXPECT_CALL(mock_hmi_capabilities_,
              set_prerecorded_speech(SmartObject(kText)));
  EXPECT_CALL(mock_hmi_capabilities_,
              SaveCachedCapabilitiesToFile(hmi_interface::tts, _, _));

  std::shared_ptr<TTSGetCapabilitiesResponse> command(
      CreateCommand<TTSGetCapabilitiesResponse>(msg));
  ASSERT_TRUE(command->Init());

  command->Run();
}

TEST_F(TTSGetCapabilitiesResponseTest, Run_OnlySpeech_SUCCESS) {
  MessageSharedPtr msg = CreateMessage();
  (*msg)[strings::params][hmi_response::code] = kSuccess;
  (*msg)[strings::msg_params][hmi_response::speech_capabilities] = kText;

  EXPECT_CALL(mock_hmi_capabilities_,
              set_speech_capabilities(SmartObject(kText)));
  EXPECT_CALL(mock_hmi_capabilities_, set_prerecorded_speech(_)).Times(0);
  EXPECT_CALL(mock_hmi_capabilities_,
              SaveCachedCapabilitiesToFile(hmi_interface::tts, _, _));

  std::shared_ptr<TTSGetCapabilitiesResponse> command(
      CreateCommand<TTSGetCapabilitiesResponse>(msg));
  ASSERT_TRUE(command->Init());

  command->Run();
}

TEST_F(TTSGetCapabilitiesResponseTest, Run_OnlyPrerecorded_SUCCESS) {
  MessageSharedPtr msg = CreateMessage();
  (*msg)[strings::params][hmi_response::code] = kSuccess;
  (*msg)[strings::msg_params][hmi_response::prerecorded_speech_capabilities] =
      kText;

  EXPECT_CALL(mock_hmi_capabilities_, set_speech_capabilities(_)).Times(0);
  EXPECT_CALL(mock_hmi_capabilities_,
              set_prerecorded_speech(SmartObject(kText)));
  EXPECT_CALL(mock_hmi_capabilities_,
              SaveCachedCapabilitiesToFile(hmi_interface::tts, _, _));

  std::shared_ptr<TTSGetCapabilitiesResponse> command(
      CreateCommand<TTSGetCapabilitiesResponse>(msg));
  ASSERT_TRUE(command->Init());

  command->Run();
}

TEST_F(TTSGetCapabilitiesResponseTest, Run_Nothing_SUCCESS) {
  MessageSharedPtr msg = CreateMessage();
  (*msg)[strings::params][hmi_response::code] = kSuccess;

  EXPECT_CALL(mock_hmi_capabilities_, set_speech_capabilities(_)).Times(0);
  EXPECT_CALL(mock_hmi_capabilities_, set_prerecorded_speech(_)).Times(0);
  EXPECT_CALL(mock_hmi_capabilities_,
              SaveCachedCapabilitiesToFile(hmi_interface::tts, _, _));

  std::shared_ptr<TTSGetCapabilitiesResponse> command(
      CreateCommand<TTSGetCapabilitiesResponse>(msg));
  ASSERT_TRUE(command->Init());

  command->Run();
}

TEST_F(TTSGetCapabilitiesResponseTest,
       onTimeOut_Run_ResponseForInterface_ReceivedError) {
  MessageSharedPtr msg = CreateMessage();
  (*msg)[strings::params][hmi_response::code] =
      hmi_apis::Common_Result::ABORTED;

  std::shared_ptr<TTSGetCapabilitiesResponse> command(
      CreateCommand<TTSGetCapabilitiesResponse>(msg));

  EXPECT_CALL(mock_hmi_capabilities_,
              UpdateRequestsRequiredForCapabilities(
                  hmi_apis::FunctionID::TTS_GetCapabilities));
  ASSERT_TRUE(command->Init());

  command->Run();
}

}  // namespace tts_get_capabilities_response
}  // namespace hmi_commands_test
}  // namespace commands_test
}  // namespace components
}  // namespace test
