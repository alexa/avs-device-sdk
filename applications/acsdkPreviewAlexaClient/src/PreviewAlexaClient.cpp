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

#include <algorithm>
#include <cctype>
#include <csignal>

#include <acsdkManufactory/Manufactory.h>
#include <AVSCommon/AVS/CapabilitySemantics/CapabilitySemantics.h>
#include <AVSCommon/AVS/Initialization/AlexaClientSDKInit.h>
#include <AVSCommon/AVS/Initialization/InitializationParametersBuilder.h>
#include <AVSCommon/SDKInterfaces/Bluetooth/BluetoothDeviceConnectionRuleInterface.h>
#include <AVSCommon/SDKInterfaces/Diagnostics/ProtocolTracerInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/PowerResourceManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Timing/TimerDelegateFactoryInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <AVSCommon/Utils/LibcurlUtils/HttpPost.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Logger/LoggerSinkManager.h>
#include <AVSCommon/Utils/Network/InternetConnectionMonitor.h>
#include <AVSCommon/Utils/Power/NoOpPowerResourceManager.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <InterruptModel/config/InterruptModelConfiguration.h>
#include <SampleApp/CaptionPresenter.h>
#include <SampleApp/ExternalCapabilitiesBuilder.h>
#include <SampleApp/KeywordObserver.h>
#include <SampleApp/LocaleAssetsManager.h>
#include <SampleApp/PlatformSpecificValues.h>
#include <SpeechEncoder/SpeechEncoder.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>

#ifdef AUTH_MANAGER
#include <acsdkAuthorization/AuthorizationManager.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationAdapter.h>
#include <acsdkAuthorization/LWA/LWAAuthorizationStorage.h>
#include <acsdkAuthorizationInterfaces/AuthorizationManagerInterface.h>
#include <acsdkSampleApplicationCBLAuthRequester/SampleApplicationCBLAuthRequester.h>
#endif

#ifdef ENABLE_REVOKE_AUTH
#include <SampleApp/RevokeAuthorizationObserver.h>
#endif

#ifdef ENABLE_PCC
#include <SampleApp/PhoneCaller.h>
#endif

#ifdef ENABLE_MCC
#include <SampleApp/CalendarClient.h>
#include <SampleApp/MeetingClient.h>
#endif

#ifdef PORTAUDIO
#include <SampleApp/PortAudioMicrophoneWrapper.h>
#endif

#ifdef GSTREAMER_MEDIA_PLAYER
#include <MediaPlayer/MediaPlayer.h>
#endif

#ifdef CUSTOM_MEDIA_PLAYER
#include <acsdkApplicationAudioPipelineFactory/CustomApplicationAudioPipelineFactory.h>
#endif

#ifdef ANDROID
#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
#include <AndroidUtilities/AndroidSLESEngine.h>
#endif

#ifdef ANDROID_MEDIA_PLAYER
#include <AndroidSLESMediaPlayer/AndroidSLESMediaPlayer.h>
#include <AndroidSLESMediaPlayer/AndroidSLESSpeaker.h>
#endif

#ifdef ANDROID_MICROPHONE
#include <AndroidUtilities/AndroidSLESMicrophone.h>
#endif

#ifdef ANDROID_LOGGER
#include <AndroidUtilities/AndroidLogger.h>
#endif
#endif

#ifdef POWER_CONTROLLER
#include <SampleApp/PeripheralEndpoint/PeripheralEndpointPowerControllerHandler.h>
#endif

#ifdef TOGGLE_CONTROLLER
#include <ToggleController/ToggleControllerAttributeBuilder.h>
#include <SampleApp/PeripheralEndpoint/PeripheralEndpointToggleControllerHandler.h>
#include <SampleApp/DefaultEndpoint/DefaultEndpointToggleControllerHandler.h>
#endif

#ifdef RANGE_CONTROLLER
#include <RangeController/RangeControllerAttributeBuilder.h>
#include <SampleApp/PeripheralEndpoint/PeripheralEndpointRangeControllerHandler.h>
#include <SampleApp/DefaultEndpoint/DefaultEndpointRangeControllerHandler.h>
#endif

#ifdef MODE_CONTROLLER
#include <ModeController/ModeControllerAttributeBuilder.h>
#include <SampleApp/PeripheralEndpoint/PeripheralEndpointModeControllerHandler.h>
#include <SampleApp/DefaultEndpoint/DefaultEndpointModeControllerHandler.h>
#endif

#ifdef ENABLE_LPM
#include <AVSCommon/Utils/Power/NoOpPowerResourceManager.h>
#endif

#include "acsdkPreviewAlexaClient/PreviewAlexaClient.h"
#include "acsdkPreviewAlexaClient/PreviewAlexaClientComponent.h"

