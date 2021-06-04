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

#ifndef ACSDKMANUFACTORY_INTERNAL_WEAKPOINTERCACHE_H_
#define ACSDKMANUFACTORY_INTERNAL_WEAKPOINTERCACHE_H_

#include "acsdkManufactory/internal/AbstractPointerCache.h"
#include "acsdkManufactory/internal/AbstractRecipe.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/**
 * Class used to cache a weak_ptr to an instance.
 */
class WeakPointerCache : public AbstractPointerCache {
public:
    /**
     * Constructor.
     *
     * @param recipe The recipe that contains information on the means of producing an instance.
     */
    WeakPointerCache(std::shared_ptr<AbstractRecipe> recipe);

    /**
     * Destructor.
     */
    ~WeakPointerCache();

    /// @name AbstractPointerCache methods.
    /// @{
    void* get(RuntimeManufactory& runtimeManufactory) override;
    void cleanup() override;
    /// @}

private:
    /// The recipe containing information about the means of producing an instance.
    std::shared_ptr<AbstractRecipe> m_recipe;

    /// The @c ProduceInstanceFunction to produce an instance.
    AbstractRecipe::ProduceInstanceFunction m_produceInstance;

    /// The cached instance. Using std::weak_ptr<void> allows this class to be unaware of the type it is caching,
    /// which reduces the use of class templates.
    std::weak_ptr<void> m_cachedValue;

    /**
     * A temporary void* to a std::shared_ptr<void>. This is necessary when producing a new instance to ensure the
     * cached weak_ptr<void> stays alive long enough for the caller to retrieve and cast the returned void* to a
     * std::shared_ptr<void>.
     *
     * @note This must be deleted after calling get() on this cache, or else there will be a memory leak. Use
     * the cleanup() method after calling get() on this cache to ensure proper deletion.
     */
    void* m_temporaryCachedValue;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_WEAKPOINTERCACHE_H_