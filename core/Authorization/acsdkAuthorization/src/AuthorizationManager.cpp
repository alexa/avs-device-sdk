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

#include <acsdkAuthorization/AuthorizationManager.h>
#include <AVSCommon/Utils/Logger/Logger.h>

/// String to identify log entries originating from this file.
static const std::string TAG{"AuthorizationManager"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace acsdkAuthorization {

using namespace acsdkAuthorizationInterfaces;
using namespace avsCommon::sdkInterfaces;

void AuthorizationManager::setRegistrationManager(
    const std::shared_ptr<registrationManager::RegistrationManagerInterface> regManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (!regManager) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullRegManager"));
    } else {
        m_registrationManager = regManager;
    }
}

std::shared_ptr<acsdkAuthorization::AuthorizationManager> AuthorizationManager::create(
    const std::shared_ptr<avsCommon::sdkInterfaces::storage::MiscStorageInterface>& storage,
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager) {
    ACSDK_DEBUG5(LX(__func__));

    if (!storage || !customerDataManager) {
        ACSDK_ERROR(
            LX("createFailed").d("isStorageNull", !storage).d("isCustomerDataManagerNull", !customerDataManager));
        return nullptr;
    }

    auto authMgrStorage = AuthorizationManagerStorage::create(storage);
    if (!authMgrStorage) {
        return nullptr;
    }

    auto authMgr = std::shared_ptr<AuthorizationManager>(new AuthorizationManager(authMgrStorage, customerDataManager));
    if (!authMgr->init()) {
        return nullptr;
    }

    return authMgr;
}

AuthorizationManager::AuthorizationManager(
    const std::shared_ptr<AuthorizationManagerStorage>& storage,
    const std::shared_ptr<registrationManager::CustomerDataManagerInterface>& customerDataManager) :
        RequiresShutdown{"AuthorizationManager"},
        CustomerDataHandler{customerDataManager},
        m_storage{storage} {
    ACSDK_DEBUG5(LX(__func__));
}

bool AuthorizationManager::init() {
    ACSDK_DEBUG5(LX(__func__));

    if (!m_storage->load(&m_activeAdapterId, &m_activeUserId)) {
        ACSDK_ERROR(LX(__func__));
        return false;
    }

    ACSDK_INFO(LX(__func__).d("activeAuthAdapter", m_activeAdapterId).sensitive("activeUserId", m_activeUserId));

    return true;
}

void AuthorizationManager::clearDataLocked() {
    ACSDK_DEBUG5(LX(__func__));

    if (m_activeAdapter) {
        m_activeAdapter->reset();
    }
    m_activeAdapter.reset();
    m_activeAdapterId.clear();
    m_activeUserId.clear();
    m_storage->clear();

    setStateLocked({AuthObserverInterface::State::UNINITIALIZED, AuthObserverInterface::Error::SUCCESS});
}

void AuthorizationManager::clearData() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    clearDataLocked();
}

void AuthorizationManager::reportStateChange(
    const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& state,
    const std::string& authId,
    const std::string& userId) {
    ACSDK_DEBUG5(LX(__func__));

    if (authId.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyAuthId"));
        return;
    }

    m_executor.submit([this, state, authId, userId] { handleTransition(state, authId, userId); });
}

void AuthorizationManager::setStateLocked(const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& state) {
    if (state.state == m_authState.state) {
        ACSDK_DEBUG5(LX(__func__).d("reason", "sameState").d("state", state.state).d("action", "skipping"));
        return;
    }

    ACSDK_DEBUG5(LX(__func__)
                     .d("fromState", m_authState.state)
                     .d("toState", state.state)
                     .d("fromError", m_authState.error)
                     .d("toError", state.error));

    m_authState = state;

    std::unique_lock<std::mutex> lock(m_observersMutex);
    for (auto& observer : m_observers) {
        observer->onAuthStateChange(state.state, state.error);
    }
}

void AuthorizationManager::setActiveLocked(const std::string& adapterId, const std::string& userId) {
    ACSDK_DEBUG5(LX(__func__));

    auto it = m_adapters.find(adapterId);
    if (it == m_adapters.end()) {
        ACSDK_ERROR(LX(__func__).d("reason", "adapterNotRegistered").d("adapterId", adapterId));
        return;
    }

    m_activeAdapter = it->second;
    m_activeAdapterId = adapterId;
    m_activeUserId = userId;
}

