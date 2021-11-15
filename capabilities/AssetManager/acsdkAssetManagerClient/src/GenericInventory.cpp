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

#include "acsdkAssetManagerClient/GenericInventory.h"

#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace acsdkAssets {
namespace client {

using namespace std;
using namespace commonInterfaces;
using namespace clientInterfaces;

/// String to identify log entries originating from this file.
static const std::string TAG{"GenericInventory"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

GenericInventory::GenericInventory(string name, shared_ptr<ArtifactWrapperFactoryInterface> artifactWrapperFactory) :
        m_name(move(name)),
        m_artifactWrapperFactory(move(artifactWrapperFactory)) {
}

GenericInventory::~GenericInventory() {
    ACSDK_DEBUG(LX("~GenericInventory").m("Shutting down manager").d("name", m_name));
    cancelChange();
}

shared_ptr<ArtifactWrapperInterface> GenericInventory::prepareForSettingChange(const SettingsMap& newSettings) {
    auto request = createRequest(newSettings);
    if (request == nullptr) {
        ACSDK_ERROR(LX("prepareForSettingChange").m("Invalid request created").d("name", m_name));
        return nullptr;
    }

    lock_guard<mutex> lock(m_eventMutex);
    // if our active artifact is the same one we'd like to prepare, then set our "pending" to our active and return it
    if (m_activeArtifact != nullptr && *m_activeArtifact->getRequest() == *request) {
        ACSDK_INFO(LX("prepareForSettingChange").m("Already using the new artifact").d("name", m_name));
        cancelChangeLocked();
        m_pendingArtifact = m_activeArtifact;
        return m_pendingArtifact;
    }

    // if our pending artifact is the same one we'd like to prepare, then return the pending artifact
    if (m_pendingArtifact != nullptr && *m_pendingArtifact->getRequest() == *request) {
        ACSDK_INFO(LX("prepareForSettingChange")
                           .m("Already in the process of fetching the new artifact")
                           .d("name", m_name));
        return m_pendingArtifact;
    }

    auto artifactWrapper = m_artifactWrapperFactory->createArtifactWrapper(request, shared_from_this());

    if (artifactWrapper == nullptr) {
        ACSDK_ERROR(LX("prepareForSettingChange")
                            .m("Failed to create an artifact request based off of given settings")
                            .d("name", m_name));
        return nullptr;
    }
    artifactWrapper->download();

    // if we were preparing a different artifact, then cancel it
    if (m_pendingArtifact != nullptr) {
        ACSDK_WARN(LX("prepareForSettingChange")
                           .m("There was already a different pending artifact, cancelling it")
                           .d("name", m_name));
        cancelChangeLocked();
    }

    m_pendingArtifact = artifactWrapper;
    m_pendingArtifact->setPriority(Priority::PENDING_ACTIVATION);
    return m_pendingArtifact;
}

bool GenericInventory::commitChange() {
    lock_guard<mutex> lock(m_eventMutex);
    if (m_pendingArtifact == nullptr) {
        ACSDK_ERROR(LX("commitChange").m("Pending artifact is NULL").d("name", m_name));
        return false;
    }

    if (!m_pendingArtifact->isAvailable()) {
        ACSDK_ERROR(LX("commitChange").m("Pending artifact is NOT downloaded or available").d("name", m_name));
        return false;
    }

    if (m_pendingArtifact == m_activeArtifact) {
        ACSDK_INFO(LX("commitChange").m("Artifact is already active").d("name", m_name));
        m_pendingArtifact->setPriority(Priority::ACTIVE);
        m_pendingArtifact.reset();
        return true;
    }

    if (!applyChangesLocked(m_pendingArtifact->getPath())) {
        ACSDK_ERROR(
                LX("commitChange").m("Pending artifact is not valid or corrupt, could not apply").d("name", m_name));
        return false;
    }

    ACSDK_INFO(LX("commitChange").m("Successfully committed changes").d("name", m_name));
    if (m_activeArtifact != nullptr) {
        m_activeArtifact->setPriority(Priority::UNUSED);
    }
    m_pendingArtifact->setPriority(Priority::ACTIVE);
    m_activeArtifact = move(m_pendingArtifact);
    return true;
}

void GenericInventory::cancelChange() {
    ACSDK_INFO(LX("cancelChange").m("Cancelling changes").d("name", m_name));
    lock_guard<mutex> lock(m_eventMutex);
    cancelChangeLocked();
}

void GenericInventory::cancelChangeLocked() {
    if (m_pendingArtifact == nullptr) {
        ACSDK_DEBUG(LX("cancelChangeLocked").m("Nothing to cancel").d("name", m_name));
        return;
    }

    if (m_pendingArtifact == m_activeArtifact) {
        ACSDK_DEBUG(LX("cancelChangeLocked")
                            .m("Won't cancel pending artifact since it's already active")
                            .d("name", m_name));
        m_pendingArtifact.reset();
        return;
    }

    if (m_pendingArtifact->isAvailable()) {
        ACSDK_DEBUG(LX("cancelChangeLocked")
                            .m("Demoting priority")
                            .d("name", m_name)
                            .d("priority", toString(Priority::UNUSED)));
        m_pendingArtifact->setPriority(Priority::UNUSED);
    } else {
        ACSDK_DEBUG(LX("cancelChangeLocked").m("Cancelling and cleaning up pending download").d("name", m_name));
        m_pendingArtifact->erase();
    }
    m_pendingArtifact.reset();
}

string GenericInventory::getArtifactPath() {
    lock_guard<mutex> lock(m_eventMutex);
    if (m_activeArtifact == nullptr) {
        ACSDK_ERROR(LX("getArtifactPath").m("No active artifact found").d("name", m_name));
        return "";
    }

    return m_activeArtifact->getPath();
}

void GenericInventory::setCurrentActivePriority(Priority priority) {
    lock_guard<mutex> lock(m_eventMutex);
    if (m_activeArtifact == nullptr) {
        ACSDK_WARN(LX("setCurrentActivePriority").m("No active priority to set").d("name", m_name));
        return;
    }

    m_activeArtifact->setPriority(priority);
}

bool GenericInventory::isSettingReady(const SettingsMap& setting) {
    auto artifact = m_artifactWrapperFactory->createArtifactWrapper(createRequest(setting));
    return artifact && artifact->isAvailable();
}

}  // namespace client
}  // namespace acsdkAssets
}  // namespace alexaClientSDK
