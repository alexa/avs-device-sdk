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

#ifndef ACSDK_NOTIFIER_PRIVATE_NOTIFIERDATA_H_
#define ACSDK_NOTIFIER_PRIVATE_NOTIFIERDATA_H_

#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

#include <acsdk/Notifier/internal/DataInterface.h>
#include <acsdk/Notifier/private/ObserverWrapper.h>

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Container for abstract observer operations.
 *
 * This class implements container operations for Notifier type.
 *
 * @ingroup Lib_acsdkNotifier
 * @private
 */
class NotifierData : public virtual DataInterface {
public:
    /**
     * Constructor.
     */
    NotifierData() noexcept;

    /// @name Methods from @c DataInterface
    /// @{
    void doAddStrongRefObserver(std::shared_ptr<void> observer) noexcept override;
    void doAddWeakRefObserver(std::shared_ptr<void> observer) noexcept override;
    void doRemoveObserver(std::shared_ptr<void> observer) noexcept override;
    void doNotifyObservers(std::function<void(const std::shared_ptr<void>&)> notify) noexcept override;
    bool doNotifyObserversInReverse(std::function<void(const std::shared_ptr<void>&)> notify) noexcept override;
    void doSetAddObserverFunction(std::function<void(const std::shared_ptr<void>&)> addObserverFunc) noexcept override;
    /// @}

private:
    /**
     * @brief Adds an observer reference.
     *
     * This method implements functionality for DataInterface::doAddStrongRefObserver() and
     * DataInterface::doAddWeakRefObserver() methods.
     *
     * @param[in] observer Observer.
     * @param[in] type     Reference type.
     */
    void doAddObserver(std::shared_ptr<void> observer, ReferenceType type) noexcept;

    /**
     * @brief Remove reference and clean up observers array.
     *
     * Eliminate the @a unwanted observer value from @c m_observers.  This will also cleanup any @c weak_ptr observer
     * that has expired.
     *
     * @param[in] unwanted The unwanted observer value.
     */
    void cleanupLocked(const std::shared_ptr<void>& unwanted) noexcept;

    /**
     * @brief Check if observer already registered.
     *
     * Method checks if observer already exists in  @c m_observers.
     *
     * @param[in] observer The observer value.
     * @return true if observer already exists, false otherwise.
     */
    bool isAlreadyExistLocked(const std::shared_ptr<void>& observer) noexcept;

    /// Mutex to serialize access to m_depth and m_observers.  Note that a recursive mutex is used
    /// here to avoid undefined behavior if notifying an observer callback in to this @c Notifier.
    std::recursive_mutex m_mutex;

    /// Depth of calls to @c notifyObservers() and notifyObserversInReverse().
    std::size_t m_depth;

    /// The set of observers.  Note that a vector is used here to allow for the addition
    /// or removal of observers while calls to notifyObservers() are in progress.
    std::vector<ObserverWrapper> m_observers;

    /// If set, this function will be called after an observer is added.
    std::function<void(const std::shared_ptr<void>&)> m_addObserverFunc;
};

}  // namespace notifier
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIER_PRIVATE_NOTIFIERDATA_H_
