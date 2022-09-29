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

#ifndef ACSDKMANUFACTORY_INTERNAL_COOKBOOK_H_
#define ACSDKMANUFACTORY_INTERNAL_COOKBOOK_H_

#include <functional>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <AVSCommon/Utils/TypeIndex.h>

#include "acsdkManufactory/Annotated.h"
#include "acsdkManufactory/internal/AbstractPointerCache.h"
#include "acsdkManufactory/internal/AbstractRecipe.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

// Forward declaration
class RuntimeManufactory;

/**
 * CookBook is a collection of recipes for creating instances.
 */
class CookBook {
public:
    /**
     * Constructor.
     */
    CookBook();

    /**
     * Add a factory that returns a @c std::unique_ptr value.
     *
     * @tparam Type The full type (including std::unique_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addUniqueFactory(std::function<std::unique_ptr<Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c std::unique_ptr value.
     *
     * @tparam Type The full type (including std::unique_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addUniqueFactory(std::unique_ptr<Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c std::shared_ptr<> to a @c primary value (i.e. a value
     * that must always be instantiated before all others).  If multiple 'primary' factories
     * are added, they will be instantiated in an arbitrary order - other than that required
     * by any dependencies.
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addPrimaryFactory(std::function<std::shared_ptr<Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c Annotated<> pointer to a @c primary value (i.e. a value
     * that must always be instantiated before all others).  If multiple 'primary' factories
     * are added, they will be instantiated in an arbitrary order - other than that required
     * by any dependencies.
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addPrimaryFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c std::shared_ptr<> to a @c primary value (i.e. a value
     * that must always be instantiated before all others).  If multiple 'primary' factories
     * are added, they will be instantiated in an arbitrary order - other than that required
     * by any dependencies.
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addPrimaryFactory(std::shared_ptr<Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c Annotated<> pointer to a @c primary value (i.e. a value
     * that must always be instantiated before all others).  If multiple 'primary' factories
     * are added, they will be instantiated in an arbitrary order - other than that required
     * by any dependencies.
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addPrimaryFactory(Annotated<Annotation, Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c std::shared_ptr<> to a @c required value (i.e. a value
     * that must always be instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addRequiredFactory(std::function<std::shared_ptr<Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c Annotated<> pointer to a @c required value (i.e. a value
     * that must always be instantiated).
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addRequiredFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c std::shared_ptr<> to a @c required value (i.e. a value
     * that must always be instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addRequiredFactory(std::shared_ptr<Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c Annotated<> pointer to a @c required value (i.e. a value
     * that must always be instantiated).
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addRequiredFactory(Annotated<Annotation, Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c std::shared_ptr<> to a @c retained value (i.e. a value
     * that must be retained once instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addRetainedFactory(std::function<std::shared_ptr<Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c Annotated<> pointer to a @c retained value (i.e. a value
     * that must be retained once instantiated).
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addRetainedFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c std::shared_ptr<> to a @c retained value (i.e. a value
     * that must be retained once instantiated).
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addRetainedFactory(std::shared_ptr<Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c Annotated<> pointer to a @c retained value (i.e. a value
     * that must be retained once instantiated).
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addRetainedFactory(Annotated<Annotation, Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c std::shared_ptr<> to an @c unloadable value (i.e. a value
     * that may be released when no longer referenced).
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addUnloadableFactory(std::function<std::shared_ptr<Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c Annotated<> pointer to an @c unloadable value (i.e. a value
     * that may be released when no longer referenced).
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addUnloadableFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> factory);

    /**
     * Add a factory that returns a @c std::shared_ptr<> to an @c unloadable value (i.e. a value
     * that may be released when no longer referenced).
     *
     * @tparam Type The full type (including std::shared_ptr<>) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Type, typename... Dependencies>
    CookBook& addUnloadableFactory(std::shared_ptr<Type> (*factory)(Dependencies...));

    /**
     * Add a factory that returns a @c Annotated<> pointer to an @c unloadable value (i.e. a value
     * that may be released when no longer referenced).
     *
     * @tparam Annotation The type used to differentiate the returned instance of @c Type.
     * @tparam Type The full type (including Annotated) returned by the factory.
     * @tparam Dependencies The dependencies (arguments) of the factory to be added.
     * @param factory The factory to be added.
     * @return This @c CookBook.
     */
    template <typename Annotation, typename Type, typename... Dependencies>
    CookBook& addUnloadableFactory(Annotated<Annotation, Type> (*factory)(Dependencies...));

