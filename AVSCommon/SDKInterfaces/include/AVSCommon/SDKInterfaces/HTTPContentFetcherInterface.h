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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACE_H_

#include <memory>

#include <AVSCommon/Utils/HTTPContent.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/**
 * This class allows users to fetch content from remote location using HTTP(S).
 */
class HTTPContentFetcherInterface {
public:
    /// Represents what HTTP content to fetch.
    enum class FetchOptions {
        /// Retrieves the content type part of the HTTP header.
        CONTENT_TYPE,

        /// Retrieves the entire body of the remote location.
        ENTIRE_BODY
    };

    /**
     * Destructor.
     */
    virtual ~HTTPContentFetcherInterface() = default;

    /**
     * This function retrieves content from a remote location. No thread safety is guaranteed.
     *
     * @param option Flag indicating desired content.
     * @param writer An optional writer parameter to be used when writing to an external stream.
     * @return A new @c HTTPContent object or @c nullptr if a failure occured.
     */
    virtual std::unique_ptr<avsCommon::utils::HTTPContent> getContent(
        FetchOptions option,
        std::shared_ptr<avsCommon::avs::attachment::AttachmentWriter> writer = nullptr) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACE_H_
