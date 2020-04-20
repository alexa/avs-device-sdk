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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTREQUESTOBSERVER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTREQUESTOBSERVER_H_

#include <future>
#include <memory>

#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

namespace alexaClientSDK {
namespace settings {

/**
 * An implementation of @c MessageRequestObserverInterface for each setting event request to AVS.
 */
class SettingEventRequestObserver
        : public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public std::enable_shared_from_this<SettingEventRequestObserver> {
public:
    /**
     * Destructor.
     */
    ~SettingEventRequestObserver() = default;

    /// @name MessageRequestObserverInterface Functions
    /// @{
    void onSendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    void onExceptionReceived(const std::string& exceptionMessage) override;
    /// @}

    /**
     * Retrieves the future that will be set with a @c MessageRequestObserverInterface::Status after a response is
     * received or an error is encountered while sending event for a setting to AVS.
     *
     * @return The future for the status of the event message request.
     */
    std::shared_future<avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> getResponseFuture();

private:
    /// The promise that will be set when a response is received or an is encountered while sending the setting event to
    /// AVS.
    std::promise<avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status> m_promise;
};

}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTREQUESTOBSERVER_H_
