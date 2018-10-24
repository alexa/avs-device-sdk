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

#include <array>

#include <AVSCommon/Utils/Logger/Logger.h>

#include "EqualizerImplementations/EqualizerLinearBandMapper.h"

namespace alexaClientSDK {
namespace equalizer {

using namespace avsCommon::sdkInterfaces::audio;

/// String to identify log entries originating from this file.
static const std::string TAG{"EqualizerLinearBandMapper"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Minimum number of target bands supported.
static const int MIN_TARGET_BANDS = 1;
/**
 * Maximum number of target bands supported. An unrealistic value is chosen to handle cases when huge number is
 * provided by mistake.
 */
static const int MAX_TARGET_BANDS = 1000;

void EqualizerLinearBandMapper::mapEqualizerBands(
    const avsCommon::sdkInterfaces::audio::EqualizerBandLevelMap& bandLevelMap,
    std::function<void(int, int)> setBandCallback) {
    if (bandLevelMap.empty()) {
        // Nothing to do here.
        ACSDK_ERROR(LX("mapEqualizerBandsFailed").d("reason", "no source bands provided"));
        return;
    }

    // Distribute supported AVS bands among the spectrum.
    std::vector<int> levels(bandLevelMap.size());
    int outBandIndex = 0;
    for (EqualizerBand band : EqualizerBandValues) {
        auto it = bandLevelMap.find(band);
        if (bandLevelMap.end() != it) {
            levels[outBandIndex++] = it->second;
        }
    }

    int numberOfBandsToMapFrom = outBandIndex;

    /*
     * Distribute provided values between bands:
     */
    if (numberOfBandsToMapFrom <= m_numberOfTargetBands) {
        /*
         * Case: AVS has less or the same number of bands than target
         * Band values
         * The idea is that we distribute our X AVS band values among X+ target band values.
         * Let's consider the following:
         * Values provided for three bands: A, B, C. Output has 5 bands: a, b, c, d, e. What we try to do is to
         * distribute three values onto 5 bands. We do a rough approximation here:
         * A  AB B  BC C
         * a  b  c  d  e
         *
         * Where AB and BC is an average between two.
         *
         * For the case of 3 to 10 mapping that would be:
         *
         * A  A  A  AB B  B  BC C  C  C
         * a  b  c  d  e  f  g  h  i  j
         *
         * This is how it works:
         * We have ACC(accumulator) = 10 output bands and INP = 3 input bands. SBand - source band iterator,
         * TBand - target band iterator
         *
         * ACC - INP = 10 - 3 = 7 : no overflow, we map SBand (A) to a TBand (a) and got to next TBand (b)
         * ACC - INP = 7 - 3 = 4 : no overflow, we map SBand (A) to a TBand (b) and got to next TBand (c)
         * ACC - INP = 4 - 3 = 1 : no overflow, we map SBand (A) to a TBand (c) and got to next TBand (d)
         * ACC - INP = 1 - 3 = -2 : we've got overflow. In case of overflow we use average value between current and
         * next SBand: (A + B) / 2 and map it to current TBand (d) and increase TBand (e). Then we reset
         * the ACC: ACC += 10 = 8. Since ACC reached zero or below we increase SBand to next one (B).
         * ACC - INP = 8 - 3 = 5 : no overflow, we map SBand (B) to a TBand (e) and got to next TBand (f)
         * ACC - INP = 5 - 3 = 2 : no overflow, we map SBand (B) to a TBand (f) and got to next TBand (g)
         * ACC - INP = 2 - 3 = -1 : overflow again: ACC += 10 = 9, TBand(g) = [SBand + (SBand++)] / 2 = (B + C) / 2,
         * TBand++, SBand++
         * ACC - INP = 9 - 3 = 6 : no overflow, we map SBand (C) to a TBand (g) and got to next TBand (h)
         * ACC - INP = 6 - 3 = 3 : no overflow, we map SBand (C) to a TBand (h) and got to next TBand (i)
         * ACC - INP = 3 - 3 = 0 : no overflow, we map SBand (C) to a TBand (i) and we mapped all the values. Fin.
         * On the last step we see that ACC reached zero, this means that we've done with the SBand and should go to a
         * next one, however since there was no overflow we do not use average.
         *
         */
        outBandIndex = 0;
        int accumulator = m_numberOfTargetBands;
        for (int i = 0; i < m_numberOfTargetBands; i++) {
            int level = levels[outBandIndex];
            accumulator -= numberOfBandsToMapFrom;
            if (accumulator < 1) {
                outBandIndex++;
                if (accumulator < 0) {
                    level += levels[outBandIndex];
                    level /= 2;
                }
                accumulator += m_numberOfTargetBands;
            }

            // Set the target band value
            setBandCallback(i, level);
        }
    } else {
        /*
         * Case: AVS has more bands than target
         */
        outBandIndex = 0;
        int accumulator = numberOfBandsToMapFrom;
        int level = 0;
        int bandsGrouped = 0;
        for (int i = 0; i < numberOfBandsToMapFrom; i++) {
            level += levels[i];
            bandsGrouped++;
            accumulator -= m_numberOfTargetBands;
            if (accumulator < 1) {
                level /= bandsGrouped;

                // Set the target band value
                setBandCallback(outBandIndex, level);
                outBandIndex++;

                accumulator += numberOfBandsToMapFrom;

                if (accumulator < numberOfBandsToMapFrom) {
                    // Overflow happened
                    level = levels[i];
                    bandsGrouped = 1;
                } else {
                    level = 0;
                    bandsGrouped = 0;
                }
            }  // if accumulator < 1
        }      // iterate AVS bands
    }          // There are more AVS bands than target bands.
}

EqualizerLinearBandMapper::EqualizerLinearBandMapper(int numberOfTargetBands) :
        m_numberOfTargetBands{numberOfTargetBands} {
}

std::shared_ptr<EqualizerLinearBandMapper> EqualizerLinearBandMapper::create(int numberOfTargetBands) {
    if (numberOfTargetBands < MIN_TARGET_BANDS || numberOfTargetBands > MAX_TARGET_BANDS) {
        ACSDK_ERROR(LX("createFailed")
                        .d("reason", "invalid number of target bands")
                        .d("target bands", numberOfTargetBands)
                        .d("min", MIN_TARGET_BANDS)
                        .d("max", MAX_TARGET_BANDS));
        return nullptr;
    }
    return std::shared_ptr<EqualizerLinearBandMapper>(new EqualizerLinearBandMapper(numberOfTargetBands));
}

}  // namespace equalizer
}  // namespace alexaClientSDK
