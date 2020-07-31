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

#ifndef ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYSTATUSOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYSTATUSOBSERVERINTERFACE_H_

#include <string>
#include <unordered_map>

#include <AVSCommon/SDKInterfaces/MessageRequestObserverInterface.h>

namespace alexaClientSDK {
namespace capabilitiesDelegate {

/**
 * Interface to observe Discovery status from @c CapabilitiesDelegate.
 */
class DiscoveryStatusObserverInterface {
public:
    /**
     * Destructor.
     */
    virtual ~DiscoveryStatusObserverInterface() = default;

    /**
     * This method will be called when the discovery event has completed successfully.
     *
     * @note: This method will be called from @c PostConnectCapabilitiesPublisher 's execution thread and should return
     * immediately.
     *
     * @note: This method will be called by the @c PostConnectCapabilitiesPublisher to report the success state when
     * ALL of the the discovery events are sent.
     *
     * @param addOrUpdateReportEndpoints The map of addOrUpdateReport endpoints sent in the discovery events.
     * @param deleteReportEndpoints The map of deleteReport endpoints sent in the discovery events.
     */
    virtual void onDiscoveryCompleted(
        const std::unordered_map<std::string, std::string>& addOrUpdateReportEndpoints,
        const std::unordered_map<std::string, std::string>& deleteReportEndpoints) = 0;

    /**
     * This method will be called when the discovery event has failed.
     * @param status The @c MessageRequestObserverInterface::Status indicating the HTTP error encountered.
     */
    virtual void onDiscoveryFailure(avsCommon::sdkInterfaces::MessageRequestObserverInterface::Status status) = 0;
};

}  // namespace capabilitiesDelegate
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITIESDELEGATE_INCLUDE_CAPABILITIESDELEGATE_DISCOVERYSTATUSOBSERVERINTERFACE_H_
