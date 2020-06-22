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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDER_H_

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#include <AVSCommon/AVS/WaitableMessageRequest.h>
#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/WaitEvent.h>

#include "DiscoveryEventSenderInterface.h"
#include "DiscoveryStatusObserverInterface.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {

/**
 * This class publishes @c Discovery.AddOrUpdateReport and @c Discovery.DeleteReport events.
 */
class DiscoveryEventSender
        : public DiscoveryEventSenderInterface
        , public std::enable_shared_from_this<DiscoveryEventSender> {
public:
    /**
     * Creates a new instance of the @c DiscoveryEventSender.
     *
     * @param addOrUpdateReportEndpoints The map of endpoints for which the @c Discovery.AddOrUpdateReport event will be
     * sent.
     * @param deleteReportEndpoints The map of endpoints for which the @c Discovery.DeleteReport event will be sent.
     * @param authDelegate The auth delegate instance to request the auth token from to be sent in the @c Discovery
     * events.
     * @return a new instance of the @c DiscoveryEventSender.
     */
    static std::shared_ptr<DiscoveryEventSender> create(
        const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
        const std::unordered_map<std::string, std::string>& deleteReportEndpoints,
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate);

    /**
     * Destructor.
     */
    ~DiscoveryEventSender();

    /// @name DiscoveryEventSenderInterface method overrides.
    /// @{
    bool sendDiscoveryEvents(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) override;
    void stop() override;
    void addDiscoveryStatusObserver(const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) override;
    void removeDiscoveryStatusObserver(const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) override;
    /// @}

    /// @name AlexaEventProcessedObserverInterface method overrides.
    /// @{
    void onAlexaEventProcessedReceived(const std::string& eventCorrelationToken) override;
    /// @}

    /// @name AuthObserverInterface method overrides.
    /// @{
    void onAuthStateChange(AuthObserverInterface::State newState, AuthObserverInterface::Error newError) override;
    /// @}
private:
    /**
     * Constructor.
     *
     * @param addOrUpdateReportEndpoints The map of endpoints for which the @c Discovery.AddOrUpdateReport event will be
     * sent.
     * @param deleteReportEndpoints The map of endpoints for which the @c Discovery.DeleteReport event will be sent.
     * @param authDelegate The auth delegate instance to request the auth token from to be sent in the @c Discovery
     * events.
     */
    DiscoveryEventSender(
        const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
        const std::unordered_map<std::string, std::string>& deleteReportEndpoints,
        const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate);

    /**
     * Sends the discovery event while taking into account retries.
     *
     * @param messageSender The @c MessageSenderInterface to send messages.
     * @param endpointConfigurations The endpointConfigurations to be sent.
     * @param isAddOrUpdateReportEvent boolean indicating which Discovery event to send, true indicates @c
     * AddOrUpdateReport, false indicates @c DeleteReport event.
     * @return true if the event was sent successfully. False if there is a fatal error response or if there
     * post connect operation is aborted.
     */
    bool sendDiscoveryEventWithRetries(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const std::vector<std::string>& endpointConfigurations,
        bool isAddOrUpdateReportEvent = true);

    /**
     * Sends the Discovery event.
     *
     * @param messageSender The @c MessageSenderInterface to send messages.
     * @param eventString The eventJson string to be sent.
     * @param waitForEventProcessed Flag indicating if the event should wait for the EventProcessed directive.
     * @return true if the event was sent successfully. False if there is a fatal error response or if there
     * post connect operation is aborted.
     */
    avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status sendDiscoveryEvent(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        const std::string& eventString,
        bool waitForEventProcessed = true);

    /**
     * Sends multiple Discovery.AddOrUpdateReport events based on the number of endpoints requested for.
     *
     * @param messageSender The @c MessageSenderInterface to send messages.
     * @return true if the event was sent successfully. False if there is a fatal error response or if there
     * post connect operation is aborted.
     */
    bool sendAddOrUpdateReportEvents(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender);

    /**
     * Sends multiple Discovery.DeleteReport events based on the number of endpoints requested for.
     *
     * @param messageSender The @c MessageSenderInterface to send messages.
     * @return true if the event was sent successfully. False if there is a fatal error response or if there
     * post connect operation is aborted.
     */
    bool sendDeleteReportEvents(const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender);

    /**
     * Sends multiple Discovery events based on the number of endpoints requested for.
     *
     * @param endpointConfigurations The list of endpointConfigurations to be sent in the discovery events.
     * @param messageSender The @c MessageSenderInterface to send messages.
     * @param isAddOrUpdateReportEvent boolean indicating if the current event is AddOrUpdateReport.
     * @return true if the event was sent successfully. False if there is a fatal error response or if there
     * post connect operation is aborted.
     */
    bool sendDiscoveryEvents(
        const std::vector<std::string>& endpointConfigurations,
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
        bool isAddOrUpdateReportEvent = true);

    /**
     * This method checks if the capabilities delegate reports the discovery status to the capabilities delegate
     * while checking if its shutting down.
     * @param status The @c MessageRequestObserverInterface::Status to be reported.
     */
    void reportDiscoveryStatus(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status);

    /**
     * Gets an auth token from the authDelegate instance.
     *
     * @note: This will wait till the auth token is available.
     *
     * @return The auth token.
     */
    std::string getAuthToken();

    /**
     * A thread safe method of checking if stop has been triggered.
     *
     * @return true if stopping has been triggered, else false.
     */
    bool isStopping();

    /// The endpoints for which @c Discovery.AddOrUpdateReport event will be sent.
    std::unordered_map<std::string, std::string> m_addOrUpdateReportEndpoints;

    /// The endpoints for which @c Discovery.DeleteReport event will be sent.
    std::unordered_map<std::string, std::string> m_deleteReportEndpoints;

    /// Auth delegate used to get the access token
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// The authDelegate's auth status.
    AuthObserverInterface::State m_currentAuthState;

    /// Mutex to serialize access to the authStatus.
    std::mutex m_authStatusMutex;

    /// Used to check if the auth delegate is ready.
    std::condition_variable m_authStatusReady;

    /// The current event correlation token.
    /// @note Access to this member is serialized by sendDiscoveryEvents() method which is only called once.
    std::string m_currentEventCorrelationToken;

    /// Flag that will be set when the @c DiscoveryEventSender is being shutdown.
    bool m_isStopping;

    /// The @c WaitEvent to wait on EventProcessed directive.
    avsCommon::utils::WaitEvent m_eventProcessedWaitEvent;

    /// The @c WaitEvent to cancel retry waits.
    avsCommon::utils::WaitEvent m_retryWait;

    /// Mutex to synchronize access to @c MessageRequest.
    std::mutex m_mutex;

    /// The @c MessageRequest used to send messages.
    std::shared_ptr<avsCommon::avs::WaitableMessageRequest> m_messageRequest;

    /// The mutex to serialize the observer added.
    std::mutex m_observerMutex;

    /// The observer to listen on notifications generated when Discovery events are sent.
    std::shared_ptr<DiscoveryStatusObserverInterface> m_observer;

    /// Mutex to synchronize access to @c m_isSendDiscoveryEventsInvoked.
    std::mutex m_isSendDiscoveryEventsInvokedMutex;

    /// Flag to guard against repeated calls to sendDiscoveryEvents method.
    bool m_isSendDiscoveryEventsInvoked;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDER_H_
