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

#include "acsdkAlerts/AlertsCapabilityAgent.h"
#include "acsdkAlerts/AlertsComponent.h"
#include "acsdkAlerts/Renderer/Renderer.h"

namespace alexaClientSDK {
namespace acsdkAlerts {

/**
 * Forwards startAlertSchedulingOnInitialization to the Alert CA's factory method.
 *
 * @param startAlertSchedulingOnInitialization Whether to start scheduling alerts after client initialization. If
 * this is set to false, no alert scheduling will occur until onSystemClockSynchronized is called.
 * @return A std::function factory method for the Alerts CA.
 */
std::function<std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>(
    const std::shared_ptr<acsdkAlerts::renderer::Renderer>&,
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>&,
    const acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>&,
    const std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>&,
    const acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>&,
    const std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>&,
    const std::shared_ptr<certifiedSender::CertifiedSender>&,
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>&,
    const std::shared_ptr<settings::DeviceSettingsManager>&,
    const std::shared_ptr<storage::AlertStorageInterface>&)>
getCreateAlertsCapabilityAgent(bool startAlertSchedulingOnInitialization) {
    return
        [startAlertSchedulingOnInitialization](
            const std::shared_ptr<acsdkAlerts::renderer::Renderer>& alertRenderer,
            const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
            const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
            const std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>& contextManager,
            const std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>& exceptionSender,
            const acsdkManufactory::Annotated<
                avsCommon::sdkInterfaces::AudioFocusAnnotation,
                avsCommon::sdkInterfaces::FocusManagerInterface>& audioFocusManager,
            const std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>& messageSender,
            const std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>& speakerManager,
            const std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>& audioFactory,
            const acsdkManufactory::Annotated<
                avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
                avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>&
                endpointCapabilitiesRegistrar,
            const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
            const std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>& systemClockMonitor,
            const std::shared_ptr<certifiedSender::CertifiedSender>& certifiedSender,
            const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& dataManager,
            const std::shared_ptr<settings::DeviceSettingsManager>& settingsManager,
            const std::shared_ptr<storage::AlertStorageInterface>& alertStorage)
            -> std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface> {
            return acsdkAlerts::AlertsCapabilityAgent::createAlertsCapabilityAgent(
                alertRenderer,
                shutdownNotifier,
                connectionManager,
                contextManager,
                exceptionSender,
                audioFocusManager,
                messageSender,
                speakerManager,
                audioFactory,
                endpointCapabilitiesRegistrar,
                metricRecorder,
                systemClockMonitor,
                certifiedSender,
                dataManager,
                settingsManager,
                alertStorage,
                startAlertSchedulingOnInitialization);
        };
}

AlertsComponent getComponent(bool startAlertSchedulingOnInitialization) {
    return acsdkManufactory::ComponentAccumulator<>()
        .addRetainedFactory(acsdkAlerts::renderer::Renderer::createAlertRenderer)
        .addRequiredFactory(getCreateAlertsCapabilityAgent(startAlertSchedulingOnInitialization));
}

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK
