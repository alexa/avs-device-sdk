/*
 * DefaultClient.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_DEFAULT_CLIENT_INCLUDE_DEFAULT_CLIENT_H_
#define ALEXA_CLIENT_SDK_DEFAULT_CLIENT_INCLUDE_DEFAULT_CLIENT_H_

#include <ACL/AVSConnectionManager.h>
#include <ADSL/DirectiveSequencer.h>
#include <AFML/FocusManager.h>
#include <AIP/AudioInputProcessor.h>
#include <AIP/AudioProvider.h>
#include <Alerts/AlertsCapabilityAgent.h>
#include <Alerts/Storage/AlertStorageInterface.h>
#include <AudioPlayer/AudioPlayer.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ConnectionStatusObserverInterface.h>
#include <AVSCommon/SDKInterfaces/DialogUXStateObserverInterface.h>
#include <AVSCommon/Utils/MediaPlayer/MediaPlayerInterface.h>
#include <System/StateSynchronizer.h>

namespace alexaClientSDK {
namespace defaultClient {

/**
 * This class serves to instantiate each default component with of the SDK with no specializations to provide an 
 * "out-of-box" component that users may utilize for AVS interaction.
 */
class DefaultClient {
public:
    /// A reserved index value which is considered invalid.
    static const auto INVALID_INDEX = capabilityAgents::aip::AudioInputProcessor::INVALID_INDEX;

    /**
     * Creates and initializes a default AVS SDK client. To connect the client to AVS, users should make a call to
     * connect() after creation.
     * 
     * @param speakMediaPlayer The media player to use to play Alexa speech from.
     * @param audioMediaPlayer The media player to use to play Alexa audio content from.
     * @param alertsMediaPlayer The media player to use to play alerts from.
     * @param authDelegate The component that provides the client with valid LWA authorization.
     * @Param alertStorage The storage interface that will be used to store alerts.
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state 
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @return A @c std::unique_ptr to a DefaultClient if all went well or @c nullptr otherwise.
     *
     * TODO: ACSDK-384 Remove the requirement of clients having to wait for authorization before making the connect()
     * call.
     * TODO: Allow the user to pass in a MediaPlayer factory rather than each media player individually.
     */
    static std::unique_ptr<DefaultClient> create(
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
            std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> 
                    alexaDialogStateObservers,
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> 
                    connectionObservers);

    /**
     * Connects the client to AVS. Note that users should first wait for the authorization state to be set to REFRESHED
     * before calling this function. After this call, users can observe the state of the connection asynchronously by 
     * using a connectionObserver that was passed in to the create() function.
     *
     * @param avsEndpoint An optional parameter to the AVS URL to connect to. This is defaulted to the North American
     * endpoint.
     */
    void connect(const std::string& avsEndpoint = "https://avs-alexa-na.amazon.com");

    /**
     * Disconnects the client from AVS if it is connected. After the call, users can observer the state of the
     * connection asynchronously by using a connectionObserver that was passed in to the create() function.
     */
    void disconnect();

    /**
     * Stops the foreground activity if there is one. This acts as a "stop" button that can be used to stop an 
     * ongoing activity. This call will block until the foreground activity has stopped all user-observable activities.
     */
    void stopForegroundActivity();

    /**
     * Adds an observer to be notified of Alexa dialog related UX state.
     *
     * @param observer The observer to add.
     */
    void addAlexaDialogStateObserver(
            std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);

    /**
     * Removes an observer to be notified of Alexa dialog related UX state.
     *
     * @note This is a synchronous call which can not be made by an observer callback.  Attempting to call
     *     @c removeObserver() from @c DialogUXStateObserverInterface::onDialogUXStateChanged() will result in a
     *     deadlock.
     *
     * @param observer The observer to remove.
     */
    void removeAlexaDialogStateObserver(
            std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface> observer);

