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

#ifndef ACSDKMANUFACTORY_INTERNAL_TYPETRAITSHELPER_H_
#define ACSDKMANUFACTORY_INTERNAL_TYPETRAITSHELPER_H_

#include <type_traits>

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {

/**
 * Template class to remove const, volatile and reference qualifiers from a type.
 *
 * @tparam Type The type to strip of const, volatile, and reference qualifiers.
 */
template <class Type>
struct RemoveCvref {
    /// The type stripped of const, volatile, and reference qualifiers.
    typedef typename std::remove_cv<typename std::remove_reference<Type>::type>::type type;
};

/**
 * Template type to remove const, volatile and reference qualifiers from a type.
 */
template <class Type>
using RemoveCvref_t = typename RemoveCvref<Type>::type;

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_TYPETRAITSHELPER_H_