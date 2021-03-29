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

#ifndef ACSDKMANUFACTORY_MANUFACTORY_H_
#define ACSDKMANUFACTORY_MANUFACTORY_H_

#include <memory>

#include "acsdkManufactory/Component.h"
#include "acsdkManufactory/internal/RuntimeManufactory.h"

namespace alexaClientSDK {
namespace acsdkManufactory {

/**
 * @c Manufactory provides a means of instantiating the interfaces provided by a @c Component.
 *
 * @tparam Exports The interface(s) that this @c Manufactory may instantiate.
 */
template <typename... Exports>
class Manufactory {
public:
    /**
     * Create an @c Manufactory based upon the recipes in @c Component.
     *
     * Note that template parameters of Manufactory and Component are used to perform compile time
     * checks that:
     * - There are no Import<Type>s in Parameters...
     * - All Exports... are included in Parameters...
     *
     * @tparam Parameters interfaces provided by @c Component.
     * @param component The @c Component to base the Manufactory upon.
     * @return A new Manufactory or nullptr if the @c Component was invalid.
     */
    template <typename... Parameters>
    static std::unique_ptr<Manufactory<Exports...>> create(const Component<Parameters...>& component);

    /**
     * Create an @c Manufactory that is a subset of another @c Manufactory.
     *
     * Note that template parameters of the this requested and the input Manufactory's are used to perform
     * a compile time check that all types in Exports... are also in Superset...
     *
     * @tparam Superset types provided by the input @c Manufactory.
     * @param input The @c Manufactory from which to create a subset @c Manufactory.
     * @return A new @c Manufactory or nullptr if the @c Component was invalid.
     */
    template <typename... Superset>
    static std::unique_ptr<Manufactory<Exports...>> createSubsetManufactory(
        const std::shared_ptr<Manufactory<Superset...>>& input);

    /**
     * Create an @c Manufactory that is a subset of another @c Manufactory.
     *
     * Note that template parameters of the this and the requested Manufactorys are used to perform a compile time
     * check that all types in Subset... are also in Exports...
     *
     * @tparam Suubset types to be provided by the resulting subset @c Manufactory.
     * @return A new Manufactory or nullptr if the @c Component was invalid.
     */
    template <typename... Subset>
    std::unique_ptr<Manufactory<Subset...>> createSubsetManufactory();

    /**
     * Get an instance of the specified Type.
     *
     * @tparam Type The type to get.
     * @return The Type retrieved (or nullptr if instantiation fails).
     */
    template <typename Type>
    Type get();

private:
    // Provide Manufactory<...> access to create(const std::shared_ptr<RuntimeManufactory>&)
    template <typename... FriendExports>
    friend class Manufactory;

    /**
     * Create an @c Manufactory that uses a specific @c RuntimeManufactory.
     *
     * @param runtimeManufactory The @c RuntimeManufactory to use to create instances.
     * @return A new Manufactory or nullptr if the @c RuntimeManufactory does not support all exported types.
     */
    static std::unique_ptr<Manufactory<Exports...>> create(
        const std::shared_ptr<internal::RuntimeManufactory>& runtimeManufactory);

    /**
     * Constructor.
     *
     * @param cookBook The @c CookBook to use to create instances.
     */
    Manufactory(const internal::CookBook& cookBook);

    /**
     * Constructor.
     *
     * @param runtimeManufactory The @c RuntimeManufactory to use to create instances.
     */
    Manufactory(const std::shared_ptr<internal::RuntimeManufactory>& runtimeManufactory);

    /// The underlying @c RuntimeManufactory (doesn't do compile time type checks).
    std::shared_ptr<internal::RuntimeManufactory> m_runtimeManufactory;
};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_MANUFACTORY_H_

#include <acsdkManufactory/internal/Manufactory_imp.h>