    /**
     * Declare support for providing instances of an type from an already existing instance.  Such instances
     * are assumed to be "retained" for the lifecycle of this @c CookBook.
     *
     * @tparam Type The type of interface supported.
     * @param instance The instance to be served up.
     * @return This @c CookBook instance, with the added factory.
     */
    template <typename Type>
    CookBook& addInstance(const Type& instance);

    /**
     * Merge the contents of a @c CookBook into this @c CookBook.
     *
     * If the incoming contents conflict (e.g. specifies a different factory for the same Type) this @c CookBook
     * will be marked invalid and will fail all further operations.
     *
     * @param cookBook The @c CookBook to merge into this @c CookBook.
     * @return This @c CookBook instance, merged with the contents of @c cookBook.
     */
    CookBook& addCookBook(const CookBook& cookBook);

    /**
     * Verify that this CookBook instance is complete (all Imports are satisfied, there are no cyclic dependencies,
     * and the the CookBook is otherwise valid).  If this check fails, this CookBook will be marked invalid.
     *
     * @return Whether this CookBook is valid.
     */
    bool checkCompleteness();

    /**
     * Perform get<Type>() for each Required<Type> registered with this CookBook.
     *
     * @param runtimeManufactory The @c RuntimeManufactory from which to retrieve the Required<Type>s and
     * any of their dependencies.
     * @return Whether all get<Type>() operations were successful.
     */
    bool doRequiredGets(RuntimeManufactory& runtimeManufactory);

    /**
     * Create a new instance @c Type and return it via @c std::unique_ptr<>.
     *
     * @tparam Type The type of value to instantiate.
     * @param runtimeManufactory The RuntimeManufactory to use to acquire dependencies of the new instance.
     * @return A @c std::unique_ptr<Type> to the new instance of Type.
     */
    template <typename Type>
    std::unique_ptr<Type> createUniquePointer(RuntimeManufactory& runtimeManufactory);

    /**
     * Create a @c PointerCache<Type> for the specified Type.
     *
     * @tparam Type The @c Type of object to be cached.
     * @return A new @c PointerCache<Type> instance.
     */
    template <typename Type>
    std::unique_ptr<AbstractPointerCache> createPointerCache();

private:
    /**
     * A recipe that wraps a function pointer to a factory that can create new instances of a type.
     */
    class FactoryRecipe : public AbstractRecipe {
    public:
        /// The function pointer to the factory.
        using Factory = void* (*)();

        /**
         * Constructor.
         * @param factory The function pointer that can create a new instance of this type.
         * @param lifecycle The lifecycle of instances produced by this recipe.
         * @param dependencies A vector of the dependencies required by the factory.
         * @param produceFunction A function pointer that can produce an instance of this type for the manufactory.
         * @param deleteFunction A function pointer that can delete a cached value.
         */
        FactoryRecipe(
            Factory factory,
            CachedInstanceLifecycle lifecycle,
            std::vector<avsCommon::utils::TypeIndex> dependencies,
            ProduceInstanceFunction produceFunction,
            DeleteInstanceFunction deleteFunction);

        /**
         * Destructor.
         */
        ~FactoryRecipe() = default;

        /**
         * Get the @c Factory wrapped by this recipe.
         * @return The @c Factory.
         */
        Factory getFactory() const;

