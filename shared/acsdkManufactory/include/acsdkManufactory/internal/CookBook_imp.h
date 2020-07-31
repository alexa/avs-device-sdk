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
#include <set>
#include <stack>
#include <tuple>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>

#include "acsdkManufactory/internal/CookBook.h"
#include "acsdkManufactory/internal/TypeTraitsHelper.h"
#include "acsdkManufactory/internal/RuntimeManufactory.h"
#include "acsdkManufactory/internal/Utils.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

template <typename Type, typename... Dependencies>
CookBook& CookBook::addUniqueFactory(std::function<std::unique_ptr<Type>(Dependencies...)> factory) {
    addFunctionRecipe<UniquePointerFunctionRecipe<Type, Dependencies...>, std::unique_ptr<Type>, Dependencies...>(
        factory);
    return *this;
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addUniqueFactory(std::unique_ptr<Type> (*factory)(Dependencies...)) {
    addFactoryRecipe<UniquePointerFactoryRecipe<Type, Dependencies...>, std::unique_ptr<Type>, Dependencies...>(
        factory);
    return *this;
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addPrimaryFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    using ResultType = std::shared_ptr<Type>;

    addFunctionRecipe<
        SharedPointerFunctionRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);

    m_primaryGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addPrimaryFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    using ResultType = Annotated<Annotation, Type>;

    addFunctionRecipe<
        SharedPointerFunctionRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);

    m_primaryGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addPrimaryFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    using ResultType = std::shared_ptr<Type>;

    addFactoryRecipe<
        SharedPointerFactoryRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);

    m_primaryGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addPrimaryFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    using ResultType = Annotated<Annotation, Type>;

    addFactoryRecipe<
        SharedPointerFactoryRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);

    m_primaryGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addRequiredFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    using ResultType = std::shared_ptr<Type>;

    addFunctionRecipe<
        SharedPointerFunctionRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);

    m_requiredGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addRequiredFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    using ResultType = Annotated<Annotation, Type>;

    addFunctionRecipe<
        SharedPointerFunctionRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);

    m_requiredGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addRequiredFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    using ResultType = std::shared_ptr<Type>;

    addFactoryRecipe<
        SharedPointerFactoryRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);

    m_requiredGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addRequiredFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    using ResultType = Annotated<Annotation, Type>;

    addFactoryRecipe<
        SharedPointerFactoryRecipe<RequiredPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);

    m_requiredGets.insert(
        [](RuntimeManufactory& runtimeManufactory) { return static_cast<bool>(runtimeManufactory.get<ResultType>()); });

    return *this;
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addRetainedFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    using ResultType = std::shared_ptr<Type>;

    return addFunctionRecipe<
        SharedPointerFunctionRecipe<RetainedPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addRetainedFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    using ResultType = Annotated<Annotation, Type>;

    return addFunctionRecipe<
        SharedPointerFunctionRecipe<RetainedPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addRetainedFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    using ResultType = std::shared_ptr<Type>;

    return addFactoryRecipe<
        SharedPointerFactoryRecipe<RetainedPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addRetainedFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    using ResultType = Annotated<Annotation, Type>;

    return addFactoryRecipe<
        SharedPointerFactoryRecipe<RetainedPointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addUnloadableFactory(std::function<std::shared_ptr<Type>(Dependencies...)> function) {
    using ResultType = std::shared_ptr<Type>;

    return addFunctionRecipe<
        SharedPointerFunctionRecipe<UnloadablePointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addUnloadableFactory(std::function<Annotated<Annotation, Type>(Dependencies...)> function) {
    using ResultType = Annotated<Annotation, Type>;

    return addFunctionRecipe<
        SharedPointerFunctionRecipe<UnloadablePointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        function);
}

template <typename Type, typename... Dependencies>
CookBook& CookBook::addUnloadableFactory(std::shared_ptr<Type> (*factory)(Dependencies...)) {
    using ResultType = std::shared_ptr<Type>;

    return addFactoryRecipe<
        SharedPointerFactoryRecipe<UnloadablePointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);
}

