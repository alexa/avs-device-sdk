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

#ifndef ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUISERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUISERVERINTERFACE_H_

#include <AIP/Initiator.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/ContentType.h>
#include <AVSCommon/SDKInterfaces/AuthObserverInterface.h>
#ifdef ENABLE_COMMS
#include <AVSCommon/SDKInterfaces/CallManagerInterface.h>
#endif
#include <AVSCommon/SDKInterfaces/ChannelObserverInterface.h>
#include <AVSCommon/SDKInterfaces/GUIActivityEvent.h>
#include <acsdk/PresentationOrchestratorInterfaces/PresentationOrchestratorTypes.h>
#include <acsdk/VisualCharacteristicsInterfaces/VisualCharacteristicsInterface.h>

#include <APLClient/AplRenderingEvent.h>
#include "NavigationEvent.h"

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {
namespace gui {

/// An interface providing APIs to be used by GUIClient.
class GUIServerInterface {
public:
    /**
     * Handle Recognize Speech Request Event
     * @param initiator the @c Initiator type for speech recognition
     * @param start true if this is the start of speech recognition
     */
    virtual void handleRecognizeSpeechRequest(capabilityAgents::aip::Initiator initiator, bool start) = 0;

    /**
     * Toggles the microphone state if the Sample App was built with wake word. When the microphone is turned off, the
     * app enters a privacy mode in which it stops recording audio data from the microphone, thus disabling Alexa waking
     * up due to wake word. Note however that hold-to-talk and tap-to-talk modes will still work by recording
     * microphone data temporarily until a user initiated interaction is complete. If this app was built without wake
     * word then this will do nothing as the microphone is already off.
     */
    virtual void handleMicrophoneToggle() = 0;

    /**
     * Handle playback 'PLAY' event.
     */
    virtual void handlePlaybackPlay() = 0;

    /**
     * Handle playback 'PAUSE' event.
     */
    virtual void handlePlaybackPause() = 0;

    /**
     * Handle playback 'NEXT' event.
     */
    virtual void handlePlaybackNext() = 0;

    /**
     * Handle playback 'PREVIOUS' event.
     */
    virtual void handlePlaybackPrevious() = 0;

    /**
     * Handle playback 'SEEK_TO' event.
     */
    virtual void handlePlaybackSeekTo(int offset) = 0;

    /**
     * Handle playback 'SKIP_FORWARD' event.
     */
    virtual void handlePlaybackSkipForward() = 0;

    /**
     * Handle playback 'SKIP_BACKWARD' event.
     */
    virtual void handlePlaybackSkipBackward() = 0;

    /**
     * Handle playback 'TOGGLE' event.
     * @param name name of the TOGGLE.
     * @param checked checked state of the TOGGLE.
     */
    virtual void handlePlaybackToggle(const std::string& name, bool checked) = 0;

    /**
     * Handle focus acquire requests.
     *
     * @param avsInterface The AVS Interface requesting focus.
     * @param channelName The channel to be requested.
     * @param contentType The type of content acquiring focus.
     * @param channelObserver the channelObserver to be notified.
     */
    virtual bool handleFocusAcquireRequest(
        std::string avsInterface,
        std::string channelName,
        avsCommon::avs::ContentType contentType,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) = 0;

    /**
     * Handle focus release requests.
     *
     * @param avsInterface The avsInterface to be released.
     * @param channelName channelName to be released.
     * @param channelObserver the channelObserver to be notified.
     */
    virtual bool handleFocusReleaseRequest(
        std::string avsInterface,
        std::string channelName,
        std::shared_ptr<avsCommon::sdkInterfaces::ChannelObserverInterface> channelObserver) = 0;

    /**
     * Handle activityEvent message.
     *
     * @param event the activity event.
     * @param source The source of the activity event. Default empty string.
     */
    virtual void handleGUIActivityEvent(
        avsCommon::sdkInterfaces::GUIActivityEvent event,
        const std::string& source = "") = 0;

    /**
     * Handle event the navigation event.
     *
     * @param event the navigation event.
     */
    virtual void handleNavigationEvent(NavigationEvent event) = 0;

    /**
     * Gets the window template configuration
     * @return vector containing the window templates
     */
    virtual std::vector<visualCharacteristicsInterfaces::WindowTemplate> getWindowTemplates() = 0;

    /**
     * Retrieve the interaction mode configuration
     * @return vector containing the interaction modes
     */
    virtual std::vector<visualCharacteristicsInterfaces::InteractionMode> getInteractionModes() = 0;

    /**
     * Get the display characteristics
     * @return the display characteristics object
     */
    virtual visualCharacteristicsInterfaces::DisplayCharacteristics getDisplayCharacteristics() = 0;

    /**
     * Sets the window instances to be reported in WindowState.
     * Replaces any windows in the existing WindowState set.
     * @param instances The vector of window instances to aad/report in WindowState
     * @param defaultWindowInstanceId The default window id to report in WindowState
     * @param audioPlaybackUIWindowId The id of the window handling audio playback to report in WindowState
     */
    virtual void setWindowInstances(
        const std::vector<presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance>& instances,
        const std::string& defaultWindowInstanceId,
        const std::string& audioPlaybackUIWindowId) = 0;

    /**
     * Adds a window instance to be reported in WindowState
     * @param instance The window instance to add
     * @return true if the instance was successfully added, false otherwise
     */
    virtual bool addWindowInstance(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) = 0;

    /**
     * Remove an existing window instance, at least one window must exist at all times
     * @param windowInstanceId The id of the window to remove
     * @return true if the instance was removed, false otherwise
     */
    virtual bool removeWindowInstance(const std::string& windowInstanceId) = 0;

    /**
     * Updates an already existing window instance
     * @param instance The updated window instance, the window ID must match an already existing window
     */
    virtual void updateWindowInstance(
        const presentationOrchestratorInterfaces::PresentationOrchestratorWindowInstance& instance) = 0;

    /**
     * Sets the default window instance
     * @param windowInstanceId The id of window to set as the default, this window id must already exist
     * @return true if the default has been set, false otherwise
     */
    virtual bool setDefaultWindowInstance(const std::string& windowInstanceId) = 0;

    /**
     * Serialize interaction modes into reportable json format.
     * @param interactionModes  Collection of @c InteractionMode to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    virtual bool serializeInteractionMode(
        const std::vector<visualCharacteristicsInterfaces::InteractionMode>& interactionModes,
        std::string& serializedJson) = 0;

    /**
     * Serialize window template into reportable json format.
     * @param windowTemplates  Collection of @c WindowTemplate to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    virtual bool serializeWindowTemplate(
        const std::vector<visualCharacteristicsInterfaces::WindowTemplate>& windowTemplates,
        std::string& serializedJson) = 0;

    /**
     * Serialize display characteristics into reportable json format.
     * @param display Instance of @c DisplayCharacteristics to be serialized
     * @param serializedJson [ out ] Serialized json payload
     *
     * @return True if serialization successful, false otherwise.
     */
    virtual bool serializeDisplayCharacteristics(
        const visualCharacteristicsInterfaces::DisplayCharacteristics& display,
        std::string& serializedJson) = 0;

    /**
     * Gets Device Time Zone Offset.
     */
    virtual std::chrono::milliseconds getDeviceTimezoneOffset() = 0;

    virtual void onUserEvent() = 0;

    /**
     * Force exit to reset focus state and clear card.
     */
    virtual void forceExit() = 0;

    /**
     * Handle accept call event.
     */
    virtual void acceptCall() = 0;

    /**
     * Handle stop call event.
     */
    virtual void stopCall() = 0;

    /**
     * Handle enable local video event.
     */
    virtual void enableLocalVideo() = 0;

    /**
     * Handle disable local video event.
     */
    virtual void disableLocalVideo() = 0;

#ifdef ENABLE_COMMS
    /**
     * Handle send DTMF tone event.
     */
    virtual void sendDtmf(avsCommon::sdkInterfaces::CallManagerInterface::DTMFTone dtmfTone) = 0;
#endif

    /**
     * Handle an onConnectionOpened event from the messaging server
     */
    virtual void handleOnMessagingServerConnectionOpened() = 0;

    /**
     * Handle document terminated result.
     * Handler should clear the associated APL document, and any active/pending ExecuteCommands directives for the
     * document.
     *
     * @param token the apl document result token.
     * @param failed the indication if document terminated due to failure.
     */
    virtual void handleDocumentTerminated(const std::string& token, bool failed) = 0;

    /**
     * Handle Locale Change
     */
    virtual void handleLocaleChange() = 0;

    /**
     * Initialize the IPC connection and inform the IPC client of the IPC framework version.
     */
    virtual void initClient() = 0;
};

}  // namespace gui
}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_LIBIPCSERVERSAMPLEAPP_INCLUDE_IPCSERVERSAMPLEAPP_GUI_GUISERVERINTERFACE_H_
