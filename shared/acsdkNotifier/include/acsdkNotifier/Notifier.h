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

#ifndef ACSDKNOTIFIER_NOTIFIER_H_
#define ACSDKNOTIFIER_NOTIFIER_H_

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include <acsdkNotifierInterfaces/NotifierInterface.h>

namespace alexaClientSDK {
namespace acsdkNotifier {

/**
 * Notifier maintains a set of observers that are notified with a caller defined function.
 *
 * @tparam ObserverType The type of observer notified by the template instantiation.
 */
template <typename ObserverType>
class Notifier : public acsdkNotifierInterfaces::NotifierInterface<ObserverType> {
public:
    /**
     * Constructor.
     */
    Notifier();

    /// @name NotifierInterface methods
    /// @{
    void addObserver(const std::shared_ptr<ObserverType>& observer) override;
    void removeObserver(const std::shared_ptr<ObserverType>& observer) override;
    void notifyObservers(std::function<void(const std::shared_ptr<ObserverType>&)> notify) override;
    bool notifyObserversInReverse(std::function<void(const std::shared_ptr<ObserverType>&)> notify) override;
    void setAddObserverFunction(std::function<void(const std::shared_ptr<ObserverType>&)> addObserverFunc) override;
    /// @}

private:
    /**
     * Eliminate the unwanted observer value from @c m_observers.
     *
     * @param unwanted The unwanted observer value.
     */
    void cleanupLocked(const std::shared_ptr<ObserverType>& unwanted);

    /// Mutex to serialize access to m_depth and m_observers.  Note that a recursive mutex is used
    /// here to avoid undefined behavior if notifying an observer callback in to this @c Notifier.
    std::recursive_mutex m_mutex;

    /// Depth of calls to @c notifyObservers() and notifyObserversInReverse().
    int m_depth;

    /// The set of observers.  Note that a vector is used here to allow for the addition
    /// or removal of observers while calls to notifyObservers() are in progress.
    std::vector<std::shared_ptr<ObserverType>> m_observers;

    /// If set, this function will be called after an observer is added.
    std::function<void(const std::shared_ptr<ObserverType>&)> m_addObserverFunc;
};

template <typename ObserverType>
inline Notifier<ObserverType>::Notifier() : m_depth{0} {
}

template <typename ObserverType>
inline void Notifier<ObserverType>::addObserver(const std::shared_ptr<ObserverType>& observer) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    for (auto& existing : m_observers) {
        if (observer == existing) {
            return;
        }
    }
    m_observers.push_back(observer);

    if (m_addObserverFunc) {
        m_addObserverFunc(observer);
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::removeObserver(const std::shared_ptr<ObserverType>& observer) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (m_depth > 0) {
        for (size_t ix = 0; ix < m_observers.size(); ix++) {
            if (m_observers[ix] == observer) {
                m_observers[ix] = nullptr;
                break;
            }
        }
    } else {
        cleanupLocked(observer);
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::notifyObservers(std::function<void(const std::shared_ptr<ObserverType>&)> notify) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_depth++;
    for (size_t ix = 0; ix < m_observers.size(); ix++) {
        auto observer = m_observers[ix];
        if (observer) {
            notify(observer);
        }
    }
    if (0 == --m_depth) {
        cleanupLocked(nullptr);
    }
}

template <typename ObserverType>
inline bool Notifier<ObserverType>::notifyObserversInReverse(
    std::function<void(const std::shared_ptr<ObserverType>&)> notify) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_depth++;
    auto initialSize = m_observers.size();
    for (auto ix = initialSize; ix-- > 0;) {
        auto observer = m_observers[ix];
        if (observer) {
            notify(observer);
        }
    }
    bool result = m_observers.size() == initialSize;
    if (0 == --m_depth) {
        cleanupLocked(nullptr);
    }
    return result;
}

template <typename ObserverType>
inline void Notifier<ObserverType>::setAddObserverFunction(
    std::function<void(const std::shared_ptr<ObserverType>&)> addObserverFunc) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_addObserverFunc = addObserverFunc;
}

template <typename ObserverType>
inline void Notifier<ObserverType>::cleanupLocked(const std::shared_ptr<ObserverType>& unwanted) {
    m_observers.erase(
        std::remove_if(
            m_observers.begin(),
            m_observers.end(),
            [unwanted](std::shared_ptr<ObserverType> observer) { return observer == unwanted; }),
        m_observers.end());
}

}  // namespace acsdkNotifier
}  // namespace alexaClientSDK

#endif  // ACSDKNOTIFIER_NOTIFIER_H_
