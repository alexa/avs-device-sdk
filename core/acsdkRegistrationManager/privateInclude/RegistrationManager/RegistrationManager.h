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

#ifndef REGISTRATIONMANAGER_REGISTRATIONMANAGER_H_
#define REGISTRATIONMANAGER_REGISTRATIONMANAGER_H_

#include <memory>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/DirectiveSequencerInterface.h>
#include <AVSCommon/Utils/Metrics/MetricRecorderInterface.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>
#include <RegistrationManager/RegistrationManagerInterface.h>
#include <RegistrationManager/RegistrationNotifierInterface.h>

namespace alexaClientSDK {
namespace registrationManager {

class RegistrationManager : public registrationManager::RegistrationManagerInterface {
public:
    /**
     * Creates a new instance of @c RegistrationManager.
     *
     * @param dataManager Object that manages customer data, which must be cleared during logout.
     * @param connectionManager Connection manager must be disabled during customer logout.
     * @param directiveSequencer Object used to clear directives during logout process.
     * @param metricRecorder Object that is used to record the logout metric.
     *
     * @return Pointer to the @c RegistrationManagerInterface.
     */
    static std::shared_ptr<registrationManager::RegistrationManagerInterface> createRegistrationManagerInterface(
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& dataManager,
        const std::shared_ptr<registrationManager::RegistrationNotifierInterface>& notifier,
        const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

    /// @name RegistrationManagerInterface methods.
    /// @{
    void logout() override;
    /// @}
private:
    /**
     * Constructor.
     *
     * @param dataManager Object that manages customer data, which must be cleared during logout.
     * @param connectionManager Connection manager must be disabled during customer logout.
     * @param directiveSequencer Object used to clear directives during logout process.
     * @param metricRecorder Object that is used to record the logout metric.
     */
    RegistrationManager(
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& dataManager,
        const std::shared_ptr<registrationManager::RegistrationNotifierInterface>& notifier,
        const std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface>& connectionManager,
        const std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface>& directiveSequencer,
        const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder);

    /// Pointer to the @c CustomerDataManager.
    std::shared_ptr<registrationManager::CustomerDataManagerInterface> m_dataManager;
    /// Pointer to the @c RegistrationNotifier.
    std::shared_ptr<registrationManager::RegistrationNotifierInterface> m_notifier;
    /// Pointer to the @c AVSConnectionManager.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> m_connectionManager;
    /// Pointer to the @c DirectiveSequencer.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> m_directiveSequencer;
    /// Pointer to the @c MetricRecorder.
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;
};

}  // namespace registrationManager
}  // namespace alexaClientSDK

#endif  // REGISTRATIONMANAGER_REGISTRATIONMANAGER_H_