    /**
     * Adds an observer to be notified of connection status changes.
     *
     * @param observer The observer to add.
     */
    void addConnectionObserver(std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Removes an observer to be notified of connection status changes.
     *
     * @param observer The observer to remove.
     */
    void removeConnectionObserver(
            std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface> observer);

    /**
     * Begins a wake word initiated Alexa interaction.
     *
     * @param wakeWordAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param beginIndex The begin index of the keyword found within the stream.
     * @param endIndex The end index of the keyword found within the stream.
     * @param keyword The keyword that was detected.
     * @return A future indicating whether the interaction was successfully started.
     */
    std::future<bool> notifyOfWakeWord(
            capabilityAgents::aip::AudioProvider wakeWordAudioProvider,
            avsCommon::avs::AudioInputStream::Index beginIndex,
            avsCommon::avs::AudioInputStream::Index endIndex,
            std::string keyword);

    /**
     * Begins a tap to talk initiated Alexa interaction. Note that this can also be used for wake word engines that
     * don't support providing both a begin and end index.
     *
     * @param tapToTalkAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @param beginIndex An optional parameter indicating where in the stream to start reading from.
     * @return A future indicating whether the interaction was successfully started.
     */
    std::future<bool> notifyOfTapToTalk(
            capabilityAgents::aip::AudioProvider tapToTalkAudioProvider,
            avsCommon::avs::AudioInputStream::Index beginIndex = INVALID_INDEX);

    /**
     * Begins a hold to talk initiated Alexa interaction.
     *
     * @param holdToTalkAudioProvider The audio provider containing the audio data stream along with its metadata.
     * @return A future indicating whether the interaction was successfully started.
     */
    std::future<bool> notifyOfHoldToTalkStart(capabilityAgents::aip::AudioProvider holdToTalkAudioProvider);

    /**
     * Ends a hold to talk interaction by forcing the client to stop streaming audio data to the cloud and ending any
     * currently ongoing recognize interactions.
     * 
     * @return A future indicating whether audio streaming was successfully stopped. This can be false if this was
     * called in the wrong state.
     */
    std::future<bool> notifyOfHoldToTalkEnd();

    /**
     * Destructor.
     */
    ~DefaultClient();

private:
    /**
     * Initializes the SDK and "glues" all the components together.
     * 
     * @param speakMediaPlayer The media player to use to play Alexa speech from.
     * @param audioMediaPlayer The media player to use to play Alexa audio content from.
     * @param alertsMediaPlayer The media player to use to play alerts from.
     * @param authDelegate The component that provides the client with valid LWA authorization.
     * @Param alertStorage The storage interface that will be used to store alerts.
     * @param alexaDialogStateObservers Observers that can be used to be notified of Alexa dialog related UX state 
     * changes.
     * @param connectionObservers Observers that can be used to be notified of connection status changes.
     * @return Whether the SDK was intialized properly.
     */
    bool initialize(
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> speakMediaPlayer,
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> audioMediaPlayer,
            std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface> alertsMediaPlayer,
            std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<capabilityAgents::alerts::storage::AlertStorageInterface> alertStorage,
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::DialogUXStateObserverInterface>> 
                    alexaDialogStateObservers,
            std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::ConnectionStatusObserverInterface>> 
                    connectionObservers);

    /// The directive sequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;

    /// The focus manager.
    std::shared_ptr<afml::FocusManager> m_focusManager;

    /// The connection manager.
    std::shared_ptr<acl::AVSConnectionManager> m_connectionManager;

    /// The audio input processor.
    std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> m_audioInputProcessor;

    /// The audio player.
    std::shared_ptr<capabilityAgents::audioPlayer::AudioPlayer> m_audioPlayer;

    /// The alerts capability agent.
    std::shared_ptr<capabilityAgents::alerts::AlertsCapabilityAgent> m_alertsCapabilityAgent;

    /// The Alexa dialog UX aggregator.
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator> m_dialogUXStateAggregator;

    /// The state synchronizer.
    std::shared_ptr<capabilityAgents::system::StateSynchronizer> m_stateSynchronizer;
};

} // namespace defaultClient
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_DEFAULT_CLIENT_INCLUDE_DEFAULT_CLIENT_H_
