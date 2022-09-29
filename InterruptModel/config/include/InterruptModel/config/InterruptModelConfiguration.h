/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ALEXA_CLIENT_SDK_INTERRUPTMODEL_CONFIG_INCLUDE_INTERRUPTMODEL_CONFIG_INTERRUPTMODELCONFIGURATION_H_
#define ALEXA_CLIENT_SDK_INTERRUPTMODEL_CONFIG_INCLUDE_INTERRUPTMODEL_CONFIG_INTERRUPTMODELCONFIGURATION_H_

#include <string>
#include <memory>
#include <sstream>

#include <AVSCommon/Utils/SDKConfig.h>

#if defined(_MSC_VER) && defined(ACSDK_CONFIG_SHARED_LIBS)
#if defined(IN_INTERRUPTMODEL)
#define interruptmodel_EXPORT __declspec(dllexport)
#else
#define interruptmodel_EXPORT __declspec(dllimport)
#endif
#else
#define interruptmodel_EXPORT
#endif

namespace alexaClientSDK {
namespace afml {
namespace interruptModel {

/**
 * This class contains the interrupt model configuration
 * for the device. It contains channel priorities
 * as well as the interrupt model for interactions
 * between these channels.
 * clients may also add their own virtual channels
 * at a configurable priority and define the interactions
 * of these virtual channels with other channels
 * by extending the interrupt model.
 */
class InterruptModelConfiguration {
public:
    /**
     * String that contains the interrupt Model configuration for platforms that support ducking
     */
    static interruptmodel_EXPORT std::string configurationJsonSupportsDucking;

    /**
     * String that contains the interrupt Model configuration for platforms that don't support ducking
     */
    static interruptmodel_EXPORT std::string configurationJsonDuckingNotSupported;

    /**
     * API that allows the clientApplication to accept
     * the interrupt model configuration during construction/initialization
     * @param supportsDucking optional :  flag to indicate if the platform is capable of supporting ducking
     * @return istream containing the configuration Json to be used by SampleApp
     */
    static interruptmodel_EXPORT std::unique_ptr<std::istream> getConfig(bool supportsDucking = true) {
        if (supportsDucking)
            return std::unique_ptr<std::istringstream>(new std::istringstream(configurationJsonSupportsDucking));
        else
            return std::unique_ptr<std::istringstream>(new std::istringstream(configurationJsonDuckingNotSupported));
    }

private:
    /**
     * Disallow explicit object construction for this static singleton class
     */
    InterruptModelConfiguration() {
    }
};

}  // namespace interruptModel
}  // namespace afml
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_INTERRUPTMODEL_CONFIG_INCLUDE_INTERRUPTMODEL_CONFIG_INTERRUPTMODELCONFIGURATION_H_
