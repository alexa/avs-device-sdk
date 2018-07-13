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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGS_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGS_H_

#include <memory>
#include <unordered_set>
#include <unordered_map>
#include <vector>

#include <AVSCommon/SDKInterfaces/GlobalSettingsObserverInterface.h>
#include <AVSCommon/SDKInterfaces/SingleSettingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManager.h>
#include "Settings/SettingsStorageInterface.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace settings {

/**
 * This class implements Settings Interface to manage Alexa Settings on the product.
 *
 * This class writes the Setting change to database and notifies the observers of the setting.
 * @see https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/settings
 */
class Settings : public registrationManager::CustomerDataHandler {
public:
    /**
     * Creates a new @c Settings instance.
     * @param settingsStorage An interface to store, load, modify and delete Settings.
     * @param globalSettingsObserver A set of SettingsGlobalObserver which are notified when all the settings are
     * changed.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     * @return An instance of Settings if construction is successful or nullptr if construction fails.
     */
    static std::shared_ptr<Settings> create(
        std::shared_ptr<SettingsStorageInterface> settingsStorage,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::GlobalSettingsObserverInterface>> observers,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager);

    /**
     * Add an observer for a single setting mapped to the setting key.
     * @param key The name of the setting to which an observer is mapped to.
     * @param settingObserver The observer to be added to @c m_mapOfSettingsAttributes.
     */
    void addSingleSettingObserver(
        const std::string& key,
        std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface> settingsObserver);

    /**
     * Remove the observer for a single setting mapped to the setting key.
     * @param key The name of the setting to which an observer is mapped to.
     * @param settingObserver The observer to be removed from @c m_mapOfSettingsAttributes.
     */
    void removeSingleSettingObserver(
        const std::string& key,
        std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface> settingObserver);

    /**
     * Add the observer from the @c m_globalSettingsObserver.
     * @param settingsGlobalObserver The observer to be added to @c m_globalSettingsObserver.
     */
    void addGlobalSettingsObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::GlobalSettingsObserverInterface> globalSettingsObserver);

    /**
     * Remove the observer from the @c m_globalSettingsObserver.
     * @param settingsGlobalObserver The observer to be removed from @c m_globalSettingsObserver.
     */
    void removeGlobalSettingsObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::GlobalSettingsObserverInterface> globalSettingsObserver);

    /**
     * Function called by the application when a Setting is changed. It calls @c executeChangeSetting via
     * the executor.
     *
     * @param key The name of the setting which is changed.
     * @param value The new value of the setting.
     * @return A future which is @c true if the setting is changed successfully else @c false.
     */
    std::future<bool> changeSetting(const std::string& key, const std::string& value);

    /**
     * Function which sends the default settings to AVS if the settings do not already exist in the database.
     * If the settings already exist, the event is not sent.
     */
    void sendDefaultSettings();

    /**
     * Clears the settings storage  and attributes
     */
    void clearData() override;

private:
    /**
     * The structure to hold all the data of a single setting.
     *
     * @param valueOfSetting The value of the setting.
     * @param singleSettingObservers The set of observers for the setting.
     */
    struct SettingElements {
        std::string valueOfSetting;
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::SingleSettingObserverInterface>>
            singleSettingObservers;
    };

    /**
     * Initializes the database and for the settings object.
     */
    bool initialize();

    /**
     * Constructor
     *
     * @param settingsStorage An interface to store, load, modify and delete Settings.
     * @param globalSettingsObserver A set of SettingsGlobalObserver which are notified when all the settings are
     * changed.
     * @param dataManager A dataManager object that will track the CustomerDataHandler.
     */
    Settings(
        std::shared_ptr<SettingsStorageInterface> settingsStorage,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::GlobalSettingsObserverInterface>> observers,
        std::shared_ptr<registrationManager::CustomerDataManager> dataManager);

    /**
     * Function which implements the setting change. The function writes to the database and notifies the
     * observers of the setting.
     *
     * @param key The name of the setting which is changed.
     * @param value The new value of the setting.
     * @return A boolean which is @c true if the setting is changed successfully else @c false.
     */
    bool executeChangeSetting(const std::string& key, const std::string& value);

    /// The SettingsStorage object.
    std::shared_ptr<SettingsStorageInterface> m_settingsStorage;
    /// A set of observers which observe the map of settings.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::GlobalSettingsObserverInterface>>
        m_globalSettingsObserver;
    /// The map of <key, SettingElements> pairs of the settings.
    std::unordered_map<std::string, SettingElements> m_mapOfSettingsAttributes;
    /// Executor that queues up the calls when a setting is changed.
    avsCommon::utils::threading::Executor m_executor;

    /**
     * Variable which indicates whether to send the default Settings to AVS or not.
     * If the setting does not exist in the database, sendDefaultSetting is set to true,
     * and SettingsUpdated event is sent.
     */
    bool m_sendDefaultSettings;
};
}  // namespace settings
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_SETTINGS_INCLUDE_SETTINGS_SETTINGS_H_
