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

#include <algorithm>
#include <atomic>
#include <limits>

#include <AVSCommon/Utils/Error/FinallyGuard.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "Settings/SettingStringConversion.h"
#include "Settings/Types/LocaleWakeWordsSetting.h"

/// String to identify log entries originating from this file.
#define TAG "LocaleWakeWordsSetting"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace settings {
namespace types {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::error;
using namespace avsCommon::utils::json;

/// Key used for locale.
static const std::string LOCALE_KEY = "System.locales";

/// Key used for locale.
static const std::string WAKE_WORDS_KEY = "SpeechRecognizer.wakeWords";

/// Default wake word.
static const WakeWords DEFAULT_WAKE_WORDS{"ALEXA"};

/// The index of the primary locale.
static constexpr size_t PRIMARY_LOCALE_INDEX = 0;

/**
 * Convert wakewords to the expected json.
 *
 * @param wakeWords The wake words to be converted to string.
 * @return The wake words string representation.
 */
static std::string toJsonString(const WakeWords& wakeWords) {
    return toSettingString<WakeWords>(wakeWords).second;
}

/**
 * Utility function to send ChangedEvent for local changes and Report for AVS requested changes.
 *
 * @param sender The sender used to send the event to AVS.
 * @param status The current setting status.
 * @param jsonString The json string representing the value to be sent to AVS.
 * @return true if event was sent successfully; @c false otherwise.
 */
static bool sendEvent(
    const std::shared_ptr<SettingEventSenderInterface>& sender,
    SettingStatus status,
    const std::string& jsonString) {
    switch (status) {
        case SettingStatus::LOCAL_CHANGE_IN_PROGRESS:
        case SettingStatus::NOT_AVAILABLE:
            return sender->sendChangedEvent(jsonString).get();
        case SettingStatus::AVS_CHANGE_IN_PROGRESS:
            return sender->sendReportEvent(jsonString).get();
        case SettingStatus::SYNCHRONIZED:
            return false;
    }
    return false;
}

/**
 * Create a request type given the setting status.
 *
 * @param status The setting status that will determine the type of request.
 * @return The request type expected given @c status.
 */
static LocaleWakeWordsSetting::RequestType toRequestType(SettingStatus status) {
    switch (status) {
        case SettingStatus::LOCAL_CHANGE_IN_PROGRESS:
            return LocaleWakeWordsSetting::RequestType::LOCAL;
        case SettingStatus::AVS_CHANGE_IN_PROGRESS:
            return LocaleWakeWordsSetting::RequestType::AVS;
        case SettingStatus::NOT_AVAILABLE:
            return LocaleWakeWordsSetting::RequestType::LOCAL;
        case SettingStatus::SYNCHRONIZED:
            return LocaleWakeWordsSetting::RequestType::NONE;
    }
    return LocaleWakeWordsSetting::RequestType::NONE;
}

/**
 * Convert json to wake words.
 *
 * @param json The wake words json string representation.
 * @return The wake words retrieved from the json string.
 */
static WakeWords toWakeWords(const std::string& jsonValue) {
    return settings::fromSettingString<WakeWords>(jsonValue, WakeWords()).second;
}

/**
 * Thread safe method used to get the next request id.
 * @return The next id.
 */
static LocaleWakeWordsSetting::RequestId nextId() {
    static std::atomic<LocaleWakeWordsSetting::RequestId> m_requestCounter{0};
    return ++m_requestCounter;
}

/**
 * Get primary locale.
 *
 * @param locales A collection of locales.
 * @return The primary locale from this collection.
 */
static Locale getPrimary(const DeviceLocales& locales) {
    if (locales.empty()) {
        return "";
    }
    return locales[PRIMARY_LOCALE_INDEX];
}

std::shared_ptr<LocaleWakeWordsSetting> LocaleWakeWordsSetting::create(
    std::shared_ptr<SettingEventSenderInterface> localeEventSender,
    std::shared_ptr<SettingEventSenderInterface> wakeWordsEventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> assetsManager) {
    ACSDK_DEBUG5(LX(__func__).d("settingName", "LocaleWakeWords"));

    if (!localeEventSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullLocaleEventSender"));
        return nullptr;
    }

    if (!wakeWordsEventSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullWakeWordsEventSender"));
        return nullptr;
    }

    if (!settingStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullSettingStorage"));
        return nullptr;
    }

    if (!assetsManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAssetsManager"));
        return nullptr;
    }

    auto setting = std::shared_ptr<LocaleWakeWordsSetting>(
        new LocaleWakeWordsSetting(localeEventSender, wakeWordsEventSender, settingStorage, assetsManager));

    setting->restoreInitialValue();

    return setting;
}

LocaleWakeWordsSetting::~LocaleWakeWordsSetting() {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_pendingRequest) {
        m_assetsManager->cancelOngoingChange();
    }
}

