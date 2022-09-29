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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_UTILS_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_UTILS_H_

#include "acsdk/SDKClient/FeatureClientBuilderInterface.h"

namespace alexaClientSDK {
namespace sdkClient {
namespace internal {
/// Helper struct to get type of non-annotated types
/// @tparam T The type of the component
template <typename T>
struct UnderlyingComponentType {
    typedef T type;
};

/// Specialization of component_type to identify underlying type of annotated types
/// @tparam Annotation The annotated name for the component
/// @tparam ComponentType The type of the underlying component
template <typename Annotation, typename ComponentType>
struct UnderlyingComponentType<Annotated<Annotation, ComponentType>> {
    typedef ComponentType type;
};

/**
 * Helper to determine if a type is a shared_ptr
 * @tparam T type to check
 */
template <typename T>
struct IsSharedPtr : std::false_type {};

/**
 * Specialization to determine if a type is a shared_ptr
 * @tparam T type to check
 */
template <typename T>
struct IsSharedPtr<std::shared_ptr<T>> : std::true_type {};

/**
 * Function to assert if the given FeatureClientBuilderType is valid for use in feature construction
 * @tparam FeatureClientBuilderType The type to validate
 */
template <typename FeatureClientBuilderType>
ACSDK_INLINE_VISIBILITY inline void AssertFeatureClientBuilderTypeIsValid() {
    static_assert(
        std::is_base_of<FeatureClientBuilderInterface, FeatureClientBuilderType>::value,
        "FeatureClientBuilderInterface must implement FeatureClientBuilderInterface");

    static_assert(
        internal::IsSharedPtr<typename std::result_of<decltype (&FeatureClientBuilderType::construct)(
            FeatureClientBuilderType, const std::shared_ptr<SDKClientRegistry>&)>::type>::value,
        "FeatureClientBuilder must define a construct(const std::shared_ptr<SDKClientRegistry>&) method which returns "
        "a shared_ptr");

    static_assert(
        std::is_base_of<
            FeatureClientInterface,
            typename std::result_of<decltype (&FeatureClientBuilderType::construct)(
                FeatureClientBuilderType, const std::shared_ptr<SDKClientRegistry>&)>::type::element_type>::value,
        "FeatureClientBuilder construct method must return a shared_ptr to a FeatureClientInterface implementation");
}

}  // namespace internal
}  // namespace sdkClient
}  // namespace alexaClientSDK
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_INTERNAL_UTILS_H_