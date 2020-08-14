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
 * POSSIBILITY OF SUCH DAMAGE. */

#include "application_manager/mock_application.h"
#include "application_manager/mock_event_dispatcher.h"
#include "application_manager/mock_message_helper.h"
#include "application_manager/mock_rpc_service.h"
#include "application_manager/smart_object_keys.h"
#include "gtest/gtest.h"
#include "mock_application_manager.h"
#include "rc_rpc_plugin/rc_pending_resumption_handler.h"
#include "rc_rpc_plugin/rc_rpc_plugin.h"

namespace rc_rpc_plugin_test {

using ::testing::_;
using ::testing::DoAll;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::SaveArg;

using rc_rpc_plugin::ModuleUid;

using application_manager::MockMessageHelper;
typedef NiceMock< ::test::components::event_engine_test::MockEventDispatcher>
    MockEventDispatcher;

typedef NiceMock<
    ::test::components::application_manager_test::MockApplicationManager>
    MockApplicationManager;
typedef NiceMock< ::test::components::application_manager_test::MockApplication>
    MockApplication;
typedef NiceMock< ::test::components::application_manager_test::MockRPCService>
    MockRPCService;
typedef std::shared_ptr<MockApplication> MockAppPtr;

namespace {
const uint32_t kAppId_1 = 1u;
const uint32_t kAppId_2 = 2u;
const uint32_t kCorrelationId = 10u;
const std::string kModuleType_1 = "CLIMATE";
const std::string kModuleId_1 = "9cb963f3-c5e8-41cb-b001-19421cc16552";
const std::string kModuleType_2 = "RADIO";
const std::string kModuleId_2 = "357a3918-9f35-4d86-a8b6-60cd4308d76f";
const uint32_t kRCPluginID = rc_rpc_plugin::RCRPCPlugin::kRCPluginID;
}  // namespace

class SubscribeCatcher {
 public:
  MOCK_METHOD2(subscribe,
               void(const int32_t, const resumption::ResumptionRequest));
};

/**
 * @brief EventCheck check that event contains apropriate data,
 * check that is matched correlation id,
 * check that function id is correct
 */
MATCHER_P(EventCheck, expected_corr_id, "") {
  namespace strings = application_manager::strings;
  const auto& response_message = arg.smart_object();
  const auto cid =
      response_message[strings::params][strings::correlation_id].asInt();
  const bool cid_ok = (cid == expected_corr_id);
  const auto fid =
      response_message[strings::params][strings::function_id].asInt();
  const bool fid_ok = (fid == hmi_apis::FunctionID::RC_GetInteriorVehicleData);
  return fid_ok && cid_ok;
}

/**
 * @brief MessageCheck check that message contains apropriate data,
 * check that is matched correlation id
 */
MATCHER_P(MessageCheck, correlation_id, "") {
  const auto& request = *arg;
  const auto cid =
      request[app_mngr::strings::params][app_mngr::strings::correlation_id]
          .asInt();
  return cid == correlation_id;
}

class RCPendingResumptionHandlerTest : public ::testing::Test {
 public:
  RCPendingResumptionHandlerTest()
      : mock_message_helper_(*MockMessageHelper::message_helper_mock()) {}

  void SetUp() OVERRIDE {
    ON_CALL(app_manager_mock_, event_dispatcher())
        .WillByDefault(ReturnRef(event_dispatcher_mock_));
    ON_CALL(app_manager_mock_, GetRPCService())
        .WillByDefault(ReturnRef(mock_rpc_service_));
    ON_CALL(app_manager_mock_, GetNextHMICorrelationID())
        .WillByDefault(Return(kCorrelationId));
    resumption_handler_.reset(
        new rc_rpc_plugin::RCPendingResumptionHandler(app_manager_mock_));
  }

  smart_objects::SmartObjectSPtr CreateHMIResponseMessage(
      const application_manager::MessageType& message_type,
      const hmi_apis::Common_Result::eType& response_code,
      uint32_t correlation_id) {
    namespace strings = application_manager::strings;
    namespace hmi_response = application_manager::hmi_response;
    smart_objects::SmartObject params(smart_objects::SmartType_Map);
    params[strings::function_id] =
        hmi_apis::FunctionID::RC_GetInteriorVehicleData;
    params[strings::message_type] = message_type;
    params[strings::correlation_id] = correlation_id;
    const auto hmi_protocol_type = 1;
    params[strings::protocol_type] = hmi_protocol_type;
    params[hmi_response::code] = response_code;

    smart_objects::SmartObjectSPtr response =
        std::make_shared<smart_objects::SmartObject>(
            smart_objects::SmartType_Map);
    auto& message = *response;
    message[strings::params] = params;
    message[strings::msg_params] =
        smart_objects::SmartObject(smart_objects::SmartType_Map);
    return response;
  }

