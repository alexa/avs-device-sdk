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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MRM_INCLUDE_MRM_MRMHANDLERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MRM_INCLUDE_MRM_MRMHANDLERINTERFACE_H_

#include <memory>
#include <mutex>
#include <string>

#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/SDKInterfaces/RenderPlayerInfoCardsProviderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

#ifdef ENABLE_AUXCONTROLLER
#include <AuxController/Interfaces/AuxPlaybackDelegateObserverInterface.h>
#include <AuxController/Interfaces/AuxPlaybackStateObserverInterface.h>
#endif  // ENABLE_AUXCONTROLLER

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
        : public avsCommon::utils::RequiresShutdown
#ifdef ENABLE_AUXCONTROLLER
        , public interfaces::auxController::AuxPlaybackStateObserverInterface
#endif  // ENABLE_AUXCONTROLLER
{
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
     * Function to handle if a speaker setting has changed.  MRM only needs to know the type of the speaker.
     *
     * @param type The type of the speaker which has changed.
     */
    virtual void onSpeakerSettingsChanged(const avsCommon::sdkInterfaces::ChannelVolumeInterface::Type& type) = 0;

    /**
     * Function to be called when a System.UserInactivityReportSent Event has been sent to AVS.
     */
    virtual void onUserInactivityReportSent() = 0;

    /**
     * Function to be called when a comms CallState has been changed.
     */
    virtual void onCallStateChange(bool active) = 0;

    /**
     * Function to set the RenderPlayerInfoCardsProviderInterface.
     *
     * @param observer The RenderPlayerInfoCardsObserverInterface to be set.
     */
    virtual void setObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsObserverInterface> observer) = 0;

#ifdef ENABLE_AUXCONTROLLER
    /**
     * Function to handle aux playback state changes.
     *
     * @param playbackState The @c AuxPlaybackState to be handled.
     */
    virtual void onAuxPlaybackStateChanged(
        alexaClientSDK::interfaces::auxController::AuxPlaybackState playbackState) = 0;

    /**
     * This function sets an observer to @c MRMHandlerInterface so that it will get notified
     * for aux playback delegate state changes.
     *
     * @param observer The @c AuxPlaybackDelegateObserverInterface object.
     */
    virtual void setObserver(
        std::shared_ptr<interfaces::auxController::AuxPlaybackDelegateObserverInterface> observer) = 0;

    /**
     * This function clears an observer from @c MRMHandlerInterface so that it will no longer
     * be notified of aux playback delegate state changes.
     *
     * @param observer The @c AuxPlaybackDelegateObserverInterface object.
     */
    virtual void clearObserver(
        std::shared_ptr<interfaces::auxController::AuxPlaybackDelegateObserverInterface> observer) = 0;
#endif  // ENABLE_AUXCONTROLLER
};

inline MRMHandlerInterface::MRMHandlerInterface(const std::string& shutdownName) :
        avsCommon::utils::RequiresShutdown{shutdownName} {
}

}  // namespace mrm
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MRM_INCLUDE_MRM_MRMHANDLERINTERFACE_H_
