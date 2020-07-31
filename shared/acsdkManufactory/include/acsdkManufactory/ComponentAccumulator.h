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

#ifndef ACSDKMANUFACTORY_COMPONENTACCUMULATOR_H_
#define ACSDKMANUFACTORY_COMPONENTACCUMULATOR_H_

#include <memory>

#include "acsdkManufactory/Component.h"
#include "acsdkManufactory/Import.h"
#include "acsdkManufactory/internal/CookBook.h"
#include "acsdkManufactory/internal/TypeTraitsHelper.h"

namespace alexaClientSDK {
namespace acsdkManufactory {

/**
 * ComponentAccumulator is a builder style helper class that is used to accumulate the set of types that
 * will be exported and imported by a component as well as how instances of exported types will be provided.
 *
 * @tparam Parameters The types potentially exported by the accumulating component as well as any
 * types required by the accumulating component (tagged via Import<Type>).
 */
template <typename... Parameters>
class ComponentAccumulator {
public:
    /**
     * Create an ComponentAccumulator with no exports or imports.
     */
    ComponentAccumulator();

    /**
     * Copy constructor.
     *
     * @tparam RhsParameters The exports and imports of the source and result @c ComponentAccumulator.
     * @param componentAccumulator The @c ComponentAccumulator to copy.
     */
    template <typename... RhsParameters>
    ComponentAccumulator(const ComponentAccumulator<RhsParameters...>& componentAccumulator);

    /**
     * Add a factory that returns a @c std::unique_ptr value.
     *
     * @tparam Type The full type (including std::unique_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addUniqueFactory(
        Type (*factory)(Dependencies...));

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to a @c primary value (i.e. a value
     * that must instantiated before all others).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addPrimaryFactory(
        std::function<Type(Dependencies...)> factory);

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to a @c primary value (i.e. a value
     * that must instantiated before all others).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addPrimaryFactory(
        Type (*factory)(Dependencies...));

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to a @c required value (i.e. a value
     * that must always be instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addRequiredFactory(
        std::function<Type(Dependencies...)> factory);

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to a @c required value (i.e. a value
     * that must always be instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addRequiredFactory(
        Type (*factory)(Dependencies...));

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to a @c retained value (i.e. a
     * value that must retained once instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addRetainedFactory(
        std::function<Type(Dependencies...)> factory);

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to a @c retained value (i.e. a
     * value that must retained once instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addRetainedFactory(
        Type (*factory)(Dependencies...));

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to an unloadable value (i.e. a value
     * that may be released when all references to it are cleared).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addUnloadableFactory(
        std::function<Type(Dependencies...)> factory);

    /**
     * Add a factory that returns a pointer (@c std::shared_ptr<> or Annotated<>) to an unloadable value (i.e. a value
     * that may be released when all references to it are cleared).
     *
     * @tparam Type The full type (including std::shared_ptr<> or Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c ComponentAccumulator.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<internal::RemoveCvref_t<Dependencies>>..., Type, Parameters...> addUnloadableFactory(
        Type (*factory)(Dependencies...));

    /**
     * Declare an specific instance as the source of instances of @c Type.
     *
     * @tparam Type The type for which an instance is being specified
     * @param instance The instance that will be used to satisfy requests for the type Type.
     * @return Return a new ComponentAccumulator whose type includes the additional exports
     */
    template <typename Type>
    ComponentAccumulator<Type, Parameters...> addInstance(Type instance);

    /**
     * Add the declarations from a @c Component to this @c ComponentAccumulator.
     *
     * @tparam SubComponentParameters The @c Parameters... defining the type of @c Component whose declarations
     * are to be added.
     * @param component The @c Componment whose declarations are to be added.
     * @return Return a new ComponentAccumulator whose type includes the additional exports and imports.
     */
    template <typename... SubComponentParameters>
    ComponentAccumulator<SubComponentParameters..., Parameters...> addComponent(
        const Component<SubComponentParameters...>& component);

    /**
     * Override the factory specified as the source of instances of an type.
     *
     * @tparam Type The type whose factory is being overriden.
     * @tparam Dependencies The arguments to the factory method creating @c Type, which also specifies the
     * Types that this factory depends upon.
     * @param factory The factory function to use when creating instances of @c type.
     * @return Return a new ComponentAccumulator whose type includes the additional exports and imports.
     */
    template <typename Type, typename... Dependencies>
    ComponentAccumulator<Import<Dependencies>..., Type, Parameters...> overrideFactory(
        Type (*factory)(Dependencies...));

    /**
     * Override the instance used as the source of instances of @c Type.
     *
     * @tparam Type The type for which an instance is being specified
     * @param instance The instance that will be used to satisfy requests for the type Type.
     * @return Return a new ComponentAccumulator whose type includes the additional exports
     */
    template <typename Type>
    ComponentAccumulator<Type, Parameters...> overrideInstance(Type instance);

private:
    // Provide Component access to getCookBook().
    template <typename...>
    friend class Component;

    // Provide ComponentAccumulator access to getCookBook().
    template <typename...>
    friend class ComponentAccumulator;

    /**
     * Get a copy of the @c CookBook underlying this ComponentAccumulator.
     *
     * @return a copy of the @c CookBook underlying this ComponentAccumulator.
     */
    internal::CookBook getCookBook() const;

    /// The set of recipes underlying the @c Component
    internal::CookBook m_cookBook;
};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#include <acsdkManufactory/internal/ComponentAccumulator_imp.h>

#endif  // ACSDKMANUFACTORY_COMPONENTACCUMULATOR_H_
