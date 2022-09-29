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

#include <string>
#include <cstring>
#include <future>
#include <vector>
#include <unordered_set>
#include <random>
#include <algorithm>

#include <gmock/gmock.h>

#include "AVSCommon/Utils/Common/Common.h"
#include "AVSCommon/Utils/HTTP/HttpResponseCode.h"
#include "AVSCommon/Utils/HTTP2/HTTP2MimeRequestEncoder.h"
#include "AVSCommon/Utils/HTTP2/HTTP2MimeResponseDecoder.h"
#include "AVSCommon/Utils/HTTP2/MockHTTP2MimeRequestEncodeSource.h"
#include "AVSCommon/Utils/HTTP2/MockHTTP2MimeResponseDecodeSink.h"
#include "AVSCommon/Utils/Logger/LoggerUtils.h"

namespace alexaClientSDK {
namespace avsCommon {
namespace test {

using namespace testing;
using namespace avsCommon::utils::http;
using namespace avsCommon::utils::http2;
using namespace avsCommon::utils;
using namespace avsCommon::utils::logger;

/// Separator for keys and values in mime part headers.
static const std::string SEPARATOR = ": ";

/// Guideline sizes for test payloads and headers
static const int SMALL{100};
static const int MEDIUM{200};
static const int LARGE{500};
static const int XLARGE{5000};
static const int HEADER_PART_SIZE{10};

/// Boundary test constants
/// Response header prefix used to set boundary for decoder
static const std::string BOUNDARY_HEADER_PREFIX = "content-type:mixed/multipart;boundary=";
/// A test boundary string, copied from a real interaction with AVS.
static const std::string MIME_TEST_BOUNDARY_STRING = "84109348-943b-4446-85e6-e73eda9fac43";
/// The newline characters that MIME parsers expect.
static const std::string MIME_NEWLINE = "\r\n";
/// The double dashes which may occur before and after a boundary string.
static const std::string MIME_BOUNDARY_DASHES = "--";
/// The test boundary string with the preceding dashes.
static const std::string BOUNDARY = MIME_BOUNDARY_DASHES + MIME_TEST_BOUNDARY_STRING;
/// A complete boundary, including the CRLF prefix
static const std::string BOUNDARY_LINE = MIME_NEWLINE + BOUNDARY;
/// Header line without prefix or suffix CRLF.
static const std::string HEADER_LINE = "Content-Type: application/json";
/// JSON payload.
static const std::string TEST_MESSAGE =
    "{\"directive\":{\"header\":{\"namespace\":\"SpeechRecognizer\",\"name\":"
    "\"StopCapture\",\"messageId\":\"4e5612af-e05c-4611-8910-1e23f47ffb41\"},"
    "\"payload\":{}}}";
/// The " char
static const std::string QUOTE_CHAR = "\"";

// The following *_LINES definitions are raw mime text for various test parts. Each one assumes that
// it will be prefixed by a boundary and a CRLF. These get concatenated by constructTestMimeString()
// which provides an initiating boundary and CRLF, and which also inserts a CRLF between each part
// that is added. Leaving out the terminal CRLFs here allows constructTestMimeString() to append a
// pair of dashes to the boundary terminating the last part. Those final dashes are the standard
// syntax for the end of a sequence of mime parts.

/// Normal section with header, test message and terminating boundary
/// @note assumes previous terminating boundary and CRLF in the mime stream that this is appended to.
static const std::string NORMAL_LINES = HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + TEST_MESSAGE + BOUNDARY_LINE;
/// Normal section preceded by a duplicate boundary (one CRLF between boundaries)
/// @note assumes previous terminating boundary and CRLF in the mime stream that this is appended to.
static const std::string DUPLICATE_BOUNDARY_LINES = BOUNDARY + MIME_NEWLINE + NORMAL_LINES;
/// Normal section preceded by a duplicate boundary and CRLF (two CRLFs between boundaries)
/// @note assumes previous terminating boundary and CRLF in the mime stream that this is appended to.
static const std::string CRLF_DUPLICATE_BOUNDARY_LINES = BOUNDARY_LINE + MIME_NEWLINE + NORMAL_LINES;
/// Normal section preceded by triplicate boundaries (one CRLF between boundaries)
/// @note assumes previous terminating boundary and CRLF in the mime stream that this is appended to.
static const std::string TRIPLICATE_BOUNDARY_LINES = BOUNDARY + MIME_NEWLINE + BOUNDARY + MIME_NEWLINE + NORMAL_LINES;
/// Normal section preceded by triplicate boundaries with trailing CRLF (two CRLFs between boundaries)
/// @note assumes previous terminating boundary and CRLF in the mime stream that this is appended to.
static const std::string CRLF_TRIPLICATE_BOUNDARY_LINES =
    BOUNDARY_LINE + MIME_NEWLINE + BOUNDARY_LINE + MIME_NEWLINE + NORMAL_LINES;

class MIMEParserTest : public ::testing::Test {
public:
    /// boundary
    const std::string boundary{"wooohooo"};
    /// payloads
    const std::string payload1{"The quick brown fox jumped over the lazy dog"};
    const std::string payload2{
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
        "tempor incididunt ut labore et dolore magna aliqua.\n Ut enim ad minim "
        "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
        "commodo consequat.\n Duis aute irure dolor in reprehenderit in "
        "voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n "
        "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui "
        "officia deserunt mollit anim id est laborum."};
    const std::string payload3{
        "Enim diam vulputate ut pharetra sit amet aliquam id. Viverra accumsan "
        "in nisl nisi scelerisque eu. Ipsum nunc aliquet bibendum enim facilisis "
        "gravida neque convallis a. Ullamcorper dignissim cras tincidunt "
        "lobortis. Mi proin sed libero enim sed faucibus turpis in."};

