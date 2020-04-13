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
#ifndef ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_LOCALEWAKEWORDSSETTING_H_
#define ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_LOCALEWAKEWORDSSETTING_H_

#include <memory>
#include <mutex>

#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>
#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>

#include "Settings/DeviceSettingsManager.h"
#include "Settings/SetSettingResult.h"
#include "Settings/SettingEventMetadata.h"
#include "Settings/SettingEventSenderInterface.h"
#include "Settings/SettingObserverInterface.h"
#include "Settings/SettingStatus.h"
#include "Settings/Storage/DeviceSettingStorageInterface.h"

namespace alexaClientSDK {
namespace settings {
namespace types {

/**
 * Implements the interfaces to set Locale and WakeWords settings.
 */
class LocaleWakeWordsSetting
        : public LocalesSetting
        , public WakeWordsSetting
        , public avsCommon::sdkInterfaces::ConnectionStatusObserverInterface {
public:
    /// Alias for request id.
    using RequestId = unsigned int;

    /// Type of request.
    enum class RequestType : uint8_t {
        /// Local request.
        LOCAL,
        /// AVS request.
        AVS,
        /// No change required.
        NONE
    };

    /**
     * Create a LocaleWakeWordsSetting object.
     *
     * @param localeEventSender Object used to send events to AVS in order to report locale changes on the device.
     * @param wakeWordsEventSender Object used to send events to AVS in order to report wakeword changes on the device.
     * @param settingStorage The setting storage object.
     * @param assetsManager An asset manager that is responsible for managing locale and wakeword assets.
     * @return A pointer to the new object if it succeeds; @c nullptr otherwise.
     */
    static std::shared_ptr<LocaleWakeWordsSetting> create(
        std::shared_ptr<SettingEventSenderInterface> localeEventSender,
        std::shared_ptr<SettingEventSenderInterface> wakeWordsEventSender,
        std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> assetsManager);

    /// @name LocaleSetting methods.
    /// @{
    SetSettingResult setLocalChange(const DeviceLocales& locale) override;
    bool setAvsChange(const DeviceLocales& locale) override;
    bool clearData(const DeviceLocales& locale) override;
    /// @}

    /// @name WakeWordsSetting methods.
    /// @{
    SetSettingResult setLocalChange(const WakeWords& wakeWords) override;
    bool setAvsChange(const WakeWords& WakeWords) override;
    bool clearData(const WakeWords& WakeWords) override;
    /// @}

    /// @name ConnectionStatusObserverInterface methods.
    /// @{
    void onConnectionStatusChanged(const Status status, const ChangedReason reason) override;
    /// @}

    /**
     * Destructor
     */
    ~LocaleWakeWordsSetting();

private:
    /**
     * Contains parameters associated to a request to modify wakeword and/or locale settings.
     */
    struct RequestParameters {
        /// The id used to check if this request should be applied or cancelled.
        const RequestId id;

        /// The request type for locale.
        const RequestType localeRequestType;

        /// Requested locale. Use empty string if there's no change to locale.
        const DeviceLocales locales;

        /// The request type for wake word.
        const RequestType wakeWordsRequestType;

        /// Requested wake words. Use empty container if there's no change to the wake words.
        const WakeWords wakeWords;

        /**
         * Constructor.
         */
        RequestParameters(
            const RequestType localeRequestType,
            const DeviceLocales& locales,
            const RequestType wakeWordsRequestType,
            const WakeWords& wakeWords);
    };

    /**
     * Constructor.
     * @param localeEventSender Object used to send events to AVS in order to report locale changes on the device.
     * @param wakeWordsEventSender Object used to send events to AVS in order to report wakeword changes on the device.
     * @param settingStorage The setting storage object.
     * @param assetsManager An asset manager that is responsible for managing locale and wakeword assets.
     */
    LocaleWakeWordsSetting(
        std::shared_ptr<SettingEventSenderInterface> localeEventSender,
        std::shared_ptr<SettingEventSenderInterface> wakeWordsEventSender,
        std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
        std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> assetsManager);

    /**
     * Helper method to apply a locale setting.
     *
     * @param locale The locale value.
     * @param requestType The type of request.
     * @return The result of the set operation.
     */
    SetSettingResult setLocales(const DeviceLocales& locale, RequestType requestType);

    /**
     * Helper method to apply a wake words setting.
     *
     * @param wake words The wake word values.
     * @param requestType The type of request.
     * @return The result of the set operation.
     */
    SetSettingResult setWakeWords(const WakeWords& wakeWords, RequestType requestType);

    /**
     * Synchronize with AVS for the current setting value.
     *
     * @param request The parameters for this request.
     */
    void synchronize(const RequestParameters& request);

    /**
     * Synchronize the wake words setting to AVS.
     *
     * @param request The parameters for the current request.
     */
    void synchronizeWakeWords(const RequestParameters& request);

    /**
     * Synchronize the locale setting to AVS.
     *
     * @param request The parameters for the current request.
     */
    void synchronizeLocale(const RequestParameters& request);

    /**
     * Handle request failure. If request is the latest and status is AVS_CHANGE_IN_PROGRESS, send a report.
     *
     * @param request The parameters for this request.
     */
    void handleFailure(const RequestParameters& request);

    /**
     * Save value and status to the storage.
     *
     * @param request The parameters for this request.
     * @return @c true if it succeeds; @c false otherwise.
     */
    bool storeValues(const RequestParameters& request);

    /**
     * Execute the request @c requestId is the most recent one. If this is an old request, a
     * cancellation notification will be send using @c notifyObservers.
     *
     * @param request The parameters for this request.
     */
    void executeChangeValue(const RequestParameters& request);

    /// Functions used to notify observers of the request result.
    ///
    /// @param request The parameters for this request.
    /// @{
    void notifyObserversOfCancellationLocked(const RequestParameters& request);
    void notifyObserversOfFailure(const RequestParameters& request);
    void notifyObserversOfSuccess(const RequestParameters& request);
    void notifyObserversOfChangeInProgress(const RequestParameters& request);
    /// @}

    /**
     * Utility function used to check if the current request is the latest or not.
     *
     * @note Caller must hold access to @c m_mutex.
     * @param request The current request.
     * @return Whether @c request is the latest or not.
     */
    inline bool isLatestRequestLocked(const RequestParameters& request) const;

    /**
     * Function used during object creation to restore the initial value kept in the database.
     *
     */
    void restoreInitialValue();

    /**
     * Utility function used to clear the pending request (@c m_pendingRequest) if it matches the request processed.
     *
     * @param request The last request processed.
     */
    void clearPendingRequest(const RequestParameters& request);

    /**
     * Get the intersection between @c wakeWords set and the wake words supported for a given locale.
     *
     * @param locale The locale where the wake words should be valid.
     * @param wakeWords The set of wake words that will be tested against the supported wake words.
     * @return A pair where @c first is a boolean indicating whether all the wake words in the @c wake words setting.
     * are supported in the given @c locale. The @c second element is the wake words intersection.
     */
    std::pair<bool, WakeWords> supportedWakeWords(const DeviceLocales& locale, const WakeWords& wakeWords);

    /// Object used to send events to AVS in order to report changes to the locale.
    std::shared_ptr<SettingEventSenderInterface> m_localeEventSender;

    /// Object used to send events to AVS in order to report changes to the wake words.
    std::shared_ptr<SettingEventSenderInterface> m_wakeWordsEventSender;

    /// The setting storage object.
    std::shared_ptr<storage::DeviceSettingStorageInterface> m_storage;

    /// Mutex used to guard @c m_pendingRequest, @c m_wakeWordsStatus, @c m_localeStatus, and storage access.
    std::mutex m_mutex;

    /// Pointer to the last request.
    std::unique_ptr<RequestParameters> m_pendingRequest;

    /// The locale synchronization status.
    SettingStatus m_localeStatus;

    /// The wake words synchronization status.
    SettingStatus m_wakeWordsStatus;

    /// The locale assets manager which is responsible for effectively configure new locale and wake words.
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> m_assetsManager;

    /// Executor used to handle events in sequence.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace types
}  // namespace settings
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SETTINGS_INCLUDE_SETTINGS_TYPES_LOCALEWAKEWORDSSETTING_H_