  MockAppPtr CreateApp(uint32_t app_id) {
    auto mock_app = std::make_shared<MockApplication>();
    ON_CALL(app_manager_mock_, application(app_id))
        .WillByDefault(Return(mock_app));
    ON_CALL(*mock_app, app_id()).WillByDefault(Return(app_id));
    return mock_app;
  }

  rc_rpc_plugin::RCAppExtensionPtr CreateExtension(MockApplication& app) {
    auto rc_app_ext = std::make_shared<rc_rpc_plugin::RCAppExtension>(
        kRCPluginID, rc_plugin_, app);
    ON_CALL(app, QueryInterface(kRCPluginID)).WillByDefault(Return(rc_app_ext));
    return rc_app_ext;
  }

  resumption::Subscriber& get_subscriber() {
    static resumption::Subscriber subscriber =
        [this](const int32_t corr_id,
               const resumption::ResumptionRequest resumption_request) {
          this->subscribe_catcher_.subscribe(corr_id, resumption_request);
        };
    return subscriber;
  }

 protected:
  SubscribeCatcher subscribe_catcher_;
  MockMessageHelper& mock_message_helper_;
  MockApplicationManager app_manager_mock_;
  MockEventDispatcher event_dispatcher_mock_;
  MockRPCService mock_rpc_service_;
  rc_rpc_plugin::RCRPCPlugin rc_plugin_;
  std::unique_ptr<rc_rpc_plugin::RCPendingResumptionHandler>
      resumption_handler_;
};

TEST_F(RCPendingResumptionHandlerTest, HandleResumptionNoSubscriptionNoAction) {
  // Create extension with no rc modules
  // call HandleResumptionSubscriptionRequest
  // expect no HMI calls , no subscriptions
  auto mock_app = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app);

  EXPECT_CALL(subscribe_catcher_, subscribe(_, _)).Times(0);
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _)).Times(0);

  resumption::Subscriber& subscribe = get_subscriber();
  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app);
}

TEST_F(RCPendingResumptionHandlerTest,
       HandleResumptionOneSubscriptionOnAction) {
  // Create extension with one module
  // call HandleResumptionSubscriptionRequest
  // expect one HMI call , one subscription

  auto mock_app = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app);

  ModuleUid module_uid{kModuleType_1, kModuleId_1};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid);

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID());
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _));

  resumption::Subscriber& subscribe = get_subscriber();
  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app);
}

TEST_F(RCPendingResumptionHandlerTest,
       HandleResumptionMultipleSubscriptionsMultipleActions) {
  // Create extension with several modules
  // call HandleResumptionSubscriptionRequest
  // expect  several HMI calls , several subscriptions
  auto mock_app = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app);

  ModuleUid module_uid_1 = {kModuleType_1, kModuleId_1};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid_1);

  ModuleUid module_uid_2 = {kModuleType_2, kModuleId_2};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid_2);

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID()).Times(2);
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _)).Times(2);
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _)).Times(2);

  resumption::Subscriber& subscribe = get_subscriber();
  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app);
}

TEST_F(RCPendingResumptionHandlerTest,
       HandleResumptionWithPendingSubscription) {
  // Create extension1 with one module
  // call HandleResumptionSubscriptionRequest
  // ASSERR one HMI call , assert subscription

  // Create extension2 with the same module as in extension1
  // call HandleResumptionSubscriptionRequest for extension2
  // EXPECT no HMI call, expect subscription
  auto mock_app = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app);

  ModuleUid module_uid = {kModuleType_1, kModuleId_1};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid);

  resumption::Subscriber& subscribe = get_subscriber();

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID());
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _));

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app);

  auto rc_app_ext_2 = CreateExtension(*mock_app);
  rc_app_ext_2->SubscribeToInteriorVehicleData(module_uid);

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID());
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _)).Times(0);

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext_2, subscribe, *mock_app);
}

