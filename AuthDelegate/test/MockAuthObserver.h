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

#ifndef ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_MOCKAUTHOBSERVER_H_
#define ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_MOCKAUTHOBSERVER_H_

#include <gmock/gmock.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace authDelegate {
namespace test {

/// Mock AuthObserverInterface class
class MockAuthObserver : public avsCommon::sdkInterfaces::AuthObserverInterface {
public:
    MOCK_METHOD2(onAuthStateChange, void(State newState, Error error));
};

}  // namespace test
}  // namespace authDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AUTHDELEGATE_TEST_MOCKAUTHOBSERVER_H_