        /// @name AbstractRecipe methods.
        /// @{
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const override;
        /// @}

    private:
        /// The @c Factory that can be used to create a new instance.
        Factory m_factory;
    };

    /**
     * A recipe that wraps a std::shared_ptr<std::function(...)> that can create new instances of a type.
     */
    class FunctionRecipe : public AbstractRecipe {
    public:
        /// A void* that points to a shared_ptr<std::function<Type(Dependencies...)>>. Using a void* allows
        /// this class to be non-templated.
        using Function = void*;

        /// A function pointer that can delete the shared_ptr<std::function<Type(Dependencies...)>> on this
        /// recipe's destruction.
        using DeleteRecipeFunction = void (*)(void*);

        /**
         * Constructor.
         * @param function The function that can create a new instance of this type.
         * @param lifecycle The lifecycle of instances produced by this recipe.
         * @param dependencies A vector of the dependencies required by the function.
         * @param produceFunction A function pointer that can produce an instance of this type for the manufactory
         * @param deleteFunction A function pointer that can delete a cached value.
         * @param deleteRecipeFunction A function pointer that can delete the @c Function.
         */
        FunctionRecipe(
            Function function,
            CachedInstanceLifecycle lifecycle,
            std::vector<avsCommon::utils::TypeIndex> dependencies,
            ProduceInstanceFunction produceFunction,
            DeleteInstanceFunction deleteFunction,
            DeleteRecipeFunction deleteRecipeFunction);

        /**
         * Destructor.
         */
        ~FunctionRecipe();

        /**
         * Get the @c Function wrapped by this recipe.
         * @return The @c Function.
         */
        Function getFunction() const;

        /// @name AbstractRecipe methods.
        /// @{
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const override;
        /// @}
    private:
        /// The @c Function that can be used to create a new instance.
        Function m_function;

        /// The @c DeleteRecipeFunction that can delete m_function, above, when this object is destroyed.
        DeleteRecipeFunction m_deleteRecipeFunction;
    };

    /**
     * A recipe that wraps a std::shared_ptr<void> to an existing instance. We use std::shared_ptr<void> so that
     * this class can be type-unaware, reducing the use of templates.
     */
    class InstanceRecipe : public AbstractRecipe {
    public:
        /**
         * Constructor.
         * @param instance The instance to wrap.
         * @param produceFunction A function pointer that can produce the instance for the manufactory.
         * @param deleteFunction  A function pointer that can delete the cached value.
         */
        InstanceRecipe(
            std::shared_ptr<void> instance,
            ProduceInstanceFunction produceFunction,
            DeleteInstanceFunction deleteFunction);

        /**
         * Destructor.
         */
        ~InstanceRecipe() = default;

        /**
         * Get the instance wrapped by this recipe.
         * @return A shared_ptr<void> to the instance.
         */
        std::shared_ptr<void> getInstance() const;

        /// @name AbstractRecipe methods.
        /// @{
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const override;
        /// @}
    private:
        std::shared_ptr<void> m_instance;
    };

    /**
     * Function template to produce an instance from a @c FactoryRecipe for a std::shared_ptr<Type>, where
     * the cached value is a shared_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Dependencies The types required by the instance.
     * @param recipe The recipe that will provide the factory to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will return the cachedValue and not create a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename... Dependencies>
    static void* produceFromSharedFactoryRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FactoryRecipe for a Annotated<Annotation, Type>, where
     * the cached value is a shared_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Annotation The annotation for this instance.
     * @tparam Dependencies The types required by the instance.
     * @param recipe The recipe that will provide the factory to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will return the cachedValue and not create a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename Annotation, typename... Dependencies>
    static void* produceFromAnnotatedSharedFactoryRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FunctionRecipe for a std::shared_ptr<Type>, where
     * the cached value is a shared_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Dependencies The types required by the instance.
     * @param recipe The recipe that will provide the function to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will return the cachedValue and not create a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename... Dependencies>
    static void* produceFromSharedFunctionRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FunctionRecipe for a Annotated<Annotation, Type>, where
     * the cached value is a shared_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Annotation The annotation for this instance.
     * @tparam Dependencies The types required by the instance.
     * @param recipe The recipe that will provide the function to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will return the cachedValue and not create a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename Annotation, typename... Dependencies>
    static void* produceFromAnnotatedSharedFunctionRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to delete a shared_ptr<Type>.
     *
     * @tparam Type The type to delete.
     * @param cachedValue The cached value (a void* to a shared_ptr<Type>) to delete.
     */
    template <typename Type>
    static void deleteFunctionForSharedRecipe(void* cachedValue);

