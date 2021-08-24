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
#ifndef ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_AUTHREFRESHEDOBSERVER_H_
#define ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_AUTHREFRESHEDOBSERVER_H_

#include "AVSCommon/SDKInterfaces/AuthObserverInterface.h"
#include "AVSGatewayManager/PostConnectVerifyGatewaySender.h"

#include <memory>
#include <mutex>
#include <functional>

namespace alexaClientSDK {
namespace avsGatewayManager {

/**
 * Observe Authorization status to call the callback function once the status is refreshed.
 */
class AuthRefreshedObserver
        : public alexaClientSDK::avsCommon::sdkInterfaces::AuthObserverInterface
        , public std::enable_shared_from_this<AuthRefreshedObserver> {
public:
    /**
     * Creates a new instance of @c AuthRefreshedObserver.
     *
     * @param callback callback function  to be called once authorization is refreshed
     * @return a new instance of the @c AuthRefreshedObserver.
     */
    static std::shared_ptr<AuthRefreshedObserver> create(std::function<void()> callback);

    /**
     * Method called with the new Authorization state
     *
     * @param newState new state
     * @param error Not necessarily an error, but a description of the result of the operation which can also be
     * success.
     */
    void onAuthStateChange(State newState, Error error) override;

private:
    /**
     * Creates an instance of @c AuthRefreshedObserver
     * @param m_afterAuthRefreshedCallback callback function to be called once the Authorization status is refreshed
     */
    explicit AuthRefreshedObserver(std::function<void()> afterAuthRefreshedCallback);
    /// Mutex for accessing @c m_state and @c m_messageQueue
    std::mutex m_mutex;

    /// Last known state
    State m_state;

    // Callback function to be called once the Authorization status is refreshed
    std::function<void()> m_afterAuthRefreshedCallback;
};

}  // namespace avsGatewayManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_AUTHREFRESHEDOBSERVER_H_
