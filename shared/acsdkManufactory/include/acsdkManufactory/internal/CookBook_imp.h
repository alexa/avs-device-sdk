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

#ifndef ACSDKMANUFACTORY_INTERNAL_COOKBOOK_IMP_H_
#define ACSDKMANUFACTORY_INTERNAL_COOKBOOK_IMP_H_

#include <iostream>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <stack>
#include <tuple>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>

#include "acsdkManufactory/internal/CookBook.h"
#include "acsdkManufactory/internal/TypeTraitsHelper.h"
#include "acsdkManufactory/internal/RuntimeManufactory.h"
#include "acsdkManufactory/internal/SharedPointerCache.h"
#include "acsdkManufactory/internal/WeakPointerCache.h"
#include "acsdkManufactory/internal/Utils.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addUniqueFactory(std::function<std::unique_ptr<Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::unique_ptr<Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::UNIQUE,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromUniqueFunctionRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForUniqueRecipe),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addUniqueFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addUniqueFactory(std::unique_ptr<Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::unique_ptr<Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::UNIQUE,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromUniqueFactoryRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForUniqueRecipe));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addUniqueFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addPrimaryFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::PRIMARY,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFunctionRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addPrimaryFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_primaryGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addPrimaryFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::PRIMARY,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFunctionRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addPrimaryFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_primaryGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addPrimaryFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::PRIMARY,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFactoryRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addPrimaryFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_primaryGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addPrimaryFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::PRIMARY,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFactoryRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addPrimaryFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_primaryGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addRequiredFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::REQUIRED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFunctionRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRequiredFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_requiredGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addRequiredFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::REQUIRED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFunctionRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRequiredFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_requiredGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addRequiredFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::REQUIRED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFactoryRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRequiredFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_requiredGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addRequiredFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::REQUIRED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFactoryRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRequiredFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    m_requiredGets->append<ResultType>(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addRetainedFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::RETAINED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFunctionRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRetainedFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addRetainedFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::RETAINED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFunctionRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRetainedFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addRetainedFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::RETAINED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFactoryRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRetainedFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addRetainedFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::RETAINED,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFactoryRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addRetainedFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addUnloadableFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::UNLOADABLE,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFunctionRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addUnloadableFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addUnloadableFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    auto funcPtr = new std::function<ResultType(Dependencies...)>(function);
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FunctionRecipe>(
        reinterpret_cast<void*>(funcPtr),
        AbstractRecipe::CachedInstanceLifecycle::UNLOADABLE,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFunctionRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>),
        static_cast<void (*)(void*)>(deleteFunctionForFunctionRecipeFunction<ResultType, Dependencies...>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addUnloadableFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Type, typename... Dependencies>
inline CookBook& CookBook::addUnloadableFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = std::shared_ptr<Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::UNLOADABLE,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromSharedFactoryRecipe<Type, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addUnloadableFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
inline CookBook& CookBook::addUnloadableFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    using ResultType = Annotated<Annotation, Type>;
    std::vector<avsCommon::utils::TypeIndex> dependencies = {avsCommon::utils::getTypeIndex<Dependencies>()...};

    auto newRecipe = std::make_shared<FactoryRecipe>(
        reinterpret_cast<void* (*)()>(factory),
        AbstractRecipe::CachedInstanceLifecycle::UNLOADABLE,
        dependencies,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(
            produceFromAnnotatedSharedFactoryRecipe<Type, Annotation, Dependencies...>),
        static_cast<void (*)(void*)>(deleteFunctionForSharedRecipe<Type>));

    auto type = avsCommon::utils::getTypeIndex<ResultType>();
    if (!addRecipe(type, newRecipe)) {
        markInvalid("addUnloadableFactoryFailed", "non-isEquivalent recipe ", type.getName());
    }

    return *this;
}

template <typename Type>
inline CookBook& CookBook::addInstance(const Type& instance) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    auto newRecipe = std::make_shared<InstanceRecipe>(
        instance,
        static_cast<void* (*)(std::shared_ptr<AbstractRecipe>, RuntimeManufactory&, void*)>(produceFromInstanceRecipe),
        static_cast<void (*)(void*)>(deleteFunctionForInstanceRecipe));

    auto type = avsCommon::utils::getTypeIndex<Type>();
    if (!addInstance(type, newRecipe)) {
        markInvalid("addInstanceFailed", "non-isEquivalent instance ", type.getName());
    }

    return *this;
}

