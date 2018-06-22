## ChangeLog

### [1.6.0] - 2018-03-08

**Enhancements**
* `rapidJson` is now included with "make install".
* Updated the `TemplateRuntimeObserverInterface` to support clearing of `displayCards`.
* Added Windows SDK support, along with an installation script (MinGW-w64).
* Updated `ContextManager` to ignore context reported by a state provider.
* The `SharedDataStream` object is now associated by playlist, rather than by URL.
* Added the `RegistrationManager` component. Now, when a user logs out all persistent user-specific data is cleared from the SDK. The log out functionality can be exercised in the sample app with the new command: `k`.

**Bug Fixes**
* [Issue 400](https://github.com/alexa/avs-device-sdk/issues/400) Fixed a bug where the alert reminder did not iterate as intended after loss of network connection.
* [Issue 477](https://github.com/alexa/avs-device-sdk/issues/477) Fixed a bug in which Alexa's weather response was being truncated.
* Fixed an issue in which there were reports of instability related to the Sensory engine. To correct this, the `portAudio` [`suggestedLatency`](https://github.com/alexa/avs-device-sdk/blob/master/Integration/AlexaClientSDKConfig.json#L62) value can now be configured.

**Known Issues**
* The `ACL` may encounter issues if audio attachments are received but not consumed.
* `SpeechSynthesizerState` currently uses `GAINING_FOCUS` and `LOSING_FOCUS` as a workaround for handling intermediate state. These states may be removed in a future release.
* Music playback doesn't immediately stop when a user barges-in on iHeartRadio.
* The Windows sample app hangs on exit.
* GDB receives a `SIGTRAP` when troubleshooting the Windows sample app.
* `make integration` doesn't work on Windows. Integration tests will need to be run individually.

### [1.5.0] - 2018-02-12

**Enhancements**
* Added the `ExternalMediaPlayer` Capability Agent. This allows playback from music providers that control their own playback queue. Example: Spotify.
* Added support for AU and NZ to the `SampleApp`.
* Firmware version can now be sent to Alexa via the `SoftwareInfo` event. The firmware version is specified in the config file under the `sampleApp` object as an integer value named [`firmwareVersion`](https://github.com/alexa/avs-device-sdk/blob/master/Integration/AlexaClientSDKConfig.json#L52).
* The new `f` command was added to the `SampleApp` which allows the firmware version to be updated at run-time.
* Optional configuration changes have been introduced. Now a [default log level](https://github.com/alexa/avs-device-sdk/blob/master/Integration/AlexaClientSDKConfig.json#L93) can be set for `ACSDK_LOG_MODULE` components, globally or individually. This value is specified under a new root level configuration object called `logger`, and the value itself is named `logLevel`. This allows you to limit the degree of logging to that default value, such as `ERROR`or `INFO`.

**Bug Fixes**
* Fixed bug where `AudioPlayer` progress reports were not being sent, or were being sent incorrectly.
* [Issue 408](https://github.com/alexa/avs-device-sdk/issues/408) - Irrelevant code related to `UrlSource` was removed from the `GStreamer-based MediaPlayer` implementation.
* The `TZ` variable no longer needs to be set to `UTC` when building the `SampleApp`.
* Fixed a bug where `CurlEasyHandleWrapper` logged unwanted data on failure conditions.
* Fixed a bug to improve `SIGPIPE` handling.
* Fixed a bug where the filename and classname were mismatched. Changed `UrlToAttachmentConverter.h` to `UrlContentToAttachmentConverter.h`,and `UrlToAttachmentConverter.cpp` to `UrlContentToAttachmentConverter.cpp`.  
* Fixed a bug where after muting and then un-muting the GStreamer-based `MediaPlayer` implementation, the next item in queue would play instead of continuing playback of the originally muted item.  

**Known Issues**
* The `ACL` may encounter issues if audio attachments are received but not consumed.
* Display Cards for Kindle don't render.  
* `SpeechSynthesizerState` currently uses `GAINING_FOCUS` and `LOSING_FOCUS` as a workaround for handling intermediate state. These states may be removed in a future release.
* Music playback doesn't immediately stop when a user barges-in on iHeartRadio.

### [1.4.0] - 2018-01-12

**Enhancements**
* Added the Notifications Capability Agent. This allows a client to receive notification indicators from Alexa.
* Added support for the `SoftwareInfo` event. This code is triggered in the `SampleApp` by providing a positive decimal integer as the "firmwareVersion" value in "sampleApp" object of the `AlexaClientSDKConfig.json`. The reported firmware version can be updated after starting the `SampleApp` by calling `SoftwareInfoSender::setFirmwareVersion()`. This code path can be exercised in the `SampleApp` with the new command: `f`.
* Added unit tests for Alerts.
* The GStreamer-based pipeline allows for the configuration of `MediaPlayer` output based on information provided in `Config`.
* Playlist streaming now uses a `BLOCKING` writer, which improves streaming efficiency.

**Bug Fixes**
* Fixed bug where `SpeechSynthesizer` would not stop playback when a state change timeout was encountered.
* Fixed the `SampleApplication` destructor to avoid segfaults if the object is not constructed correctly.
* Fixed bug where `AudioPlayer` would erroneously call `executeStop()` in `cancelDirective()`.
* [Issue 396](https://github.com/alexa/avs-device-sdk/issues/396) - Fixed bug for compilation error with GCC7 in `AVSCommon/SDKInterfaces/include/AVSCommon/SDKInterfaces/Audio/AlertsAudioFactoryInterface.h`
* [Issue 384](https://github.com/alexa/avs-device-sdk/issues/384) - Fixed bug that caused `AuthServer.py` to crash.
* Fixed bug where a long delay was encountered after pausing and resuming a large Audible chapter.
* Fixed bug that caused named timers and reminders to loop for an additional `loopCount` .
* Fixed memory corruption bug in `MessageInterpreter`.
* Fixed illegal memory accesses in `MediaPlayer` logging.

**Known Issues**
* The `ACL` may encounter issues if audio attachments are received but not consumed.
* Display Cards for Kindle don't render.
* If using the GStreamer-based `MediaPlayer` implementation, after muting and un-muting an audio item, the next item in the queue will begin playing rather than continuing playback of the originally muted audio item.
* `SpeechSynthesizerState` currently uses `GAINING_FOCUS` and `LOSING_FOCUS` as a workaround for handling intermediate state. These states may be removed in a future release.
* Music playback doesn't immediately stop when a user barges-in on iHeartRadio.

### [1.3.0] - 2017-12-08

* **Enhancements**
  * ContextManager now passes the namespace/name of the desired state to StateProviderInterface::provideState().  This is helpful when a single StateProvider object provides multiple states, and needs to know which one ContextManager is asking for.
  * The mime parser was hardened against duplicate boundaries.
  * Added functionality to add and remove AudioPlayer observers to the DefaultClient.
  * Unit tests for Alerts were added.
  * Added en-IN, en-CA and ja-JP to the SampleApp locale selection menu.
  * Added default alert and timer audio data to the SDK SampleApp.  There is no longer a requirement to download these audio files and configure them in the json configuration file.
  * Added support in SDS Reader and AttachmentReader for seeking into the future.  This allows a reader to move to an index which has not yet arrived in the SDS and poll/block until it arrives.
  * Added support for blocking Writers in the SharedDataStream class.
  * Changed the default status code sent by MessageRequestObserverInterface::onSendCompleted() to SERVER_OTHER_ERROR, and mapped HTTP code 500 to SERVER_INTERNAL_ERROR_V2.
  * Added support for parsing stream duration out of playlists.
  * Added a configuration option ("sampleApp":"displayCardsSupported") that allows the displaying of display cards to be enabled or disabled.
  * Named Timers and Reminders have been updated to fall back to the on-device background audio sound when cloud urls cannot be accessed or rendered.

* **Bug Fixes**
  * Removed floating point dependencies from core SDK libraries.
  * Fixed bug in SpeechSynthesizer where it's erroneously calling stop more than once.
  * Fixed an issue in ContentFetcher where it could hang during destruction until an active GET request completed.
  * Fixed a couple of parsing bugs in LibCurlHttpContentFetcher related to case-sensitivity and mime-type handling.
  * Fixed a bug where MediaPlayerObserverInterface::onPlaybackResumed() wasn't being called after resuming from a pause with a pending play/resume.
  * Fixed a bug in LibCurlContentFetcher where it could error out if data is written to the SDS faster than it is consumed.
  * The GStreamer-based MediaPlayer reference implementation now uses the ACL HTTP configured client.
    * An API change has been made to MediaPlayerInterface::setSource(). This method now takes in an optional offset as well to allow for immediately streaming to the offset if possible.
    * Next and Previous buttons now work with Audible.
    * Pandora resume stuttering is addressed.
    * Pausing and resuming Amazon music no longer seeks back to the beginning of the song.
  * libcurl CURLOPT_NOSIGNAL option is set to 1 (https://curl.haxx.se/libcurl/c/CURLOPT_NOSIGNAL.html) to avoid crashes  observed in SampleApp.
  * Fixed the timing of the PlaybackReportIntervalEvent and PlaybackReportDelayEvent as specified in the directives.
  * Fixed potential deadlocks in MediaPlayer during shutdown related to queued callbacks.
  * Fixed a crash in MediaPlayer that could occur if the network is disconnected during playback.
  * Fixed a bug where music could keep playing while Alexa is speaking.
  * Fixed a bug which was causing problems with pause/resume and next/previous with Amazon Music.
  * Fixed a bug where music could briefly start playing between speaks.
  * Fixed a bug where HLS playlists would stop streaming after the initial playlist had been played to completion.
  * Fixed a bug where Audible playback could not advance to the next chapter.
  * Fixed some occurrences of SDK entering the IDLE state during the transition between Listening and Speaking states.
  * Fixed a bug where PlaybackFinished events were not reporting the correct offset.
    * An API change has been made to MediaPlayerInterface::getOffset().  This method is now required to return the final offset when called after playback has stopped.
  * Fixed a problem where AIP was erroneously resetting its state upon getting a cancelDirective() callback.

* **Known Issues**
  * Capability agent for Notifications is not included in this release.
  * `ACL`'s asynchronous receipt of audio attachments may manage resources poorly in scenarios where attachments are received but not consumed.
  * GUI cards don't show for Kindle.
  * The new SpeechSynthesizerState state values GAINING_FOCUS and LOSING_FOCUS were added as part of a work-around.  The will likely be removed in subsequent releases.
  * With the gstreamer-based MediaPlayer, after muting and unmuting, the next item starts playing rather than continuing with the current item.

### [1.2.1] - 2017-11-16

* **Enhancements**
  * Added comments to `AlexaClientSDKConfig.json`. These descriptions provide additional guidance for what is expected for each field.
  * Enabled pause and resume controls for Pandora.

* **Bug Fixes**
  * Bug fix for [issue #329](https://github.com/alexa/avs-device-sdk/issues/329) - `HTTP2Transport` instances no longer leak when `SERVER_SIDE_DISCONNECT` is encountered.
  * Bug fix for [issue #189](https://github.com/alexa/avs-device-sdk/issues/189) - Fixed a race condition in the `Timer` class that sometimes caused `SpeechSynthesizer` to get stuck in the "Speaking" state.
  * Bug fix for a race condition that caused `SpeechSynthesizer` to ignore subsequent `Speak` directives.
  * Bug fix for corrupted mime attachments.

### [1.2.0] - 2017-10-27
* **Enhancements**
  * Updated MediaPlayer to solve stability issues
  * All capability agents were refined to work with the updated MediaPlayer
  * Added the TemplateRuntime capability agent
  * Added the SpeakerManager capability agent
  * Added a configuration option ("sampleApp":"endpoint") that allows the endpoint that SampleApp connects to to be specified without changing code or rebuilding
  * Added very verbose capture of libcurl debug information
  * Added an observer interface to observer audio state changes from AudioPlayer
  * Added support for StreamMetadataExtracted Event. Stream tags found in the stream are represented in JSON and sent to AVS
  * Added to the SampleApp a simple GuiRenderer as an observer to the TemplateRuntime Capability Agent
  * Moved shared libcurl functionality to AVSCommon/Utils
  * Added a CMake option to exclude tests from the "make all" build. Use "cmake <absolute-path-to-source>
  -DACSDK_EXCLUDE_TEST_FROM_ALL=ON" to enable it. When this option is enabled "make unit" and "make integration" still could be used to build and run the tests

* **Bug fixes**:
  * Previously scheduled alerts now play following a restart
  * General stability fixes
  * Bug fix for CertifiedSender blocking forever if the network goes down while it's trying to send a message to AVS
  * Fixes for known issue of Alerts integration tests fail: AlertsTest.UserLongUnrelatedBargeInOnActiveTimer and AlertsTest.handleOneTimerWithVocalStop
  * Attempting to end a tap-to-talk interaction with the tap-to-talk button wouldn't work
  * SharedDataStream could encounter a race condition due to a combination of a blocking Reader and a Writer closing before writing any data
  * Bug-fix for the ordering of notifications within alerts scheduling. This fixes the issue where a local-stop on an alert would also stop a subsequent alert if it were to begin without delay

* **Known Issues**

* Capability agent for Notifications is not included in this release
* Inconsistent playback behavior when resuming content ("Alexa, pause." / "Alexa, resume."). Specifically, handling playback offsets, which causes the content to play from the start. This behavior is also occasionally seen with "Next" /
"Previous".
* `ACL`'s asynchronous receipt of audio attachments may manage resources poorly in scenarios where attachments are received but not consumed.
* Music playback failures may result in an error Text to Speech being rendered repeatedly
* Reminders and named timers don't sound when there is no connection
* GUI cards don't show for Kindle

### [1.1.0] - 2017-10-02
* **Enhancements**
  * Better GStreamer error reporting. MediaPlayer used to only report   `MEDIA_ERROR_UNKNOWN`, now reports more specific errors as defined in `ErrorType.h`.
  * Codebase has been formatted for easier reading.
  * `DirectiveRouter::removeDirectiveHandler()` signature changed and now returns a bool indicating if given handler should be successfully removed or not.
  * Cleanup of raw and shared pointers in the creation of `Transport` objects.
  * `HTTP2Stream`s now have IDs assigned as they are acquired as opposed to created, making associated logs easier to interpret.
  * `AlertsCapabilityAgent` has been refactored.
      * Alert management has been factored out into an `AlertScheduler` class.
  * Creation of Reminder (implements Alert) class.
  * Added new capability agent for `PlaybackController` with unit tests.
  * Added Settings interface with unit tests.
  * Return type of `getOffsetInMilliseconds()` changed from `int64_t` to `std::chronology::milliseconds`.
  * Added `AudioPlayer` unit tests.
  * Added teardown for all Integration tests except Alerts.
  * Implemented PlaylistParser.

* **Bug fixes**:
  * AIP getting stuck in `LISTENING` or `THINKING` and refusing user input on network outage.
  * SampleApp crashing if running for 5 minutes after network disconnect.
  * Issue where on repeated user barge-ins, `AudioPlayer` would not pause. Specifically, the third attempt to “Play iHeartRadio” would not result in currently-playing music pausing.
  * Utterances being ignored after particularly long TTS.
  * GStreamer errors cropping up on SampleApp exit as a result of accessing the pipeline before it’s been setup.
  * Crashing when playing one URL after another.
  * Buffer overrun in Alerts Renderer.
  * [SampleApp crashing when issuing "Alexa skip" command with iHeartRadio.](https://github.com/alexa/avs-device-sdk/issues/153)
  * [`HTTP2Transport` network thread triggering a join on itself.](https://github.com/alexa/avs-device-sdk/issues/127)
  * [`HTTP2Stream` request handling truncating exception messages.](https://github.com/alexa/avs-device-sdk/issues/67)
  * [`AudioPlayer` was attempting an incorrect state transition from `STOPPED` to `PLAYING` through a `playbackResumed`.](https://github.com/alexa/avs-device-sdk/issues/138)

* **Known Issues**

* Native components for the following capability agents are **not** included in this release: `Speaker`, `TemplateRuntime`, and `Notifications`
* `ACL`'s asynchronous receipt of audio attachments may manage resources poorly in scenarios where attachments are received but not consumed.
* When an `AttachmentReader` does not deliver data for prolonged periods, `MediaPlayer` may not resume playing the delayed audio.
* Without the refresh token in the JSON file, the sample app crashes on start up.
* Alerts do not play after restarting the device.
* Alexa's responses are cut off by about half a second when asking "What's up" or barging into an active alarm to ask the time.
* Switching from Kindle to Amazon Music after pausing and resuming Kindle doesn't work.
* Pause/resume on Amazon Music causes entire song to start over.
* Stuck in listening state if `ExpectSpeech` comes in when the microphone has been turned off.
* Pausing and resuming Pandora causes stuttering, looped audio.
* Audible features are not fully supported.
* `Recognize` event after regaining network connection and during an alarm going off can cause client to get stuck in `Recognizing` state.
* Three Alerts integration tests fail: `handleMultipleTimersWithLocalStop`, `AlertsTest.UserLongUnrelatedBargeInOnActiveTimer`, `AlertsTest.handleOneTimerWithVocalStop`
* `MediaPlayerTest.testSetOffsetSeekableSource` unit test fails intermittently on Linux.

### [1.0.3] - 2017-09-19
* **Enhancements**
  * Implemented `setOffSet` in `MediaPlayer`.
  * [Updated `LoggerUtils.cpp`](https://github.com/alexa/avs-device-sdk/issues/77).

* **Bug Fixes**
  * [Bug fix to address incorrect stop behavior caused when Audio Focus is set to `NONE` and released](https://github.com/alexa/avs-device-sdk/issues/129).
  * Bug fix for intermittent failure in `handleMultipleConsecutiveSpeaks`.
  * Bug fix for `jsonArrayExist` incorrectly parsing JSON when trying to locate array children.
  * Bug fix for ADSL test failures with `sendDirectiveWithoutADialogRequestId`.
  * Bug fix for `SpeechSynthesizer` showing the wrong UX state when a burst of `Speak` directives are received.
  * Bug fix for recursive loop in `AudioPlayer.Stop`.
>>>>>>> 3553854... Updated CHANGELOG.md and README.md for 1.1

### [1.0.2] - 2017-08-23
* Removed code from AIP which propagates ExpectSpeech initiator strings to subsequent Recognize events.  This code will be re-introduced when AVS starts sending initiator strings.

### [1.0.1] - 2017-08-17

* Added a fix to the sample app so that the `StateSynchronization` event is the first that gets sent to AVS.
* Added a `POST_CONNECTED` enum to `ConnectionStatusObserver`.
* `StateSychronizer` now automatically sends a `StateSynchronization` event when it receives a notification that `ACL` is `CONNECTED`.
* Added `make install` for installing the AVS Device SDK.
* Added an optional `make networkIntegration` for integration tests for slow network (only on Linux platforms).
* Added shutdown management which fully cleans up SDK objects during teardown.
* Fixed an issue with `AudioPlayer` barge-in which was preventing subsequent audio from playing.
* Changed `Mediaplayer` buffering to reduce stuttering.
* Known Issues:
  * Connection loss during listening keeps the app in that state even after connection is regained. Pressing ‘s’ unsticks the state.
  * Play/Pause media restarts it from the beginning.
  * `SpeechSynthesizer` shows wrong UX state during a burst of Speaks.
  * Quitting the sample app while `AudioPlayer` is playing something causes a segmentation fault.
  * `AudioPlayer` sending `PlaybackPaused` during flash briefing.
  * Long Delay playing live stations on iHeartRadio.
  * Teardown warnings at the end of integration tests.

### [1.0.0] - 2017-08-07

* Added `AudioPlayer` capability agent.
  * Supports iHeartRadio.
* `StateSynchronizer` has been updated to better enforce that `System.SynchronizeState` is the first Event sent on a connection to AVS.
* Additional tests have been added to `ACL`.
* The `Sample App` has been updated with several small fixes and improvements.
* `ADSL` was updated such that all directives are now blocked while the handling of previous `SpeechSynthesizer.Speak` directives complete.  Because any directive may now be blocked, the `preHandleDirective() / handleDirective()` path is now used for handling all directives.
* Fixes for the following GitHub issues:
  * [EXPECTING_SPEECH + SPEAK directive simultaneously on multi-turn conversation](https://github.com/alexa/alexa-client-sdk/issues/44).
* A bug causing `ACL` to not send a ping to AVS every 5 minutes, leading to periodic server disconnects, was fixed.
* Subtle race condition issues were addressed in the `Executor` class, resolving some intermittent crashes.
* Known Issues
  * Native components for the following capability agents are **not** included in this release: `PlaybackController`, `Speaker`, `Settings`, `TemplateRuntime`, and `Notifications`.
  * `MediaPlayer`
    * Long periods of buffer underrun can cause an error related with seeking and subsequent stopped playback.
    * Long periods of buffer underrun can cause flip flopping between buffer_underrun and playing states.
    * Playlist parsing is not supported unless -DTOTEM_PLPARSER=ON is specified.
  * `AudioPlayer`
    * Amazon Music, TuneIn, and SiriusXM are not supported in this release.
    * Our parsing of urls currently depends upon GNOME/totem-pl-parser which only works on some Linux platforms.
  * `AlertsCapabilityAgent`
    * Satisfies the [AVS specification](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/timers-and-alarms-conceptual-overview) except for sending retrospective Events. For example, sending `AlertStarted` Event for an Alert which rendered when there was no internet connection.
  * `Sample App`:
      * Any connection loss during the `Listening` state keeps the app stuck in that state, unless the ongoing interaction is manually stopped by the user.
      * The user must wait several seconds after starting up the sample app before the sample app is properly usable.
  * `Tests`:
    * `SpeechSynthesizer` unit tests hang on some older versions of GCC due to a tear down issue in the test suite
    * Intermittent Alerts integration test failures caused by rigidness in expected behavior in the tests

### [0.6.0] - 2017-07-14

* Added a sample app that leverages the SDK.
* Added `Alerts` capability agent.
* Added the `DefaultClient` class.
* Added the following classes to support directives and events in the [`System` interface](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system): `StateSynchronizer`, `EndpointHandler`, and `ExceptionEncounteredSender`.
* Added unit tests for `ACL`.
* Updated `MediaPlayer` to play local files given an `std::istream`.
* Changed build configuration from `Debug` to `Release`.
* Removed `DeprecatedLogger` class.
* Known Issues:
    * `MediaPlayer`: Our `GStreamer` based implementation of `MediaPlayer` is not fully robust, and may result in fatal runtime errors, under the following conditions:
        * Attempting to play multiple simultaneous audio streams
        * Calling `MediaPlayer::play()` and `MediaPlayer::stop()` when the MediaPlayer is already playing or stopped, respectively.
        * Other miscellaneous issues, which will be addressed in the near future
    * `AlertsCapabilityAgent`:
        * This component has been temporarily simplified to work around the known `MediaPlayer` issues mentioned above
        * Fully satisfies the AVS specification except for sending retrospective Events, for example, sending `AlertStarted` for an Alert which rendered when there was no Internet connection
        * This component is not fully thread-safe, however, this will be addressed shortly
        * Alerts currently run indefinitely until stopped manually by the user. This will be addressed shortly by having a timeout value for an alert to stop playing.
        * Alerts do not play in the background when Alexa is speaking, but will continue playing after Alexa stops speaking.
    * `Sample App`:
      * Without the refresh token being filled out in the JSON file, the sample app crashes on start up.
      * Any connection loss during the `Listening` state keeps the app stuck in that state, unless the ongoing interaction is manually stopped by the user.
      * At the end of a shopping list with more than 5 items, the interaction in which Alexa asks the user if he/she would like to hear more does not finish properly.
  * `Tests`:
    * `SpeechSynthesizer` unit tests hang on some older versions of GCC due to a tear down issue in the test suite
    * Intermittent Alerts integration test failures caused by rigidness in expected behavior in the tests

### [0.5.0] - 2017-06-23

* Updated most SDK components to use new logging abstraction.
* Added a `getConfiguration()` method to `DirectiveHandlerInterface` to register capability agents with Directive Sequencer.
* Added `ACL` stream processing with pause and redrive.
* Removed the dependency of `ACL` library on `Authdelegate`.
* Added an interface to allow `ACL` to add/remove `ConnectionStatusObserverInterface`.
* Fixed compile errors in KITT.ai, `DirectiveHandler` and compiler warnings in `AIP` tests.
* Corrected formatting of code in many files.
* Fixes for the following GitHub issues:
  * [MessageRequest callbacks never triggered if disconnected](https://github.com/alexa/alexa-client-sdk/issues/21)
  * [AttachmentReader::read() returns ReadStatus::CLOSED if an AttachmentWriter has not been created yet](https://github.com/alexa/alexa-client-sdk/issues/25)

### [0.4.1] - 2017-06-09

* Implemented Sensory wake word detector functionality.
* Removed the need for a `std::recursive_mutex` in `MessageRouter`.
* Added `AIP` unit tests.
* Added `handleDirectiveImmediately` functionality to `SpeechSynthesizer`.
* Added memory profiles for:
  * AIP
  * SpeechSynthesizer
  * ContextManager
  * AVSUtils
  * AVSCommon
* Bug fix for `MessageRouterTest` aborting intermittently.
* Bug fix for `MultipartParser.h` compiler warning.
* Suppression of sensitive log data even in debug builds. Use CMake parameter -DACSDK_EMIT_SENSITIVE_LOGS=ON to allow logging of sensitive information in DEBUG builds.
* Fixed crash in `ACL` when attempting to use more than 10 streams.
* Updated `MediaPlayer` to use `autoaudiosink` instead of requiring `pulseaudio`.
* Updated `MediaPlayer` build to suppport local builds of GStreamer.
* Fixes for the following GitHub issues:
  * [MessageRouter::send() does not take the m_connectionMutex](https://github.com/alexa/alexa-client-sdk/issues/5)
  * [MessageRouter::disconnectAllTransportsLocked flow leads to erase while iterating transports vector](https://github.com/alexa/alexa-client-sdk/issues/8)
  * [Build errors when building with KittAi enabled](https://github.com/alexa/alexa-client-sdk/issues/9)
  * [HTTP2Transport race may lead to deadlock](https://github.com/alexa/alexa-client-sdk/issues/10)
  * [Crash in HTTP2Transport::cleanupFinishedStreams()](https://github.com/alexa/alexa-client-sdk/issues/17)
  * [The attachment writer interface should take a `const void*` instead of `void*`](https://github.com/alexa/alexa-client-sdk/issues/24)

### [0.4.0] - 2017-05-31 (patch)

* Added `AuthServer`, an authorization server implementation used to retrieve refresh tokens from LWA.

### [0.4.0] - 2017-05-24

* Added `SpeechSynthesizer`, an implementation of the `SpeechRecognizer` capability agent.
* Implemented a reference `MediaPlayer` based on [GStreamer](https://gstreamer.freedesktop.org/) for audio playback.
* Added `MediaPlayerInterface` that allows you to implement your own media player.
* Updated `ACL` to support asynchronous receipt of audio attachments from AVS.
* Bug Fixes:
  * Some intermittent unit test failures were fixed.
* Known Issues:
  * `ACL`'s asynchronous receipt of audio attachments may manage resources poorly in scenarios where attachments are received but not consumed.
  * When an `AttachmentReader` does not deliver data for prolonged periods `MediaPlayer` may not resume playing the delayed audio.

### [0.3.0] - 2017-05-17

* Added the `CapabilityAgent` base class that is used to build capability agent implementations.
* Added `ContextManager`, a component that allows multiple capability agents to store and access state. These Events include `Context`, which is used to communicate the state of each capability agent to AVS in the following Events:
  * [`Recognize`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer#recognize)
  * [`PlayCommandIssued`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller#playcommandissued)
  * [`PauseCommandIssued`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller#pausecommandissued)
  * [`NextCommandIssued`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller#nextcommandissued)
  * [`PreviousCommandIssued`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller#previouscommandissued)
  * [`SynchronizeState`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system#synchronizestate)
  * [`ExceptionEncountered`](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system#exceptionencountered)
* Added `SharedDataStream` (SDS) to asynchronously communicate data between a local reader and writer.
* Added `AudioInputProcessor` (AIP), an implementation of a `SpeechRecognizer` capability agent.
* Added WakeWord Detector (WWD), which recognizes keywords in audio streams. [0.3.0] implements a wrapper for KITT.ai.
* Added a new implementation of `AttachmentManager` and associated classes for use with SDS.
* Updated `ACL` to support asynchronously sending audio to AVS.

### [0.2.1] - 2017-05-03

* Replaced the configuration file `AuthDelegate.config` with `AlexaClientSDKConfig.json`.
* Added the ability to specify a `CURLOPT_CAPATH` value to be used when libcurl is used by ACL and AuthDelegate.  See See Appendix C in the README for details.
* Changes to ADSL interfaces:
  * The [0.2.0] interface for registering directive handlers (`DirectiveSequencer::setDirectiveHandlers()`) was problematic because it canceled the ongoing processing of directives and dropped further directives until it completed. The revised API makes the operation immediate without canceling or dropping any handling.  However, it does create the possibility that `DirectiveHandlerInterface` methods `preHandleDirective()` and `handleDirective()` may be called on different handlers for the same directive.
  * `DirectiveSequencerInterface::setDirectiveHandlers()` was replaced by `addDirectiveHandlers()` and `removeDirectiveHandlers()`.
  * `DirectiveHandlerInterface::shutdown()` was replaced with `onDeregistered()`.
  * `DirectiveHandlerInterface::preHandleDirective()` now takes a `std::unique_ptr` instead of a `std::shared_ptr` to `DirectiveHandlerResultInterface`.
  * `DirectiveHandlerInterface::handleDirective()` now returns a bool indicating if the handler recognizes the `messageId`.
* Bug fixes:
  * ACL and AuthDelegate now require TLSv1.2.
  * `onDirective()` now sends `ExceptionEncountered` for unhandled directives.
  * `DirectiveSequencer::shutdown()` no longer sends `ExceptionEncountered()` for queued directives.

### [0.2.0] - 2017-03-27 (patch)

* Added memory profiling for ACL and ADSL.  See Appendix A in the README.
* Added a command to build the API documentation.

### [0.2.0] - 2017-03-09

* Added `Alexa Directive Sequencer Library` (ADSL) and `Alexa Focus Manager Library` (AMFL).
* CMake build types and options have been updated.
* Documentation for libcurl optimization included.

### [0.1.0] - 2017-02-10

* Initial release of the `Alexa Communications Library` (ACL), a component which manages network connectivity with AVS, and `AuthDelegate`, a component which handles user authorization with AVS.