SetSettingResult LocaleWakeWordsSetting::setLocales(const DeviceLocales& locales, RequestType requestType) {
    const auto supportedLocales = m_assetsManager->getSupportedLocales();
    for (const auto& locale : locales) {
        if (supportedLocales.find(locale) == supportedLocales.end()) {
            ACSDK_ERROR(LX("setLocalChangeFailed")
                            .d("reason", "unsupportedLocale")
                            .d("locale", locale)
                            .d("supported", toJsonString(supportedLocales)));
            return SetSettingResult::INVALID_VALUE;
        }
    }

    // make sure the locale combination is valid as well.
    if (locales.size() > 1) {
        const auto supportedLocaleCombinations = m_assetsManager->getSupportedLocaleCombinations();
        if (supportedLocaleCombinations.find(locales) == supportedLocaleCombinations.end()) {
            ACSDK_ERROR(LX("setLocalChangeFailed")
                            .d("reason", "unsupportedLocaleCombination")
                            .d("locales", toSettingString<DeviceLocales>(locales).second));
            return SetSettingResult::INVALID_VALUE;
        }
    }

    ACSDK_INFO(LX(__func__).d("locales", toSettingString<DeviceLocales>(locales).second));
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_pendingRequest) {
        if (m_pendingRequest->locales == locales) {
            ACSDK_DEBUG5(LX(__func__)
                             .d("result", "changeAlreadyPending")
                             .d("locale", toSettingString<DeviceLocales>(locales).second));
            return SetSettingResult::NO_CHANGE;
        }
        m_assetsManager->cancelOngoingChange();
    }

    // If the changed is done locally and the value being set did not change, then we are done.
    // If the changed is done via AVS, we still need to respond with an event even though the value is the same.
    if ((RequestType::AVS != requestType) && (locales == LocalesSetting::get())) {
        ACSDK_DEBUG5(LX(__func__)
                         .d("result", "requestValueAlreadyApplied")
                         .d("locale", toSettingString<DeviceLocales>(locales).second));
        return SetSettingResult::NO_CHANGE;
    }

    WakeWords wakeWords;
    bool allSupported = false;
    auto wakeWordsRequestType = RequestType::NONE;
    if (m_pendingRequest && (m_pendingRequest->wakeWordsRequestType != RequestType::NONE)) {
        wakeWordsRequestType = m_pendingRequest->wakeWordsRequestType;
        std::tie(allSupported, wakeWords) = supportedWakeWords(locales, m_pendingRequest->wakeWords);
    } else {
        std::tie(allSupported, wakeWords) = supportedWakeWords(locales, WakeWordsSetting::get());
    }

    if (!allSupported) {
        wakeWordsRequestType = RequestType::LOCAL;
        if (wakeWords.empty()) {
            wakeWords = DEFAULT_WAKE_WORDS;
        }
    }

    // If this is a request from AVS, set the state to AVS_CHANGE_IN_PROGRESS to ensure that any disruption while
    // applying the setting will ensure that an event to AVS will be sent in response.
    if (RequestType::AVS == requestType) {
        m_localeStatus = SettingStatus::AVS_CHANGE_IN_PROGRESS;
        if (!m_storage->updateSettingStatus(LOCALE_KEY, m_localeStatus)) {
            ACSDK_WARN(LX(__func__).d("reason", "storageUpdateFailed"));
        }
    }

    RequestParameters request{requestType, locales, wakeWordsRequestType, wakeWords};
    m_pendingRequest.reset(new RequestParameters(request));
    m_executor.execute([this, request] { executeChangeValue(request); });
    return SetSettingResult::ENQUEUED;
}

