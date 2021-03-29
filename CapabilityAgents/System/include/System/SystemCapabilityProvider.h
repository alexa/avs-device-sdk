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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCAPABILITYPROVIDER_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCAPABILITYPROVIDER_H_

#include <memory>

#include <AVSCommon/AVS/CapabilityChangeNotifierInterface.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsObserverInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

/**
 * This class handles providing configuration for the System Capability agent, since a single class
 * does not handle all of the capability agent's functionings.
 */
class SystemCapabilityProvider
        : public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::sdkInterfaces::LocaleAssetsObserverInterface
        , public std::enable_shared_from_this<SystemCapabilityProvider> {
public:
    /**
     * Create an instance of @c SystemCapabilityProvider.
     *
     * @param localeAssetsManager The locale assets manager that provides supported locales.
     * @param capabilityChangeNotifier The object with which to notify observers of @c SystemCapabilityProvider
     *     capability configurations change.
     */
    static std::shared_ptr<SystemCapabilityProvider> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
        const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier);

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name LocaleAssetsObserverInterface Functions
    /// @{
    void onLocaleAssetsChanged() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param localeAssetsManager The locale assets manager that provides supported locales.
     * @param capabilityChangeNotifier The object with which to notify observers of @c SystemCapabilityProvider
     *     capability configurations change.
     */
    SystemCapabilityProvider(
        const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
        const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier);

    /**
     * Initialize the system capability provider.
     *
     * @return true on success, false on failure.
     */
    bool initialize();

    /// Set of capability configurations that will get published using the Capabilities API
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// The locale assets manager.
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> m_assetsManager;

    /// The object to notify of @c SystemCapabilityProvider capability configurations change.
    std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface> m_capabilityChangeNotifier;

    /// Mutex to serialize access to m_capabilityConfigurations.
    std::mutex m_mutex;
};

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SYSTEM_INCLUDE_SYSTEM_SYSTEMCAPABILITYPROVIDER_H_
