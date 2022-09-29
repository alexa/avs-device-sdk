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
#include <memory>

#include <acsdk/SpeakerManager/Factories.h>
#include <acsdk/SpeakerManager/private/DefaultChannelVolumeFactory.h>
#include <acsdk/SpeakerManager/private/SpeakerManager.h>
#include <acsdk/SpeakerManager/private/SpeakerManagerConfig.h>
#include <acsdk/SpeakerManager/private/SpeakerManagerMiscStorage.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace speakerManager {

/// @addtogroup Lib_acsdkSpeakerManager
/// @{

/// @brief String to identify log entries originating from this file.
/// @private
#define TAG "SpeakerManagerFactories"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// @}

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
    const std::vector<std::shared_ptr<avsCommon::sdkInterfaces::ChannelVolumeInterface>>& volumeInterfaces) noexcept {
    if (!shutdownNotifier) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullShutdownNotifier"));
        return nullptr;
    }
    if (!endpointCapabilitiesRegistrar) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullEndpointCapabilitiesRegistrar"));
        return nullptr;
    }

    auto speakerManager = SpeakerManager::create(
        std::move(config),
        std::move(storage),
        volumeInterfaces,
        std::move(contextManager),
        std::move(messageSender),
        std::move(exceptionEncounteredSender),
        std::move(metricRecorder));

    if (!speakerManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "errorSpeakerManagerCreate"));
        return nullptr;
    }

    shutdownNotifier->addObserver(speakerManager);
    endpointCapabilitiesRegistrar->withCapability(speakerManager, speakerManager);

    return speakerManager;
}

std::shared_ptr<SpeakerManagerStorageInterface> createSpeakerManagerStorage(
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface> storage) noexcept {
    return SpeakerManagerMiscStorage::create(std::move(storage));
}

std::shared_ptr<SpeakerManagerConfigInterface> createSpeakerManagerConfig() noexcept {
    return std::make_shared<SpeakerManagerConfig>();
}

}  // namespace speakerManager
}  // namespace alexaClientSDK
