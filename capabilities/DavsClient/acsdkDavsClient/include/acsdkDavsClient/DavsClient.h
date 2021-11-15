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

#ifndef ACSDKDAVSCLIENT_DAVSCLIENT_H_
#define ACSDKDAVSCLIENT_DAVSCLIENT_H_

#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionObserverInterface.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <unordered_map>
#include "DavsHandler.h"
#include "acsdkDavsClientInterfaces/DavsEndpointHandlerInterface.h"

namespace alexaClientSDK {
namespace acsdkAssets {
namespace davs {

class DavsClient
        : public davsInterfaces::ArtifactHandlerInterface
        , public alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionObserverInterface {
public:
    ~DavsClient() override = default;

    /**
     * Creates a DAVS Client given a working directory.
     *
     * @param workingDirectory REQUIRED, place to store the temporary davs files, will be wiped on initialization.
     * @param authDelegate REQUIRED, the Authentication Delegate to generate the authentication token
     * @param wifiMonitor REQUIRED, the InternetConnectionMonitor that Davs client will subscribe to. It will check to
     * see if we are connected to the internet or not and notify the DAVS client.
     * @param davsEndpointHandler REQUIRED, the endpoint handler which is used to generate the proper DAVS request URL.
     * @param metricRecorder OPTIONAL, metric recorder used for recording metrics.
     * @param forcedUpdateInterval OPTIONAL, sets the update interval for all artifact to a specific value. This is to
     * be use only for testing! your devices will be throttled if used in production!
     * @return NULLABLE, new DAVS Client
     */
    static std::shared_ptr<DavsClient> create(
            std::string workingDirectory,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::InternetConnectionMonitorInterface> wifiMonitor,
            std::shared_ptr<davsInterfaces::DavsEndpointHandlerInterface> davsEndpointHandler,
            std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr,
            std::chrono::seconds forcedUpdateInterval = std::chrono::seconds(0));

    /**
     * @return true if the device is idle
     */
    bool getIdleState() const;

    void setIdleState(bool idleState);

    /// @name ArtifactHandlerInterface Functions
    /// @{
    std::string registerArtifact(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            std::shared_ptr<davsInterfaces::DavsDownloadCallbackInterface> downloadCallback,
            std::shared_ptr<davsInterfaces::DavsCheckCallbackInterface> checkCallback,
            bool downloadImmediately) override;
    void deregisterArtifact(const std::string& requestUUID) override;
    std::string downloadOnce(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            std::shared_ptr<davsInterfaces::DavsDownloadCallbackInterface> downloadCallback,
            std::shared_ptr<davsInterfaces::DavsCheckCallbackInterface> checkCallback) override;
    void enableAutoUpdate(const std::string& requestUUID, bool enable) override;
    /// @}

    /**
     * @name InternetConnectionObserverInterface functions
     */
    void onConnectionStatusChanged(bool connected) override;

    /***
     * Information from parsed Json Artifact Push Notification
     */
    struct ArtifactGroup {
        std::string type;
        std::string key;
    };

    /***
     * Take a json containing list of artifact group
     * Parse the json. E.g:{"artifactList":[{"type":"test","key":"tar"}]}
     * Then check and update the corresponding artifact
     * @param jsonArtifactList
     */
    void checkAndUpdateArtifactGroupFromJson(const std::string& jsonArtifactList);

    /***
     * Take a vector of ArtifactGroup and iterate every Artifact to check and then update
     * @param artifactVector containing artifact requesting to be updated
     */
    void checkAndUpdateArtifactGroupVector(const std::vector<DavsClient::ArtifactGroup>& artifactVector);

private:
    DavsClient(
            std::string workingDirectory,
            std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> authDelegate,
            std::shared_ptr<davsInterfaces::DavsEndpointHandlerInterface> davsEndpointHandler,
            std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder = nullptr,
            std::chrono::seconds forcedUpdateInterval = std::chrono::seconds(0));

    std::string handleRequest(
            std::shared_ptr<commonInterfaces::DavsRequest> artifactRequest,
            std::shared_ptr<davsInterfaces::DavsDownloadCallbackInterface> downloadCallback,
            std::shared_ptr<davsInterfaces::DavsCheckCallbackInterface> checkCallback,
            bool enableAutoUpdate,
            bool downloadImmediately);

    /***
     * Check below if an artifact requesting update before do it:
     * + Still relevant
     * + Enabled update
     * + Registered
     * @param artifactGroup that is requesting an update
     */
    void executeUpdateRegisteredArtifact(const ArtifactGroup& artifactGroup);

    /**
     * Utility: Parse the json from AIPC
     * @param jsonArtifactList from APIC callback value
     * @return Artifact Group Vector
     */
    std::vector<ArtifactGroup> executeParseArtifactGroupFromJson(const std::string& jsonArtifactList);

private:
    const std::string m_workingDirectory;

    /// AuthDelegate that curlWrapper will use to get the Authentication Token
    const std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::AuthDelegateInterface> m_authDelegate;

    /// Endpoint handler which is used to generate the proper DAVS request URL.
    const std::shared_ptr<davsInterfaces::DavsEndpointHandlerInterface> m_davsEndpointHandler;

    /// Metric Recorder used for recording metrics if provided.
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> m_metricRecorder;

    /// If set above 0, then this interval will be used to override the update polling interval dicated by the cloud TTL
    const std::chrono::seconds m_forcedUpdateInterval;

    /// PowerResource used to acquire/release the wakelock
    const std::shared_ptr<alexaClientSDK::avsCommon::utils::power::PowerResource> m_powerResource;

    /// Map of Request UUID to Davs Handlers
    std::unordered_map<std::string, std::shared_ptr<DavsHandler>> m_handlers;

    alexaClientSDK::avsCommon::utils::threading::Executor m_executor;

    bool m_isDeviceIdle;
    bool m_isConnected;
};

}  // namespace davs
}  // namespace acsdkAssets
}  // namespace alexaClientSDK

#endif  // ACSDKDAVSCLIENT_DAVSCLIENT_H_
