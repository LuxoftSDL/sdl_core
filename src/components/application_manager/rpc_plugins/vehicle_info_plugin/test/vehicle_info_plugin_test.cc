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

#include "gtest/gtest.h"

#include "application_manager/mock_application.h"
#include "application_manager/mock_hmi_capabilities.h"
#include "application_manager/mock_message_helper.h"
#include "application_manager/mock_rpc_service.h"
#include "application_manager/policies/mock_policy_handler_interface.h"
#include "application_manager/smart_object_keys.h"
#include "test/application_manager/mock_application_manager.h"

namespace app_manager_test = test::components::application_manager_test;
namespace vehicle_info_plugin_test {

using ::testing::_;
using ::testing::Mock;
using ::testing::NiceMock;
using ::testing::Return;
using ::testing::ReturnRef;

namespace {
const std::string kVehicleData = "speed";
const unsigned kExtensionUID =
    VehicleInfoAppExtension::VehicleInfoAppExtensionUID;
}  // namespace

class VehicleInfoPluginTest : public ::testing::Test {
 public:
  VehicleInfoPluginTest()
      : vehicle_info_plugin_(nullptr)
      , mock_message_helper_(
            *application_manager::MockMessageHelper::message_helper_mock())
      , applications_lock_(std::make_shared<sync_primitives::Lock>())
      , applications_(application_set_, applications_lock_)
      , mock_app_ptr_(nullptr) {
    Mock::VerifyAndClearExpectations(&mock_message_helper_);
  }

  void SetUp() {
    vehicle_info_plugin_ = std::make_shared<VehicleInfoPlugin>();
    vehicle_info_plugin_->Init(app_manager_mock_,
                               mock_rpc_service_,
                               mock_hmi_capabilities_,
                               mock_policy_handler_,
                               nullptr);
    mock_app_ptr_ =
        std::make_shared<NiceMock<app_manager_test::MockApplication> >();
  }

  ~VehicleInfoPluginTest() {
    Mock::VerifyAndClearExpectations(&mock_message_helper_);
    Mock::VerifyAndClearExpectations(&app_manager_mock_);
  }

  std::shared_ptr<VehicleInfoPlugin> vehicle_info_plugin_;
  NiceMock<app_manager_test::MockApplicationManager> app_manager_mock_;
  NiceMock<app_manager_test::MockRPCService> mock_rpc_service_;
  NiceMock<app_manager_test::MockHMICapabilities> mock_hmi_capabilities_;
  NiceMock<test::components::policy_test::MockPolicyHandlerInterface>
      mock_policy_handler_;
  MockMessageHelper& mock_message_helper_;
  application_manager::ApplicationSet application_set_;
  std::shared_ptr<sync_primitives::Lock> applications_lock_;
  DataAccessor<application_manager::ApplicationSet> applications_;
  std::shared_ptr<NiceMock<app_manager_test::MockApplication> > mock_app_ptr_;
};

TEST_F(VehicleInfoPluginTest, OnPolicyEvent_NoSubscriptions_Unsuccess) {
  EXPECT_CALL(app_manager_mock_, applications())
      .WillOnce(Return(applications_));
  EXPECT_CALL(app_manager_mock_, GetRPCService()).Times(0);

  vehicle_info_plugin_->OnPolicyEvent(
      plugins::PolicyEvent::kApplicationsDisabled);
}

TEST_F(VehicleInfoPluginTest, OnPolicyEvent_SubscriptionExists_Unsuccess) {
  application_manager::ApplicationSet application_set;
  application_set.insert(mock_app_ptr_);
  auto applications_lock = std::make_shared<sync_primitives::Lock>();
  DataAccessor<application_manager::ApplicationSet> applications(
      application_set, applications_lock);
  EXPECT_CALL(app_manager_mock_, applications()).WillOnce(Return(applications));

  VehicleInfoAppExtension app_extension(*vehicle_info_plugin_, *mock_app_ptr_);
  auto extension_ptr = std::make_shared<VehicleInfoAppExtension>(app_extension);
  ON_CALL(*mock_app_ptr_, QueryInterface(kExtensionUID))
      .WillByDefault(Return(extension_ptr));

  EXPECT_CALL(app_manager_mock_, GetRPCService()).Times(0);

  vehicle_info_plugin_->OnPolicyEvent(
      plugins::PolicyEvent::kApplicationsDisabled);
}

TEST_F(VehicleInfoPluginTest, OnApplicationEvent_AppRegistered_Success) {
  EXPECT_CALL(*mock_app_ptr_, AddExtension(_)).WillOnce(Return(true));
  vehicle_info_plugin_->OnApplicationEvent(
      plugins::ApplicationEvent::kApplicationRegistered, mock_app_ptr_);
}

TEST_F(VehicleInfoPluginTest,
       OnApplicationEvent_AppUnregisteredNoSubscriptions_Unsuccess) {
  VehicleInfoAppExtension app_extension(*vehicle_info_plugin_, *mock_app_ptr_);
  auto extension_ptr = std::make_shared<VehicleInfoAppExtension>(app_extension);
  ON_CALL(*mock_app_ptr_, QueryInterface(kExtensionUID))
      .WillByDefault(Return(extension_ptr));

  EXPECT_CALL(app_manager_mock_, GetRPCService()).Times(0);
  vehicle_info_plugin_->OnApplicationEvent(
      plugins::ApplicationEvent::kApplicationUnregistered, mock_app_ptr_);
}

TEST_F(VehicleInfoPluginTest,
       OnApplicationEvent_DeleteAppDataSendCommandsToHmi_Success) {
  VehicleInfoAppExtension app_extension(*vehicle_info_plugin_, *mock_app_ptr_);
  app_extension.subscribeToVehicleInfo(kVehicleData);
  auto extension_ptr = std::make_shared<VehicleInfoAppExtension>(app_extension);
  ON_CALL(*mock_app_ptr_, QueryInterface(kExtensionUID))
      .WillByDefault(Return(extension_ptr));

  EXPECT_CALL(app_manager_mock_, applications())
      .WillOnce(Return(applications_));

  application_manager::VehicleData speed_data;
  speed_data[kVehicleData] = mobile_apis::VehicleDataType::VEHICLEDATA_SPEED;
  ON_CALL(mock_message_helper_, vehicle_data())
      .WillByDefault(ReturnRef(speed_data));
  const uint32_t next_corr_id = 2u;
  ON_CALL(app_manager_mock_, GetNextHMICorrelationID())
      .WillByDefault(Return(next_corr_id));

  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  auto request = std::make_shared<smart_objects::SmartObject>(msg_params);
  EXPECT_CALL(mock_message_helper_,
              CreateMessageForHMI(hmi_apis::messageType::request, next_corr_id))
      .WillOnce(Return(request));

  EXPECT_CALL(app_manager_mock_, GetRPCService())
      .WillOnce(ReturnRef(mock_rpc_service_));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(request, _))
      .WillOnce(Return(true));

