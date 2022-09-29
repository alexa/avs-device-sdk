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

#ifndef ACSDK_NOTIFIER_NOTIFIER_H_
#define ACSDK_NOTIFIER_NOTIFIER_H_

#include <memory>

#include <acsdk/NotifierInterfaces/NotifierInterface.h>
#include <acsdk/Notifier/internal/DataInterface.h>
#include <acsdk/Notifier/internal/NotifierTraits.h>

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Generic implementation of @c NotifierInterface.
 *
 * Notifier maintains a set of observers that are notified with a caller defined function.
 *
 * @tparam ObserverType The type of observer notified by the template instantiation.
 *
 * @ingroup Lib_acsdkNotifier
 */
template <typename ObserverType>
class Notifier : public notifierInterfaces::NotifierInterface<ObserverType> {
public:
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
    /// @brief internal::NotifierTraits specialization for type @a ObserverType.
    using Traits = NotifierTraits<ObserverType>;

    /// @brief Actual container for managing observer references.
    std::unique_ptr<DataInterface> m_data;
};

template <typename ObserverType>
inline Notifier<ObserverType>::Notifier() : m_data(createDataInterface()) {
}

template <typename ObserverType>
inline void Notifier<ObserverType>::addObserver(const std::shared_ptr<ObserverType>& observer) {
    m_data->doAddStrongRefObserver(Traits::toVoidPtr(observer));
}

template <typename ObserverType>
inline void Notifier<ObserverType>::removeObserver(const std::shared_ptr<ObserverType>& observer) {
    m_data->doRemoveObserver(Traits::toVoidPtr(observer));
}

template <typename ObserverType>
inline void Notifier<ObserverType>::addWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) {
    m_data->doAddWeakRefObserver(Traits::toVoidPtr(observer.lock()));
}

template <typename ObserverType>
inline void Notifier<ObserverType>::removeWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) {
    m_data->doRemoveObserver(Traits::toVoidPtr(observer.lock()));
}

template <typename ObserverType>
inline void Notifier<ObserverType>::notifyObservers(std::function<void(const std::shared_ptr<ObserverType>&)> notify) {
    m_data->doNotifyObservers(Traits::adaptFunction(std::move(notify)));
}

template <typename ObserverType>
inline bool Notifier<ObserverType>::notifyObserversInReverse(
    std::function<void(const std::shared_ptr<ObserverType>&)> notify) {
    return m_data->doNotifyObserversInReverse(Traits::adaptFunction(std::move(notify)));
}

template <typename ObserverType>
inline void Notifier<ObserverType>::setAddObserverFunction(
    std::function<void(const std::shared_ptr<ObserverType>&)> addObserverFunc) {
    m_data->doSetAddObserverFunction(Traits::adaptFunction(std::move(addObserverFunc)));
}

}  // namespace notifier
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIER_NOTIFIER_H_