inline bool CookBook::addInstance(
    avsCommon::utils::TypeIndex type,
    const std::shared_ptr<AbstractRecipe>& newInstanceRecipe) {
    auto& RecipePtr = m_recipes[type];
    if (RecipePtr) {
        if (!RecipePtr->isEquivalent(newInstanceRecipe)) {
            return false;
        }
    } else {
        RecipePtr = newInstanceRecipe;
    }

    return true;
}

inline bool CookBook::addRecipe(avsCommon::utils::TypeIndex type, const std::shared_ptr<AbstractRecipe>& newRecipe) {
    auto& RecipePtr = m_recipes[type];
    if (RecipePtr) {
/// Remove this check for Windows only.
#ifndef _WIN32
        if (!RecipePtr->isEquivalent(newRecipe)) {
            return false;
        }
#endif
    } else {
        RecipePtr = newRecipe;
    }

    return true;
}

inline CookBook& CookBook::addCookBook(const CookBook& cookBook) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    if (!cookBook.checkIsValid(__func__)) {
        markInvalid("addCookBookFailed", "invalid component");
        return *this;
    }

    // Merge component's recipes in to this component's recipes.

    for (auto item : cookBook.m_recipes) {
        auto it = m_recipes.find(item.first);
        if (it != m_recipes.end()) {
            if (!it->second->isEquivalent(item.second)) {
                markInvalid("addCookBookFailed", "isEquivalentFailed");
                return *this;
            }
        } else {
            m_recipes.insert(item);
        }
    }

    m_primaryGets->append(cookBook.m_primaryGets);
    m_requiredGets->append(cookBook.m_requiredGets);

    return *this;
};

inline bool CookBook::doRequiredGets(
    alexaClientSDK::acsdkManufactory::internal::RuntimeManufactory& runtimeManufactory) {
    for (auto getFcn : *m_primaryGets) {
        if (!getFcn || !getFcn(runtimeManufactory)) {
            return false;
        }
    }

    for (auto getFcn : *m_requiredGets) {
        if (!getFcn || !getFcn(runtimeManufactory)) {
            return false;
        }
    }

    return true;
}

template <typename Type>
inline std::unique_ptr<Type> CookBook::createUniquePointer(RuntimeManufactory& runtimeManufactory) {
    if (!checkIsValid(__func__)) {
        return std::unique_ptr<Type>();
    }

    auto type = avsCommon::utils::getTypeIndex<std::unique_ptr<Type>>();
    auto it = m_recipes.find(type);
    if (it != m_recipes.end()) {
        if (it->second) {
            auto recipe = it->second;
            auto createUniquePointer = recipe->getProduceInstanceFunction();

            /// createUniquePointer() returns a void* to a std::unique_ptr<Type> created on the heap.
            /// The following ensures that we retain the object, but delete the temporary std::unique_ptr pointed to by
            /// the void* before returning a std::unique_ptr to the object.

            /// Cast the void* to a std::unique_ptr<Type>*.
            auto temporaryPointerToUniquePointerOnHeap =
                static_cast<std::unique_ptr<Type>*>(createUniquePointer(recipe, runtimeManufactory, nullptr));

            /// Move the object from the underlying temporary std::unique_ptr on the heap. Now
            /// *temporaryPointerToUniquePointerOnHeap no longer owns the object.
            auto uniquePointerToReturn = std::move(*temporaryPointerToUniquePointerOnHeap);

            /// Delete the temporary std::unique_ptr created on the heap. Without this, there is a memory leak.
            delete temporaryPointerToUniquePointerOnHeap;

            return uniquePointerToReturn;

        } else {
            markInvalid("createUniquePointerFailed", "null Recipe for type: ", type.getName());
            return std::unique_ptr<Type>();
        }
    }

    markInvalid("createUniquePointerFailed", "no Recipe for type", type.getName());
    return std::unique_ptr<Type>();
}

