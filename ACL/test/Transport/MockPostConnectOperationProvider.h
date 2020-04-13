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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKPOSTCONNECTOPERATIONPROVIDER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKPOSTCONNECTOPERATIONPROVIDER_H_

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

/**
 * A mock class of @c PostConnectOperationProviderInterface.
 */
class MockPostConnectOperationProvider : public avsCommon::sdkInterfaces::PostConnectOperationProviderInterface {
public:
    MOCK_METHOD0(
        createPostConnectOperation,
        std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface>());
};

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK

#endif /* ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKPOSTCONNECTOPERATIONPROVIDER_H_ */
