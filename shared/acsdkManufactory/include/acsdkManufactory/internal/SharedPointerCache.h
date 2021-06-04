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

#ifndef ACSDKMANUFACTORY_INTERNAL_SHAREDPOINTERCACHE_H_
#define ACSDKMANUFACTORY_INTERNAL_SHAREDPOINTERCACHE_H_

#include "acsdkManufactory/internal/AbstractPointerCache.h"
#include "acsdkManufactory/internal/AbstractRecipe.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/**
 * Class used to cache a shared_ptr to an instance.
 */
class SharedPointerCache : public AbstractPointerCache {
public:
    /**
     * Constructor.
     *
     * @param recipe The recipe that contains information on the means of producing an instance.
     */
    SharedPointerCache(std::shared_ptr<AbstractRecipe> recipe);

    /**
     * Destructor.
     */
    ~SharedPointerCache();

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

    /// The @c DeleteInstanceFunction to delete the cached instance on this cache's destruction.
    AbstractRecipe::DeleteInstanceFunction m_deleteInstance;

    /// The cached instance (if any). This is a void pointer to a std::shared_ptr<Type>. Using a void pointer
    /// allows this class to be unaware of the type it is caching, to reduce use of class templates.
    void* m_cachedValue;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_SHAREDPOINTERCACHE_H_