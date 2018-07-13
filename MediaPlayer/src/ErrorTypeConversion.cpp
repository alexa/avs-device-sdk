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

#include "MediaPlayer/ErrorTypeConversion.h"

namespace alexaClientSDK {
namespace mediaPlayer {

static avsCommon::utils::mediaPlayer::ErrorType gstCoreErrorToErrorType(const GstCoreError& error) {
    // all GST_CORE_ERRORs are MEDIA_ERROR_INTERNAL_DEVICE_ERRORs.
    return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR;
}

static avsCommon::utils::mediaPlayer::ErrorType gstLibraryErrorToErrorType(const GstLibraryError& error) {
    // all GST_LIBRARY_ERRORs are MEDIA_ERROR_INTERNAL_DEVICE_ERRORs.
    return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR;
}

static avsCommon::utils::mediaPlayer::ErrorType gstResourceErrorToErrorType(
    const GstResourceError& error,
    bool remoteResource) {
    if (remoteResource) {
        switch (error) {
            case GST_RESOURCE_ERROR_NOT_FOUND:
            case GST_RESOURCE_ERROR_OPEN_READ:
            case GST_RESOURCE_ERROR_OPEN_WRITE:
            case GST_RESOURCE_ERROR_OPEN_READ_WRITE:
            case GST_RESOURCE_ERROR_CLOSE:
            case GST_RESOURCE_ERROR_READ:
            case GST_RESOURCE_ERROR_WRITE:
            case GST_RESOURCE_ERROR_SEEK:
            case GST_RESOURCE_ERROR_SYNC:
            case GST_RESOURCE_ERROR_SETTINGS:
            case GST_RESOURCE_ERROR_NOT_AUTHORIZED:
                return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INVALID_REQUEST;
            case GST_RESOURCE_ERROR_BUSY:
                return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_SERVICE_UNAVAILABLE;
            default:
                return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR;
        }
    }
    return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR;
}

static avsCommon::utils::mediaPlayer::ErrorType gstStreamErrorToErrorType(const GstStreamError& error) {
    // all GST_STREAM_ERRORs are MEDIA_ERROR_INTERNAL_DEVICE_ERRORs.
    return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_INTERNAL_DEVICE_ERROR;
}

avsCommon::utils::mediaPlayer::ErrorType gerrorToErrorType(const GError* error, bool remoteResource) {
    const auto& domain = error->domain;
    if (domain == GST_CORE_ERROR) {
        return gstCoreErrorToErrorType(static_cast<GstCoreError>(error->code));
    } else if (domain == GST_LIBRARY_ERROR) {
        return gstLibraryErrorToErrorType(static_cast<GstLibraryError>(error->code));
    } else if (domain == GST_RESOURCE_ERROR) {
        return gstResourceErrorToErrorType(static_cast<GstResourceError>(error->code), remoteResource);
    } else if (domain == GST_STREAM_ERROR) {
        return gstStreamErrorToErrorType(static_cast<GstStreamError>(error->code));
    } else {
        return avsCommon::utils::mediaPlayer::ErrorType::MEDIA_ERROR_UNKNOWN;
    }
}

}  // namespace mediaPlayer
}  // namespace alexaClientSDK
