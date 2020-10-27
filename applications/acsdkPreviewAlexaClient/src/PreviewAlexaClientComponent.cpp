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

#include <ACL/AVSConnectionManager.h>
#include <acsdkAlexaCommunications/AlexaCommunicationsComponent.h>
#include <acsdkAudioPlayer/AudioPlayerComponent.h>
#include <acsdkAuthorizationDelegate/AuthorizationDelegateComponent.h>
#include <acsdkCore/CoreComponent.h>
#include <acsdkEqualizerImplementations/EqualizerComponent.h>
#include <acsdkInternetConnectionMonitor/InternetConnectionMonitorComponent.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayerComponent.h>
#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkMetricRecorder/MetricRecorderComponent.h>
#include <acsdkSampleApplicationCBLAuthRequester/SampleApplicationCBLAuthRequester.h>
#include <acsdkShared/SharedComponent.h>
#include <DefaultClient/StubApplicationAudioPipelineFactory.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>
#include <Captions/CaptionsComponent.h>
#include <ContextManager/ContextManager.h>
#include <DefaultClient/EqualizerRuntimeSetup.h>
#include <PlaybackController/PlaybackControllerComponent.h>
#include <SampleApp/CaptionPresenter.h>
#include <SampleApp/LocaleAssetsManager.h>
#include <SampleApp/SampleEqualizerModeController.h>
#include <SampleApp/UIManager.h>
#include <SpeakerManager/DefaultChannelVolumeFactory.h>
#include <SpeakerManager/SpeakerManagerComponent.h>
#include <TemplateRuntime/RenderPlayerInfoCardsProviderRegistrar.h>

#ifdef GSTREAMER_MEDIA_PLAYER
#include <acsdkGstreamerApplicationAudioPipelineFactory/GstreamerApplicationAudioPipelineFactory.h>
#elif defined(ANDROID_MEDIA_PLAYER)
#include <acsdkAndroidApplicationAudioPipelineFactory/AndroidApplicationAudioPipelineFactory.h>
#elif defined(CUSTOM_MEDIA_PLAYER)
#include <acsdkCustomApplicationAudioPipelineFactory/CustomApplicationAudioPipelineFactory.h>
#endif

#ifdef EXTERNAL_MEDIA_ADAPTERS
#include <acsdkExternalMediaPlayerAdapters/ExternalMediaPlayerAdaptersComponent.h>
#endif

#include "acsdkPreviewAlexaClient/PreviewAlexaClient.h"
#include "acsdkPreviewAlexaClient/PreviewAlexaClientComponent.h"
#include "SampleApp/CaptionPresenter.h"
#include "SampleApp/LocaleAssetsManager.h"
#include "SampleApp/SampleEqualizerModeController.h"
#include "SampleApp/UIManager.h"

namespace alexaClientSDK {
namespace acsdkPreviewAlexaClient {

using namespace acl;
using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace acsdkManufactory;
using namespace acsdkSampleApplicationCBLAuthRequester;
using namespace acsdkSampleApplicationInterfaces;
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
        .addComponent(acsdkInternetConnectionMonitor::getComponent())
        .addComponent(acsdkShared::getComponent())
        .addRetainedFactory(certifiedSender::SQLiteMessageStorage::createMessageStorageInterface)
        .addRetainedFactory(certifiedSender::CertifiedSender::create)
        .addRetainedFactory(HTTPContentFetcherFactory::createHTTPContentFetcherInterfaceFactoryInterface)

        /**
         * Although these are the default options for PreviewAlexaClient, applications commonly modify or replace
         * these with custom implementations. These include components like ACL, the logger, and AuthDelegateInterface,
         * among others.
         * For example, to replace the default null MetricRecorder with your own implementation, you could remove the
         * default applications/acsdkNullMetricRecorder library and instead define your own metric recorder component
         * in the same acsdkMetricRecorder namespace.
         */
        .addComponent(acsdkAlexaCommunications::getComponent())
        .addComponent(acsdkAuthorizationDelegate::getComponent())
        .addComponent(acsdkMetricRecorder::getComponent())
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
        .addRetainedFactory(
            capabilityAgents::speakerManager::DefaultChannelVolumeFactory::createChannelVolumeFactoryInterface)
        .addRetainedFactory(createApplicationAudioPipelineFactoryInterface)
        .addRetainedFactory(createUIManagerInterface)
        .addRetainedFactory(defaultClient::EqualizerRuntimeSetup::createEqualizerRuntimeSetupInterface)
        .addRequiredFactory(sampleApp::CaptionPresenter::createCaptionPresenterInterface)
        .addRetainedFactory(sampleApp::LocaleAssetsManager::createLocaleAssetsManagerInterface)
        .addRetainedFactory(sampleApp::SampleEqualizerModeController::createEqualizerModeControllerInterface)
        .addRetainedFactory(sampleApp::UIManager::create)
        .addUnloadableFactory(SampleApplicationCBLAuthRequester::createCBLAuthRequesterInterface)

        /// Optional, horizontal components. These may be enabled via cmake or AlexaClientSDKConfig.json.
        /// Applications are not expected to modify these.
        .addComponent(captions::getComponent())
        .addRetainedFactory(getCreateEventTracker(diagnostics))
#ifdef EXTERNAL_MEDIA_ADAPTERS
        .addComponent(acsdkExternalMediaPlayerAdapters::getComponent())
#endif

        /// Capability Agents. Many CAs are still created in Default Client.
        .addComponent(acsdkAudioPlayer::getComponent())
        .addComponent(acsdkEqualizer::getComponent())
        .addComponent(acsdkExternalMediaPlayer::getComponent())
        .addComponent(capabilityAgents::playbackController::getComponent())
        .addComponent(capabilityAgents::speakerManager::getComponent())
        .addRetainedFactory(capabilityAgents::templateRuntime::RenderPlayerInfoCardsProviderRegistrar::
                                createRenderPlayerInfoCardsProviderRegistrarInterface);
}

}  // namespace acsdkPreviewAlexaClient
}  // namespace alexaClientSDK
