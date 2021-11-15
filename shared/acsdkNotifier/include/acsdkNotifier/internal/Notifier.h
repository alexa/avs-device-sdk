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

#ifndef ACSDKNOTIFIER_INTERNAL_NOTIFIER_H_
#define ACSDKNOTIFIER_INTERNAL_NOTIFIER_H_

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

#include <acsdkNotifierInterfaces/internal/NotifierInterface.h>

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
    void addWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) override;
    void removeWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) override;
    void notifyObservers(std::function<void(const std::shared_ptr<ObserverType>&)> notify) override;
    bool notifyObserversInReverse(std::function<void(const std::shared_ptr<ObserverType>&)> notify) override;
    void setAddObserverFunction(std::function<void(const std::shared_ptr<ObserverType>&)> addObserverFunc) override;
    /// @}

private:
    /**
     * Eliminate the unwanted observer value from @c m_observers.  This will also cleanup any @c weak_ptr observer that
     * has been expired.
     *
     * @param unwanted The unwanted observer value.
     */
    void cleanupLocked(const std::shared_ptr<ObserverType>& unwanted);

    /**
     * Checks if observer already exists in  @c m_observers.
     *
     * @param unwanted The observer value.
     * @return true if observer already exists, false otherwise.
     */
    bool isAlreadyExistLocked(const std::shared_ptr<ObserverType>& observer);

    /// A class for a Notifier observer
    class NotifierObserver {
    public:
        /**
         * Constructor.
         *
         * @param observer The @c std::shared_ptr observer.
         */
        explicit NotifierObserver(const std::shared_ptr<ObserverType>& observer);

        /**
         * Constructor.
         *
         * @param observer The @c std::weak_ptr observer.
         */
        explicit NotifierObserver(const std::weak_ptr<ObserverType>& observer);

        /**
         * Gets the observer.
         *
         * @return The observer as a @c std::shared_ptr.
         */
        std::shared_ptr<ObserverType> get() const;

        /**
         * Clears the observer.
         */
        void clear();

        /**
         * Checks if the notifier observer matches the observer that is passed in, or if the notifier observer (if it's
         * a weak_ptr observer) has expired.
         *
         * @param The observer to check against if it's equal.
         * @return true if observer is equal to notifier observer or if notifier observer has expired, false otherwise.
         */
        bool isEqualOrExpired(const std::shared_ptr<ObserverType>& observer) const;

    private:
        /// A enum type to specify the observer pointer type
        enum class ObserverPointerType {
            /// Observer is SHARED_PTR
            SHARED_PTR,
            /// Observer is WEAK_PTR
            WEAK_PTR
        };

        /// Type of observer
        ObserverPointerType m_type;
        /// shared_ptr of observer, if its type is SHARED_PTR
        std::shared_ptr<ObserverType> m_sharedPtrObserver;
        /// weak_ptr of observer, if its type is WEAK_PTR
        std::weak_ptr<ObserverType> m_weakPtrObserver;
    };

    /// Mutex to serialize access to m_depth and m_observers.  Note that a recursive mutex is used
    /// here to avoid undefined behavior if notifying an observer callback in to this @c Notifier.
    std::recursive_mutex m_mutex;

    /// Depth of calls to @c notifyObservers() and notifyObserversInReverse().
    int m_depth;

    /// The set of observers.  Note that a vector is used here to allow for the addition
    /// or removal of observers while calls to notifyObservers() are in progress.
    std::vector<NotifierObserver> m_observers;

    /// If set, this function will be called after an observer is added.
    std::function<void(const std::shared_ptr<ObserverType>&)> m_addObserverFunc;
};

template <typename ObserverType>
inline Notifier<ObserverType>::NotifierObserver::NotifierObserver(const std::shared_ptr<ObserverType>& observer) :
        m_type{ObserverPointerType::SHARED_PTR},
        m_sharedPtrObserver{observer} {
}

template <typename ObserverType>
inline Notifier<ObserverType>::NotifierObserver::NotifierObserver(const std::weak_ptr<ObserverType>& observer) :
        m_type{ObserverPointerType::WEAK_PTR},
        m_weakPtrObserver{observer} {
}