SetSettingResult LocaleWakeWordsSetting::setWakeWords(const WakeWords& wakeWords, RequestType requestType) {
    ACSDK_INFO(LX(__func__).d("wakeWords", toJsonString(wakeWords)));
    DeviceLocales locales;
    auto localeRequestType = RequestType::NONE;

    if (wakeWords.empty()) {
        ACSDK_ERROR(LX("setWakeWordsFailed").d("reason", "requireAtLeastOneWakeWord"));
        return SetSettingResult::INVALID_VALUE;
    }

    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_pendingRequest) {
        if (m_pendingRequest->wakeWords == wakeWords) {
            ACSDK_DEBUG5(LX(__func__).d("result", "changeAlreadyPending").d("wakeWords", toJsonString(wakeWords)));
            return SetSettingResult::NO_CHANGE;
        }
        m_assetsManager->cancelOngoingChange();
    }

    // If the changed is done locally and the value being set did not change, then we are done.
    // If the changed is done via AVS, we still need to respond with an event even though the value is the same.
    if ((RequestType::AVS != requestType) && (wakeWords == WakeWordsSetting::get())) {
        ACSDK_DEBUG5(LX(__func__).d("result", "requestValueAlreadyApplied").d("wakeWords", toJsonString(wakeWords)));
        return SetSettingResult::NO_CHANGE;
    }

    if (m_pendingRequest && (m_pendingRequest->localeRequestType != RequestType::NONE)) {
        localeRequestType = m_pendingRequest->localeRequestType;
        locales = m_pendingRequest->locales;
    } else {
        locales = LocalesSetting::get();
    }

    if (!supportedWakeWords(locales, wakeWords).first) {
        ACSDK_ERROR(LX("setAvsWakeWordsChangeFailed")
                        .d("reason", "unsupportedWakeWords")
                        .d("wakeWords", toJsonString(wakeWords))
                        .d("locale", getPrimary(locales)));
        return SetSettingResult::INVALID_VALUE;
    }

    // If this is a request from AVS, set the state to AVS_CHANGE_IN_PROGRESS to ensure that any disruption while
    // applying the setting will ensure that an event to AVS will be sent in response.
    if (RequestType::AVS == requestType) {
        m_wakeWordsStatus = SettingStatus::AVS_CHANGE_IN_PROGRESS;
        if (!m_storage->updateSettingStatus(WAKE_WORDS_KEY, m_wakeWordsStatus)) {
            ACSDK_WARN(LX(__func__).d("reason", "storageUpdateFailed"));
        }
    }

    RequestParameters request{localeRequestType, locales, requestType, wakeWords};
    m_pendingRequest.reset(new RequestParameters(request));
    m_executor.execute([this, request] { executeChangeValue(request); });
    return SetSettingResult::ENQUEUED;
}

SetSettingResult LocaleWakeWordsSetting::setLocalChange(const WakeWords& wakeWords) {
    return setWakeWords(wakeWords, RequestType::LOCAL);
}

SetSettingResult LocaleWakeWordsSetting::setLocalChange(const DeviceLocales& locales) {
    return setLocales(locales, RequestType::LOCAL);
}

static bool returnValueFromSetSettingResult(const SetSettingResult& status) {
    switch (status) {
        case SetSettingResult::NO_CHANGE:
        // fall-through
        case SetSettingResult::ENQUEUED:
            return true;

        case SetSettingResult::BUSY:
        // fall-through
        case SetSettingResult::UNAVAILABLE_SETTING:
        // fall-through
        case SetSettingResult::INVALID_VALUE:
        // fall-through
        case SetSettingResult::INTERNAL_ERROR:
        // fall-through
        case SetSettingResult::UNSUPPORTED_OPERATION:
            return false;
    }

    return false;
}

bool LocaleWakeWordsSetting::setAvsChange(const DeviceLocales& locales) {
    auto status = setLocales(locales, RequestType::AVS);
    return returnValueFromSetSettingResult(status);
}

