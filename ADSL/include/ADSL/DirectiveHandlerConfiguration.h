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

#include <memory>
#include <unordered_map>

#include "ADSL/DirectiveHandlerInterface.h"
#include "ADSL/HandlerAndPolicy.h"
#include "ADSL/NamespaceAndName.h"

namespace alexaClientSDK {
namespace adsl {

/**
 * Map of @c NamespaceAndName values to @c HandlerAndPolicy values with which to register @c DirectiveHandlers with a
 * @c DirectiveSequencer.
 */
using DirectiveHandlerConfiguration = std::unordered_map<NamespaceAndName, HandlerAndPolicy>;

} // namespace adsl
} // namespace alexaClientSDK

#endif // ALEXA_CLIENT_SDK_ADSL_INCLUDE_ADSL_DIRECTIVE_HANDLER_CONFIGURATION_H_
