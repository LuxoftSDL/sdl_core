/*
 * Copyright (c) 2015, Ford Motor Company
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
#include <vector>

#include "gmock/gmock.h"
#include "utils/macro.h"

#include "application_manager/event_engine/event_dispatcher.h"
#include "application_manager/mock_application.h"
#include "application_manager/mock_application_manager.h"
#include "application_manager/mock_help_prompt_manager.h"
#include "application_manager/mock_rpc_service.h"
#include "application_manager/policies/policy_handler.h"
#include "application_manager/resumption/resume_ctrl.h"
#include "application_manager/state_controller.h"
#include "policy/mock_policy_settings.h"
#include "utils/custom_string.h"
#include "utils/lock.h"

#include "application_manager/commands/command_impl.h"
#include "policy/policy_table/types.h"
#include "rpc_base/rpc_base_json_inl.h"

#ifdef EXTERNAL_PROPRIETARY_MODE
#include "policy/policy_external/include/policy/policy_types.h"
#endif

namespace test {
namespace components {
namespace application_manager_test {

namespace HmiLanguage = hmi_apis::Common_Language;
namespace HmiResults = hmi_apis::Common_Result;
namespace MobileResults = mobile_apis::Result;

using namespace application_manager;

typedef std::shared_ptr<MockApplication> MockApplicationSharedPtr;
typedef std::vector<std::string> StringArray;
typedef std::shared_ptr<application_manager::Application> ApplicationSharedPtr;

using testing::_;
using testing::AtLeast;
using testing::Return;
using testing::ReturnRef;
using testing::ReturnRefOfCopy;
using testing::SaveArg;

TEST(MessageHelperTestCreate,
     CreateBlockedByPoliciesResponse_SmartObject_Equal) {
  mobile_apis::FunctionID::eType function_id =
      mobile_apis::FunctionID::eType::AddCommandID;
  mobile_apis::Result::eType result = mobile_apis::Result::eType::ABORTED;
  uint32_t correlation_id = 0;
  uint32_t connection_key = 0;
  bool success = false;

  smart_objects::SmartObjectSPtr ptr =
      application_manager::MessageHelper::CreateBlockedByPoliciesResponse(
          function_id, result, correlation_id, connection_key);

  EXPECT_TRUE((bool)ptr);

  smart_objects::SmartObject& obj = *ptr;

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(kResponse, obj[strings::params][strings::message_type].asInt());
  EXPECT_EQ(success, obj[strings::msg_params][strings::success].asBool());
  EXPECT_EQ(result, obj[strings::msg_params][strings::result_code].asInt());
  EXPECT_EQ(correlation_id,
            obj[strings::params][strings::correlation_id].asUInt());
  EXPECT_EQ(connection_key,
            obj[strings::params][strings::connection_key].asUInt());
  EXPECT_EQ(protocol_handler::MajorProtocolVersion::PROTOCOL_VERSION_2,
            obj[strings::params][strings::protocol_version].asInt());
}

TEST(MessageHelperTestCreate, CreateSetAppIcon_SendNullPathImagetype_Equal) {
  std::string path_to_icon = "";
  uint32_t app_id = 0;
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateSetAppIcon(path_to_icon, app_id);

  EXPECT_TRUE((bool)ptr);

  smart_objects::SmartObject& obj = *ptr;

  int image_type = static_cast<int>(mobile_api::ImageType::DYNAMIC);

  EXPECT_EQ(path_to_icon,
            obj[strings::sync_file_name][strings::value].asString());
  EXPECT_EQ(image_type,
            obj[strings::sync_file_name][strings::image_type].asInt());
  EXPECT_EQ(app_id, obj[strings::app_id].asUInt());
}

TEST(MessageHelperTestCreate, CreateSetAppIcon_SendPathImagetype_Equal) {
  std::string path_to_icon = "/qwe/qwe/";
  uint32_t app_id = 10;
  smart_objects::SmartObjectSPtr ptr =
      MessageHelper::CreateSetAppIcon(path_to_icon, app_id);

  EXPECT_TRUE((bool)ptr);

  smart_objects::SmartObject& obj = *ptr;

  int image_type = static_cast<int>(mobile_api::ImageType::DYNAMIC);

  EXPECT_EQ(path_to_icon,
            obj[strings::sync_file_name][strings::value].asString());
  EXPECT_EQ(image_type,
            obj[strings::sync_file_name][strings::image_type].asInt());
  EXPECT_EQ(app_id, obj[strings::app_id].asUInt());
}

TEST(MessageHelperTestCreate,
     CreateGlobalPropertiesRequestsToHMI_SmartObject_EmptyList) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  EXPECT_CALL(*appSharedMock, vr_help_title()).Times(AtLeast(1));
  EXPECT_CALL(*appSharedMock, vr_help()).Times(AtLeast(1));
  EXPECT_CALL(*appSharedMock, help_prompt()).Times(AtLeast(1));
  EXPECT_CALL(*appSharedMock, timeout_prompt()).Times(AtLeast(1));

  std::shared_ptr<MockHelpPromptManager> mock_help_prompt_manager =
      std::make_shared<MockHelpPromptManager>();
  EXPECT_CALL(*appSharedMock, help_prompt_manager())
      .WillRepeatedly(ReturnRef(*mock_help_prompt_manager));
  EXPECT_CALL(*mock_help_prompt_manager, GetSendingType())
      .WillRepeatedly(Return(HelpPromptManager::SendingType::kSendBoth));

  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateGlobalPropertiesRequestsToHMI(appSharedMock, 0u);

  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate,
     CreateGlobalPropertiesRequestsToHMI_SmartObject_NotEmpty) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  smart_objects::SmartObjectSPtr objPtr =
      std::make_shared<smart_objects::SmartObject>();

  (*objPtr)[0][strings::vr_help_title] = "111";
  (*objPtr)[1][strings::vr_help] = "222";
  (*objPtr)[2][strings::keyboard_properties] = "333";
  (*objPtr)[3][strings::menu_title] = "444";
  (*objPtr)[4][strings::menu_icon] = "555";
  (*objPtr)[5][strings::help_prompt] = "666";
  (*objPtr)[6][strings::timeout_prompt] = "777";

  EXPECT_CALL(*appSharedMock, vr_help_title())
      .Times(AtLeast(3))
      .WillRepeatedly(Return(&(*objPtr)[0]));
  EXPECT_CALL(*appSharedMock, vr_help())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(&(*objPtr)[1]));
  EXPECT_CALL(*appSharedMock, help_prompt())
      .Times(AtLeast(3))
      .WillRepeatedly(Return(&(*objPtr)[5]));
  EXPECT_CALL(*appSharedMock, timeout_prompt())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(&(*objPtr)[6]));
  EXPECT_CALL(*appSharedMock, keyboard_props())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(&(*objPtr)[2]));
  EXPECT_CALL(*appSharedMock, menu_title())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(&(*objPtr)[3]));
  EXPECT_CALL(*appSharedMock, menu_icon())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(&(*objPtr)[4]));
  EXPECT_CALL(*appSharedMock, app_id()).WillRepeatedly(Return(0));

  std::shared_ptr<MockHelpPromptManager> mock_help_prompt_manager =
      std::make_shared<MockHelpPromptManager>();
  EXPECT_CALL(*appSharedMock, help_prompt_manager())
      .WillRepeatedly(ReturnRef(*mock_help_prompt_manager));
  EXPECT_CALL(*mock_help_prompt_manager, GetSendingType())
      .WillRepeatedly(Return(HelpPromptManager::SendingType::kSendBoth));

  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateGlobalPropertiesRequestsToHMI(appSharedMock, 0u);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& first = *ptr[0];
  smart_objects::SmartObject& second = *ptr[1];

  EXPECT_EQ((*objPtr)[0], first[strings::msg_params][strings::vr_help_title]);
  EXPECT_EQ((*objPtr)[1], first[strings::msg_params][strings::vr_help]);
  EXPECT_EQ((*objPtr)[2],
            first[strings::msg_params][strings::keyboard_properties]);
  EXPECT_EQ((*objPtr)[3], first[strings::msg_params][strings::menu_title]);
  EXPECT_EQ((*objPtr)[4], first[strings::msg_params][strings::menu_icon]);
  EXPECT_EQ((*objPtr)[5], second[strings::msg_params][strings::help_prompt]);
  EXPECT_EQ((*objPtr)[6], second[strings::msg_params][strings::timeout_prompt]);
}

TEST(MessageHelperTestCreate, CreateShowRequestToHMI_SendSmartObject_Equal) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();

  smart_objects::SmartObjectSPtr smartObjectPtr =
      std::make_shared<smart_objects::SmartObject>();

  const smart_objects::SmartObject& object = *smartObjectPtr;

  EXPECT_CALL(*appSharedMock, show_command())
      .Times(AtLeast(2))
      .WillRepeatedly(Return(&object));

  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateShowRequestToHMI(appSharedMock, 0u);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& obj = *ptr[0];

  int function_id = static_cast<int>(hmi_apis::FunctionID::UI_Show);

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(*smartObjectPtr, obj[strings::msg_params]);
}

TEST(MessageHelperTestCreate,
     CreateAddCommandRequestToHMI_SendSmartObject_Empty) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  ::application_manager::CommandsMap vis;
  DataAccessor<application_manager::CommandsMap> data_accessor(
      vis, std::make_shared<sync_primitives::RecursiveLock>());

  EXPECT_CALL(*appSharedMock, commands_map()).WillOnce(Return(data_accessor));
  application_manager_test::MockApplicationManager mock_application_manager;
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddCommandRequestToHMI(appSharedMock,
                                                  mock_application_manager);

  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate,
     CreateAddCommandRequestToHMI_SendSmartObject_Equal) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  CommandsMap vis;
  DataAccessor<CommandsMap> data_accessor(
      vis, std::make_shared<sync_primitives::RecursiveLock>());
  smart_objects::SmartObjectSPtr smartObjectPtr =
      std::make_shared<smart_objects::SmartObject>();

  smart_objects::SmartObject& object = *smartObjectPtr;

  const uint32_t app_id = 1u;
  const std::string cmd_icon_value = "10";
  const uint32_t cmd_id = 5u;
  const uint32_t internal_id = 1u;

  object[strings::menu_params] = 1;
  object[strings::cmd_icon] = 1;
  object[strings::cmd_icon][strings::value] = cmd_icon_value;
  object[strings::cmd_id] = cmd_id;

  vis.insert(
      std::pair<uint32_t, smart_objects::SmartObject*>(internal_id, &object));

  EXPECT_CALL(*appSharedMock, commands_map()).WillOnce(Return(data_accessor));
  EXPECT_CALL(*appSharedMock, app_id()).WillOnce(Return(app_id));
  application_manager_test::MockApplicationManager mock_application_manager;
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddCommandRequestToHMI(appSharedMock,
                                                  mock_application_manager);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& obj = *ptr[0];

  int function_id = static_cast<int>(hmi_apis::FunctionID::UI_AddCommand);

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(app_id, obj[strings::msg_params][strings::app_id].asUInt());
  EXPECT_EQ(cmd_id, obj[strings::msg_params][strings::cmd_id].asUInt());
  EXPECT_EQ(object[strings::menu_params],
            obj[strings::msg_params][strings::menu_params]);
  EXPECT_EQ(object[strings::cmd_icon],
            obj[strings::msg_params][strings::cmd_icon]);
  EXPECT_EQ(
      cmd_icon_value,
      obj[strings::msg_params][strings::cmd_icon][strings::value].asString());
}

TEST(MessageHelperTestCreate,
     CreateAddVRCommandRequestFromChoiceToHMI_SendEmptyData_EmptyList) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  application_manager::ChoiceSetMap vis;
  DataAccessor< ::application_manager::ChoiceSetMap> data_accessor(
      vis, std::make_shared<sync_primitives::RecursiveLock>());

  EXPECT_CALL(*appSharedMock, choice_set_map()).WillOnce(Return(data_accessor));
  application_manager_test::MockApplicationManager mock_application_manager;
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddVRCommandRequestFromChoiceToHMI(
          appSharedMock, mock_application_manager);

  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate,
     CreateAddVRCommandRequestFromChoiceToHMI_SendObject_EqualList) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  application_manager::ChoiceSetMap vis;
  DataAccessor< ::application_manager::ChoiceSetMap> data_accessor(
      vis, std::make_shared<sync_primitives::RecursiveLock>());
  smart_objects::SmartObjectSPtr smartObjectPtr =
      std::make_shared<smart_objects::SmartObject>();

  smart_objects::SmartObject& object = *smartObjectPtr;

  object[strings::choice_set] = "10";
  object[strings::grammar_id] = 111;
  object[strings::choice_set][0][strings::choice_id] = 1;
  object[strings::choice_set][0][strings::vr_commands] = 2;

  vis.insert(std::pair<uint32_t, smart_objects::SmartObject*>(5, &object));
  vis.insert(std::pair<uint32_t, smart_objects::SmartObject*>(6, &object));
  vis.insert(std::pair<uint32_t, smart_objects::SmartObject*>(7, &object));
  vis.insert(std::pair<uint32_t, smart_objects::SmartObject*>(8, &object));
  vis.insert(std::pair<uint32_t, smart_objects::SmartObject*>(9, &object));

  EXPECT_CALL(*appSharedMock, choice_set_map()).WillOnce(Return(data_accessor));
  EXPECT_CALL(*appSharedMock, app_id())
      .Times(AtLeast(5))
      .WillRepeatedly(Return(1u));
  application_manager_test::MockApplicationManager mock_application_manager;
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddVRCommandRequestFromChoiceToHMI(
          appSharedMock, mock_application_manager);

  EXPECT_FALSE(ptr.empty());

  int function_id = static_cast<int>(hmi_apis::FunctionID::VR_AddCommand);
  int type = static_cast<int>(hmi_apis::Common_VRCommandType::Choice);

  smart_objects::SmartObject& obj = *ptr[0];

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(1u, obj[strings::msg_params][strings::app_id].asUInt());
  EXPECT_EQ(111u, obj[strings::msg_params][strings::grammar_id].asUInt());
  EXPECT_EQ(object[strings::choice_set][0][strings::choice_id],
            obj[strings::msg_params][strings::cmd_id]);
  EXPECT_EQ(object[strings::choice_set][0][strings::vr_commands],
            obj[strings::msg_params][strings::vr_commands]);
  EXPECT_EQ(type, obj[strings::msg_params][strings::type].asInt());
}

TEST(MessageHelperTestCreate, CreateAddSubMenuRequestToHMI_SendObject_Equal) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  application_manager::SubMenuMap vis;
  DataAccessor< ::application_manager::SubMenuMap> data_accessor(
      vis, std::make_shared<sync_primitives::RecursiveLock>());
  smart_objects::SmartObjectSPtr smartObjectPtr =
      std::make_shared<smart_objects::SmartObject>();

  smart_objects::SmartObject& object = *smartObjectPtr;

  object[strings::position] = 1;
  object[strings::menu_name] = 1;

  vis.insert(std::pair<uint32_t, smart_objects::SmartObject*>(5, &object));

  EXPECT_CALL(*appSharedMock, sub_menu_map()).WillOnce(Return(data_accessor));
  EXPECT_CALL(*appSharedMock, app_id()).Times(AtLeast(1)).WillOnce(Return(1u));

  const uint32_t cor_id = 0u;
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddSubMenuRequestToHMI(appSharedMock, cor_id);

  EXPECT_FALSE(ptr.empty());

  smart_objects::SmartObject& obj = *ptr[0];

  int function_id = static_cast<int>(hmi_apis::FunctionID::UI_AddSubMenu);

  EXPECT_EQ(function_id, obj[strings::params][strings::function_id].asInt());
  EXPECT_EQ(5, obj[strings::msg_params][strings::menu_id].asInt());
  EXPECT_EQ(1,
            obj[strings::msg_params][strings::menu_params][strings::position]
                .asInt());
  EXPECT_EQ(1,
            obj[strings::msg_params][strings::menu_params][strings::menu_name]
                .asInt());
  EXPECT_EQ(1u, obj[strings::msg_params][strings::app_id].asUInt());
}

TEST(MessageHelperTestCreate,
     CreateAddSubMenuRequestToHMI_SendEmptyMap_EmptySmartObjectList) {
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  application_manager::SubMenuMap vis;
  DataAccessor< ::application_manager::SubMenuMap> data_accessor(
      vis, std::make_shared<sync_primitives::RecursiveLock>());

  EXPECT_CALL(*appSharedMock, sub_menu_map()).WillOnce(Return(data_accessor));

  const uint32_t cor_id = 0u;
  smart_objects::SmartObjectList ptr =
      MessageHelper::CreateAddSubMenuRequestToHMI(appSharedMock, cor_id);

  EXPECT_TRUE(ptr.empty());
}

TEST(MessageHelperTestCreate, CreateNegativeResponse_SendSmartObject_Equal) {
  uint32_t connection_key = 111;
  int32_t function_id = 222;
  uint32_t correlation_id = 333u;
  int32_t result_code = 0;

  smart_objects::SmartObjectSPtr ptr = MessageHelper::CreateNegativeResponse(
      connection_key, function_id, correlation_id, result_code);

  EXPECT_TRUE((bool)ptr);

  smart_objects::SmartObject& obj = *ptr;

  int objFunction_id = obj[strings::params][strings::function_id].asInt();
  uint32_t objCorrelation_id =
      obj[strings::params][strings::correlation_id].asUInt();
  int objResult_code = obj[strings::msg_params][strings::result_code].asInt();
  uint32_t objConnection_key =
      obj[strings::params][strings::connection_key].asUInt();

  int message_type = static_cast<int>(mobile_apis::messageType::response);
  int protocol_type =
      static_cast<int>(commands::CommandImpl::mobile_protocol_type_);
  int protocol_version =
      static_cast<int>(commands::CommandImpl::protocol_version_);
  bool success = false;

  EXPECT_EQ(function_id, objFunction_id);
  EXPECT_EQ(message_type, obj[strings::params][strings::message_type].asInt());
  EXPECT_EQ(protocol_type,
            obj[strings::params][strings::protocol_type].asInt());
  EXPECT_EQ(protocol_version,
            obj[strings::params][strings::protocol_version].asInt());
  EXPECT_EQ(correlation_id, objCorrelation_id);
  EXPECT_EQ(result_code, objResult_code);
  EXPECT_EQ(success, obj[strings::msg_params][strings::success].asBool());
  EXPECT_EQ(connection_key, objConnection_key);
}

class MessageHelperTest : public ::testing::Test {
 public:
  MessageHelperTest()
      : language_strings{"EN-US", "ES-MX", "FR-CA", "DE-DE", "ES-ES", "EN-GB",
                         "RU-RU", "TR-TR", "PL-PL", "FR-FR", "IT-IT", "SV-SE",
                         "PT-PT", "NL-NL", "EN-AU", "ZH-CN", "ZH-TW", "JA-JP",
                         "AR-SA", "KO-KR", "PT-BR", "CS-CZ", "DA-DK", "NO-NO",
                         "NL-BE", "EL-GR", "HU-HU", "FI-FI", "SK-SK", "EN-IN",
                         "TH-TH", "EN-SA", "HE-IL", "RO-RO", "UK-UA", "ID-ID",
                         "VI-VN", "MS-MY", "HI-IN"}
      , hmi_result_strings{"SUCCESS",
                           "UNSUPPORTED_REQUEST",
                           "UNSUPPORTED_RESOURCE",
                           "DISALLOWED",
                           "REJECTED",
                           "ABORTED",
                           "IGNORED",
                           "RETRY",
                           "IN_USE",
                           "DATA_NOT_AVAILABLE",
                           "TIMED_OUT",
                           "INVALID_DATA",
                           "CHAR_LIMIT_EXCEEDED",
                           "INVALID_ID",
                           "DUPLICATE_NAME",
                           "APPLICATION_NOT_REGISTERED",
                           "WRONG_LANGUAGE",
                           "OUT_OF_MEMORY",
                           "TOO_MANY_PENDING_REQUESTS",
                           "NO_APPS_REGISTERED",
                           "NO_DEVICES_CONNECTED",
                           "WARNINGS",
                           "GENERIC_ERROR",
                           "USER_DISALLOWED",
                           "TRUNCATED_DATA"}
      , mobile_result_strings{"SUCCESS",
                              "UNSUPPORTED_REQUEST",
                              "UNSUPPORTED_RESOURCE",
                              "DISALLOWED",
                              "REJECTED",
                              "ABORTED",
                              "IGNORED",
                              "RETRY",
                              "IN_USE",
                              "VEHICLE_DATA_NOT_AVAILABLE",
                              "TIMED_OUT",
                              "INVALID_DATA",
                              "CHAR_LIMIT_EXCEEDED",
                              "INVALID_ID",
                              "DUPLICATE_NAME",
                              "APPLICATION_NOT_REGISTERED",
                              "WRONG_LANGUAGE",
                              "OUT_OF_MEMORY",
                              "TOO_MANY_PENDING_REQUESTS",
                              "TOO_MANY_APPLICATIONS",
                              "APPLICATION_REGISTERED_ALREADY",
                              "WARNINGS",
                              "GENERIC_ERROR",
                              "USER_DISALLOWED",
                              "UNSUPPORTED_VERSION",
                              "VEHICLE_DATA_NOT_ALLOWED",
                              "FILE_NOT_FOUND",
                              "CANCEL_ROUTE",
                              "TRUNCATED_DATA",
                              "SAVED",
                              "INVALID_CERT",
                              "EXPIRED_CERT",
                              "RESUME_FAILED"}
      , function_id_strings{"RESERVED",
                            "RegisterAppInterface",
                            "UnregisterAppInterface",
                            "SetGlobalProperties",
                            "ResetGlobalProperties",
                            "AddCommand",
                            "DeleteCommand",
                            "AddSubMenu",
                            "DeleteSubMenu",
                            "CreateInteractionChoiceSet",
                            "PerformInteraction",
                            "DeleteInteractionChoiceSet",
                            "Alert",
                            "Show",
                            "Speak",
                            "SetMediaClockTimer",
                            "PerformAudioPassThru",
                            "EndAudioPassThru",
                            "SubscribeButton",
                            "UnsubscribeButton",
                            "SubscribeVehicleData",
                            "UnsubscribeVehicleData",
                            "GetVehicleData",
                            "ReadDID",
                            "GetDTCs",
                            "ScrollableMessage",
                            "Slider",
                            "ShowConstantTBT",
                            "AlertManeuver",
                            "UpdateTurnList",
                            "ChangeRegistration",
                            "GenericResponse",
                            "PutFile",
                            "DeleteFile",
                            "ListFiles",
                            "SetAppIcon",
                            "SetDisplayLayout",
                            "DiagnosticMessage",
                            "SystemRequest",
                            "SendLocation",
                            "DialNumber"}
      , events_id_strings{"OnHMIStatus",
                          "OnAppInterfaceUnregistered",
                          "OnButtonEvent",
                          "OnButtonPress",
                          "OnVehicleData",
                          "OnCommand",
                          "OnTBTClientState",
                          "OnDriverDistraction",
                          "OnPermissionsChange",
                          "OnAudioPassThru",
                          "OnLanguageChange",
                          "OnKeyboardInput",
                          "OnTouchEvent",
                          "OnSystemRequest",
                          "OnHashChange"}
      , hmi_level_strings{"FULL", "LIMITED", "BACKGROUND", "NONE"}
      , delta_from_functions_id(32768)
      , hmi_function_id_strings{
            "Buttons.GetCapabilities",
            "Buttons.ButtonPress",
            "Buttons.OnButtonEvent",
            "Buttons.OnButtonPress",
            "Buttons.OnButtonSubscription",
            "BasicCommunication.OnServiceUpdate",
            "BasicCommunication.GetSystemTime",
            "BasicCommunication.OnSystemTimeReady",
            "BasicCommunication.OnReady",
            "BasicCommunication.OnStartDeviceDiscovery",
            "BasicCommunication.OnUpdateDeviceList",
            "BasicCommunication.OnResumeAudioSource",
            "BasicCommunication.OnSDLPersistenceComplete",
            "BasicCommunication.UpdateAppList",
            "BasicCommunication.UpdateDeviceList",
            "BasicCommunication.OnFileRemoved",
            "BasicCommunication.OnDeviceChosen",
            "BasicCommunication.OnFindApplications",
            "BasicCommunication.ActivateApp",
            "BasicCommunication.CloseApplication",
            "BasicCommunication.OnAppActivated",
            "BasicCommunication.OnAppDeactivated",
            "BasicCommunication.OnAppRegistered",
            "BasicCommunication.OnAppUnregistered",
            "BasicCommunication.OnExitApplication",
            "BasicCommunication.OnExitAllApplications",
            "BasicCommunication.OnAwakeSDL",
            "BasicCommunication.MixingAudioSupported",
            "BasicCommunication.DialNumber",
            "BasicCommunication.OnResetTimeout",
            "BasicCommunication.OnSystemRequest",
            "BasicCommunication.SystemRequest",
            "BasicCommunication.PolicyUpdate",
            "BasicCommunication.OnSDLClose",
            "BasicCommunication.OnPutFile",
            "BasicCommunication.GetFilePath",
            "BasicCommunication.GetSystemInfo",
            "BasicCommunication.OnSystemInfoChanged",
            "BasicCommunication.OnIgnitionCycleOver",
            "BasicCommunication.DecryptCertificate",
            "BasicCommunication.OnEventChanged",
            "BasicCommunication.OnSystemCapabilityUpdated",
            "BasicCommunication.SetAppProperties",
            "BasicCommunication.GetAppProperties",
            "BasicCommunication.OnAppPropertiesChange",
            "VR.IsReady",
            "VR.Started",
            "VR.Stopped",
            "VR.AddCommand",
            "VR.DeleteCommand",
            "VR.PerformInteraction",
            "VR.OnCommand",
            "VR.ChangeRegistration",
            "VR.OnLanguageChange",
            "VR.GetSupportedLanguages",
            "VR.GetLanguage",
            "VR.GetCapabilities",
            "TTS.GetCapabilities",
            "TTS.Started",
            "TTS.Stopped",
            "TTS.IsReady",
            "TTS.Speak",
            "TTS.StopSpeaking",
            "TTS.ChangeRegistration",
            "TTS.OnLanguageChange",
            "TTS.GetSupportedLanguages",
            "TTS.GetLanguage",
            "TTS.SetGlobalProperties",
            "TTS.OnResetTimeout",
            "UI.Alert",
            "UI.SetDisplayLayout",
            "UI.Show",
            "UI.CreateWindow",
            "UI.DeleteWindow",
            "UI.AddCommand",
            "UI.DeleteCommand",
            "UI.AddSubMenu",
            "UI.DeleteSubMenu",
            "UI.ShowAppMenu",
            "UI.PerformInteraction",
            "UI.CancelInteraction",
            "UI.SetMediaClockTimer",
            "UI.SetGlobalProperties",
            "UI.OnCommand",
            "UI.OnSystemContext",
            "UI.GetCapabilities",
            "UI.ChangeRegistration",
            "UI.OnLanguageChange",
            "UI.GetSupportedLanguages",
            "UI.GetLanguage",
            "UI.OnDriverDistraction",
            "UI.SetAppIcon",
            "UI.OnKeyboardInput",
            "UI.OnTouchEvent",
            "UI.Slider",
            "UI.ScrollableMessage",
            "UI.PerformAudioPassThru",
            "UI.EndAudioPassThru",
            "UI.IsReady",
            "UI.ClosePopUp",
            "UI.OnResetTimeout",
            "UI.OnRecordStart",
            "UI.SendHapticData",
            "Navigation.IsReady",
            "Navigation.SendLocation",
            "Navigation.ShowConstantTBT",
            "Navigation.AlertManeuver",
            "Navigation.UpdateTurnList",
            "Navigation.OnTBTClientState",
            "Navigation.SetVideoConfig",
            "Navigation.StartStream",
            "Navigation.StopStream",
            "Navigation.StartAudioStream",
            "Navigation.StopAudioStream",
            "Navigation.OnAudioDataStreaming",
            "Navigation.OnVideoDataStreaming",
            "Navigation.GetWayPoints",
            "Navigation.OnWayPointChange",
            "Navigation.SubscribeWayPoints",
            "Navigation.UnsubscribeWayPoints",
            "VehicleInfo.IsReady",
            "VehicleInfo.GetVehicleType",
            "VehicleInfo.ReadDID",
            "VehicleInfo.GetDTCs",
            "VehicleInfo.DiagnosticMessage",
            "VehicleInfo.SubscribeVehicleData",
            "VehicleInfo.UnsubscribeVehicleData",
            "VehicleInfo.GetVehicleData",
            "VehicleInfo.OnVehicleData",
            "SDL.ActivateApp",
            "SDL.GetUserFriendlyMessage",
            "SDL.OnAllowSDLFunctionality",
            "SDL.OnReceivedPolicyUpdate",
            "SDL.OnPolicyUpdate",
            "SDL.GetListOfPermissions",
            "SDL.OnAppPermissionConsent",
            "SDL.OnAppPermissionChanged",
            "SDL.OnSDLConsentNeeded",
            "SDL.UpdateSDL",
            "SDL.GetStatusUpdate",
            "SDL.OnStatusUpdate",
            "SDL.OnSystemError",
            "SDL.AddStatisticsInfo",
            "SDL.OnDeviceStateChanged",
            "SDL.GetPolicyConfigurationData",
            "RC.IsReady",
            "RC.GetCapabilities",
            "RC.SetGlobalProperties",
            "RC.SetInteriorVehicleData",
            "RC.GetInteriorVehicleData",
            "RC.GetInteriorVehicleDataConsent",
            "RC.OnInteriorVehicleData",
            "RC.OnRemoteControlSettings",
            "RC.OnRCStatus",
        } {}

 protected:
  application_manager_test::MockApplicationManager mock_application_manager;
  application_manager_test::MockRPCService mock_rpc_service_;
  const StringArray language_strings;
  const StringArray hmi_result_strings;
  const StringArray mobile_result_strings;
  const StringArray function_id_strings;
  const StringArray events_id_strings;
  const StringArray hmi_level_strings;
  const size_t delta_from_functions_id;
  const StringArray hmi_function_id_strings;
};

TEST_F(MessageHelperTest,
       CommonLanguageFromString_StringValueOfEnum_CorrectEType) {
  HmiLanguage::eType enum_value;
  HmiLanguage::eType enum_from_string_value;
  // Check all languages >= 0
  for (size_t array_index = 0; array_index < language_strings.size();
       ++array_index) {
    enum_value = static_cast<HmiLanguage::eType>(array_index);
    enum_from_string_value =
        MessageHelper::CommonLanguageFromString(language_strings[array_index]);
    EXPECT_EQ(enum_value, enum_from_string_value);
  }
  // Check InvalidEnum == -1
  enum_value = HmiLanguage::INVALID_ENUM;
  enum_from_string_value = MessageHelper::CommonLanguageFromString("");
  EXPECT_EQ(enum_value, enum_from_string_value);
}

TEST_F(MessageHelperTest,
       CommonLanguageToString_ETypeValueOfEnum_CorrectString) {
  std::string string_from_enum;
  HmiLanguage::eType casted_enum;
  // Check all languages >=0
  for (size_t array_index = 0; array_index < language_strings.size();
       ++array_index) {
    casted_enum = static_cast<HmiLanguage::eType>(array_index);
    string_from_enum = MessageHelper::CommonLanguageToString(casted_enum);
    EXPECT_EQ(language_strings[array_index], string_from_enum);
  }
  // Check InvalidEnum == -1
  string_from_enum =
      MessageHelper::CommonLanguageToString(HmiLanguage::INVALID_ENUM);
  EXPECT_EQ("", string_from_enum);
}

TEST_F(MessageHelperTest, ConvertEnumAPINoCheck_AnyEnumType_AnotherEnumType) {
  hmi_apis::Common_LayoutMode::eType tested_enum_value =
      hmi_apis::Common_LayoutMode::ICON_ONLY;
  hmi_apis::Common_AppHMIType::eType converted =
      MessageHelper::ConvertEnumAPINoCheck<hmi_apis::Common_LayoutMode::eType,
                                           hmi_apis::Common_AppHMIType::eType>(
          tested_enum_value);
  EXPECT_EQ(hmi_apis::Common_AppHMIType::DEFAULT, converted);
}

TEST_F(MessageHelperTest, HMIResultFromString_StringValueOfEnum_CorrectEType) {
  HmiResults::eType enum_value;
  HmiResults::eType enum_from_string_value;
  // Check all results >= 0
  for (size_t array_index = 0; array_index < hmi_result_strings.size();
       ++array_index) {
    enum_value = static_cast<HmiResults::eType>(array_index);
    enum_from_string_value =
        MessageHelper::HMIResultFromString(hmi_result_strings[array_index]);
    EXPECT_EQ(enum_value, enum_from_string_value);
  }
  // Check InvalidEnum == -1
  enum_value = HmiResults::INVALID_ENUM;
  enum_from_string_value = MessageHelper::HMIResultFromString("");
  EXPECT_EQ(enum_value, enum_from_string_value);
}

TEST_F(MessageHelperTest, HMIResultToString_ETypeValueOfEnum_CorrectString) {
  std::string string_from_enum;
  HmiResults::eType casted_enum;
  // Check all results >=0
  for (size_t array_index = 0; array_index < hmi_result_strings.size();
       ++array_index) {
    casted_enum = static_cast<HmiResults::eType>(array_index);
    string_from_enum = MessageHelper::HMIResultToString(casted_enum);
    EXPECT_EQ(hmi_result_strings[array_index], string_from_enum);
  }
  // Check InvalidEnum == -1
  string_from_enum = MessageHelper::HMIResultToString(HmiResults::INVALID_ENUM);
  EXPECT_EQ("", string_from_enum);
}

TEST_F(MessageHelperTest,
       HMIToMobileResult_HmiResultEType_GetCorrectMobileResultEType) {
  MobileResults::eType tested_enum;
  HmiResults::eType casted_hmi_enum;
  MobileResults::eType converted_enum;
  // Check enums >=0
  for (size_t enum_index = 0; enum_index < hmi_result_strings.size();
       ++enum_index) {
    tested_enum =
        MessageHelper::MobileResultFromString(hmi_result_strings[enum_index]);
    casted_hmi_enum = static_cast<HmiResults::eType>(enum_index);
    converted_enum = MessageHelper::HMIToMobileResult(casted_hmi_enum);
    EXPECT_EQ(tested_enum, converted_enum);
  }
  // Check invalid enums == -1
  tested_enum = MobileResults::INVALID_ENUM;
  converted_enum = MessageHelper::HMIToMobileResult(HmiResults::INVALID_ENUM);
  EXPECT_EQ(tested_enum, converted_enum);
  // Check when out of range (true == result.empty())
  casted_hmi_enum = static_cast<HmiResults::eType>(INT_MAX);
  converted_enum = MessageHelper::HMIToMobileResult(casted_hmi_enum);
  EXPECT_EQ(tested_enum, converted_enum);
}

TEST_F(MessageHelperTest, VerifySoftButtonString_WrongStrings_False) {
  const StringArray wrong_strings{"soft_button1\t\ntext",
                                  "soft_button1\\ntext",
                                  "soft_button1\\ttext",
                                  " ",
                                  "soft_button1\t\n",
                                  "soft_button1\\n",
                                  "soft_button1\\t"};
  for (size_t i = 0; i < wrong_strings.size(); ++i) {
    EXPECT_FALSE(MessageHelper::VerifyString(wrong_strings[i]));
  }
}

TEST_F(MessageHelperTest, VerifySoftButtonString_CorrectStrings_True) {
  const StringArray wrong_strings{"soft_button1.text",
                                  "soft_button1?text",
                                  " asd asdasd    .././/",
                                  "soft_button1??....asd",
                                  "soft_button12313fcvzxc./.,"};
  for (size_t i = 0; i < wrong_strings.size(); ++i) {
    EXPECT_TRUE(MessageHelper::VerifyString(wrong_strings[i]));
  }
}

TEST_F(MessageHelperTest,
       ProcessSoftButtons_SmartObjectWithoutButtonsKey_Success) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject object;
  policy_handler_test::MockPolicySettings policy_settings_;
  const policy::PolicyHandler policy_handler(policy_settings_,
                                             mock_application_manager);
  // Method call
  mobile_apis::Result::eType result = MessageHelper::ProcessSoftButtons(
      object, appSharedMock, policy_handler, mock_application_manager);
  // Expect
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F(MessageHelperTest,
       ProcessSoftButtons_IncorectSoftButonValue_InvalidData) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject object;
  smart_objects::SmartObject& buttons = object[strings::soft_buttons];
  // Setting invalid image string to button
  buttons[0][strings::image][strings::value] = "invalid\\nvalue";
  policy_handler_test::MockPolicySettings policy_settings_;
  const policy::PolicyHandler policy_handler(policy_settings_,
                                             mock_application_manager);
  // Method call
  mobile_apis::Result::eType result = MessageHelper::ProcessSoftButtons(
      object, appSharedMock, policy_handler, mock_application_manager);
  // Expect
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F(MessageHelperTest, VerifyImage_ImageTypeIsStatic_Success) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject image;
  image[strings::image_type] = mobile_apis::ImageType::STATIC;
  image[strings::value] = "static_icon";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImage(
      image, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F(MessageHelperTest, VerifyImage_ImageValueNotValid_InvalidData) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject image;
  image[strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  // Invalid value
  image[strings::value] = "   ";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImage(
      image, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F(MessageHelperTest, VerifyImageApplyPath_ImageTypeIsStatic_Success) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject image;
  image[strings::image_type] = mobile_apis::ImageType::STATIC;
  image[strings::value] = "icon.png";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImage(
      image, appSharedMock, mock_application_manager);
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
  // EXPECT
  EXPECT_EQ("icon.png", image[strings::value].asString());
}

TEST_F(MessageHelperTest, VerifyImageApplyPath_ImageValueNotValid_InvalidData) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject image;
  image[strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  // Invalid value
  image[strings::value] = "   ";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImage(
      image, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F(MessageHelperTest, VerifyImageFiles_SmartObjectWithValidData_Success) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject images;
  images[0][strings::image_type] = mobile_apis::ImageType::STATIC;
  images[1][strings::image_type] = mobile_apis::ImageType::STATIC;
  images[0][strings::value] = "static_icon";
  images[1][strings::value] = "static_icon";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImageFiles(
      images, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F(MessageHelperTest,
       VerifyImageFiles_SmartObjectWithInvalidData_NotSuccsess) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject images;
  images[0][strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  images[1][strings::image_type] = mobile_apis::ImageType::DYNAMIC;
  // Invalid values
  images[0][strings::value] = "   ";
  images[1][strings::value] = "image\\n";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImageFiles(
      images, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F(MessageHelperTest,
       VerifyImageVrHelpItems_SmartObjectWithSeveralValidImages_Succsess) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject message;
  message[0][strings::image][strings::image_type] =
      mobile_apis::ImageType::STATIC;
  message[1][strings::image][strings::image_type] =
      mobile_apis::ImageType::STATIC;

  message[0][strings::image][strings::value] = "static_icon";
  message[1][strings::image][strings::value] = "static_icon";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImageVrHelpItems(
      message, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::SUCCESS, result);
}

TEST_F(MessageHelperTest,
       VerifyImageVrHelpItems_SmartObjWithSeveralInvalidImages_NotSuccsess) {
  // Creating std::shared_ptr to MockApplication
  MockApplicationSharedPtr appSharedMock = std::make_shared<MockApplication>();
  // Creating input data for method
  smart_objects::SmartObject message;
  message[0][strings::image][strings::image_type] =
      mobile_apis::ImageType::DYNAMIC;
  message[1][strings::image][strings::image_type] =
      mobile_apis::ImageType::DYNAMIC;
  // Invalid values
  message[0][strings::image][strings::value] = "   ";
  message[1][strings::image][strings::value] = "image\\n";
  // Method call
  mobile_apis::Result::eType result = MessageHelper::VerifyImageVrHelpItems(
      message, appSharedMock, mock_application_manager);
  // EXPECT
  EXPECT_EQ(mobile_apis::Result::INVALID_DATA, result);
}

TEST_F(MessageHelperTest,
       StringifiedFunctionID_FinctionId_EqualsWithStringsInArray) {
  // Start from 1 because 1 == RESERVED and haven`t ID in last 2 characters
  // if FUNCTION ID == 1 inner DCHECK is false
  mobile_apis::FunctionID::eType casted_enum;
  std::string converted;
  for (size_t i = 1; i < function_id_strings.size(); ++i) {
    casted_enum = static_cast<mobile_apis::FunctionID::eType>(i);
    converted = MessageHelper::StringifiedFunctionID(casted_enum);
    EXPECT_EQ(function_id_strings[i], converted);
  }
  // EventIDs emum strarts from delta_from_functions_id = 32768
  for (size_t i = delta_from_functions_id;
       i < events_id_strings.size() + delta_from_functions_id;
       ++i) {
    casted_enum = static_cast<mobile_apis::FunctionID::eType>(i);
    converted = MessageHelper::StringifiedFunctionID(casted_enum);
    EXPECT_EQ(events_id_strings[i - delta_from_functions_id], converted);
  }
}

TEST_F(MessageHelperTest,
       StringifiedHmiLevel_LevelEnum_EqualsWithStringsInArray) {
  mobile_apis::HMILevel::eType casted_enum;
  std::string converted_value;
  for (size_t i = 0; i < hmi_level_strings.size(); ++i) {
    casted_enum = static_cast<mobile_apis::HMILevel::eType>(i);
    converted_value = MessageHelper::StringifiedHMILevel(casted_enum);
    EXPECT_EQ(hmi_level_strings[i], converted_value);
  }
}

TEST_F(MessageHelperTest, StringToHmiLevel_LevelString_EqEType) {
  mobile_apis::HMILevel::eType tested_enum;
  mobile_apis::HMILevel::eType converted_enum;
  for (size_t i = 0; i < hmi_level_strings.size(); ++i) {
    tested_enum = static_cast<mobile_apis::HMILevel::eType>(i);
    converted_enum = MessageHelper::StringToHMILevel(hmi_level_strings[i]);
    EXPECT_EQ(tested_enum, converted_enum);
  }
}

TEST_F(MessageHelperTest, SubscribeApplicationToSoftButton_CallFromApp) {
  // Create application mock
  MockApplicationSharedPtr appSharedPtr = std::make_shared<MockApplication>();
  // Prepare data for method
  smart_objects::SmartObject message_params;
  size_t function_id = 1;
  //
  EXPECT_CALL(*appSharedPtr,
              SubscribeToSoftButtons(function_id, SoftButtonID()))
      .Times(1);
  MessageHelper::SubscribeApplicationToSoftButton(
      message_params, appSharedPtr, function_id);
}
#ifdef EXTERNAL_PROPRIETARY_MODE
TEST_F(MessageHelperTest, SendGetListOfPermissionsResponse_SUCCESS) {
  std::vector<policy::FunctionalGroupPermission> permissions;
  policy::ExternalConsentStatus external_consent_status;
  policy::FunctionalGroupPermission permission;
  permission.state = policy::GroupConsent::kGroupAllowed;
  permissions.push_back(permission);

  smart_objects::SmartObjectSPtr result;

  ON_CALL(mock_application_manager, GetRPCService())
      .WillByDefault(ReturnRef(mock_rpc_service_));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&result), Return(true)));

  const uint32_t correlation_id = 0u;
  MessageHelper::SendGetListOfPermissionsResponse(permissions,
                                                  external_consent_status,
                                                  correlation_id,
                                                  mock_application_manager);

  ASSERT_TRUE(result.get());

  EXPECT_EQ(hmi_apis::FunctionID::SDL_GetListOfPermissions,
            (*result)[strings::params][strings::function_id].asInt());

  smart_objects::SmartObject& msg_params = (*result)[strings::msg_params];
  const std::string external_consent_status_key = "externalConsentStatus";
  EXPECT_TRUE(msg_params.keyExists(external_consent_status_key));
  EXPECT_TRUE(msg_params[external_consent_status_key].empty());
}

TEST_F(MessageHelperTest,
       SendGetListOfPermissionsResponse_ExternalConsentStatusNonEmpty_SUCCESS) {
  std::vector<policy::FunctionalGroupPermission> permissions;

  policy::ExternalConsentStatus external_consent_status;
  const int32_t entity_type_1 = 1;
  const int32_t entity_id_1 = 2;
  const policy::EntityStatus entity_status_1 = policy::kStatusOn;
  const policy::EntityStatus entity_status_2 = policy::kStatusOff;
  const int32_t entity_type_2 = 3;
  const int32_t entity_id_2 = 4;
  external_consent_status.insert(policy::ExternalConsentStatusItem(
      entity_type_1, entity_id_1, entity_status_1));
  external_consent_status.insert(policy::ExternalConsentStatusItem(
      entity_type_2, entity_id_2, entity_status_2));

  smart_objects::SmartObjectSPtr result;

  ON_CALL(mock_application_manager, GetRPCService())
      .WillByDefault(ReturnRef(mock_rpc_service_));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&result), Return(true)));

  const uint32_t correlation_id = 0u;
  MessageHelper::SendGetListOfPermissionsResponse(permissions,
                                                  external_consent_status,
                                                  correlation_id,
                                                  mock_application_manager);

  ASSERT_TRUE(result.get());

  smart_objects::SmartObject& msg_params = (*result)[strings::msg_params];
  const std::string external_consent_status_key = "externalConsentStatus";
  EXPECT_TRUE(msg_params.keyExists(external_consent_status_key));

  smart_objects::SmartArray* status_array =
      msg_params[external_consent_status_key].asArray();
  EXPECT_TRUE(external_consent_status.size() == status_array->size());

  const std::string entityType = "entityType";
  const std::string entityID = "entityID";
  const std::string status = "status";

  smart_objects::SmartObject item_1_so =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  item_1_so[entityType] = entity_type_1;
  item_1_so[entityID] = entity_id_1;
  item_1_so[status] = hmi_apis::Common_EntityStatus::ON;

  smart_objects::SmartObject item_2_so =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  item_2_so[entityType] = entity_type_2;
  item_2_so[entityID] = entity_id_2;
  item_2_so[status] = hmi_apis::Common_EntityStatus::OFF;

  EXPECT_TRUE(status_array->end() !=
              std::find(status_array->begin(), status_array->end(), item_1_so));
  EXPECT_TRUE(status_array->end() !=
              std::find(status_array->begin(), status_array->end(), item_2_so));
}
#endif

TEST_F(MessageHelperTest, SendNaviSetVideoConfigRequest) {
  smart_objects::SmartObjectSPtr result;
  ON_CALL(mock_application_manager, GetRPCService())
      .WillByDefault(ReturnRef(mock_rpc_service_));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _))
      .WillOnce(DoAll(SaveArg<0>(&result), Return(true)));

  int32_t app_id = 123;
  smart_objects::SmartObject video_params(smart_objects::SmartType_Map);
  video_params[strings::protocol] =
      hmi_apis::Common_VideoStreamingProtocol::RTP;
  video_params[strings::codec] = hmi_apis::Common_VideoStreamingCodec::H264;
  video_params[strings::width] = 640;
  video_params[strings::height] = 480;

  MessageHelper::SendNaviSetVideoConfig(
      app_id, mock_application_manager, video_params);

  EXPECT_EQ(hmi_apis::FunctionID::Navigation_SetVideoConfig,
            (*result)[strings::params][strings::function_id].asInt());

  smart_objects::SmartObject& msg_params = (*result)[strings::msg_params];
  EXPECT_TRUE(msg_params.keyExists(strings::config));

  EXPECT_TRUE(msg_params[strings::config].keyExists(strings::protocol));
  EXPECT_EQ(1, msg_params[strings::config][strings::protocol].asInt());
  EXPECT_TRUE(msg_params[strings::config].keyExists(strings::codec));
  EXPECT_EQ(0, msg_params[strings::config][strings::codec].asInt());
  EXPECT_TRUE(msg_params[strings::config].keyExists(strings::width));
  EXPECT_EQ(640, msg_params[strings::config][strings::width].asInt());
  EXPECT_TRUE(msg_params[strings::config].keyExists(strings::height));
  EXPECT_EQ(480, msg_params[strings::config][strings::height].asInt());
}

TEST_F(MessageHelperTest, ExtractWindowIdFromSmartObject_SUCCESS) {
  const WindowID window_id = 145;
  smart_objects::SmartObject message(smart_objects::SmartType_Map);
  message[strings::msg_params][strings::window_id] = window_id;
  EXPECT_EQ(window_id,
            MessageHelper::ExtractWindowIdFromSmartObject(
                message[strings::msg_params]));
}

TEST_F(MessageHelperTest, ExtractWindowIdFromSmartObject_FromEmptyMessage) {
  smart_objects::SmartObject message(smart_objects::SmartType_Map);
  EXPECT_EQ(mobile_apis::PredefinedWindows::DEFAULT_WINDOW,
            MessageHelper::ExtractWindowIdFromSmartObject(message));
}

TEST_F(MessageHelperTest, ExtractWindowIdFromSmartObject_FromWrongType) {
  smart_objects::SmartObject message(smart_objects::SmartType_Array);
  EXPECT_EQ(mobile_apis::PredefinedWindows::DEFAULT_WINDOW,
            MessageHelper::ExtractWindowIdFromSmartObject(message));
}

TEST_F(MessageHelperTest, HMIFunctionIDFromString) {
  hmi_apis::FunctionID::eType enum_value;
  hmi_apis::FunctionID::eType enum_from_string_value;
  // Check function_ids >= 0
  for (size_t array_index = 0; array_index < hmi_function_id_strings.size();
       ++array_index) {
    enum_value = static_cast<hmi_apis::FunctionID::eType>(array_index);
    enum_from_string_value = MessageHelper::HMIFunctionIDFromString(
        hmi_function_id_strings[array_index]);
    EXPECT_EQ(enum_value, enum_from_string_value);
  }
  // Check InvalidEnum == -1
  enum_value = hmi_apis::FunctionID::eType::INVALID_ENUM;
  enum_from_string_value = MessageHelper::HMIFunctionIDFromString("");
  EXPECT_EQ(enum_value, enum_from_string_value);
}

}  // namespace application_manager_test
}  // namespace components
}  // namespace test
