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

#ifndef ACSDKMANUFACTORY_INTERNAL_RUNTIMEMANUFACTORY_IMP_H_
#define ACSDKMANUFACTORY_INTERNAL_RUNTIMEMANUFACTORY_IMP_H_

#include "acsdkManufactory/internal/CookBook.h"
#include "acsdkManufactory/internal/PointerCache.h"
#include "acsdkManufactory/internal/RuntimeManufactory.h"
#include "acsdkManufactory/internal/TypeIndex.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

inline RuntimeManufactory::RuntimeManufactory(const CookBook& cookBook) : m_cookBook{new CookBook{cookBook}} {
    m_cookBook->doRequiredGets(*this);
}

template <typename Type>
inline Type RuntimeManufactory::get() {
    Type* TypedPointer = nullptr;
    return innerGet(TypedPointer);
}

template <typename Type>
std::unique_ptr<Type> RuntimeManufactory::innerGet(std::unique_ptr<Type>*) {
    return m_cookBook->createUniquePointer<Type>(*this);
}

template <typename Type>
std::shared_ptr<Type> RuntimeManufactory::innerGet(std::shared_ptr<Type>*) {
    using ResultType = std::shared_ptr<Type>;

    auto resultTypeIndex = getTypeIndex<ResultType>();

    auto it = m_values.find(resultTypeIndex);
    if (m_values.end() == it || !it->second) {
        std::shared_ptr<PointerCache<ResultType>> cache(std::move(m_cookBook->createPointerCache<ResultType>()));
        m_values[resultTypeIndex] = cache;
        return cache->get(*this);
    } else {
        auto cache = static_cast<PointerCache<ResultType>*>(it->second.get());
        return cache->get(*this);
    }
}

template <typename Annotation, typename Type>
Annotated<Annotation, Type> RuntimeManufactory::innerGet(Annotated<Annotation, Type>*) {
    using ResultType = Annotated<Annotation, Type>;

    auto resultTypeIndex = getTypeIndex<ResultType>();

    auto it = m_values.find(resultTypeIndex);
    if (m_values.end() == it || !it->second) {
        std::shared_ptr<PointerCache<ResultType>> cache(std::move(m_cookBook->createPointerCache<ResultType>()));
        m_values[resultTypeIndex] = cache;
        return cache->get(*this);
    } else {
        auto cache = static_cast<PointerCache<ResultType>*>(it->second.get());
        return cache->get(*this);
    }
}

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_RUNTIMEMANUFACTORY_IMP_H_