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
inline Component<Parameters...>::Component(
    ComponentAccumulator<ComponentAccumulatorParameters...>&& componentAccumulator) :
        m_cookBook{componentAccumulator.getCookBook()} {
    using AccumulatorTypes = typename internal::GetImportsAndExports<ComponentAccumulatorParameters...>::type;

    using ComponentTypes = typename internal::GetImportsAndExports<Parameters...>::type;

    // Check if any export is missing. If missing, assertion will fail and PrintMissingExport will print a compilation
    // error with the name of the types missing.
    using MissingExports =
        typename internal::RemoveTypes<typename ComponentTypes::exports, typename AccumulatorTypes::exports>::type;
    static_assert(
        std::tuple_size<MissingExports>() == 0, "One or more exports declared by this Component was not provided.");
    internal::PrintMissingExport<MissingExports>()();

    // Check if any required import that has not been satisfied is missing from the component declaration. If missing,
    // the assertion below will fail and PrintMissingImport statement will print a compilation error with the name of
    // the types missing.
    using MissingRequiredImports =
        typename internal::RemoveTypes<typename AccumulatorTypes::required, typename ComponentTypes::required>::type;
    static_assert(
        std::tuple_size<MissingRequiredImports>() == 0,
        "One or more required import wasn't declared by this Component.");
    internal::PrintMissingImport<MissingRequiredImports>()();

    // Check if any import that has not been satisfied is missing from the component declaration. If missing,
    // the assertion below will fail and PrintMissingImport statement will print a compilation error with the name of
    // the types missing.
    using MissingOptionalImports =
        typename internal::RemoveTypes<typename AccumulatorTypes::optional, typename ComponentTypes::optional>::type;
    static_assert(
        std::tuple_size<MissingOptionalImports>() == 0,
        "One or more optional import wasn't declared by this Component.");
    internal::PrintMissingImport<MissingOptionalImports>()();
}

template <typename... Parameters>
inline internal::CookBook Component<Parameters...>::getCookBook() const {
    return m_cookBook;
}

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_COMPONENT_IMP_H_
