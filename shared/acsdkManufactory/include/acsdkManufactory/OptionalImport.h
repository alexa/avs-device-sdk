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

#ifndef ACSDKMANUFACTORY_OPTIONALIMPORT_H_
#define ACSDKMANUFACTORY_OPTIONALIMPORT_H_

namespace alexaClientSDK {
namespace acsdkManufactory {
/**
 * Template for tagging a type as an optional dependency (an import). This is similar to Import, but manufactory
 * will not fail if it cannot find any recipe to build the object or if the recipe returns an empty object. When
 * there is no recipe defined, manufactory will automatically use empty object / default creation where that object
 * is needed. For example, for shared pointers, an empty shared pointer will be passed instead.
 *
 * @tparam Types Template parameters of the form <Type>+, where @c Types is a list of types to be imported.
 */
template <typename... Types>
struct OptionalImport {};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_OPTIONALIMPORT_H_