bool LocaleWakeWordsSetting::setAvsChange(const WakeWords& wakeWords) {
    auto status = setWakeWords(wakeWords, RequestType::AVS);
    return returnValueFromSetSettingResult(status);
}

bool LocaleWakeWordsSetting::clearData(const DeviceLocales& locales) {
    ACSDK_DEBUG5(LX(__func__).d("setting", LOCALE_KEY));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_pendingRequest.reset();
    m_localeStatus = SettingStatus::NOT_AVAILABLE;
    LocalesSetting::m_value = locales;
    // Clear customer's data before restoring the initial value
    auto result = m_storage->deleteSetting(LOCALE_KEY);
    if (result) {
        // As m_localeStatus == SettingStatus::NOT_AVAILABLE restoreInitialValue()
        // calls LocalesSetting::get() which returns LocalesSetting::m_value
        restoreInitialValue();
    }
    return result;
}

bool LocaleWakeWordsSetting::clearData(const WakeWords& wakeWords) {
    ACSDK_DEBUG5(LX(__func__).d("setting", WAKE_WORDS_KEY));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_pendingRequest.reset();
    m_wakeWordsStatus = SettingStatus::NOT_AVAILABLE;
    WakeWordsSetting::m_value = wakeWords;
    // Clear customer's data before restoring the initial value
    auto result = m_storage->deleteSetting(WAKE_WORDS_KEY);
    if (result) {
        // As m_wakeWordsStatus == SettingStatus::NOT_AVAILABLE restoreInitialValue()
        // calls WakeWordsSetting::get() which returns WakeWordsSetting::m_value
        restoreInitialValue();
    }
    return result;
}

void LocaleWakeWordsSetting::restoreInitialValue() {
    std::string localeJsonValue;
    std::tie(m_localeStatus, localeJsonValue) = m_storage->loadSetting(LOCALE_KEY);
    if (m_localeStatus != SettingStatus::NOT_AVAILABLE) {
        LocalesSetting::m_value = fromSettingString<DeviceLocales>(localeJsonValue, LocalesSetting::get()).second;
    }

    if (m_assetsManager->getDefaultSupportedWakeWords().empty()) {
        // No wake words are supported by this device so there's no need to restore its value.
        m_wakeWordsStatus = SettingStatus::SYNCHRONIZED;
        WakeWordsSetting::m_value = WakeWords();
    } else {
        std::string wakeWordsJsonValue;
        std::tie(m_wakeWordsStatus, wakeWordsJsonValue) = m_storage->loadSetting(WAKE_WORDS_KEY);
        if (m_wakeWordsStatus != SettingStatus::NOT_AVAILABLE) {
            WakeWordsSetting::m_value = toWakeWords(wakeWordsJsonValue);
        }
    }

    DeviceLocales locales = LocalesSetting::get();
    WakeWords wakeWords = WakeWordsSetting::get();
    ACSDK_DEBUG2(LX(__func__)
                     .d("wakeWords", toJsonString(wakeWords))
                     .d("locale", toSettingString<DeviceLocales>(locales).second));
    if (m_localeStatus != SettingStatus::SYNCHRONIZED || m_wakeWordsStatus != SettingStatus::SYNCHRONIZED) {
        // If not synchronized, apply changes and synchronize.
        RequestType localeRequestType = toRequestType(m_localeStatus);
        RequestType wakeWordsRequestType = toRequestType(m_wakeWordsStatus);
        auto pendingRequest = RequestParameters(localeRequestType, locales, wakeWordsRequestType, wakeWords);
        m_pendingRequest.reset(new RequestParameters(pendingRequest));
        m_executor.execute([this, pendingRequest] { executeChangeValue(pendingRequest); });
    } else {
        m_executor.execute([this, locales, wakeWords] {
            // Make sure assets manager is correctly initialized.
            if (!m_assetsManager->changeAssets(locales, wakeWords)) {
                ACSDK_ERROR(LX("restoreInitialValueFailed")
                                .d("reason", "unableToRestoreAssetsManager")
                                .d("locale", toSettingString<DeviceLocales>(locales).second)
                                .d("wakeWords", toJsonString(wakeWords)));
            }
        });
    }
}

