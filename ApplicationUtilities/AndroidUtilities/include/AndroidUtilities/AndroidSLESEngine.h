/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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
#ifndef ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESENGINE_H_
#define ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESENGINE_H_

#include <SLES/OpenSLES.h>
#include <memory>

#include <AVSCommon/AVS/AudioInputStream.h>
#include <AndroidUtilities/AndroidSLESObject.h>

namespace alexaClientSDK {
namespace applicationUtilities {
namespace androidUtilities {

class AndroidSLESMicrophone;

/**
 * Class that represents the OpenSL ES engine object + interfaces.
 *
 * The engine is used to create other OpenSL ES objects, and each application should only have one engine. Furthermore,
 * the engine should be the first OpenSL ES object to be created and the last to be destroyed.
 *
 * The @c create method ensures that the application has only one engine.
 */
class AndroidSLESEngine : public std::enable_shared_from_this<AndroidSLESEngine> {
public:
    /**
     * Creates an @c AndroidSLESEngine. This method will only succeed if there is no other engine alive.
     *
     * @return A shared pointer to an @c AndroidSLESEngine object if succeeds; otherwise return @c nullptr.
     */
    static std::shared_ptr<AndroidSLESEngine> create();

    /**
     * Creates an OpenSL ES audio recorder.
     *
     * @param stream The new microphone will write the audio recorded to this stream.
     * @return A unique pointer to an @c AndroidSLESRecorder object if succeeds; otherwise return @c nullptr.
     */
    std::unique_ptr<AndroidSLESMicrophone> createMicrophoneRecorder(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream);

    /**
     * Creates an OpenSL ES audio player. Although the parameters are read-only, we cannot use const because android's
     * API expects non-constant parameters.
     *
     * @param[in] source The audio data source specification (Read-Only).
     * @param[in] sink The audio data sink specification (Read-Only).
     * @param[in] requireEqualizer set to true if equalizer support is required.
     * @return A unique pointer to an @c AndroidSLESObject object if succeeds; otherwise return @c nullptr.
     */
    std::unique_ptr<AndroidSLESObject> createPlayer(SLDataSource& source, SLDataSink& sink, bool requireEqualizer)
        const;

    /**
     * Creates an OpenSL ES output mix.
     *
     * @return A unique pointer to an @c AndroidSLESObject object if succeeds; otherwise return @c nullptr.
     */
    std::unique_ptr<AndroidSLESObject> createOutputMix() const;

    /**
     * Destructor.
     */
    ~AndroidSLESEngine();

private:
    /**
     * AndroidSLESEngine constructor.
     *
     * @param object Pointer to an @c AndroidSLESObject abstraction.
     * @param engine The engine interface used to access the @c object.
     */
    AndroidSLESEngine(std::unique_ptr<AndroidSLESObject> object, SLEngineItf engine);

    /// Internal AndroidSLES engine object which implements the engine.
    std::unique_ptr<AndroidSLESObject> m_object;

    /// Internal AndroidSLES engine interface used to access the OpenSL ES object.
    SLEngineItf m_engine;

    /// Atomic flag used to ensure there is only one @c AndroidSLESEngine.
    static std::atomic_flag m_created;
};

}  // namespace androidUtilities
}  // namespace applicationUtilities
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_APPLICATIONUTILITIES_ANDROIDUTILITIES_INCLUDE_ANDROIDUTILITIES_ANDROIDSLESENGINE_H_