template <typename Annotation, typename Type, typename... Dependencies>
CookBook& CookBook::addUnloadableFactory(Annotated<Annotation, Type> (*factory)(Dependencies...)) {
    using ResultType = Annotated<Annotation, Type>;

    return addFactoryRecipe<
        SharedPointerFactoryRecipe<UnloadablePointerCache<ResultType, Dependencies...>, ResultType, Dependencies...>>(
        factory);
}

template <typename Type>
inline CookBook& CookBook::addInstance(const Type& instance) {
    if (!checkIsValid(__func__)) {
        return *this;
    }

    auto newRecipe = std::make_shared<SharedPointerInstanceRecipe<Type>>(instance);
    auto& RecipePtr = m_recipes[getTypeIndex<Type>()];
    if (RecipePtr) {
        if (!RecipePtr->isEquivalent(newRecipe)) {
            markInvalid("addInstanceFailed", "non-isEquivalent instance ", getTypeIndex<Type>().getName());
        }
    } else {
        RecipePtr = newRecipe;
    }

    return *this;
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

    m_primaryGets.insert(cookBook.m_primaryGets.begin(), cookBook.m_primaryGets.end());
    m_requiredGets.insert(cookBook.m_requiredGets.begin(), cookBook.m_requiredGets.end());

    return *this;
};

