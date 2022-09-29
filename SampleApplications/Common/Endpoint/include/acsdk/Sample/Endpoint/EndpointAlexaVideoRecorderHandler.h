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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAVIDEORECORDERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAVIDEORECORDERHANDLER_H_

#include "acsdkAlexaVideoRecorderInterfaces/VideoRecorderInterface.h"

#include <memory>
#include <mutex>
#include <string>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c AlexaVideoRecorderInterface.
 */

class EndpointAlexaVideoRecorderHandler : public acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface {
public:
    /**
     * Create a EndpointRemoteVideoPlayerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @return A pointer to a new EndpointRemoteVideoPlayerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::unique_ptr<EndpointAlexaVideoRecorderHandler> create(const std::string endpointName);

    /// @name VideoRecorderHandlerInterface methods
    /// @{
    acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface::Response searchAndRecord(
        std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest> context) override;

    acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface::Response cancelRecording(
        std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest> context) override;

    acsdkAlexaVideoRecorderInterfaces::VideoRecorderInterface::Response deleteRecording(
        std::unique_ptr<acsdkAlexaVideoRecorderInterfaces::VideoRecorderRequest> context) override;

    bool isExtendedRecordingGUIShown() override;

    int getStorageUsedPercentage() override;
    /// @}

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     */
    EndpointAlexaVideoRecorderHandler(const std::string& endpointName);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAVIDEORECORDERHANDLER_H_
