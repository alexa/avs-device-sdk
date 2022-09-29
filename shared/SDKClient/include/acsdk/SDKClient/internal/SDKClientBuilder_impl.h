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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_SDKCLIENTBUILDER_IMPL_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_SDKCLIENTBUILDER_IMPL_H_
#include <AVSCommon/Utils/TypeIndex.h>

#include "Utils.h"

namespace alexaClientSDK {
namespace sdkClient {

template <typename FeatureClientBuilderType>
ACSDK_INLINE_VISIBILITY inline SDKClientBuilder& SDKClientBuilder::withFeature(
    std::unique_ptr<FeatureClientBuilderType> feature) {
    internal::AssertFeatureClientBuilderTypeIsValid<FeatureClientBuilderType>();

    auto featureTypeId =
        avsCommon::utils::getTypeIndex<typename std::result_of<decltype (&FeatureClientBuilderType::construct)(
            FeatureClientBuilderType, const std::shared_ptr<SDKClientRegistry>&)>::type::element_type>();
    auto constructFn = std::bind(&FeatureClientBuilderType::construct, feature.get(), std::placeholders::_1);
    auto client = std::shared_ptr<Client>(new Client{featureTypeId, std::move(feature), std::move(constructFn)});

    withFeature(std::move(client));

    return *this;
}
}  // namespace sdkClient
}  // namespace alexaClientSDK
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_SDKCLIENTBUILDER_IMPL_H_
