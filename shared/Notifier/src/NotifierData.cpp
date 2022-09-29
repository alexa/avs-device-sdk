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

#include <acsdk/Notifier/private/NotifierData.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
/// @ingroup Lib_acsdkNotifier
/// @private
#define TAG "NotifierData"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @ingroup Lib_acsdkNotifier
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Helper to invoke function and catch exceptions.
 *
 * Method invokes function @a fn with argument @a ptr if @a fn is not empty, and @a ptr is not nullptr. If exception
 * is thrown during callback, it is logged.
 *
 * @param[in] fn   Function to invoke.
 * @param[in] ptr  Argument to use for function invocation.
 *
 * @ingroup Lib_acsdkNotifier
 * @private
 */
static void safeInvoke(std::function<void(const std::shared_ptr<void>&)>& fn, const std::shared_ptr<void>& ptr) {
    if (!ptr || !fn) {
        return;
    }

#if __cpp_exceptions || defined(__EXCEPTIONS)
    try {
#endif
        fn(ptr);
#if __cpp_exceptions || defined(__EXCEPTIONS)
    } catch (const std::exception& ex) {
        ACSDK_ERROR(LX(__func__).d("taskException", ex.what()));
    } catch (...) {
        ACSDK_ERROR(LX(__func__).d("taskException", "other"));
    }
#endif
}

std::unique_ptr<DataInterface> createDataInterface() {
    return std::unique_ptr<DataInterface>(new NotifierData);
}

NotifierData::NotifierData() noexcept : m_depth{0} {
}

void NotifierData::doAddStrongRefObserver(std::shared_ptr<void> observer) noexcept {
    doAddObserver(std::move(observer), ReferenceType::StrongRef);
}

void NotifierData::doAddWeakRefObserver(std::shared_ptr<void> observer) noexcept {
    doAddObserver(std::move(observer), ReferenceType::WeakRef);
}

void NotifierData::doAddObserver(std::shared_ptr<void> observer, ReferenceType type) noexcept {
    if (!observer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (isAlreadyExistLocked(observer)) {
        return;
    }
    m_observers.emplace_back(type, observer);

    if (m_addObserverFunc) {
        m_addObserverFunc(observer);
    }
}

void NotifierData::doRemoveObserver(std::shared_ptr<void> observer) noexcept {
    if (!observer) {
        ACSDK_WARN(LX(__func__).d("reason", "nullObserver"));
        return;
    }
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (m_depth > 0) {
        for (std::size_t ix = 0; ix < m_observers.size(); ix++) {
            auto& notifierObserver = m_observers[ix];
            if (notifierObserver.get() == observer) {
                notifierObserver.reset();
                break;
            }
        }
    } else {
        cleanupLocked(observer);
    }
}

void NotifierData::doNotifyObservers(std::function<void(const std::shared_ptr<void>&)> notify) noexcept {
    if (!notify) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullNotify"));
        return;
    }
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_depth++;
    for (std::size_t ix = 0; ix < m_observers.size(); ix++) {
        const auto& notifierObserver = m_observers[ix];
        auto observer = notifierObserver.get();
        safeInvoke(notify, observer);
    }
    if (0 == --m_depth) {
        cleanupLocked(nullptr);
    }
}

bool NotifierData::doNotifyObserversInReverse(std::function<void(const std::shared_ptr<void>&)> notify) noexcept {
    if (!notify) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullNotify"));
        return true;
    }
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_depth++;
    auto initialSize = m_observers.size();
    for (auto ix = initialSize; ix-- > 0;) {
        const auto& notifierObserver = m_observers[ix];
        auto observer = notifierObserver.get();
        safeInvoke(notify, observer);
    }
    auto result = m_observers.size() == initialSize;
    if (0 == --m_depth) {
        cleanupLocked(nullptr);
    }
    return result;
}

void NotifierData::doSetAddObserverFunction(
    std::function<void(const std::shared_ptr<void>&)> addObserverFunc) noexcept {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    const auto notifyAddedObservers = !m_addObserverFunc && addObserverFunc;
    m_addObserverFunc = std::move(addObserverFunc);
    if (notifyAddedObservers) {
        doNotifyObservers(m_addObserverFunc);
    }
}

void NotifierData::cleanupLocked(const std::shared_ptr<void>& unwanted) noexcept {
    auto ptr = unwanted.get();
    auto matcher = [ptr](const ObserverWrapper& notifierObserver) { return notifierObserver.isEqualOrExpired(ptr); };
    m_observers.erase(std::remove_if(m_observers.begin(), m_observers.end(), std::move(matcher)), m_observers.end());
}

bool NotifierData::isAlreadyExistLocked(const std::shared_ptr<void>& observer) noexcept {
    auto matcher = [&observer](const ObserverWrapper& arg) { return arg.get() == observer; };
    return std::find_if(m_observers.begin(), m_observers.end(), std::move(matcher)) != m_observers.end();
}

}  // namespace notifier
}  // namespace alexaClientSDK
