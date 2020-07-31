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

#ifndef ACSDKMANUFACTORY_COMPONENT_H_
#define ACSDKMANUFACTORY_COMPONENT_H_

#include "acsdkManufactory/internal/CookBook.h"

namespace alexaClientSDK {
namespace acsdkManufactory {

// Forward declaration
template <typename... Parameters>
class ComponentAccumulator;

/**
 * The @c Component class encapsulates code that exports (i.e. provides an implementation) of one or more interfaces
 * and which imports (depends upon) zero or more interfaces.
 *
 * @tparam Parameters The set of interfaces exported and imported by this Component (imported types are tagged
 * by wrapping them in Import<Type>)
 */
template <typename... Parameters>
class Component {
public:
    /**
     * Construct a @c Component from a ComponentAccumulator.
     *
     * Note that the implementation of this constructor performs compile time validation that:
     * - All unsatisfied imported types in @c ComponentAccumulatorParameters are declared as Import<Type>
     * in @c Parameters.
     * - All exports specified in Parameters are also found as exports in @c ComponentAccumulatorParameters.
     *
     * @tparam ComponentAccumulatorParameters The interfaces potentially exported by the new @c Component as well
     * as any interfaces required by the new component (tagged via Import<Type>).
     * @param componentAccumulator The @c ComponentAccumulator providing the specification of the new @c Component.
     */
    template <typename... ComponentAccumulatorParameters>
    Component(ComponentAccumulator<ComponentAccumulatorParameters...>&& componentAccumulator);

private:
    // Provide @c Manufactory access to getCookBook().
    template <typename...>
    friend class Manufactory;

    // Provide ComponentAccumulator access to getCookBook().
    template <typename...>
    friend class ComponentAccumulator;

    /**
     * Get the @c CookBook for all interfaces underlying this this @c Component.
     *
     * @return The @c CookBook for all interfaces underlying this @c Component.
     */
    internal::CookBook getCookBook() const;

    /// The CookBook for all interfaces underlying this @c Component.
    internal::CookBook m_cookBook;
};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#include <acsdkManufactory/internal/Component_imp.h>

#endif  // ACSDKMANUFACTORY_COMPONENT_H_