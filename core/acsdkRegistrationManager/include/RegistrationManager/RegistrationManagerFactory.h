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

#ifndef REGISTRATIONMANAGER_REGISTRATIONMANAGERFACTORY_H_
#define REGISTRATIONMANAGER_REGISTRATIONMANAGERFACTORY_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <AVSCommon/Utils/Optional.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

/**
 * Creates a new instance of @c RegistrationManagerInterface.
 *
 * @param customerDataManager The @c CustomerDataManagerInterface to handle customer data handlers.
 * @param connectionManager The @c AVSConnectionManagerInterface to handle connection with AVS.
 * @param directiveSequencer The @c DirectiveSequencerInterface to handle @c AVSDirective instances.
 * @param metricRecorder The @c MetricRecorderInterface to record metrics to send to sinks.
 * @param notifier The @c RegistrationNotifierInterface to observe changes to the RegistrationManager.
 * @return A pointer to the @c RegistrationManagerInterface.
 */
std::shared_ptr<registrationManager::RegistrationManagerInterface> createRegistrationManager(
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
    const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder,
    const std::shared_ptr<registrationManager::RegistrationNotifierInterface>& notifier);

/**
 * Creates a new instance of @c RegistrationNotifierInterface.
 *
 * @return A pointer to the @c RegistrationNotifierInterface.
 */
std::shared_ptr<registrationManager::RegistrationNotifierInterface> createRegistrationNotifier();

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_REGISTRATIONMANAGERFACTORY_H_
