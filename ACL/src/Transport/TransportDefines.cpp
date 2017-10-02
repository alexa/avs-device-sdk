/*
 * TransportDefines.cpp
 *
 * Copyright 2016-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "ACL/Transport/TransportDefines.h"

namespace alexaClientSDK {
namespace acl {

/// Table with the retry times on subsequent retries.
int TransportDefines::RETRY_TABLE[] = {
    250,    // Retry 1:  0.25s
    1000,   // Retry 2:  1.00s
    3000,   // Retry 3:  3.00s
    5000,   // Retry 4:  5.00s
    10000,  // Retry 5: 10.00s
    20000,  // Retry 6: 20.00s
};

/// Size of the retry table.
int TransportDefines::RETRY_TABLE_SIZE = (sizeof(RETRY_TABLE) / sizeof(RETRY_TABLE[0]));

/// Randomization used in the computation of retry times.
double TransportDefines::RETRY_RANDOMIZATION_FACTOR = 0.5;

double TransportDefines::RETRY_DECREASE_FACTOR = 1 / (TransportDefines::RETRY_RANDOMIZATION_FACTOR + 1);

/// Upper bound of the interval to distribute the retry duration.
double TransportDefines::RETRY_INCREASE_FACTOR = (TransportDefines::RETRY_RANDOMIZATION_FACTOR + 1);

/// Retry Timer object.
avsCommon::utils::RetryTimer TransportDefines::RETRY_TIMER(
    TransportDefines::RETRY_TABLE,
    TransportDefines::RETRY_TABLE_SIZE,
    TransportDefines::RETRY_DECREASE_FACTOR,
    TransportDefines::RETRY_INCREASE_FACTOR);

}  // namespace acl
}  // namespace alexaClientSDK
