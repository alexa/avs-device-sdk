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

#ifndef ALEXA_CLIENT_SDK_ACSDKMULTIROOMMUSIC_INCLUDE_ACSDKMULTIROOMMUSIC_MRMHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_ACSDKMULTIROOMMUSIC_INCLUDE_ACSDKMULTIROOMMUSIC_MRMHANDLERINTERFACE_H_

#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace mrm {

/**
 * An interface which should be extended by a class which wishes to implement
 * lower level MRM functionality, such as
 * device / platform, local network, time synchronization, and audio playback.
 * The api provided here is minimal and
 * sufficient with respect to integration with other AVS Client SDK components.
 */
class MRMHandlerInterface
        : public avsCommon::sdkInterfaces::SpeakerManagerObserverInterface
        , public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor.
     */
    explicit MRMHandlerInterface(const std::string& shutdownName);

    /**
     * Destructor.
     */
    virtual ~MRMHandlerInterface() override = default;

    /**
     * Returns the string representation of the version of this MRM implementation.
     *
     * @return The string representation of the version of this MRM implementation.
     */
    virtual std::string getVersionString() const = 0;

    /**
     * Function to handle an MRM Directive.
     *
     * @param nameSpace The namespace of the @c AVSDirective to be handled.
     * @param name The name of the @c AVSDirective to be handled.
     * @param messageId The messageId of the @c AVSDirective to be handled.
     * @param payload The payload of the @c AVSDirective to be handled.
     * @return Whether the Directive was handled successfully.
     */
    virtual bool handleDirective(
        const std::string& nameSpace,
        const std::string& name,
        const std::string& messageId,
        const std::string& payload) = 0;

    /**
     * Function to be called when a System.UserInactivityReportSent Event has been sent to AVS.
     */
    virtual void onUserInactivityReportSent() = 0;

    /**
     * Function to be called when a comms CallState has been changed.
     */
    virtual void onCallStateChange(bool active) = 0;

    /**
     * Function to be called when the DialogUXState has been changed.
     */
    virtual void onDialogUXStateChanged(
        avsCommon::sdkInterfaces::DialogUXStateObserverInterface::DialogUXState newState) = 0;

    /**
     * Function to set the RenderPlayerInfoCardsProviderInterface.
     *
     * @param observer The RenderPlayerInfoCardsObserverInterface to be set.
     */
    virtual void setObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) = 0;
};

inline MRMHandlerInterface::MRMHandlerInterface(const std::string& shutdownName) :
        avsCommon::utils::RequiresShutdown{shutdownName} {
}

}  // namespace mrm
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_ACSDKMULTIROOMMUSIC_INCLUDE_ACSDKMULTIROOMMUSIC_MRMHANDLERINTERFACE_H_
