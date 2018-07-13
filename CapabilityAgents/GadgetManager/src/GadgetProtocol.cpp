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

#include "GadgetManager/GadgetProtocol.h"
#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include <AVSCommon/Utils/Logger/Logger.h>

#include "GadgetManager/GadgetProtocolConstants.h"

/// String to identify log entries originating from this file.
static const std::string TAG{"GadgetProtocol"};

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace capabilityAgents {
namespace gadgetManager {

/// Location of Command ID value in a properly formed GadgetProtocol packet.
static const size_t COMMAND_ID_INDEX = 1;
/// Location of Error ID value in a properly formed GadgetProtocol packet.
static const size_t ERROR_ID_INDEX = 2;
/// Location of Sequence ID value in a properly formed GadgetProtocol packet.
static const size_t SEQUENCE_ID_INDEX = 3;

/**
 * @param byte The character the needs determination if it is a special character within the Gadget Protocol.
 * @return true if byte is a special character.
 */
static bool isSpecialCharacter(uint8_t byte) {
    return GadgetProtocolConstants::PACKET_BEGIN == byte || GadgetProtocolConstants::PACKET_END == byte ||
           GadgetProtocolConstants::ESCAPE_BYTE == byte;
}

/**
 * The checksum is the penultimate 16 bits (in big-endian form), unless the checksum contains one of the special
 * characters.  If that is the case, the special characters are escaped as they would be in the payload.  This makes for
 * a bit of ugliness in determining the actual bytes of the checksum.  Starting at the end of the packet, we search for
 * a four-byte checksum, then a three-byte checksum and if neither of those match, it's a two byte checksum.  This
 * function returns a 2, 3 or 4 byte-vector with the segment that represents the checksum.  This function requires a
 * properly formatted Gadget Protocol packet.
 *
 * @param packet a properly formatted Gadget Protocol packet
 * @return a vector of the checksum portion of the packet.
 */
static std::vector<uint8_t> getChecksumSection(const std::vector<uint8_t>& packet) {
    // the end of the packet of a 4 byte checksum looks like this:
    //   begin to (end-6): ... header of packet ...
    //   end-5           : ESCAPE_BYTE
    //   end-4           : escaped byte (low-byte)
    //   end-3           : ESCAPE_BYTE
    //   end-2           : escaped byte (high-byte)
    //   end-1           : PACKET_END
    const std::vector<uint8_t> fourByteChecksum(
        std::end(packet) - (4 + sizeof(GadgetProtocolConstants::PACKET_END)),
        std::end(packet) - sizeof(GadgetProtocolConstants::PACKET_END));

    // check the 4 byte checksum.  Look for {ESCAPE_BYTE, escaped byte, ESCAPE_BYTE, escaped byte}
    if ((fourByteChecksum[0] == GadgetProtocolConstants::ESCAPE_BYTE) &&
        (isSpecialCharacter(GadgetProtocolConstants::ESCAPE_BYTE ^ fourByteChecksum[1])) &&
        (fourByteChecksum[2] == GadgetProtocolConstants::ESCAPE_BYTE) &&
        (isSpecialCharacter(GadgetProtocolConstants::ESCAPE_BYTE ^ fourByteChecksum[3]))) {
        return fourByteChecksum;
    }

    // the end of the packet of a 3 byte checksum takes on two forms:
    //   begin to (end-5): ... header of packet ...
    //   end-4           : unescaped byte (low-byte)
    //   end-3           : ESCAPE_BYTE
    //   end-2           : escaped byte (high-byte)
    //   end-1           : PACKET_END
    //   OR
    //   begin to (end-5): ... header of packet ...
    //   end-4           : ESCAPE_BYTE
    //   end-3           : escaped byte (low-byte)
    //   end-2           : unescaped byte (high-byte)
    //   end-1           : PACKET_END
    const std::vector<uint8_t> threeByteChecksum(
        std::end(packet) - (3 + sizeof(GadgetProtocolConstants::PACKET_END)),
        std::end(packet) - sizeof(GadgetProtocolConstants::PACKET_END));
    // check the 3 byte checksum.  Look for {ESCAPE_BYTE, escaped byte, unescaped byte} or {unescaped byte, ESCAPE_BYTE,
    // escaped byte}
    if (((threeByteChecksum[0] == GadgetProtocolConstants::ESCAPE_BYTE) &&
         (isSpecialCharacter(GadgetProtocolConstants::ESCAPE_BYTE ^ threeByteChecksum[1])) &&
         (!isSpecialCharacter(threeByteChecksum[2]))) ||
        ((!isSpecialCharacter(threeByteChecksum[0])) &&
         (threeByteChecksum[1] == GadgetProtocolConstants::ESCAPE_BYTE) &&
         (isSpecialCharacter(GadgetProtocolConstants::ESCAPE_BYTE ^ threeByteChecksum[2])))) {
        return threeByteChecksum;
    }

    // It must be the two byte checksum
    //   begin to (end-4): ... header of packet ...
    //   end-3           : unescaped byte (low-byte)
    //   end-2           : unescaped byte (high-byte)
    //   end-1           : PACKET_END
    return std::vector<uint8_t>(
        std::end(packet) - (2 + sizeof(GadgetProtocolConstants::PACKET_END)),
        std::end(packet) - sizeof(GadgetProtocolConstants::PACKET_END));
}

/**
 * This returns the 16-bit checksum of what is stored in the packet (not what is calculated).  If necessary, it is
 * byte-swapped, as it is in big-endian form in the packet.
 *
 * @param packet a properly formatted Gadget Protocol packet
 * @return the checksum of the packet
 */
static uint16_t readChecksum(const std::vector<uint8_t>& packet) {
    auto checksumSection = getChecksumSection(packet);

    union {
        uint8_t bytes[2];
        uint16_t value;
    } bigEndianChecksum;

    switch (checksumSection.size()) {
        case 4:
            // 4 byte checksum {ESCAPE_BYTE, escaped byte, ESCAPE_BYTE, escaped byte};
            bigEndianChecksum.bytes[0] = (GadgetProtocolConstants::ESCAPE_BYTE ^ checksumSection[1]);
            bigEndianChecksum.bytes[1] = (GadgetProtocolConstants::ESCAPE_BYTE ^ checksumSection[3]);
            break;
        case 3:
            // 3 byte checksum
            if (checksumSection[0] == GadgetProtocolConstants::ESCAPE_BYTE) {
                // this is the case {ESCAPE_BYTE, escaped byte, normal byte};
                bigEndianChecksum.bytes[0] = (GadgetProtocolConstants::ESCAPE_BYTE ^ checksumSection[1]);
                bigEndianChecksum.bytes[1] = checksumSection[2];
            } else {
                // this is the case {normal byte, ESCAPE_BYTE, escaped byte};
                bigEndianChecksum.bytes[0] = checksumSection[0];
                bigEndianChecksum.bytes[1] = (GadgetProtocolConstants::ESCAPE_BYTE ^ checksumSection[2]);
            }
            break;
        default:
            // two byte checksum
            bigEndianChecksum.value = *(reinterpret_cast<uint16_t*>(checksumSection.data()));
            break;
    }
    return ntohs(bigEndianChecksum.value);
}

/**
 * Cut off the head and footer of a Gadget Protocol packet.
 *
 * @param packet a properly formatted Gadget Protocol packet
 * @return the still-escaped payload of the Gadget Protocol packet.
 */
static std::vector<uint8_t> getPayloadSection(const std::vector<uint8_t>& packet) {
    // the header is 4 bytes (start of frame, Command ID, Error ID, Sequence ID).
    const int numberOfHeaderBytes = 4;

    // see how many characters trail the payload.
    const int numberOfTrailingBytes = getChecksumSection(packet).size() + sizeof(GadgetProtocolConstants::PACKET_END);

    return std::vector<uint8_t>(
        packet.begin() + numberOfHeaderBytes, packet.begin() + (packet.size() - numberOfTrailingBytes));
}

bool GadgetProtocol::decode(const std::vector<uint8_t>& packet, std::vector<uint8_t>* unpackedPayload) {
    if (!unpackedPayload) {
        ACSDK_ERROR(LX(__func__).m("null unpackedPayload"));
        return false;
    }

    /// The smallest packet possible, for use in determining if a packet to decode is large enough.
    static const auto minimumSizePacket = encode(std::vector<uint8_t>(), 0).second;

    if (packet.size() < minimumSizePacket.size()) {
        ACSDK_ERROR(LX(__func__).d("packet.size()", packet.size()).m("packet is too short"));
        return false;
    }

    // The following checks are safe because we already determined that packet is big enough.
    if (GadgetProtocolConstants::PACKET_BEGIN != packet.front()) {
        ACSDK_ERROR(LX(__func__).d("invalid StartOfFrame", static_cast<int>(packet.front())));
        return false;
    }

    const uint8_t commandId = packet[COMMAND_ID_INDEX];
    if (GadgetProtocolConstants::DEFAULT_COMMAND_ID != commandId) {
        ACSDK_ERROR(LX(__func__).d("invalid CommandId", static_cast<int>(commandId)));
        return false;
    }

    const uint8_t errorId = packet[ERROR_ID_INDEX];
    if (GadgetProtocolConstants::DEFAULT_ERROR_ID != errorId) {
        ACSDK_ERROR(LX(__func__).d("invalid ErrorId", static_cast<int>(errorId)));
        return false;
    }

    if (isSpecialCharacter(packet[SEQUENCE_ID_INDEX])) {
        ACSDK_ERROR(LX(__func__).d("invalid SequenceId", static_cast<int>(packet[SEQUENCE_ID_INDEX])));
        return false;
    }

    if (GadgetProtocolConstants::PACKET_END != packet.back()) {
        ACSDK_ERROR(LX(__func__).d("invalid EndOfFrame", static_cast<int>(packet.back())));
        return false;
    }

    auto packedPayload = getPayloadSection(packet);

    uint16_t checksum = (commandId + errorId);

    // the unpacked payload is at most the same size as the packedPayload.
    unpackedPayload->reserve(packedPayload.size());
    unpackedPayload->clear();

    bool escaping = false;
    for (const auto& byte : packedPayload) {
        if (escaping) {
            const uint8_t unescaped = GadgetProtocolConstants::ESCAPE_BYTE ^ byte;
            checksum += (unescaped);
            unpackedPayload->push_back(unescaped);
            escaping = false;
        } else {
            if (isSpecialCharacter(byte)) {
                escaping = true;
            } else {
                checksum += byte;
                unpackedPayload->push_back(byte);
            }
        }
    }

    if (checksum != readChecksum(packet)) {
        ACSDK_ERROR(LX(__func__).d("received", readChecksum(packet)).d("calculated", checksum).m("invalid checksum"));
        return false;
    }

    return true;
}

/**
 * Properly escape (if required) and append a byte onto a vector
 *
 * @param byte the byte to append
 * @param v the vector to append to
 */
static void append(uint8_t byte, std::vector<uint8_t>* v) {
    if (isSpecialCharacter(byte)) {
        v->push_back(GadgetProtocolConstants::ESCAPE_BYTE);
        v->push_back(GadgetProtocolConstants::ESCAPE_BYTE ^ byte);
    } else {
        v->push_back(byte);
    }
}

/**
 * Function to simplify the handling of the sequenceId.  The sequenceId is incremented, rolling over from 0xFF->0x00.
 * Special characters are skipped.
 *
 * @param sequenceId the previous sequence ID
 * @return the next sequence ID
 */
static uint8_t getNextSequenceId(uint8_t sequenceId) {
    ++sequenceId;

    // step over the special characters, as those are not valid sequence IDs
    while (isSpecialCharacter(sequenceId)) {
        ++sequenceId;
    }

    return sequenceId;
}

std::pair<uint8_t, std::vector<uint8_t>> GadgetProtocol::encode(
    const std::vector<uint8_t>& command,
    uint8_t prevSequenceId) {
    const uint8_t sequenceId = getNextSequenceId(prevSequenceId);
    const uint8_t commandId = GadgetProtocolConstants::DEFAULT_COMMAND_ID;
    const uint8_t errorId = GadgetProtocolConstants::DEFAULT_ERROR_ID;

    // the checksum is commandId + errorId + each (unescaped) byte in the payload.  It is calculated later in the
    // function, but is included here for the packet reservation.
    uint16_t checksum = commandId + errorId;

    std::vector<uint8_t> packet;
    // reserve the maximum size that could be needed.  The 2 * command.size() and @ * sizeof(checksum) is because each
    // byte might need to be escaped.
    packet.reserve(
        sizeof(GadgetProtocolConstants::PACKET_BEGIN) + sizeof(commandId) + sizeof(errorId) + sizeof(sequenceId) +
        2 * command.size() + 2 * sizeof(checksum) + sizeof(GadgetProtocolConstants::PACKET_END));

    // HEADER
    packet.push_back(GadgetProtocolConstants::PACKET_BEGIN);
    // The spec says that these are specific values; it also says that they should be escaped if they are special
    // characters.  Play it safe and use a common insertion interface.
    append(commandId, &packet);
    append(errorId, &packet);
    append(sequenceId, &packet);

    // PAYLOAD
    for (const auto& byte : command) {
        checksum += byte;
        append(byte, &packet);
    }

    // The packet is big-endian, so insert the higher order byte first.
    append((checksum >> 8) & 0xFF, &packet);
    append(checksum & 0xFF, &packet);

    packet.push_back(GadgetProtocolConstants::PACKET_END);

    return std::make_pair(sequenceId, packet);
}

}  // namespace gadgetManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