    // Header Keys
    const std::string key1{"content-type"};
    const std::string key2{"content-type"};
    const std::string key3{"xyz-abc"};
    const std::string key4{"holy-cow"};
    const std::string key5{"x-amzn-id"};

    // header Values
    const std::string value1{"plain/text"};
    const std::string value2{"application/xml"};
    const std::string value3{"123243124"};
    const std::string value4{"tellmehow"};
    const std::string value5{"eg1782ge71g172ge1"};

/// headers
#define BUILD_HEADER(number) key##number + SEPARATOR + value##number
    const std::string header1{BUILD_HEADER(1)};
    const std::string header2{BUILD_HEADER(2)};
    const std::string header3{BUILD_HEADER(3)};
    const std::string header4{BUILD_HEADER(4)};
    const std::string header5{BUILD_HEADER(5)};
#undef BUILD_HEADER

    /// Expected output from encoding the above
    const std::string encodedPayload =
        "\r\n--wooohooo"
        "\r\ncontent-type: application/xml"
        "\r\nxyz-abc: 123243124"
        "\r\nholy-cow: tellmehow"
        "\r\n"
        "\r\nThe quick brown fox jumped over the lazy dog"
        "\r\n--wooohooo"
        "\r\ncontent-type: plain/text"
        "\r\nx-amzn-id: eg1782ge71g172ge1"
        "\r\n"
        "\r\nLorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt "
        "ut labore et dolore magna aliqua.\n Ut enim ad minim veniam, quis nostrud exercitation ullamco "
        "laboris nisi ut aliquip ex ea commodo consequat.\n Duis aute irure dolor in reprehenderit in "
        "voluptate velit esse cillum dolore eu fugiat nulla pariatur.\n Excepteur sint occaecat cupidatat "
        "non proident, sunt in culpa qui officia deserunt mollit anim id est laborum."
        "\r\n--wooohooo"
        "\r\ncontent-type: plain/text"
        "\r\n"
        "\r\nEnim diam vulputate ut pharetra sit amet aliquam id. Viverra accumsan in nisl nisi "
        "scelerisque eu. Ipsum nunc aliquet bibendum enim facilisis gravida neque convallis a. "
        "Ullamcorper dignissim cras tincidunt lobortis. Mi proin sed libero enim sed faucibus turpis in."
        "\r\n"
        "--wooohooo--\r\n";

    /// Expected size
    const size_t encodedSize = encodedPayload.size();
    /// Header sets for each payload
    const std::vector<std::string> headers1 = {header2, header3, header4};
    const std::vector<std::string> headers2 = {header1, header5};
    const std::vector<std::string> headers3 = {header1};
    /// Expected header values.
    const std::vector<std::multimap<std::string, std::string>> expectedHeaders = {
        {{key2, value2}, {key3, value3}, {key4, value4}},
        {{key1, value1}, {key5, value5}},
        {{key1, value1}}};
    /// Expected data (payloads).
    const std::vector<std::string> expectedData = {payload1, payload2, payload3};
};

/**
 * Test the basic encoding use case with a 3 part MIME request
 * Test for correct encoded size, header presence and validity
 * of Return status for every call as well as bytes written.
 */
TEST_F(MIMEParserTest, test_encodingSanity) {
    /// We choose an arbitrary buffer size
    const int bufferSize{25};

    std::vector<std::string> data;
    data.push_back(payload1);
    data.push_back(payload2);
    data.push_back(payload3);

    std::vector<std::vector<std::string>> headerSets;
    headerSets.push_back(headers1);
    headerSets.push_back(headers2);
    headerSets.push_back(headers3);

    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);
    HTTP2MimeRequestEncoder encoder{boundary, source};

    /// characters array to store the output, size chosen to be more than
    /// the size calculated above
    std::vector<char> bufferVec(encodedPayload.size() * 2);
    char* buf = bufferVec.data();
    size_t index{0};
    size_t lastSize{bufferSize};
    HTTP2SendDataResult result{0};
    while (result.status == HTTP2SendStatus::CONTINUE) {
        result = encoder.onSendData(buf + index, bufferSize);
        index += result.size;
        // size returned should be bufferSize followed by {0,buffer_size}(last chunk)
        if (lastSize < result.size) {
            FAIL();
        } else {
            lastSize = result.size;
        }
    }
    ASSERT_EQ(HTTP2SendStatus::COMPLETE, result.status);
    ASSERT_TRUE(strncmp(encodedPayload.c_str(), buf, encodedSize) == 0);
    ASSERT_EQ(index, encodedSize);
}

/*
 * Helper method to run boundary check tests.
 */
void runDecodingBoundariesTest(
    const std::vector<std::pair<std::string, size_t>>& partsToIndex,
    const std::vector<std::multimap<std::string, std::string>>& testCaseExpectedHeaders,
    const std::vector<std::string>& testCaseExpectedData,
    const std::vector<std::string>& headers,
    HTTP2ReceiveDataStatus expectedStatus = HTTP2ReceiveDataStatus::SUCCESS) {
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    HTTP2MimeResponseDecoder decoder{sink};
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    for (auto header : headers) {
        ASSERT_TRUE(decoder.onReceiveHeaderLine(header));
    }
    decoder.onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);
    // send the data part by part like the cloud does.
    for (auto part : partsToIndex) {
        status = decoder.onReceiveData(part.first.c_str(), part.first.size());
        // the class MockHTTP2MimeResponseDecodeSink is not a real gtest mock so we cannot use EXPECT_CALL
        // this assertion is trying to detect if a partEnd call happened, we use m_index to detect that
        ASSERT_EQ(sink->m_index, part.second);
    }
    ASSERT_EQ(sink->m_index, partsToIndex.back().second);
    ASSERT_EQ(sink->m_headers, testCaseExpectedHeaders);
    ASSERT_EQ(sink->m_data, testCaseExpectedData);
    ASSERT_EQ(expectedStatus, status);
}

/*
 * Helper method to run boundary tests with a default payload, and provided headers
 */
void generatePayloadAndTest(const std::vector<std::string>& headers, const std::string& payloadBoundary) {
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--" + payloadBoundary + "\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"1111", 0},
        {"2222", 0},
        {"3333", 0},
        {"\r\n--" + payloadBoundary, 1},  // the boundary is sent on its own and we want to detect a part end here.
        {"\r\ncontent-type: multipart/related\r\n\r\nlast\r\n--" + payloadBoundary, 2},
        {"--\r\n", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"111122223333", "last"};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Helper method to run boundary tests with a default payload, and provided boundary
 */
void generatePayloadAndTest(const std::string& headerBoundary, const std::string& payloadBoundary) {
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + headerBoundary};
    generatePayloadAndTest(headers, payloadBoundary);
}

