/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKTRANSPORTOBSERVER_H_
#define ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKTRANSPORTOBSERVER_H_

#include <ACL/Transport/TransportObserverInterface.h>

#include <gmock/gmock.h>

#include <memory>

namespace alexaClientSDK {
namespace acl {
namespace transport {
namespace test {

class MockTransportObserver : public TransportObserverInterface {
public:
    MOCK_METHOD1(onConnected, void(std::shared_ptr<TransportInterface> transport));
    MOCK_METHOD2(
        onDisconnected,
        void(
            std::shared_ptr<TransportInterface> transport,
            avsCommon::sdkInterfaces::ConnectionStatusObserverInterface::ChangedReason reason));
    MOCK_METHOD1(onServerSideDisconnect, void(std::shared_ptr<TransportInterface> transport));
};

}  // namespace test
}  // namespace transport
}  // namespace acl
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACL_TEST_TRANSPORT_MOCKTRANSPORTOBSERVER_H_
