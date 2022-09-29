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

#ifndef ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATIONLIFESPANTOTIMEOUTMAPPER_H_
#define ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATIONLIFESPANTOTIMEOUTMAPPER_H_

#include <chrono>
#include <memory>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationTypes.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

/**
 * Class which handles mappings from @c PresentationLifespan to durations
 */
class PresentationLifespanToTimeoutMapper {
public:
    /**
     * Create an instance of @c PresentationLifespanToTimeoutMapper
     * @return Pointer to @c PresentationLifespanToTimeoutMapper instance
     */
    static std::shared_ptr<PresentationLifespanToTimeoutMapper> create();

    /**
     * Return the timeout duration corresponding to a presentation lifespan
     * @param lifespan presentation lifespan
     * @return timeout duration
     */
    std::chrono::milliseconds getTimeoutDuration(
        const presentationOrchestratorInterfaces::PresentationLifespan& lifespan);

private:
    /**
     * Constructor
     */
    PresentationLifespanToTimeoutMapper();

    /**
     * Initializes the object by reading the values from configuration.
     */
    void initialize();

    /// The timeout value in ms for SHORT presentations
    std::chrono::milliseconds m_shortPresentationTimeout;

    /// The timeout value in ms for TRANSIENT presentations
    std::chrono::milliseconds m_transientPresentationTimeout;

    /// The timeout value in ms for LONG presentations
    std::chrono::milliseconds m_longPresentationTimeout;
};
}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
#endif  // ALEXA_CLIENT_SDK_AVS_SHARED_PRESENTATIONORCHESTRATOR_PRESENTATIONORCHESTRATORCLIENT_PRIVATEINCLUDE_ACSDK_PRESENTATIONORCHESTRATORCLIENT_PRIVATE_PRESENTATIONLIFESPANTOTIMEOUTMAPPER_H_
