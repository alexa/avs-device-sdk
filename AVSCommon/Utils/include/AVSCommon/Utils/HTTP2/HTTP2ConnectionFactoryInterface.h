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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2CONNECTIONFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2CONNECTIONFACTORYINTERFACE_H_

#include <memory>

#include "AVSCommon/Utils/HTTP2/HTTP2ConnectionInterface.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace http2 {

/**
 * Interface for creating instances of @c HTTP2ConnectionInterface.
 */
class HTTP2ConnectionFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~HTTP2ConnectionFactoryInterface() = default;

    /**
     * Create an instance of HTTP2ConnectionInterface.
     *
     * @return An instance of HTTP2ConnectionInterface or nullptr if the operation failed.
     */
    virtual std::shared_ptr<HTTP2ConnectionInterface> createHTTP2Connection() = 0;
};

}  // namespace http2
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_HTTP2_HTTP2CONNECTIONFACTORYINTERFACE_H_
