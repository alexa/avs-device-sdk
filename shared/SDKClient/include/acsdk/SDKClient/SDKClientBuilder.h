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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_SDKCLIENTBUILDER_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_SDKCLIENTBUILDER_H_

#include <list>
#include <memory>

#include <AVSCommon/Utils/PlatformDefinitions.h>
#include <AVSCommon/Utils/TypeIndex.h>

#include "FeatureClientBuilderInterface.h"
#include "FeatureClientInterface.h"
#include "SDKClientRegistry.h"

namespace alexaClientSDK {
namespace sdkClient {
/**
 * Class which takes a number of feature client builders and builds them to form a single SDKClientRegistry
 *
 * This class can be used as follows:
 * @code
 * auto clientBuilder1 = FeatureClientBuilder1::create(...);
 * auto clientBuilder2 = FeatureClientBuilder2::create(...);
 *
 * SDKClientBuilder builder;
 * auto sdkClient = builder.withFeature(client1).withFeature(client2).build();
 * auto client1 = sdkClient.get<FeatureClient1>();
 * auto client2 = sdkClient.get<FeatureClient2>();
 */
class SDKClientBuilder {
public:
    /**
     * Add the feature to this @c SDKClientBuilder
     * @param feature The pointer to the @c FeatureClientBuilderInterface to be added
     * @tparam FeatureClientBuilderType The type of the feature to add
     * @return The @c SDKClientBuilder object
     */
    template <typename FeatureClientBuilderType>
    ACSDK_INLINE_VISIBILITY inline SDKClientBuilder& withFeature(std::unique_ptr<FeatureClientBuilderType> feature);

    /**
     * Construct the SDKClientRegistry
     * After calling build() the @c SDKClientBuilder object should not be used
     * @return shared_ptr to the @c SDKClientRegistry, or nullptr if the build failed
     */
    std::shared_ptr<SDKClientRegistry> build();

private:
    /// Helper struct to store type and pointer to client
    struct Client {
        /// The type of the feature client created by the builder (not the type of the builder)
        avsCommon::utils::TypeIndex typeId;

        /// Pointer to the client builder object
        std::unique_ptr<FeatureClientBuilderInterface> client;

        /// Pointer to the construct function
        std::function<std::shared_ptr<FeatureClientInterface>(const std::shared_ptr<SDKClientRegistry>&)> constructorFn;
    };

    /**
     * Internal function to add a feature client to the SDKClientBuilder
     * @param client Pointer to the client object
     */
    void withFeature(std::shared_ptr<Client> client);

    /// The list of clients which have been added to this builder
    std::list<std::shared_ptr<Client>> m_clients;
};
}  // namespace sdkClient
}  // namespace alexaClientSDK

#include "acsdk/SDKClient/internal/SDKClientBuilder_impl.h"
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_SDKCLIENTBUILDER_H_
