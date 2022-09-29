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

#ifndef ACSDK_NOTIFIERINTERFACES_NOTIFIERINTERFACE_H_
#define ACSDK_NOTIFIERINTERFACES_NOTIFIERINTERFACE_H_

#include <functional>
#include <memory>

namespace alexaClientSDK {
namespace notifierInterfaces {

/**
 * @brief Generic observer operations interface.
 *
 * Interface for maintaining a set of observers that are notified with a caller defined function.
 *
 * @tparam ObserverType The type of observer notified by the template instantiation.
 */
template <typename ObserverType>
class NotifierInterface {
public:
    /**
     * Destructor.
     */
    virtual ~NotifierInterface() = default;

    /**
     * @brief Add an observer.
     *
     * Method adds a strong reference to observer. If the same observer has already been added as a strong or weak
     * reference, the method does nothing.
     *
     * If the observer function has been installed prior to this call, it will be invoked with the newly added observer.
     *
     * @param[in] observer The observer to add. If @a observer is nullptr, the method does nothing.
     *
     * @deprecated In the future, @c Notifier will no longer maintain the life cycle of its @c observers.  Please start
     * using the new @c addWeakPtrObserver() API instead.
     */
    virtual void addObserver(const std::shared_ptr<ObserverType>& observer) = 0;

    /**
     * @brief Remove an observer.
     *
     * Method removes a strong or weak reference to @a observer. If the class doesn't have a strong or weak reference
     * to @a observer, the method does nothing.
     *
     * @param[in] observer The observer to remove. If @a observer is nullptr, the method does nothing.
     *
     * @deprecated In the future, @c Notifier will no longer maintain the life cycle of its @c observers.  Please start
     * using the new @c removeWeakPtrObserver() API instead.
     */
    virtual void removeObserver(const std::shared_ptr<ObserverType>& observer) = 0;

    /**
     * @brief Add a weak reference to an observer.
     *
     * Method adds a weak reference to observer. If the same observer has already been added as a strong or weak
     * reference, the method does nothing.
     *
     * If the observer function has been installed prior to this call, it will be invoked with the newly added observer.
     *
     * @param[in] observer The observer to add. If @a observer is nullptr, the method does nothing.
     *
     * @note Lifecycle of @a observer will not be managed by the Notifier.  If @a observer object is expired, then
     *       no callback will be called to that object.
     */
    virtual void addWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) = 0;

    /**
     * @brief Remove an observer.
     *
     * Method removes a strong or weak reference to @a observer. If the class doesn't have a strong or weak reference
     * to @a observer, the method does nothing.
     *
     * @param[in] observer The observer to remove. If @a observer is nullptr, the method does nothing.
     */
    virtual void removeWeakPtrObserver(const std::weak_ptr<ObserverType>& observer) = 0;

    /**
     * @brief Notify the observers in the order that they were added.
     *
     * @param[in] notify The function to invoke to notify an observer.
     */
    virtual void notifyObservers(std::function<void(const std::shared_ptr<ObserverType>&)> notify) = 0;

    /**
     * @brief Notify the observers in the reverse order that they were added.
     *
     * @param notify The function to invoke to notify an observer.
     * @return true if (and only if) all observers were notified (observers added during calls to this method
     * will miss out).
     */
    virtual bool notifyObserversInReverse(std::function<void(const std::shared_ptr<ObserverType>&)> notify) = 0;

    /**
     * @brief Set a function for notifications on an observer additions.
     *
     * Set the function to be called after an observer is added (for example, to notify the newly-added observer
     * of the current state).
     *
     * If there's any observers that were added before @c setAddObserverFunction is called, those added observers will
     * be notified as well.
     *
     * @warn Use caution when setting this function. The function MUST be reentrant, or else you run the risk
     * of deadlock. When an observer adds itself to a @c NotifierInterface, this function will be called in the
     * same context.
     *
     * @param[in] postAddFunc The function to call after an observer is added.
     */
    virtual void setAddObserverFunction(std::function<void(const std::shared_ptr<ObserverType>&)> postAddFunc) = 0;
};

}  // namespace notifierInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIERINTERFACES_NOTIFIERINTERFACE_H_