template <typename ObserverType>
inline std::shared_ptr<ObserverType> Notifier<ObserverType>::NotifierObserver::get() const {
    if (ObserverPointerType::SHARED_PTR == m_type) {
        return m_sharedPtrObserver;
    } else {
        return m_weakPtrObserver.lock();
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::NotifierObserver::clear() {
    if (ObserverPointerType::SHARED_PTR == m_type) {
        m_sharedPtrObserver.reset();
    } else {
        m_weakPtrObserver.reset();
    }
}

template <typename ObserverType>
inline bool Notifier<ObserverType>::NotifierObserver::isEqualOrExpired(
    const std::shared_ptr<ObserverType>& observer) const {
    if (m_type == ObserverPointerType::SHARED_PTR) {
        return m_sharedPtrObserver == observer;
    } else {
        return m_weakPtrObserver.expired() || m_weakPtrObserver.lock() == observer;
    }
}

template <typename ObserverType>
inline Notifier<ObserverType>::Notifier() : m_depth{0} {
}

template <typename ObserverType>
inline void Notifier<ObserverType>::addObserver(const std::shared_ptr<ObserverType>& observer) {
    if (!observer) {
        return;
    }

    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (isAlreadyExistLocked(observer)) {
        return;
    }
    m_observers.push_back(NotifierObserver{observer});

    if (m_addObserverFunc) {
        m_addObserverFunc(observer);
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::removeObserver(const std::shared_ptr<ObserverType>& observer) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (m_depth > 0) {
        for (size_t ix = 0; ix < m_observers.size(); ix++) {
            auto& notifierObserver = m_observers[ix];
            if (notifierObserver.get() == observer) {
                notifierObserver.clear();
            }
        }
    } else {
        cleanupLocked(observer);
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::addWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) {
    auto observerSharedPtr = observer.lock();
    if (!observerSharedPtr) {
        return;
    }
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    if (isAlreadyExistLocked(observerSharedPtr)) {
        return;
    }
    m_observers.push_back(NotifierObserver{observer});

    if (m_addObserverFunc) {
        m_addObserverFunc(observerSharedPtr);
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::removeWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) {
    auto observerSharedPtr = observer.lock();
    if (!observerSharedPtr) {
        return;
    }
    removeObserver(observerSharedPtr);
}

template <typename ObserverType>
inline void Notifier<ObserverType>::notifyObservers(std::function<void(const std::shared_ptr<ObserverType>&)> notify) {
    std::lock_guard<std::recursive_mutex> guard(m_mutex);
    m_depth++;
    for (size_t ix = 0; ix < m_observers.size(); ix++) {
        const auto& notifierObserver = m_observers[ix];
        auto observer = notifierObserver.get();
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
        const auto& notifierObserver = m_observers[ix];
        auto observer = notifierObserver.get();
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
    bool notifyAddedObservers = false;
    if (!m_addObserverFunc && addObserverFunc) {
        notifyAddedObservers = true;
    }
    m_addObserverFunc = addObserverFunc;
    if (notifyAddedObservers) {
        notifyObservers(m_addObserverFunc);
    }
}

template <typename ObserverType>
inline void Notifier<ObserverType>::cleanupLocked(const std::shared_ptr<ObserverType>& unwanted) {
    auto matches = [unwanted](NotifierObserver notifierObserver) {
        return notifierObserver.isEqualOrExpired(unwanted);
    };

    m_observers.erase(std::remove_if(m_observers.begin(), m_observers.end(), matches), m_observers.end());
}

template <typename ObserverType>
inline bool Notifier<ObserverType>::isAlreadyExistLocked(const std::shared_ptr<ObserverType>& observer) {
    for (const auto& existing : m_observers) {
        if (existing.get() == observer) {
            return true;
        }
    }
    return false;
}

}  // namespace acsdkNotifier
}  // namespace alexaClientSDK

#endif  // ACSDKNOTIFIER_INTERNAL_NOTIFIER_H_
