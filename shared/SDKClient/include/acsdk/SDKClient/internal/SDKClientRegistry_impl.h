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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_SDKCLIENTREGISTRY_IMPL_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_SDKCLIENTREGISTRY_IMPL_H_
#include <AVSCommon/Utils/TypeIndex.h>

#include "Utils.h"

namespace alexaClientSDK {
namespace sdkClient {

template <
    typename ClientType,
    typename std::enable_if<std::is_base_of<FeatureClientInterface, ClientType>::value>::type*>
ACSDK_INLINE_VISIBILITY inline std::shared_ptr<ClientType> SDKClientRegistry::get() {
    auto clientTypeId = avsCommon::utils::getTypeIndex<ClientType>();
    auto it = m_clientMapping.find(clientTypeId);
    if (it == m_clientMapping.end()) {
        return nullptr;
    }
    return std::static_pointer_cast<ClientType>(it->second);
}

template <typename ComponentType>
ACSDK_INLINE_VISIBILITY inline std::shared_ptr<typename internal::UnderlyingComponentType<ComponentType>::type>
SDKClientRegistry::getComponent() {
    auto clientTypeId = avsCommon::utils::getTypeIndex<ComponentType>();
    auto it = m_componentMapping.find(clientTypeId);
    if (it == m_componentMapping.end()) {
        return nullptr;
    }
    return std::static_pointer_cast<typename internal::UnderlyingComponentType<ComponentType>::type>(it->second);
}

template <typename ComponentType>
ACSDK_INLINE_VISIBILITY inline bool SDKClientRegistry::registerComponent(std::shared_ptr<ComponentType> component) {
    auto clientTypeId = avsCommon::utils::getTypeIndex<ComponentType>();
    return registerComponent(clientTypeId, std::move(component));
}

template <typename AnnotatedName, typename ComponentType>
ACSDK_INLINE_VISIBILITY inline bool SDKClientRegistry::registerComponent(
    Annotated<AnnotatedName, ComponentType> component) {
    auto clientTypeId = avsCommon::utils::getTypeIndex<Annotated<AnnotatedName, ComponentType>>();
    std::shared_ptr<ComponentType> spComponent = std::move(component);
    return registerComponent(clientTypeId, std::move(spComponent));
}

template <typename FeatureClientBuilderType>
ACSDK_INLINE_VISIBILITY inline bool SDKClientRegistry::addFeature(
    std::unique_ptr<FeatureClientBuilderType> featureBuilder) {
    internal::AssertFeatureClientBuilderTypeIsValid<FeatureClientBuilderType>();

    const auto typeIndex =
        avsCommon::utils::getTypeIndex<typename std::result_of<decltype (&FeatureClientBuilderType::construct)(
            FeatureClientBuilderType, const std::shared_ptr<SDKClientRegistry>&)>::type::element_type>();
    auto constructFn = std::bind(&FeatureClientBuilderType::construct, featureBuilder.get(), std::placeholders::_1);
    return addFeature(typeIndex, std::move(featureBuilder), std::move(constructFn));
}

}  // namespace sdkClient
}  // namespace alexaClientSDK
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_SDKCLIENTREGISTRY_IMPL_H_
