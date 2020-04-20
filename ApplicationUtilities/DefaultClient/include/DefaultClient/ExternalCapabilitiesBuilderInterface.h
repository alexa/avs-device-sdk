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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EXTERNALCAPABILITIESBUILDERINTERFACE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EXTERNALCAPABILITIESBUILDERINTERFACE_H_

#include <list>
#include <utility>

#include <AIP/AudioInputProcessor.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <CertifiedSender/CertifiedSender.h>
#include <ExternalMediaPlayer/ExternalMediaPlayer.h>
#include <MRM/MRMCapabilityAgent.h>
#include <RegistrationManager/CustomerDataManager.h>
#include <Settings/Storage/DeviceSettingStorageInterface.h>
#include <System/ReportStateHandler.h>

namespace alexaClientSDK {
namespace defaultClient {
/**
 * This interface provides a facility to add external capabilities to default client.
 *
 * Default client will provide its core components in the build call.
 *
 * @note Any object created during the buildCapabilities that keep a pointer to a core component should be added or
 * managed by an object in the list of @c RequiresShutdown objects returned by buildCapabilities(). This will ensure
 * that these objects are shutdown during @c DefaultClient shutdown and before any core component is shutdown.
 */
class ExternalCapabilitiesBuilderInterface {
public:
    /**
     * Define a capability structure.
     */
    struct Capability {
        /// The optional capability configuration which will be included in the list of supported capabilities sent
        /// to AVS.
        avsCommon::utils::Optional<avsCommon::avs::CapabilityConfiguration> configuration;

        /// An optional directive handler used to process any directive included in this capability.
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> directiveHandler;
    };

    /**
     * Destructor.
     */
    virtual ~ExternalCapabilitiesBuilderInterface() = default;

    /**
     * This method sets the focus manager responsible for visual interactions.
     *
     * This method will only get called if GUI support has been enabled.
     *
     * @param visualFocusManager The focus manager object.
     * @return A reference to this builder to allow nested function calls.
     */
    virtual ExternalCapabilitiesBuilderInterface& withVisualFocusManager(
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> visualFocusManager) = 0;

    /**
     * This method sets the MRM capability agent.
     *
     * This method will only get called if MRM support has been enabled.
     *
     * @param mrmCapabilityAgent The MRM capability agent.
     * @return A reference to this builder to allow nested function calls.
     */
    virtual ExternalCapabilitiesBuilderInterface& withMRMCapabilityAgent(
        std::shared_ptr<capabilityAgents::mrm::MRMCapabilityAgent> mrmCapabilityAgent) = 0;

    /**
     * This method sets the storage using for setting.
     *
     * @warning The settings storage is opened and closed by DefaultClient during creation and shutdown respectively.
     * You can add your objects that use this storage inside the list of RequiresShutdown objects managed by
     * DefaultClient to ensure the storage usage will not be done after it closure.
     */
    virtual ExternalCapabilitiesBuilderInterface& withSettingsStorage(
        std::shared_ptr<alexaClientSDK::settings::storage::DeviceSettingStorageInterface> settingStorage) = 0;

    /**
     * Build the capabilities with the given core components.
     *
     * @param externalMediaPlayer Object used to manage external media playback.
     * @param connectionManager Object responsible for managing the SDK connection with AVS.
     * @param messageSender Object that can be used to send events to AVS.
     * @param exceptionSender Object that can be used to send exceptions to AVS.
     * @param certifiedSender Object that can be used to send events to AVS that require stronger guarantee.
     * @param dataManager Object used to manage objects that store customer data.
     * @param stateReportHandler Object used to report the device state and its settings.
     * @param audioInputProcessor Object used to recognize voice interactions.
     * @param speakerManager Object used to manage all speaker instances that can be controlled by Alexa.
     * @return A list with all capabilities as well as objects that require explicit shutdown. Shutdown will be
     * performed in the reverse order of occurrence.
     */
    virtual std::pair<std::list<Capability>, std::list<std::shared_ptr<avsCommon::utils::RequiresShutdown>>>
    buildCapabilities(
        std::shared_ptr<capabilityAgents::externalMediaPlayer::ExternalMediaPlayer> externalMediaPlayer,
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::avs::ExceptionEncounteredSender> exceptionSender,
        std::shared_ptr<certifiedSender::CertifiedSender> certifiedSender,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> audioFocusManager,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager,
        std::shared_ptr<capabilityAgents::system::ReportStateHandler> stateReportHandler,
        std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> audioInputProcessor,
        std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> speakerManager) = 0;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_EXTERNALCAPABILITIESBUILDERINTERFACE_H_
