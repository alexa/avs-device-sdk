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

#ifndef ACSDKMANUFACTORY_INTERNAL_ABSTRACTRECIPE_H_
#define ACSDKMANUFACTORY_INTERNAL_ABSTRACTRECIPE_H_

#include <AVSCommon/Utils/TypeIndex.h>

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/// Forward declaration.
class RuntimeManufactory;

/**
 * The abstract class for 'recipes' for creating instances.
 */
class AbstractRecipe {
public:
    /**
     * Alias for a function pointer that produces an instance. Note that this function may not instantiate a new
     * instance, depending on the lifecycle for the object and the cached value passed to this function.
     *
     * @param recipe The @c AbstractRecipe that can provide the factory or std::function for this type of instance.
     * @param runtimeManufactory The runtime manufactory that can provide other required instances.
     * @param cachedValue The previously cached instance, if it exists. For example, a type added as a retained factory
     * may have a cached value; in that case, the cached value should be used instead of creating a new instance.
     *
     * @return A void* to either a shared_ptr<Type> or unique_ptr<Type>. The caller is responsible for casting
     * appropriately. Using void* is necessary so that recipes can be Type-unaware, reducing reliance on class
     * templates.
     */
    using ProduceInstanceFunction =
        void* (*)(std::shared_ptr<AbstractRecipe> recipe, RuntimeManufactory& runtimeManufactory, void* cachedValue);

    /**
     * Alias for a function pointer that deletes an instance.
     *
     * @param void* The void* to the cached value, if it exists.
     */
    using DeleteInstanceFunction = void (*)(void*);

    /**
     * The type of this @c AbstractRecipe. This is used for comparing equivalence between @c AbstractRecipes.
     */
    enum RecipeType {
        /// A recipe whose means to produce the object is a function pointer.
        FACTORY,
        /// A recipe whose means to produce the object is a std::function.
        FUNCTION,
        /// A recipe whose means to produce the object is a pre-made object.
        ADD_INSTANCE
    };

    /**
     * The desired lifecycle for the instances cached in the manufactory. This is used for comparing equivalence between
     * @c AbstractRecipes.
     */
    enum CachedInstanceLifecycle {
        /// A recipe added by addInstance will cache the pre-made instance with a std::shared_ptr.
        INSTANCE,
        /// A recipe added by addPrimaryFactory will produce an object cached with a std::shared_ptr.
        PRIMARY,
        /// A recipe added by addRetainedFactory will produce an object cached with a std::shared_ptr.
        RETAINED,
        /// A recipe added by addRequiredFactory will produce an object cached with a std::shared_ptr.
        REQUIRED,
        /// A recipe added by addUniqueFactory will produce a new instance every time, and never cache them.
        UNIQUE,
        /// A recipe added by addUnloadableFactory will produce an object cached with a std::weak_ptr.
        UNLOADABLE,
    };

    /**
     * Destructor
     */
    virtual ~AbstractRecipe() = default;

    /**
     * Is this instance of @c AbstractRecipe equivalent to the specified recipe.
     *
     * @param Recipe The @c Recipe to compare with.
     *
     * @return Whether this instance of @c AbstractRecipe equivalent to the specified recipe.
     */
    virtual bool isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const = 0;

    /**
     * The type of this instance of @c AbstractRecipe.
     *
     * @return The type of this instance of @c AbstractRecipe.
     */
    RecipeType getRecipeType() const;

    /**
     * The object lifecycle of @c AbstractRecipe.
     *
     * @return The type of this instance of @c AbstractRecipe.
     */
    CachedInstanceLifecycle getLifecycle() const;

    /**
     * Get the @c ProduceInstanceFunction for producing an instance of this type.
     * @return The @c ProduceInstanceFunction.
     */
    ProduceInstanceFunction getProduceInstanceFunction() const;

    /**
     * Get the @c DeleteInstanceFunction for producing an instance of this type.
     * @return The @c DeleteInstanceFunction.
     */
    DeleteInstanceFunction getDeleteInstanceFunction() const;

    /**
     * Get the start of a vector enumerating the dependencies of the interface this Recipe creates.
     *
     * @return The start of a vector enumerating the dependencies of the interface this Recipe creates.
     */
    std::vector<avsCommon::utils::TypeIndex>::const_iterator begin() const;

    /**
     * Get the end of a vector enumerating the dependencies of the interface this Recipe creates.
     *
     * @return The end of a vector enumerating the dependencies of the interface this Recipe creates.
     */
    std::vector<avsCommon::utils::TypeIndex>::const_iterator end() const;

protected:
    /// Vector enumerating the dependencies of the interface this @c AbstractRecipe creates.
    std::vector<avsCommon::utils::TypeIndex> m_dependencies;

    /// Function pointer that can produce an instance of this type.
    ProduceInstanceFunction m_produceFunction;

    /// Function pointer that can delete a cached value of this type.
    DeleteInstanceFunction m_deleteFunction;

    /// The @c RecipeType of this recipe.
    RecipeType m_recipeType;

    /// The @c CachedInstanceLifecycle of a cached instance of this Type.
    CachedInstanceLifecycle m_objectLifecycle;
};

inline AbstractRecipe::RecipeType AbstractRecipe::getRecipeType() const {
    return m_recipeType;
}

inline AbstractRecipe::CachedInstanceLifecycle AbstractRecipe::getLifecycle() const {
    return m_objectLifecycle;
}

inline AbstractRecipe::ProduceInstanceFunction AbstractRecipe::getProduceInstanceFunction() const {
    return m_produceFunction;
}

inline AbstractRecipe::DeleteInstanceFunction AbstractRecipe::getDeleteInstanceFunction() const {
    return m_deleteFunction;
}

inline std::vector<avsCommon::utils::TypeIndex>::const_iterator AbstractRecipe::begin() const {
    return m_dependencies.begin();
}

inline std::vector<avsCommon::utils::TypeIndex>::const_iterator AbstractRecipe::end() const {
    return m_dependencies.end();
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_ABSTRACTRECIPE_H_