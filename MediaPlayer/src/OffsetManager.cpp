/*
 * Copyright 2017-2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include "MediaPlayer/OffsetManager.h"

namespace alexaClientSDK {
namespace mediaPlayer {

using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("OffsetManager");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

OffsetManager::OffsetManager() {
    clear();
}

void OffsetManager::setIsSeekable(bool seekable) {
    m_isSeekable = seekable;
}

void OffsetManager::setSeekPoint(std::chrono::milliseconds seekPoint) {
    m_isSeekPointSet = true;
    m_seekPoint = seekPoint;
}

std::chrono::milliseconds OffsetManager::getSeekPoint() {
    return m_seekPoint;
}

bool OffsetManager::isSeekable() {
    return m_isSeekable;
}

bool OffsetManager::isSeekPointSet() {
    return m_isSeekPointSet;
}

void OffsetManager::clear() {
    m_seekPoint = std::chrono::milliseconds::zero();
    m_isSeekable = false;
    m_isSeekPointSet = false;
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
