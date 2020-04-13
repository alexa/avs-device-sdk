/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <chrono>
#include <memory>
#include <string>

#include <gtest/gtest.h>

#include <System/ReportStateHandler.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <AVSCommon/SDKInterfaces/MockExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/MockMessageSender.h>
#include <AVSCommon/SDKInterfaces/MockAVSConnectionManager.h>
#include <AVSCommon/SDKInterfaces/Storage/StubMiscStorage.h>
#include <AVSCommon/SDKInterfaces/MockDirectiveHandlerResult.h>
#include <AVSCommon/AVS/AVSMessageHeader.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>

using alexaClientSDK::avsCommon::avs::AVSDirective;
using alexaClientSDK::avsCommon::avs::AVSMessageHeader;
using alexaClientSDK::avsCommon::avs::MessageRequest;
using alexaClientSDK::avsCommon::avs::attachment::AttachmentManager;
using alexaClientSDK::avsCommon::sdkInterfaces::MessageRequestObserverInterface;
using alexaClientSDK::avsCommon::sdkInterfaces::storage::test::StubMiscStorage;
using alexaClientSDK::avsCommon::sdkInterfaces::test::MockAVSConnectionManager;
using alexaClientSDK::avsCommon::sdkInterfaces::test::MockDirectiveHandlerResult;
using alexaClientSDK::avsCommon::sdkInterfaces::test::MockExceptionEncounteredSender;
using alexaClientSDK::avsCommon::sdkInterfaces::test::MockMessageSender;
using alexaClientSDK::registrationManager::CustomerDataManager;

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {
namespace test {

/// Fake message ID used for testing.
static const std::string TEST_MESSAGE_ID("c88f970a-3519-4ecb-bdcc-0488aca22b87");

/// Fake request ID used for testing.
static const std::string TEST_REQUEST_ID("4b73575e-2e7d-425b-bfa4-c6615e0fbd43");

/// Fake context ID used for testing.
static const std::string TEST_CONTEXT_ID("71c967d8-ad58-47b0-924d-b752deb75e4e");

class MockStateReportGenerator : public StateReportGenerator {
public:
    MockStateReportGenerator(std::function<std::string()> reportFunction) : StateReportGenerator({reportFunction}) {
    }
};

class ReportStateHandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        m_customerDataManager = std::make_shared<CustomerDataManager>();
        m_mockExceptionEncounteredSender = std::make_shared<MockExceptionEncounteredSender>();
        m_mockAVSConnectionManager = std::make_shared<::testing::NiceMock<MockAVSConnectionManager>>();
        m_mockMessageSender = std::make_shared<MockMessageSender>();
        m_stubMiscStorage = StubMiscStorage::create();

        m_attachmentManager = std::make_shared<AttachmentManager>(AttachmentManager::AttachmentType::IN_PROCESS);
        m_mockDirectiveHandlerResult = std::unique_ptr<MockDirectiveHandlerResult>(new MockDirectiveHandlerResult);
    }

    void waitUntilEventSent() {
        std::unique_lock<std::mutex> lock(m_mutex);
        // This will take 2 seconds only in case it fails (normally it exits right away).
        ASSERT_TRUE(m_condition.wait_for(
            lock, std::chrono::seconds(2), [this] { return m_directiveCompleted && m_eventSent; }));
    }

    std::shared_ptr<ReportStateHandler> m_unit;
    std::shared_ptr<CustomerDataManager> m_customerDataManager;
    std::shared_ptr<MockExceptionEncounteredSender> m_mockExceptionEncounteredSender;
    std::shared_ptr<MockMessageSender> m_mockMessageSender;
    std::shared_ptr<::testing::NiceMock<MockAVSConnectionManager>> m_mockAVSConnectionManager;
    std::shared_ptr<StubMiscStorage> m_stubMiscStorage;
    std::vector<StateReportGenerator> generators;
    std::shared_ptr<AttachmentManager> m_attachmentManager;
    std::unique_ptr<MockDirectiveHandlerResult> m_mockDirectiveHandlerResult;

    std::mutex m_mutex;
    std::condition_variable m_condition;
    bool m_directiveCompleted = false;
    bool m_eventSent = false;
    std::string m_eventJson;

    std::shared_ptr<AVSDirective> createDirective() {
        auto avsMessageHeader =
            std::make_shared<AVSMessageHeader>("System", "ReportState", TEST_MESSAGE_ID, TEST_REQUEST_ID);
        auto directive = AVSDirective::create("", avsMessageHeader, "", m_attachmentManager, TEST_CONTEXT_ID);
        return std::move(directive);
    }
};

TEST_F(ReportStateHandlerTest, testReportState) {
    void* leak = new int[23];
    leak = nullptr;
    ASSERT_EQ(nullptr, leak);

    MockStateReportGenerator mockGenerator([] { return R"({"unitTest":"ON","complaints":"OFF"})"; });
    generators.push_back(mockGenerator);
    m_unit = ReportStateHandler::create(
        m_customerDataManager,
        m_mockExceptionEncounteredSender,
        m_mockAVSConnectionManager,
        m_mockMessageSender,
        m_stubMiscStorage,
        generators);

    EXPECT_CALL(*m_mockDirectiveHandlerResult, setCompleted()).WillOnce(::testing::InvokeWithoutArgs([this] {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_directiveCompleted = true;
        m_condition.notify_all();
    }));

    EXPECT_CALL(*m_mockMessageSender, sendMessage(::testing::_))
        .WillOnce(::testing::WithArg<0>(::testing::Invoke([this](std::shared_ptr<MessageRequest> request) {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_eventSent = true;
            m_eventJson = request->getJsonContent();
            request->sendCompleted(MessageRequestObserverInterface::Status::SUCCESS);
            m_condition.notify_all();
        })));

    m_unit->CapabilityAgent::preHandleDirective(createDirective(), std::move(m_mockDirectiveHandlerResult));
    m_unit->CapabilityAgent::handleDirective(TEST_MESSAGE_ID);

    waitUntilEventSent();
    ASSERT_TRUE(m_eventJson.find(R"("unitTest":"ON")") != m_eventJson.npos);
}

}  // namespace test
}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
