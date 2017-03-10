/*
 * DirectiveRouter.h
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_ROUTER_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_ROUTER_H_

#include <set>

#include "ADSL/DirectiveHandlerConfiguration.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Mapping from (@c namespace, @c name) pairs to (@c BlockingPolicy, @c DirectiveHandlerInterface) pairs.
 */
class DirectiveRouter {
public:
    /**
     * Add mappings from from (@c namespace, @c name) pairs to (@c DirectiveHandler, @c BlockingPolicy) pairs.
     * The addition of a mapping for a (@c namespace, @c name) pair that already has a mapping replaces the
     * existing mapping.  The addition of mapping to (@c DirectiveHandler, @c BlockingPolicy) pairs with either
     * a nullptr @c DirectiveHandler or a @c BlockingPolicy of NONE removes the mapping for that
     * (@c namespace, @c name) pair.
     *
     * @param configuration The configuration to add to the current mapping.
     * @return The set of @c DirectiveHandlers that were removed from the configuration.
     */
    std::set<std::shared_ptr<DirectiveHandlerInterface>> setDirectiveHandlers(
            const DirectiveHandlerConfiguration& configuration);

    /**
     * Get the (@c DirectiveHandlerInterface, @c BlockingPolicy) pair for a given @c AVSDirective.
     *
     * @param directive The @c AVSDirective to get the pair for.
     * @return The (@cDirectiveHandler, @c BlockingPolicy) pair specified by calls to @c setDirectiveHandlers().
     * If there is no match, the returned @c DirectiveHandlerInterface will be @c nullptr and the BlockingPolicy
     * will be @c NONE.
     */
    DirectiveHandlerAndBlockingPolicyPair getDirectiveHandlerAndBlockingPolicy(
            std::shared_ptr<avsCommon::AVSDirective> directive);

    /**
     * Remove all configured handlers from the @c DirectiveRouter.
     *
     * @return The set of @c DirectiveHandlers that were removed from the configuration.
     */
    std::set<std::shared_ptr<DirectiveHandlerInterface>> clear();

private:
    /// Mapping from (namespace, name) pairs to (@c BlockingPolicy, @c DirectiveHandlerInterface) pairs.
    DirectiveHandlerConfiguration m_configuration;
};

} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_ROUTER_H_