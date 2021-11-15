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

#include <acsdkAlerts/AlertsComponent.h>
#include <acsdkAlerts/Storage/SQLiteAlertStorage.h>
#include <acsdkAlexaCommunications/AlexaCommunicationsComponent.h>
#include <acsdkApplicationAudioPipelineFactory/ApplicationAudioPipelineFactoryComponent.h>
#include <acsdkAudioInputStream/AudioInputStreamComponent.h>
#include <acsdkAudioPlayer/AudioPlayerComponent.h>
#include <acsdkAuthorizationDelegate/AuthorizationDelegateComponent.h>
#include <acsdkBluetooth/BasicDeviceConnectionRulesProvider.h>
#include <acsdkBluetooth/BluetoothComponent.h>
#include <acsdkBluetooth/SQLiteBluetoothStorage.h>
#include <acsdkBluetoothImplementation/BluetoothImplementationComponent.h>
#include <acsdkCore/CoreComponent.h>
#include <acsdkCrypto/CryptoFactory.h>
#include <acsdkPkcs11/KeyStoreFactory.h>
#include <acsdkDeviceSettingsManager/DeviceSettingsManagerComponent.h>
#include <acsdkDeviceSetup/DeviceSetupComponent.h>
#include <acsdkDoNotDisturb/DoNotDisturbComponent.h>
#include <acsdkEqualizerImplementations/EqualizerComponent.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayerComponent.h>
#include <acsdkHTTPContentFetcher/HTTPContentFetcherComponent.h>
#include <acsdkInteractionModel/InteractionModelComponent.h>
#include <acsdkInternetConnectionMonitor/InternetConnectionMonitorComponent.h>
#include <acsdkKWD/KWDComponent.h>
#include <acsdkManufactory/ComponentAccumulator.h>
#ifdef ENABLE_MC
#include <acsdkMessagingController/MessagingControllerComponent.h>
#include <acsdkMessenger/MessengerComponent.h>
#endif
#include <acsdkMetricRecorder/MetricRecorderComponent.h>
#include <acsdkNotifications/NotificationsComponent.h>
#include <acsdkNotifications/SQLiteNotificationsStorage.h>
#include <acsdkSampleApplicationCBLAuthRequester/SampleApplicationCBLAuthRequester.h>
#include <acsdkShared/SharedComponent.h>
#include <acsdkSpeechEncoder/SpeechEncoderComponent.h>
#include <acsdkSystemTimeZone/SystemTimeZoneComponent.h>
#include <Audio/AudioFactory.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/Utils/LibcurlUtils/DefaultSetCurlOptionsCallbackFactory.h>
#include <Captions/CaptionsComponent.h>
#include <DefaultClient/EqualizerRuntimeSetup.h>
#include <PlaybackController/PlaybackControllerComponent.h>
#include <SampleApp/CaptionPresenter.h>
#include <SampleApp/LocaleAssetsManager.h>
#include <SampleApp/SampleEqualizerModeController.h>
#include <SampleApp/UIManager.h>
#include <Settings/Storage/SQLiteDeviceSettingStorage.h>
#include <SpeakerManager/DefaultChannelVolumeFactory.h>
#include <SpeakerManager/SpeakerManagerComponent.h>
#include <System/SystemComponent.h>
#include <SystemSoundPlayer/SystemSoundPlayer.h>
#include <TemplateRuntime/RenderPlayerInfoCardsProviderRegistrar.h>

#ifdef EXTERNAL_MEDIA_ADAPTERS
#include <acsdkExternalMediaPlayerAdapters/ExternalMediaPlayerAdaptersComponent.h>
#endif

#include "acsdkPreviewAlexaClient/PreviewAlexaClient.h"
#include "acsdkPreviewAlexaClient/PreviewAlexaClientComponent.h"