LocaleWakeWordsSetting::LocaleWakeWordsSetting(
    std::shared_ptr<SettingEventSenderInterface> localeEventSender,
    std::shared_ptr<SettingEventSenderInterface> wakeWordsEventSender,
    std::shared_ptr<storage::DeviceSettingStorageInterface> settingStorage,
    std::shared_ptr<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface> assetsManager) :
        LocalesSetting{assetsManager->getDefaultLocales()},
        WakeWordsSetting{DEFAULT_WAKE_WORDS},
        m_localeEventSender{localeEventSender},
        m_wakeWordsEventSender{wakeWordsEventSender},
        m_storage{settingStorage},
        m_localeStatus{SettingStatus::NOT_AVAILABLE},
        m_wakeWordsStatus{SettingStatus::NOT_AVAILABLE},
        m_assetsManager{assetsManager} {
}

void LocaleWakeWordsSetting::synchronize(const RequestParameters& request) {
    if (m_wakeWordsStatus != SettingStatus::SYNCHRONIZED) {
        synchronizeWakeWords(request);
    }

    if (m_localeStatus != SettingStatus::SYNCHRONIZED) {
        synchronizeLocale(request);
    }
}

void LocaleWakeWordsSetting::synchronizeWakeWords(const RequestParameters& request) {
    if (sendEvent(m_wakeWordsEventSender, m_wakeWordsStatus, toJsonString(WakeWordsSetting::get()))) {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (isLatestRequestLocked(request)) {
            // Store SYNCHRONIZED in the database if no other request is in the queue.
            m_wakeWordsStatus = SettingStatus::SYNCHRONIZED;
            if (!this->m_storage->updateSettingStatus(WAKE_WORDS_KEY, m_wakeWordsStatus)) {
                ACSDK_ERROR(LX("synchronizeWakeWordsFailed").d("reason", "cannotUpdateWakeWord"));
            }
        }
    } else {
        ACSDK_ERROR(LX("synchronizeWakeWordsFailed").d("reason", "sendEventFailed"));
    }
}

void LocaleWakeWordsSetting::synchronizeLocale(const RequestParameters& request) {
    if (sendEvent(m_localeEventSender, m_localeStatus, toSettingString<DeviceLocales>(LocalesSetting::get()).second)) {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (isLatestRequestLocked(request)) {
            // Store SYNCHRONIZED in the database if no other request is in the queue.
            m_localeStatus = SettingStatus::SYNCHRONIZED;
            if (!this->m_storage->updateSettingStatus(LOCALE_KEY, m_localeStatus)) {
                ACSDK_ERROR(LX("synchronizeLocaleFailed").d("reason", "cannotUpdateLocale"));
            }
        }
    } else {
        ACSDK_ERROR(LX("synchronizeLocaleFailed").d("reason", "sendEventFailed"));
    }
}

void LocaleWakeWordsSetting::handleFailure(const RequestParameters& request) {
    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!isLatestRequestLocked(request)) {
            // Stop immediately if a new request is already in the queue.
            notifyObserversOfCancellationLocked(request);
            return;
        }
    }

    // Notify observers of failure.
    ACSDK_ERROR(LX(__func__)
                    .d("wakeWords", toJsonString(request.wakeWords))
                    .d("locale", toSettingString<DeviceLocales>(request.locales).second));
    notifyObserversOfFailure(request);

    // Send report if we have a pending report.
    synchronize(request);
}

