/*
 * WebApplication.h
 *
 * Copyright (c) 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#ifndef ALEXA_CLIENT_SDK_WEBEXTENSION_INCLUDE_WEBEXTENSION_WEBAPPLICATION_H_
#define ALEXA_CLIENT_SDK_WEBEXTENSION_INCLUDE_WEBEXTENSION_WEBAPPLICATION_H_

#include "ConsolePrinter.h"
#include "UserInputManager.h"

#ifdef KWD
#include <KWD/AbstractKeywordDetector.h>
#endif
#include <MediaPlayer/MediaPlayer.h>

#include <memory>

namespace alexaClientSDK {
namespace webExtension {

/// Class to manage the top-level components of the AVS Client Application
class WebApplication {
public:
    /**
     * Create a WebApplication.
     *
     * @param pathToConfig The path to the SDK configuration file.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return A new @c WebApplication, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<WebApplication> create(
        const std::string& pathToConfig,
        const std::string& pathToInputFolder,
        const std::string& logLevel = "");

    /// Runs the application, blocking until the user asks the application to quit.
    void run();

    /// Destructor which manages the @c WebApplication shutdown sequence.
    ~WebApplication();

private:
    /**
     * Initialize a WebApplication.
     *
     * @param pathToConfig The path to the SDK configuration file.
     * @param pathToInputFolder The path to the inputs folder containing data files needed by this application.
     * @param logLevel The level of logging to enable.  If this parameter is an empty string, the SDK's default
     *     logging level will be used.
     * @return @c true if initialization succeeded, else @c false.
     */
    bool initialize(const std::string& pathToConfig, const std::string& pathToInputFolder, const std::string& logLevel);

    /// The @c UserInputManager which controls the client.
    std::unique_ptr<UserInputManager> m_userInputManager;

    /// The @c MediaPlayer used by @c SpeechSynthesizer.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_speakMediaPlayer;

    /// The @c MediaPlayer used by @c AudioPlayer.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_audioMediaPlayer;

    /// The @c MediaPlayer used by @c Alerts.
    std::shared_ptr<mediaPlayer::MediaPlayer> m_alertsMediaPlayer;

#ifdef KWD
    /// The Wakeword Detector which can wake up the client using audio input.
    std::unique_ptr<kwd::AbstractKeywordDetector> m_keywordDetector;
#endif
};

}  // namespace webExtension
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_WEBEXTENSION_INCLUDE_WEBEXTENSION_WEBAPPLICATION_H_
