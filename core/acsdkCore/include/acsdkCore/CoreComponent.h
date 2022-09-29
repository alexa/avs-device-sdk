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

#ifndef ACSDKCORE_CORECOMPONENT_H_
#define ACSDKCORE_CORECOMPONENT_H_

#include <memory>

#include <acsdkAlexaEventProcessedNotifierInterfaces/AlexaEventProcessedNotifierInterface.h>
#include <acsdkManufactory/Component.h>
#include <acsdkPostConnectOperationProviderRegistrarInterfaces/PostConnectOperationProviderRegistrarInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockNotifierInterface.h>
#include <acsdkSystemClockMonitorInterfaces/SystemClockMonitorInterface.h>
#include <AVSCommon/AVS/DialogUXStateAggregator.h>
#include <AVSCommon/AVS/Attachment/AttachmentManagerInterface.h>
#include <AVSCommon/Utils/DeviceInfo.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/Timing/MultiTimer.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilitiesDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointBuilderInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <Alexa/AlexaInterfaceMessageSender.h>
#include <InterruptModel/InterruptModel.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkCore {

using DefaultEndpointAnnotation = avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation;

/**
 * Definition of a Manufactory component for core types of the SDK.
 */
using CoreComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::avs::attachment::AttachmentManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>,
    std::shared_ptr<
        acsdkPostConnectOperationProviderRegistrarInterfaces::PostConnectOperationProviderRegistrarInterface>,
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockNotifierInterface>,
    std::shared_ptr<acsdkSystemClockMonitorInterfaces::SystemClockMonitorInterface>,
    std::shared_ptr<avsCommon::utils::DeviceInfo>,
    std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayManagerInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::CapabilitiesDelegateInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>,
    acsdkManufactory::
        Annotated<DefaultEndpointAnnotation, avsCommon::sdkInterfaces::endpoints::EndpointBuilderInterface>,
    acsdkManufactory::Annotated<
        DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>,
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>,
    std::shared_ptr<capabilityAgents::alexa::AlexaInterfaceMessageSender>,
    std::shared_ptr<afml::interruptModel::InterruptModel>,
    acsdkManufactory::
        Annotated<avsCommon::sdkInterfaces::AudioFocusAnnotation, avsCommon::sdkInterfaces::FocusManagerInterface>,
    std::shared_ptr<registrationManager::CustomerDataManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationManagerInterface>,
    std::shared_ptr<registrationManager::RegistrationNotifierInterface>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AuthDelegateInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::timing::MultiTimer>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::DeviceInfo>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>>>;

/**
 * Get a manufactory @c Component exporting core AVS client functionality.
 *
 * @return The dependency injection @c Component for acsdkCore.
 */
CoreComponent getComponent();

}  // namespace acsdkCore
}  // namespace alexaClientSDK

#endif  // ACSDKCORE_CORECOMPONENT_H_
