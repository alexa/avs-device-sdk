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

#ifndef ACSDKDAVSCLIENTINTERFACES_DAVSENDPOINTHANDLERINTERFACE_H_
#define ACSDKDAVSCLIENTINTERFACES_DAVSENDPOINTHANDLERINTERFACE_H_

#include <string>

#include "acsdkAssetsInterfaces/DavsRequest.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davsInterfaces {

/**
 * Interface for managing the URL generation for DAVS specific requests.
 */
class DavsEndpointHandlerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~DavsEndpointHandlerInterface() = default;

    /**
     * Generates the URL given the DAVS requests, will return an empty string upon failure.
     *
     * @param request DAVS request containing all the necessary information needed to identify a DAVS artifact.
     * @return a full valid URL string, empty upon failure.
     */
    virtual std::string getDavsUrl(std::shared_ptr<commonInterfaces::DavsRequest> request) = 0;
};

}  // namespace davsInterfaces
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKDAVSCLIENTINTERFACES_DAVSENDPOINTHANDLERINTERFACE_H_