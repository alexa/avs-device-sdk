/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_CERTIFIEDSENDER_TEST_MOCKCERTIFIEDSENDER_H_
#define ALEXA_CLIENT_SDK_CERTIFIEDSENDER_TEST_MOCKCERTIFIEDSENDER_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

#include "AVSCommon/SDKInterfaces/MockAVSConnectionManager.h"
#include "AVSCommon/SDKInterfaces/MockMessageSender.h"

#include <CertifiedSender/CertifiedSender.h>
#include <RegistrationManager/CustomerDataHandler.h>

#include <MockMessageStorage.h>

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

/**
 * Mock class of @c CertifiedSender.
 */
class MockCertifiedSender {
public:
    /**
     * Constructor.
     * Creates the test instance of the @c CertifiedSender.
     */
    MockCertifiedSender();

    /**
     * Retrieves the test instance of @c CertifiedSender.
     */
    std::shared_ptr<certifiedSender::CertifiedSender> get();

    /**
     * Retrieves the @c MessageSender used by the @c CertifiedSender.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> getMockMessageSender();

private:
    /// The test instance of the certified sender.
    std::shared_ptr<certifiedSender::CertifiedSender> m_certifiedSender;

    /// Mock of @c MessageSenderInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> m_mockMessageSender;

    /// Mock of @c MessageStorageInterface.
    std::shared_ptr<certifiedSender::test::MockMessageStorage> m_mockMessageStorage;

    /// Mock of @c AVSConnectionManagerInterface.
    std::shared_ptr<avsCommon::sdkInterfaces::test::MockAVSConnectionManager> m_mockAVSConnectionManager;

    /// Contains an instance of @c CustomerDataManager used by the certified sender.
    std::shared_ptr<registrationManager::CustomerDataManager> m_customerDataManager;
};

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CERTIFIEDSENDER_TEST_MOCKCERTIFIEDSENDER_H_
