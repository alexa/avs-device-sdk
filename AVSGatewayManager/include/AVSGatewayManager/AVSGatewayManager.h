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

#ifndef ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_AVSGATEWAYMANAGER_H_
#define ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_AVSGATEWAYMANAGER_H_

#include <memory>
#include <mutex>
#include <string>
#include <unordered_set>

#include <AVSCommon/SDKInterfaces/AVSGatewayAssignerInterface.h>
#include <AVSCommon/SDKInterfaces/AVSGatewayManagerInterface.h>
#include <AVSCommon/SDKInterfaces/PostConnectOperationProviderInterface.h>
#include <AVSCommon/Utils/Configuration/ConfigurationNode.h>
#include <AVSGatewayManager/Storage/AVSGatewayManagerStorageInterface.h>
#include <RegistrationManager/CustomerDataHandler.h>

namespace alexaClientSDK {
namespace avsGatewayManager {

/**
 * A class to manage the AVS Gateway the device is currently connected to.
 *
 * It also provides a method to execute the gateway verification sequence by creating the @c
 * PostConnectVerifyGatewaySender. Before establishing connection with AVS, clients must send the VerifyGateway event.
 * If the response to this event is a 204 the client is connected to the right endpoint. If the response is 200, the
 * client must connect to the new endpoint sent in the SetGateway directive.
 *
 * Note that the AVS gateway verification sequence should be performed only on fresh devices or after a factory reset.
 */
class AVSGatewayManager
        : public avsCommon::sdkInterfaces::AVSGatewayManagerInterface
        , public avsCommon::sdkInterfaces::PostConnectOperationProviderInterface
        , public registrationManager::CustomerDataHandler {
public:
    /**
     * Creates an instance of the @c AVSGatewayManager.
     *
     * @param avsGatewayManagerStorage The @c AVSGatewayManagerInterface to store avs gateway information.
     * @param customerDataManager The @c CustomerDataManager object that will track the CustomerDataHandler.
     * @param configurationRoot The @c ConfigurationNode to get AVS gateway information from the config file.
     * @return A new instance of the @c AVSGatewayManager.
     */
    static std::shared_ptr<AVSGatewayManager> create(
        std::shared_ptr<storage::AVSGatewayManagerStorageInterface> avsGatewayManagerStorage,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        const avsCommon::utils::configuration::ConfigurationNode& configurationRoot);

    /// @name AVSGatewayManagerInterface Functions
    /// @{
    bool setAVSGatewayAssigner(
        std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayAssignerInterface> avsGatewayAssigner) override;
    std::string getGatewayURL() const override;
    bool setGatewayURL(const std::string& avsGatewayURL) override;
    void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayObserverInterface> observer) override;
    void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayObserverInterface> observer) override;
    /// @}

    /// @name PostConnectOperationProviderInterface Functions
    /// @{
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface> createPostConnectOperation() override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

    /**
     * Callback method that will be called from @c PostConnectVerifyGatewaySender to signal successful verification of
     * AVS gateway
     *
     * @param verifyGatewaySender The pointer to the @c PostConnectVerifyGatewaySender.
     */
    void onGatewayVerified(
        const std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface>& verifyGatewaySender);

    /**
     * Destructor.
     */
    ~AVSGatewayManager();

private:
    /**
     * Constructor.
     *
     * @param avsGatewayManagerStorage The @c AVSGatewayManagerInterface to store avs gateway information.
     * @param customerDataManager The @c CustomerDataManager object that will track the CustomerDataHandler.
     * @param defaultGateway The default AVS Gateway URL to use.
     */
    AVSGatewayManager(
        std::shared_ptr<storage::AVSGatewayManagerStorageInterface> avsGatewayManagerStorage,
        std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
        const std::string& defaultGateway);

    /**
     * Initializes the @c AVSGatewayManager.
     *
     * @return True if initialization is successful, else false.
     */
    bool init();

    /**
     * Saves the current state to database.
     *
     * Note: This method is not thread safe.
     * @return True if save is successful, else false.
     */
    bool saveStateLocked();

    /// The AVS Gateway Manager storage.
    std::shared_ptr<storage::AVSGatewayManagerStorageInterface> m_avsGatewayStorage;

    /// The AVS Gateway Assigner.
    std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayAssignerInterface> m_avsGatewayAssigner;

    /// The mutex to synchronize access to members.
    mutable std::mutex m_mutex;

    /// The current @c PostConnectVerifyGateway sender used to send the verify gateway event.
    std::shared_ptr<avsCommon::sdkInterfaces::PostConnectOperationInterface> m_currentVerifyGatewaySender;

    /// The current AVS Gateway Verification state.
    GatewayVerifyState m_currentState;

    /// The set of @c AVSGatewayObservers. Access is synchronized using m_mutex.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::AVSGatewayObserverInterface>> m_observers;
};

}  // namespace avsGatewayManager
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSGATEWAYMANAGER_INCLUDE_AVSGATEWAYMANAGER_AVSGATEWAYMANAGER_H_
