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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_HANDLERANDPOLICY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_HANDLERANDPOLICY_H_

#include <memory>
#include "AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h"
#include "BlockingPolicy.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

/**
 * Conjoined @c DirectiveHandler and @c BlockingPolicy values.
 */
class HandlerAndPolicy {
public:
    /**
     * Constructor to initialize with default property values.
     */
    HandlerAndPolicy() = default;

    /**
     * Constructor to initialize with specific property values.
     *
     * @param handlerIn The @c AVSDirectiveHandlerInterface value for this instance.
     * @param policyIn The @c BlockingPolicy value for this instance.
     */
    HandlerAndPolicy(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handlerIn,
        BlockingPolicy policyIn);

    /**
     * Return whether this instance specifies a non-null directive handler and a non-NONE BlockingPolicy.
     *
     * @return Whether this instance specifies a non-null directive handler and a non-NONE BlockingPolicy.
     */
    operator bool() const;

    /// The @c DirectiveHandlerInterface value for this instance.
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerInterface> handler;

    /// The @c BlockingPolicy value for this instance.
    BlockingPolicy policy;
};

/**
 * == operator.
 *
 * @param lhs The HandlerAndPolicy instance on the left hand side of the == operation.
 * @param rhs The HandlerAndPolicy instance on the right hand side of the == operation.
 * @return Whether the @c lhs instance is equal to the @c rhs instance.
 */
bool operator==(const HandlerAndPolicy& lhs, const HandlerAndPolicy& rhs);

/**
 * != operator.
 *
 * @param lhs The HandlerAndPolicy instance on the left hand side of the == operation.
 * @param rhs The HandlerAndPolicy instance on the right hand side of the != operation.
 * @return Whether the @c lhs instance is NOT equal to the @c rhs instance.
 */
bool operator!=(const HandlerAndPolicy& lhs, const HandlerAndPolicy& rhs);

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_HANDLERANDPOLICY_H_