bool LocaleWakeWordsSetting::storeValues(const RequestParameters& request) {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (isLatestRequestLocked(request)) {
        std::vector<std::tuple<std::string, std::string, SettingStatus>> dbValues;

        auto localeStatus = m_localeStatus;
        auto wakeWordsStatus = m_wakeWordsStatus;
        if (request.localeRequestType != RequestType::NONE) {
            localeStatus = (LocaleWakeWordsSetting::RequestType::AVS == request.localeRequestType)
                               ? SettingStatus::AVS_CHANGE_IN_PROGRESS
                               : SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
            dbValues.push_back(
                std::make_tuple(LOCALE_KEY, toSettingString<DeviceLocales>(request.locales).second, localeStatus));
        }

        if (request.wakeWordsRequestType != RequestType::NONE) {
            wakeWordsStatus = (LocaleWakeWordsSetting::RequestType::AVS == request.wakeWordsRequestType)
                                  ? SettingStatus::AVS_CHANGE_IN_PROGRESS
                                  : SettingStatus::LOCAL_CHANGE_IN_PROGRESS;
            dbValues.push_back(std::make_tuple(WAKE_WORDS_KEY, toJsonString(request.wakeWords), wakeWordsStatus));
        }

        if (!dbValues.empty()) {
            if (!this->m_storage->storeSettings(dbValues)) {
                ACSDK_ERROR(LX("storeValueFailed").d("reason", "cannotSaveLocaleWakeWord"));
                return false;
            }

            if (request.localeRequestType != RequestType::NONE) {
                LocalesSetting::m_value = request.locales;
                m_localeStatus = localeStatus;
            }

            if (request.wakeWordsRequestType != RequestType::NONE) {
                WakeWordsSetting::m_value = request.wakeWords;
                m_wakeWordsStatus = wakeWordsStatus;
            }
        }
    }

    return true;
}

void LocaleWakeWordsSetting::notifyObserversOfChangeInProgress(const RequestParameters& request) {
    if (request.localeRequestType != RequestType::NONE) {
        LocalesSetting::notifyObservers(
            RequestType::AVS == request.localeRequestType ? SettingNotifications::AVS_CHANGE_IN_PROGRESS
                                                          : SettingNotifications::LOCAL_CHANGE_IN_PROGRESS);
    }

    if (request.wakeWordsRequestType != RequestType::NONE) {
        WakeWordsSetting::notifyObservers(
            RequestType::AVS == request.wakeWordsRequestType ? SettingNotifications::AVS_CHANGE_IN_PROGRESS
                                                             : SettingNotifications::LOCAL_CHANGE_IN_PROGRESS);
    }
}

void LocaleWakeWordsSetting::notifyObserversOfCancellationLocked(const RequestParameters& request) {
    ACSDK_DEBUG5(LX(__func__).d("id", request.id).d("pendingId", m_pendingRequest ? m_pendingRequest->id : -1));
    if (request.localeRequestType != RequestType::NONE && (request.locales != m_pendingRequest->locales)) {
        auto notification = (RequestType::AVS == request.localeRequestType)
                                ? SettingNotifications::AVS_CHANGE_CANCELLED
                                : SettingNotifications::LOCAL_CHANGE_CANCELLED;
        LocalesSetting::notifyObservers(notification);
    }

    if ((request.wakeWordsRequestType != RequestType::NONE) && (request.wakeWords != m_pendingRequest->wakeWords)) {
        auto notification = (RequestType::AVS == request.wakeWordsRequestType)
                                ? SettingNotifications::AVS_CHANGE_CANCELLED
                                : SettingNotifications::LOCAL_CHANGE_CANCELLED;
        WakeWordsSetting::notifyObservers(notification);
    }
}

void LocaleWakeWordsSetting::notifyObserversOfFailure(const RequestParameters& request) {
    ACSDK_DEBUG5(LX(__func__).d("id", request.id));
    if (request.localeRequestType != RequestType::NONE) {
        auto notification = (RequestType::AVS == request.localeRequestType) ? SettingNotifications::AVS_CHANGE_FAILED
                                                                            : SettingNotifications::LOCAL_CHANGE_FAILED;
        LocalesSetting::notifyObservers(notification);
    }

    if ((request.wakeWordsRequestType != RequestType::NONE)) {
        auto notification = (RequestType::AVS == request.wakeWordsRequestType)
                                ? SettingNotifications::AVS_CHANGE_FAILED
                                : SettingNotifications::LOCAL_CHANGE_FAILED;
        WakeWordsSetting::notifyObservers(notification);
    }
}

