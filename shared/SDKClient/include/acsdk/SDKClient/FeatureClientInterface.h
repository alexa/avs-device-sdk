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

#ifndef ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_FEATURECLIENTINTERFACE_H_
#define ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_FEATURECLIENTINTERFACE_H_
#include <memory>
#include <string>

#include <AVSCommon/Utils/RequiresShutdown.h>

#include "acsdk/SDKClient/internal/TypeRegistry.h"

namespace alexaClientSDK {
namespace sdkClient {

// Forward declaration
class SDKClientRegistry;

/**
 * The FeatureClientInterface must be implemented by feature clients which expose additional capabilities to
 * applications utilizing the AVS SDK. To allow feature clients to be added to the @c SDKClientRegistry (directly or
 * through the @c SDKClientBuilder) a matching feature client builder which implements the
 * @c FeatureClientBuilderInterface and which constructs the feature client should be implemented.
 */
class FeatureClientInterface : public avsCommon::utils::RequiresShutdown {
public:
    /**
     * Constructor
     * @param name The name of the class
     */
    FeatureClientInterface(const std::string& name);

    /**
     * Perform any addition configuration on the client, called after all clients have been built successfully.
     * @param sdkClientRegistry Reference to the SDKClientRegistry
     * @return true if successful
     */
    virtual bool configure(const std::shared_ptr<SDKClientRegistry>& sdkClientRegistry) = 0;

    /// Destructor
    virtual ~FeatureClientInterface() = default;
};

inline FeatureClientInterface::FeatureClientInterface(const std::string& name) :
        avsCommon::utils::RequiresShutdown(name) {
}
}  // namespace sdkClient
}  // namespace alexaClientSDK
#endif  // ACSDK_SDKCLIENT_INCLUDE_ACSDK_SDKCLIENT_FEATURECLIENTINTERFACE_H_
