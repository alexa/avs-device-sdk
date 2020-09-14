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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDERINTERFACE_H_

#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>

#include "DiscoveryStatusObserverInterface.h"

namespace alexaClientSDK {
namespace capabilitiesDelegate {

/**
 * Interface to send Discovery events.
 */
class DiscoveryEventSenderInterface
        : public avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface
        , public avsCommon::sdkInterfaces::AuthObserverInterface {
public:
    /**
     * Sends Discovery.AddOrUpdateReport and Discovery.deleteReport events for the addOrUpdateEndpoints and
     * deleteReportEndpoints with which this object was created.
     *
     * @param messageSender The @c MessageSenderInterface to send messages.
     * @return Whether sending the Discovery events was successful.
     */
    virtual bool sendDiscoveryEvents(
        const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender) = 0;

    /**
     * Method to stop execution and unblock any condition variables that are waiting.
     */
    virtual void stop() = 0;

    /**
     * Adds a @c DiscoveryStatusObserverInterface observer.
     */
    virtual void addDiscoveryStatusObserver(const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) = 0;

    /**
     * Removes a @c DiscoveryStatusObserverInterface observer.
     */
    virtual void removeDiscoveryStatusObserver(const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) = 0;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDERINTERFACE_H_
