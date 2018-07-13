/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "Settings/Settings.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;
/// String to identify log entries originating from this file.
static const std::string TAG{"Settings"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// The key in our config file to find the root of settings.
static const std::string SETTINGS_CONFIGURATION_ROOT_KEY = "settings";
/// The key in our config file to find the database file path.
static const std::string SETTINGS_DEFAULT_SETTINGS_ROOT_KEY = "defaultAVSClientSettings";
/// The acceptable setting keys to find in our config file.
static const std::unordered_set<std::string> SETTINGS_ACCEPTED_KEYS = {"locale"};

std::shared_ptr<Settings> Settings::create(
    std::shared_ptr<SettingsStorageInterface> settingsStorage,
    std::unordered_set<std::shared_ptr<GlobalSettingsObserverInterface>> globalSettingsObserver,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager) {
    if (!settingsStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "settingsStorageNullReference").d("return", "nullptr"));
        return nullptr;
    }

    if (globalSettingsObserver.empty()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "emptysettingsGlobalObserver").d("return", "nullptr"));
        return nullptr;
    }

    for (auto observer : globalSettingsObserver) {
        if (!observer) {
            ACSDK_ERROR(LX("createFailed").d("reason", "settingsGlobalObserverNullReference").d("return", "nullptr"));
            return nullptr;
        }
    }

    auto settingsObject = std::shared_ptr<Settings>(new Settings(settingsStorage, globalSettingsObserver, dataManager));

    if (!settingsObject->initialize()) {
        ACSDK_ERROR(LX("createFailed").d("reason", "Initialization error."));
        return nullptr;
    }

    return settingsObject;
}

/*
 * TODO - Find if default settings can be sent by connection observer
 *
 * https://issues.labcollab.net/browse/ACSDK-516
 */
void Settings::sendDefaultSettings() {
    // If m_sendDefaultSettings is true, load mapOfSettings with values from @c m_mapOfSettingsAttributes and send to
    // AVS.
    if (m_sendDefaultSettings) {
        std::unordered_map<std::string, std::string> mapOfSettings;

        for (auto& it : m_mapOfSettingsAttributes) {
            mapOfSettings.insert(make_pair(it.first, it.second.valueOfSetting));
        }

        for (auto observer : m_globalSettingsObserver) {
            observer->onSettingChanged(mapOfSettings);
        }
    }
}

void Settings::addGlobalSettingsObserver(std::shared_ptr<GlobalSettingsObserverInterface> globalSettingsObserver) {
    if (!globalSettingsObserver) {
        ACSDK_ERROR(LX("addglobalSettingsObserverFailed").d("reason", "globalSettingsObserverNullReference"));
        return;
    }

    m_executor.submit([this, globalSettingsObserver] {
        // if the observer already exists, it is not added.
        if (!m_globalSettingsObserver.insert(globalSettingsObserver).second) {
            return;
        }
    });
}

void Settings::removeGlobalSettingsObserver(std::shared_ptr<GlobalSettingsObserverInterface> globalSettingsObserver) {
    if (!globalSettingsObserver) {
        ACSDK_ERROR(LX("removeGlobalSettingsObserverFailed").d("reason", "globalSettingsObserverNullReference"));
        return;
    }
    m_executor.submit([this, globalSettingsObserver] { m_globalSettingsObserver.erase(globalSettingsObserver); });
}

void Settings::addSingleSettingObserver(
    const std::string& key,
    std::shared_ptr<SingleSettingObserverInterface> settingObserver) {
    if (!settingObserver) {
        ACSDK_ERROR(LX("addSingleSettingObserverFailed").d("reason", "singleSettingObserverNullReference"));
        return;
    }
    m_executor.submit([this, key, settingObserver] {
        auto it = m_mapOfSettingsAttributes.find(key);
        if (it != m_mapOfSettingsAttributes.end()) {
            it->second.singleSettingObservers.insert(settingObserver);
        }
    });
}

void Settings::removeSingleSettingObserver(
    const std::string& key,
    std::shared_ptr<SingleSettingObserverInterface> settingObserver) {
    if (!settingObserver) {
        ACSDK_ERROR(LX("removeSingleSettingObserverFailed").d("reason", "singleSettingObserverNullReference"));
        return;
    }
    m_executor.submit([this, key, settingObserver] {
        auto it = m_mapOfSettingsAttributes.find(key);
        if (it != m_mapOfSettingsAttributes.end()) {
            it->second.singleSettingObservers.erase(settingObserver);
        }
    });
}

