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

#ifndef ACSDKMANUFACTORY_INTERNAL_POINTERCACHE_H_
#define ACSDKMANUFACTORY_INTERNAL_POINTERCACHE_H_

#include "acsdkManufactory/internal/AbstractPointerCache.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

class RuntimeManufactory;

/**
 * Class used to cache an instances of a specific type.
 *
 * @tparam Type The @c Type to be cached.
 */
template <typename Type>
class PointerCache : public AbstractPointerCache {
public:
    /**
     * Get the instance from the cache.
     *
     * @param runtimeManufactory The @c RuntimeManufactory to use to acquire a @c Type if the cache is empty.
     * @return The cached @c Type.
     */
    virtual Type get(RuntimeManufactory& runtimeManufactory) = 0;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_POINTERCACHE_H_