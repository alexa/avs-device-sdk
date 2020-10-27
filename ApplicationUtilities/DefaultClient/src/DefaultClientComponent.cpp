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
#include <acsdkManufactory/ComponentAccumulator.h>
#include <acsdkShared/SharedComponent.h>
#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>

#include <acsdkAudioPlayer/AudioPlayerComponent.h>
#include <acsdkExternalMediaPlayer/ExternalMediaPlayerComponent.h>
#include <acsdkManufactory/ConstructorAdapter.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AFML/FocusManagementComponent.h>
#include <AVSCommon/AVS/Attachment/AttachmentManager.h>
#include <AVSCommon/AVS/ExceptionEncounteredSender.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSGatewayManager/AVSGatewayManager.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorage.h>
#include <Alexa/AlexaEventProcessedNotifier.h>
#include <Alexa/AlexaInterfaceCapabilityAgent.h>
#include <CapabilitiesDelegate/CapabilitiesDelegate.h>
#include <CapabilitiesDelegate/Storage/SQLiteCapabilitiesDelegateStorage.h>
#include <Captions/CaptionsComponent.h>
#include <ContextManager/ContextManager.h>
#include <Endpoints/DefaultEndpointBuilder.h>
#include <PlaybackController/PlaybackControllerComponent.h>
#include <SpeakerManager/SpeakerManagerComponent.h>
#include <SynchronizeStateSender/SynchronizeStateSenderFactory.h>
#include <TemplateRuntime/RenderPlayerInfoCardsProviderRegistrar.h>

#include "DefaultClient/DefaultClientComponent.h"
#include "DefaultClient/StubApplicationAudioPipelineFactory.h"

