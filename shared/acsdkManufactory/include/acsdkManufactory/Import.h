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

#ifndef ACSDKMANUFACTORY_IMPORT_H_
#define ACSDKMANUFACTORY_IMPORT_H_

namespace alexaClientSDK {
namespace acsdkManufactory {

/**
 * Template for tagging a type as a dependency (an import) vs an export in a parameter pack that specifies the
 * imports and exports of a @c Container or @c ContainerAccumulator.
 *
 * @tparam Types Template parameters of the form <Type>+, where @c Types is a list of types to be imported.
 */
template <typename... Types>
class Import {};

}  // namespace acsdkManufactory
}  // namespace alexaClientSDK

#endif  // ACSDKMANUFACTORY_IMPORT_H_
