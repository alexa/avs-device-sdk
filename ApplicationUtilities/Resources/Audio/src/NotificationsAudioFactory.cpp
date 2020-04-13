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

#include "Audio/NotificationsAudioFactory.h"

#include <AVSCommon/Utils/Stream/StreamFunctions.h>

#include "Audio/Data/med_alerts_notification_01.mp3.h"

namespace alexaClientSDK {
namespace applicationUtilities {
namespace resources {
namespace audio {

static std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType> notificationDefaultFactory() {
    return std::make_pair(
        avsCommon::utils::stream::streamFromData(
            data::med_alerts_notification_01_mp3, sizeof(data::med_alerts_notification_01_mp3)),
        avsCommon::utils::MimeTypeToMediaType(data::med_alerts_notification_01_mp3_mimetype));
}

std::function<std::pair<std::unique_ptr<std::istream>, const avsCommon::utils::MediaType>()> NotificationsAudioFactory::
    notificationDefault() const {
    return notificationDefaultFactory;
}

}  // namespace audio
}  // namespace resources
}  // namespace applicationUtilities
}  // namespace alexaClientSDK
