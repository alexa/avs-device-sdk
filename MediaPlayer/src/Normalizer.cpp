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

#include <cmath>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "MediaPlayer/Normalizer.h"

namespace alexaClientSDK {
namespace mediaPlayer {

/// String to identify log entries originating from this file.
static const std::string TAG("Normalizer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::unique_ptr<Normalizer> Normalizer::create(
    const double& sourceMin,
    const double& sourceMax,
    const double& normalizedMin,
    const double& normalizedMax) {
    if (sourceMin >= sourceMax) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "sourceMinGreaterEqThanMax")
                        .d("sourceMin", sourceMin)
                        .d("sourceMax", sourceMax));
        return nullptr;
    } else if (normalizedMin > normalizedMax) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "normalizedMinGreaterThanMax")
                        .d("normalizedMin", normalizedMin)
                        .d("normalizedMax", normalizedMax));
        return nullptr;
    }

    return std::unique_ptr<Normalizer>(new Normalizer(sourceMin, sourceMax, normalizedMin, normalizedMax));
}

Normalizer::Normalizer(
    const double& sourceMin,
    const double& sourceMax,
    const double& normalizedMin,
    const double& normalizedMax) :
        m_sourceMin{sourceMin},
        m_sourceMax{sourceMax},
        m_normalizedMin{normalizedMin},
        m_normalizedMax{normalizedMax} {
    m_scaleFactor = (m_normalizedMax - m_normalizedMin) / (m_sourceMax - m_sourceMin);
}

bool Normalizer::normalize(const double& unnormalizedInput, double* normalizedOutput) {
    if (!normalizedOutput) {
        ACSDK_ERROR(LX("normalizeFailed").d("reason", "nullNormalizedOutput"));
        return false;
    }

    if (unnormalizedInput < m_sourceMin || unnormalizedInput > m_sourceMax) {
        ACSDK_ERROR(LX("normalizeFailed")
                        .d("reason", "outOfBounds")
                        .d("input", unnormalizedInput)
                        .d("sourceMin", m_sourceMin)
                        .d("sourceMax", m_sourceMax));
        return false;
    }

    *normalizedOutput = (unnormalizedInput - m_sourceMin) * m_scaleFactor + m_normalizedMin;

    return true;
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