TEST_F(RCPendingResumptionHandlerTest,
       HandleResumptionWithPendingSubscriptionAndNotPendingOne) {
  // Create extension1 with one module
  // call HandleResumptionSubscriptionRequest
  // ASSERR one HMI call , assert subscription

  // Create extension2 with the same module as in extension1 and another one
  // module call HandleResumptionSubscriptionRequest for extension2 EXPECT HMI
  // call for new module EXPECT no HMI call for first module expect subscription
  // for both modules
  auto mock_app = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app);

  ModuleUid module_uid_1 = {kModuleType_1, kModuleId_1};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid_1);

  resumption::Subscriber& subscribe = get_subscriber();

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID());
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _));

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app);

  auto rc_app_ext_2 = CreateExtension(*mock_app);
  ModuleUid module_uid_2 = {kModuleType_2, kModuleId_2};
  rc_app_ext_2->SubscribeToInteriorVehicleData(module_uid_1);
  rc_app_ext_2->SubscribeToInteriorVehicleData(module_uid_2);

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID()).Times(2);
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _)).Times(2);
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _)).Times(1);

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext_2, subscribe, *mock_app);
}

TEST_F(RCPendingResumptionHandlerTest,
       Resumption2ApplicationsWithCommonDataSuccess) {
  // Create extension1 with one module1
  // call HandleResumptionSubscriptionRequest
  // ASSERR one HMI call , assert subscription
  // Create extension2 with the same module1 as in extension1
  // call HandleResumptionSubscriptionRequest for extension2
  // assert creating HMI message for module1 app2
  // assert subscription for module1
  // assert no HMI call for module1

  // call on_event with succesfull response to module1
  // expect raizing event with succesfull response to module1 for app2
  auto mock_app_1 = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app_1);

  ModuleUid module_uid = {kModuleType_1, kModuleId_1};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid);

  resumption::Subscriber& subscribe = get_subscriber();

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID())
      .WillOnce(Return(kAppId_1));
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _));

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app_1);

  auto mock_app_2 = CreateApp(kAppId_2);
  auto rc_app_ext_2 = CreateExtension(*mock_app_2);

  rc_app_ext_2->SubscribeToInteriorVehicleData(module_uid);

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID())
      .WillOnce(Return(kAppId_2));
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _)).Times(0);

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext_2, subscribe, *mock_app_2);

  auto response =
      CreateHMIResponseMessage(application_manager::MessageType::kResponse,
                               hmi_apis::Common_Result::SUCCESS,
                               kAppId_1);

  application_manager::event_engine::Event event(
      hmi_apis::FunctionID::RC_GetInteriorVehicleData);
  event.set_smart_object(*response);

  EXPECT_CALL(event_dispatcher_mock_, raise_event(EventCheck(kAppId_2)));

  resumption_handler_->on_event(event);
}

TEST_F(RCPendingResumptionHandlerTest,
       Resumption2ApplicationsWithCommonDataFailedRetry) {
  // Create extension1 with one module1
  // call HandleResumptionSubscriptionRequest
  // ASSERR one HMI call , assert subscription
  // Create extension2 with the same module1 as in extension1
  // call HandleResumptionSubscriptionRequest for extension2
  // assert creating HMI message for module1 app2
  // assert subscription for module1
  // assert no HMI call for module1

  // call on_event with unsuccessful response to module1
  // Expect HMI request with module1 subscription
  auto mock_app_1 = CreateApp(kAppId_1);
  auto rc_app_ext = CreateExtension(*mock_app_1);

  ModuleUid module_uid = {kModuleType_1, kModuleId_1};
  rc_app_ext->SubscribeToInteriorVehicleData(module_uid);

  resumption::Subscriber& subscribe = get_subscriber();

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID())
      .WillOnce(Return(kAppId_1));
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _));

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext, subscribe, *mock_app_1);

  auto mock_app_2 = CreateApp(kAppId_2);
  auto rc_app_ext_2 = CreateExtension(*mock_app_2);

  rc_app_ext_2->SubscribeToInteriorVehicleData(module_uid);

  EXPECT_CALL(app_manager_mock_, GetNextHMICorrelationID())
      .WillOnce(Return(kAppId_2));
  EXPECT_CALL(subscribe_catcher_, subscribe(_, _));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(_, _)).Times(0);

  resumption_handler_->HandleResumptionSubscriptionRequest(
      *rc_app_ext_2, subscribe, *mock_app_2);

  auto response =
      CreateHMIResponseMessage(application_manager::MessageType::kErrorResponse,
                               hmi_apis::Common_Result::SUCCESS,
                               kAppId_1);

  application_manager::event_engine::Event event(
      hmi_apis::FunctionID::RC_GetInteriorVehicleData);
  event.set_smart_object(*response);

  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(MessageCheck(kAppId_2), _));

  resumption_handler_->on_event(event);
}

}  // namespace rc_rpc_plugin_test