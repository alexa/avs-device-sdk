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

#include <acsdkManufactory/ComponentAccumulator.h>

#include <AVSCommon/Utils/LibcurlUtils/HTTPContentFetcherFactory.h>

#include "acsdkHTTPContentFetcher/HTTPContentFetcherComponent.h"

namespace alexaClientSDK {
namespace acsdkHTTPContentFetcher {

using namespace acsdkManufactory;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::libcurlUtils;

/**
 * This function adapts an Annotated<HTTPContentFetcherInterfaceFactoryInterface,
 * LibcurlSetCurlOptionsCallbackFactoryInterface> to a shared_ptr<AVSConnectionManagerInterface>
 * and returns a HTTPContentFetcherInterfaceFactoryInterface.
 *
 * TODO: ACSDK-4957
 * @note This is only necessary because HTTPContentFetcherFactory is in AVSCommon and is prohibited
 * from using acsdkManufactory::Annotated<> because acsdkManufactory depends upon AVSCommon.  This will
 * be fixed when acsdkManufactory's depenency upon AVSCommon is removed.
 *
 * @param callbackFactory The Annotated<> curl options callback factory.
 * @return A HTTPContentFetcherInterfaceFactoryInterface.
 */
static std::shared_ptr<HTTPContentFetcherInterfaceFactoryInterface> createHTTPContentFetcherFactory(
    const Annotated<HTTPContentFetcherInterfaceFactoryInterface, LibcurlSetCurlOptionsCallbackFactoryInterface>&
        callbackFactory) {
    return HTTPContentFetcherFactory::createHTTPContentFetcherInterfaceFactoryInterface(callbackFactory);
}

HTTPContentFetcherComponent getComponent() {
    return ComponentAccumulator<>().addRetainedFactory(createHTTPContentFetcherFactory);
}

}  // namespace acsdkHTTPContentFetcher
}  // namespace alexaClientSDK