namespace alexaClientSDK {
namespace acsdkPreviewAlexaClient {

using namespace acl;
using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace acsdkManufactory;
using namespace acsdkSampleApplicationCBLAuthRequester;
using namespace acsdkSampleApplicationInterfaces;
using namespace applicationUtilities::systemSoundPlayer;
using namespace avsCommon::avs;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::avs::initialization;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::libcurlUtils;

using DefaultEndpointAnnotation = avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation;

/**
 * @c UIManagerInterface factory that just forwards the instance of UIManager.
 *
 * @param uiManager The @c UIManager instance that will provide the implementation.
 * @return The implementation of @c CBLAuthRequesterInterface to use.
 */
static std::shared_ptr<UIManagerInterface> createUIManagerInterface(
    const std::shared_ptr<sampleApp::UIManager>& uiManager) {
    return uiManager;
}

/**
 * Function to create a std::function that creates instances of @c EventTrackerInterface.
 * @param diagnostics An instance of DiagnosticsInterface.  If non-null, it is used to get an instance
 * of EventTracerInterface.
 * @return A new instance of @c EventTrackerInterface.
 */
static std::function<std::shared_ptr<EventTracerInterface>()> getCreateEventTracker(
    const std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface>& diagnostics) {
    return [diagnostics]() {
        if (diagnostics) {
            auto tracer = diagnostics->getProtocolTracer();
            if (tracer) {
                return std::shared_ptr<EventTracerInterface>(tracer);
            }
        }
        return std::shared_ptr<EventTracerInterface>(nullptr);
    };
}

/**
 * Function to create a LibcurlSetCurlOptionsCallbackFactoryInterface that is Annotated<> to target the
 * instance for consumption by AVSConnectionManager.
 *
 * @return Pointer to LibcurlSetCurlOptionsCallbackFactoryInterface Annotated<> for consumption by
 * AVSCnnectonManager.
 */
static Annotated<AVSConnectionManagerInterface, LibcurlSetCurlOptionsCallbackFactoryInterface>
createSetCurlOptionsCallbackForAVSConnectionManager() {
    return DefaultSetCurlOptionsCallbackFactory::createSetCurlOptionsCallbackFactoryInterface();
}

/**
 * Function to create a LibcurlSetCurlOptionsCallbackFactoryInterface that is Annotated<> to target the
 * instance for consumption by HTTPContentFetcherInterfaceFactory.
 *
 * @return Pointer to LibcurlSetCurlOptionsCallbackFactoryInterface Annotated<> for consumption by
 * HTTPContentFetcherInterfaceFactory.
 */
static Annotated<HTTPContentFetcherInterfaceFactoryInterface, LibcurlSetCurlOptionsCallbackFactoryInterface>
createSetCurlOptionsCallbackForHTTPContentFetcherInterfaceFactory() {
    return DefaultSetCurlOptionsCallbackFactory::createSetCurlOptionsCallbackFactoryInterface();
}

PreviewAlexaClientComponent getComponent(
    std::unique_ptr<avsCommon::avs::initialization::InitializationParameters> initParams,
    const std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface>& diagnostics,
    const std::shared_ptr<sampleApp::PlatformSpecificValues>& platformSpecificValues,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>& expectSpeechTimeoutHandler,
    const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>& powerResourceManager) {
    /// This component is provided as a preview of changes to come. The imports, exports, and factory methods
    /// will change while manufactory integration is incrementally released over the next several releases.

    return ComponentAccumulator<>()
        /// Initialize the SDK with @c InitalizationParameters. For example, low-power mode is initialized
        /// using @c InitializationParamters.
        .addPrimaryFactory(AlexaClientSDKInit::getCreateAlexaClientSDKInit(std::move(initParams)))

        /// This allows applications to provide platform-specific objects to factory methods. For example,
        /// AndroidApplicationAudioPipelineFactory requires the AndroidSLESEngine.
        .addInstance(platformSpecificValues)

        /// Implementations created at the application level, added to the manufactory to satisfy dependencies.
        .addInstance(expectSpeechTimeoutHandler)
        .addInstance(powerResourceManager)

        /// Baseline SDK components. Applications are not expected to modify these.
        .addComponent(acsdkCore::getComponent())
        .addComponent(acsdkDeviceSettingsManager::getComponent())
        .addComponent(acsdkInternetConnectionMonitor::getComponent())
        .addComponent(acsdkHTTPContentFetcher::getComponent())
        .addComponent(acsdkShared::getComponent())
        .addRetainedFactory(certifiedSender::CertifiedSender::create)
        .addRetainedFactory(createSetCurlOptionsCallbackForAVSConnectionManager)
        .addRetainedFactory(createSetCurlOptionsCallbackForHTTPContentFetcherInterfaceFactory)
        .addRetainedFactory(DialogUXStateAggregator::createDialogUXStateAggregator)
        .addRetainedFactory(SystemSoundPlayer::createSystemSoundPlayerInterface)

        /**
         * Although these are the default options for PreviewAlexaClient, applications may modify or replace
         * these with custom implementations. These include components like ACL, the logger, and
         * AuthDelegateInterface, among others.
         *
         * For example, to replace the default null MetricRecorder with your own implementation, you could remove
         * the default applications/acsdkNullMetricRecorder library and instead define your own metric recorder
         * component in the same acsdkMetricRecorder namespace.
         */
        .addComponent(acsdkAlexaCommunications::getComponent())
        .addComponent(acsdkApplicationAudioPipelineFactory::getComponent())
        .addComponent(acsdkAudioInputStream::getComponent())
        .addComponent(acsdkAuthorizationDelegate::getComponent())
        .addComponent(acsdkBluetoothImplementation::getComponent())
        .addComponent(acsdkMetricRecorder::getComponent())
        .addComponent(acsdkSpeechEncoder::getComponent())
        .addComponent(acsdkSystemTimeZone::getComponent())
#ifdef ANDROID_LOGGER
        .addPrimaryFactory(applicationUtilities::androidUtilities::AndroidLogger::getAndroidLogger)
#else
        .addPrimaryFactory(avsCommon::utils::logger::getConsoleLogger)
#endif

        /**
         * Sample implementations used to satisfy dependencies for Capability Agents and other components.
         * Applications may choose to provide their own custom implementations.
         *
         * For example, to use a custom CaptionPresenterInterface, you can replace this line:
         *
         *     .addRequiredFactory(sampleApp::CaptionPresenter::createCaptionPresenterInterface)
         *
         * with your own factory method:
         *
         *     .addRequiredFactory(myCustomApp::CustomCaptionPresenter::createCaptionPresenterInterface)
         */
        .addRetainedFactory(applicationUtilities::resources::audio::AudioFactory::createAudioFactoryInterface)
        .addRetainedFactory(
            acsdkBluetooth::BasicDeviceConnectionRulesProvider::createBluetoothDeviceConnectionRulesProviderInterface)
        .addRetainedFactory(
            capabilityAgents::speakerManager::DefaultChannelVolumeFactory::createChannelVolumeFactoryInterface)
        .addRetainedFactory(createUIManagerInterface)
        .addRetainedFactory(defaultClient::EqualizerRuntimeSetup::createEqualizerRuntimeSetupInterface)
        .addRequiredFactory(sampleApp::CaptionPresenter::createCaptionPresenterInterface)
        .addRetainedFactory(sampleApp::LocaleAssetsManager::createLocaleAssetsManagerInterface)
        .addRetainedFactory(sampleApp::SampleEqualizerModeController::createEqualizerModeControllerInterface)
        .addRetainedFactory(sampleApp::UIManager::create)
        .addUnloadableFactory(SampleApplicationCBLAuthRequester::createCBLAuthRequesterInterface)

        /// SQLite implementations of databases used by Capability Agents and other components.
        /// Applications may choose to replace these with their own database implementations.
        .addRetainedFactory(acsdkAlerts::storage::SQLiteAlertStorage::createAlertStorageInterface)
        .addRetainedFactory(acsdkBluetooth::SQLiteBluetoothStorage::createBluetoothStorageInterface)
        .addRetainedFactory(acsdkNotifications::SQLiteNotificationsStorage::createNotificationsStorageInterface)
        .addRetainedFactory(certifiedSender::SQLiteMessageStorage::createMessageStorageInterface)
        .addRetainedFactory(settings::storage::SQLiteDeviceSettingStorage::createDeviceSettingStorageInterface)

        /// Optional, horizontal components. These may be enabled via cmake or AlexaClientSDKConfig.json.
        /// Applications are not expected to modify these.
        .addComponent(captions::getComponent())
        .addRetainedFactory(getCreateEventTracker(diagnostics))
#ifdef EXTERNAL_MEDIA_ADAPTERS
        .addComponent(acsdkExternalMediaPlayerAdapters::getComponent())
#endif

        /// KWD Component. Default component is the null component.
        .addComponent(acsdkKWD::getComponent())

        /// Capability Agents. Some CAs are still created in Default Client.
        .addComponent(acsdkAlerts::getComponent())
        .addComponent(acsdkAudioPlayer::getComponent())
        .addComponent(acsdkBluetooth::getComponent())
        .addComponent(acsdkDoNotDisturb::getComponent())
        .addComponent(acsdkEqualizer::getComponent())
        .addComponent(acsdkExternalMediaPlayer::getComponent())
        .addComponent(acsdkInteractionModel::getComponent())
#ifdef ENABLE_MC
        .addComponent(acsdkMessenger::getComponent())
        .addComponent(acsdkMessagingController::getComponent())
#endif
        .addComponent(acsdkNotifications::getComponent())
        .addComponent(capabilityAgents::playbackController::getComponent())
        .addComponent(capabilityAgents::speakerManager::getComponent())
        .addComponent(capabilityAgents::system::getComponent())
        .addRetainedFactory(capabilityAgents::templateRuntime::RenderPlayerInfoCardsProviderRegistrar::
                                createRenderPlayerInfoCardsProviderRegistrarInterface)
        .addComponent(acsdkDeviceSetup::getComponent())
#ifdef ENABLE_PKCS11
        .addRetainedFactory(acsdkPkcs11::createKeyStore)
#else
        .addInstance(std::shared_ptr<acsdkCryptoInterfaces::KeyStoreInterface>())
#endif
        .addRetainedFactory(acsdkCrypto::createCryptoFactory);
}

}  // namespace acsdkPreviewAlexaClient
}  // namespace alexaClientSDK
