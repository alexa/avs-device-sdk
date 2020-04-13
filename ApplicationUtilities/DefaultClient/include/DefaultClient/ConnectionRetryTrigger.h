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

#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_CONNECTIONRETRYTRIGGER_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_CONNECTIONRETRYTRIGGER_H_

#include <memory>

#include <AIP/AudioInputProcessor.h>
#include <AVSCommon/SDKInterfaces/AudioInputProcessorObserverInterface.h>
#include <AVSCommon/SDKInterfaces/AVSConnectionManagerInterface.h>

namespace alexaClientSDK {
namespace defaultClient {

/**
 * Class used to wake up connection retries when the user tries to use the client.
 */
class ConnectionRetryTrigger : public avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface {
public:
    /**
     * Create a new ConnectionRetryTrigger.
     *
     * @param connectionManager The AVSConnectionManager instance to wake.
     * @param audioInputProcessor The AudioInputProcessor to listen to.
     * @return
     */
    static std::shared_ptr<ConnectionRetryTrigger> create(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
        std::shared_ptr<capabilityAgents::aip::AudioInputProcessor> audioInputProcessor);

private:
    /**
     * Constructor.
     *
     * @param connectionManager The AVSConnectionManager instance to wake.
     */
    ConnectionRetryTrigger(std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager);

    /// @name AudioInputProcessorObserverInterface methods.
    /// @{
    void onStateChanged(AudioInputProcessorObserverInterface::State state) override;
    /// @}

    /// The last state reported by AudioInputProcessor.
    avsCommon::sdkInterfaces::AudioInputProcessorObserverInterface::State m_state;

    /// The AVSConnectionManagerInstance to wake.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> m_connectionManager;
};

}  // namespace defaultClient
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_DEFAULTCLIENT_INCLUDE_DEFAULTCLIENT_CONNECTIONRETRYTRIGGER_H_
