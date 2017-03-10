/*
 * DirectiveHandlerConfiguration.h
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

#ifndef ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_CONFIGURATION_H_
#define ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_CONFIGURATION_H_

#include <map>
#include <memory>
#include <utility>

#include <AVSCommon/AVSMessage.h>
#include "ADSL/BlockingPolicy.h"
#include "ADSL/DirectiveHandlerInterface.h"

namespace alexaClientSDK {
namespace adsl {

/// @c AVSMessage namespace and name pair for associating types of directives with a handler and blocking policy.
using NamespaceAndNamePair = std::pair<std::string, std::string>;

/// Pair combining A DirectiveHandler and a BlockingPolicy in that order.
using DirectiveHandlerAndBlockingPolicyPair = std::pair<std::shared_ptr<DirectiveHandlerInterface>, BlockingPolicy>;

/// Mapping from (namespace,name) pairs to (handler,policy) pairs.
using DirectiveHandlerConfiguration = std::map<NamespaceAndNamePair, DirectiveHandlerAndBlockingPolicyPair>;

} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_CONFIGURATION_H_
