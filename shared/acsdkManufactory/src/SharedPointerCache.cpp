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

#include "acsdkManufactory/internal/SharedPointerCache.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

SharedPointerCache::SharedPointerCache(std::shared_ptr<AbstractRecipe> recipe) :
        m_recipe{recipe},
        m_produceInstance{recipe->getProduceInstanceFunction()},
        m_deleteInstance{recipe->getDeleteInstanceFunction()},
        m_cachedValue{nullptr} {
}

SharedPointerCache::~SharedPointerCache() {
    m_deleteInstance(m_cachedValue);
}

void* SharedPointerCache::get(RuntimeManufactory& runtimeManufactory) {
    if (m_cachedValue) {
        return m_cachedValue;
    }

    m_cachedValue = m_produceInstance(m_recipe, runtimeManufactory, m_cachedValue);
    return m_cachedValue;
}

void SharedPointerCache::cleanup() {
    /// No-op.
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK
