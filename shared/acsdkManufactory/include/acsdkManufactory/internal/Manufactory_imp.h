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

#ifndef ACSDKMANUFACTORY_INTERNAL_MANUFACTORY_IMP_H_
#define ACSDKMANUFACTORY_INTERNAL_MANUFACTORY_IMP_H_

#include <tuple>

#include <AVSCommon/Utils/Logger/LoggerUtils.h>

#include "acsdkManufactory/Component.h"
#include "acsdkManufactory/Manufactory.h"
#include "acsdkManufactory/internal/Utils.h"

namespace alexaClientSDK {
namespace acsdkManufactory {

template <typename... Exports>
template <typename... Parameters>
inline std::unique_ptr<Manufactory<Exports...>> Manufactory<Exports...>::create(
    const Component<Parameters...>& component) {
    static_assert(!internal::HasRequiredImport<Parameters...>::value, "Component has non satisfied Import<Type>.");

    // Check if any export is missing. If missing, assertion will fail and PrintMissingExport will print a compilation
    // error with the name of the types missing.
    using MissingExports = typename internal::RemoveTypes<std::tuple<Exports...>, std::tuple<Parameters...>>::type;
    static_assert(std::tuple_size<MissingExports>() == 0, "Missing export types required by Manufactory::create.");
    internal::PrintMissingExport<MissingExports>()();

    auto cookBook = component.getCookBook();
    internal::DefaultValues<Parameters...>::apply(cookBook);
    if (!cookBook.checkCompleteness()) {
        return nullptr;
    }
    return std::unique_ptr<Manufactory>(new Manufactory(cookBook));
}

template <typename... Exports>
template <typename... Superset>
std::unique_ptr<Manufactory<Exports...>> Manufactory<Exports...>::createSubsetManufactory(
    const std::shared_ptr<Manufactory<Superset...>>& input) {
    // Check if any export is missing. If missing, assertion will fail and PrintMissingExport will print a compilation
    // error with the name of the types missing.
    using MissingExports = typename internal::RemoveTypes<std::tuple<Exports...>, std::tuple<Superset...>>::type;
    static_assert(
        std::tuple_size<MissingExports>() == 0,
        "Input does not provide all the types to be exported by "
        "subset manufactory");
    internal::PrintMissingExport<MissingExports>()();

    if (!input) {
        avsCommon::utils::logger::acsdkError(
            avsCommon::utils::logger::LogEntry("Manufactory", "createSubsetManufactoryFailed")
                .d("reason", "nullSuperSetManufactory"));
        return nullptr;
    }
    return Manufactory<Exports...>::create(input->m_runtimeManufactory);
}

template <typename... Exports>
template <typename... Subset>
inline std::unique_ptr<Manufactory<Subset...>> Manufactory<Exports...>::createSubsetManufactory() {
    // Check if any export is missing. If missing, assertion will fail and PrintMissingExport will print a compilation
    // error with the name of the types missing.
    using MissingExports = typename internal::RemoveTypes<std::tuple<Subset...>, std::tuple<Exports...>>::type;
    static_assert(std::tuple_size<MissingExports>() == 0, "Manufactory does not export all types in Subset...");
    internal::PrintMissingExport<MissingExports>()();

    return Manufactory<Subset...>::create(m_runtimeManufactory);
}

template <typename... Exports>
template <typename Type>
inline Type Manufactory<Exports...>::get() {
    static_assert(
        internal::ContainsType<std::tuple<Exports...>, Type>::value,
        "Manufactory::get() does not support requested Type");

    return m_runtimeManufactory->get<Type>();
}

template <typename... Exports>
inline std::unique_ptr<Manufactory<Exports...>> Manufactory<Exports...>::create(
    const std::shared_ptr<internal::RuntimeManufactory>& runtimeManufactory) {
    // Perhaps add run-time check here that runtimeManufactory supports Exports...

    return std::unique_ptr<Manufactory>(new Manufactory(runtimeManufactory));
}

template <typename... Exports>
inline Manufactory<Exports...>::Manufactory(const internal::CookBook& cookBook) {
    m_runtimeManufactory.reset(new internal::RuntimeManufactory(cookBook));
}

template <typename... Exports>
inline Manufactory<Exports...>::Manufactory(const std::shared_ptr<internal::RuntimeManufactory>& runtimeManufactory) :
        m_runtimeManufactory{runtimeManufactory} {
}

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_MANUFACTORY_IMP_H_