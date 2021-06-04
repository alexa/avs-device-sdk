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

#include "acsdkManufactory/internal/WeakPointerCache.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

WeakPointerCache::WeakPointerCache(std::shared_ptr<AbstractRecipe> recipe) :
        m_recipe{recipe},
        m_produceInstance{recipe->getProduceInstanceFunction()},
        m_cachedValue{std::weak_ptr<void>()},
        m_temporaryCachedValue{nullptr} {
}

WeakPointerCache::~WeakPointerCache() {
}

void* WeakPointerCache::get(RuntimeManufactory& runtimeManufactory) {
    auto temp = m_cachedValue.lock();

    if (!temp) {
        m_temporaryCachedValue =
            static_cast<std::shared_ptr<void>*>(m_produceInstance(m_recipe, runtimeManufactory, nullptr));
        temp = *static_cast<std::shared_ptr<void>*>(m_temporaryCachedValue);
        m_cachedValue = temp;
    } else {
        m_temporaryCachedValue = new std::shared_ptr<void>(temp);
    }

    return m_temporaryCachedValue;
}

void WeakPointerCache::cleanup() {
    auto objectToDelete = static_cast<std::shared_ptr<void>*>(m_temporaryCachedValue);
    delete objectToDelete;
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK
