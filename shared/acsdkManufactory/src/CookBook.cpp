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

#include <AVSCommon/Utils/Logger/Logger.h>

#include "acsdkManufactory/internal/CookBook.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/// String to identify log entries originating from this file.
#define TAG "Cookbook"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

bool CookBook::checkForCyclicDependencies() {
    // Check for cyclic dependencies is performed via depth first search of the dependency graph, marking visited
    // dependencies as 'in progress' when they are first encountered (by adding them to a map<type, bool> with a
    // value of false).  Dependencies are marked 'complete' on the way back up the graph when all of the
    // dependencies of a type have been explored.  If during this traversal a dependency is found to already be
    // 'in progress' that indicates a circular dependency.

    // Note, std::map<> used here for stability of iterators.
    using TypeToCompletedMap = std::map<avsCommon::utils::TypeIndex, bool>;

    // Position at one level in DFS traversal.
    struct Position {
        TypeToCompletedMap::iterator typeToCompletedIt;
        std::vector<avsCommon::utils::TypeIndex>::const_iterator nextDependency;
        std::vector<avsCommon::utils::TypeIndex>::const_iterator endOfDependencies;
    };

    // Regularize the depth first search to include all types in the CookBook by creating a dummy 'root'
    // type that includes all types as dependencies.

    TypeToCompletedMap typeToCompletedMap;
    auto terminalCompletedIt = typeToCompletedMap.insert({avsCommon::utils::getTypeIndex<CookBook>(), false}).first;
    std::vector<avsCommon::utils::TypeIndex> allTypes;
    for (auto item : m_recipes) {
        allTypes.push_back(item.first);
    }

    std::stack<Position> positions;
    positions.push({terminalCompletedIt, allTypes.begin(), allTypes.end()});

    while (!positions.empty()) {
        while (positions.top().nextDependency != positions.top().endOfDependencies) {
            auto typeIndex = *(positions.top().nextDependency++);
            auto completedIt = typeToCompletedMap.find(typeIndex);
            if (typeToCompletedMap.end() == completedIt) {
                auto recipeIt = m_recipes.find(typeIndex);
                if (m_recipes.end() == recipeIt) {
                    markInvalid(__func__, "type not in m_recipes", typeIndex.getName());
                    logDependencies();
                    return false;
                }
                completedIt = typeToCompletedMap.insert({typeIndex, false}).first;
                positions.push({completedIt, recipeIt->second->begin(), recipeIt->second->end()});
            } else if (!completedIt->second) {
                markInvalid(__func__, "cyclic dependency:", completedIt->first.getName());
                while (positions.size() > 1) {
                    auto& top = positions.top();
                    if (top.typeToCompletedIt->second) {
                        break;
                    }
                    avsCommon::utils::logger::acsdkError(avsCommon::utils::logger::LogEntry(getLoggerTag(), "cycle:")
                                                             .d("type", top.typeToCompletedIt->first.getName()));
                    positions.pop();
                }
                logDependencies();
                return false;
            }
        }
        positions.top().typeToCompletedIt->second = true;
        positions.pop();
    }
    return true;
}

void CookBook::logDependencies() const {
    ACSDK_INFO(LX(__func__));
    for (const auto& item : m_recipes) {
        ACSDK_INFO(LX("recipe").d("type", item.first.getName()));
        for (auto ix = item.second->begin(); ix < item.second->end(); ix++) {
            ACSDK_INFO(LX("dependency").d("type", ix->getName()));
        }
    }
}

std::string CookBook::getLoggerTag() {
    return TAG;
}

void CookBook::deleteFunctionForUniqueRecipe(void* cachedValue) {
    // No-op. Unique pointers should not get cached, so they do not need to be deleted.

    // Sanity check
    if (cachedValue) {
        ACSDK_CRITICAL(LX(__func__).m("Unexpected cached value in unique pointer recipe. Possible memory leak."));
    }
}

void* CookBook::produceFromInstanceRecipe(
    std::shared_ptr<AbstractRecipe> recipe,
    RuntimeManufactory& runtimeManufactory,
    void* cachedValue) {
    if (cachedValue) {
        return cachedValue;
    }

    auto instanceRecipe = reinterpret_cast<InstanceRecipe*>(recipe.get());
    auto instance = new std::shared_ptr<void>(instanceRecipe->getInstance());

    return instance;
}

void CookBook::deleteFunctionForInstanceRecipe(void* cachedValue) {
    auto objectToDelete = static_cast<std::shared_ptr<void>*>(cachedValue);
    delete objectToDelete;
}

////////// CookBook::FactoryRecipe

