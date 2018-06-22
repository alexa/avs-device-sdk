/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_STATEREFRESHPOLICY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_STATEREFRESHPOLICY_H_

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * An enum class used to specify the refresh policy for the state information provided by a @c stateProviderInterface.
 * The @c stateProviderInterface must specify the refresh policy when it updates its state via @c setState.
 */
enum class StateRefreshPolicy {
    /**
     * Indicates to the ContextManager that the state information need not be updated on a @c getContext request. The
     * @c stateProviderInterface is responsible for updating its state manually.
     */
    NEVER,

    /**
     * Indicates to the @c ContextManager that the stateProvider needs to be queried and the state refreshed every time
     * it processes a @c getContext request.
     */
    ALWAYS,

    /**
     * Indicates to the @c ContextManager that the stateProvider needs to be queried and the state refreshed every time
     * it processes a @c getContext request.  The stateProvider may choose to not report context by supplying an empty
     * @c jsonState via @c setState.
     */
    SOMETIMES
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_STATEREFRESHPOLICY_H_