inline bool CookBook::doRequiredGets(
    alexaClientSDK::acsdkManufactory::internal::RuntimeManufactory& runtimeManufactory) {
    for (auto getFcn : m_primaryGets) {
        if (!getFcn || !getFcn(runtimeManufactory)) {
            return false;
        }
    }
    for (auto getFcn : m_requiredGets) {
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

    auto type = getTypeIndex<std::unique_ptr<Type>>();
    auto it = m_recipes.find(type);
    if (it != m_recipes.end()) {
        if (it->second) {
            auto recipe = static_cast<UniquePointerRecipe<Type>*>(it->second.get());
            return recipe->createUniquePointer(runtimeManufactory);
        } else {
            markInvalid("createUniquePointerFailed", "null Recipe for type: ", type.getName());
            return std::unique_ptr<Type>();
        }
    }

    markInvalid("createUniquePointerFailed", "no Recipe for type", type.getName());
    return std::unique_ptr<Type>();
}

template <typename Type>
inline std::unique_ptr<PointerCache<Type>> CookBook::createPointerCache() {
    if (!checkIsValid(__func__)) {
        return std::unique_ptr<PointerCache<Type>>();
    }

    auto type = getTypeIndex<Type>();
    auto it = m_recipes.find(type);
    if (it != m_recipes.end()) {
        if (it->second) {
            auto recipe = static_cast<SharedPointerRecipe<Type>*>(it->second.get());
            return recipe->createSharedPointerCache();
        } else {
            markInvalid("createSharedPointerCacheFailed", "null Recipe for type: ", type.getName());
            return std::unique_ptr<PointerCache<Type>>();
        }
    }

    markInvalid("createSharedPointerCacheFailed", "no Recipe for type", type.getName());
    return std::unique_ptr<PointerCache<Type>>();
}

inline CookBook::CookBook() : m_isValid{true} {
}

inline bool CookBook::checkCompleteness() {
    if (!checkIsValid(__func__)) {
        return false;
    }

    return checkForCyclicDependencies();
}

template <typename RecipeType, typename ResultType, typename... Dependencies>
CookBook& CookBook::addFunctionRecipe(std::function<ResultType(Dependencies...)> function) {
    if (!checkIsValid(__func__)) {
        return *this;
    }
    auto newRecipe = std::make_shared<RecipeType>(function);
    auto& RecipePtr = m_recipes[getTypeIndex<ResultType>()];
    if (RecipePtr) {
        if (!RecipePtr->isEquivalent(newRecipe)) {
            markInvalid("addFunctionRecipeFiled", "Recipes are not equivalent", getTypeIndex<ResultType>().getName());
        }
    } else {
        RecipePtr = newRecipe;
    }

    return *this;
}

template <typename RecipeType, typename ResultType, typename... Dependencies>
CookBook& CookBook::addFactoryRecipe(ResultType (*factory)(Dependencies...)) {
    if (!checkIsValid(__func__)) {
        return *this;
    }
    auto newRecipe = std::make_shared<RecipeType>(factory);
    auto& RecipePtr = m_recipes[getTypeIndex<ResultType>()];
    if (RecipePtr) {
        if (!RecipePtr->isEquivalent(newRecipe)) {
            markInvalid("addFactoryRecipeFiled", "Recipes are not equivalent", getTypeIndex<ResultType>().getName());
        }
    } else {
        RecipePtr = newRecipe;
    }

    return *this;
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
    if (!checkNonNull(dependencies...)) {
        return Type();
    }

    return function(std::move(dependencies)...);
}

template <typename First, typename... Remainder>
inline bool CookBook::checkNonNull(const First& first, const Remainder&... remainder) {
    return first && checkNonNull(remainder...);
}

inline bool CookBook::checkNonNull() {
    return true;
}

////////// CookBook::AbstractRecipe

inline std::vector<TypeIndex>::const_iterator CookBook::AbstractRecipe::begin() const {
    return m_dependencies.begin();
}

inline std::vector<TypeIndex>::const_iterator CookBook::AbstractRecipe::end() const {
    return m_dependencies.end();
}

////////// CookBook::UniquePointerRecipe

template <typename Type>
TypeIndex CookBook::UniquePointerRecipe<Type>::getValueType() const {
    return getTypeIndex<std::unique_ptr<Type>>();
}

////////// CookBook::UniquePointerFunctionRecipe

template <typename Type, typename... Dependencies>
CookBook::UniquePointerFunctionRecipe<Type, Dependencies...>::UniquePointerFunctionRecipe(
    std::function<std::unique_ptr<Type>(Dependencies...)> function) :
        m_function{function} {
    this->m_dependencies = {getTypeIndex<Dependencies>()...};
}

template <typename Type, typename... Dependencies>
TypeIndex CookBook::UniquePointerFunctionRecipe<Type, Dependencies...>::getRecipeType() const {
    return getTypeIndex<CookBook::UniquePointerFunctionRecipe<Type, Dependencies...>>();
}

template <typename Type, typename... Dependencies>
bool CookBook::UniquePointerFunctionRecipe<Type, Dependencies...>::isEquivalent(
    const std::shared_ptr<AbstractRecipe>& recipe) const {
    return false;
}

template <typename Type, typename... Dependencies>
std::unique_ptr<Type> CookBook::UniquePointerFunctionRecipe<Type, Dependencies...>::createUniquePointer(
    RuntimeManufactory& runtimeManufactory) const {
    if (!m_function) {
        return std::unique_ptr<Type>();
    }

    InstanceGetter<std::unique_ptr<Type>, Dependencies...> getter = [this](Dependencies... dependencies) {
        return m_function(std::move(dependencies)...);
    };

    return CookBook::invokeWithDependencies(runtimeManufactory, getter);
}

////////// CookBook::UniquePointerFactoryRecipe

template <typename Type, typename... Dependencies>
CookBook::UniquePointerFactoryRecipe<Type, Dependencies...>::UniquePointerFactoryRecipe(
    std::unique_ptr<Type> (*factory)(Dependencies...)) :
        m_factory{factory} {
    this->m_dependencies = {getTypeIndex<Dependencies>()...};
}

template <typename Type, typename... Dependencies>
TypeIndex CookBook::UniquePointerFactoryRecipe<Type, Dependencies...>::getRecipeType() const {
    return getTypeIndex<CookBook::UniquePointerFactoryRecipe<Type, Dependencies...>>();
}

template <typename Type, typename... Dependencies>
bool CookBook::UniquePointerFactoryRecipe<Type, Dependencies...>::isEquivalent(
    const std::shared_ptr<AbstractRecipe>& recipe) const {
    if (!recipe || recipe->getRecipeType() != getRecipeType()) {
        return false;
    }

    auto factoryRecipe = reinterpret_cast<UniquePointerFactoryRecipe<Type, Dependencies...>*>(recipe.get());
    return factoryRecipe->m_dependencies == this->m_dependencies && factoryRecipe->m_factory == m_factory;
}

template <typename Type, typename... Dependencies>
std::unique_ptr<Type> CookBook::UniquePointerFactoryRecipe<Type, Dependencies...>::createUniquePointer(
    RuntimeManufactory& runtimeManufactory) const {
    if (!m_factory) {
        return std::unique_ptr<Type>();
    }

    InstanceGetter<std::unique_ptr<Type>, Dependencies...> getter = [this](Dependencies... dependencies) {
        return m_factory(std::move(dependencies)...);
    };

    return CookBook::invokeWithDependencies(runtimeManufactory, getter);
}

////////// CookBook::SharedPointerRecipe

template <typename Type>
TypeIndex CookBook::SharedPointerRecipe<Type>::getValueType() const {
    return getTypeIndex<std::shared_ptr<Type>>();
}

////////// CookBook::SharedPointerFunctionRecipe

template <typename CacheType, typename Type, typename... Dependencies>
CookBook::SharedPointerFunctionRecipe<CacheType, Type, Dependencies...>::SharedPointerFunctionRecipe(
    std::function<Type(Dependencies...)> function) :
        m_function{function} {
    this->m_dependencies = {getTypeIndex<Dependencies>()...};
}

template <typename CacheType, typename Type, typename... Dependencies>
std::unique_ptr<PointerCache<Type>> CookBook::SharedPointerFunctionRecipe<CacheType, Type, Dependencies...>::
    createSharedPointerCache() const {
    return std::unique_ptr<CacheType>(new CacheType(m_function));
}

template <typename CacheType, typename Type, typename... Dependencies>
TypeIndex CookBook::SharedPointerFunctionRecipe<CacheType, Type, Dependencies...>::getRecipeType() const {
    return getTypeIndex<CookBook::SharedPointerFunctionRecipe<CacheType, Type, Dependencies...>>();
}

template <typename CacheType, typename Type, typename... Dependencies>
bool CookBook::SharedPointerFunctionRecipe<CacheType, Type, Dependencies...>::isEquivalent(
    const std::shared_ptr<AbstractRecipe>& recipe) const {
    return false;
}

////////// CookBook::SharedPointerFactoryRecipe

template <typename CacheType, typename Type, typename... Dependencies>
CookBook::SharedPointerFactoryRecipe<CacheType, Type, Dependencies...>::SharedPointerFactoryRecipe(
    Type (*factory)(Dependencies...)) :
        m_factory{factory} {
    this->m_dependencies = {getTypeIndex<Dependencies>()...};
}

template <typename CacheType, typename Type, typename... Dependencies>
std::unique_ptr<PointerCache<Type>> CookBook::SharedPointerFactoryRecipe<CacheType, Type, Dependencies...>::
    createSharedPointerCache() const {
    return std::unique_ptr<CacheType>(new CacheType(m_factory));
}

template <typename CacheType, typename Type, typename... Dependencies>
TypeIndex CookBook::SharedPointerFactoryRecipe<CacheType, Type, Dependencies...>::getRecipeType() const {
    return getTypeIndex<CookBook::SharedPointerFactoryRecipe<CacheType, Type, Dependencies...>>();
}

template <typename CacheType, typename Type, typename... Dependencies>
bool CookBook::SharedPointerFactoryRecipe<CacheType, Type, Dependencies...>::isEquivalent(
    const std::shared_ptr<AbstractRecipe>& recipe) const {
    if (!recipe || recipe->getRecipeType() != getRecipeType()) {
        return false;
    }

    auto factoryRecipe = reinterpret_cast<SharedPointerFactoryRecipe<CacheType, Type, Dependencies...>*>(recipe.get());

    return factoryRecipe->m_factory == m_factory;
}

////////// CookBook::InstanceRecipe

template <typename Type>
inline CookBook::SharedPointerInstanceRecipe<Type>::SharedPointerInstanceRecipe(const Type& instance) :
        m_instance{instance} {
}

template <typename Type>
std::unique_ptr<PointerCache<Type>> CookBook::SharedPointerInstanceRecipe<Type>::createSharedPointerCache() const {
    return std::unique_ptr<InstancePointerCache<Type>>(new InstancePointerCache<Type>(m_instance));
}

template <typename Type>
inline TypeIndex CookBook::SharedPointerInstanceRecipe<Type>::getRecipeType() const {
    return getTypeIndex<CookBook::SharedPointerInstanceRecipe<Type>>();
}

template <typename Type>
inline bool CookBook::SharedPointerInstanceRecipe<Type>::isEquivalent(
    const std::shared_ptr<CookBook::AbstractRecipe>& Recipe) const {
    if (!Recipe || Recipe->getRecipeType() != getRecipeType()) {
        return false;
    }

    auto instanceRecipe = reinterpret_cast<SharedPointerInstanceRecipe<Type>*>(Recipe.get());

    return instanceRecipe->m_dependencies == this->m_dependencies && instanceRecipe->m_instance == m_instance;
}

////////// CookBook::RequiredPointerCache

template <typename Type, typename... Dependencies>
CookBook::RequiredPointerCache<Type, Dependencies...>::RequiredPointerCache(
    std::function<Type(Dependencies...)> factory) :
        m_factory{factory} {
}

template <typename Type, typename... Dependencies>
Type CookBook::RequiredPointerCache<Type, Dependencies...>::get(RuntimeManufactory& runtimeManufactory) {
    if (!m_value) {
        InstanceGetter<Type, Dependencies...> getter = [this](Dependencies... dependencies) {
            return m_factory(std::move(dependencies)...);
        };

        m_value = invokeWithDependencies(runtimeManufactory, getter);
    }

    return m_value;
}

////////// CookBook::RetainedPointerCache

template <typename Type, typename... Dependencies>
CookBook::RetainedPointerCache<Type, Dependencies...>::RetainedPointerCache(
    std::function<Type(Dependencies...)> factory) :
        m_factory{factory} {
}

template <typename Type, typename... Dependencies>
Type CookBook::RetainedPointerCache<Type, Dependencies...>::get(RuntimeManufactory& runtimeManufactory) {
    if (!m_value) {
        InstanceGetter<Type, Dependencies...> getter = [this](Dependencies... dependencies) {
            return m_factory(std::move(dependencies)...);
        };

        m_value = invokeWithDependencies(runtimeManufactory, getter);
    }

    return m_value;
}

////////// CookBook::UnloadablePointerCache

template <typename Type, typename... Dependencies>
CookBook::UnloadablePointerCache<Type, Dependencies...>::UnloadablePointerCache(
    std::function<Type(Dependencies...)> factory) :
        m_factory{factory} {
}

template <typename Type, typename... Dependencies>
Type CookBook::UnloadablePointerCache<Type, Dependencies...>::get(RuntimeManufactory& runtimeManufactory) {
    auto result = m_value.lock();
    if (!result) {
        InstanceGetter<Type, Dependencies...> getter = [this](Dependencies... dependencies) {
            return m_factory(std::move(dependencies)...);
        };

        std::shared_ptr<typename Type::element_type> temp = invokeWithDependencies(runtimeManufactory, getter);
        m_value = temp;
        result = m_value.lock();
    }

    return result;
}

////////// CookBook::InstancePointerCache

template <typename Type>
CookBook::InstancePointerCache<Type>::InstancePointerCache(Type instance) : m_instance{instance} {
}

template <typename Type>
Type CookBook::InstancePointerCache<Type>::get(RuntimeManufactory& runtimeManufactory) {
    return m_instance;
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_COOKBOOK_IMP_H_