/*
 * Given a partition and a payload, send each part of the payload individually.
 * Assert if the parsing is correct and if the content is as expected.
 */
void runTestForCombination(std::vector<int>& partition, const std::string& payload, const std::string& boundary) {
    // we can skip a combination that has a empty partition, this means that the same combination will appear elsewhere.
    // example numberOfPartitions = 3 => [][a][bcd] is the same as numberOfPartitions = 2 => [a][bcd]
    if (std::find(partition.begin(), partition.end(), 0) != partition.end()) {
        return;
    }

    const std::vector<std::string> testCaseExpectedData = {"data"};
    const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + boundary};
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    HTTP2MimeResponseDecoder decoder{sink};
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
    ASSERT_TRUE(resp);
    decoder.onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);

    size_t pIndex = 0;
    for (size_t i = 0; i < partition.size(); i++) {
        // send a part of the payload to the parser
        status = decoder.onReceiveData(payload.c_str() + pIndex, partition[i]);
        ASSERT_EQ(status, HTTP2ReceiveDataStatus::SUCCESS);
        pIndex += partition[i];
    }
    // at the end of all parts check if the message contents is as expected.
    ASSERT_EQ(sink->m_data, testCaseExpectedData);
}

void generateCombinationsAndRunTest(
    std::vector<int>& partitions,
    size_t pos,
    int remaining,
    const std::string& words,
    const std::string& boundary) {
    if (remaining == 0) {
        runTestForCombination(partitions, words, boundary);
        return;
    }
    if (pos == partitions.size()) {
        return;
    }
    for (int i = remaining; i >= 0; --i) {
        partitions[pos] = i;
        generateCombinationsAndRunTest(partitions, pos + 1, remaining - i, words, boundary);
    }
}

/*
 * This test will split the payload into groups [abcd] of a given size k.
 * We then vary k to have all possible group sizes.
 * numberOfPartitions = 1 => [abcd]
 * numberOfPartitions = 2 => [abc,d], [ab,cd], [a,bcd]
 * until numberOfPartitions = payload.size (one char at a time)
 */
TEST_F(MIMEParserTest, test_multipleCombinations) {
    std::string payload =
        "--WWWoooAAA\r\nContent-Type: "
        "application/json\r\n\r\ndata\r\n--WWWoooAAA--\r\n";
    size_t numberOfPartitionsIncrement = 1;
    // this test can be really slow, so the maxNumberOfPartitions is chosen so it can run fast
    // if any potential problem is detected in MIME parser this can be changed to payload.length to run all possible
    // combinations this will take at least 4 hours to run as it tests all possible combinations
    size_t maxNumberOfPartitions = 3;

    for (size_t numberOfPartitions = 1; numberOfPartitions <= maxNumberOfPartitions;
         numberOfPartitions += numberOfPartitionsIncrement) {
        std::vector<int> partitions(numberOfPartitions);
        generateCombinationsAndRunTest(partitions, 0, static_cast<int>(payload.size()), payload, "WWWoooAAA");
    }
}

/*
 * Test: Sends the data in groups of fixed size (from 1 to the size of the payload)
 *  example string: abcdef, group size = 3
 *     [abc],[def]
 * Expected Result: Pass
 */
TEST_F(MIMEParserTest, test_fixedSizeGroups) {
    std::string parts =
        "--whoLetTheDogsOut\r\ncharset: UTF-8\r\nContent-Type: "
        "application/json\r\n\r\n{json-content1}\r\n--whoLetTheDogsOut\r\ncharset: UTF-8\r\nContent-Type: "
        "application/json\r\n\r\n{json-content2}\r\n--whoLetTheDogsOut--\r\n";

    const std::vector<std::string> testCaseExpectedData = {"{json-content1}", "{json-content2}"};

    for (size_t groupSize = 1; groupSize <= parts.size(); groupSize++) {
        const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + "whoLetTheDogsOut"};
        auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
        HTTP2MimeResponseDecoder decoder{sink};
        HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
        bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
        ASSERT_TRUE(resp);
        decoder.onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);

        std::vector<std::string> groups;
        size_t nGroups = (parts.size() + groupSize - 1) / groupSize;
        for (size_t i = 0; i < nGroups; i++) {
            groups.push_back("");
        }

        size_t currentGroup = 0;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i != 0 && i % groupSize == 0) {
                currentGroup++;
            }
            groups[currentGroup] = groups[currentGroup] + parts[i];
        }

        // send the data part by part
        for (auto part : groups) {
            status = decoder.onReceiveData(part.c_str(), part.size());
            ASSERT_EQ(status, HTTP2ReceiveDataStatus::SUCCESS);
        }
        ASSERT_EQ(sink->m_data, testCaseExpectedData);
    }
}

