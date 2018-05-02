/*
 * Copyright 2016-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AUTHOBSERVER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AUTHOBSERVER_H_

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>

namespace alexaClientSDK {
namespace integration {

class AuthObserver : public avsCommon::sdkInterfaces::AuthObserverInterface {
public:
    AuthObserver();
    void onAuthStateChange(
        const avsCommon::sdkInterfaces::AuthObserverInterface::State,
        const avsCommon::sdkInterfaces::AuthObserverInterface::Error =
            avsCommon::sdkInterfaces::AuthObserverInterface::Error::SUCCESS) override;
    AuthObserverInterface::State getAuthState() const;
    bool waitFor(
        const avsCommon::sdkInterfaces::AuthObserverInterface::State,
        const std::chrono::seconds = std::chrono::seconds(20));

private:
    avsCommon::sdkInterfaces::AuthObserverInterface::State m_authState;
    avsCommon::sdkInterfaces::AuthObserverInterface::Error m_authError;
    std::mutex m_mutex;
    std::condition_variable m_wakeTrigger;
};

}  // namespace integration
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_AUTHOBSERVER_H_
