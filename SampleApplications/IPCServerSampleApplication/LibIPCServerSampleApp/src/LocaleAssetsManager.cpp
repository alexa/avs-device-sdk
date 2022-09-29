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

#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <Settings/SettingStringConversion.h>

#include "IPCServerSampleApp/LocaleAssetsManager.h"

/// String to identify log entries originating from this file.
#define TAG "LocaleAssetsManager"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace sampleApplications {
namespace ipcServerSampleApp {

using namespace acsdkShutdownManagerInterfaces;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
using namespace avsCommon::utils::json::jsonUtils;

/// The key in our config file to find the root of settings for this database.
static const std::string SETTING_CONFIGURATION_ROOT_KEY = "deviceSettings";

/// The key to find the locales configuration.
static const std::string LOCALES_CONFIGURATION_KEY = "locales";

/// The key to find the locales configuration.
static const std::string DEFAULT_LOCALE_CONFIGURATION_KEY = "defaultLocale";

/// The key to find the locales combinations configuration.
static const std::string LOCALE_COMBINATION_CONFIGURATION_KEY = "localeCombinations";

/// The default locale value used if the locale configuration is not present.
static const std::string DEFAULT_LOCALE_VALUE = "en-US";

/// The index for primary locale in the default locales vector.
static const int PRIMARY_LOCALE_INDEX = 0;

/// The default supported wake word.
static const std::string DEFAULT_SUPPORTED_WAKEWORD = "ALEXA";

std::shared_ptr<LocaleAssetsManagerInterface> LocaleAssetsManager::createLocaleAssetsManagerInterface(
    const std::shared_ptr<ConfigurationNode>& configurationNode,
    const std::shared_ptr<ShutdownNotifierInterface>& shutdownNotifier) {
    if (!shutdownNotifier) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceFailed").d("reason", "null shutdown notifier"));
        return nullptr;
    }

    auto manager = std::shared_ptr<LocaleAssetsManager>(new LocaleAssetsManager());
    if (!manager) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceFailed").d("reason", "null LocaleAssetsManager"));
        return nullptr;
    }

    if (!manager->initialize(configurationNode)) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceFailed").d("reason", "initialize failed"));
        return nullptr;
    }
    shutdownNotifier->addObserver(manager);

    return manager;
}

std::shared_ptr<LocaleAssetsManager> LocaleAssetsManager::createLocaleAssetsManager(
    const std::shared_ptr<ConfigurationNode>& configurationNode) {
    auto manager = std::shared_ptr<LocaleAssetsManager>(new LocaleAssetsManager());
    if (!manager->initialize(configurationNode)) {
        ACSDK_ERROR(LX("createLocaleAssetsManager").d("reason", "initialize failed"));
        return nullptr;
    }
    return manager;
}

std::shared_ptr<LocaleAssetsManager> LocaleAssetsManager::create(bool enableWakeWord) {
    auto manager = std::shared_ptr<LocaleAssetsManager>(new LocaleAssetsManager());
    if (!manager->initialize(enableWakeWord, ConfigurationNode::getRoot())) {
        ACSDK_ERROR(LX("createFailed").d("reason", "initialize failed"));
        return nullptr;
    }
    return manager;
}

#ifdef KWD
#ifdef ASSET_MANAGER
std::shared_ptr<LocaleAssetsManagerInterface> LocaleAssetsManager::createLocaleAssetsManagerInterfaceWithKwdAndDavs(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
    const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector,
    const std::shared_ptr<AssetManager>& assetManager) {
    if (!shutdownNotifier) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdAndDavsFailed").d("reason", "null shutdown notifier"));
        return nullptr;
    }
    if (!audioInputStream) {
        ACSDK_ERROR(
            LX("createLocaleAssetsManagerInterfaceWithKwdAndDavsFailed").d("reason", "null audio input stream"));
        return nullptr;
    }
    if (!audioFormat) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdAndDavsFailed").d("reason", "null audio format"));
        return nullptr;
    }
    if (!keywordDetector) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdAndDavsFailed").d("reason", "null keywordDetector"));
        return nullptr;
    }
    if (!assetManager) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdAndDavsFailed").d("reason", "null davsAssetManager"));
        return nullptr;
    }

    auto manager = std::shared_ptr<LocaleAssetsManager>(
        new LocaleAssetsManager(audioInputStream, audioFormat, keywordDetector, assetManager));
    if (!manager->initialize(configurationNode)) {
        ACSDK_ERROR(
            LX("createLocaleAssetsManagerInterfaceWithKwdAndDavsFailedFailed").d("reason", "initialize failed"));
        return nullptr;
    }
    shutdownNotifier->addObserver(manager);

    return manager;
}