namespace alexaClientSDK {
namespace acsdkPreviewAlexaClient {

/// Key for the @c firmwareVersion value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string FIRMWARE_VERSION_KEY("firmwareVersion");

/// Key for the @c endpoint value under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string ENDPOINT_KEY("endpoint");

/// Key for setting if display cards are supported or not under the @c SAMPLE_APP_CONFIG_KEY configuration node.
static const std::string DISPLAY_CARD_KEY("displayCardsSupported");

using namespace acsdkExternalMediaPlayer;
using namespace acsdkManufactory;
using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace sampleApp;
using MediaPlayerInterface = alexaClientSDK::avsCommon::utils::mediaPlayer::MediaPlayerInterface;
using ApplicationMediaInterfaces = alexaClientSDK::avsCommon::sdkInterfaces::ApplicationMediaInterfaces;

/** The PreviewAlexaClientManufactory will change significantly while manufactory integration is completed
 * over the next several releases. Applications should not expect this template to be stable until
 * manufactory integration is finished.
 */
using PreviewAlexaClientManufactory = Manufactory<
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::VisualFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>,
    std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>,
    std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>,
    std::shared_ptr<acsdkAudioPlayerInterfaces::AudioPlayerInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothLocalInterface>,
    std::shared_ptr<acsdkBluetoothInterfaces::BluetoothNotifierInterface>,
    std::shared_ptr<acsdkDeviceSetupInterfaces::DeviceSetupInterface>,
    std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>,
    std::shared_ptr<acsdkExternalMediaPlayer::ExternalMediaPlayer>,
    std::shared_ptr<acsdkExternalMediaPlayerInterfaces::ExternalMediaPlayerInterface>,
    std::shared_ptr<acsdkInteractionModelInterfaces::InteractionModelNotifierInterface>,
    std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>,
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownManagerInterface>,
    std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>,
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>,
    std::shared_ptr<afml::interruptModel::InterruptModel>,
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface>,
    std::shared_ptr<avsCommon::avs::AudioInputStream>,
    std::shared_ptr<avsCommon::avs::DialogUXStateAggregator>,
    std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>,
    std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PlaybackRouterInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::RenderPlayerInfoCardsProviderRegistrarInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemSoundPlayerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::SystemTimeZoneInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::UserInactivityMonitorInterface>,
    std::shared_ptr<avsCommon::utils::AudioFormat>,
    std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>,
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
    std::shared_ptr<capabilityAgents::doNotDisturb::DoNotDisturbCapabilityAgent>,
    std::shared_ptr<captions::CaptionManagerInterface>,
    std::shared_ptr<certifiedSender::CertifiedSender>,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    std::shared_ptr<sampleApp::UIManager>,
    std::shared_ptr<settings::DeviceSettingsManager>,
    std::shared_ptr<settings::storage::DeviceSettingStorageInterface>,
    std::shared_ptr<speechencoder::SpeechEncoder>,
    std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>,
    std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>>;

/// String to identify log entries originating from this file.
static const std::string TAG("PreviewAlexaClient");

#ifdef ENABLE_ENDPOINT_CONTROLLERS
/// The instance name for the toggle controller in default endpoint
static const std::string DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME("DefaultEndpoint.Light");

/// The friendly name of the toggle controller in default endpoint.
static const std::string DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME("Light");

/// The instance name for the range controller in the default endpoint.
static const std::string DEFAULT_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME("DefaultEndpoint.FanSpeed");

/// The instance name for the mode controller in the default endpoint.
static const std::string DEFAULT_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME("DefaultEndpoint.Mode");

#ifdef RANGE_CONTROLLER
/// The range value for the prest 'high' for range controller in the default endpoint.
static const double DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH = 5;

/// The range value for the prest 'medium' for range controller in the default endpoint.
static const double DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM = 3;

/// The range value for the prest 'low' for range controller in the default endpoint.
static const double DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW = 1;
#endif  // RANGE_CONTROLLER

// Note: Discoball is an imaginary peripheral endpoint that is connected to the device with the following
// capabilities : power, light (toggle), height (range), and the color of light (mode).
// Discoball uses semantic annotations to enable additional natural utterances.

/// The derived endpoint Id used in endpoint creation.
static const std::string PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID("Discoball");

/// The description of the endpoint.
static const std::string PERIPHERAL_ENDPOINT_DESCRIPTION("Sample Discoball Description");

/// The friendly name of the peripheral endpoint. This is used in utterance.
static const std::string PERIPHERAL_ENDPOINT_FRIENDLYNAME("Discoball");

/// The manufacturer of peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_MANUFACTURER_NAME("Sample Manufacturer");

/// The display category of the peripheral endpoint.
static const std::vector<std::string> PERIPHERAL_ENDPOINT_DISPLAYCATEGORY({"OTHER"});

/// The instance name for the toggle controller in peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME("Discoball.Light");

/// The friendly name of the toggle controller on peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME("Light");

/// The instance name for the range controller peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME("Discoball.Height");

/// The friendly name of the range controller on peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_FRIENDLY_NAME("Height");

/// The instance name for the mode controller peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME("Discoball.Mode");

/// The friendly name of the mode controller on peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_MODE_CONTROLLER_FRIENDLY_NAME("Light");

/// The model of the peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL("Model1");

/// Serial number of the peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER("123456789");

/// Firmware version number peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION("1.0");

/// Software Version number peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION("1.0");

/// The custom identifier peripheral endpoint.
static const std::string PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER("SampleApp");

#ifdef RANGE_CONTROLLER
/// The range controller preset 'high' in peripheral endpoint.
static const double PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH = 10;

/// The range controller preset 'medium' in peripheral endpoint.
static const double PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM = 5;

/// The range controller preset 'low' in peripheral endpoint.
static const double PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW = 1;

/// The action ID for "raise" semantic annotations.
static const std::string SEMANTICS_ACTION_ID_RAISE("Alexa.Actions.Raise");

/// The action ID for "lower" semantic annotations.
static const std::string SEMANTICS_ACTION_ID_LOWER("Alexa.Actions.Lower");

/// The range controller SetRangeValue directive name.
static const std::string SETRANGE_DIRECTIVE_NAME("SetRangeValue");

/// The range controller SetRangeValue payload mapped to the raise action.
static const std::string PERIPHERAL_ENDPOINT_RAISE_PAYLOAD =
    R"({"rangeValue":)" + std::to_string(PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH) + R"(})";

/// The range controller SetRangeValue payload mapped to the lower action.
static const std::string PERIPHERAL_ENDPOINT_LOWER_PAYLOAD =
    R"({"rangeValue":)" + std::to_string(PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW) + R"(})";
#endif  // RANGE_CONTROLLER

/// US English locale string.
static const std::string EN_US("en-US");
#endif  // ENABLE_ENDPOINT_CONTROLLERS

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// A set of all log levels.
static const std::set<alexaClientSDK::avsCommon::utils::logger::Level> allLevels = {
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG9,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG8,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG7,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG6,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG5,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG4,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG3,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG2,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG1,
    alexaClientSDK::avsCommon::utils::logger::Level::DEBUG0,
    alexaClientSDK::avsCommon::utils::logger::Level::INFO,
    alexaClientSDK::avsCommon::utils::logger::Level::WARN,
    alexaClientSDK::avsCommon::utils::logger::Level::ERROR,
    alexaClientSDK::avsCommon::utils::logger::Level::CRITICAL,
    alexaClientSDK::avsCommon::utils::logger::Level::NONE};

#ifdef ENABLE_ENDPOINT_CONTROLLERS

/// Struct representing the friendly name.
struct FriendlyName {
    /// Type of the friendly name.
    enum class Type {
        /// Friendly name as asset type.
        ASSET,
        /// Friendly name as text type.
        TEXT
    };

    /// Holds the type of the friendly name.
    Type type;

    /// Value contains the assetId when Type::ASSET or it contains the text when Type::TEXT.
    std::string value;
};

/// The capability resources of primitive controllers.
struct CapabilityResources {
    /// Represents the friendly name of capability.
    std::vector<FriendlyName> friendlyNames;
};

#ifdef RANGE_CONTROLLER
/// This struct represents the Range Controller preset and its friendly names.
struct RangeControllerPresetResources {
    /// The value of a preset.
    double presetValue;

    /// The friendly names of the presets.
    std::vector<FriendlyName> friendlyNames;
};
#endif

#ifdef MODE_CONTROLLER
/// This struct represents the Mode Controller modes and its friendly names.
struct ModeControllerModeResources {
    /// The mode in the Mode Controller.
    std::string mode;

