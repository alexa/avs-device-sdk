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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_FEATURECLIENTBUILDERINTERFACE_IMPL_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_FEATURECLIENTBUILDERINTERFACE_IMPL_H_

namespace alexaClientSDK {
namespace sdkClient {

template <typename ComponentType>
ACSDK_INLINE_VISIBILITY inline void FeatureClientBuilderInterface::addRequiredType() {
    m_requiredTypes.addType<ComponentType>();
}

}  // namespace sdkClient
}  // namespace alexaClientSDK
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_FEATURECLIENTBUILDERINTERFACE_IMPL_H_