CookBook::FactoryRecipe::FactoryRecipe(
    Factory factory,
    CachedInstanceLifecycle lifecycle,
    std::vector<avsCommon::utils::TypeIndex> dependencies,
    ProduceInstanceFunction produceFunction,
    DeleteInstanceFunction deleteFunction) {
    this->m_objectLifecycle = lifecycle;
    this->m_dependencies = dependencies;
    this->m_produceFunction = produceFunction;
    this->m_deleteFunction = deleteFunction;
    this->m_recipeType = FACTORY;
    m_factory = factory;
}

bool CookBook::FactoryRecipe::isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const {
    if (!recipe || (recipe->getRecipeType() != getRecipeType()) || (recipe->getLifecycle() != getLifecycle())) {
        return false;
    }

    auto factoryRecipe = reinterpret_cast<FactoryRecipe*>(recipe.get());
    return factoryRecipe->m_dependencies == this->m_dependencies && factoryRecipe->m_factory == m_factory;
}

CookBook::FactoryRecipe::Factory CookBook::FactoryRecipe::getFactory() const {
    return m_factory;
}

////////// CookBook::FunctionRecipe

CookBook::FunctionRecipe::FunctionRecipe(
    Function function,
    CachedInstanceLifecycle lifecycle,
    std::vector<avsCommon::utils::TypeIndex> dependencies,
    ProduceInstanceFunction produceFunction,
    DeleteInstanceFunction deleteFunction,
    DeleteRecipeFunction deleteRecipeFunction) {
    this->m_objectLifecycle = lifecycle;
    this->m_dependencies = dependencies;
    this->m_produceFunction = produceFunction;
    this->m_deleteFunction = deleteFunction;
    this->m_recipeType = FUNCTION;
    m_function = function;
    m_deleteRecipeFunction = deleteRecipeFunction;
}

bool CookBook::FunctionRecipe::isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const {
    return false;
}

CookBook::FunctionRecipe::Function CookBook::FunctionRecipe::getFunction() const {
    return m_function;
}

CookBook::FunctionRecipe::~FunctionRecipe() {
    m_deleteRecipeFunction(m_function);
}

////////// CookBook::InstanceRecipe

CookBook::InstanceRecipe::InstanceRecipe(
    std::shared_ptr<void> instance,
    ProduceInstanceFunction produceFunction,
    DeleteInstanceFunction deleteFunction) {
    this->m_objectLifecycle = INSTANCE;
    this->m_produceFunction = produceFunction;
    this->m_deleteFunction = deleteFunction;
    this->m_recipeType = ADD_INSTANCE;
    m_instance = instance;
}

bool CookBook::InstanceRecipe::isEquivalent(const std::shared_ptr<AbstractRecipe>& recipe) const {
    if (!recipe || (recipe->getRecipeType() != getRecipeType()) || (recipe->getLifecycle() != getLifecycle())) {
        return false;
    }

    auto instanceRecipe = reinterpret_cast<InstanceRecipe*>(recipe.get());
    return instanceRecipe->m_dependencies == this->m_dependencies && instanceRecipe->m_instance == m_instance;
}

std::shared_ptr<void> CookBook::InstanceRecipe::getInstance() const {
    return m_instance;
}

////////// CookBook::GetWrapperCollection

void CookBook::GetWrapperCollection::append(const std::shared_ptr<GetWrapperCollection>& collection) {
    auto typesToAdd = collection->m_types;
    auto getWrappersToAdd = collection->m_orderedGetWrappers;

    for (auto type : typesToAdd) {
        if (m_types.end() == m_types.find(type.first)) {
            m_orderedGetWrappers.push_back(getWrappersToAdd[type.second]);
            m_types.insert({type.first, m_orderedGetWrappers.size() - 1});
        }
    }
}

CookBook::GetWrapperCollection::GetWrapperIterator CookBook::GetWrapperCollection::begin() {
    return m_orderedGetWrappers.begin();
}
CookBook::GetWrapperCollection::GetWrapperIterator CookBook::GetWrapperCollection::end() {
    return m_orderedGetWrappers.end();
}
CookBook::GetWrapperCollection::ConstGetWrapperIterator CookBook::GetWrapperCollection::cbegin() const {
    return m_orderedGetWrappers.cbegin();
}

CookBook::GetWrapperCollection::ConstGetWrapperIterator CookBook::GetWrapperCollection::cend() const {
    return m_orderedGetWrappers.cend();
}

CookBook::GetWrapperCollection::ConstGetWrapperIterator CookBook::GetWrapperCollection::begin() const {
    return m_orderedGetWrappers.begin();
}

CookBook::GetWrapperCollection::ConstGetWrapperIterator CookBook::GetWrapperCollection::end() const {
    return m_orderedGetWrappers.end();
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK
