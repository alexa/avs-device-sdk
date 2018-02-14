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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SOFTWAREINFOSENDREQUEST_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SOFTWAREINFOSENDREQUEST_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Timing/Timer.h>

#include "System/SoftwareInfoSender.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * Object to send a @c System.SoftwareInfo event to @c AVS.
 *
 * @note If the event fails with SERVER_INTERNAL_ERROR_V2, sending the event is retried until
 * it succeeds or the request is cancelled.
 */
class SoftwareInfoSendRequest
        : public avsCommon::sdkInterfaces::MessageRequestObserverInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<SoftwareInfoSendRequest> {
public:
    /**
     * Constructor.
     *
     * @param firmwareVersion The firmware version to send to AVS.
     * @param messageSender The object to use to send messages to AVS
     * @param observer An object to receive notification that the send succeeded.
     * @return The newly created instance of InfoSendRequest, or nullptr if the operation failed.
     */
    static std::shared_ptr<SoftwareInfoSendRequest> create(
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> observer);

    /**
     * Send the SoftwareInfo event to AVS.
     */
    void send();

    /// @name MessageRequestObserverInterface Functions
    /// @{
    void onSendCompleted(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) override;
    void onExceptionReceived(const std::string& message) override;
    /// @}

    /// @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Construct a new @c SoftwareInfoSendRequest instance.
     *
     * @param firmwareVersion The firmware version to send to AVS.
     * @param messageSender The object to use to send messages to AVS
     * @param observer An object to receive notification that the send succeeded.
     * @return The newly created instance of InfoSendRequest, or nullptr if the operation failed.
     */
    SoftwareInfoSendRequest(
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> observer);

    /**
     * Render the JSON for a System.SoftwareInfo event.
     *
     * @param[out] The string in which to return the rendered event JSON.
     * @param firmwareVersion The firmware version to include in the event.
     * @return Whether or not the operation was successful.
     */
    static bool buildJsonForSoftwareInfo(
        std::string* jsonContent,
        avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion firmwareVersion);

    /// The firmware version to send with the request.
    const avsCommon::sdkInterfaces::softwareInfo::FirmwareVersion m_firmwareVersion;

    /// Object to send messages to AVS.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /**
     * A promise to fulfill with the send status when the send operation completes.
     * @note Access to this member is serialized with @c m_mutex.
     */
    std::shared_ptr<avsCommon::sdkInterfaces::SoftwareInfoSenderObserverInterface> m_observer;

    /// Mutex to serialize access to data members.
    std::mutex m_mutex;

    /**
     * The number of times we have retried sending the event.
     * @note Access to this member is serialized with @c m_mutex.
     */
    int m_retryCounter;

    /**
     * Timers used to schedule retries when a send attempt results in the SERVER_INTERNAL_ERROR_V2 status.
     * @note A @c Timer's task may not be used to @c re-start() its triggering @ Timer.  To get around
     * this limitation, two timers are used here such that a failed retry triggered by one of the @c Timers
     * will use the other @c Timer to specify the delay before the next retry.
     * TODO: ACSDK-841 Enhance the Timer class to allow re-start() of a timer from the Timer's task
     * @note Access to this member is serialized with @c m_mutex.
     */
    avsCommon::utils::timing::Timer m_retryTimers[2];
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SOFTWAREINFOSENDREQUEST_H_