void LocaleWakeWordsSetting::notifyObserversOfSuccess(const RequestParameters& request) {
    ACSDK_DEBUG5(LX(__func__).d("id", request.id));
    if (request.localeRequestType != RequestType::NONE) {
        auto notification = (RequestType::AVS == request.localeRequestType) ? SettingNotifications::AVS_CHANGE
                                                                            : SettingNotifications::LOCAL_CHANGE;
        LocalesSetting::notifyObservers(notification);
    }

    if ((request.wakeWordsRequestType != RequestType::NONE)) {
        auto notification = (RequestType::AVS == request.wakeWordsRequestType) ? SettingNotifications::AVS_CHANGE
                                                                               : SettingNotifications::LOCAL_CHANGE;
        WakeWordsSetting::notifyObservers(notification);
    }
}

bool LocaleWakeWordsSetting::isLatestRequestLocked(const RequestParameters& request) const {
    return !m_pendingRequest || (m_pendingRequest->id == request.id);
}

void LocaleWakeWordsSetting::clearPendingRequest(const RequestParameters& request) {
    std::lock_guard<std::mutex> lock{m_mutex};
    if (isLatestRequestLocked(request)) {
        m_pendingRequest.reset();
    }
}

void LocaleWakeWordsSetting::executeChangeValue(const RequestParameters& request) {
    ACSDK_DEBUG5(LX(__func__)
                     .d("RequestId", request.id)
                     .d("wwRequest", toJsonString(request.wakeWords))
                     .d("localeRequest", toSettingString<DeviceLocales>(request.locales).second));

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!isLatestRequestLocked(request)) {
            notifyObserversOfCancellationLocked(request);
            return;
        }
    }

    auto locales = (request.localeRequestType != RequestType::NONE) ? request.locales : LocalesSetting::get();
    auto wakeWords = (request.wakeWordsRequestType != RequestType::NONE) ? request.wakeWords : WakeWordsSetting::get();

    notifyObserversOfChangeInProgress(request);

    // Apply the change.
    bool ok = m_assetsManager->changeAssets(locales, wakeWords);

    // Clear pending request field by the end of this method.
    FinallyGuard finally{[this, &request] { clearPendingRequest(request); }};
    if (!ok) {
        ACSDK_ERROR(LX("changeAssetsFailed"));
        handleFailure(request);
        return;
    }

    if (!storeValues(request)) {
        m_assetsManager->changeAssets(LocalesSetting::get(), WakeWordsSetting::get());
        ACSDK_ERROR(LX("storeFailed"));
        handleFailure(request);
        return;
    }

    notifyObserversOfSuccess(request);

    synchronize(request);
}

void LocaleWakeWordsSetting::onConnectionStatusChanged(const Status status, const ChangedReason reason) {
    // Handle only transition to connected state.
    if (Status::CONNECTED == status) {
        // Create a dummy request that doesn't interrupt any ongoing operation but that respect newer requests.
        RequestParameters request(RequestType::NONE, DeviceLocales(), RequestType::NONE, WakeWords());
        m_executor.execute([this, request] { synchronize(request); });
    }
}

std::pair<bool, WakeWords> LocaleWakeWordsSetting::supportedWakeWords(
    const DeviceLocales& locales,
    const WakeWords& wakeWords) {
    auto supportedWakeWords = m_assetsManager->getSupportedWakeWords(getPrimary(locales));
    WakeWords intersection;
    if (supportedWakeWords.find(wakeWords) != supportedWakeWords.end()) {
        intersection = wakeWords;
    }
    return std::make_pair(intersection.size() == wakeWords.size(), intersection);
}

LocaleWakeWordsSetting::RequestParameters::RequestParameters(
    const LocaleWakeWordsSetting::RequestType requestTypeLocale,
    const DeviceLocales& localesValue,
    const LocaleWakeWordsSetting::RequestType requestTypeWakeWords,
    const WakeWords& wakeWordsValue) :
        id{nextId()},
        localeRequestType{requestTypeLocale},
        locales{localesValue},
        wakeWordsRequestType{requestTypeWakeWords},
        wakeWords{wakeWordsValue} {
}

}  // namespace types
}  // namespace settings
}  // namespace alexaClientSDK
