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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_ESPDATA_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_ESPDATA_H_

#include <ctype.h>
#include <string>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace aip {

/**
 * A class representation of the Echo Spatial Perception (ESP) data.  This data is used by AVS for device
 * arbitration.  The ESP measurement data needs to be sent in the @c ReportEchoSpatialPerceptionData event to AVS before
 * its corresponding @c Recognize event.  Please note that AVS specifies that the voice energy and ambient energy ESP
 * measurements are float numbers, but in order to remove the dependency of float with the SDK, the float numbers will
 * be represented as a string.
 */
class ESPData {
public:
    /*
     * Getter function to retrieve empty ESP DATA which can be used as the default value for a function parameter to
     * indicate ESP is not supported.
     *
     * @return the empty ESP DATA
     */
    static const ESPData getEmptyESPData();

    /*
     * Default constructor that initializes the @c m_voiceEnergy and @c m_ambientEnergy to a empty string.
     */
    ESPData() = default;

    /*
     * Constructor that initializes @c m_voiceEnergy and @c m_ambientEnergy to the values specified.
     *
     * @param voiceEnergy The string representation of the voice energy measurement in float.
     * @param ambientEnergy The string representation of the ambient energy measurement in float.
     */
    ESPData(const std::string& voiceEnergy, const std::string& ambientEnergy);

    /*
     * Get the voice energy ESP measurement.
     *
     * @return The string representation of the voice energy measurement in float.
     */
    std::string getVoiceEnergy() const;

    /*
     * Get the ambient energy ESP measurement.
     *
     * @return The string representation of the ambient energy measurement in float.
     */
    std::string getAmbientEnergy() const;

    /*
     * Provide rudimentary verification to the ESPData to make sure the measurement strings do not contain anything
     * malicious, only alphanumeric, decimal, and positive/negative sign characters are allowed. AVS will be doing the
     * verification at the end.
     *
     * @return @c true if both @c m_voiceEnergy and @c m_ambientEnergy don't contain invalid characters, else @c false.
     */
    bool verify() const;

    /*
     * Check if the ESP data is empty.
     *
     * @return @c true if both @c m_voiceEnergy and @c m_ambientEnergy are empty, else @c false.
     */
    bool isEmpty() const;

private:
    /*
     * Provide rudimentary verification to see if a string does not contain anything malicious, only alphanumeric,
     * decimal, and positive/negative sign characters are allowed.
     *
     * @param valueToVerify The string to verify if it contains a value float number.
     * @return @c true if @c valueToVerify contains only valid characters, else @c false.
     */
    static bool verifyHelper(const std::string& valueToVerify);

    /// String representation of voice energy ESP measurement in float.
    std::string m_voiceEnergy;

    /// String representation of ambient energy ESP measurement in float.
    std::string m_ambientEnergy;
};

}  // namespace aip
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_AIP_INCLUDE_AIP_ESPDATA_H_
