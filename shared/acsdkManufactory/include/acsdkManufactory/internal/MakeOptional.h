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

#ifndef ACSDKMANUFACTORY_INTERNAL_MAKEOPTIONAL_H_
#define ACSDKMANUFACTORY_INTERNAL_MAKEOPTIONAL_H_

#include <memory>

namespace alexaClientSDK {
namespace acsdkManufactory {
namespace internal {
/**
 * Template type used by the component accumulator to allow marking a dependency as optional.
 *
 * @warning Do not use this directly.
 * @tparam Type The dependency type to be marked as optional in the component created from an accumulator.
 */
template <typename Type>
struct MakeOptional {};

}  // namespace internal
}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_INTERNAL_MAKEOPTIONAL_H_