std::shared_ptr<LocaleAssetsManager> LocaleAssetsManager::createLocaleAssetsManagerWithKwdAndDavs(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector,
    const std::shared_ptr<AssetManager>& assetManager) {
    if (!audioInputStream) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdAndDavsFailed").d("reason", "null audio input stream"));
        return nullptr;
    }
    if (!audioFormat) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdAndDavsFailed").d("reason", "null audio format"));
        return nullptr;
    }
    if (!keywordDetector) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdAndDavsFailed").d("reason", "null keywordDetector"));
        return nullptr;
    }
    if (!assetManager) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdAndDavsFailed").d("reason", "null davsAssetManager"));
        return nullptr;
    }

    auto manager = std::shared_ptr<LocaleAssetsManager>(
        new LocaleAssetsManager(audioInputStream, audioFormat, keywordDetector, assetManager));
    if (!manager->initialize(configurationNode)) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdAndDavsFailed").d("reason", "initialize failed"));
        return nullptr;
    }

    return manager;
}
#else
std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> LocaleAssetsManager::
    createLocaleAssetsManagerInterfaceWithKwd(
        const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
        const std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface>& shutdownNotifier,
        const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
        const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
        const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector) {
    if (!shutdownNotifier) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdFailed").d("reason", "null shutdown notifier"));
        return nullptr;
    }
    if (!audioInputStream) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdFailed").d("reason", "null audio input stream"));
        return nullptr;
    }
    if (!audioFormat) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdFailed").d("reason", "null audio format"));
        return nullptr;
    }
    if (!keywordDetector) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdFailed").d("reason", "null keywordDetector"));
        return nullptr;
    }

    auto manager =
        std::shared_ptr<LocaleAssetsManager>(new LocaleAssetsManager(audioInputStream, audioFormat, keywordDetector));
    if (!manager->initialize(configurationNode)) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerInterfaceWithKwdFailed").d("reason", "initialize failed"));
        return nullptr;
    }
    shutdownNotifier->addObserver(manager);

    return manager;
}

std::shared_ptr<LocaleAssetsManager> LocaleAssetsManager::createLocaleAssetsManagerWithKwd(
    const std::shared_ptr<avsCommon::utils::configuration::ConfigurationNode>& configurationNode,
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector) {
    if (!audioInputStream) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdFailed").d("reason", "null audio input stream"));
        return nullptr;
    }
    if (!audioFormat) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdFailed").d("reason", "null audio format"));
        return nullptr;
    }
    if (!keywordDetector) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdFailed").d("reason", "null keywordDetector"));
        return nullptr;
    }
    auto manager =
        std::shared_ptr<LocaleAssetsManager>(new LocaleAssetsManager(audioInputStream, audioFormat, keywordDetector));
    if (!manager->initialize(configurationNode)) {
        ACSDK_ERROR(LX("createLocaleAssetsManagerWithKwdFailed").d("reason", "initialize failed"));
        return nullptr;
    }

    return manager;
}
#endif
void LocaleAssetsManager::setDefaultClient(const std::shared_ptr<defaultClient::DefaultClient>& defaultClient) {
    std::lock_guard<std::mutex> lock{m_defaultClientMutex};
    if (defaultClient == nullptr) {
        ACSDK_ERROR(LX("setDefaultClientFailed").d("reason", "nullDefaultClient"));
        return;
    }
    m_DefaultClient = defaultClient;
}
#endif

bool LocaleAssetsManager::changeAssets(const Locales& locales, const WakeWords& wakeWords) {
    // The device should set the locale and wakeWord here.
    ACSDK_INFO(LX(__func__)
                   .d("Locale", settings::toSettingString<Locales>(locales).second)
                   .d("WakeWords", settings::toSettingString<WakeWords>(wakeWords).second));
    return true;
}