std::future<bool> Settings::changeSetting(const std::string& key, const std::string& value) {
    return m_executor.submit([this, key, value] { return executeChangeSetting(key, value); });
}

bool Settings::executeChangeSetting(const std::string& key, const std::string& value) {
    if (!m_settingsStorage->modify(key, value)) {
        ACSDK_ERROR(LX("executeSettingChangedFailed").d("reason", "databaseUpdateFailed"));
        return false;
    }

    // Store the setting in the map @c m_mapOfSettingsAttributes.
    auto search = m_mapOfSettingsAttributes.find(key);
    if (search != m_mapOfSettingsAttributes.end()) {
        search->second.valueOfSetting = value;
    }

    // Notify the observers of the single settting with value of setting.
    if (search != m_mapOfSettingsAttributes.end()) {
        for (auto observer : search->second.singleSettingObservers) {
            observer->onSettingChanged(key, value);
        }
    }

    std::unordered_map<std::string, std::string> mapOfSettings;

    for (auto& it : m_mapOfSettingsAttributes) {
        mapOfSettings.insert(make_pair(it.first, it.second.valueOfSetting));
    }

    // Notify the global observers with the entire map of settings.
    for (auto observer : m_globalSettingsObserver) {
        observer->onSettingChanged(mapOfSettings);
    }
    return true;
}

bool Settings::initialize() {
    if (!m_settingsStorage->open()) {
        ACSDK_INFO(LX("initialize").m("database file does not exist.  Creating."));
        if (!m_settingsStorage->createDatabase()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "SettingsDatabaseCreationFailed"));
            return false;
        }
    }

    std::unordered_map<std::string, std::string> mapOfSettings;

    // Load all the settings from the database.
    if (!m_settingsStorage->load(&mapOfSettings)) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "databaseReadFailed"));
        return false;
    }

    auto configurationRoot = ConfigurationNode::getRoot()[SETTINGS_CONFIGURATION_ROOT_KEY];
    if (!configurationRoot) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "SettingsConfigurationRootNotFound."));
        return false;
    }

    auto defaultSettingRoot = configurationRoot[SETTINGS_DEFAULT_SETTINGS_ROOT_KEY];

    if (!defaultSettingRoot) {
        ACSDK_ERROR(LX("initializeFailed").d("reason", "DefaultSettingsRootNotFound"));
        return false;
    }

    std::string settingValue;

    for (auto& it : SETTINGS_ACCEPTED_KEYS) {
        if (!defaultSettingRoot.getString(it, &settingValue) || settingValue.empty()) {
            ACSDK_ERROR(LX("initializeFailed").d("reason", "SettingNotFoundinConfigFile"));
            return false;
        }
        // search if the setting exists in database.
        auto searchDatabase = mapOfSettings.find(it);
        SettingElements elem;

        // if the setting exists in the database, store that value in @c m_mapOfSettingsAttributes.
        if (searchDatabase != mapOfSettings.end()) {
            elem.valueOfSetting = searchDatabase->second;
            m_mapOfSettingsAttributes[searchDatabase->first] = elem;
        } else {  // if it doesn't exist, store the default Value in the database and @c m_mapOfSettingsAttributes.
            elem.valueOfSetting = settingValue;
            m_mapOfSettingsAttributes[it] = elem;
            if (!m_settingsStorage->store(it, settingValue)) {
                ACSDK_ERROR(LX("initializeFailed").d("reason", "databaseStoreFailed"));
                return false;
            }
            m_sendDefaultSettings = true;
        }
    }
    return true;
}

void Settings::clearData() {
    auto result = m_executor.submit([this]() {
        // Notify the observers of the single settting with value of setting.
        m_settingsStorage->clearDatabase();
        m_mapOfSettingsAttributes.clear();
    });
    result.wait();
}

Settings::Settings(
    std::shared_ptr<SettingsStorageInterface> settingsStorage,
    std::unordered_set<std::shared_ptr<GlobalSettingsObserverInterface>> globalSettingsObserver,
    std::shared_ptr<registrationManager::CustomerDataManager> dataManager) :
        CustomerDataHandler{dataManager},
        m_settingsStorage{settingsStorage},
        m_globalSettingsObserver{globalSettingsObserver},
        m_sendDefaultSettings{false} {
}
}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
