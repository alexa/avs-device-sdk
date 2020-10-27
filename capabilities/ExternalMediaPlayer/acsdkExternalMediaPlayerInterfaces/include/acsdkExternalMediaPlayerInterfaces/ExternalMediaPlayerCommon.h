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

#ifndef ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAPLAYERCOMMON_H_
#define ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAPLAYERCOMMON_H_

namespace alexaClientSDK {
namespace acsdkExternalMediaPlayerInterfaces {

/// Enum describing the available validation methods for External Media Players
enum class ValidationMethod {
    /// Use a signing certificate for validation
    SIGNING_CERTIFICATE,
    /// Use a generated certificate
    GENERATED_CERTIFICATE,
    /// No validation
    NONE
};

/// Converts the ValidationMethod enum to a string representation
inline std::string validationMethodToString(ValidationMethod value) {
    switch (value) {
        case ValidationMethod::SIGNING_CERTIFICATE:
            return "SIGNING_CERTIFICATE";
        case ValidationMethod::GENERATED_CERTIFICATE:
            return "GENERATED_CERTIFICATE";
        case ValidationMethod::NONE:
            return "NONE";
    }
    return "UNKNOWN";
}

/**
 * Struct describing the basic PlayerInfo information
 */
struct PlayerInfoBase {
    /// The opaque token that uniquely identifies the local external player app
    std::string localPlayerId;

    /// The service provider interface (SPI) version.
    std::string spiVersion;
};

/**
 * Describes a discovered external media player app
 */
struct DiscoveredPlayerInfo : public PlayerInfoBase {
    /// The validation method used for this player
    ValidationMethod validationMethod;

    /** Validation data :
     *  1. Device platform issued app signing certificate. A list of certificates may be attached.
     *  2. In some cases validation is performed locally. The certificate is trasmitted as validationData during
     * discovery to announce the activated app's identity in order to allow app activation to be revoked.
     *  3. empty
     */
    std::vector<std::string> validationData;
};

/**
 * Describes the information and status for a player
 */
struct PlayerInfo : public PlayerInfoBase {
    /**
     * Constructor
     * @param localPlayerId The local player ID
     * @param spiVersion The SPI Version
     * @param supported Whether the player is supported as an External Media Player
     */
    PlayerInfo(const std::string& localPlayerId = "", const std::string& spiVersion = "", bool supported = false) :
            PlayerInfoBase{localPlayerId, spiVersion},
            playerSupported(supported) {
    }

    /// The cloud provided playerID
    std::string playerId;

    /// An opaque token for the domain or skill that is presently associated with this player.
    std::string skillToken;

    /// A universally unique identifier (UUID) generated to the RFC 4122 specification.
    std::string playbackSessionId;

    /// Whether this player is supported and permitted to act as an External Media Player, if a player is not supported
    /// then no state updates should be sent, and no playback controls will be received
    bool playerSupported;
};

}  // namespace acsdkExternalMediaPlayerInterfaces
}  // namespace alexaClientSDK

#endif  // ACSDKEXTERNALMEDIAPLAYERINTERFACES_EXTERNALMEDIAPLAYERCOMMON_H_
