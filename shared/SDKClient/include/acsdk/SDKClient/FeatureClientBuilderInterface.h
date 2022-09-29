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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_FEATURECLIENTBUILDERINTERFACE_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_FEATURECLIENTBUILDERINTERFACE_H_

#include <AVSCommon/Utils/PlatformDefinitions.h>

#include "FeatureClientInterface.h"

namespace alexaClientSDK {
namespace sdkClient {
/**
 * Interface to be implemented by Feature Client Builders
 *
 * @note During construction of the feature client builder any required types must be requested by calling
 * the @c addRequiredType<T> function exposed by the @c FeatureClientBuilderInterface.
 * @code
 *  MyFeature::MyFeature() {
 *      addRequiredType<RequiredType1>();
 *      addRequiredType<RequiredType2>();
 *  }
 * @endcode
 */
class FeatureClientBuilderInterface {
public:
    /**
     * Virtual destructor
     */
    virtual ~FeatureClientBuilderInterface() = default;

    /**
     * Retrieve the name of the client, used to get friendly names in case of errors
     * @return string containing the friendly name of the client
     */
    virtual std::string name() = 0;

protected:
    /**
     * Adds a type which is required for the construction of the client.
     * Types added in this way are guaranteed to be available when @c construct is called.
     * All required types should be registered in the constructor of the feature client.
     * @tparam ComponentType The type which is required
     */
    template <typename ComponentType>
    ACSDK_INLINE_VISIBILITY inline void addRequiredType();

private:
    /**
     * Returns the types required by this FeatureClientInterface - used by the @c SDKClientBuilder
     * @return TypeRegistry containing the required types
     */
    const internal::TypeRegistry& getRequiredTypes() {
        return m_requiredTypes;
    }

    /// TypeRegistry containing the types required by this FeatureClientInterface
    internal::TypeRegistry m_requiredTypes;

    /// SDKClientBuilder and SDKClientRegistry are friends to allow them to retrieve types
    friend class SDKClientBuilder;
    friend class SDKClientRegistry;
};

}  // namespace sdkClient
}  // namespace alexaClientSDK

#include "internal/FeatureClientBuilderInterface_impl.h"
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_FEATURECLIENTBUILDERINTERFACE_H_