namespace alexaClientSDK {
namespace defaultClient {

using namespace ::alexaClientSDK::storage;
using namespace ::alexaClientSDK::storage::sqliteStorage;
using namespace acl;
using namespace acsdkAlexaEventProcessedNotifierInterfaces;
using namespace acsdkApplicationAudioPipelineFactoryInterfaces;
using namespace acsdkManufactory;
using namespace acsdkShutdownManagerInterfaces;
using namespace avsCommon::avs::attachment;
using namespace avsCommon::utils;
using namespace avsCommon::utils::libcurlUtils;
using namespace avsGatewayManager;
using namespace avsGatewayManager::storage;
using namespace capabilityAgents::alexa;

/// String to identify log entries originating from this file.
static const std::string TAG("DefaultClientComponent");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/**
 * Factory method for @c AlexaEventProcessedNotifierInterface.
 *
 * @param capabilitiesDelegate The CapabilitiesDelegate that must be added as an observer.
 * @return An implementation of @c AlexaEventProcessedNotifierInterface.
 */
static std::shared_ptr<AlexaEventProcessedNotifierInterface> createAlexaEventProcessedNotifierInterface(
    const std::shared_ptr<CapabilitiesDelegateInterface>& capabilitiesDelegate) {
    auto notifier = std::make_shared<AlexaEventProcessedNotifier>();
    notifier->addObserver(capabilitiesDelegate);
    return notifier;
}

/**
 * Adapt from a MessageRouter factory object to a std::function to create a MessageRouter.
 *
 * @param messageRouterFactory The factory object to use.
 * @return A std::function for creating MessageRouter instances.
 */
static std::function<std::shared_ptr<MessageRouterInterface>(
    std::shared_ptr<ShutdownNotifierInterface> shutdownNotifier,
    std::shared_ptr<AuthDelegateInterface> authDelegate,
    std::shared_ptr<AttachmentManagerInterface> attachmentManager,
    std::shared_ptr<TransportFactoryInterface> transportFactory)>
getCreateMessageRouter(const std::shared_ptr<MessageRouterFactoryInterface>& messageRouterFactory) {
    return [messageRouterFactory](
               std::shared_ptr<ShutdownNotifierInterface> shutdownNotifier,
               std::shared_ptr<AuthDelegateInterface> authDelegate,
               std::shared_ptr<AttachmentManagerInterface> attachmentManager,
               std::shared_ptr<TransportFactoryInterface> transportFactory) {
        std::shared_ptr<MessageRouterInterface> result;
        if (!shutdownNotifier) {
            ACSDK_ERROR(LX("createMessageRouterFailed").d("reason", "nullShutdownNotifier"));
        } else if (!messageRouterFactory) {
            ACSDK_ERROR(LX("createMessageRouterFailed").d("reason", "nullMessageRouterFactory"));
        } else {
            result = messageRouterFactory->createMessageRouter(authDelegate, attachmentManager, transportFactory);
            if (!result) {
                ACSDK_ERROR(LX("createMessageRouterFailed").d("reason", "createMessageRouterFailed"));
            } else {
                shutdownNotifier->addObserver(result);
            }
        }
        return result;
    };
}

/**
 * Returns an std::function that finishes configuring the @c StubAudioPipelineFactory with objects created by the
 * manufactory and forwards the stub factory as the @c ApplicationAudioPipelineFactoryInterface.
 * @param stubAudioPipelineFactory The stub factory to finish configuring.
 * @return A shared pointer to an @c ApplicationAudioPipelineFactoryInterface.
 */
static std::function<std::shared_ptr<ApplicationAudioPipelineFactoryInterface>(
    std::shared_ptr<SpeakerManagerInterface>,
    std::shared_ptr<captions::CaptionManagerInterface>)>
getCreateApplicationAudioPipelineFactory(
    std::shared_ptr<StubApplicationAudioPipelineFactory> stubAudioPipelineFactory) {
    return [stubAudioPipelineFactory](
               std::shared_ptr<SpeakerManagerInterface> speakerManager,
               std::shared_ptr<captions::CaptionManagerInterface> captionManager)
               -> std::shared_ptr<ApplicationAudioPipelineFactoryInterface> {
        if (!stubAudioPipelineFactory || !speakerManager) {
            ACSDK_ERROR(LX("getCreateApplicationAudioPipelineFactoryFailed")
                            .d("isStubAudioPipelineFactoryNull", !stubAudioPipelineFactory)
                            .d("isSpeakerManagerNull", !speakerManager));
            return nullptr;
        }

        stubAudioPipelineFactory->addSpeakerManager(speakerManager);
        stubAudioPipelineFactory->addCaptionManager(captionManager);

        return stubAudioPipelineFactory;
    };
}

DefaultClientComponent getComponent(
    const std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>& authDelegate,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
    const std::shared_ptr<avsCommon::utils::DeviceInfo>& deviceInfo,
    const std::shared_ptr<registrationManager::CustomerDataManager>& customerDataManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& miscStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>& internetConnectionMonitor,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>& avsGatewayManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>& capabilitiesDelegate,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<avsCommon::sdkInterfaces::diagnostics::DiagnosticsInterface>& diagnostics,
    const std::shared_ptr<alexaClientSDK::acl::TransportFactoryInterface>& transportFactory,
    const std::shared_ptr<alexaClientSDK::acl::MessageRouterFactoryInterface>& messageRouterFactory,
    const std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface>& channelVolumeFactory,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExpectSpeechTimeoutHandlerInterface>& expectSpeechTimeoutHandler,
    const std::shared_ptr<acsdkEqualizerInterfaces::EqualizerRuntimeSetupInterface>& equalizerRuntimeSetup,
    const std::shared_ptr<StubApplicationAudioPipelineFactory>& stubAudioPipelineFactory,
    const std::shared_ptr<avsCommon::utils::mediaPlayer::PooledMediaResourceProviderInterface>&
        audioMediaResourceProvider,
    const std::shared_ptr<certifiedSender::MessageStorageInterface>& messageStorage,
    const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>& powerResourceManager,
    const acsdkExternalMediaPlayer::ExternalMediaPlayer::AdapterCreationMap& adapterCreationMap) {
    return ComponentAccumulator<>()
        /// Instead of using factory methods to instantiate these interfaces, DefaultClientComponent accepts pre-made
        /// implementations and adds them to the manufactory.
        .addInstance(authDelegate)
        .addInstance(contextManager)
        .addInstance(localeAssetsManager)
        .addInstance(deviceInfo)
        .addInstance(customerDataManager)
        .addInstance(miscStorage)
        .addInstance(internetConnectionMonitor)
        .addInstance(avsGatewayManager)
        .addInstance(capabilitiesDelegate)
        .addInstance(metricRecorder)
        .addInstance(diagnostics)
        .addInstance(transportFactory)
        .addInstance(channelVolumeFactory)
        .addInstance(expectSpeechTimeoutHandler)
        .addInstance(equalizerRuntimeSetup)
        .addInstance(audioMediaResourceProvider)
        .addInstance(messageStorage)
        .addInstance(powerResourceManager)
        .addRetainedFactory(getCreateApplicationAudioPipelineFactory(stubAudioPipelineFactory))

        /// Baseline SDK components. Applications are not expected to modify these.
        .addComponent(acsdkShared::getComponent())
        .addRetainedFactory(afml::interruptModel::InterruptModel::createInterruptModel)
        .addComponent(afml::getComponent())
        .addRetainedFactory(AlexaInterfaceMessageSender::createAlexaInterfaceMessageSender)
        .addRetainedFactory(AlexaInterfaceMessageSender::createAlexaInterfaceMessageSenderInternalInterface)
        .addRequiredFactory(AlexaInterfaceCapabilityAgent::createDefaultAlexaInterfaceCapabilityAgent)
        .addRetainedFactory(
            alexaClientSDK::endpoints::DefaultEndpointBuilder::createDefaultEndpointCapabilitiesRegistrarInterface)
        .addRetainedFactory(alexaClientSDK::endpoints::DefaultEndpointBuilder::createDefaultEndpointBuilderInterface)
        .addRetainedFactory(avsCommon::avs::ExceptionEncounteredSender::createExceptionEncounteredSenderInterface)
        .addRetainedFactory(AVSConnectionManager::createAVSConnectionManagerInterface)
        .addRetainedFactory(AVSConnectionManager::createMessageSenderInterface)
        .addRetainedFactory(AttachmentManager::createInProcessAttachmentManagerInterface)
        .addRetainedFactory(certifiedSender::CertifiedSender::create)
        .addRetainedFactory(createAlexaEventProcessedNotifierInterface)
        .addRetainedFactory(HTTPContentFetcherFactory::createHTTPContentFetcherInterfaceFactoryInterface)
        .addRetainedFactory(getCreateMessageRouter(messageRouterFactory))
        .addUniqueFactory(capabilitiesDelegate::storage::SQLiteCapabilitiesDelegateStorage::
                              createCapabilitiesDelegateStorageInterface)

        /// Optional, horizontal components.
        .addComponent(captions::getComponent())

        /// Capability Agents. Most CAs are still instantiated in DefaultClient.cpp.
        .addComponent(acsdkAudioPlayer::getBackwardsCompatibleComponent())
        .addComponent(acsdkExternalMediaPlayer::getBackwardsCompatibleComponent(adapterCreationMap))
        .addComponent(capabilityAgents::playbackController::getComponent())
        .addComponent(capabilityAgents::speakerManager::getComponent())
        .addRetainedFactory(capabilityAgents::templateRuntime::RenderPlayerInfoCardsProviderRegistrar::
                                createRenderPlayerInfoCardsProviderRegistrarInterface);
}

}  // namespace defaultClient
}  // namespace alexaClientSDK
