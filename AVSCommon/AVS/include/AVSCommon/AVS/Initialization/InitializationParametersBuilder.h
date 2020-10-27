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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_INITIALIZATIONPARAMETERSBUILDER_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_INITIALIZATIONPARAMETERSBUILDER_H_

#include "AVSCommon/AVS/Initialization/InitializationParameters.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {
namespace initialization {

/** A class to contain the various parameters that are needed to initialize @c AlexaClientSDKInit.
 */
class InitializationParametersBuilder {
public:
    /**
     * Creates an instance of the @c InitializationParametersBuilder.
     *
     * @return A ptr to this builder.
     */
    static std::unique_ptr<InitializationParametersBuilder> create();

    /**
     * Add JSON streams.
     *
     * @param jsonStreams.
     * @return This instance of the builder.
     */
    InitializationParametersBuilder& withJsonStreams(
        const std::shared_ptr<std::vector<std::shared_ptr<std::istream>>>& jsonStreams);

    /**
     * Add @c PowerResourceManagerInterface.
     *
     * @param powerResourceManager The @c PowerResourceManagerInterface.
     * @return This instance of the builder.
     */
    InitializationParametersBuilder& withPowerResourceManager(
        const std::shared_ptr<avsCommon::sdkInterfaces::PowerResourceManagerInterface>& powerResourceManager);

    /**
     * Add @c TimerDelegateFactoryInterface.
     *
     * @param timerDelegateFactory The @c TimerDelegateFactoryInterface.
     * @return This instance of the builder.
     */
    InitializationParametersBuilder& withTimerDelegateFactory(
        const std::shared_ptr<avsCommon::sdkInterfaces::timing::TimerDelegateFactoryInterface>& timerDelegateFactory);

    /**
     * Build the @c InitializationParameters object.
     *
     * @return If valid, an @c InitializationParameters object, otherwise nullptr.
     */
    std::unique_ptr<InitializationParameters> build();

private:
    /// Constructor.
    InitializationParametersBuilder();

    /// The InitializationParameters.
    InitializationParameters m_initParams;
};

}  // namespace initialization
}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_INITIALIZATION_INITIALIZATIONPARAMETERSBUILDER_H_
