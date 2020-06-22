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

#ifndef ACSDKMANUFACTORY_INTERNAL_COMPONENT_IMP_H_
#define ACSDKMANUFACTORY_INTERNAL_COMPONENT_IMP_H_

#include "acsdkManufactory/Component.h"
#include "acsdkManufactory/internal/Utils.h"

namespace alexaClientSDK {
namespace acsdkManufactory {

template <typename... Parameters>
template <typename... ComponentAccumulatorParameters>
Component<Parameters...>::Component(ComponentAccumulator<ComponentAccumulatorParameters...>&& componentAccumulator) :
        m_cookBook{componentAccumulator.getCookBook()} {
    using AccumulatorTypes = typename internal::GetImportsAndExports<ComponentAccumulatorParameters...>::type;

    using ComponentTypes = typename internal::GetImportsAndExports<Parameters...>::type;

    using MissingExports =
        typename internal::RemoveTypes<typename ComponentTypes::exports, typename AccumulatorTypes::exports>::type;

    ACSDK_STATIC_ASSERT_IS_SAME(
        std::tuple<>, MissingExports, "One or more export declared by this Component was not provided.");

    using MissingImports =
        typename internal::RemoveTypes<typename AccumulatorTypes::imports, typename ComponentTypes::imports>::type;

    ACSDK_STATIC_ASSERT_IS_SAME(
        std::tuple<>, MissingImports, "One or more Import<Type> was not declared by this Component.");
}

template <typename... Parameters>
internal::CookBook Component<Parameters...>::getCookBook() const {
    return m_cookBook;
}

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_COMPONENT_IMP_H_