  vehicle_info_plugin_->OnApplicationEvent(
      plugins::ApplicationEvent::kDeleteApplicationData, mock_app_ptr_);
}

TEST_F(VehicleInfoPluginTest,
       OnApplicationEvent_DeleteAppDataAnotherAppStillSubscribed_Unsuccess) {
  VehicleInfoAppExtension app_extension(*vehicle_info_plugin_, *mock_app_ptr_);
  app_extension.subscribeToVehicleInfo(kVehicleData);
  auto extension_ptr = std::make_shared<VehicleInfoAppExtension>(app_extension);
  EXPECT_CALL(*mock_app_ptr_, QueryInterface(kExtensionUID))
      .WillOnce(Return(extension_ptr));

  auto another_app_ptr =
      std::make_shared<NiceMock<app_manager_test::MockApplication> >();
  application_manager::ApplicationSet application_set;
  application_set.insert(another_app_ptr);
  auto applications_lock = std::make_shared<sync_primitives::Lock>();
  DataAccessor<application_manager::ApplicationSet> applications(
      application_set, applications_lock);
  EXPECT_CALL(app_manager_mock_, applications()).WillOnce(Return(applications));

  VehicleInfoAppExtension another_extension(*vehicle_info_plugin_,
                                            *another_app_ptr);
  another_extension.subscribeToVehicleInfo(kVehicleData);
  auto another_extension_ptr =
      std::make_shared<VehicleInfoAppExtension>(another_extension);
  EXPECT_CALL(*another_app_ptr, QueryInterface(kExtensionUID))
      .WillOnce(Return(another_extension_ptr));

  EXPECT_CALL(app_manager_mock_, GetRPCService()).Times(0);
  vehicle_info_plugin_->OnApplicationEvent(
      plugins::ApplicationEvent::kDeleteApplicationData, mock_app_ptr_);
}

TEST_F(VehicleInfoPluginTest,
       ProcessResumptionSubscription_NoSubscriptionsNoCommand_Unsuccess) {
  VehicleInfoAppExtension extension(*vehicle_info_plugin_, *mock_app_ptr_);

  EXPECT_CALL(app_manager_mock_, GetRPCService()).Times(0);
  vehicle_info_plugin_->ProcessResumptionSubscription(*mock_app_ptr_,
                                                      extension);
}

TEST_F(VehicleInfoPluginTest,
       ProcessResumptionSubscription_CommandCreated_Success) {
  VehicleInfoAppExtension extension(*vehicle_info_plugin_, *mock_app_ptr_);
  extension.subscribeToVehicleInfo(kVehicleData);

  smart_objects::SmartObject msg_params =
      smart_objects::SmartObject(smart_objects::SmartType_Map);
  auto request = std::make_shared<smart_objects::SmartObject>(msg_params);

  EXPECT_CALL(mock_message_helper_,
              CreateModuleInfoSO(
                  hmi_apis::FunctionID::VehicleInfo_SubscribeVehicleData, _))
      .WillOnce(Return(request));
  EXPECT_CALL(app_manager_mock_, GetRPCService())
      .WillOnce(ReturnRef(mock_rpc_service_));
  EXPECT_CALL(mock_rpc_service_, ManageHMICommand(request, _))
      .WillOnce(Return(true));

  vehicle_info_plugin_->ProcessResumptionSubscription(*mock_app_ptr_,
                                                      extension);
}

}  // namespace vehicle_info_plugin_test
