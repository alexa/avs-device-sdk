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

#ifndef ACSDKALERTS_ALERTSCOMPONENT_H_
#define ACSDKALERTS_ALERTSCOMPONENT_H_

#include <memory>

#include <acsdkAlertsInterfaces/AlertsCapabilityAgentInterface.h>
#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/SpeakerManagerInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <CertifiedSender/CertifiedSender.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <Settings/DeviceSettingsManager.h>

#include <acsdkAlerts/Storage/AlertStorageInterface.h>

namespace alexaClientSDK {
namespace acsdkAlerts {

/**
 * Definition of a Manufactory component that exports an Alerts Capability Agent.
 */
using AlertsComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkAlertsInterfaces::AlertsCapabilityAgentInterface>,
    acsdkManufactory::Import<
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionMonitorInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::SpeakerManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<certifiedSender::CertifiedSender>>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<settings::DeviceSettingsManager>>,
    acsdkManufactory::Import<std::shared_ptr<storage::AlertStorageInterface>>>;

/**
 * Get the manufactory @c Component for an Alerts Capability Agent.
 *
 * @param startAlertSchedulingOnInitialization Whether to start scheduling alerts after client initialization. If
 * this is set to false, no alert scheduling will occur until onSystemClockSynchronized is called.
 * @return An @c AlertsComponent.
 */
AlertsComponent getComponent(bool startAlertSchedulingOnInitialization = true);

}  // namespace acsdkAlerts
}  // namespace alexaClientSDK

#endif  // ACSDKALERTS_ALERTSCOMPONENT_H_
