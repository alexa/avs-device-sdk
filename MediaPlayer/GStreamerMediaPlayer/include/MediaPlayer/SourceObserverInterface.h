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

#ifndef ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_SOURCEOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_SOURCEOBSERVERINTERFACE_H_

namespace alexaClientSDK {
namespace mediaPlayer {

/**
 * An interface which allows notification of events which occur with respect to an audio data source.
 */
class SourceObserverInterface {
public:
    virtual ~SourceObserverInterface() = default;

    /**
     * This indicates that the first bytes of data have been read from the source.
     */
    virtual void onFirstByteRead() = 0;
};

}  // namespace mediaPlayer
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_MEDIAPLAYER_GSTREAMERMEDIAPLAYER_INCLUDE_MEDIAPLAYER_SOURCEOBSERVERINTERFACE_H_
