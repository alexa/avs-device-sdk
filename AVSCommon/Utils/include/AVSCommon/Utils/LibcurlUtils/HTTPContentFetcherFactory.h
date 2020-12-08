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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPCONTENTFETCHERFACTORY_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPCONTENTFETCHERFACTORY_H_

#include <memory>
#include <string>

#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterface.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlSetCurlOptionsCallbackFactoryInterface.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace libcurlUtils {

/**
 * A class that produces @c HTTPContentFetchers.
 */
class HTTPContentFetcherFactory : public avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface {
public:
    /**
     * Factory for creating instances of avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface
     *
     * @param setCurlOptionsCallbackFactory The @c LibcurlSetCurlOptionsCallbackFactoryInterface to set user defined
     * curl options.
     * @return A new instance of avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface.
     */
    static std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>
    createHTTPContentFetcherInterfaceFactoryInterface(
        const std::shared_ptr<LibcurlSetCurlOptionsCallbackFactoryInterface>& setCurlOptionsCallbackFactory = nullptr);

    /**
     * Constructor.
     *
     * @param setCurlOptionsCallbackFactory The optional @c LibcurlSetCurlOptionsCallbackFactoryInterface to set user
     * defined curl options
     */
    HTTPContentFetcherFactory(
        const std::shared_ptr<LibcurlSetCurlOptionsCallbackFactoryInterface>& setCurlOptionsCallbackFactory = nullptr);

    /// @name HTTPContentFetcherInterfaceFactoryInterface methods
    /// @{
    std::unique_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterface> create(const std::string& url) override;
    /// @}

private:
    /// The optional @c LibcurlSetCurlOptionsCallbackFactoryInterface to set user defined curl options.
    std::shared_ptr<LibcurlSetCurlOptionsCallbackFactoryInterface> m_setCurlOptionsCallbackFactory;
};

}  // namespace libcurlUtils
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_LIBCURLUTILS_HTTPCONTENTFETCHERFACTORY_H_
