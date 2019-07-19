/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "AVSCommon/Utils/HTTP2/HTTP2GetMimeHeadersResult.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

const HTTP2GetMimeHeadersResult HTTP2GetMimeHeadersResult::PAUSE{HTTP2SendStatus::PAUSE, {}};
const HTTP2GetMimeHeadersResult HTTP2GetMimeHeadersResult::COMPLETE{HTTP2SendStatus::COMPLETE, {}};
const HTTP2GetMimeHeadersResult HTTP2GetMimeHeadersResult::ABORT{HTTP2SendStatus::ABORT, {}};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
