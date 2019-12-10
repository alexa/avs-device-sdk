/*
 * Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/SDKInterfaces/AlexaEventProcessedObserverInterface.h>
#include <CapabilitiesDelegate/DiscoveryStatusObserverInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

/**
 * Interface to observe discovery status from the @c PostConnectCapabilitiesPublisher.
 */
class DiscoveryEventSenderInterface : public avsCommon::sdkInterfaces::AlexaEventProcessedObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~DiscoveryEventSenderInterface() = default;

    /**
     * This method adds an @c DiscoveryStatusObserver. The observer gets notified of SUCCESS or FAILURE responses during
     * discovery.
     *
     * @note: If there is a failure to send a Discovery event, the onDiscoveryFailed() method will be called
     * immediately. If the discovery event is sent successfully, the onDiscoverySuccess() method will be called only
     * after all the events are successfully sent.
     * @param observer The pointer to the observer that will be added.
     */
    virtual void addDiscoveryStatusObserver(const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) = 0;

    /**
     * This method removes an @c DiscoveryStatusObserver.
     * @param observer The pointer to the observer that needs to be removed. If the observer is not found, this
     * method returns silently without changing the existing observer.
     */
    virtual void removeDiscoveryStatusObserver(const std::shared_ptr<DiscoveryStatusObserverInterface>& observer) = 0;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYEVENTSENDERINTERFACE_H_
