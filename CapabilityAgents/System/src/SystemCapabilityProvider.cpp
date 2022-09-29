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

#include <string>
#include <unordered_set>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "System/SystemCapabilityProvider.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace system {

using namespace avsCommon::avs;

/// String to identify log entries originating from this file.
#define TAG "SystemCapabilityProvider"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// System capability constants
/// System interface type
static const std::string SYSTEM_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";
/// System interface name
static const std::string SYSTEM_CAPABILITY_INTERFACE_NAME = "System";
/// System interface version
static const std::string SYSTEM_CAPABILITY_INTERFACE_VERSION = "2.1";

/// LOCALES_CONFIGURATION_KEY
static const std::string LOCALES_CONFIGURATION_KEY = "locales";

/// Locale Combinations Key
static const std::string LOCALE_COMBINATION_CONFIGURATION_KEY = "localeCombinations";

/**
 * Creates the System capability configuration.
 *
 * @param localeAssetsManager The locale assets manager that provides supported locales.
 * @return The System capability configuration.
 */
static std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSystemCapabilityConfiguration(
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager);

std::shared_ptr<SystemCapabilityProvider> SystemCapabilityProvider::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
    const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier) {
    if (!localeAssetsManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLocaleAssetsManager"));
        return nullptr;
    } else if (!capabilityChangeNotifier) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullCapabilityChangeNotifier"));
        return nullptr;
    }
    auto systemCapabilityProvider = std::shared_ptr<SystemCapabilityProvider>(
        new SystemCapabilityProvider(localeAssetsManager, capabilityChangeNotifier));

    if (!systemCapabilityProvider->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        return nullptr;
    }
    return systemCapabilityProvider;
}

SystemCapabilityProvider::SystemCapabilityProvider(
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager,
    const std::shared_ptr<avsCommon::avs::CapabilityChangeNotifierInterface>& capabilityChangeNotifier) :
        m_assetsManager{localeAssetsManager},
        m_capabilityChangeNotifier{capabilityChangeNotifier} {
    auto capabilityConfiguration = getSystemCapabilityConfiguration(localeAssetsManager);
    m_capabilityConfigurations.insert(capabilityConfiguration);
}

std::shared_ptr<avsCommon::avs::CapabilityConfiguration> getSystemCapabilityConfiguration(
    const std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface>& localeAssetsManager) {
    std::unordered_map<std::string, std::string> configMap;
    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, SYSTEM_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, SYSTEM_CAPABILITY_INTERFACE_NAME});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, SYSTEM_CAPABILITY_INTERFACE_VERSION});

    avsCommon::utils::json::JsonGenerator generator;
    generator.addStringArray(LOCALES_CONFIGURATION_KEY, localeAssetsManager->getSupportedLocales());
    generator.addCollectionOfStringArray(
        LOCALE_COMBINATION_CONFIGURATION_KEY, localeAssetsManager->getSupportedLocaleCombinations());
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, generator.toString()});

    ACSDK_DEBUG5(LX(__func__).d("locales", generator.toString()));

    return std::make_shared<avsCommon::avs::CapabilityConfiguration>(configMap);
}

std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> SystemCapabilityProvider::
    getCapabilityConfigurations() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_capabilityConfigurations;
}

void SystemCapabilityProvider::onLocaleAssetsChanged() {
    ACSDK_DEBUG(LX(__func__));
    std::unique_lock<std::mutex> lock(m_mutex);
    m_capabilityConfigurations.clear();
    auto newLocales = getSystemCapabilityConfiguration(m_assetsManager);
    m_capabilityConfigurations.insert(newLocales);
    lock.unlock();
    m_capabilityChangeNotifier->notifyObservers(
        [newLocales](
            std::shared_ptr<avsCommon::sdkInterfaces::CapabilityConfigurationChangeObserverInterface> observer) {
            observer->onConfigurationChanged(*newLocales);
        });
}

bool SystemCapabilityProvider::initialize() {
    m_assetsManager->addLocaleAssetsObserver(shared_from_this());
    return true;
}

}  // namespace system
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