    /**
     * Function template to delete a std::shared_ptr<std::function<Type(Dependencies...)>>.
     *
     * @tparam Type The return value type of the std::function.
     * @tparam Dependencies The parameter types of the std::function.
     * @param function The std::shared_ptr<std:;function<Type(Dependencies...)>> to delete.
     */
    template <typename Type, typename... Dependencies>
    static void deleteFunctionForFunctionRecipeFunction(void* function);

    /**
     * Function template to produce an instance from a @c FactoryRecipe for a shared_ptr<Type>, where the cached value
     * is a weak_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Dependencies The types required by this instance.
     * @param recipe The recipe that will provide the factory to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will lock the cached value and return it, without creating a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename... Dependencies>
    static void* produceFromWeakFactoryRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FactoryRecipe for a shared_ptr<Type>, where the cached value
     * is a weak_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Annotation The annotation for this instance.
     * @tparam Dependencies The types required by this instance.
     * @param recipe The recipe that will provide the factory to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will lock the cached value and return it, without creating a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename Annotation, typename... Dependencies>
    static void* produceFromAnnotatedWeakFactoryRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FunctionRecipe for a shared_ptr<Type>, where the cached value
     * is a weak_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Dependencies The types required by this instance.
     * @param recipe The recipe that will provide the std::function to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will lock the cached value and return it, without creating a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename... Dependencies>
    static void* produceFromWeakFunctionRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FunctionRecipe for a shared_ptr<Type>, where the cached value
     * is a weak_ptr<Type>*.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Annotation The annotation for this instance.
     * @tparam Dependencies The types required by this instance.
     * @param recipe The recipe that will provide the std::function to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance, if any. If a value is already cached, then this function
     * will lock the cached value and return it, without creating a new instance.
     *
     * @return A void* to a shared_ptr<Type>.
     */
    template <typename Type, typename Annotation, typename... Dependencies>
    static void* produceFromAnnotatedWeakFunctionRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FactoryRecipe for a std::unique_ptr<Type>.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Dependencies The types required by the instance.
     * @param recipe The recipe that will provide the factory to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance. This should always be null, since unique pointers cannot
     * be cached.
     *
     * @return A void* to a unique_ptr<Type>.
     */
    template <typename Type, typename... Dependencies>
    static void* produceFromUniqueFactoryRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * Function template to produce an instance from a @c FunctionRecipe for a std::unique_ptr<Type>.
     *
     * @tparam Type The type of instance to produce.
     * @tparam Dependencies The types required by the instance.
     * @param recipe The recipe that will provide the std::function to create a new instance of this type.
     * @param runtimeManufactory The manufactory that can provide instances for all the dependencies.
     * @param cachedValue The cached value of the instance. This should always be null, since unique pointers
     * cannot be cached.
     *
     * @return A void* to a unique_ptr<Type>.
     */
    template <typename Type, typename... Dependencies>
    static void* produceFromUniqueFunctionRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * This is a no-op function. Since unique pointers cannot be cached, there is nothing to delete.
     *
     * @param cachedValue The cached value to delete. This should be a nullptr.
     */
    static void deleteFunctionForUniqueRecipe(void* cachedValue);

