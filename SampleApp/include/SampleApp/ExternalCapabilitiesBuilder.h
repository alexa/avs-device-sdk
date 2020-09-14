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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_EXTERNALCAPABILITIESBUILDER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_EXTERNALCAPABILITIESBUILDER_H_

#include <AVSCommon/Utils/DeviceInfo.h>
#include <DefaultClient/ExternalCapabilitiesBuilderInterface.h>

namespace alexaClientSDK {
namespace sampleApp {
/**
 * This class provides a facility to add external capabilities to default client.
 *
 * Default client will provide its core components in the build call.
 *
 * @note Any object created during the buildCapabilities that keep a pointer to a core component should be added or
 * managed by an object in the list of @c RequiresShutdown objects returned by buildCapabilities(). This will ensure
 * that these objects are shutdown during @c DefaultClient shutdown and before any core component is shutdown.
 */
class ExternalCapabilitiesBuilder : public alexaClientSDK::defaultClient::ExternalCapabilitiesBuilderInterface {
public:
    /**
     * Constructor.
     *
     * @param deviceInfo DeviceInfo which reflects the device setup credentials.
     */
    ExternalCapabilitiesBuilder(std::shared_ptr<avsCommon::utils::DeviceInfo> deviceInfo);

    /// @name ExternalCapabilitiesBuilderInterface methods.
    /// @{
    ExternalCapabilitiesBuilder& withVisualFocusManager(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::FocusManagerInterface> focusManager) override;

    ExternalCapabilitiesBuilder& withSettingsStorage(
        std::shared_ptr<alexaClientSDK::settings::storage ::DeviceSettingStorageInterface> settingStorage) override;

    ExternalCapabilitiesBuilder& withTemplateRunTime(
        std::shared_ptr<alexaClientSDK::capabilityAgents::templateRuntime::TemplateRuntime> templateRuntime) override;

    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CallManagerInterface> getCallManager() override;

    ExternalCapabilitiesBuilder& withInternetConnectionMonitor(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>
            internetConnectionMonitor) override;

    ExternalCapabilitiesBuilder& withDialogUXStateAggregator(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::DialogUXStateAggregator> dialogUXStateAggregator) override;

    std::pair<std::list<Capability>, std::list<std::shared_ptr<alexaClientSDK::avsCommon::utils::RequiresShutdown>>>
    buildCapabilities(
        std::shared_ptr<alexaClientSDK::capabilityAgents::externalMediaPlayer::ExternalMediaPlayer> externalMediaPlayer,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<alexaClientSDK::avsCommon::avs::ExceptionEncounteredSender> exceptionSender,
        std::shared_ptr<alexaClientSDK::certifiedSender::CertifiedSender> certifiedSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
        std::shared_ptr<alexaClientSDK::registrationManager::CustomerDataManager> dataManager,
        std::shared_ptr<alexaClientSDK::capabilityAgents::system::ReportStateHandler> stateReportHandler,
        std::shared_ptr<alexaClientSDK::capabilityAgents::aip::AudioInputProcessor> audioInputProcessor,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<alexaClientSDK::capabilityAgents::system::UserInactivityMonitor> userInactivityMonitor,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AVSGatewayManagerInterface> avsGatewayManager,
        std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> ringtoneMediaPlayer,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::audio::AudioFactoryInterface> audioFactory,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ChannelVolumeInterface>
            ringtoneChannelVolumeInterface,
#ifdef ENABLE_COMMS_AUDIO_PROXY
        std::shared_ptr<alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface> commsMediaPlayer,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface> commsSpeaker,
        std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream> sharedDataStream,
#endif
        std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface> powerResourceManager) override;
    /// @}

private:
    /// DeviceInfo which reflects the device setup credentials.
    std::shared_ptr<alexaClientSDK::avsCommon::utils::DeviceInfo> m_deviceInfo;

    /// When GUI is enabled, this will hold the TemplateRuntime Capability Agent.
    std::shared_ptr<alexaClientSDK::capabilityAgents::templateRuntime::TemplateRuntime> m_templateRuntime;

    /// When COMMS is enabled, this will hold the CallManager.
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::CallManagerInterface> m_callManager;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_SAMPLEAPP_EXTERNALCAPABILITIESBUILDER_H_