void LocaleAssetsManager::cancelOngoingChange() {
    // No work is done by changeAssets.
}

bool LocaleAssetsManager::initialize(const std::shared_ptr<ConfigurationNode>& configurationNode) {
    if (!configurationNode) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "null configuration node"));
        return false;
    }

#ifdef KWD
    static constexpr bool enableWakeWord = true;
#else
    static constexpr bool enableWakeWord = false;
#endif

    return initialize(enableWakeWord, *configurationNode);
}

bool LocaleAssetsManager::initialize(bool enableWakeWord, const ConfigurationNode& configurationNode) {
    auto settingsConfig = configurationNode[SETTING_CONFIGURATION_ROOT_KEY];
    if (!settingsConfig) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "configurationKeyNotFound")
                        .d("configurationKey", SETTING_CONFIGURATION_ROOT_KEY));
        return false;
    }

    if (!settingsConfig.getStringValues(LOCALES_CONFIGURATION_KEY, &m_supportedLocales)) {
        ACSDK_ERROR(LX("initializeFailed")
                        .d("reason", "configurationKeyNotFound")
                        .d("configurationKey", LOCALES_CONFIGURATION_KEY));
        return false;
    }

    auto combinationArray = settingsConfig.getArray(LOCALE_COMBINATION_CONFIGURATION_KEY);
    if (combinationArray) {
        for (std::size_t arrayIndex = 0; arrayIndex < combinationArray.getArraySize(); ++arrayIndex) {
            auto stringVector = retrieveStringArray<std::vector<std::string>>(combinationArray[arrayIndex].serialize());

            // Make sure the combination is more than one locale.
            if (stringVector.size() <= 1) {
                ACSDK_ERROR(
                    LX("initializeFailed").d("reason", "LocaleCombinationSizeError").d("size", stringVector.size()));
                return false;
            }

            // Make sure all the locales in the combination are supported.
            for (const auto& locale : stringVector) {
                if (m_supportedLocales.count(locale) == 0) {
                    ACSDK_ERROR(
                        LX("initializeFailed").d("reason", "notSupportedLocalesInCombination").d("locale", locale));
                    return false;
                }
            }
            m_supportedLocalesCombinations.insert(stringVector);
        }
    }

    auto localesArray = settingsConfig.getArray(DEFAULT_LOCALE_CONFIGURATION_KEY);
    if (!localesArray) {
        if (!settingsConfig.getString(DEFAULT_LOCALE_CONFIGURATION_KEY, &m_defaultLocale)) {
            ACSDK_ERROR(
                LX("initializeFailed")
                    .d("reason", "configurationKeyNotFound")
                    .d("configurationKey", SETTING_CONFIGURATION_ROOT_KEY + "." + DEFAULT_LOCALE_CONFIGURATION_KEY));
            return false;
        }
        m_defaultLocales.push_back(m_defaultLocale);
    } else {
        m_defaultLocales = retrieveStringArray<std::vector<std::string>>(localesArray.serialize());
    }

    if (m_supportedLocales.empty()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "noSupportedLocalesInConfiguration"));
        return false;
    }

    if (m_defaultLocales.empty()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "noDefaultLocaleInConfiguration"));
        return false;
    }

    // Check if the default is in the supported locales.
    if (m_supportedLocales.find(m_defaultLocales[PRIMARY_LOCALE_INDEX]) == m_supportedLocales.end()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "defaultLocaleNotInSupportedLocaleList"));
        return false;
    }

    // Check if the default multilingual locale is in the supported locale combinations.
    if (m_defaultLocales.size() > 1 &&
        m_supportedLocalesCombinations.find(m_defaultLocales) == m_supportedLocalesCombinations.end()) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "defaultLocalesNotInSupportedLocalesCombinationsList"));
        return false;
    }

    WakeWords wakeWords;
    if (enableWakeWord) {
        m_supportedWakeWords.insert({DEFAULT_SUPPORTED_WAKEWORD});
    } else {
        ACSDK_INFO(LX(__func__).d("supportedWakeWords", "NONE"));
    }

    return true;
}

LocaleAssetsManager::WakeWordsSets LocaleAssetsManager::getSupportedWakeWords(const Locale& locale) const {
    return m_supportedWakeWords;
}

