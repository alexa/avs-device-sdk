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

#include "acsdkManufactory/Annotated.h"
#include "acsdkManufactory/internal/AbstractPointerCache.h"
#include "acsdkManufactory/internal/PointerCache.h"
#include "acsdkManufactory/internal/TypeIndex.h"

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
    std::unique_ptr<PointerCache<Type>> createPointerCache();

private:
    /**
     * The base class for 'recipes' for creating instances.
     */
    class AbstractRecipe {
    public:
        /**
         * Destructor
         */
        virtual ~AbstractRecipe() = default;

        /**
         * Get the type of instance generated by this recipe.
         *
         * @return The type of instance generated by this recipe.
         */
        virtual TypeIndex getValueType() const = 0;

        /**
         * The type of this instance of @c AbstractRecipe.
         *
         * @return The type of this instance of @c AbstractRecipe.
         */
        virtual TypeIndex getRecipeType() const = 0;

        /**
         * Is this instance of @c AbstractRecipe equivalent to the specified recipe.
         *
         * @param Recipe The @c Recipe to compare with.
         *
         * @return Whether this instance of @c AbstractRecipe equivalent to the specified recipe.
         */
        virtual bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const = 0;

        /**
         * Get the start of a vector enumerating the dependencies of the interface this Recipe creates.
         *
         * @return The start of a vector enumerating the dependencies of the interface this Recipe creates.
         */
        std::vector<TypeIndex>::const_iterator begin() const;

        /**
         * Get the end of a vector enumerating the dependencies of the interface this Recipe creates.
         *
         * @return The end of a vector enumerating the dependencies of the interface this Recipe creates.
         */
        std::vector<TypeIndex>::const_iterator end() const;

    protected:
        /// a vector enumerating the dependencies of the interface this @c AbstractRecipe creates.
        std::vector<TypeIndex> m_dependencies;
    };

    /**
     * The base class for recipes used to create instances of @c Type and return them via std::unique_ptr<Type>.
     *
     * @tparam Type The Type of instance created by this recipe.
     */
    template <typename Type>
    class UniquePointerRecipe : public AbstractRecipe {
    public:
        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getValueType() const override;
        /// @}

        /**
         * Create a new instance of @c Type and return it via @c std::unique_ptr<Type>.
         *
         * @param runtimeManufactory The @c RuntimeManufactory to use to acquire any dependencies of the
         * factory used to create istances of @c Type.
         * @return A new instance of @c Type returned via @c std::unique_ptr<Type>.
         */
        virtual std::unique_ptr<Type> createUniquePointer(RuntimeManufactory& runtimeManufactory) const = 0;
    };

    template <typename Type, typename... Dependencies>
    class UniquePointerFunctionRecipe : public UniquePointerRecipe<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        UniquePointerFunctionRecipe(std::function<std::unique_ptr<Type>(Dependencies...)> function);

        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getRecipeType() const override;
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const override;
        /// @}

        /// @name UniquePointerRecipe<Type> methods
        /// @{
        std::unique_ptr<Type> createUniquePointer(RuntimeManufactory& runtimeManufactory) const override;
        /// @}

    private:
        /// The factory function used to create instances of @c Type
        std::function<std::unique_ptr<Type>(Dependencies...)> m_function;
    };

    template <typename Type, typename... Dependencies>
    class UniquePointerFactoryRecipe : public UniquePointerRecipe<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        UniquePointerFactoryRecipe(std::unique_ptr<Type> (*factory)(Dependencies...));

        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getRecipeType() const override;
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const override;
        /// @}

        /// @name UniquePointerRecipe<Type> methods
        /// @{
        std::unique_ptr<Type> createUniquePointer(RuntimeManufactory& runtimeManufactory) const override;
        /// @}

    private:
        /// The factory function used to create instances of @c Type
        std::unique_ptr<Type> (*m_factory)(Dependencies...);
    };

    /**
     * The base class for recipes used to create instances of @c Type and return them via std::shared_ptr<Type>.
     * @tparam Type
     */
    template <typename Type>
    class SharedPointerRecipe : public AbstractRecipe {
    public:
        /**
         * Create a PointerCache used to access instances of @c Type.
         *
         * @return A PointerCache used to access instances of @c Type.
         */
        virtual std::unique_ptr<PointerCache<Type>> createSharedPointerCache() const = 0;

        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getValueType() const override;
        /// @}
    };

    template <typename CacheType, typename Type, typename... Dependencies>
    class SharedPointerFunctionRecipe : public SharedPointerRecipe<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        SharedPointerFunctionRecipe(std::function<Type(Dependencies...)> function);

        /// @name SharedPtrRecipe methods
        /// @{
        std::unique_ptr<PointerCache<Type>> createSharedPointerCache() const override;
        /// @}

        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getRecipeType() const override;
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& Recipe) const override;
        /// @}

    private:
        /// The factory to use to create instances.
        std::function<Type(Dependencies...)> m_function;
    };

    template <typename CacheType, typename Type, typename... Dependencies>
    class SharedPointerFactoryRecipe : public SharedPointerRecipe<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        SharedPointerFactoryRecipe(Type (*factory)(Dependencies...));

        /// @name SharedPtrRecipe methods
        /// @{
        std::unique_ptr<PointerCache<Type>> createSharedPointerCache() const override;
        /// @}

        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getRecipeType() const override;
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& Recipe) const override;
        /// @}

    private:
        /// The factory to use to create instances.
        Type (*m_factory)(Dependencies...);
    };

    template <typename Type>
    class SharedPointerInstanceRecipe : public SharedPointerRecipe<Type> {
    public:
        /**
         * Constructor.
         *
         * @param instance The instance to return from instances of @c SharedPointerCache<Type>
         * created by this instance.
         */
        SharedPointerInstanceRecipe(const Type& instance);

        /// @name SharedPtrRecipe methods
        /// @{
        std::unique_ptr<PointerCache<Type>> createSharedPointerCache() const override;
        /// @}

        /// @name AbstractRecipe methods.
        /// @{
        TypeIndex getRecipeType() const override;
        bool isEquivalent(const std::shared_ptr<AbstractRecipe>& Recipe) const override;
        /// @}

    private:
        /// The instance to return.
        Type m_instance;
    };

    /**
     * A specialization @c PointerCache<Type> that creates @c required instances with a factory method and
     * which does not release the cached value until the @c PointerCache is destroyed.
     *
     * @tparam Type The Type of value to cache.
     * @tparam Dependencies The arguments (dependencies) of factory used ot create instances of @c Type.
     */
    template <typename Type, typename... Dependencies>
    class RequiredPointerCache : public PointerCache<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        RequiredPointerCache(std::function<Type(Dependencies...)> factory);

        /// name PointerCache methods.
        /// @{
        Type get(RuntimeManufactory& runtimeManufactory) override;
        /// @}

    private:
        /// The cached value.
        Type m_value;

        /// The factory to use to create new instances of @c Type.
        std::function<Type(Dependencies...)> m_factory;
    };

    /**
     * A specialization @c PointerCache<Type> that creates @c retained instances with a factory method and
     * which does not release the cached value until the @c PointerCache is destroyed.
     *
     * @tparam Type The Type of value to cache.
     * @tparam Dependencies The arguments (dependencies) of factory used ot create instances of @c Type.
     */
    template <typename Type, typename... Dependencies>
    class RetainedPointerCache : public PointerCache<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        RetainedPointerCache(std::function<Type(Dependencies...)> factory);

        /// name PointerCache methods.
        /// @{
        Type get(RuntimeManufactory& runtimeManufactory) override;
        /// @}

    private:
        /// The cached value.
        Type m_value;

        /// The factory to use to create new instances of @c Type.
        std::function<Type(Dependencies...)> m_factory;
    };

    /**
     * A specialization @c PointerCache<Type> that creates @c unloadable instances with a factory method and
     * which allows the cached value to be released whenever all other references to the cached value have
     * been released.
     *
     * @tparam Type The Type of value to cache.
     * @tparam Dependencies The arguments (dependencies) of factory used ot create instances of @c Type.
     */
    template <typename Type, typename... Dependencies>
    class UnloadablePointerCache : public PointerCache<Type> {
    public:
        /**
         * Constructor.
         *
         * @param factory The factory to use to create new instances of @c Type.
         */
        UnloadablePointerCache(std::function<Type(Dependencies...)> factory);

        /// name PointerCache methods.
        /// @{
        Type get(RuntimeManufactory& runtimeManufactory) override;
        /// @}

    private:
        /// The cached value.
        std::weak_ptr<typename Type::element_type> m_value;

        /// The factory to use to create new instances of @c Type.
        std::function<Type(Dependencies...)> m_factory;
    };

    /**
     * A specialization @c PointerCache<Type> that provides access to an instance of @c Type.
     *
     * @tparam Type The Type of value cached.
     */
    template <typename Type>
    class InstancePointerCache : public PointerCache<Type> {
    public:
        InstancePointerCache(Type instance);

        /// name PointerCache methods.
        /// @{
        Type get(RuntimeManufactory& runtimeManufactory) override;
        /// @}

    private:
        /// The instance to return.
        Type m_instance;
    };

    /**
     * Common signature of functions used to call get<Type>() for all required types.
     */
    using GetWrapper = bool (*)(RuntimeManufactory& runtimeManufactory);

    /**
     * Template type used to describe the signature of an std::function used to create instances.
     */
    template <typename Type, typename... Dependencies>
    using InstanceGetter = std::function<Type(Dependencies...)>;

    /**
     * Add a function recipe.
     *
     * @tparam RecipeType The type of recipe to add.
     * @tparam ResultType The type of object created by the recipe.
     * @tparam dependencies The types of parameters to the factory.
     * @param factory The factory method to be invoked by the recipe.
     * @return This CookBook.
     */
    template <typename RecipeType, typename ResultType, typename... Dependencies>
    CookBook& addFunctionRecipe(std::function<ResultType(Dependencies...)> function);

    /**
     * Add a factory recipe.
     *
     * @tparam RecipeType The type of recipe to add.
     * @tparam ResultType The type of object created by the recipe.
     * @tparam dependencies The types of parameters to the factory.
     * @param factory The factory method to be invoked by the recipe.
     * @return This CookBook.
     */
    template <typename RecipeType, typename ResultType, typename... Dependencies>
    CookBook& addFactoryRecipe(ResultType (*factory)(Dependencies...));

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
     * Return true if all of the provided parameters a non-null.
     *
     * @tparam First The type of the first parameter
     * @tparam Remainder The types of the rest of the parameters.
     * @param first The first parameter.
     * @param remainder The remaining parameters.
     * @return Whether all of the provided parameters a true.
     */
    template <typename First, typename... Remainder>
    static bool checkNonNull(const First& first, const Remainder&... remainder);

    /**
     * Terminal recursive template expansion (empty parameter list) of checkNonNull<First, ...Remainder>()
     *
     * @return Retuens true.
     */
    static bool checkNonNull();

    /**
     * Returns the Tag used by CookBook for logging.
     */
    static std::string getLoggerTag();

    /// Is this @c Component valid?
    bool m_isValid;

    /// Map from interface types to the recipe for getting an instance of that type.
    std::unordered_map<TypeIndex, std::shared_ptr<AbstractRecipe>> m_recipes;

    /// Functions that need to be called to trigger @c get<Type>() calls for all primary types in this @ CookBook.
    std::unordered_set<GetWrapper> m_primaryGets;

    /// Functions that need to be called to trigger @c get<Type>() calls for all required types in this @ CookBook.
    std::unordered_set<GetWrapper> m_requiredGets;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#include <acsdkManufactory/internal/CookBook_imp.h>

#endif  // ACSDKMANUFACTORY_INTERNAL_COOKBOOK_H_