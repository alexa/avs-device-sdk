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

#include <acsdkManufactory/ComponentAccumulator.h>

#include <acsdk/SpeakerManager/SpeakerManagerComponent.h>
#include <acsdk/SpeakerManager/Factories.h>

namespace alexaClientSDK {
namespace speakerManagerComponent {

using namespace acsdkManufactory;
using namespace speakerManager;

/**
 * @brief Helper for manufactory.
 *
 * This method uses annotated types for correct interface lookup in manufactory.
 *
 * @param[in] storage                        A @c MiscStorageInterface to access persistent configuration.
 * @param[in] contextManager                 A @c ContextManagerInterface to manage the context.
 * @param[in] messageSender                  A @c MessageSenderInterface to send messages to AVS.
 * @param[in] exceptionEncounteredSender     An @c ExceptionEncounteredSenderInterface to send directive processing
 * exceptions to AVS.
 * @param[in] shutdownNotifier               A @c ShutdownNotifierInterface to notify the SpeakerManager when it's time
 * to shut down.
 * @param[in] endpointCapabilitiesRegistrar  The @c EndpointCapabilitiesRegistrarInterface for the default endpoint
 * (annotated with DefaultEndpointAnnotation), so that the SpeakerManager can register itself as a capability
 * with the default endpoint.
 * @param[in] metricRecorder                 The metric recorder.
 *
 * @return Reference to SpeakerManager CA or nullptr on error.
 *
 * @see createSpeakerManagerCapabilityAgent()
 * @private
 * @ingroup Lib_acsdkSpeakerManagerComponent
 */
static std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface> createSpeakerManagerCA(
    std::shared_ptr<SpeakerManagerConfigInterface> config,
    std::shared_ptr<SpeakerManagerStorageInterface> storage,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionEncounteredSender,
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
    const Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>& endpointCapabilitiesRegistrar,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) noexcept {
    return createSpeakerManagerCapabilityAgent(
        std::move(config),
        std::move(storage),
        std::move(contextManager),
        std::move(messageSender),
        std::move(exceptionEncounteredSender),
        std::move(metricRecorder),
        shutdownNotifier,
        endpointCapabilitiesRegistrar);
}

SpeakerManagerComponent getSpeakerManagerComponent() noexcept {
    return ComponentAccumulator<>()
        .addRequiredFactory(createSpeakerManagerCA)
        .addRequiredFactory(createSpeakerManagerStorage)
        .addRequiredFactory(createSpeakerManagerConfig);
}

}  // namespace speakerManagerComponent
}  // namespace alexaClientSDK