LocaleAssetsManager::WakeWordsSets LocaleAssetsManager::getDefaultSupportedWakeWords() const {
    return m_supportedWakeWords;
}

std::map<LocaleAssetsManager::Locale, LocaleAssetsManager::WakeWordsSets> LocaleAssetsManager::
    getLocaleSpecificWakeWords() const {
    return std::map<LocaleAssetsManager::Locale, LocaleAssetsManager::WakeWordsSets>();
}

std::map<LocaleAssetsManager::LanguageTag, LocaleAssetsManager::WakeWordsSets> LocaleAssetsManager::
    getLanguageSpecificWakeWords() const {
    return std::map<LocaleAssetsManager::LanguageTag, LocaleAssetsManager::WakeWordsSets>();
}

std::set<LocaleAssetsManager::Locale> LocaleAssetsManager::getSupportedLocales() const {
    return m_supportedLocales;
}

LocaleAssetsManager::LocaleCombinations LocaleAssetsManager::getSupportedLocaleCombinations() const {
    return m_supportedLocalesCombinations;
}

LocaleAssetsManager::Locale LocaleAssetsManager::getDefaultLocale() const {
    ACSDK_ERROR(LX("getDefaultLocale").d("reason", "methodDeprecated"));
    return m_defaultLocale;
}

LocaleAssetsManager::Locales LocaleAssetsManager::getDefaultLocales() const {
    return m_defaultLocales;
}

void LocaleAssetsManager::addLocaleAssetsObserver(
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::LocaleAssetsObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    if (observer == nullptr) {
        ACSDK_ERROR(LX("addLocaleAssetsObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_observers.insert(observer);
}

void LocaleAssetsManager::removeLocaleAssetsObserver(
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::LocaleAssetsObserverInterface>& observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    if (observer == nullptr) {
        ACSDK_ERROR(LX("removeLocaleAssetsObserverFailed").d("reason", "nullObserver"));
        return;
    }
    m_observers.erase(observer);
}

void LocaleAssetsManager::setEndpointRegistrationManager(
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::endpoints::EndpointRegistrationManagerInterface>&
        manager) {
    std::lock_guard<std::mutex> lock{m_ermMutex};
    if (manager == nullptr) {
        ACSDK_ERROR(LX("setEndpointRegistrationManagerFailed").m("null EndpointRegistrationManager"));
        return;
    }
    m_endpointRegistrationManager = manager;
}

void LocaleAssetsManager::onConfigurationChanged(
    const alexaClientSDK::avsCommon::avs::CapabilityConfiguration& configuration) {
    // No-op
}

LocaleAssetsManager::LocaleAssetsManager() :
        RequiresShutdown{"LocaleAssetsManager"},
        m_defaultLocale{DEFAULT_LOCALE_VALUE} {
}

#ifdef KWD
#ifdef ASSET_MANAGER
LocaleAssetsManager::LocaleAssetsManager(
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector,
    const std::shared_ptr<AssetManager>& assetManager) :
        RequiresShutdown{"LocaleAssetsManager"},
        m_audioInputStream(audioInputStream),
        m_audioFormat(audioFormat),
        m_keywordDetector(keywordDetector),
        m_assetManager(assetManager) {
}
#else
LocaleAssetsManager::LocaleAssetsManager(
    const std::shared_ptr<avsCommon::avs::AudioInputStream>& audioInputStream,
    const std::shared_ptr<avsCommon::utils::AudioFormat>& audioFormat,
    const std::shared_ptr<acsdkKWDImplementations::AbstractKeywordDetector>& keywordDetector) :
        RequiresShutdown{"LocaleAssetsManager"},
        m_audioInputStream(audioInputStream),
        m_audioFormat(audioFormat),
        m_keywordDetector(keywordDetector) {
}
#endif
#endif

void LocaleAssetsManager::doShutdown() {
    {
        std::lock_guard<std::mutex> lock{m_ermMutex};
        m_endpointRegistrationManager.reset();
    }
    {
        std::lock_guard<std::mutex> lock{m_observersMutex};
        m_observers.clear();
    }
}

}  // namespace ipcServerSampleApp
}  // namespace sampleApplications
}  // namespace alexaClientSDK