    /// The friendly names of the @c mode.
    std::vector<FriendlyName> friendlyNames;
};
#endif
#endif

/**
 * Gets a log level consumable by the SDK based on the user input string for log level.
 *
 * @param userInputLogLevel The string to be parsed into a log level.
 * @return The log level. This will default to NONE if the input string is not properly parsable.
 */
static alexaClientSDK::avsCommon::utils::logger::Level getLogLevelFromUserInput(std::string userInputLogLevel) {
    std::transform(userInputLogLevel.begin(), userInputLogLevel.end(), userInputLogLevel.begin(), ::toupper);
    return alexaClientSDK::avsCommon::utils::logger::convertNameToLevel(userInputLogLevel);
}

/**
 * Allows the process to ignore the SIGPIPE signal.
 * The SIGPIPE signal may be received when the application performs a write to a closed socket.
 * This is a case that arises in the use of certain networking libraries.
 *
 * @return true if the action for handling SIGPIPEs was correctly set to ignore, else false.
 */
static bool ignoreSigpipeSignals() {
#ifndef NO_SIGPIPE
    if (std::signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        return false;
    }
#endif
    return true;
}

#ifdef ENABLE_ENDPOINT_CONTROLLERS
#ifdef TOGGLE_CONTROLLER
/**
 * Helper function to build the @c ToggleControllerAttributes
 *
 * @param capabilityResources The @c CapabilityResources containing friendly names of controller.
 * @return A non optional @c ToggleControllerAttributes if the build succeeds; otherwise, an empty
 * @c ToggleControllerAttributes object.
 */
static avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes>
buildToggleControllerAttributes(const CapabilityResources& capabilityResources) {
    auto controllerAttribute =
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::toggleController::ToggleControllerAttributes>();
    auto toggleControllerAttributeBuilder =
        capabilityAgents::toggleController::ToggleControllerAttributeBuilder::create();
    if (!toggleControllerAttributeBuilder) {
        ACSDK_ERROR(LX("Failed to create default endpoint toggle controller attribute builder!"));
        return controllerAttribute;
    }

    if (capabilityResources.friendlyNames.size() == 0) {
        ACSDK_ERROR(LX("buildToggleControllerAttributesFailed").m("noFriendlyNames"));
        return controllerAttribute;
    }

    auto toggleControllerCapabilityResources = avsCommon::avs::CapabilityResources();
    for (auto& friendlyName : capabilityResources.friendlyNames) {
        if (FriendlyName::Type::ASSET == friendlyName.type) {
            if (!toggleControllerCapabilityResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                ACSDK_ERROR(LX("buildToggleControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
        if (FriendlyName::Type::TEXT == friendlyName.type) {
            if (!toggleControllerCapabilityResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                ACSDK_ERROR(LX("buildToggleControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
    }

    toggleControllerAttributeBuilder->withCapabilityResources(toggleControllerCapabilityResources);

    return toggleControllerAttributeBuilder->build();
}
#endif  // TOGGLE_CONTROLLER

#ifdef RANGE_CONTROLLER
/**
 * Helper function to build the @c RangeControllerAttributes
 *
 * @param capabilityResources The @c CapabilityResources containing friendly names of controller.
 * @param rangeControllerPresetResources A vector representing the preset resources used for building the configuration.
 * @param semantics An optional @c CapabilitySemantics for the range controller.
 * @return A non optional @c RangeControllerAttributes if the build succeeds; otherwise, an empty
 * @c RangeControllerAttributes object.
 */
static avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>
buildRangeControllerAttributes(
    const CapabilityResources& capabilityResources,
    const std::vector<RangeControllerPresetResources>& rangeControllerPresetResources,
    const avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics> semantics =
        avsCommon::utils::Optional<avsCommon::avs::capabilitySemantics::CapabilitySemantics>()) {
    auto controllerAttribute =
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::rangeController::RangeControllerAttributes>();

    auto rangeControllerAttributeBuilder = capabilityAgents::rangeController::RangeControllerAttributeBuilder::create();
    if (!rangeControllerAttributeBuilder) {
        ACSDK_ERROR(LX("Failed to create range controller attribute builder!"));
        return controllerAttribute;
    }

    if (capabilityResources.friendlyNames.size() == 0) {
        ACSDK_ERROR(LX("buildRangeControllerAttributesFailed").m("emptyCapabilityFriendlyNames"));
        return controllerAttribute;
    }

    auto rangeControllerCapabilityResources = avsCommon::avs::CapabilityResources();
    for (auto& friendlyName : capabilityResources.friendlyNames) {
        if (FriendlyName::Type::ASSET == friendlyName.type) {
            if (!rangeControllerCapabilityResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
        if (FriendlyName::Type::TEXT == friendlyName.type) {
            if (!rangeControllerCapabilityResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
    }

    rangeControllerAttributeBuilder->withCapabilityResources(rangeControllerCapabilityResources);

    if (rangeControllerPresetResources.size() > 0) {
        for (auto& presetResource : rangeControllerPresetResources) {
            if (presetResource.friendlyNames.size() == 0) {
                ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                .m("buildRangeControllerAttributesFailed")
                                .m("noPresetFriendlyNames")
                                .d("presetValue", presetResource.presetValue));
                return controllerAttribute;
            }
            auto presetResources = avsCommon::avs::CapabilityResources();
            for (auto& friendlyName : presetResource.friendlyNames) {
                if (FriendlyName::Type::ASSET == friendlyName.type) {
                    if (!presetResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                        ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("presetValue", presetResource.presetValue));
                        return controllerAttribute;
                    }
                }
                if (FriendlyName::Type::TEXT == friendlyName.type) {
                    if (!presetResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                        ACSDK_ERROR(LX("buildRangeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("presetValue", presetResource.presetValue));
                        return controllerAttribute;
                    }
                }
            }
            rangeControllerAttributeBuilder->addPreset(std::make_pair(presetResource.presetValue, presetResources));
        }
    }

    if (semantics.hasValue()) {
        if (!semantics.value().isValid()) {
            ACSDK_ERROR(LX("buildRangeControllerAttributes").m("invalidSemantics"));
            return controllerAttribute;
        }
        rangeControllerAttributeBuilder->withSemantics(semantics.value());
    }

    return rangeControllerAttributeBuilder->build();
}
#endif  // RANGE_CONTROLLER

#ifdef MODE_CONTROLLER
/**
 * Helper function to build the @c ModeControllerAttributes
 *
 * @param capabilityResources The @c CapabilityResources containing friendly names of controller.
 * @param presetResources A vector representing the mode resources used for building the configuration.
 * @return A non optional @c ModeControllerAttributes if the build succeeds; otherwise, an empty
 * @c ModeControllerAttributes object.
 */
static avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>
buildModeControllerAttributes(
    const CapabilityResources& capabilityResources,
    const std::vector<ModeControllerModeResources>& modeControllerModeResources) {
    auto controllerAttribute =
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::modeController::ModeControllerAttributes>();

    auto modeControllerAttributeBuilder = capabilityAgents::modeController::ModeControllerAttributeBuilder::create();
    if (!modeControllerAttributeBuilder) {
        ACSDK_ERROR(LX("Failed to create mode controller attribute builder!"));
        return controllerAttribute;
    }

    if (capabilityResources.friendlyNames.size() == 0) {
        ACSDK_ERROR(LX("buildModeControllerAttributesFailed").m("emptyCapabilityFriendlyNames"));
        return controllerAttribute;
    }

    auto modeControllerCapabilityResources = avsCommon::avs::CapabilityResources();
    for (auto& friendlyName : capabilityResources.friendlyNames) {
        if (FriendlyName::Type::ASSET == friendlyName.type) {
            if (!modeControllerCapabilityResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                ACSDK_ERROR(LX("buildModeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
        if (FriendlyName::Type::TEXT == friendlyName.type) {
            if (!modeControllerCapabilityResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                ACSDK_ERROR(LX("buildModeControllerAttributes")
                                .m("addFriendlyNameWithAssetIdFailed")
                                .d("value", friendlyName.value));
                return controllerAttribute;
            }
        }
    }

    modeControllerAttributeBuilder->withCapabilityResources(modeControllerCapabilityResources);

    if (modeControllerModeResources.size() > 0) {
        for (auto& modeResource : modeControllerModeResources) {
            if (modeResource.friendlyNames.size() == 0) {
                ACSDK_ERROR(LX("buildModeControllerAttributes")
                                .m("buildModeControllerAttributesFailed")
                                .m("noPresetFriendlyNames")
                                .d("mode", modeResource.mode));
                return controllerAttribute;
            }
            auto modeResources = avsCommon::avs::CapabilityResources();
            for (auto& friendlyName : modeResource.friendlyNames) {
                if (FriendlyName::Type::ASSET == friendlyName.type) {
                    if (!modeResources.addFriendlyNameWithAssetId(friendlyName.value)) {
                        ACSDK_ERROR(LX("buildModeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("mode", modeResource.mode));
                        return controllerAttribute;
                    }
                }
                if (FriendlyName::Type::TEXT == friendlyName.type) {
                    if (!modeResources.addFriendlyNameWithText(friendlyName.value, EN_US)) {
                        ACSDK_ERROR(LX("buildModeControllerAttributes")
                                        .m("addFriendlyNameWithAssetIdFailed")
                                        .d("value", friendlyName.value)
                                        .d("mode", modeResource.mode));
                        return controllerAttribute;
                    }
                }
            }
            modeControllerAttributeBuilder->addMode(modeResource.mode, modeResources);
        }
        modeControllerAttributeBuilder->setOrdered(true);
    }

    return modeControllerAttributeBuilder->build();
}
#endif  // MODE_CONTROLLER
#endif  // ENABLE_ENDPOINT_CONTROLLERS

std::unique_ptr<PreviewAlexaClient> PreviewAlexaClient::create(
    std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
    const std::vector<std::string>& configFiles,
    const std::string& logLevel,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) {
    auto clientApplication = std::unique_ptr<PreviewAlexaClient>(new PreviewAlexaClient);
    if (!clientApplication->initialize(consoleReader, configFiles, logLevel, diagnostics)) {
        ACSDK_CRITICAL(LX("Failed to initialize SampleApplication"));
        return nullptr;
    }
    if (!ignoreSigpipeSignals()) {
        ACSDK_CRITICAL(LX("Failed to set a signal handler for SIGPIPE"));
        return nullptr;
    }

    return clientApplication;
}

SampleAppReturnCode PreviewAlexaClient::run() {
    return m_userInputManager->run();
}

PreviewAlexaClient::~PreviewAlexaClient() {
    if (m_shutdownManager) {
        m_shutdownManager->shutdown();
    }
    // Clean up anything that depends on the the MediaPlayers.
    m_userInputManager.reset();

    for (auto& shutdownable : m_shutdownRequiredList) {
        if (!shutdownable) {
            ACSDK_ERROR(LX("shutdownFailed").m("Component requiring shutdown was null"));
        }
        shutdownable->shutdown();
    }

    m_sdkInit.reset();
}

bool PreviewAlexaClient::initialize(
    std::shared_ptr<alexaClientSDK::sampleApp::ConsoleReader> consoleReader,
    const std::vector<std::string>& configFiles,
    const std::string& logLevel,
    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface> diagnostics) {
    avsCommon::utils::logger::Level logLevelValue = avsCommon::utils::logger::Level::UNKNOWN;

    if (!logLevel.empty()) {
        logLevelValue = getLogLevelFromUserInput(logLevel);
        if (alexaClientSDK::avsCommon::utils::logger::Level::UNKNOWN == logLevelValue) {
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Unknown log level input!");
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Possible log level options are: ");
            for (auto it = allLevels.begin(); it != allLevels.end(); ++it) {
                alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
                    alexaClientSDK::avsCommon::utils::logger::convertLevelToName(*it));
            }
            return false;
        }

        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "Running app with log level: " +
            alexaClientSDK::avsCommon::utils::logger::convertLevelToName(logLevelValue));
    }

    alexaClientSDK::avsCommon::utils::logger::LoggerSinkManager::instance().setLevel(logLevelValue);

    auto configJsonStreams = std::make_shared<std::vector<std::shared_ptr<std::istream>>>();

    for (auto configFile : configFiles) {
        if (configFile.empty()) {
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Config filename is empty!");
            return false;
        }

        auto configInFile = std::shared_ptr<std::ifstream>(new std::ifstream(configFile));
        if (!configInFile->good()) {
            ACSDK_CRITICAL(LX("Failed to read config file").d("filename", configFile));
            alexaClientSDK::sampleApp::ConsolePrinter::simplePrint("Failed to read config file " + configFile);
            return false;
        }

        configJsonStreams->push_back(configInFile);
    }

    auto platformSpecificValues = std::shared_ptr<sampleApp::PlatformSpecificValues>(new PlatformSpecificValues());
#if defined(ANDROID_MEDIA_PLAYER) || defined(ANDROID_MICROPHONE)
    m_openSlEngine = applicationUtilities::androidUtilities::AndroidSLESEngine::create();
    if (!m_openSlEngine) {
        ACSDK_ERROR(LX("createAndroidMicFailed").d("reason", "failed to create engine"));
        return false;
    }

    platformSpecificValues->openSlEngine = m_openSlEngine;
#endif

#ifdef DISABLE_DUCKING
    bool enableDucking = false;
#else
    bool enableDucking = true;
#endif

    // Add the InterruptModel Configuration.
    configJsonStreams->push_back(
        alexaClientSDK::afml::interruptModel::InterruptModelConfiguration::getConfig(enableDucking));

    auto builder = initialization::InitializationParametersBuilder::create();
    if (!builder) {
        ACSDK_ERROR(LX("createInitializeParamsFailed").d("reason", "nullBuilder"));
        return false;
    }

    builder->withJsonStreams(configJsonStreams);

    auto powerResourceManager = std::make_shared<avsCommon::utils::power::NoOpPowerResourceManager>();

#ifdef ENABLE_LPM
    builder->withPowerResourceManager(powerResourceManager);
#endif

    auto initParams = builder->build();

    auto previewAlexaClientComponent = acsdkPreviewAlexaClient::getComponent(
        std::move(initParams), diagnostics, platformSpecificValues, nullptr, powerResourceManager);

    std::shared_ptr<PreviewAlexaClientManufactory> manufactory =
        PreviewAlexaClientManufactory::create(previewAlexaClientComponent);

    m_sdkInit = manufactory->get<std::shared_ptr<avsCommon::avs::initialization::AlexaClientSDKInit>>();

    if (!m_sdkInit) {
        ACSDK_CRITICAL(LX("Failed to get SDKInit!"));
        return false;
    }

    auto startupManager = manufactory->get<std::shared_ptr<acsdkStartupManagerInterfaces::StartupManagerInterface>>();
    if (!startupManager) {
        ACSDK_CRITICAL(LX("Failed to get StartupManager!"));
        return false;
    }

    if (!startupManager->startup()) {
        ACSDK_CRITICAL(LX("Startup Failed!"));
        return false;
    }

    auto configPtr = manufactory->get<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>();
    if (!configPtr) {
        ACSDK_CRITICAL(LX("Failed to get the configuration"));
        return false;
    }
    auto& config = *configPtr;
    std::string sampleAppConfigKey = SAMPLE_APP_CONFIG_KEY;
    auto sampleAppConfig = config[sampleAppConfigKey];

    auto httpContentFetcherFactory =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>>();
    if (!httpContentFetcherFactory) {
        ACSDK_CRITICAL(LX("Failed to get HTTPContentFetcherFactory!"));
        return false;
    }

    auto miscStorage = manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>>();
    if (!miscStorage) {
        ACSDK_CRITICAL(LX("Failed to get MiscStorage!"));
        return false;
    }

    /*
     * Creating customerDataManager which will be used by the registrationManager and all classes that extend
     * CustomerDataHandler
     */
    auto customerDataManager = manufactory->get<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>();
    if (!customerDataManager) {
        ACSDK_CRITICAL(LX("Failed to get CustomerDataManager!"));
        return false;
    }

    std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate;

#ifdef AUTH_MANAGER
    m_authManager = acsdkAuthorization::AuthorizationManager::create(miscStorage, customerDataManager);
    if (!m_authManager) {
        ACSDK_CRITICAL(LX("Failed to create AuthorizationManager!"));
        return false;
    }
    authDelegate = m_authManager;
#else
    /*
     * Creating the AuthDelegate - this component takes care of LWA and authorization of the client.
     */
    authDelegate = manufactory->get<std::shared_ptr<AuthDelegateInterface>>();
#endif

    if (!authDelegate) {
        ACSDK_CRITICAL(LX("Creation of AuthDelegate failed!"));
        return false;
    }

    auto equalizerRuntimeSetup =
        manufactory->get<std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>>();
    if (!equalizerRuntimeSetup) {
        ACSDK_CRITICAL(LX("Failed to get EqualizerRuntimeSetup!"));
        return false;
    }

    auto ringtoneMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "RingtoneMediaPlayer");
    if (!ringtoneMediaInterfaces) {
        alexaClientSDK::sampleApp::ConsolePrinter::simplePrint(
            "Failed to create application media interfaces for ringtones!");
        return false;
    }
    m_ringtoneMediaPlayer = ringtoneMediaInterfaces->mediaPlayer;

#ifdef ENABLE_COMMS_AUDIO_PROXY
    auto commsMediaInterfaces =
        createApplicationMediaPlayer(httpContentFetcherFactory, false, "CommsMediaPlayer", true);
    if (!commsMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for comms!"));
        return false;
    }
    m_commsMediaPlayer = commsMediaInterfaces->mediaPlayer;
    auto commsSpeaker = commsMediaInterfaces->speaker;
#endif

#ifdef ENABLE_PCC
    auto phoneMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "PhoneMediaPlayer");
    if (!phoneMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for phone!"));
        return false;
    }
    auto phoneSpeaker = phoneMediaInterfaces->speaker;
#endif

#ifdef ENABLE_MCC
    auto meetingMediaInterfaces = createApplicationMediaPlayer(httpContentFetcherFactory, false, "MeetingMediaPlayer");
    if (!meetingMediaInterfaces) {
        ACSDK_CRITICAL(LX("Failed to create application media interfaces for meeting client!"));
        return false;
    }
    auto meetingSpeaker = meetingMediaInterfaces->speaker;
#endif

    // Creating the message storage object to be used for storing message to be sent later.
    auto messageStorage = alexaClientSDK::certifiedSender::SQLiteMessageStorage::create(config);

    /*
     * Create sample locale asset manager.
     */
    auto localeAssetsManager = manufactory->get<std::shared_ptr<LocaleAssetsManagerInterface>>();
    if (!localeAssetsManager) {
        ACSDK_CRITICAL(LX("Failed to get LocaleAssetsManager!"));
        return false;
    }

    /*
     * Creating the UI component that observes various components and prints to the console accordingly.
     */
    auto userInterfaceManager = manufactory->get<std::shared_ptr<alexaClientSDK::sampleApp::UIManager>>();
    if (!userInterfaceManager) {
        ACSDK_CRITICAL(LX("Failed to get UIManager!"));
        return false;
    }

#ifdef ENABLE_PCC
    auto phoneCaller = std::make_shared<alexaClientSDK::sampleApp::PhoneCaller>();
#endif

#ifdef ENABLE_MCC
    auto meetingClient = std::make_shared<alexaClientSDK::sampleApp::MeetingClient>();
    auto calendarClient = std::make_shared<alexaClientSDK::sampleApp::CalendarClient>();
#endif

    /*
     * Creating the deviceInfo object
     */
    auto deviceInfo = manufactory->get<std::shared_ptr<avsCommon::utils::DeviceInfo>>();
    if (!deviceInfo) {
        ACSDK_CRITICAL(LX("Creation of DeviceInfo failed!"));
        return false;
    }

    /*
     * Supply a SALT for UUID generation, this should be as unique to each individual device as possible
     */
    alexaClientSDK::avsCommon::utils::uuidGeneration::setSalt(
        deviceInfo->getClientId() + deviceInfo->getDeviceSerialNumber());

    /*
     * Creating the CapabilitiesDelegate - This component provides the client with the ability to send messages to the
     * Capabilities API.
     */
    m_capabilitiesDelegate =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>>();
    if (!m_capabilitiesDelegate) {
        ACSDK_CRITICAL(LX("Creation of CapabilitiesDelegate failed!"));
        return false;
    }

    authDelegate->addAuthObserver(userInterfaceManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(userInterfaceManager);

    // INVALID_FIRMWARE_VERSION is passed to @c getInt() as a default in case FIRMWARE_VERSION_KEY is not found.
    int firmwareVersion = static_cast<int>(avsCommon::sdkInterfaces::softwareInfo::INVALID_FIRMWARE_VERSION);
    sampleAppConfig.getInt(FIRMWARE_VERSION_KEY, &firmwareVersion, firmwareVersion);

    /*
     * Check to see if displayCards is supported on the device. The default is supported unless specified otherwise in
     * the configuration.
     */
    bool displayCardsSupported;
    sampleAppConfig.getBool(DISPLAY_CARD_KEY, &displayCardsSupported, true);

    /*
     * Creating the InternetConnectionMonitor that will notify observers of internet connection status changes.
     */
    auto internetConnectionMonitor =
        manufactory->get<std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>>();
    if (!internetConnectionMonitor) {
        ACSDK_CRITICAL(LX("Failed to get InternetConnectionMonitor"));
        return false;
    }

    /*
     * Creating the buffer (Shared Data Stream) that will hold user audio data. This is the main input into the SDK.
     */
    auto sharedDataStream = manufactory->get<std::shared_ptr<alexaClientSDK::avsCommon::avs::AudioInputStream>>();
    if (!sharedDataStream) {
        ACSDK_CRITICAL(LX("Failed to get shared data stream!"));
        return false;
    }

    auto compatibleAudioFormat = manufactory->get<std::shared_ptr<avsCommon::utils::AudioFormat>>();
    if (!compatibleAudioFormat) {
        ACSDK_CRITICAL(LX("Failed to get compatible audio format!"));
        return false;
    }

    /*
     * Creating the Context Manager - This component manages the context of each of the components to update to AVS.
     * It is required for each of the capability agents so that they may provide their state just before any event is
     * fired off.
     */
    auto contextManager = manufactory->get<std::shared_ptr<ContextManagerInterface>>();
    if (!contextManager) {
        ACSDK_CRITICAL(LX("Creation of ContextManager failed."));
        return false;
    }

    std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::ProtocolTracerInterface> deviceProtocolTracer;
    if (diagnostics) {
        /*
         * Create the deviceProtocolTracer to trace events and directives.
         */
        deviceProtocolTracer = diagnostics->getProtocolTracer();

        if (deviceProtocolTracer) {
            const std::string DIAGNOSTICS_KEY = "diagnostics";
            const std::string MAX_TRACED_MESSAGES_KEY = "maxTracedMessages";
            const std::string TRACE_FROM_STARTUP = "protocolTraceFromStartup";

            int configMaxValue = -1;
            bool configProtocolTraceEnabled = false;

            if (alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[DIAGNOSTICS_KEY].getInt(
                    MAX_TRACED_MESSAGES_KEY, &configMaxValue)) {
                if (configMaxValue < 0) {
                    ACSDK_WARN(LX("ignoringMaxTracedMessages")
                                   .d("reason", "negativeValue")
                                   .d("maxTracedMessages", configMaxValue));
                } else {
                    deviceProtocolTracer->setMaxMessages(static_cast<unsigned int>(configMaxValue));
                }
            }

            if (alexaClientSDK::avsCommon::utils::configuration::ConfigurationNode::getRoot()[DIAGNOSTICS_KEY].getBool(
                    TRACE_FROM_STARTUP, &configProtocolTraceEnabled)) {
                if (configProtocolTraceEnabled) {
                    deviceProtocolTracer->setProtocolTraceFlag(configProtocolTraceEnabled);
                    ACSDK_DEBUG9(LX("Protocol Trace has been enabled at startup"));
                }
            }
        }
    }

    /*
     * Create a 'subset' of the SampleApp Manufactory that provides the types directly consumed by DefaultClient.
     */
    std::shared_ptr<defaultClient::DefaultClient::DefaultClientSubsetManufactory> subsetManufactory =
        defaultClient::DefaultClient::DefaultClientSubsetManufactory::createSubsetManufactory(manufactory);

    /*
     * Creating each of the audio providers. An audio provider is a simple package of data consisting of the stream
     * of audio data, as well as metadata about the stream. For each of the three audio providers created here, the same
     * stream is used since this sample application will only have one microphone.
     */

    // Creating tap to talk audio provider.
    auto tapToTalkAudioProvider = alexaClientSDK::capabilityAgents::aip::AudioProvider::TapAudioProvider(
        sharedDataStream, *compatibleAudioFormat);

    // Creating hold to talk audio provider.
    auto holdToTalkAudioProvider = alexaClientSDK::capabilityAgents::aip::AudioProvider::HoldAudioProvider(
        sharedDataStream, *compatibleAudioFormat);

    /*
     * Creating the DefaultClient - this component serves as an out-of-box default object that instantiates and "glues"
     * together all the modules.
     */
    std::shared_ptr<defaultClient::DefaultClient> client = alexaClientSDK::defaultClient::DefaultClient::create(
        subsetManufactory,
        m_ringtoneMediaPlayer,
        ringtoneMediaInterfaces->speaker,
        {},
#ifdef ENABLE_PCC
        phoneSpeaker,
        phoneCaller,
#endif
#ifdef ENABLE_MCC
        meetingSpeaker,
        meetingClient,
        calendarClient,
#endif
#ifdef ENABLE_COMMS_AUDIO_PROXY
        m_commsMediaPlayer,
        commsSpeaker,
        sharedDataStream,
#endif
        {userInterfaceManager},
        {userInterfaceManager},
        displayCardsSupported,
        firmwareVersion,
        true,
        nullptr,
        diagnostics,
        std::make_shared<alexaClientSDK::sampleApp::ExternalCapabilitiesBuilder>(deviceInfo),
        tapToTalkAudioProvider);
    if (!client) {
        ACSDK_CRITICAL(LX("Failed to create default SDK client!"));
        return false;
    }

    client->addSpeakerManagerObserver(userInterfaceManager);

    client->addNotificationsObserver(userInterfaceManager);

    client->addBluetoothDeviceObserver(userInterfaceManager);

    userInterfaceManager->configureSettingsNotifications(client->getSettingsManager());

    m_shutdownManager = client->getShutdownManager();
    if (!m_shutdownManager) {
        ACSDK_CRITICAL(LX("Failed to get ShutdownManager!"));
        return false;
    }

    /*
     * Add GUI Renderer as an observer if display cards are supported.
     */
    if (displayCardsSupported) {
        m_guiRenderer = std::make_shared<GuiRenderer>();
        client->addTemplateRuntimeObserver(m_guiRenderer);
    }

#ifdef PORTAUDIO
    std::shared_ptr<PortAudioMicrophoneWrapper> micWrapper = PortAudioMicrophoneWrapper::create(sharedDataStream);
#elif defined(ANDROID_MICROPHONE)
    std::shared_ptr<applicationUtilities::androidUtilities::AndroidSLESMicrophone> micWrapper =
        m_openSlEngine->createAndroidMicrophone(sharedDataStream);
#elif AUDIO_INJECTION
    std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::diagnostics::AudioInjectorInterface> audioInjector;
    if (diagnostics) {
        audioInjector = diagnostics->getAudioInjector();
    }

    if (!audioInjector) {
        ACSDK_CRITICAL(LX("No audio injector provided!"));
        return false;
    }
    std::shared_ptr<applicationUtilities::resources::audio::MicrophoneInterface> micWrapper =
        audioInjector->getMicrophone(sharedDataStream, *compatibleAudioFormat);
#else
    ACSDK_CRITICAL(LX("No microphone module provided!"));
    return false;
#endif
    if (!micWrapper) {
        ACSDK_CRITICAL(LX("Failed to create microphone wrapper!"));
        return false;
    }

#ifdef ENABLE_ENDPOINT_CONTROLLERS
    // Default Endpoint
    if (!addControllersToDefaultEndpoint(client->getDefaultEndpointBuilder())) {
        ACSDK_CRITICAL(LX("Failed to add controllers to default endpoint!"));
        return false;
    }

    // Peripheral Endpoint
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> peripheralEndpointBuilder =
        client->createEndpointBuilder();
    if (!peripheralEndpointBuilder) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint Builder!"));
        return false;
    }

    peripheralEndpointBuilder->withDerivedEndpointId(PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID)
        .withDescription(PERIPHERAL_ENDPOINT_DESCRIPTION)
        .withFriendlyName(PERIPHERAL_ENDPOINT_FRIENDLYNAME)
        .withManufacturerName(PERIPHERAL_ENDPOINT_MANUFACTURER_NAME)
        .withAdditionalAttributes(
            PERIPHERAL_ENDPOINT_MANUFACTURER_NAME,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_MODEL,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SERIAL_NUMBER,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_FIRMWARE_VERSION,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_SOFTWARE_VERSION,
            PERIPHERAL_ENDPOINT_ADDITIONAL_ATTRIBUTE_CUSTOM_IDENTIFIER)
        .withDisplayCategory(PERIPHERAL_ENDPOINT_DISPLAYCATEGORY);

    if (!addControllersToPeripheralEndpoint(peripheralEndpointBuilder)) {
        ACSDK_CRITICAL(LX("Failed to add controllers to peripheral endpoint!"));
        return false;
    }

    auto peripheralEndpoint = peripheralEndpointBuilder->build();
    if (!peripheralEndpoint) {
        ACSDK_CRITICAL(LX("Failed to create Peripheral Endpoint!"));
        return false;
    }

    client->registerEndpoint(std::move(peripheralEndpoint));
#endif

    // Create null wake word audio provider and replace with wake word audio provider if KWD is on.
    auto wakeWordAudioProvider = alexaClientSDK::capabilityAgents::aip::AudioProvider::null();
#ifdef KWD
    // Check if keywordDetector was provided to manufactory and create wakeWordAudioProvider and keywordObserver if that
    // is the case.
    m_keywordDetector =
        manufactory->get<std::shared_ptr<alexaClientSDK::acsdkKWDImplementations::AbstractKeywordDetector>>();
    if (m_keywordDetector) {
        wakeWordAudioProvider = alexaClientSDK::capabilityAgents::aip::AudioProvider::WakeAudioProvider(
            sharedDataStream, *compatibleAudioFormat);
        auto keywordObserver =
            alexaClientSDK::sampleApp::KeywordObserver::create(client, wakeWordAudioProvider, m_keywordDetector);
    } else {
        ACSDK_CRITICAL(LX("Failed to create KWD"));
        return false;
    }
#endif

    // clang-format off
    // Create InteractionManager
    m_interactionManager = std::make_shared<alexaClientSDK::sampleApp::InteractionManager>(
        client,
        micWrapper,
        userInterfaceManager,
#ifdef ENABLE_PCC
        phoneCaller,
#endif
#ifdef ENABLE_MCC
        meetingClient,
        calendarClient,
#endif
        holdToTalkAudioProvider,
        tapToTalkAudioProvider,
        m_guiRenderer,
        wakeWordAudioProvider
#ifdef POWER_CONTROLLER
        ,
        m_peripheralEndpointPowerHandler
#endif
#ifdef TOGGLE_CONTROLLER
        ,
        m_peripheralEndpointToggleHandler
#endif
#ifdef RANGE_CONTROLLER
        ,
        m_peripheralEndpointRangeHandler
#endif
#ifdef MODE_CONTROLLER
        ,
        m_peripheralEndpointModeHandler
#endif
        ,
        nullptr,
        diagnostics);
    // clang-format on

    m_shutdownRequiredList.push_back(m_interactionManager);
    client->addAlexaDialogStateObserver(m_interactionManager);
    client->addCallStateObserver(m_interactionManager);

#ifdef ENABLE_REVOKE_AUTH
    // Creating the revoke authorization observer.
    auto revokeObserver =
        std::make_shared<alexaClientSDK::sampleApp::RevokeAuthorizationObserver>(client->getRegistrationManager());
    client->addRevokeAuthorizationObserver(revokeObserver);
#endif

    // Creating the input observer.
    m_userInputManager = alexaClientSDK::sampleApp::UserInputManager::create(
        m_interactionManager, consoleReader, localeAssetsManager, deviceInfo->getDefaultEndpointId());
    if (!m_userInputManager) {
        ACSDK_CRITICAL(LX("Failed to create UserInputManager!"));
        return false;
    }

    authDelegate->addAuthObserver(m_userInputManager);
    client->addRegistrationObserver(m_userInputManager);
    m_capabilitiesDelegate->addCapabilitiesObserver(m_userInputManager);

#ifdef AUTH_MANAGER
    m_authManager->setRegistrationManager(client->getRegistrationManager());

    auto cryptoFactory = manufactory->get<std::shared_ptr<acsdkCryptoInterfaces::CryptoFactoryInterface>>();
    auto keyStore = manufactory->get<std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>>();
    auto httpPost = avsCommon::utils::libcurlUtils::HttpPost::createHttpPostInterface();
    m_lwaAdapter = acsdkAuthorization::lwa::LWAAuthorizationAdapter::create(
        configPtr,
        std::move(httpPost),
        deviceInfo,
        acsdkAuthorization::lwa::LWAAuthorizationStorage::createLWAAuthorizationStorageInterface(
            configPtr, "", cryptoFactory, keyStore));

    if (!m_lwaAdapter) {
        ACSDK_CRITICAL(LX("Failed to create LWA Adapter!"));
        return false;
    }

    m_authManager->add(m_lwaAdapter);

    auto cblRequest = acsdkSampleApplicationCBLAuthRequester::SampleApplicationCBLAuthRequester::
        createCBLAuthorizationObserverInterface(userInterfaceManager);

    m_lwaAdapter->authorizeUsingCBL(cblRequest);
#endif

    client->connect();

    return true;
}

std::shared_ptr<ApplicationMediaInterfaces> PreviewAlexaClient::createApplicationMediaPlayer(
    const std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>&
        httpContentFetcherFactory,
    bool enableEqualizer,
    const std::string& name,
    bool enableLiveMode) {
#ifdef GSTREAMER_MEDIA_PLAYER
    /*
     * For the SDK, the MediaPlayer happens to also provide volume control functionality.
     * Note the externalMusicProviderMediaPlayer is not added to the set of SpeakerInterfaces as there would be
     * more actions needed for these beyond setting the volume control on the MediaPlayer.
     */
    auto mediaPlayer = alexaClientSDK::mediaPlayer::MediaPlayer::create(
        httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!mediaPlayer) {
        return nullptr;
    }
    auto speaker = std::static_pointer_cast<alexaClientSDK::avsCommon::sdkInterfaces::SpeakerInterface>(mediaPlayer);
    auto equalizer =
        std::static_pointer_cast<alexaClientSDK::acsdkEqualizerInterfaces::EqualizerInterface>(mediaPlayer);
    auto requiresShutdown = std::static_pointer_cast<alexaClientSDK::avsCommon::utils::RequiresShutdown>(mediaPlayer);
    auto applicationMediaInterfaces =
        std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, equalizer, requiresShutdown);
#elif defined(ANDROID_MEDIA_PLAYER)
    // TODO - Add support of live mode to AndroidSLESMediaPlayer (ACSDK-2530).
    std::shared_ptr<mediaPlayer::android::AndroidSLESMediaPlayer> mediaPlayer =
        mediaPlayer::android::AndroidSLESMediaPlayer::create(
            httpContentFetcherFactory,
            m_openSlEngine,
            enableEqualizer,
            mediaPlayer::android::PlaybackConfiguration(),
            name);
    if (!mediaPlayer) {
        return nullptr;
    }
    auto speaker = mediaPlayer->getSpeaker();
    auto equalizer =
        std::static_pointer_cast<alexaClientSDK::acsdkEqualizerInterfaces::EqualizerInterface>(mediaPlayer);
    auto requiresShutdown = std::static_pointer_cast<alexaClientSDK::avsCommon::utils::RequiresShutdown>(mediaPlayer);
    auto applicationMediaInterfaces =
        std::make_shared<ApplicationMediaInterfaces>(mediaPlayer, speaker, equalizer, requiresShutdown);
#elif defined(CUSTOM_MEDIA_PLAYER)
    // Custom media players must implement the createCustomMediaPlayer function
    auto applicationMediaInterfaces =
        createCustomMediaPlayer(httpContentFetcherFactory, enableEqualizer, name, enableLiveMode);
    if (!applicationMediaInterfaces) {
        return nullptr;
    }
#endif

    if (applicationMediaInterfaces->requiresShutdown) {
        m_shutdownRequiredList.push_back(applicationMediaInterfaces->requiresShutdown);
    }
    return applicationMediaInterfaces;
}

#ifdef ENABLE_ENDPOINT_CONTROLLERS
bool PreviewAlexaClient::addControllersToDefaultEndpoint(
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> defaultEndpointBuilder) {
    if (!defaultEndpointBuilder) {
        ACSDK_CRITICAL(LX("addControllersToDefaultEndpointFailed").m("invalidDefaultEndpointBuilder"));
        return false;
    }

#ifdef TOGGLE_CONTROLLER
    auto toggleHandler =
        DefaultEndpointToggleControllerHandler::create(DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME);
    if (!toggleHandler) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint toggle controller handler!"));
        return false;
    }

    auto toggleControllerAttributes = buildToggleControllerAttributes(
        {{{FriendlyName::Type::TEXT, DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME}}});
    if (!toggleControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint toggle controller attributes!"));
        return false;
    }

    defaultEndpointBuilder->withToggleController(
        toggleHandler,
        DEFAULT_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME,
        toggleControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef RANGE_CONTROLLER
    auto rangeHandler = DefaultEndpointRangeControllerHandler::create(DEFAULT_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME);
    if (!rangeHandler) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint range controller handler!"));
        return false;
    }

    auto rangeControllerAttributes = buildRangeControllerAttributes(
        {{{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_SETTING_FANSPEED}}},
        {{DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MAXIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_HIGH}}},
         {DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MEDIUM}}},
         {DEFAULT_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MINIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_LOW}}}});

    if (!rangeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint range controller attributes!"));
        return false;
    }

    defaultEndpointBuilder->withRangeController(
        rangeHandler,
        DEFAULT_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME,
        rangeControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef MODE_CONTROLLER
    auto modeHandler = DefaultEndpointModeControllerHandler::create(DEFAULT_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME);
    if (!modeHandler) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint mode controller handler!"));
        return false;
    }

    auto modeControllerAttributes = buildModeControllerAttributes(
        {{{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_SETTING_MODE}}},
        {{DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_FAN_ONLY,
          {{FriendlyName::Type::TEXT,
            DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_FAN_ONLY_FRIENDLY_NAME}}},
         {DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_HEAT,
          {{FriendlyName::Type::TEXT, DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_HEAT_FRIENDLY_NAME}}},
         {DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_COOL,
          {{FriendlyName::Type::TEXT,
            DefaultEndpointModeControllerHandler::MODE_CONTROLLER_MODE_COOL_FRIENDLY_NAME}}}});

    if (!modeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint mode controller attributes!"));
        return false;
    }

    defaultEndpointBuilder->withModeController(
        modeHandler,
        DEFAULT_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME,
        modeControllerAttributes.value(),
        true,
        true,
        false);
#endif

    return true;
}

bool PreviewAlexaClient::addControllersToPeripheralEndpoint(
    std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface> peripheralEndpointBuilder) {
    if (!peripheralEndpointBuilder) {
        ACSDK_CRITICAL(LX("addControllersToPeripheralEndpointFailed").m("invalidPeripheralEndpointBuilder"));
        return false;
    }

#ifdef POWER_CONTROLLER
    m_peripheralEndpointPowerHandler =
        PeripheralEndpointPowerControllerHandler::create(PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID);
    if (!m_peripheralEndpointPowerHandler) {
        ACSDK_CRITICAL(LX("Failed to create power controller handler!"));
        return false;
    }
    peripheralEndpointBuilder->withPowerController(m_peripheralEndpointPowerHandler, true, true);
#endif

#ifdef TOGGLE_CONTROLLER
    m_peripheralEndpointToggleHandler = PeripheralEndpointToggleControllerHandler::create(
        PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID, PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME);
    if (!m_peripheralEndpointToggleHandler) {
        ACSDK_CRITICAL(LX("Failed to create toggle controller handler!"));
        return false;
    }

    auto peripheralEndpointToggleControllerAttributes = buildToggleControllerAttributes(
        {{{FriendlyName::Type::TEXT, PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_FRIENDLY_NAME}}});
    if (!peripheralEndpointToggleControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint toggle controller attributes!"));
        return false;
    }

    peripheralEndpointBuilder->withToggleController(
        m_peripheralEndpointToggleHandler,
        PERIPHERAL_ENDPOINT_TOGGLE_CONTROLLER_INSTANCE_NAME,
        peripheralEndpointToggleControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef RANGE_CONTROLLER
    m_peripheralEndpointRangeHandler = PeripheralEndpointRangeControllerHandler::create(
        PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID, PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME);
    if (!m_peripheralEndpointRangeHandler) {
        ACSDK_CRITICAL(LX("Failed to create range controller handler!"));
        return false;
    }

    // Enables "raise" and "lower" utterances for the peripheral endpoint by mapping actions to directives.
    auto raiseActionMapping = capabilitySemantics::ActionsToDirectiveMapping();
    raiseActionMapping.addAction(SEMANTICS_ACTION_ID_RAISE);
    raiseActionMapping.setDirective(SETRANGE_DIRECTIVE_NAME, PERIPHERAL_ENDPOINT_RAISE_PAYLOAD);

    auto lowerActionMapping = capabilitySemantics::ActionsToDirectiveMapping();
    lowerActionMapping.addAction(SEMANTICS_ACTION_ID_LOWER);
    lowerActionMapping.setDirective(SETRANGE_DIRECTIVE_NAME, PERIPHERAL_ENDPOINT_LOWER_PAYLOAD);

    auto peripheralEndpointRangeSemantics = capabilitySemantics::CapabilitySemantics();
    peripheralEndpointRangeSemantics.addActionsToDirectiveMapping(raiseActionMapping);
    peripheralEndpointRangeSemantics.addActionsToDirectiveMapping(lowerActionMapping);
    if (!peripheralEndpointRangeSemantics.isValid()) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint semantic annotations!"));
        return false;
    }

    auto peripheralEndpointRangeControllerAttributes = buildRangeControllerAttributes(
        {{{FriendlyName::Type::TEXT, PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_FRIENDLY_NAME}}},
        {{PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_HIGH,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MAXIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_HIGH}}},
         {PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_MEDIUM,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MEDIUM}}},
         {PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_PRESET_LOW,
          {{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_MINIMUM},
           {FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_VALUE_LOW}}}},
        utils::Optional<capabilitySemantics::CapabilitySemantics>(peripheralEndpointRangeSemantics));

    if (!peripheralEndpointRangeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create peripheral endpoint range controller attributes!"));
        return false;
    }

    peripheralEndpointBuilder->withRangeController(
        m_peripheralEndpointRangeHandler,
        PERIPHERAL_ENDPOINT_RANGE_CONTROLLER_INSTANCE_NAME,
        peripheralEndpointRangeControllerAttributes.value(),
        true,
        true,
        false);
#endif

#ifdef MODE_CONTROLLER
    m_peripheralEndpointModeHandler = PeripheralEndpointModeControllerHandler::create(
        PERIPHERAL_ENDPOINT_DERIVED_ENDPOINT_ID, PERIPHERAL_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME);
    if (!m_peripheralEndpointModeHandler) {
        ACSDK_CRITICAL(LX("Failed to create mode controller handler!"));
        return false;
    }

    auto peripheralEndpointModeControllerAttributes = buildModeControllerAttributes(
        {{{FriendlyName::Type::ASSET, avsCommon::avs::resources::ASSET_ALEXA_SETTING_MODE},
          {FriendlyName::Type::TEXT, PERIPHERAL_ENDPOINT_MODE_CONTROLLER_FRIENDLY_NAME}}},
        {{PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_RED,
          {{FriendlyName::Type::TEXT, PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_RED}}},
         {PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_GREEN,
          {{FriendlyName::Type::TEXT, PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_GREEN}}},
         {PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_BLUE,
          {{FriendlyName::Type::TEXT, PeripheralEndpointModeControllerHandler::MODE_CONTROLLER_MODE_BLUE}}}});

    if (!peripheralEndpointModeControllerAttributes.hasValue()) {
        ACSDK_CRITICAL(LX("Failed to create default endpoint mode controller attributes!"));
        return false;
    }

    peripheralEndpointBuilder->withModeController(
        m_peripheralEndpointModeHandler,
        PERIPHERAL_ENDPOINT_MODE_CONTROLLER_INSTANCE_NAME,
        peripheralEndpointModeControllerAttributes.value(),
        true,
        true,
        false);
#endif

    return true;
}
#endif

#ifdef DIAGNOSTICS
bool PreviewAlexaClient::initiateRestart() {
    m_userInputManager->onLogout();
    return true;
}
#endif

}  // namespace acsdkPreviewAlexaClient
}  // namespace alexaClientSDK
