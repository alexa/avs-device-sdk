/*
 * Copyright 2018-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_NETWORK_INTERNETCONNECTIONMONITOR_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_NETWORK_INTERNETCONNECTIONMONITOR_H_

#include <chrono>
#include <memory>
#include <mutex>
#include <unordered_set>

#include <AVSCommon/AVS/Attachment/InProcessAttachment.h>
#include <AVSCommon/AVS/Attachment/InProcessAttachmentReader.h>
#include <AVSCommon/SDKInterfaces/HTTPContentFetcherInterfaceFactoryInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionMonitorInterface.h>
#include <AVSCommon/SDKInterfaces/InternetConnectionObserverInterface.h>
#include <AVSCommon/Utils/Timing/Timer.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace network {

/**
 * A class to monitor internet connection and notify observers of connection status changes.
 */
class InternetConnectionMonitor : public sdkInterfaces::InternetConnectionMonitorInterface {
public:
    /**
     * Creates a InternetConnectionMonitor.
     *
     * @param contentFetcherFactory The content fetcher that will make the test run to an S3 endpoint.
     * @return A unique_ptr to the InternetConnectionMonitor instance.
     */
    static std::unique_ptr<InternetConnectionMonitor> create(
        std::shared_ptr<sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

    /**
     * Destructor.
     */
    ~InternetConnectionMonitor();

    /// @name InternetConnectionMonitorInterface Methods
    /// @{
    void addInternetConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer) override;
    void removeInternetConnectionObserver(
        std::shared_ptr<avsCommon::sdkInterfaces::InternetConnectionObserverInterface> observer) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param contentFetcherFactory The content fetcher that will make the test run to an S3 endpoint.
     */
    InternetConnectionMonitor(
        std::shared_ptr<sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> contentFetcherFactory);

    /**
     * Begin monitoring internet connection.
     */
    void startMonitoring();

    /**
     * Stop monitoring internet connection.
     */
    void stopMonitoring();

    /**
     * Test internet connection by connecting to an S3 endpoint and fetching HTTP content.
     * The HTTP content is scanned for a validation string.
     *
     * @note The URL tested is http://spectrum.s3.amazonaws.com/kindle-wifi/wifistub.html, the Kindle reachability probe
     * page.
     */
    void testConnection();

    /**
     * Update the connection status.
     *
     * @param connected The new connection status.
     */
    void updateConnectionStatus(bool connected);

    /**
     * Notify observers of connection status.
     *
     * @note This should only be called while holding a lock to ensure synchronicity.
     */
    void notifyObserversLocked();

    /// The set of connection observers.
    std::unordered_set<std::shared_ptr<sdkInterfaces::InternetConnectionObserverInterface>> m_observers;

    /// The current internet connection status.
    bool m_connected;

    /// The period (in seconds) after which the monitor should re-test internet connection.
    std::chrono::seconds m_period;

    /// The timer that will call testConnection() every m_period seconds.
    avsCommon::utils::timing::Timer m_connectionTestTimer;

    /// A flag to tell the HTTP content fetcher that it is time to shutdown.
    std::atomic<bool> m_isShuttingDown;

    /// The stream that will hold downloaded data.
    std::shared_ptr<avsCommon::avs::attachment::InProcessAttachment> m_stream;

    /// The content fetcher factory that will produce a content fetcher.
    std::shared_ptr<avsCommon::sdkInterfaces::HTTPContentFetcherInterfaceFactoryInterface> m_contentFetcherFactory;

    /// Mutex to serialize access to m_connected and m_observers.
    std::mutex m_mutex;
};

}  // namespace network
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_UTILS_INCLUDE_AVSCOMMON_UTILS_NETWORK_INTERNETCONNECTIONMONITOR_H_
