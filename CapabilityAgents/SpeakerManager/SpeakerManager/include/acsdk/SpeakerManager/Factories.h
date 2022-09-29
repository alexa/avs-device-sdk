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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_FACTORIES_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_FACTORIES_H_

#include <memory>

#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerConfigInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageInterface.h>
#include <acsdk/SpeakerManager/SpeakerManagerStorageState.h>
#include <AVSCommon/AVS/AVSDirective.h>
#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ChannelVolumeFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerObserverInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>

namespace alexaClientSDK {
namespace speakerManager {

/**
 * @brief Creates speaker manager CA.
 *
 * Method creates new speaker manager capability agent, adds supplied volume interfaces, and registers instance in
 * capabilities registry and in shutdown manager.
 *
 * Additional channel volume interfaces can be added after construction using public SpeakerManagerInterface methods.
 *
 * Speaker manager groups all channels by type, and applies volume settings and configurations uniformly to all channels
 * of the same type.
 *
 * Speaker manager uses SpeakerManagerConfigInterface to load initial (bootstrap) platform configuration, and
 * SpeakerManagerStorageInterface to store and load persistent settings. Those interfaces
 *
 * @param[in] config                         Interface to load platform configuration.
 * @param[in] storage                        Interface to load and store persistent configuration.
 * @param[in] contextManager                 A @c ContextManagerInterface to manage the context.
 * @param[in] messageSender                  A @c MessageSenderInterface to send messages to AVS.
 * @param[in] exceptionEncounteredSender     An @c ExceptionEncounteredSenderInterface to send directive processing
 *                                           exceptions to AVS.
 * @param[in] metricRecorder                 The metric recorder.
 * @param[in] shutdownNotifier               Factory uses this interface to register for shutdown notifications.
 * @param[in] endpointCapabilitiesRegistrar  Factory uses this interface to register capability.
 * @param[in] volumeInterfaces               Optional vector of @c ChannelVolumeInterfaces to register. Additional
 *                                           interfaces can be added with @c
 *                                           SpeakerManagerInterface::addChannelVolumeInterface() calls.
 *
 * @see createSpeakerManagerStorage()
 * @see createSpeakerManagerConfig()
 * @see createChannelVolumeFactory()
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> createSpeakerManagerCapabilityAgent(
    std::shared_ptr<SpeakerManagerConfigInterface> config,
    std::shared_ptr<SpeakerManagerStorageInterface> storage,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder,
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
    const std::shared_ptr<avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
        endpointCapabilitiesRegistrar,
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& volumeInterfaces =
        {}) noexcept;

/**
 * @brief Create default implementation of @c ChannelVolumeFactoryInterface.
 *
 * @return Channel volume factory interface or nullptr on error.
 */
std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeFactoryInterface> createChannelVolumeFactory() noexcept;

/**
 * @brief Adapt generic @c MiscStorageInterface into @c SpeakerManagerStorageInterface.
 *
 * Method returns an adapter of @c SpeakerManagerStorageInterface to @c SpeakerManagerStorageInterface.
 *
 * @param[in] storage Reference of @c MiscStorageInterface. This parameter must not be nullptr.
 *
 * @return Storage interface or nullptr on error.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
std::shared_ptr<SpeakerManagerStorageInterface> createSpeakerManagerStorage(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage) noexcept;

/**
 * @brief Creates configuration interface for speaker manager.
 *
 * The method returns an interface that accesses configuration using @c ConfigurationNode facility. The returned object
 * uses "speakerManagerCapabilityAgent" child and looks up the following keys:
 * - "persistentStorage" -- Boolean flag that indicates if persistent storage is enabled.
 * - "minUnmuteVolume" -- Minimum volume level for unmuting the channel. This setting applies to all channel types.
 * - "defaultSpeakerVolume" -- Default speaker volume.
 * - "defaultAlertsVolume" -- Default alerts volume.
 * - "restoreMuteState" -- Boolean flag that indicates if mute state shall be preserved between device reboots.
 *
 * If AlexaClientSDKConfig.json configuration file is used, the example configuration may look like:
 * @code{.json}
 * {
 *    "speakerManagerCapabilityAgent": {
 *        "persistentStorage": true,
 *        "minUnmuteVolume": 10,
 *        "defaultSpeakerVolume": 40,
 *        "defaultAlertsVolume": 40,
 *        "restoreMuteState": true
 *    }
 * }
 * @endcode
 *
 * @return Interface to load initial configuration or nullptr on error.
 *
 * @ingroup Lib_acsdkSpeakerManager
 */
std::shared_ptr<SpeakerManagerConfigInterface> createSpeakerManagerConfig() noexcept;

}  // namespace speakerManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SPEAKERMANAGER_SPEAKERMANAGER_INCLUDE_ACSDK_SPEAKERMANAGER_FACTORIES_H_