void AuthorizationManager::persist(const std::string& adapterId, const std::string& userId) {
    if (!m_storage->store(adapterId, userId)) {
        ACSDK_CRITICAL(LX(__func__)
                           .d("reason", "failedToStoreAuthIdentifiers")
                           .d("adapter", m_activeAdapterId)
                           .sensitive("userId", m_activeUserId));
    }
}

void AuthorizationManager::handleTransition(
    const avsCommon::sdkInterfaces::AuthObserverInterface::FullState& newState,
    const std::string& authId,
    const std::string& userId) {
    ACSDK_DEBUG5(LX(__func__));

    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_adapters.find(authId);
    if (m_adapters.end() == it) {
        ACSDK_ERROR(LX(__func__).d("reason", "unrecognizedAdapter").d("authId", authId));
        return;
    }

    std::shared_ptr<AuthorizationAdapterInterface> reportingAdapter = it->second;
    bool valid = false;
    bool interruptingAuthorization = false;
    if ((!m_activeAdapterId.empty() && m_activeAdapterId != authId) ||
        (!m_activeUserId.empty() && m_activeUserId != userId)) {
        ACSDK_INFO(LX(__func__)
                       .d("reason", "interruptingAuthorizationDetected")
                       .d("activeAdapterId", m_activeAdapterId)
                       .sensitive("activeUserId", m_activeUserId)
                       .d("newAdapterId", authId)
                       .sensitive("newUserId", userId));
        interruptingAuthorization = true;
    }

    // Only allow implicit logout if the new authorization is reporting AUTHORIZING.
    // This is to simplify the state machine logic. This restriction can be potentially be
    // loosened to allow other states.
    if (interruptingAuthorization) {
        if (newState.state == AuthObserverInterface::State::AUTHORIZING) {
            logoutHelper(lock);
        } else if (newState.state == AuthObserverInterface::State::REFRESHED) {
            /*
             * Another adapter besides the active one has REFRESHED. This indicates
             * an inconsistent state in authorization. Force a logout to protect
             * customer data.
             */
            ACSDK_ERROR(LX(__func__)
                            .d("reason", "mismatchingAdapter")
                            .d("activeAdapterId", m_activeAdapterId)
                            .d("incomingAdapterId", authId));

            reportingAdapter->reset();
            logoutHelper(lock);
            return;
        } else {
            ACSDK_WARN(LX(__func__)
                           .d("reason", "invalidStateNewAuth")
                           .d("authId", authId)
                           .sensitive("userId", userId)
                           .d("newState", newState.state)
                           .d("action", "noOp"));
            return;
        }
    }

    // From this point on the authorization interruption has been handled.
    if (newState.state == m_authState.state) {
        ACSDK_DEBUG0(LX(__func__).d("reason", "sameState").d("authId", m_activeAdapter).d("state", newState.state));
        return;
    }

    switch (newState.state) {
        case AuthObserverInterface::State::UNINITIALIZED:
            break;
        case AuthObserverInterface::State::AUTHORIZING:
            if (m_authState.state == AuthObserverInterface::State::UNINITIALIZED) {
                setActiveLocked(authId, userId);
                valid = true;
            }
            break;
        case AuthObserverInterface::State::REFRESHED:
            if (m_authState.state == AuthObserverInterface::State::AUTHORIZING) {
                if (m_activeUserId.empty()) {
                    m_activeUserId = userId;
                }
                persist(m_activeAdapterId, m_activeUserId);
                valid = true;
            } else if (m_authState.state == AuthObserverInterface::State::REFRESHED) {
                valid = true;
            }

            break;
        case AuthObserverInterface::State::EXPIRED:
            if (m_authState.state == AuthObserverInterface::State::REFRESHED) {
                valid = true;
            }
            break;
        case AuthObserverInterface::State::UNRECOVERABLE_ERROR:
            setStateLocked(newState);
            logoutHelper(lock);
            return;
    };

    if (!valid) {
        ACSDK_ERROR(LX(__func__)
                        .d("reason", "invalidTransition")
                        .d("adapterId", authId)
                        .d("from", m_authState.state)
                        .d("to", newState.state));
    } else {
        setStateLocked(newState);
    }
}

