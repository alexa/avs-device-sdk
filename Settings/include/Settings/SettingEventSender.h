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

#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTSENDER_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTSENDER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/AVS/EventBuilder.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>
#include <AVSCommon/Utils/RetryTimer.h>
#include <AVSCommon/Utils/WaitEvent.h>
#include <Settings/SettingEventMetadata.h>
#include <Settings/SettingEventSenderInterface.h>

namespace alexaClientSDK {
namespace settings {

/**
 * An implementation of the @c SettingEventSenderInterface.
 */
class SettingEventSender : public SettingEventSenderInterface {
public:
    /**
     * Creates an instance of the @SettingEventSender.
     *
     * @param metadata Contains the information needed to construct AVS events.
     * @param messageSender The delivery service for the AVS events.
     * @return The shared pointer to the created @c SettingEventSender instance.
     */
    static std::unique_ptr<SettingEventSender> create(
        const SettingEventMetadata& metadata,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        const std::vector<int>& retryTable = getDefaultRetryTable());

    /**
     * Destructor.
     * Cancels any pending requests.
     */
    ~SettingEventSender();

    /// @name SettingEventSenderInterface Functions
    /// @{
    std::shared_future<bool> sendChangedEvent(const std::string& value) override;
    std::shared_future<bool> sendReportEvent(const std::string& value) override;
    std::shared_future<bool> sendStateReportEvent(const std::string& payload) override;
    void cancel() override;
    /// @}

private:
    /*
     * Constructor.
     *
     * @param messageSender The delivery service for the AVS events.
     * @param retryTable A list of back-off times in milliseconds used in resending events.
     */
    SettingEventSender(
        const SettingEventMetadata& metadata,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        const std::vector<int>& retryTable);

    /*
     * Helper function to send the changed and report events.
     *
     * @param eventJson The event json to be sent.
     * @return A future expressing if the event has been sent to AVS.
     */
    std::shared_future<bool> sendEvent(const std::string& eventJson);

    /**
     * Creates the event content.
     *
     * @param eventName The name of the event.
     * @param value The value of the setting. It should be a valid JSON string value.
     * @return The event json string.
     */
    std::string buildEventJson(const std::string& eventName, const std::string& value) const;

    /**
     * Retrieves the default back-off times for resending events.
     *
     * @return The default back-off times for resending events.
     */
    static const std::vector<int>& getDefaultRetryTable();

    /// Contains information needed to construct AVS events.
    const SettingEventMetadata m_metadata;

    /// The delivery service for the AVS events.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// A mutex to ensure only one event is sent at a time.
    std::mutex m_sendMutex;

    /// Object used to wait for event transmission cancellation.
    avsCommon::utils::WaitEvent m_waitCancelEvent;

    /// Retry Timer object.
    avsCommon::utils::RetryTimer m_retryTimer;

    /// The number of retries that will be done on an event in case of send failure.
    const std::size_t m_maxRetries;
};

}  // namespace settings
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_SETTINGEVENTSENDER_H_