    /**
     * A function to produce the instance from a @c InstanceRecipe for a std::shared_ptr<void>.
     *
     * @param recipe The recipe that will provide the std::shared_ptr<void>.
     * @param runtimeManufactory The manufactory that can provide instances for all dependencies.
     * @param cachedValue The cached value of the instance. If the instance is already cached, return it.
     * @return A void* to a std::shared_ptr<void>.
     */
    static void* produceFromInstanceRecipe(
        std::shared_ptr<AbstractRecipe> recipe,
        RuntimeManufactory& runtimeManufactory,
        void* cachedValue);

    /**
     * A function to delete a cached std::shared_ptr<void>.
     *
     * @param cachedValue A void* to the std::shared_ptr<void> that needs to be deleted.
     */
    static void deleteFunctionForInstanceRecipe(void* cachedValue);

    /**
     * A collection of GetWrappers. This class guarantees that a CookBook will only have one GetWrapper per TypeIndex
     * (by maintaining a map of <TypeIndex, vector index of GetWrapper>), but also ensures that the order of
     * instantiation for objects in the manufactory is predictable and consistent at runtime (by adding the GetWrappers
     * to a vector, which will be iterated through when instantiating the objects).
     *
     * Simply iterating through a set of <TypeIndex, GetWrapper> is insufficient because when the SDK is loaded
     * dynamically and ENABLE_RTTI=OFF, the TypeIndex value may actually change each time the SDK runs. This can lead
     * to unpredictable instantiation order in the manufactory.
     */
    class GetWrapperCollection {
        /**
         * Common signature of functions used to call get<Type>() for all required and primary types.
         */
        using GetWrapper = bool (*)(RuntimeManufactory& runtimeManufactory);

        /// Aliases for readability.
        using GetWrapperIterator =
            std::vector<bool (*)(RuntimeManufactory&), std::allocator<bool (*)(RuntimeManufactory&)>>::iterator;
        using ConstGetWrapperIterator =
            std::vector<bool (*)(RuntimeManufactory&), std::allocator<bool (*)(RuntimeManufactory&)>>::const_iterator;

    public:
        /**
         * Template function to append a @c GetWrapper for a given @c TypeIndex to this collection.
         * @tparam Type The Type of value this @c GetWrapper will create.
         * @param getWrapper  The @c GetWrapper for the given type.
         * @return Whether appending was successful. If a @c GetWrapper for this @c Type is already part of the
         * collection, then this fails.
         */
        template <typename Type>
        bool append(GetWrapper getWrapper);

        /**
         * Append another @c GetWrapperCollection to this one. If this collection already contains one of the
         * @c TypeIndex, that @c TypeIndex and associated @c GetWrapper will not be added, but the rest of the
         * collection will still be processed.
         */
        void append(const std::shared_ptr<GetWrapperCollection>& collection);

        /*
         * Return iterator to beginning of m_getWrappers.
         * @return iterator of beginning of m_getWrappers.
         */
        GetWrapperIterator begin();

        /*
         * Return iterator to end of m_getWrappers.
         * @return iterator of end of m_getWrappers.
         */
        GetWrapperIterator end();

        /*
         * Return const_iterator to beginning of m_getWrappers.
         * @return const_iterator of beginning of m_getWrappers.
         */
        ConstGetWrapperIterator cbegin() const;

        /*
         * Return const_iterator to end of m_getWrappers.
         * @return const_iterator of end of m_getWrappers.
         */
        ConstGetWrapperIterator cend() const;

        /*
         * Return const_iterator to beginning of m_getWrappers.
         * @return const_iterator of beginning of m_getWrappers.
         */
        ConstGetWrapperIterator begin() const;

        /*
         * Return const_iterator to end of m_getWrappers.
         * @return const_iterator of end of m_getWrappers.
         */
        ConstGetWrapperIterator end() const;

