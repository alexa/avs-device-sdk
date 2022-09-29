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

#include <AVSCommon/Utils/Logger/Logger.h>
#include "AVSCommon/Utils/Power/PowerResource.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace utils {
namespace power {

using namespace avsCommon::sdkInterfaces;

/// String to identify log entries originating from this file.
#define TAG "PowerResource"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<PowerResource> PowerResource::create(
    const std::string& identifier,
    std::shared_ptr<PowerResourceManagerInterface> powerManager,
    PowerResourceManagerInterface::PowerResourceLevel level,
    bool refCounted) {
    if (!powerManager) {
        ACSDK_ERROR(LX(__func__).d("error", "nullPowerManager"));
        return nullptr;
    }

    // Identifier name with PREFIX for calling @c PowerResourceManagerInterface.
    // This will make ACSDK created power resources easy to identify.
    auto prefixedIdentifier = std::string(PREFIX) + identifier;
    auto powerResourceId = powerManager->create(prefixedIdentifier, refCounted, level);

    if (!powerResourceId) {
        ACSDK_ERROR(LX(__func__).d("reason", "createFailed").d("method", "PowerResourceManagerInterface::create"));
        return nullptr;
    }

    return std::shared_ptr<PowerResource>(
        new PowerResource(identifier, powerManager, level, refCounted, powerResourceId));
}

PowerResource::~PowerResource() {
    ACSDK_DEBUG9(LX(__func__));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isFrozen) {
        ACSDK_WARN(LX(__func__).d("reason", "isFrozen"));
    }

    if (m_powerManager) {
        for (uint64_t i = 0; i < m_refCount; i++) {
            m_powerManager->release(m_powerResourceId);
        }
    }

    m_powerManager->close(m_powerResourceId);
}

PowerResource::PowerResource(
    const std::string& identifier,
    std::shared_ptr<PowerResourceManagerInterface> powerManager,
    PowerResourceManagerInterface::PowerResourceLevel level,
    bool refCounted,
    std::shared_ptr<sdkInterfaces::PowerResourceManagerInterface::PowerResourceId> powerResourceId) :
        m_identifier{identifier},
        m_isRefCounted{refCounted},
        m_powerResourceId{powerResourceId},
        m_refCount{0},
        m_level{level},
        m_isFrozen{false},
        m_powerManager{powerManager} {
}

bool PowerResource::isRefCounted() const {
    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier));

    return m_isRefCounted;
}

std::string PowerResource::getId() const {
    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier));

    return m_identifier;
}

bool PowerResource::isFrozen() const {
    std::lock_guard<std::mutex> lock(m_mutex);

    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier).d("isFrozen", m_isFrozen).d("refCount", m_refCount));

    return m_isFrozen;
}

PowerResourceManagerInterface::PowerResourceLevel PowerResource::getLevel() const {
    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier));

    std::lock_guard<std::mutex> lock(m_mutex);
    return m_level;
}

void PowerResource::acquire() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isFrozen) {
        ACSDK_ERROR(LX(__func__).d("reason", "frozen"));
        return;
    }

    if (m_powerManager) {
        if (m_isRefCounted) {
            m_refCount++;
        } else {
            m_refCount = 1;
        }

        m_powerManager->acquire(m_powerResourceId);
    }

    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier).d("refCount", m_refCount));
}

void PowerResource::release() {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_isFrozen) {
        ACSDK_ERROR(LX(__func__).d("reason", "frozen"));
        return;
    }

    if (m_powerManager) {
        if (m_isRefCounted) {
            if (m_refCount > 0) {
                m_refCount--;
            }
        } else {
            m_refCount = 0;
        }

        m_powerManager->release(m_powerResourceId);
    }

    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier).d("refCount", m_refCount));
}

void PowerResource::freeze() {
    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier).d("refCount", m_refCount));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isFrozen) {
        ACSDK_DEBUG9(LX(__func__).d("reason", "alreadyFrozen"));
        return;
    }

    if (m_powerManager) {
        m_isFrozen = true;

        for (uint64_t i = 0; i < m_refCount; i++) {
            m_powerManager->release(m_powerResourceId);
        }
    }
}

void PowerResource::thaw() {
    ACSDK_DEBUG9(LX(__func__).d("id", m_identifier).d("refCount", m_refCount));

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_isFrozen) {
        ACSDK_DEBUG9(LX(__func__).d("reason", "notFrozen"));
        return;
    }

    if (m_powerManager) {
        for (uint64_t i = 0; i < m_refCount; i++) {
            m_powerManager->acquire(m_powerResourceId);
        }
    }

    m_isFrozen = false;
}

}  // namespace power
}  // namespace utils
}  // namespace avsCommon
}  // namespace alexaClientSDK
