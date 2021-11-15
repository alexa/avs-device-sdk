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

#ifndef ACSDKHTTPCONTENTFETCHER_HTTPCONTENTFETCHERCOMPONENT_H_
#define ACSDKHTTPCONTENTFETCHER_HTTPCONTENTFETCHERCOMPONENT_H_

#include <memory>

#include <acsdkManufactory/Annotated.h>
#include <acsdkManufactory/Component.h>
#include <acsdkManufactory/Import.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/Utils/LibcurlUtils/LibcurlSetCurlOptionsCallbackFactoryInterface.h>

namespace alexaClientSDK {
namespace acsdkHTTPContentFetcher {

/**
 * Manufactory Component definition for Libcurl implementation of HTTPContentFetcherInbterfaceFactoryInterface
 */
using HTTPContentFetcherComponent = acsdkManufactory::Component<
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface>,
    acsdkManufactory::Import<acsdkManufactory::Annotated<
        avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface,
        avsCommon::utils::libcurlUtils::LibcurlSetCurlOptionsCallbackFactoryInterface>>>;

/**
 * Get the default @c Manufactory component for creating instances of HTTPContentFetcherInterfaceFactoryInterface.
 *
 * @return The default @c Manufactory component for creating instances of HTTPContentFetcher.
 */
HTTPContentFetcherComponent getComponent();

}  // namespace acsdkHTTPContentFetcher
}  // namespace alexaClientSDK

#endif  // ACSDKHTTPCONTENTFETCHER_HTTPCONTENTFETCHERCOMPONENT_H_
