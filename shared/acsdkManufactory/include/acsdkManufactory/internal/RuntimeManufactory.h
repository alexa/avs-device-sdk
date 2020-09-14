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

#ifndef ACSDKMANUFACTORY_INTERNAL_RUNTIMEMANUFACTORY_H_
#define ACSDKMANUFACTORY_INTERNAL_RUNTIMEMANUFACTORY_H_

#include <memory>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "acsdkManufactory/Annotated.h"
#include "acsdkManufactory/internal/AbstractPointerCache.h"
#include "acsdkManufactory/internal/TypeIndex.h"

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

class CookBook;

/**
 * @c RuntimeManufactory provides instances of interfaces supported by a @c CookBook, automatically
 * creating instances of other interfaces that the requested instance depends upon.
 */
class RuntimeManufactory {
public:
    /**
     * Constructor.
     *
     * @param cookBook The @c CookBook that specifies the recipes for the instances to be provided.
     */
    RuntimeManufactory(const CookBook& cookBook);

    /**
     * Get an instance of the specified @c Type.
     *
     * @tparam Type The type of instance to provide.
     * @return The requested instance of the specified @c Type or nullptr.
     */
    template <typename Type>
    Type get();

private:
    /**
     * Get a std::unique_ptr<Type>
     *
     * @tparam Type The type of std:unique_ptr to get.
     * @return The requested instance of std::unique_ptr<Type>.
     */
    template <typename Type>
    std::unique_ptr<Type> innerGet(std::unique_ptr<Type>*);

    /**
     * Get a std::shared_ptr<Type>
     *
     * @tparam Type The type of std:shared_ptr to get.
     * @return The requested instance of std::shared_ptr<Type>.
     */
    template <typename Type>
    std::shared_ptr<Type> innerGet(std::shared_ptr<Type>*);

    /**
     * Get an Annotated<Annotation, Type>.
     *
     * @tparam Annotation The type used to distinguish this use of @c Type.
     * @tparam Type The type of object pointed to by the returned Annotated<Annotation, Type>.
     * @return The requested instance of Annotated<Annotation, Type>.
     */
    template <typename Annotation, typename Type>
    Annotated<Annotation, Type> innerGet(Annotated<Annotation, Type>*);

    /// The @c CookBook to use to create instances of requested interfaces.
    std::unique_ptr<CookBook> m_cookBook;

    /// Map from interface types to cached values.
    std::unordered_map<TypeIndex, std::shared_ptr<AbstractPointerCache>> m_values;
};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#include <acsdkManufactory/internal/RuntimeManufactory_imp.h>

#endif  // ACSDKMANUFACTORY_INTERNAL_RUNTIMEMANUFACTORY_H_
