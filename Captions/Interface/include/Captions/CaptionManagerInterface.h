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

#ifndef ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONMANAGERINTERFACE_H_
#define ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONMANAGERINTERFACE_H_

#include <memory>

#include "CaptionData.h"
#include "CaptionFrame.h"
#include "CaptionPresenterInterface.h"

namespace alexaClientSDK {
namespace captions {

/**
 * An interface to allow handling of @c CaptionData objects and route them to a @c CaptionPresenterInterface instance.
 * Implementations of this interface must be capable of receiving captions from multiple media sources in parallel.
 */
class CaptionManagerInterface {
public:
    /**
     * Destructor.
     */
    virtual ~CaptionManagerInterface() = default;

    /**
     * Starts processing the provided @c CaptionData with the available parser. If no parser is present, then a warning
     * will be logged to the console, and the @c CaptionData object will be ignored.
     *
     * @param sourceId The ID of the media source for this caption.
     * @param captionData The object containing the raw caption content and metadata.
     */
    virtual void onCaption(CaptionFrame::MediaPlayerSourceId sourceId, const CaptionData& captionData) = 0;

    /**
     * Sets the @c CaptionPresenterInterface instance responsible for measuring styled caption text and displaying or
     * hiding the captions. If called multiple times, the last @c CaptionPresenterInterface set will be the active
     * presenter.
     *
     * @param presenter The @c CaptionPresenterInterface instance to use for caption text measurement and presentation.
     */
    virtual void setCaptionPresenter(const std::shared_ptr<CaptionPresenterInterface>& presenter) = 0;

    /**
     * Sets the @c MediaPlayerInterface instances responsible for producing caption content. If called multiple times,
     * the last vector of @c MediaPlayerInterface set will be the active media players.
     *
     * @param mediaPlayers The media players which should be observed for media state changes.
     */
    virtual void setMediaPlayers(
        const std::vector<std::shared_ptr<avsCommon::utils::mediaPlayer::MediaPlayerInterface>>& mediaPlayers) = 0;
};

}  // namespace captions
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPTIONS_INTERFACE_INCLUDE_CAPTIONS_CAPTIONMANAGERINTERFACE_H_