/*
 * Test: Sends the boundary without a CRLF and more data, this should raise an issue as the boundary is required to be
 *       terminated by CRLF
 * Expected Result: an ABORT error should be returned
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendBoundaryWithoutCRLF) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {{"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
                                                         {"Part1", 0},
                                                         {"\r\n--wooohoooMorePart1", 1},
                                                         {"\r\n--wooohooo", 1},
                                                         {"\r\ncontent-type: multipart/related\r\n\r\n", 1},
                                                         {"Part2\r\n", 1},
                                                         {"--wooohooo--\r\n", 1}};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"Part1"};
    runDecodingBoundariesTest(
        parts, testCaseExpectedHeaders, testCaseExpectedData, headers, HTTP2ReceiveDataStatus::ABORT);
}

/*
 * Test: sends a -- after the boundary which will terminate the transfer considering everything else as a epilogue.
 * Expected Result: consider everything before -- as data and discard everything after --
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendBoundaryWithTerminatorShouldIgnoreEpilogue) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {{"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
                                                         {"Part1", 0},
                                                         {"\r\n--wooohooo", 1},
                                                         {"--hello", 1},  // this will end the transfer (--wooohooo--)
                                                         {"\r\ncontent-type: multipart/related\r\n\r\n", 1},
                                                         {"Part2\r\n", 1},
                                                         {"--wooohooo--\r\n", 1}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"Part1"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: sends a double boundary, the second one will be ignored
 * Expected Result: second boundary ignored
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendDuplicatedBoundaryAsHeader) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {{"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
                                                         {"Part1", 0},
                                                         {"\r\n--wooohooo\r\n", 1},
                                                         {"\r\n--wooohooo", 1},
                                                         {"\r\ncontent-type: multipart2/related\r\n\r\n", 1},
                                                         {"Part2\r\n", 1},
                                                         {"--wooohooo--\r\n", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart2/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"Part1", "Part2"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: sends boundaries using the allowed chars
 * Expected Result: all cases pass
 */
TEST_F(MIMEParserTest, test_decodingRandomBoundaries) {
    const std::string CHARACTERS = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz'()+_,-.:=?";
    std::random_device random_device;
    std::mt19937 generator(random_device());
    std::uniform_int_distribution<> distribution(0, static_cast<int>(CHARACTERS.size() - 1));

    for (size_t length = 5; length < 50; length++) {
        std::string quotedRandomBoundary = QUOTE_CHAR;
        std::string randomBoundary = "";
        for (std::size_t i = 0; i < length; ++i) {
            char newChar = CHARACTERS[distribution(generator)];
            quotedRandomBoundary += newChar;
            randomBoundary += newChar;
        }
        quotedRandomBoundary += QUOTE_CHAR;
        generatePayloadAndTest(quotedRandomBoundary, randomBoundary);
        generatePayloadAndTest(randomBoundary, randomBoundary);
    }
}

/*
 * The RFC is not extra clear on the quoted boundary cases, so we are being lenient
 * when there's a quote inside the boundary we ignore what's after the second quote and process the rest
 * using the same rule as the unquoted case.
 */
TEST_F(MIMEParserTest, test_decodingBoundariesWithQuotesAndMoreHeaders) {
    std::string payloadBoundary = "mybou";
    std::string unquotedBoundary = payloadBoundary + QUOTE_CHAR + "ndary";
    std::string quotedBoundary = QUOTE_CHAR + unquotedBoundary + QUOTE_CHAR;
    std::vector<std::string> headerAfterBoundary = {"", ";otherprop=yes", " somethingelse"};
    for (auto extraHeader : headerAfterBoundary) {
        std::string boundaryHeader = quotedBoundary + extraHeader;
        generatePayloadAndTest(boundaryHeader, payloadBoundary);
    }
}

/*
 * Test: sends invalid boundaries
 * Expected Result: all cases should fail
 */
TEST_F(MIMEParserTest, test_decodingInvalidBoundaries) {
    std::vector<std::string> headerAfterBoundary = {"", ";otherprop=yes", " somethingelse"};
    std::vector<std::string> invalid_boundaries = {
        "",
        "thisstringhasmorethanseventycharacterssoitsinvalid123123123123123123123",
        "^invalidchar",
        "\"^invalidchar\""};
    for (auto testBoundary : invalid_boundaries) {
        for (auto header : headerAfterBoundary) {
            const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + testBoundary + header};
            auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
            HTTP2MimeResponseDecoder decoder{sink};
            ASSERT_FALSE(decoder.onReceiveHeaderLine(boundaryHeader));
        }
    }
}

/*
 * Test: sends a header without boundary followed by a header with a bondary
 * Expected Result: pass
 */
TEST_F(MIMEParserTest, test_decodingBoundaryAfteraNonBoundaryHeader) {
    std::string testBoundary = "myboundary";
    const std::vector<std::string> headers = {"content-type:nana;myprop:abc\r\n",
                                              BOUNDARY_HEADER_PREFIX + testBoundary};
    generatePayloadAndTest(headers, testBoundary);
}

/*
 * Test: sends a header without boundary followed by a header with a bondary
 * Expected Result: pass
 */
TEST_F(MIMEParserTest, test_decodingValidBoundariesWithMoreHeaders) {
    std::vector<std::string> headerAfterBoundary = {"", ";otherprop=yes", " somethingelse"};
    std::string testBoundary = "myboundary";
    for (auto header : headerAfterBoundary) {
        const std::string boundaryHeader{testBoundary + header};
        generatePayloadAndTest(boundaryHeader, testBoundary);
    }
}

