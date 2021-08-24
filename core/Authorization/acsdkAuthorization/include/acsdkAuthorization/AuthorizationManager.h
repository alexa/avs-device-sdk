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

#ifndef ACSDKAUTHORIZATION_AUTHORIZATIONMANAGER_H_
#define ACSDKAUTHORIZATION_AUTHORIZATIONMANAGER_H_

#include <memory>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <string>

#include <acsdkAuthorization/AuthorizationManagerStorage.h>
#include <acsdkAuthorizationInterfaces/AuthorizationAuthorityInterface.h>
#include <acsdkAuthorizationInterfaces/AuthorizationManagerInterface.h>
#include <AVSCommon/SDKInterfaces/AuthDelegateInterface.h>
#include <AVSCommon/SDKInterfaces/Storage/MiscStorageInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <RegistrationManager/CustomerDataHandler.h>
#include <RegistrationManager/CustomerDataManagerInterface.h>

namespace alexaClientSDK {
namespace acsdkAuthorization {

/**
 * This class allows for runtime switching between different authorization mechanisms.
 * It implements the following interfaces:
 * - @c AuthorizationManagerInterface, which is used for communication and operation with the adapter.
 * - @c AuthorizationAuthorityInterface, which contains APIs that that can be used to understand general authorization
 * state.
 *
 * To integrate an @c AuthorizationAdapterInterface with this class, first it must be added via the
 * @c AuthorizationManagerInterface::add method. Then, adapters can report state changes via
 * @c AuthorizationManagerInterface::reportStateChange. When reporting state, the unique identifer for the adapter must
 * be sent. Additionally, it is advised to attach a userId that represents the customer. This is currently optional, but
 * allows for strong protection of customer data. AuthorizationManager will initiate a logout in cases where ids
 * mismatch.
 *
 * Logout:
 *
 * @c AuthorizationAdapterInterface instances should not extend @c CustomerDataHandler. Instead,
 * expect an @c AuthorizationAdapterInterface::reset for when persisted user data should be cleared.
 *
 * States:
 *
 * AuthObserverInterface::State::UNINITIALIZED
 *
 * This is an uninitialized state.
 *
 * AuthObserverInterface::State::AUTHORIZING
 *
 * Upon first authorization, the state must be reported. This tells
 * @c AuthorizationManager that a new authorization is occuring, and will be used for future token requests
 * once @c AuthObserverInterface::State::REFRESHED is reported.
 *
 * Implicit Logout:
 * For convenience, when switching between authorizations, a non-active adapter can force logout by sending a
 * reportStateChange with AuthObserverInterface::State::AUTHORIZING. This will signal to the @c AuthorizationManager
 * that a new adapter wishes to authorize, and implicitly log the system out.
 *
 * AuthObserverInterface::State::REFRESHED
 *
 * This state indicates a successful authorization, and indicates the @c AuthorizationManager
 * will respond to getAuthToken() requests by querying the adapter. Sending this with a non-active authorization
 * will cause a logout. This restriction is to ensure
 * authorization state consistency is maintained and that customer data is protected.
 * The active adapter's id and user id is persisted and will be used to validate future reportStateChange calls.
 *
 * AuthObserverInterface::State::EXPIRED
 *
 * This state indicates the token has expired.
 *
 * AuthObserverItnerface::State::UNRECOVERABLE_ERROR
 *
 * This should be reserved for unrecoverable errors, and will cause AuthorizationManager to force a logout.
 *
 * AuthorizationManager currently implements CustomDataHandler and subscribes itself to RegistrationManager.
 * This is because RegistrationManager still acts as the main implementation of RegistrationManagerInterface in
 * the AVS SDK.
 */
class AuthorizationManager
        : public acsdkAuthorizationInterfaces::AuthorizationManagerInterface
        , public acsdkAuthorizationInterfaces::AuthorizationAuthorityInterface
        , public avsCommon::utils::RequiresShutdown
        , public registrationManager::CustomerDataHandler
        , public std::enable_shared_from_this<AuthorizationManager> {
public:
    /**
     * Creates an instance of @c AuthorizationManager.
     *
     * @param storage The storage interface.
     * @param customerDataManager A @c CustomerDataManagerInterface instance to initiate logout.
     *
     * @return AuthorizationManager. Will return a nullptr if creation fails.
     */
    static std::shared_ptr<acsdkAuthorization::AuthorizationManager> create(
        const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage,
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager);

    /// @name AuthorizationManagerInterface Functions
    /// @{
    void reportStateChange(
        const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& state,
        const std::string& authId,
        const std::string& userId) override;
    void add(const std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationAdapterInterface>& adapter) override;
    /// @}

    /// @name AuthorizationAuthorityInterface Functions
    /// @{
    void logout() override;
    avsCommon::sdkInterfaces::AuthObserverInterface::State getState() override;
    std::string getActiveAuthorization() override;
    /// @}

    /// @name AuthDelegateInterface Functions
    /// @{
    void addAuthObserver(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> observer) override;
    void removeAuthObserver(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> observer) override;
    std::string getAuthToken() override;
    void onAuthFailure(const std::string& token) override;
    /// @}

    /// @name CustomerDataHandler Functions
    /// @{
    void clearData() override;
    /// @}

    /**
     * Set the @c RegistrationManagerInterface instance.
     *
     * @param registrationManager The object used to logout.
     */
    void setRegistrationManager(
        const std::shared_ptr<registrationManager::RegistrationManagerInterface> registrationManager);

    // @name RequiresShutdown Functions
    /// @{
    void doShutdown() override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param storage The storage interface.
     * @param customerDataManager A @c CustomerDataManagerInterface instance to initiate logout.
     */
    AuthorizationManager(
        const std::shared_ptr<AuthorizationManagerStorage>& storage,
        const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager);

    /**
     * Initializes.
     *
     * @return Whether intialization was successful.
     */
    bool init();

    /**
     * A helper function for handling the state transition of AuthorizationManager.
     *
     * @param newState The new state reported by an adapter.
     * @param authId The id of the adapter.
     * @param userId The userId of the customer.
     */
    void handleTransition(
        const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& newState,
        const std::string& authId,
        const std::string& userId);

    /**
     * Clear the storage and reset various cached states.
     */
    void clearDataLocked();

    /**
     * Initiates a logout sequence. This takes in a lock and unlocks before calling.
     * Once AuthorizationManager replaces RegistrationManager, this is no longer needed
     * as there is no need for AuthorizationManager to implement CustomDataHandler.
     *
     * @param lock Need to unlock before calling logout on RegistrationManager.
     */
    void logoutHelper(std::unique_lock<std::mutex>& lock);

    /**
     * Sets the state.
     *
     * @param The desired state.
     */
    void setStateLocked(const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& newState);

    /**
     * Set the active adapter pointer, the activeAdapterId and the userId.
     *
     * @param adapterId The id of the adapter.
     * @param userId The id of the user.
     */
    void setActiveLocked(const std::string& adapterId, const std::string& userId);

    /**
     * Persist information into storage.
     *
     * @param adapterId The id of the adapter.
     * @param userId The id of the user.
     */
    void persist(const std::string& adapterId, const std::string& userId);

    /// Mutex to protect private members.
    std::mutex m_mutex;

    /**
     * A separate mutex used to protect AuthObserverInterface. By implementing both
     * RegistrationManagerInterface and AuthDelegateInterface, there are objects which may call protected
     * methods in AuthDelegateInterface when logging out. This prevents deadlocks from occurring.
     */
    std::mutex m_observersMutex;

    /// Collection of @c AuthorizationAdapterInterface.
    std::unordered_map<std::string, std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationAdapterInterface>>
        m_adapters;

    /// Colleciton of @c AuthObserverInterface.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface>> m_observers;

    /// Active Adapter.
    std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationAdapterInterface> m_activeAdapter;

    /// Active adaterId.
    std::string m_activeAdapterId;

    /// Active UserId
    std::string m_activeUserId;

    /// The current authorization state.
    avsCommon::sdkInterfaces::AuthObserverInterface::FullState m_authState;

    /// The storage interface for storing information.
    std::shared_ptr<AuthorizationManagerStorage> m_storage;

    /// RegistrationManager Instance.
    std::shared_ptr<registrationManager::RegistrationManagerInterface> m_registrationManager;

    /// An executor used for serializing requests on the AuthorizationManager's own thread of execution.
    avsCommon::utils::threading::Executor m_executor;
};

}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK

#endif  // ACSDKAUTHORIZATION_AUTHORIZATIONMANAGER_H_
