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

#include <functional>

#include "AVSGatewayManager/AVSGatewayManager.h"

#include <AVSGatewayManager/PostConnectVerifyGatewaySender.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsGatewayManager {

using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
static const std::string TAG("AVSGatewayManager");

/**
 * Create a LogEntry using the file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Name of the @c ConfigurationNode for AVSGatewayManager.
static const std::string AVS_GATEWAY_MANAGER_ROOT_KEY = "avsGatewayManager";

/// Name of the @c AVS Gateway value in the AVSGatewayManager's @c ConfigurationNode.
static const std::string AVS_GATEWAY = "avsGateway";

/// Default @c AVS gateway to connect to.
static const std::string DEFAULT_AVS_GATEWAY = "https://alexa.na.gateway.devices.a2z.com";

std::shared_ptr<AVSGatewayManager> AVSGatewayManager::create(
    std::shared_ptr<storage::AVSGatewayManagerStorageInterface> avsGatewayManagerStorage,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    const ConfigurationNode& configurationRoot) {
    ACSDK_DEBUG5(LX(__func__));
    if (!avsGatewayManagerStorage) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullAvsGatewayManagerStorage"));
    } else {
        /// Retrieve AVS Gateway from config file if available else set Default value.

        std::string avsGateway;

        auto avsGatewayManagerConfig = configurationRoot[AVS_GATEWAY_MANAGER_ROOT_KEY];
        if (avsGatewayManagerConfig.getString(AVS_GATEWAY, &avsGateway, DEFAULT_AVS_GATEWAY)) {
            ACSDK_DEBUG5(LX(__func__).d("default AVS Gateway from config", avsGateway));
        }

        if (avsGateway.empty()) {
            avsGateway = DEFAULT_AVS_GATEWAY;
        }

        auto avsGatewayManager = std::shared_ptr<AVSGatewayManager>(
            new AVSGatewayManager(avsGatewayManagerStorage, customerDataManager, avsGateway));
        if (avsGatewayManager->init()) {
            return avsGatewayManager;
        } else {
            ACSDK_ERROR(LX("createFailed").d("reason", "initializationFailed"));
        }
    }
    return nullptr;
}

AVSGatewayManager::AVSGatewayManager(
    std::shared_ptr<storage::AVSGatewayManagerStorageInterface> avsGatewayManagerStorage,
    std::shared_ptr<registrationManager::CustomerDataManager> customerDataManager,
    const std::string& defaultAVSGateway) :
        CustomerDataHandler{customerDataManager},
        m_avsGatewayStorage{avsGatewayManagerStorage},
        m_currentState{defaultAVSGateway, false} {
}

AVSGatewayManager::~AVSGatewayManager() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_observers.clear();
}

std::shared_ptr<PostConnectOperationInterface> AVSGatewayManager::createPostConnectOperation() {
    std::lock_guard<std::mutex> m_lock{m_mutex};
    ACSDK_DEBUG5(LX(__func__));
    /// Create a PostConnectOperation only when required.
    if (!m_currentState.isVerified) {
        auto callback = std::bind(&AVSGatewayManager::onGatewayVerified, this, std::placeholders::_1);
        m_currentVerifyGatewaySender = PostConnectVerifyGatewaySender::create(callback);
        return m_currentVerifyGatewaySender;
    }
    ACSDK_DEBUG5(LX(__func__).m("Gateway already verified, skipping gateway verification step"));

    return nullptr;
}

bool AVSGatewayManager::init() {
    std::lock_guard<std::mutex> m_lock{m_mutex};

    if (!m_avsGatewayStorage->init()) {
        ACSDK_ERROR(LX("initFailed").d("reason", "unable to initialize gateway storage."));
        return false;
    }

    if (!m_avsGatewayStorage->loadState(&m_currentState)) {
        ACSDK_ERROR(LX("initFailed").d("reason", "unable to load gateway verification state"));
        return false;
    }

    ACSDK_DEBUG5(LX("init").d("avsGateway", m_currentState.avsGatewayURL));
    return true;
}

bool AVSGatewayManager::setAVSGatewayAssigner(std::shared_ptr<AVSGatewayAssignerInterface> avsGatewayAssigner) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> m_lock{m_mutex};

    if (!avsGatewayAssigner) {
        ACSDK_ERROR(LX("setAVSGatewayAssignerFailed").d("reason", "nullAvsGatewayAssigner"));
        return false;
    }
    m_avsGatewayAssigner = avsGatewayAssigner;
    m_avsGatewayAssigner->setAVSGateway(m_currentState.avsGatewayURL);
    return true;
}

std::string AVSGatewayManager::getGatewayURL() const {
    std::lock_guard<std::mutex> lock{m_mutex};
    return m_currentState.avsGatewayURL;
}

bool AVSGatewayManager::setGatewayURL(const std::string& avsGatewayURL) {
    ACSDK_DEBUG5(LX(__func__).d("avsGateway", avsGatewayURL));
    if (avsGatewayURL.empty()) {
        ACSDK_ERROR(LX("setGatewayURLFailed").d("reason", "empty avsGatewayURL"));
        return false;
    };

    std::unordered_set<std::shared_ptr<AVSGatewayObserverInterface>> observersCopy;
    std::shared_ptr<AVSGatewayAssignerInterface> avsGatewayAssignerCopy;

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (avsGatewayURL != m_currentState.avsGatewayURL) {
            m_currentState = GatewayVerifyState{avsGatewayURL, false};
            saveStateLocked();
            observersCopy = m_observers;
            avsGatewayAssignerCopy = m_avsGatewayAssigner;
        } else {
            ACSDK_ERROR(LX("setGatewayURLFailed").d("reason", "no change in URL"));
            return false;
        }
    }

    if (avsGatewayAssignerCopy) {
        avsGatewayAssignerCopy->setAVSGateway(avsGatewayURL);
    } else {
        ACSDK_WARN(LX("setGatewayURLWARN").d("reason", "invalid AVSGatewayAssigner"));
    }

    if (!observersCopy.empty()) {
        for (auto& observer : observersCopy) {
            observer->onAVSGatewayChanged(avsGatewayURL);
        }
    }

    return true;
}

void AVSGatewayManager::onGatewayVerified(const std::shared_ptr<PostConnectOperationInterface>& verifyGatewaySender) {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    if (m_currentVerifyGatewaySender == verifyGatewaySender && !m_currentState.isVerified) {
        m_currentState.isVerified = true;
        saveStateLocked();
    }
}

bool AVSGatewayManager::saveStateLocked() {
    ACSDK_DEBUG5(LX(__func__).d("gateway", m_currentState.avsGatewayURL).d("isVerified", m_currentState.isVerified));
    if (!m_avsGatewayStorage->saveState(m_currentState)) {
        ACSDK_ERROR(LX("saveStateLockedFailed").d("reason", "unable to save to database"));
        return false;
    }
    return true;
}

void AVSGatewayManager::addObserver(std::shared_ptr<AVSGatewayObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!m_observers.insert(observer).second) {
            ACSDK_ERROR(LX("addObserverFailed").d("reason", "observer already added!"));
            return;
        }
    }
}

void AVSGatewayManager::removeObserver(std::shared_ptr<AVSGatewayObserverInterface> observer) {
    ACSDK_DEBUG5(LX(__func__));
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    {
        std::lock_guard<std::mutex> lock{m_mutex};
        if (!m_observers.erase(observer)) {
            ACSDK_ERROR(LX("removeObserverFailed").d("reason", "observer not found"));
            return;
        }
    }
}

void AVSGatewayManager::clearData() {
    ACSDK_DEBUG5(LX(__func__));
    std::lock_guard<std::mutex> lock{m_mutex};
    m_avsGatewayStorage->clear();
}

}  // namespace avsGatewayManager
}  // namespace alexaClientSDK