/*
 * Test: sends a boundary terminated by CRLF (real server case)
 * Expected Result: pass
 */
TEST_F(MIMEParserTest, test_decodingBoundaryTerminatedWithCRLF) {
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "application/json"}}};
    const std::vector<std::string> testCaseExpectedData = {"data"};
    const std::string boundaryHeader = "content-type: multipart/related; boundary=directives\r\n";
    std::string payload = "--directives\r\ncontent-type: application/json\r\n\r\ndata";
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    HTTP2MimeResponseDecoder decoder{sink};
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
    ASSERT_TRUE(resp);
    decoder.onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);
    // send the data part by part like the cloud does.
    status = decoder.onReceiveData(payload.c_str(), payload.size());
    ASSERT_EQ(sink->m_headers, testCaseExpectedHeaders);
    ASSERT_EQ(sink->m_data, testCaseExpectedData);
    ASSERT_EQ(status, HTTP2ReceiveDataStatus::SUCCESS);
}

/*
 * Test: simulates what the AVS does when sending multipart files the boundary is usually sent alone without a trailing
 * CRLF in a standalone message. Expected Result: it should be successful
 */
TEST_F(MIMEParserTest, test_decodingBoundariesAvs) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"1111", 0},
        {"2222", 0},
        {"3333", 0},
        {"\r\n--wooohooo", 1},  // the boundary is sent on its own and we want to detect a part end here.
        {"\r\ncontent-type: multipart/related\r\n\r\nlast\r\n--wooohooo", 2},
        {"--\r\n", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"111122223333", "last"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: sends the boundary (using the same length as the real boundary) as a data part with other data.
 * Expected Result: the fake boundary should not be interpreted as such, instead it should be part of the data.
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendFakeBoundaryAsData) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"1111", 0},
        {"aa--wooohooo", 0},  // try to set the parser off
        {"2222", 0},
        {"\r\n--wooohooo", 1},
        {"\r\ncontent-type: multipart/related\r\n\r\nlast\r\n--wooohooo", 2},
        {"--\r\n", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"1111aa--wooohooo2222", "last"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: sends the boundary (using the same length as the real boundary) as a data part with no other data.
 * Expected Result: the fake boundary should not be interpreted as such, instead it should be part of the data.
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendFakeBoundaryAsOnlyData) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"aa--wooohooo", 0},    // try to set the parser off
        {"\r\n--wooohooo", 1},  //<CRLF>--boundary
        {"\r\ncontent-type: multipart/related\r\n\r\nlast\r\n--wooohooo", 2},
        {"--\r\n", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"aa--wooohooo", "last"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: sends the boundary with a trailing/ending CRLF in the same chunk instead in the chunk of the header, this is
 * slightly different than what the AVS server does, it should still work as the chars are the same Expected Result: it
 * should work
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendBoundaryWithCRLF) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"1111", 0},
        {"2222", 0},
        {"3333", 0},
        {"\r\n--wooohooo\r\n", 1},  //<CRLF>--boundary<CRLF>
        {"content-type: multipart/related\r\n\r\nlast\r\n--wooohooo", 2},
        {"--\r\n", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"111122223333", "last"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: Send the last boundary without CRLF
 * Expected Result: It should work
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendEndBoundaryWithoutCRLF) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"1111", 0},
        {"2222", 0},
        {"3333", 0},
        {"\r\n--wooohooo", 1},
        {"\r\ncontent-type: multipart/related\r\n\r\nlast\r\n--wooohooo--", 2}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}, {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"111122223333", "last"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(parts, testCaseExpectedHeaders, testCaseExpectedData, headers);
}

/*
 * Test: Send the boundary without a trailing CR LF or --
 * Expected Result: It should raise an error (ABORT) because boundaries are required to have -- or CRLF after it
 */
TEST_F(MIMEParserTest, test_decodingBoundariesSendBoundaryWithData) {
    /* this a structure to workaround the lack of a real mock
        the indexes after each part denotes what is the expected m_index in our MockHTTP2MimeResponseDecoderSink.
        in the mock the m_index attribute is incremented every time we receive a new part end.
    */
    std::vector<std::pair<std::string, size_t>> parts = {
        {"--wooohooo\r\ncontent-type: multipart/related\r\n\r\n", 0},
        {"1111", 0},
        {"\r\n--wooohooo", 1},
        {"3333", 1},
        {"\r\n--wooohooo", 1},
        {"\r\ncontent-type: multipart/related\r\n\r\nlast\r\n--wooohooo--", 1}};
    const std::vector<std::multimap<std::string, std::string>> testCaseExpectedHeaders = {
        {{"content-type", "multipart/related"}}};
    const std::vector<std::string> testCaseExpectedData = {"1111"};
    const std::vector<std::string> headers = {BOUNDARY_HEADER_PREFIX + boundary};
    runDecodingBoundariesTest(
        parts, testCaseExpectedHeaders, testCaseExpectedData, headers, HTTP2ReceiveDataStatus::ABORT);
}

TEST_F(MIMEParserTest, test_decodingSanity) {
    /// We choose an arbitrary buffer size
    const int bufferSize{25};
    /// We need to pass a header with boundary info
    const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + boundary};
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    HTTP2MimeResponseDecoder decoder{sink};
    size_t index{0};
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
    ASSERT_TRUE(resp);
    decoder.onReceiveResponseCode(HTTPResponseCode::SUCCESS_OK);
    while (status == HTTP2ReceiveDataStatus::SUCCESS && index < encodedSize) {
        auto sizeToSend = (index + bufferSize) > encodedSize ? encodedSize - index : bufferSize;
        status = decoder.onReceiveData(encodedPayload.c_str() + index, sizeToSend);
        index += sizeToSend;
    }
    ASSERT_EQ(HTTP2ReceiveDataStatus::SUCCESS, status);
    /**
     * Test individual MIME parts and headers
     * note: header order may have changed
     */
    ASSERT_TRUE(3 == sink->m_data.size());
    ASSERT_TRUE(3 == sink->m_headers.size());
    ASSERT_TRUE(3 == sink->m_headers.at(0).size());
    ASSERT_EQ(sink->m_headers, expectedHeaders);
    ASSERT_EQ(sink->m_data, expectedData);
}

void runCodecTest(
    std::shared_ptr<MockHTTP2MimeRequestEncodeSource> source,
    std::shared_ptr<MockHTTP2MimeResponseDecodeSink> sink,
    const int bufferSize) {
    char buf[XLARGE];
    size_t pauseCount{0};

    // setup encoder
    std::string boundary = createRandomAlphabetString(10);
    HTTP2MimeRequestEncoder encoder{boundary, source};

    // setup decoder
    HTTP2MimeResponseDecoder decoder{sink};
    size_t index{0};
    HTTP2SendDataResult result{0};
    while (result.status == HTTP2SendStatus::CONTINUE || result.status == HTTP2SendStatus::PAUSE) {
        result = encoder.onSendData(buf + index, bufferSize);
        if (result.status == HTTP2SendStatus::PAUSE) {
            pauseCount++;
        } else {
            std::string chunk(buf, index, result.size);
            index += result.size;
        }
    }
    /**
     * Since encoding output gives partial chunks in case of PAUSE if some bytes have already been encoded,
     * the PAUSE count encountered may not match. However, since we add a PAUSE in the very beginning, that one
     * is guaranteed to be received
     */
    if (source->m_pauseCount > 0) {
        ASSERT_TRUE(pauseCount > 0);
    }
    auto finalSize = index;
    index = 0;
    pauseCount = 0;
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + boundary};
    bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
    ASSERT_TRUE(resp);
    decoder.onReceiveResponseCode(
        static_cast<std::underlying_type<HTTPResponseCode>::type>(HTTPResponseCode::SUCCESS_OK));
    while ((status == HTTP2ReceiveDataStatus::SUCCESS || status == HTTP2ReceiveDataStatus::PAUSE) &&
           index < finalSize) {
        auto sizeToSend = (index + bufferSize) > finalSize ? finalSize - index : bufferSize;
        status = decoder.onReceiveData(buf + index, sizeToSend);
        if (status == HTTP2ReceiveDataStatus::PAUSE) {
            pauseCount++;
        } else {
            index += sizeToSend;
        }
    }
    ASSERT_TRUE(sink->hasSameContentAs(source));
    ASSERT_EQ(pauseCount, sink->m_pauseCount);
}

TEST_F(MIMEParserTest, test_singlePayloadSinglePass) {
    // Sufficiently large buffer to accommodate payload
    const int bufferSize{LARGE};

    // setup source
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // small single payload
    data.push_back(createRandomAlphabetString(SMALL));
    std::vector<std::string> headers;
    headers.push_back(
        {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
    headerSets.push_back(headers);
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);

    // setup sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();

    // run actual test
    runCodecTest(source, sink, bufferSize);
}

TEST_F(MIMEParserTest, test_singlePayloadMultiplePasses) {
    // Medium sized payload and buffer
    const int bufferSize{SMALL};
    // setup source
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // single large payload that cannot fit into buffer ensuring >1 pass
    data.push_back(createRandomAlphabetString(LARGE));
    std::vector<std::string> headers;
    headers.push_back(
        {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
    headers.push_back(
        {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
    headerSets.push_back(headers);
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);

    // setup sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();

    // run actual test
    runCodecTest(source, sink, bufferSize);
}

TEST_F(MIMEParserTest, test_multiplePayloadsSinglePass) {
    const int bufferSize{LARGE};
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // 3 small payloads
    for (int i = 0; i < 3; ++i) {
        data.push_back(createRandomAlphabetString(SMALL));
        std::vector<std::string> headers;
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headerSets.push_back(headers);
    }
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);

    // setup sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();

    // run actual test
    runCodecTest(source, sink, bufferSize);
}

TEST_F(MIMEParserTest, test_multiplePayloadsMultiplePasses) {
    const int bufferSize{SMALL};
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // 3 medium payloads
    for (int i = 0; i < 3; ++i) {
        data.push_back(createRandomAlphabetString(MEDIUM));
        std::vector<std::string> headers;
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headerSets.push_back(headers);
    }
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);

    // setup sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();

    // run actual test
    runCodecTest(source, sink, bufferSize);
}

/**
 * Test feeding mime text including duplicate boundaries that we want to just skip over.
 */
TEST_F(MIMEParserTest, test_duplicateBoundaries) {
    /// We choose an arbitrary buffer size
    const int bufferSize{25};
    /// We need to pass a header with boundary info
    const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + MIME_TEST_BOUNDARY_STRING};
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    HTTP2MimeResponseDecoder decoder{sink};
    std::string testPayload = BOUNDARY_LINE;
    for (int i = 0; i < 2; ++i) {
        testPayload += MIME_NEWLINE + HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + createRandomAlphabetString(SMALL) +
                       BOUNDARY_LINE;
    }
    testPayload += MIME_NEWLINE + NORMAL_LINES;
    testPayload +=
        MIME_NEWLINE + HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + createRandomAlphabetString(SMALL) + BOUNDARY_LINE;
    testPayload += MIME_NEWLINE + DUPLICATE_BOUNDARY_LINES;
    testPayload +=
        MIME_NEWLINE + HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + createRandomAlphabetString(SMALL) + BOUNDARY_LINE;
    testPayload += MIME_NEWLINE + CRLF_DUPLICATE_BOUNDARY_LINES;
    testPayload +=
        MIME_NEWLINE + HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + createRandomAlphabetString(SMALL) + BOUNDARY_LINE;
    testPayload += MIME_NEWLINE + TRIPLICATE_BOUNDARY_LINES;
    testPayload +=
        MIME_NEWLINE + HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + createRandomAlphabetString(SMALL) + BOUNDARY_LINE;
    testPayload += MIME_NEWLINE + CRLF_TRIPLICATE_BOUNDARY_LINES;
    testPayload +=
        MIME_NEWLINE + HEADER_LINE + MIME_NEWLINE + MIME_NEWLINE + createRandomAlphabetString(SMALL) + BOUNDARY_LINE;
    testPayload += MIME_BOUNDARY_DASHES;

    size_t index{0};
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
    decoder.onReceiveResponseCode(
        static_cast<std::underlying_type<HTTPResponseCode>::type>(HTTPResponseCode::SUCCESS_OK));
    while (status == HTTP2ReceiveDataStatus::SUCCESS && index < testPayload.size()) {
        auto sizeToSend = (index + bufferSize) > testPayload.size() ? testPayload.size() - index : bufferSize;
        status = decoder.onReceiveData(testPayload.c_str() + index, sizeToSend);
        index += sizeToSend;
    }
    ASSERT_EQ(HTTP2ReceiveDataStatus::SUCCESS, status);
    ASSERT_TRUE(resp);
    /// verify only the 12 messages added above are written to sink(no empty payloads from newlines)
    ASSERT_TRUE(12 == sink->m_data.size());
    // verify TEST_MESSAGE was correctly decoded
    for (int j = 2; j < 12; j += 2) {
        ASSERT_EQ(TEST_MESSAGE, sink->m_data.at(j));
    }
    // negative test
    for (int j = 1; j < 12; j += 2) {
        ASSERT_NE(TEST_MESSAGE, sink->m_data.at(j));
    }
}

TEST_F(MIMEParserTest, test_aBORT) {
    // setup source
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // small single payload
    data.push_back(createRandomAlphabetString(SMALL));
    std::vector<std::string> headers;
    headers.push_back(
        {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
    headerSets.push_back(headers);
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);
    source->m_abort = true;

    // setup sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    sink->m_abort = true;

    // setup encoder
    std::string testBoundary = createRandomAlphabetString(10);
    HTTP2MimeRequestEncoder encoder{testBoundary, source};
    char buf[LARGE];
    // Ensure repeated calls return ABORT
    ASSERT_TRUE(encoder.onSendData(buf, SMALL).status == HTTP2SendStatus::ABORT);
    source->m_abort = false;
    ASSERT_TRUE(encoder.onSendData(buf, SMALL).status == HTTP2SendStatus::ABORT);

    // setup decoder
    HTTP2MimeResponseDecoder decoder{sink};
    decoder.onReceiveResponseCode(
        static_cast<std::underlying_type<HTTPResponseCode>::type>(HTTPResponseCode::SUCCESS_OK));
    // Ensure repeated calls return ABORT
    ASSERT_EQ(decoder.onReceiveData(encodedPayload.c_str(), SMALL), HTTP2ReceiveDataStatus::ABORT);
    sink->m_abort = false;
    ASSERT_EQ(decoder.onReceiveData(encodedPayload.c_str(), SMALL), HTTP2ReceiveDataStatus::ABORT);
}

TEST_F(MIMEParserTest, test_pAUSE) {
    const int bufferSize{SMALL};
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // 3 medium payloads
    for (int i = 0; i < 3; ++i) {
        data.push_back(createRandomAlphabetString(MEDIUM));
        std::vector<std::string> headers;
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headerSets.push_back(headers);
    }
    // setup slow source
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);
    source->m_slowSource = true;

    // setup slow sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    sink->m_slowSink = true;

    // run actual test
    runCodecTest(source, sink, bufferSize);
    ASSERT_TRUE(sink->m_pauseCount > 0);
    ASSERT_TRUE(source->m_pauseCount > 0);
}

/**
 * We test for cases when the amount of data to be encoded/decoded from chunk varies a lot
 * between calls
 */
TEST_F(MIMEParserTest, test_variableChunkSizes) {
    std::vector<std::string> data;
    std::vector<std::vector<std::string>> headerSets;
    // 3 medium payloads
    for (int i = 0; i < 3; ++i) {
        data.push_back(createRandomAlphabetString(MEDIUM));
        std::vector<std::string> headers;
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headers.push_back(
            {createRandomAlphabetString(HEADER_PART_SIZE) + SEPARATOR + createRandomAlphabetString(HEADER_PART_SIZE)});
        headerSets.push_back(headers);
    }
    // setup source
    auto source = std::make_shared<MockHTTP2MimeRequestEncodeSource>(data, headerSets);
    // setup sink
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    // sufficiently large buffer
    char buf[XLARGE];

    // setup encoder
    std::string testBoundary = createRandomAlphabetString(10);
    HTTP2MimeRequestEncoder encoder{testBoundary, source};

    // setup decoder
    HTTP2MimeResponseDecoder decoder{sink};

    size_t index{0};
    size_t pauseCount{0};

    HTTP2SendDataResult result{0};
    while (result.status == HTTP2SendStatus::CONTINUE || result.status == HTTP2SendStatus::PAUSE) {
        // We will randomize this size from SMALL/2 to SMALL
        int bufferSize{generateRandomNumber(SMALL / 2, SMALL)};
        result = encoder.onSendData(buf + index, bufferSize);
        if (result.status == HTTP2SendStatus::PAUSE) {
            pauseCount++;
        } else {
            std::string chunk(buf, index, result.size);
            index += result.size;
        }
    }
    /**
     * Since encoding output gives partial chunks in case of PAUSE if some bytes have already been encoded,
     * the PAUSE count encountered may not match. However, since we add a PAUSE in the very beginning, that one
     * is guaranteed to be received
     */
    if (source->m_pauseCount > 0) {
        ASSERT_TRUE(pauseCount > 0);
    }

    auto finalSize = index;
    index = 0;
    pauseCount = 0;
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    const std::string boundaryHeader{BOUNDARY_HEADER_PREFIX + testBoundary};
    bool resp = decoder.onReceiveHeaderLine(boundaryHeader);
    decoder.onReceiveResponseCode(
        static_cast<std::underlying_type<HTTPResponseCode>::type>(HTTPResponseCode::SUCCESS_OK));
    ASSERT_TRUE(resp);
    while ((status == HTTP2ReceiveDataStatus::SUCCESS || status == HTTP2ReceiveDataStatus::PAUSE) &&
           index < finalSize) {
        // We will randomize this size from SMALL/2 to SMALL
        int bufferSize{generateRandomNumber(SMALL / 2, SMALL)};
        auto sizeToSend = (index + bufferSize) > finalSize ? finalSize - index : bufferSize;
        status = decoder.onReceiveData(buf + index, sizeToSend);
        if (status == HTTP2ReceiveDataStatus::PAUSE) {
            pauseCount++;
        } else {
            index += sizeToSend;
        }
    }
    ASSERT_TRUE(sink->hasSameContentAs(source));
    ASSERT_EQ(pauseCount, sink->m_pauseCount);
}

/**
 * Test one of many prefix use cases.
 *
 * @param readablePrefix The prefix string to log if there is a failure (some prefixes are unreadable).
 * @param prefix The prefix to add to the body to decode.
 * @param firstChunkSize The size of the first chunk to pass to the decoder.
 * @param numberChunks The number of chunks in which to send the body.
 * @param expectSuccess Whether or not to expect the parse to succeed.
 */
static void testPrefixCase(
    const std::string& readablePrefix,
    const std::string& prefix,
    size_t firstChunkSize,
    int numberChunks,
    bool expectSuccess) {
    auto sink = std::make_shared<MockHTTP2MimeResponseDecodeSink>();
    HTTP2MimeResponseDecoder decoder{sink};

    ASSERT_TRUE(decoder.onReceiveHeaderLine(BOUNDARY_HEADER_PREFIX + MIME_TEST_BOUNDARY_STRING));
    ASSERT_TRUE(decoder.onReceiveResponseCode(
        static_cast<std::underlying_type<HTTPResponseCode>::type>(HTTPResponseCode::SUCCESS_OK)));

    std::string data = prefix + BOUNDARY + MIME_NEWLINE + NORMAL_LINES + MIME_BOUNDARY_DASHES;
    auto writeQuantum = data.length();
    HTTP2ReceiveDataStatus status{HTTP2ReceiveDataStatus::SUCCESS};
    size_t index{0};
    int chunksSent = 0;

    if (numberChunks != 1 && firstChunkSize != 0 && firstChunkSize < writeQuantum) {
        status = decoder.onReceiveData(data.c_str(), firstChunkSize);
        index = firstChunkSize;
        writeQuantum -= firstChunkSize;
        chunksSent++;
    }

    if (numberChunks > 1) {
        writeQuantum /= (numberChunks - chunksSent);
    }

    while (status == HTTP2ReceiveDataStatus::SUCCESS && index < data.length() && chunksSent < (numberChunks + 1)) {
        auto size = (index + writeQuantum) > data.length() ? data.length() - index : writeQuantum;
        status = decoder.onReceiveData(data.c_str() + index, size);
        index += size;
        chunksSent++;
    }

    std::string message = "prefix=" + readablePrefix + ", firstChunkSize=" + std::to_string(firstChunkSize) +
                          ", numberChunks=" + std::to_string(numberChunks);

    if (expectSuccess) {
        ASSERT_EQ(HTTP2ReceiveDataStatus::SUCCESS, status) << message;
        ASSERT_EQ(sink->m_data.size(), static_cast<size_t>(1)) << message;
        ASSERT_EQ(TEST_MESSAGE, sink->m_data[0]) << message;
    } else {
        ASSERT_NE(HTTP2ReceiveDataStatus::SUCCESS, status) << message;
    }
}

TEST_F(MIMEParserTest, test_prefixCases) {
    // Value used to drive testes of first chunk sizes 0 (i.e. none), 1, 2, and 3.
    static const int MAX_FIRST_CHUNK_SIZE = 3;

    // Value used to drive sending data in 1, 2, 3, 4 or 5 parts.
    static const int MAX_CHUNKS = 5;

    for (int firstChunkSize = 0; firstChunkSize <= MAX_FIRST_CHUNK_SIZE; firstChunkSize++) {
        for (int numberChunks = 1; numberChunks <= MAX_CHUNKS; numberChunks++) {
            testPrefixCase("empty", "", firstChunkSize, numberChunks, true);
            testPrefixCase("\\r\\n", MIME_NEWLINE, firstChunkSize, numberChunks, true);
            testPrefixCase("\\r", "\r", firstChunkSize, numberChunks, false);
            testPrefixCase("\\n", "\n", firstChunkSize, numberChunks, false);
            testPrefixCase("x", "x", firstChunkSize, numberChunks, false);
        }
    }
}

}  // namespace test
}  // namespace avsCommon
}  // namespace alexaClientSDK
