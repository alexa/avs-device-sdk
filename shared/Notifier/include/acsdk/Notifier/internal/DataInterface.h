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

#ifndef ACSDK_NOTIFIER_INTERNAL_DATAINTERFACE_H_
#define ACSDK_NOTIFIER_INTERNAL_DATAINTERFACE_H_

#include <functional>
#include <memory>

#include <acsdk/Notifier/internal/NotifierTraits.h>

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Interface for notifier operations.
 *
 * @ingroup Lib_acsdkNotifier
 * @private
 */
class DataInterface {
public:
    /**
     * Constructor.
     */
    virtual ~DataInterface() noexcept = default;

    /**
     * @{
     * @brief Add an observer.
     *
     * Method adds @a observer to observer list unless @a observer is nullptr or already present in the list.
     *
     * @param[in] observer Observer as shared pointer to void.
     */
    virtual void doAddStrongRefObserver(std::shared_ptr<void> observer) noexcept = 0;
    virtual void doAddWeakRefObserver(std::shared_ptr<void> observer) noexcept = 0;
    /// @}

    /**
     * @brief Remove an observer.
     *
     * Method removes an observer from observer list if it is present there.
     *
     * @param[in] observer Observer as shared pointer to void.
     */
    virtual void doRemoveObserver(std::shared_ptr<void> observer) noexcept = 0;

    /**
     * @brief Notify observers in forward order.
     *
     * Method invokes @a notify function for all observers in their addition order.
     *
     * @param[in] notify Function to handle notification.
     */
    virtual void doNotifyObservers(std::function<void(const std::shared_ptr<void>&)> notify) noexcept = 0;

    /**
     * @brief Notify observers in reverse order.
     *
     * Method invokes @a notify function for all observers in order inverse to addition.
     *
     * @param[in] notify Function to handle notification.
     *
     * @return True if the size of collection hasn't changed, false otherwise.
     */
    virtual bool doNotifyObserversInReverse(std::function<void(const std::shared_ptr<void>&)> notify) noexcept = 0;

    /**
     * @brief Install or remove function to handle observer additions.
     *
     * @param[in] addObserverFunc Function to call when a new observer is added. If function is empty, previously
     *                            installed function is reset.
     */
    virtual void doSetAddObserverFunction(
        std::function<void(const std::shared_ptr<void>&)> addObserverFunc) noexcept = 0;
};

/**
 * @brief Constructs a new instance of @c DataInterface.
 *
 * @return Reference to @c DataInterface instance.
 */
std::unique_ptr<DataInterface> createDataInterface();

}  // namespace notifier
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIER_INTERNAL_DATAINTERFACE_H_
