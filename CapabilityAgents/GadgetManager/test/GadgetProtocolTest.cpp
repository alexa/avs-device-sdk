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

#include <gtest/gtest.h>

#include <set>
#include <unordered_set>

#ifdef _WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif

#include "GadgetManager/GadgetProtocol.h"
#include "GadgetManager/GadgetProtocolConstants.h"
#include "GenerateRandomVector.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace gadgetManager {
namespace test {

/// Utility function to determine if the packet is properly formed
static bool isValid(const std::vector<uint8_t>& packet) {
    std::vector<uint8_t> dummyPayload;
    return GadgetProtocol::decode(packet, &dummyPayload);
}

/// Verify proper error handling on bad parameters.
TEST(GadgetProtocolTest, DecodeNullOutputParameter) {
    const auto valid = GadgetProtocol::encode(std::vector<uint8_t>(), 0).second;
    ASSERT_TRUE(isValid(valid));
    ASSERT_FALSE(GadgetProtocol::decode(valid, nullptr));
}

/// Check that packets that are too short are rejected.
TEST(GadgetProtocolTest, InvalidLength) {
    const auto smallestValid = GadgetProtocol::encode(std::vector<uint8_t>(), 0).second;
    ASSERT_TRUE(isValid(smallestValid));

    // all lengths under the valid packet are illegal
    for (size_t length = 0; length < smallestValid.size(); ++length) {
        ASSERT_FALSE(isValid(std::vector<uint8_t>(std::begin(smallestValid), std::begin(smallestValid) + length)));
    }
}

/// utility function that creates a packet, modifies the byte at the specificed index and checks to see if the value
/// it's being changed to is in the set of possible correct bytes.  The packet should be invalid if the byte is changes
/// to something outside of the correct bytes.
static bool modifyThenCheckForValidity(const std::unordered_set<uint8_t>& possibleCorrectBytes, size_t indexToChange) {
    auto packet = GadgetProtocol::encode(std::vector<uint8_t>(), 0).second;
    for (int i = 0; i < 0x100; ++i) {
        const uint8_t byteToChangeTo = i & 0xFF;
        if (indexToChange > packet.size()) {
            return false;
        }
        packet[indexToChange] = byteToChangeTo;

        if ((possibleCorrectBytes.find(byteToChangeTo) != possibleCorrectBytes.end()) != isValid(packet)) {
            // isValid should return true when byteToChangeTo is in possibleCorrectBytes
            return false;
        }
    }

    return true;
}

/// Test to change the 1st byte.
TEST(GadgetProtocolTest, InvalidStartOfFrame) {
    // the SOF is at index 0
    ASSERT_TRUE(modifyThenCheckForValidity(std::unordered_set<uint8_t>{GadgetProtocolConstants::PACKET_BEGIN}, 0));
}

/// Test to change the 2nd byte.
TEST(GadgetProtocolTest, InvalidCommandId) {
    // the commandID is at index 1
    ASSERT_TRUE(
        modifyThenCheckForValidity(std::unordered_set<uint8_t>{GadgetProtocolConstants::DEFAULT_COMMAND_ID}, 1));
}

/// Test to change the 3rd byte.
TEST(GadgetProtocolTest, InvalidErrorId) {
    // the errorID is at index 2
    ASSERT_TRUE(modifyThenCheckForValidity(std::unordered_set<uint8_t>{GadgetProtocolConstants::DEFAULT_ERROR_ID}, 2));
}

/// Test to change the 4th byte.
TEST(GadgetProtocolTest, InvalidSequenceId) {
    // create a set of all legitimate sequenceId values (i.e. everything except SOF, EOF and the ESCAPE_BYTE byte)
    std::unordered_set<uint8_t> goodSequenceIdValues;
    for (int i = 0; i < 0x100; ++i) {
        if ((GadgetProtocolConstants::PACKET_BEGIN != i) && (GadgetProtocolConstants::PACKET_END != i) &&
            (GadgetProtocolConstants::ESCAPE_BYTE != i)) {
            goodSequenceIdValues.insert(i);
        }
    }

    // the sequenceID is at index 3
    ASSERT_TRUE(modifyThenCheckForValidity(goodSequenceIdValues, 3));
}

/// Utility function to determine if a byte is a special character.
static bool isSpecial(uint8_t byte) {
    return GadgetProtocolConstants::PACKET_BEGIN == byte || GadgetProtocolConstants::PACKET_END == byte ||
           GadgetProtocolConstants::ESCAPE_BYTE == byte;
}

/// Counts the number of escaped bytes in a checksum
static size_t numEscapedBytes(uint16_t checksum) {
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(&checksum);
    return isSpecial(*bytePtr) + isSpecial(*(bytePtr + 1));
}

/// Given a 16 bit checksum, create a vector that escapes the bytes properly.  The checksum vector is big-endian.
static std::vector<uint8_t> createEscapedChecksum(uint16_t checksum) {
    std::vector<uint8_t> checksumVector;
    for (unsigned int i = sizeof(checksum); i > 0; --i) {
        const uint8_t workingByte = (checksum >> (8 * (i - 1))) & 0xFF;
        if (isSpecial(workingByte)) {
            checksumVector.push_back(GadgetProtocolConstants::ESCAPE_BYTE);
            checksumVector.push_back(workingByte ^ GadgetProtocolConstants::ESCAPE_BYTE);
        } else {
            checksumVector.push_back(workingByte);
        }
    }

    return checksumVector;
}

/// Test for cases where the checksum has no escape bytes.  It checks to see that if the checksum is incorrect it fails
/// to decode and if it's correct, it decodes correctly.
TEST(GadgetProtocolTest, ChecksumNoEscapeBytes) {
    // zero length payload results in checksum that is CommandID + ErrorID
    const uint16_t expectedChecksum =
        GadgetProtocolConstants::DEFAULT_COMMAND_ID + GadgetProtocolConstants::DEFAULT_ERROR_ID;
    for (int i = 0; i < 0x10000; ++i) {
        const size_t CHECKSUM_INDEX = 4;
        const uint16_t checksum = i & 0xFFFF;
        if (0 == numEscapedBytes(checksum)) {
            auto packet = GadgetProtocol::encode(std::vector<uint8_t>(), 0).second;
            ASSERT_GE(
                packet.size(),
                CHECKSUM_INDEX + sizeof(checksum));  // verfiy the packet is long enough for the next statement

            // overwrite the checksum with 'checksum'
            uint16_t* checksumPtr = reinterpret_cast<uint16_t*>(packet.data() + CHECKSUM_INDEX);
            *checksumPtr = htons(checksum);

            // it should be invalid
            ASSERT_EQ(checksum == expectedChecksum, isValid(packet));
        }
    }
}

/// This function is for testing checksums that have at least one escaped byte.  A packet is created and all of the 3 or
/// 4 byte checksums are substituted into the Gadget Protocol packet (depending on whether the expected checksum needs 1
/// or 2 escaped bytes).  The test checks to see wheter or not the packet is valid with the substituted checksum.  If
/// the substituted checksum is the same as the expected checksum, the packet should be valid, otherwise it is invalid.
static void checkEscapedByteChecksum(uint16_t expectedChecksum) {
    // First step, generate a payload so the expected checksum is correct.
    uint16_t currentChecksum =
        (GadgetProtocolConstants::DEFAULT_COMMAND_ID + GadgetProtocolConstants::DEFAULT_ERROR_ID);
    std::vector<uint8_t> payload;
    // keep the step below 0xF0 so we don't get any escape bytes in the payload.
    const uint8_t step = 0xEE;
    while (currentChecksum + step < expectedChecksum) {
        payload.push_back(step);
        currentChecksum += step;
    }

    const int finalPayloadByteToGenerateExpectedChecksum = expectedChecksum - currentChecksum;
    ASSERT_LE(finalPayloadByteToGenerateExpectedChecksum, step);
    payload.push_back(finalPayloadByteToGenerateExpectedChecksum);

    // Exhaustively search the checksum space for all checksums that are the same length as the expected checksum.  This
    // is so they can be substituted into the Gadget Protocol packet without any packet size modification.
    for (int i = 0; i < 0x10000; ++i) {
        const uint16_t checksum = i & 0xFFFF;

        const size_t expectedChecksumVectorSize = 2 + numEscapedBytes(checksum);
        if (expectedChecksumVectorSize == (numEscapedBytes(expectedChecksum) + 2)) {
            auto packet = GadgetProtocol::encode(payload, 0).second;
            const size_t CHECKSUM_INDEX = 4 + payload.size();

            auto checksumVector = createEscapedChecksum(checksum);
            ASSERT_EQ(expectedChecksumVectorSize, checksumVector.size());

            memcpy(packet.data() + CHECKSUM_INDEX, checksumVector.data(), checksumVector.size());

            ASSERT_EQ(checksum == expectedChecksum, isValid(packet));
        }
    }
}

/// Test where the checksum has a leading escaped byte (e.g. 0xF200)
TEST(GadgetProtocolTest, ChecksumLeadingEscapeByte) {
    for (const auto& leadingEscapeByte : std::set<uint8_t>{GadgetProtocolConstants::PACKET_BEGIN,
                                                           GadgetProtocolConstants::PACKET_END,
                                                           GadgetProtocolConstants::ESCAPE_BYTE}) {
        checkEscapedByteChecksum(leadingEscapeByte << 8);
    }
}

/// Test where the checksum has a trailing escaped byte (e.g. 0x00F1)
TEST(GadgetProtocolTest, ChecksumTrailingEscapeByte) {
    for (const auto& leadingEscapeByte : std::set<uint8_t>{GadgetProtocolConstants::PACKET_BEGIN,
                                                           GadgetProtocolConstants::PACKET_END,
                                                           GadgetProtocolConstants::ESCAPE_BYTE}) {
        checkEscapedByteChecksum(leadingEscapeByte);
    }
}

/// Test where the checksum has both a leading and trailing escaped byte (e.g. 0xF1F1)
TEST(GadgetProtocolTest, ChecksumLeadingAndTrailingEscapeByte) {
    std::set<uint8_t> escapeBytes = {GadgetProtocolConstants::PACKET_BEGIN,
                                     GadgetProtocolConstants::PACKET_END,
                                     GadgetProtocolConstants::ESCAPE_BYTE};
    for (const auto& leadingByte : escapeBytes) {
        for (const auto& trailingByte : escapeBytes) {
            const uint16_t expectedChecksum = (leadingByte << 8 | trailingByte);
            checkEscapedByteChecksum(expectedChecksum);
        }
    }
}

/// Test where the end of frame byte is replaced and packet validity is checked.
TEST(GadgetProtocolTest, InvalidEndOfFrame) {
    for (int i = 0; i < 0x100; ++i) {
        const uint8_t endOfFrame = i & 0xFF;
        auto packet = GadgetProtocol::encode(std::vector<uint8_t>(), 0).second;
        ASSERT_GE(packet.size(), 1u);  // verfiy the packet is long enough for the next statement
        packet[packet.size() - 1] = endOfFrame;

        // all values for the end byte except PACKET_END shall fail
        ASSERT_EQ(endOfFrame == GadgetProtocolConstants::PACKET_END, isValid(packet));
    }
}

/// Insert special characters into the payload to verify that they are escaped correctly.
TEST(GadgetProtocolTest, EscapeBytesInPayload) {
    for (const auto& escapeByte : std::set<uint8_t>{GadgetProtocolConstants::PACKET_BEGIN,
                                                    GadgetProtocolConstants::PACKET_END,
                                                    GadgetProtocolConstants::ESCAPE_BYTE}) {
        std::vector<uint8_t> payload{escapeByte};
        auto packet = GadgetProtocol::encode(payload, 0).second;
        ASSERT_GE(packet.size(), 6u);
        ASSERT_EQ(GadgetProtocolConstants::ESCAPE_BYTE, packet[4]);
        ASSERT_EQ(GadgetProtocolConstants::ESCAPE_BYTE ^ escapeByte, packet[5]);
    }
}

/// Insert normal bytes into the payload to verify that they are never escapedescaped correctly.
TEST(GadgetProtocolTest, NonEscapeBytesInPayload) {
    for (const auto& normalByte : std::set<uint8_t>{0, 1, 2, 3, 4, 5, 6, 0xF3}) {
        std::vector<uint8_t> payload{normalByte};
        auto packet = GadgetProtocol::encode(payload, 0).second;
        ASSERT_GE(packet.size(), 5u);
        ASSERT_EQ(normalByte, packet[4]);
    }
}

// Test normal sequenceId operation
TEST(GadgetProtocolTest, UnescapedSequenceId) {
    for (int i = 0; i < 0x100; ++i) {
        uint8_t prevSequenceId = (0xFF & i);
        if (!isSpecial(prevSequenceId + 1)) {
            auto encodeResult = GadgetProtocol::encode(std::vector<uint8_t>(), prevSequenceId);
            ASSERT_EQ((prevSequenceId + 1) & 0xFF, encodeResult.first);
            ASSERT_EQ(encodeResult.second[3], encodeResult.first);
        }
    }
}

/// Test skipping of special byte SequenceId's.
TEST(GadgetProtocolTest, EscapedSequenceId) {
    for (const auto& escapeByte : std::set<uint8_t>{GadgetProtocolConstants::PACKET_BEGIN,
                                                    GadgetProtocolConstants::PACKET_END,
                                                    GadgetProtocolConstants::ESCAPE_BYTE}) {
        auto encodeResult = GadgetProtocol::encode(std::vector<uint8_t>(), escapeByte);
        ASSERT_EQ(0xF3, encodeResult.first);
        ASSERT_EQ(encodeResult.second[3], encodeResult.first);
    }
}

/// Test the SequenceId wrapping around from 0xFF to 0x00.
TEST(GadgetProtocolTest, WrappedSequenceId) {
    auto encodeResult = GadgetProtocol::encode(std::vector<uint8_t>(), 0xFF);
    ASSERT_EQ(0x00, encodeResult.first);
    ASSERT_EQ(encodeResult.second[3], encodeResult.first);
}

static void encodeAndDecode(const std::vector<uint8_t> originalPayload) {
    const auto packet = GadgetProtocol::encode(originalPayload, 0).second;
    std::vector<uint8_t> unpackedPayload;
    ASSERT_TRUE(GadgetProtocol::decode(packet, &unpackedPayload));
    ASSERT_EQ(originalPayload, unpackedPayload);
}

/// Test the encoding and decoding of some representative payloads.
TEST(GadgetProtocolTest, InterestingPayloads) {
    const std::vector<std::vector<uint8_t>> payloads = {{},
                                                        {0},
                                                        {0xF0},
                                                        {0xF1},
                                                        {0xF2},
                                                        {0xF3},
                                                        {0x00, 0xF0},
                                                        {0xF0, 0x00},
                                                        {0x01, 0xF0, 0x02},
                                                        {0xF0, 0xF1},
                                                        {0xF0, 0xF1, 0xF2},
                                                        {0xF2, 0x11, 0xF1}};

    for (const auto& payload : payloads) {
        encodeAndDecode(payload);
    }
}

/// Generate random vectors and see if an encode and decode results in the same original payload.
TEST(GadgetProtocolTest, FuzzTest) {
    for (int i = 0; i < 1024; ++i) {
        auto payload = generateRandomVector(251);
        encodeAndDecode(payload);
    }
}

}  // namespace test
}  // namespace gadgetManager
}  // namespace capabilityAgents
}  // namespace alexaClientSDK
