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
static const std::string TAG("Cookbook");

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
    using TypeToCompletedMap = std::map<TypeIndex, bool>;

    // Position at one level in DFS traversal.
    struct Position {
        TypeToCompletedMap::iterator typeToCompletedIt;
        std::vector<TypeIndex>::const_iterator nextDependency;
        std::vector<TypeIndex>::const_iterator endOfDependencies;
    };

    // Regularize the depth first search to include all types in the CookBook by creating a dummy 'root'
    // type that includes all types as dependencies.

    TypeToCompletedMap typeToCompletedMap;
    auto terminalCompletedIt = typeToCompletedMap.insert({getTypeIndex<CookBook>(), false}).first;
    std::vector<TypeIndex> allTypes;
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

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK
