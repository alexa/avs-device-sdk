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

#include <vector>

#include "ACL/Transport/TransportDefines.h"

namespace alexaClientSDK {
namespace acl {

/// Table with the retry times on subsequent retries.
const std::vector<int> TransportDefines::RETRY_TABLE = {
    250,    // Retry 1:  0.25s
    1000,   // Retry 2:  1.00s
    3000,   // Retry 3:  3.00s
    5000,   // Retry 4:  5.00s
    10000,  // Retry 5: 10.00s
    20000,  // Retry 6: 20.00s
};

/// Retry Timer object.
avsCommon::utils::RetryTimer TransportDefines::RETRY_TIMER(TransportDefines::RETRY_TABLE);

}  // namespace acl
}  // namespace alexaClientSDK
