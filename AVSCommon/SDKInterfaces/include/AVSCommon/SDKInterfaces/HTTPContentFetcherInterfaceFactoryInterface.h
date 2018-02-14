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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACEFACTORYINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACEFACTORYINTERFACE_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {

/*
 * A factory interface class to produce @c HTTPContentFetcherInterface objects.
 */
class HTTPContentFetcherInterfaceFactoryInterface {
public:
    /**
     * Destructor.
     */
    virtual ~HTTPContentFetcherInterfaceFactoryInterface() = default;

    /**
     * Produces an @c HTTPContentFetcherInterface object or @c nullptr on failure.
     */
    virtual std::unique_ptr<HTTPContentFetcherInterface> create(const std::string& url) = 0;
};

}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_HTTPCONTENTFETCHERINTERFACEFACTORYINTERFACE_H_