template <typename Type>
inline std::unique_ptr<AbstractPointerCache> CookBook::createPointerCache() {
    if (!checkIsValid(__func__)) {
        return std::unique_ptr<SharedPointerCache>();
    }

    auto type = avsCommon::utils::getTypeIndex<Type>();
    auto it = m_recipes.find(type);
    if (it != m_recipes.end()) {
        if (it->second) {
            auto recipe = it->second;
            if (AbstractRecipe::CachedInstanceLifecycle::UNLOADABLE == recipe->getLifecycle()) {
                return std::unique_ptr<WeakPointerCache>(new WeakPointerCache(recipe));
            }
            return std::unique_ptr<SharedPointerCache>(new SharedPointerCache(recipe));
        } else {
            markInvalid("createPointerCacheFailed", "null Recipe for type: ", type.getName());
            return std::unique_ptr<SharedPointerCache>();
        }
    }

    markInvalid("createPointerCacheFailed", "no Recipe for type", type.getName());
    return std::unique_ptr<SharedPointerCache>();
}

inline CookBook::CookBook() :
        m_isValid{true},
        m_primaryGets{std::make_shared<GetWrapperCollection>()},
        m_requiredGets{std::make_shared<GetWrapperCollection>()} {
}

inline bool CookBook::checkCompleteness() {
    if (!checkIsValid(__func__)) {
        return false;
    }

    return checkForCyclicDependencies();
}

inline bool CookBook::checkIsValid(const char* functionName) const {
    if (!m_isValid) {
        avsCommon::utils::logger::acsdkError(
            avsCommon::utils::logger::LogEntry(getLoggerTag(), "checkIsValidFailed").d("function", functionName));
    }
    return m_isValid;
}

inline void CookBook::markInvalid(const std::string& event, const std::string& reason, const std::string& type) {
    m_isValid = false;
    avsCommon::utils::logger::acsdkError(
        avsCommon::utils::logger::LogEntry(getLoggerTag(), event).d("reason", reason).d("type", type));
}

template <typename Type, typename... Dependencies>
inline Type CookBook::invokeWithDependencies(
    RuntimeManufactory& runtimeManufactory,
    std::function<Type(Dependencies...)> function) {
    using FcnType = typename std::function<Type(Dependencies...)>;
    return CookBook::template innerInvokeWithDependencies<Type, FcnType, Dependencies...>(
        function, runtimeManufactory.get<RemoveCvref_t<Dependencies>>()...);
};

template <typename Type, typename FunctionType, typename... Dependencies>
inline Type CookBook::innerInvokeWithDependencies(FunctionType function, Dependencies... dependencies) {
    return function(std::move(dependencies)...);
}

template <typename Type, typename... Dependencies>
inline void* CookBook::produceFromSharedFactoryRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }

    auto factoryRecipe = reinterpret_cast<FactoryRecipe*>(recipe.get());
    auto factory = reinterpret_cast<std::shared_ptr<Type> (*)(Dependencies...)>(factoryRecipe->getFactory());

    InstanceGetter<std::shared_ptr<Type>, Dependencies...> getter = [factory](Dependencies... dependencies) {
        return factory(std::move(dependencies)...);
    };

    return new std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, getter));
}

template <typename Type, typename Annotation, typename... Dependencies>
inline void* CookBook::produceFromAnnotatedSharedFactoryRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }

    auto factoryRecipe = reinterpret_cast<FactoryRecipe*>(recipe.get());
    auto factory = reinterpret_cast<Annotated<Annotation, Type> (*)(Dependencies...)>(factoryRecipe->getFactory());

    InstanceGetter<std::shared_ptr<Type>, Dependencies...> getter = [factory](Dependencies... dependencies) {
        return factory(std::move(dependencies)...);
    };

    return new std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, getter));
}

template <typename Type, typename... Dependencies>
inline void* CookBook::produceFromSharedFunctionRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }
    auto functionRecipe = reinterpret_cast<FunctionRecipe*>(recipe.get());
    auto function = *static_cast<std::function<std::shared_ptr<Type>(Dependencies...)>*>(functionRecipe->getFunction());

    return new std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, function));
}

template <typename Type, typename Annotation, typename... Dependencies>
inline void* CookBook::produceFromAnnotatedSharedFunctionRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }
    auto functionRecipe = reinterpret_cast<FunctionRecipe*>(recipe.get());
    auto function =
        *static_cast<std::function<Annotated<Annotation, Type>(Dependencies...)>*>(functionRecipe->getFunction());

    auto temp = std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, function));
    return new std::shared_ptr<void>(temp);
}

