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

#ifndef ACSDKMANUFACTORY_ANNOTATED_H_
#define ACSDKMANUFACTORY_ANNOTATED_H_

#include <memory>

namespace alexaClientSDK {
namespace acsdkManufactory {

/**
 * Annotated is a @c shared_ptr wrapper used to define a shared_ptr to a @c Type where the
 * type of the wrapper is distinguished by an @c Annotation type. You can use this when you
 * have multiple instances of the same interface but want to identify or distinguish them by
 * type rather than the underlying pointer value.
 *
 * @tparam Annotation A type used to flavor the template instantiation.  This is typically an
 * empty @c struct definition, where just the name of the @c struct is used to distinguish types.
 * @tparam Type The underlying type that the Annotated @c shared_ptr refers to.
 */
template <typename Annotation, typename Type>
struct Annotated {
public:
    /// The underlying type that is being annotated.
    using element_type = Type;

    /**
     * Constructor.
     */
    Annotated() = default;

    /**
     * Create an @c Annotated<> instance from a pointer to the underlying @c Type.  The resulting instance of
     * @c Annotated takes ownership of the pointer.
     *
     * @param value A pointer to an instance of the underlying @c Type.
     */
    Annotated(Type* value);

    /**
     * Create from a shared_ptr to an instance underlying @c Type.
     *
     * @param value A shared_ptr to an instance underlying @c Type.
     */
    Annotated(const std::shared_ptr<Type>& value);

    /**
     * Create from a shared_ptr to an instance underlying @c Type.
     *
     * @param value A shared_ptr to an instance underlying @c Type.
     */
    Annotated(std::shared_ptr<Type>&& value);

    /**
     * Release this reference to the underlying instance.
     */
    void reset();

    /**
     * Get a naked pointer to the underlying instance.
     *
     * @return A naked pointer to the underlying instance.
     */
    Type* get() const;

    /**
     * Get a reference to the underlying instance.
     *
     * @return A reference to the underlying instance.
     */
    Type& operator*() const;

    /**
     * Get a naked pointer to the underlying instance.
     *
     * @return A naked pointer to the underlying instance.
     */
    Type* operator->() const;

    /**
     * Convert to a shared_ptr to the underlying instance.
     *
     * @return A shared_ptr to the underlying instance.
     */
    operator std::shared_ptr<Type>() const;

    /**
     * Return true if the pointer is non-null.
     *
     * @return True if the pointer is non-null.
     */
    explicit operator bool() const;

private:
    /// shared_ptr to the underlying instance.
    std::shared_ptr<Type> m_value;
};

template <typename Annotation, typename Type>
inline Annotated<Annotation, Type>::Annotated(Type* value) : m_value{value} {
}

template <typename Annotation, typename Type>
inline Annotated<Annotation, Type>::Annotated(const std::shared_ptr<Type>& value) : m_value{value} {
}

template <typename Annotation, typename Type>
inline Annotated<Annotation, Type>::Annotated(std::shared_ptr<Type>&& value) : m_value{value} {
}

template <typename Annotation, typename Type>
inline void Annotated<Annotation, Type>::reset() {
    m_value.reset();
}

template <typename Annotation, typename Type>
inline Type* Annotated<Annotation, Type>::get() const {
    return m_value.get();
}

template <typename Annotation, typename Type>
inline Type& Annotated<Annotation, Type>::operator*() const {
    return *m_value;
}

template <typename Annotation, typename Type>
inline Type* Annotated<Annotation, Type>::operator->() const {
    return m_value.get();
}

template <typename Annotation, typename Type>
inline Annotated<Annotation, Type>::operator std::shared_ptr<Type>() const {
    return m_value;
}

template <typename Annotation, typename Type>
inline Annotated<Annotation, Type>::operator bool() const {
    return !!m_value;
}

template <typename Annotation, typename Type>
inline bool operator==(const Annotated<Annotation, Type>& lhs, const Annotated<Annotation, Type>& rhs) {
    return lhs.get() == rhs.get();
};

template <typename Annotation, typename Type>
inline bool operator!=(const Annotated<Annotation, Type>& lhs, const Annotated<Annotation, Type>& rhs) {
    return !(lhs == rhs);
};

template <typename Annotation, typename Type>
inline bool operator==(const Annotated<Annotation, Type>& lhs, const std::shared_ptr<Type>& rhs) {
    return lhs.get() == rhs.get();
};

template <typename Annotation, typename Type>
inline bool operator!=(const Annotated<Annotation, Type>& lhs, const std::shared_ptr<Type>& rhs) {
    return !(lhs == rhs);
};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_ANNOTATED_H_
