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

#include "acsdk/PresentationOrchestratorClient/private/PresentationLifespanToTimeoutMapper.h"

#include <string>

#include <acsdk/PresentationOrchestratorInterfaces/PresentationInterface.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace presentationOrchestratorClient {

using namespace presentationOrchestratorInterfaces;
using namespace alexaClientSDK::avsCommon::utils;
using namespace alexaClientSDK::avsCommon::utils::configuration;

/// String to identify log entries originating from this file.
#define TAG "PresentationLifespanToTimeoutMapper"

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) logger::LogEntry(TAG, event)

/// The key in config file to find the root of presentation options.
static const std::string PRESENTATION_OPTIONS_ROOT_KEY = "presentationOptions";

/// The key in config file to find the timeout for SHORT presentations.
static const char SHORT_PRESENTATION_TIMEOUT_KEY[] = "shortPresentationTimeoutMs";

/// The key in config file to find the timeout for TRANSIENT presentations.
static const char TRANSIENT_PRESENTATION_TIMEOUT_KEY[] = "transientPresentationTimeoutMs";

/// The key in config file to find the timeout for LONG presentations.
static const char LONG_PRESENTATION_TIMEOUT_KEY[] = "longPresentationTimeoutMs";

/// Default timeout for SHORT presentations.
static const std::chrono::milliseconds DEFAULT_TIMEOUT_SHORT_PRESENTATION_MS{30000};

/// Default timeout for TRANSIENT presentations.
static const std::chrono::milliseconds DEFAULT_TIMEOUT_TRANSIENT_PRESENTATION_MS{10000};

/// Default timeout for LONG presentations.
static const std::chrono::milliseconds DEFAULT_TIMEOUT_LONG_PRESENTATION_MS{
    PresentationInterface::getTimeoutDisabled()};

/// Default timeout for PERMANENT presentations.
static const std::chrono::milliseconds DEFAULT_TIMEOUT_PERMANENT_PRESENTATION_MS{
    PresentationInterface::getTimeoutDisabled()};

std::shared_ptr<PresentationLifespanToTimeoutMapper> PresentationLifespanToTimeoutMapper::create() {
    auto mapper = std::shared_ptr<PresentationLifespanToTimeoutMapper>(new PresentationLifespanToTimeoutMapper());
    mapper->initialize();

    return mapper;
}

void PresentationLifespanToTimeoutMapper::initialize() {
    auto configurationRoot = ConfigurationNode::getRoot()[PRESENTATION_OPTIONS_ROOT_KEY];

    configurationRoot.getDuration<std::chrono::milliseconds>(
        SHORT_PRESENTATION_TIMEOUT_KEY, &m_shortPresentationTimeout, DEFAULT_TIMEOUT_SHORT_PRESENTATION_MS);

    configurationRoot.getDuration<std::chrono::milliseconds>(
        TRANSIENT_PRESENTATION_TIMEOUT_KEY, &m_transientPresentationTimeout, DEFAULT_TIMEOUT_TRANSIENT_PRESENTATION_MS);

    configurationRoot.getDuration<std::chrono::milliseconds>(
        LONG_PRESENTATION_TIMEOUT_KEY, &m_longPresentationTimeout, DEFAULT_TIMEOUT_LONG_PRESENTATION_MS);
}

PresentationLifespanToTimeoutMapper::PresentationLifespanToTimeoutMapper() = default;

static bool isTimeoutDisabled(std::chrono::milliseconds timeoutVal) {
    return timeoutVal.count() == -1;
}

std::chrono::milliseconds PresentationLifespanToTimeoutMapper::getTimeoutDuration(
    const PresentationLifespan& lifespan) {
    ACSDK_DEBUG5(LX(__func__).d("lifespan", lifespan));

    std::chrono::milliseconds configurableTimeout;

    switch (lifespan) {
        case PresentationLifespan::SHORT:
            configurableTimeout = m_shortPresentationTimeout;
            break;
        case PresentationLifespan::TRANSIENT:
            configurableTimeout = m_transientPresentationTimeout;
            break;
        case PresentationLifespan::LONG:
            configurableTimeout = m_longPresentationTimeout;
            break;
        case PresentationLifespan::PERMANENT:
            return DEFAULT_TIMEOUT_PERMANENT_PRESENTATION_MS;
    }

    // Configurable Timeout value of -1 equals disabled
    if (isTimeoutDisabled(configurableTimeout)) {
        configurableTimeout = PresentationInterface::getTimeoutDisabled();
    }

    return configurableTimeout;
}

}  // namespace presentationOrchestratorClient
}  // namespace alexaClientSDK