template <typename Type, typename... Dependencies>
inline void* CookBook::produceFromWeakFactoryRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    auto factoryRecipe = reinterpret_cast<FactoryRecipe*>(recipe.get());
    auto factory = reinterpret_cast<std::shared_ptr<Type> (*)(Dependencies...)>(factoryRecipe->getFactory());

    InstanceGetter<std::shared_ptr<Type>, Dependencies...> getter = [factory](Dependencies... dependencies) {
        return factory(std::move(dependencies)...);
    };

    auto temp = std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, getter));
    return new std::shared_ptr<void>(temp);
}

template <typename Type, typename Annotation, typename... Dependencies>
inline void* CookBook::produceFromAnnotatedWeakFactoryRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }

    auto factoryRecipe = reinterpret_cast<FactoryRecipe*>(recipe.get());
    auto factory = reinterpret_cast<Annotated<Annotation, Type> (*)(Dependencies...)>(factoryRecipe->getFactory());

    InstanceGetter<std::shared_ptr<Type>, Dependencies...> getter = [factory](Dependencies... dependencies) {
        return factory(std::move(dependencies)...);
    };

    auto temp = std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, getter));
    return new std::shared_ptr<void>(temp);
}

template <typename Type, typename... Dependencies>
inline void* CookBook::produceFromWeakFunctionRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }
    auto functionRecipe = reinterpret_cast<FunctionRecipe*>(recipe.get());
    auto function = *static_cast<std::function<std::shared_ptr<Type>(Dependencies...)>*>(functionRecipe->getFunction());

    auto temp = std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, function));
    return new std::shared_ptr<void>(temp);
}

template <typename Type, typename Annotation, typename... Dependencies>
inline void* CookBook::produceFromAnnotatedWeakFunctionRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    if (cachedInstance) {
        return cachedInstance;
    }
    auto functionRecipe = reinterpret_cast<FunctionRecipe*>(recipe.get());
    auto function =
        *static_cast<std::function<Annotated<Annotation, Type>(Dependencies...)>*>(functionRecipe->getFunction());

    auto temp = std::shared_ptr<Type>(invokeWithDependencies(runtimeManufactory, function));
    return new std::shared_ptr<void>(temp);
}

template <typename Type>
inline void CookBook::deleteFunctionForSharedRecipe(void* cachedInstance) {
    auto objectToDelete = static_cast<std::shared_ptr<Type>*>(cachedInstance);
    delete objectToDelete;
}

template <typename Type, typename... Dependencies>
inline void CookBook::deleteFunctionForFunctionRecipeFunction(void* function) {
    auto objectToDelete = static_cast<std::function<Type(Dependencies...)>*>(function);
    delete objectToDelete;
}

template <typename Type, typename... Dependencies>
inline void* CookBook::produceFromUniqueFactoryRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    auto factoryRecipe = reinterpret_cast<FactoryRecipe*>(recipe.get());
    auto factory = reinterpret_cast<std::unique_ptr<Type> (*)(Dependencies...)>(factoryRecipe->getFactory());

    InstanceGetter<std::unique_ptr<Type>, Dependencies...> getter = [factory](Dependencies... dependencies) {
        return factory(std::move(dependencies)...);
    };

    return new std::unique_ptr<Type>(invokeWithDependencies(runtimeManufactory, getter));
}

template <typename Type, typename... Dependencies>
inline void* CookBook::produceFromUniqueFunctionRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedInstance) {
    auto functionRecipe = reinterpret_cast<FunctionRecipe*>(recipe.get());
    auto function = *static_cast<std::function<std::unique_ptr<Type>(Dependencies...)>*>(functionRecipe->getFunction());

    return new std::unique_ptr<Type>(invokeWithDependencies(runtimeManufactory, function));
}

////////// CookBook::GetWrapperCollection

template <typename Type>
inline bool CookBook::GetWrapperCollection::append(GetWrapper getWrapper) {
    auto typeIndex = avsCommon::utils::getTypeIndex<Type>();

    if (m_types.end() == m_types.find(typeIndex)) {
        m_orderedGetWrappers.push_back(getWrapper);
        m_types.insert({typeIndex, m_orderedGetWrappers.size() - 1});

        return true;
    }

    return false;
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_COOKBOOK_IMP_H_
