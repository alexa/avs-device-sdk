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

#include <unordered_map>

#include <gst/gst.h>
#include <gtest/gtest.h>

#include "MediaPlayer/ErrorTypeConversion.h"

namespace alexaClientSDK {
namespace mediaPlayer {
namespace test {

using namespace avsCommon::utils::mediaPlayer;

/**
 * Test to verify that the ErrorTypes convert to the error strings properly.
 */
TEST(ErrorTypeConversionTest, ErrorTypeToString) {
    const std::map<ErrorType, std::string> m = {
        {ErrorType::MEDIA_ERROR_UNKNOWN, "MEDIA_ERROR_UNKNOWN"},
        {ErrorType::MEDIA_ERROR_INVALID_REQUEST, "MEDIA_ERROR_INVALID_REQUEST"},
        {ErrorType::MEDIA_ERROR_SERVICE_UNAVAILABLE, "MEDIA_ERROR_SERVICE_UNAVAILABLE"},
        {ErrorType::MEDIA_ERROR_INTERNAL_SERVER_ERROR, "MEDIA_ERROR_INTERNAL_SERVER_ERROR"},
        {ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR, "MEDIA_ERROR_INTERNAL_DEVICE_ERROR"}};

    for (const auto& i : m) {
        ASSERT_EQ(i.second, errorTypeToString(i.first));
    }
}

/**
 * Test to verify that the correct GErrors convert to the proper ErrorType.
 */
TEST(ErrorTypeConversionTest, GstCoreErrorToErrorType) {
    const std::map<std::pair<gint, bool>, ErrorType> goldStandardMapping = {
        {{GST_CORE_ERROR_FAILED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_FAILED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_TOO_LAZY, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_TOO_LAZY, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_NOT_IMPLEMENTED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_NOT_IMPLEMENTED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_STATE_CHANGE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_STATE_CHANGE, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_PAD, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_PAD, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_THREAD, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_THREAD, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_NEGOTIATION, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_NEGOTIATION, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_EVENT, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_EVENT, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_SEEK, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_SEEK, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_CAPS, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_CAPS, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_TAG, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_TAG, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_MISSING_PLUGIN, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_MISSING_PLUGIN, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_CLOCK, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_CLOCK, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_DISABLED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_CORE_ERROR_DISABLED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR}};

    for (gint gstCoreErrorNumber = GST_CORE_ERROR_FAILED; gstCoreErrorNumber < GST_CORE_ERROR_NUM_ERRORS;
         ++gstCoreErrorNumber) {
        for (bool remoteResource : {false, true}) {
            const auto check_in_map = goldStandardMapping.find(std::make_pair(gstCoreErrorNumber, remoteResource));

            ASSERT_NE(goldStandardMapping.end(), check_in_map);
            const GError gerror{GST_CORE_ERROR, static_cast<GstCoreError>(gstCoreErrorNumber)};
            ASSERT_EQ(check_in_map->second, gerrorToErrorType(&gerror, remoteResource));
        }
    }
}

/**
 * Test to verify that the correct Library GErrors convert to the proper ErrorType.
 */
TEST(ErrorTypeConversionTest, GstLibraryErrorToErrorType) {
    const std::map<std::pair<gint, bool>, ErrorType> goldStandardMapping = {
        {{GST_LIBRARY_ERROR_FAILED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_FAILED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_TOO_LAZY, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_TOO_LAZY, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_INIT, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_INIT, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_SHUTDOWN, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_SHUTDOWN, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_SETTINGS, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_SETTINGS, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_ENCODE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_ENCODE, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_NUM_ERRORS, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_LIBRARY_ERROR_NUM_ERRORS, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR}};

    for (gint gstLibraryErrorNumber = GST_LIBRARY_ERROR_FAILED; gstLibraryErrorNumber < GST_LIBRARY_ERROR_NUM_ERRORS;
         ++gstLibraryErrorNumber) {
        for (bool remoteResource : {false, true}) {
            const auto check_in_map = goldStandardMapping.find(std::make_pair(gstLibraryErrorNumber, remoteResource));

            ASSERT_NE(goldStandardMapping.end(), check_in_map);
            const GError gerror{GST_LIBRARY_ERROR, static_cast<GstLibraryError>(gstLibraryErrorNumber)};
            ASSERT_EQ(check_in_map->second, gerrorToErrorType(&gerror, remoteResource));
        }
    }
}

/**
 * Test to verify that the correct Resource GErrors convert to the proper ErrorType.
 */
TEST(ErrorTypeConversionTest, GstResourceErrorToErrorType) {
    const std::map<std::pair<gint, bool>, ErrorType> goldStandardMapping = {
        {{GST_RESOURCE_ERROR_FAILED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_FAILED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_TOO_LAZY, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_TOO_LAZY, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_NOT_FOUND, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_NOT_FOUND, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_BUSY, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_BUSY, true}, ErrorType::MEDIA_ERROR_SERVICE_UNAVAILABLE},
        {{GST_RESOURCE_ERROR_OPEN_READ, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_OPEN_READ, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_OPEN_WRITE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_OPEN_WRITE, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_OPEN_READ_WRITE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_OPEN_READ_WRITE, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_CLOSE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_CLOSE, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_READ, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_READ, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_WRITE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_WRITE, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_SEEK, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_SEEK, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_SYNC, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_SYNC, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_SETTINGS, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_SETTINGS, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_NO_SPACE_LEFT, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_NO_SPACE_LEFT, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_NOT_AUTHORIZED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_NOT_AUTHORIZED, true}, ErrorType::MEDIA_ERROR_INVALID_REQUEST},
        {{GST_RESOURCE_ERROR_NUM_ERRORS, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_RESOURCE_ERROR_NUM_ERRORS, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR}};

    for (gint gstResourceErrorNumber = GST_RESOURCE_ERROR_FAILED;
         gstResourceErrorNumber < GST_RESOURCE_ERROR_NUM_ERRORS;
         ++gstResourceErrorNumber) {
        for (bool remoteResource : {false, true}) {
            const auto check_in_map = goldStandardMapping.find(std::make_pair(gstResourceErrorNumber, remoteResource));

            ASSERT_NE(goldStandardMapping.end(), check_in_map);
            const GError gerror{GST_RESOURCE_ERROR, static_cast<GstResourceError>(gstResourceErrorNumber)};
            ASSERT_EQ(check_in_map->second, gerrorToErrorType(&gerror, remoteResource));
        }
    }
}

/**
 * Test to verify that the correct Stream GErrors convert to the proper ErrorType.
 */
TEST(ErrorTypeConversionTest, GstStreamErrorToErrorType) {
    const std::map<std::pair<gint, bool>, ErrorType> goldStandardMapping = {
        {{GST_STREAM_ERROR_FAILED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_FAILED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_TOO_LAZY, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_TOO_LAZY, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_NOT_IMPLEMENTED, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_NOT_IMPLEMENTED, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_TYPE_NOT_FOUND, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_TYPE_NOT_FOUND, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_WRONG_TYPE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_WRONG_TYPE, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_CODEC_NOT_FOUND, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_CODEC_NOT_FOUND, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DECODE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DECODE, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_ENCODE, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_ENCODE, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DEMUX, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DEMUX, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_MUX, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_MUX, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_FORMAT, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_FORMAT, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DECRYPT, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DECRYPT, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DECRYPT_NOKEY, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_DECRYPT_NOKEY, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_NUM_ERRORS, false}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR},
        {{GST_STREAM_ERROR_NUM_ERRORS, true}, ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR}};

    for (gint gstStreamErrorNumber = GST_STREAM_ERROR_FAILED; gstStreamErrorNumber < GST_STREAM_ERROR_NUM_ERRORS;
         ++gstStreamErrorNumber) {
        for (bool remoteResource : {false, true}) {
            const auto check_in_map = goldStandardMapping.find(std::make_pair(gstStreamErrorNumber, remoteResource));

            ASSERT_NE(goldStandardMapping.end(), check_in_map);
            const GError gerror{GST_STREAM_ERROR, static_cast<GstStreamError>(gstStreamErrorNumber)};
            ASSERT_EQ(check_in_map->second, gerrorToErrorType(&gerror, remoteResource));
        }
    }
}

}  // namespace test
}  // namespace mediaPlayer
}  // namespace alexaClientSDK
