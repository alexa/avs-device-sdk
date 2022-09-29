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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAREMOTEVIDEOPLAYERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAREMOTEVIDEOPLAYERHANDLER_H_

#include <memory>
#include <mutex>
#include <string>

#include <acsdk/AlexaRemoteVideoPlayerInterfaces/RemoteVideoPlayerInterface.h>
#include <acsdk/Sample/Endpoint/EndpointFocusAdapter.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of a Remote Video Player.
 */
class EndpointAlexaRemoteVideoPlayerHandler : public alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface {
public:
    /**
     * Create an @c EndpointAlexaRemoteVideoPlayerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @param focusAdapter The @c EndpointFocusAdapter to manage focus.
     * @return A pointer to a new @c EndpointAlexaRemoteVideoPlayerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaRemoteVideoPlayerHandler> create(
        std::string endpointName,
        std::shared_ptr<EndpointFocusAdapter> focusAdapter);

    /// @name RemoteVideoPlayerInterface methods
    /// @{
    alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface::Response playVideo(
        std::unique_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerRequest> request) override;
    alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerInterface::Response displaySearchResults(
        std::unique_ptr<alexaRemoteVideoPlayerInterfaces::RemoteVideoPlayerRequest> request) override;
    alexaRemoteVideoPlayerInterfaces::Configuration getConfiguration() override;
    /// @}

    /**
     * Actions performed when focus has been acquired. No OP in this stub implementation.
     * Should invoke the video application to either start video playback or display search results.
     */
    void onFocusAcquired();

private:
    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     * @param focusAdapter The @c EndpointFocusAdapter to manage focus.
     */
    EndpointAlexaRemoteVideoPlayerHandler(std::string endpointName, std::shared_ptr<EndpointFocusAdapter> focusAdapter);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// The focus adapter
    std::shared_ptr<EndpointFocusAdapter> m_focusAdapter;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAREMOTEVIDEOPLAYERHANDLER_H_
