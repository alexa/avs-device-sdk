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

#ifndef ACSDKAUTHORIZATION_PRIVATE_LOGGING_H_
#define ACSDKAUTHORIZATION_PRIVATE_LOGGING_H_

#include <AVSCommon/Utils/Logger/Logger.h>

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param event The event string for this @c LogEntry.
 * @private
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

#endif  // ACSDKAUTHORIZATION_PRIVATE_LOGGING_H_
