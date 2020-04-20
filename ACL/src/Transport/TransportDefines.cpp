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

#include <vector>

#include "ACL/Transport/TransportDefines.h"

namespace alexaClientSDK {
namespace acl {

/// Table with the retry times (in milliseconds) on subsequent retries.
const std::vector<int> TransportDefines::RETRY_TABLE = {250, 1000, 2000, 4000, 8000, 16000, 32000, 64000};

/// Retry Timer object.
avsCommon::utils::RetryTimer TransportDefines::RETRY_TIMER(TransportDefines::RETRY_TABLE);

}  // namespace acl
}  // namespace alexaClientSDK
