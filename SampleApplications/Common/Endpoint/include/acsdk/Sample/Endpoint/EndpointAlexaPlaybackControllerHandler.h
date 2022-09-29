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

#ifndef ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAPLAYBACKCONTROLLERHANDLER_H_
#define ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAPLAYBACKCONTROLLERHANDLER_H_

#include <memory>
#include <mutex>
#include <set>
#include <string>

#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerInterface.h>
#include <acsdkAlexaPlaybackControllerInterfaces/AlexaPlaybackControllerObserverInterface.h>
#include <acsdkAlexaPlaybackControllerInterfaces/PlaybackOperation.h>
#include <acsdk/Notifier/Notifier.h>
#include <acsdk/Sample/Endpoint/EndpointFocusAdapter.h>
#include <AVSCommon/Utils/Threading/Executor.h>

namespace alexaClientSDK {
namespace sampleApplications {
namespace common {

/**
 * Sample implementation of an @c AlexaPlaybackControllerInterface.
 */

class EndpointAlexaPlaybackControllerHandler
        : public acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface {
public:
    /**
     * Create a AlexaPlaybackControllerHandler object.
     *
     * @param endpointName The name of the endpoint.
     * @param focusAdapter The @c EndpointFocusAdapter to manage focus.
     * @return A pointer to a new @c AlexaPlaybackControllerHandler object if it succeeds; otherwise, @c nullptr.
     */
    static std::shared_ptr<EndpointAlexaPlaybackControllerHandler> create(
        const std::string& endpointName,
        std::shared_ptr<EndpointFocusAdapter> focusAdapter);

    /// @name AlexaPlaybackControllerInterface methods
    /// @{
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response play() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response pause() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response stop() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response startOver() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response previous() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response next() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response rewind() override;
    acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerInterface::Response fastForward() override;
    acsdkAlexaPlaybackControllerInterfaces::PlaybackState getPlaybackState() override;
    bool addObserver(
        const std::weak_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerObserverInterface>& observer)
        override;
    void removeObserver(
        const std::weak_ptr<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerObserverInterface>& observer)
        override;
    std::set<acsdkAlexaPlaybackControllerInterfaces::PlaybackOperation> getSupportedOperations() override;
    /// @}

private:
    /// Alias for Notifier based on @c AlexaPlaybackControllerObserverInterface.
    using AlexaPlaybackControllerNotifier =
        notifier::Notifier<acsdkAlexaPlaybackControllerInterfaces::AlexaPlaybackControllerObserverInterface>;

    /**
     * Constructor.
     * @param endpointName The name of the endpoint the capability is associated.
     * @param focusAdapter The @c EndpointFocusAdapter to manage focus.
     */
    EndpointAlexaPlaybackControllerHandler(
        const std::string& endpointName,
        std::shared_ptr<EndpointFocusAdapter> focusAdapter);

    /**
     * Helper function to notify playback state change to the observers of @c PlaybackControllerObserverInterface.
     *
     * @param playbackState The changed playback state to be notified to the observer.
     */
    void notifyObservers(acsdkAlexaPlaybackControllerInterfaces::PlaybackState playbackState);

    /**
     * Helper function to log the playback information, for the purpose of verifying playback operations are
     * received by the handler.
     *
     * @param endpointName The name of the endpoint the capability is assoicated.
     * @param playbackOperation The playback operation @c PlaybackOperation to log.
     */
    static void logOperation(const std::string& endpointName, const std::string& playbackOperation);

    /// Mutex to serialize access to variables.
    std::mutex m_mutex;

    /// The name of the endpoint that this controller is associated.
    std::string m_endpointName;

    /// Pointer to the AlexaPlaybackController notifier.
    std::shared_ptr<AlexaPlaybackControllerNotifier> m_notifier;

    /// Current playback state of the endpoint.
    acsdkAlexaPlaybackControllerInterfaces::PlaybackState m_currentPlaybackState;

    /// The focus adapter
    std::shared_ptr<EndpointFocusAdapter> m_focusAdapter;
};

}  // namespace common
}  // namespace sampleApplications
}  // namespace alexaClientSDK

#endif  // ACSDK_SAMPLE_ENDPOINT_ENDPOINTALEXAPLAYBACKCONTROLLERHANDLER_H_
