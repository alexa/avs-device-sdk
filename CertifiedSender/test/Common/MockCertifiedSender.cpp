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
#include "MockCertifiedSender.h"

namespace alexaClientSDK {
namespace certifiedSender {
namespace test {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::test;
using namespace avsCommon::utils;
using namespace certifiedSender;
using namespace certifiedSender::test;
using namespace registrationManager;

using namespace ::testing;

MockCertifiedSender::MockCertifiedSender() {
    m_mockMessageSender = std::make_shared<NiceMock<MockMessageSender>>();
    m_mockAVSConnectionManager = std::make_shared<NiceMock<MockAVSConnectionManager>>();
    m_customerDataManager = std::make_shared<CustomerDataManager>();
    m_mockMessageStorage = std::make_shared<NiceMock<MockMessageStorage>>();

    ON_CALL(*(m_mockMessageStorage), createDatabase()).WillByDefault(Return(true));
    ON_CALL(*(m_mockMessageStorage), open()).WillByDefault(Return(true));
    ON_CALL(*(m_mockMessageStorage), load(_)).WillByDefault(Return(true));
    ON_CALL(*(m_mockMessageStorage), erase(_)).WillByDefault(Return(true));
    ON_CALL(*(m_mockMessageStorage), store(_, _)).WillByDefault(Return(true));

    m_certifiedSender = CertifiedSender::create(
        m_mockMessageSender, m_mockAVSConnectionManager, m_mockMessageStorage, m_customerDataManager);
}

std::shared_ptr<certifiedSender::CertifiedSender> MockCertifiedSender::get() {
    return m_certifiedSender;
}

std::shared_ptr<avsCommon::sdkInterfaces::test::MockMessageSender> MockCertifiedSender::getMockMessageSender() {
    return m_mockMessageSender;
}

}  // namespace test
}  // namespace certifiedSender
}  // namespace alexaClientSDK