    private:
        /// Map of TypeIndex to the index of the GetWrapper in m_orderedGetWrappers.
        std::unordered_map<avsCommon::utils::TypeIndex, std::size_t> m_types;

        /// Vector of GetWrappers.
        std::vector<GetWrapper> m_orderedGetWrappers;
    };

    /**
     * Template type used to describe the signature of an std::function used to create instances.
     */
    template <typename Type, typename... Dependencies>
    using InstanceGetter = std::function<Type(Dependencies...)>;

    /**
     * Add an instance to the CookBook.
     *
     * @param type The @c TypeIndex of this instance.
     * @param newRecipe The new instance recipe to add.
     * @return Whether adding the instance was successful.
     */
    bool addInstance(avsCommon::utils::TypeIndex type, const std::shared_ptr<AbstractRecipe>& newInstanceRecipe);

    /**
     * Add a recipe to the CookBook.
     *
     * @param type The @c TypeIndex of this instance.
     * @param newRecipe The new recipe to add.
     * @return Whether adding the recipe was successful.
     */
    bool addRecipe(avsCommon::utils::TypeIndex type, const std::shared_ptr<AbstractRecipe>& newRecipe);

    /**
     * Check this cookbook for cyclic dependency relationships.
     *
     * @return Whether this cookbook is valid (i.e. doesn't include circular dependencies).
     */
    bool checkForCyclicDependencies();

    /**
     * Check if this @c Component is valid.
     *
     * @param functionName Name of the calling function.  Used to log failures.
     * @return Whether this @c Component is valid.
     */
    bool checkIsValid(const char* functionName) const;

    /**
     * Mark this @c Component as invalid.
     *
     * @param event The 'event' to log
     * @param reason The reason this CookBook is now invalid.
     * @param type The type for which an error was detected.
     */
    void markInvalid(const std::string& event, const std::string& reason, const std::string& type = "");

    /**
     * Log each recipe and its dependencies.
     */
    void logDependencies() const;

    /**
     * Invoke a factory function with its parameters supplied by an @c RuntimeManufactory.
     *
     * @tparam Type The type of interface to return.
     * @tparam Dependencies The dependencies of the requested instance.
     * @param runtimeManufactory The @c RuntimeManufactory to use to get dependencies.
     * @param function The function to use to get the instance.
     * @return An instance of the interface @c Type.
     */
    template <typename Type, typename... Dependencies>
    static Type invokeWithDependencies(
        RuntimeManufactory& runtimeManufactory,
        std::function<Type(Dependencies...)> function);

    /**
     * Invoke a function with its dependencies if they are all non-null.
     *
     * @tparam Type The type of interface to return.
     * @tparam FunctionType The type of function to invoke.
     * @tparam Dependencies The types of arguments to pass to the @c function.
     * @param function The function to invoke.
     * @param dependencies The arguments to pass to the @c function.
     * @return An instance of the interface @c Type.
     */
    template <typename Type, typename FunctionType, typename... Dependencies>
    static Type innerInvokeWithDependencies(FunctionType function, Dependencies... dependencies);

    /**
     * Returns the Tag used by CookBook for logging.
     */
    static std::string getLoggerTag();

    /// Is this @c Component valid?
    bool m_isValid;

    /// Map from interface types to the recipe for getting an instance of that type.
    std::unordered_map<avsCommon::utils::TypeIndex, std::shared_ptr<AbstractRecipe>> m_recipes;

    /// The collection of functions that need to be called to trigger @c get<Type>() calls for all primary types in this
    /// @c CookBook.
    std::shared_ptr<GetWrapperCollection> m_primaryGets;

    /// The collection of  that need to be called to trigger @c get<Type>() calls for all required types in this @c
    /// CookBook.
    std::shared_ptr<GetWrapperCollection> m_requiredGets;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#include <acsdkManufactory/internal/CookBook_imp.h>

#endif  // ACSDKMANUFACTORY_INTERNAL_COOKBOOK_H_