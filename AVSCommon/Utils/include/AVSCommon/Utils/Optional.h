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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_OPTIONAL_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_OPTIONAL_H_

#include <AVSCommon/Utils/Logger/LoggerUtils.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {

/**
 * Auxiliary class that implements an optional object, where the value may or may not be present.
 *
 * @note @c ValueT MUST have a copy constructor.
 * @note Method @c value() is available only for @c ValueT types that have a default constructor.
 * @warning This class is not thread-safe.
 */
template <typename ValueT>
class Optional {
public:
    /**
     * Creates an optional object with no valid value.
     */
    Optional();

    /**
     * Creates an optional object with a valid value.
     *
     * @param value Object used to initialize the new optional value.
     * @note This method required @c ValueT to have a copy constructor is available.
     */
    Optional(const ValueT& value);

    /**
     * Copy constructor.
     *
     * @param other Object used to initialize the new optional object.
     */
    Optional(const Optional<ValueT>& other);

    /**
     * Sets optional value to the given ValueT.
     *
     * @param value Object that will be assigned to the optional value.
     * @note ValueT MUST have an assignment operator defined.
     */
    void set(const ValueT& value);

    /**
     * Modifies the optional object so it no longer holds any valid value.
     */
    void reset();

    /**
     * Checks whether the optional contains a value or not.
     *
     * @return true if it contains a value, false if it does not contain a value.
     */
    bool hasValue() const;

    /**
     * Gets the value if present or return other. @c ValueT must have a copy constructor.
     *
     * @param other Object that will be returned if this optional does not hold a value.
     * @return This optional value if *this has a value; otherwise, return @c other.
     */
    ValueT valueOr(const ValueT& other) const;

    /**
     * Gets the value if present or return other. @c ValueT must have a default constructor.
     *
     * @return The object being held by this if @c m_object is valid; otherwise, return @c ValueT().
     */
    ValueT value() const;

    /**
     * Optional destructor.
     */
    ~Optional();

    /**
     * Assignment operator.
     *
     * @param rhs The optional object source of the assignment.
     * @return A reference to @c *this.
     */
    Optional<ValueT>& operator=(const Optional<ValueT>& rhs);

    /**
     * Equality operator.
     *
     * @param rhs The object to compare *this against.
     * @return @c true if both objects don't hold a value, and @c false if only one object holds a value. If both
     * optionals hold valid values, return the result of operator== for their values.
     */
    bool operator==(const Optional<ValueT>& rhs) const;

    /**
     * Inequality operator.
     *
     * @param rhs The object to compare *this against.
     * @return @c true if only one object holds a value, and @c false if both objects don't hold a value. If both
     * optionals hold valid values, return the result of operator!= for their values.
     */
    bool operator!=(const Optional<ValueT>& rhs) const;

    /**
     *  @name Comparison operators.
     *
     *  Compare the current optional object against a second optional.
     *
     *  @param rhs The object to compare against this.
     *  @return @c true if the comparison holds; @c false otherwise.
     */
    /// @{
    bool operator<(const Optional& rhs) const;
    bool operator>(const Optional& rhs) const;
    bool operator<=(const Optional& rhs) const;
    bool operator>=(const Optional& rhs) const;
    /// @}

private:
    /**
     * Function used to retrieve the value as a reference to ValueT.
     *
     * @warning This can only be used if the object has a valid value.
     * @return The reference to the object being kept inside @c m_value.
     */
    /// @{
    inline ValueT& getReference();
    inline const ValueT& getReference() const;
    /// @}

    /// Boolean flag indicating whether the value exists or not.
    bool m_hasValue;

    /// Place holder for the actual value.
    ///
    /// @note We chose to use @c std::aligned_storage so we can control the underlying object lifecycle without the
    /// burden of always using the heap.
    typename std::aligned_storage<sizeof(ValueT), alignof(ValueT)>::type m_value;
};

template <typename ValueT>
Optional<ValueT>::Optional() : m_hasValue{false} {
}

template <typename ValueT>
Optional<ValueT>::Optional(const ValueT& other) : m_hasValue{true} {
    // Create object in the allocated space.
    new (&m_value) ValueT(other);
}

template <typename ValueT>
Optional<ValueT>::Optional(const Optional<ValueT>& other) : m_hasValue{other.m_hasValue} {
    if (hasValue()) {
        new (&m_value) ValueT(other.getReference());
    }
}

template <typename ValueT>
ValueT& Optional<ValueT>::getReference() {
    return *reinterpret_cast<ValueT*>(&m_value);
}

template <typename ValueT>
const ValueT& Optional<ValueT>::getReference() const {
    return *reinterpret_cast<const ValueT*>(&m_value);
}

template <typename ValueT>
void Optional<ValueT>::set(const ValueT& other) {
    if (hasValue()) {
        getReference() = other;
    } else {
        m_hasValue = true;
        new (&m_value) ValueT(other);
    }
}

template <typename ValueT>
void Optional<ValueT>::reset() {
    if (hasValue()) {
        m_hasValue = false;
        getReference().~ValueT();
    }
}

template <typename ValueT>
bool Optional<ValueT>::hasValue() const {
    return m_hasValue;
}

template <typename ValueT>
ValueT Optional<ValueT>::value() const {
    if (hasValue()) {
        return getReference();
    }
    logger::acsdkError(logger::LogEntry("Optional", "valueFailed").d("reason", "optionalHasNoValue"));
    return ValueT();
}

template <typename ValueT>
ValueT Optional<ValueT>::valueOr(const ValueT& other) const {
    if (hasValue()) {
        return getReference();
    }
    return other;
}

template <typename ValueT>
Optional<ValueT>::~Optional() {
    if (hasValue()) {
        getReference().~ValueT();
    }
}

template <typename ValueT>
Optional<ValueT>& Optional<ValueT>::operator=(const Optional<ValueT>& rhs) {
    if (hasValue()) {
        if (rhs.hasValue()) {
            getReference() = rhs.value();
        } else {
            m_hasValue = false;
            getReference().~ValueT();
        }
    } else if (rhs.hasValue()) {
        m_hasValue = true;
        new (&m_value) ValueT(rhs.value());
    }
    return *this;
}

template <typename ValueT>
bool Optional<ValueT>::operator==(const Optional<ValueT>& rhs) const {
    if (this->hasValue()) {
        return rhs.hasValue() && (this->value() == rhs.value());
    }
    return !rhs.hasValue();
}

template <typename ValueT>
bool Optional<ValueT>::operator!=(const Optional<ValueT>& rhs) const {
    return !(*this == rhs);
}

template <typename ValueT>
bool Optional<ValueT>::operator<(const Optional& rhs) const {
    if (m_hasValue && rhs.m_hasValue) {
        return getReference() < rhs.getReference();
    }
    return m_hasValue < rhs.m_hasValue;
}

template <typename ValueT>
bool Optional<ValueT>::operator>(const Optional& rhs) const {
    return rhs < *this;
}

template <typename ValueT>
bool Optional<ValueT>::operator<=(const Optional& rhs) const {
    return !(rhs < *this);
}

template <typename ValueT>
bool Optional<ValueT>::operator>=(const Optional& rhs) const {
    return !(*this < rhs);
}

}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_OPTIONAL_H_
