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

#ifndef ACSDK_NOTIFIER_PRIVATE_OBSERVERWRAPPER_H_
#define ACSDK_NOTIFIER_PRIVATE_OBSERVERWRAPPER_H_

#include <memory>

#include <acsdk/Notifier/private/ReferenceType.h>

namespace alexaClientSDK {
namespace notifier {

/**
 * @brief Container class for holding shared or weak reference.
 *
 * This class holds either shared or weak reference. The reference may be nullptr.
 *
 * @ingroup Lib_acsdkNotifier
 * @private
 */
class ObserverWrapper {
public:
    /**
     * @brief Constructor for a reference with a given type.
     *
     * @param[in] observer The @c std::shared_ptr observer.
     */
    explicit ObserverWrapper(ReferenceType type, const std::shared_ptr<void>& observer) noexcept;

    /**
     * @brief Move constructor.
     *
     * @param[in] src Source Observer object.
     */
    ObserverWrapper(ObserverWrapper&& src) noexcept;

    /**
     * @brief Copy constructor.
     *
     * @param[in] src Source Observer object.
     */
    ObserverWrapper(const ObserverWrapper& src) noexcept;

    /**
     * @brief Destructor.
     */
    ~ObserverWrapper() noexcept;

    /**
     * Gets the observer.
     *
     * @return The observer as a @c std::shared_ptr.
     */
    std::shared_ptr<void> get() const;

    /**
     * @brief Drop managed reference.
     *
     * Method releases strong or weak reference if any. This may lead to release of managed instance and recursive
     * calls.
     */
    void reset() noexcept;

    /**
     * Checks if the notifier observer matches the observer that is passed in, or if the notifier observer (if it's
     * a weak_ptr observer) has expired.
     *
     * @param observer The observer to check against if it's equal.
     * @return true if observer is equal to notifier observer or if notifier observer has expired, false otherwise.
     */
    bool isEqualOrExpired(void* observer) const noexcept;

    /**
     * @brief Move assignment operator.
     *
     * @param[in] src Source object.
     * @return Reference to @a *this.
     */
    ObserverWrapper& operator=(ObserverWrapper&& src) noexcept;

    /**
     * @brief Returns type.
     *
     * @return Type of managed reference.
     */
    ReferenceType getReferenceType() const noexcept;

private:
    /// Type of managed reference.
    ReferenceType m_type;
    union {
        /// shared_ptr of observer, if @a m_type is Type::StrongRef.
        std::shared_ptr<void> m_sharedPtrObserver;
        /// weak_ptr of observer, if @a m_type is Type::WeakRef.
        std::weak_ptr<void> m_weakPtrObserver;
    };
};

}  // namespace notifier
}  // namespace alexaClientSDK

#endif  // ACSDK_NOTIFIER_PRIVATE_OBSERVERWRAPPER_H_
