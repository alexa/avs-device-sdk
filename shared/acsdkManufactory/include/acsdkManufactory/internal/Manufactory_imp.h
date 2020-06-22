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

#include "acsdkManufactory/Component.h"
#include "acsdkManufactory/Manufactory.h"
#include "acsdkManufactory/internal/Utils.h"

namespace alexaClientSDK {
namespace acsdkManufactory {

template <typename... Exports>
template <typename... Parameters>
std::unique_ptr<Manufactory<Exports...>> Manufactory<Exports...>::create(const Component<Parameters...>& component) {
    static_assert(!internal::HasImport<Parameters...>::value, "Parameters... includes Import<Type>.");

    using MissingExports = typename internal::RemoveTypes<std::tuple<Exports...>, std::tuple<Parameters...>>::type;

    ACSDK_STATIC_ASSERT_IS_SAME(
        std::tuple<>,
        MissingExports,
        "Component does not export all types required by Manufactory::create(component).");

    auto cookBook = component.getCookBook();
    if (!cookBook.checkCompleteness()) {
        return nullptr;
    }
    return std::unique_ptr<Manufactory>(new Manufactory(cookBook));
}

template <typename... Exports>
template <typename... Subset>
std::unique_ptr<Manufactory<Subset...>> Manufactory<Exports...>::createSubsetManufactory() {
    using MissingExports = typename internal::RemoveTypes<std::tuple<Subset...>, std::tuple<Exports...>>::type;

    ACSDK_STATIC_ASSERT_IS_SAME(std::tuple<>, MissingExports, "Manufactory does not export all types in Subset...");

    return Manufactory<Subset...>::create(m_runtimeManufactory);
}

template <typename... Exports>
template <typename Type>
Type Manufactory<Exports...>::get() {
    static_assert(
        internal::ContainsType<std::tuple<Exports...>, Type>::value,
        "Manufactory::get() does not support requested Type");

    return m_runtimeManufactory->get<Type>();
}

template <typename... Exports>
std::unique_ptr<Manufactory<Exports...>> Manufactory<Exports...>::create(
    const std::shared_ptr<internal::RuntimeManufactory>& runtimeManufactory) {
    // Perhaps add run-time check here that runtimeManufactory supports Exports...

    return std::unique_ptr<Manufactory>(new Manufactory(runtimeManufactory));
}

template <typename... Exports>
Manufactory<Exports...>::Manufactory(const internal::CookBook& cookBook) {
    m_runtimeManufactory.reset(new internal::RuntimeManufactory(cookBook));
}

template <typename... Exports>
Manufactory<Exports...>::Manufactory(const std::shared_ptr<internal::RuntimeManufactory>& runtimeManufactory) :
        m_runtimeManufactory{runtimeManufactory} {
}

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_MANUFACTORY_IMP_H_