void AuthorizationManager::add(
    const std::shared_ptr<acsdkAuthorizationInterfaces::AuthorizationAdapterInterface>& adapter) {
    ACSDK_DEBUG5(LX(__func__));

    if (!adapter) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullAdapter"));
        return;
    }

    std::string adapterId;
    if (adapter->getAuthorizationInterface()) {
        adapterId = adapter->getAuthorizationInterface()->getId();
    }

    if (adapterId.empty()) {
        ACSDK_ERROR(LX(__func__).d("reason", "emptyAuthAdapterId"));
        return;
    }

    std::unique_lock<std::mutex> lock(m_mutex);

    if (m_adapters.count(adapterId) != 0) {
        ACSDK_ERROR(LX(__func__).d("reason", "alreadyAdded").d("adapterId", adapterId));
        return;
    }

    m_adapters.insert({adapterId, adapter});

    if (m_activeAdapterId == adapterId) {
        m_activeAdapter = adapter;
    }

    lock.unlock();

    auto state = adapter->onAuthorizationManagerReady(shared_from_this());

    lock.lock();

    // Immediately set the state as it's possible we may miss some reportStateChange transitions
    setStateLocked(state);
}

void AuthorizationManager::logout() {
    ACSDK_INFO(LX(__func__));

    if (m_registrationManager) {
        m_registrationManager->logout();
    } else {
        ACSDK_CRITICAL(LX(__func__).d("reason", "nullRegistrationManager").m("Unable to Complete Logout"));
    }
}

void AuthorizationManager::logoutHelper(std::unique_lock<std::mutex>& lock) {
    ACSDK_INFO(LX(__func__));

    lock.unlock();
    logout();
    lock.lock();
}

avsCommon::sdkInterfaces::AuthObserverInterface::State AuthorizationManager::getState() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    ACSDK_DEBUG5(LX(__func__).d("state", m_authState.state));
    return m_authState.state;
}

std::string AuthorizationManager::getActiveAuthorization() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    std::string activeAdapterId;
    if (m_activeAdapter && m_activeAdapter->getAuthorizationInterface()) {
        activeAdapterId = m_activeAdapter->getAuthorizationInterface()->getId();
    }

    return activeAdapterId;
}

void AuthorizationManager::addAuthObserver(std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));

    if (!observer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullObserver"));
        return;
    }

    AuthObserverInterface::FullState state;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        state = m_authState;
    }

    observer->onAuthStateChange(state.state, state.error);

    std::lock_guard<std::mutex> lock(m_observersMutex);
    m_observers.insert(observer);
}

void AuthorizationManager::removeAuthObserver(
    std::shared_ptr<avsCommon::sdkInterfaces::AuthObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));

    if (!observer) {
        ACSDK_ERROR(LX(__func__).d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock(m_observersMutex);
    m_observers.erase(observer);
}

std::string AuthorizationManager::getAuthToken() {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    std::string authToken;

    if (m_activeAdapter && AuthObserverInterface::State::REFRESHED == m_authState.state) {
        authToken = m_activeAdapter->getAuthToken();
    } else {
        ACSDK_WARN(LX(__func__).d("reason", "noActiveAdapter"));
    }

    ACSDK_DEBUG0(LX(__func__).sensitive("token", authToken));

    return authToken;
}

void AuthorizationManager::onAuthFailure(const std::string& token) {
    ACSDK_DEBUG5(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_activeAdapter) {
        m_activeAdapter->onAuthFailure(token);
    }
}

void AuthorizationManager::doShutdown() {
    ACSDK_DEBUG5(LX(__func__));

    m_executor.shutdown();

    {
        std::lock_guard<std::mutex> lock(m_observersMutex);
        m_observers.clear();
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_storage.reset();
    m_activeAdapter.reset();
    m_activeAdapterId.clear();
    m_activeUserId.clear();
    m_storage.reset();
    m_authState = AuthObserverInterface::FullState();
    m_adapters.clear();
    m_registrationManager.reset();
}

}  // namespace acsdkAuthorization
}  // namespace alexaClientSDK
