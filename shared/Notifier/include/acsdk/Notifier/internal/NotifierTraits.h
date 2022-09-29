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

#ifndef ACSDK_NOTIFIER_INTERNAL_NOTIFIERTRAITS_H_
#define ACSDK_NOTIFIER_INTERNAL_NOTIFIERTRAITS_H_

#include <functional>
#include <memory>

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Type helper for Notifier template class.
 *
 * This helper provides common functions for a particular type @a T.
 *
 * @tparam T Observer type.
 * @ingroup Lib_acsdkNotifier
 * @private
 */
template <class T>
class NotifierTraits {
public:
    // This type is not constructable.
    NotifierTraits() noexcept = delete;
    ~NotifierTraits() noexcept = delete;
    void operator=(const NotifierTraits<T>&) noexcept = delete;

    /**
     * @{
     * @brief Convert typed pointer into void pointer.
     *
     * Method converts pointer of type @a T into void pointer through static cast.
     *
     * @param[in] src Pointer to convert.
     * @return New pointer of type void.
     *
     * @see #fromVoidPtr()
     */
    static std::shared_ptr<void> toVoidPtr(const std::shared_ptr<T>& src) noexcept;
    /// @}

    /**
     * @brief Convert pointer to void to typed pointer.
     *
     * Method converts pointer to void into pointer of type @a T.
     *
     * @param[in] src Pointer to void.
     *
     * @return Pointer to type @a T.
     *
     * @see #toVoidPtr()
     */
    static std::shared_ptr<T> fromVoidPtr(const std::shared_ptr<void>& src) noexcept;

    /**
     * @brief Create consumer function that handles pointer conversion.
     *
     * Method creates a wrapper function around @a notify to convert void pointers into pointers of type @a T and invoke
     * @a notify.
     *
     * @param[in] notify Void function that consumes pointer of type @a T.
     * @return Function that consumes pointer to void, converts it into type @a T, and invokes @a notify with conversion
     *         result. If @a notify is empty, the method returns an empty function.
     *
     * @see #fromVoidPtr()
     */
    static std::function<void(const std::shared_ptr<void>&)> adaptFunction(
        std::function<void(const std::shared_ptr<T>&)>&& notify) noexcept;

private:
    /**
     * @brief Invoke callback after pointer type conversion.
     *
     * Method converts pointer to void into pointer to type @a T and invokes @a notify with the conversion result.
     *
     * @note The returned function is valid until reference to @a notify is valid.
     *
     * @param[in] notify Function to invoke.
     * @param[in] data   Pointer to void to convert and use as a @a notify argument.
     */
    static void invokeNotify(
        const std::function<void(const std::shared_ptr<T>&)>& notify,
        const std::shared_ptr<void>& data) noexcept;
};

template <class T>
std::shared_ptr<void> NotifierTraits<T>::toVoidPtr(const std::shared_ptr<T>& src) noexcept {
    return std::static_pointer_cast<void>(src);
}

template <class T>
std::shared_ptr<T> NotifierTraits<T>::fromVoidPtr(const std::shared_ptr<void>& src) noexcept {
    return std::static_pointer_cast<T>(src);
}

template <class T>
std::function<void(const std::shared_ptr<void>&)> NotifierTraits<T>::adaptFunction(
    std::function<void(const std::shared_ptr<T>&)>&& notify) noexcept {
    if (notify) {
        return std::bind(&NotifierTraits<T>::invokeNotify, std::move(notify), std::placeholders::_1);
    } else {
        return nullptr;
    }
}

template <class T>
void NotifierTraits<T>::invokeNotify(
    const std::function<void(const std::shared_ptr<T>&)>& notify,
    const std::shared_ptr<void>& data) noexcept {
    notify(fromVoidPtr(data));
}

}  // namespace notifier
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIER_INTERNAL_NOTIFIERTRAITS_H_
