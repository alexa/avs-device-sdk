/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <map>

#include <rapidjson/document.h>

#include <AVSCommon/AVS/BlockingPolicy.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace rapidjson;
using namespace avsCommon::avs;
using namespace avsCommon::utils::json;

/// Flag indicating @c AUDIO medium is used.
static const BlockingPolicy::Mediums MEDIUM_FLAG_AUDIO{1};

/// Flag indicating @c VISUAL medium is used.
static const BlockingPolicy::Mediums MEDIUM_FLAG_VISUAL{2};

/// String to identify log entries originating from this file.
static const std::string TAG("BlockingPolicy");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

const BlockingPolicy::Mediums BlockingPolicy::MEDIUM_AUDIO{MEDIUM_FLAG_AUDIO};
const BlockingPolicy::Mediums BlockingPolicy::MEDIUM_VISUAL{MEDIUM_FLAG_VISUAL};
const BlockingPolicy::Mediums BlockingPolicy::MEDIUMS_AUDIO_AND_VISUAL{MEDIUM_FLAG_AUDIO | MEDIUM_FLAG_VISUAL};
const BlockingPolicy::Mediums BlockingPolicy::MEDIUMS_NONE{};

BlockingPolicy::BlockingPolicy(const Mediums& mediums, bool isBlocking) : m_mediums{mediums}, m_isBlocking{isBlocking} {
}

bool BlockingPolicy::isValid() const {
    // We use neither mediums + blocking to indicate invalid policy
    // as this is invalid combination.
    auto isValid = !((MEDIUMS_NONE == m_mediums) && m_isBlocking);

    return isValid;
}

bool BlockingPolicy::isBlocking() const {
    return m_isBlocking;
}

BlockingPolicy::Mediums BlockingPolicy::getMediums() const {
    return m_mediums;
}

bool operator==(const BlockingPolicy& lhs, const BlockingPolicy& rhs) {
    return ((lhs.getMediums() == rhs.getMediums()) && (lhs.isBlocking() == rhs.isBlocking()));
}

bool operator!=(const BlockingPolicy& lhs, const BlockingPolicy& rhs) {
    return !(lhs == rhs);
}

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK