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

#ifndef ACSDKNOTIFICATIONS_NOTIFICATIONSCOMPONENT_H_
#define ACSDKNOTIFICATIONS_NOTIFICATIONSCOMPONENT_H_

#include <memory>

#include <acsdkApplicationAudioPipelineFactoryInterfaces/ApplicationAudioPipelineFactoryInterface.h>
#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <acsdkNotificationsInterfaces/NotificationsNotifierInterface.h>
#include <acsdkNotificationsInterfaces/NotificationsStorageInterface.h>
#include <acsdkShutdownManagerInterfaces/ShutdownNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/AudioFocusAnnotation.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/FocusManagerInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Audio/AudioFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/Endpoints/DefaultEndpointAnnotation.h>
#include <AVSCommon/SDKInterfaces/Endpoints/EndpointCapabilitiesRegistrarInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkNotifications {

/**
 * Definition of a Manufactory Component for Notifications.
 */
using NotificationsComponent = acsdkManufactory::Component<
    std::shared_ptr<acsdkNotificationsInterfaces::NotificationsNotifierInterface>,
    acsdkManufactory::Import<
        std::shared_ptr<acsdkApplicationAudioPipelineFactoryInterfaces::ApplicationAudioPipelineFactoryInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkNotificationsInterfaces::NotificationsStorageInterface>>,
    acsdkManufactory::Import<std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::AudioFocusAnnotation,
        avsCommon::sdkInterfaces::FocusManagerInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::sdkInterfaces::audio::AudioFactoryInterface>>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::endpoints::DefaultEndpointAnnotation,
        avsCommon::sdkInterfaces::endpoints::EndpointCapabilitiesRegistrarInterface>>,
    acsdkManufactory::Import<std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>>,
    acsdkManufactory::Import<std::shared_ptr<registrationManager::CustomerDataManagerInterface>>>;

/**
 * Creates an manufactory component that exports a @c NotificationsNotifierInterface.
 *
 * @return A component.
 */
NotificationsComponent getComponent();

}  // namespace acsdkNotifications
}  // namespace alexaClientSDK

#endif  // ACSDKNOTIFICATIONS_NOTIFICATIONSCOMPONENT_H_
