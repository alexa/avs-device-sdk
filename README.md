### What is the Alexa Voice Service (AVS)?

The Alexa Voice Service (AVS) enables developers to integrate Alexa directly into their products, bringing the convenience of voice control to any connected device. AVS provides developers with access to a suite of resources to quickly and easily build Alexa-enabled products, including APIs, hardware development kits, software development kits, and documentation.

[Learn more »](https://developer.amazon.com/alexa-voice-service)

### Overview of the AVS Device SDK

The AVS Device SDK provides C++-based (11 or later) libraries that leverage the AVS API to create device software for Alexa-enabled products. It is modular and abstracted, providing components for handling discrete functions such as speech capture, audio processing, and communications, with each component exposing the APIs that you can use and customize for your integration. It also includes a sample app, which demonstrates the interactions with AVS.   

### Get Started

You can set up the SDK on the following platforms:
* [Linux or macOS](https://github.com/alexa/avs-device-sdk/wiki/Linux-Quick-Start-Guide)  
* [Raspberry Pi](https://github.com/alexa/avs-device-sdk/wiki/Raspberry-Pi-Quick-Start-Guide) (Raspbian Stretch)  

Or if you prefer, you can start with our [SDK API Documentation](https://alexa.github.io/avs-device-sdk/).

### Learn More About The AVS Device SDK

[Watch this tutorial](https://youtu.be/F5DixCPJYo8) to learn about the how this SDK works and the set up process.  

[![Tutorial](https://img.youtube.com/vi/F5DixCPJYo8/0.jpg)](https://www.youtube.com/watch?v=F5DixCPJYo8)

### SDK Architecture  

This diagram illustrates the data flows between components that comprise the AVS Device SDK for C++.

![SDK Architecture Diagram](https://m.media-amazon.com/images/G/01/mobile-apps/dex/avs/Alexa_Device_SDK_Architecture.png)

**Audio Signal Processor (ASP)** - Third-party software that applies signal processing algorithms to both input and output audio channels. The applied algorithms are designed to produce clean audio data and include, but are not limited to acoustic echo cancellation (AEC), beam forming (fixed or adaptive), voice activity detection (VAD), and dynamic range compression (DRC). If a multi-microphone array is present, the ASP constructs and outputs a single audio stream for the array.

**Shared Data Stream (SDS)** - A single producer, multi-consumer buffer that allows for the transport of any type of data between a single writer and one or more readers. SDS performs two key tasks:

1. It passes audio data between the audio front end (or Audio Signal Processor), the wake word engine, and the Alexa Communications Library (ACL) before sending to AVS  
2. It passes data attachments sent by AVS to specific capability agents via the ACL  

SDS is implemented atop a ring buffer on a product-specific memory segment (or user-specified), which allows it to be used for in-process or interprocess communication. Keep in mind, the writer and reader(s) may be in different threads or processes.

**Wake Word Engine (WWE)** - Software that spots wake words in an input stream. It is comprised of two binary interfaces. The first handles wake word spotting (or detection), and the second handles specific wake word models (in this case "Alexa"). Depending on your implementation, the WWE may run on the system on a chip (SOC) or dedicated chip, like a digital signal processor (DSP).

**Audio Input Processor (AIP)** - Handles audio input that is sent to AVS via the ACL. These include on-device microphones, remote microphones, an other audio input sources.

The AIP also includes the logic to switch between different audio input sources. Only one audio input source can be sent to AVS at a given time.

**Alexa Communications Library (ACL)** - Serves as the main communications channel between a client and AVS. The ACL performs two key functions:

1. Establishes and maintains long-lived persistent connections with AVS. ACL adheres to the messaging specification detailed in [Managing an HTTP/2 Connection with AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection).
2. Provides message sending and receiving capabilities, which includes support JSON-formatted text, and binary audio content. For additional information, see [Structuring an HTTP/2 Request to AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/avs-http2-requests).

**Alexa Directive Sequencer Library (ADSL)**: Manages the order and sequence of directives from AVS, as detailed in the [AVS Interaction Model](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/interaction-model#channels). This component manages the lifecycle of each directive, and informs the Directive Handler (which may or may not be a Capability Agent) to handle the message.

**Activity Focus Manager Library (AFML)**: Provides centralized management of audiovisual focus for the device. Focus is based on channels, as detailed in the [AVS Interaction Model](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/interaction-model#channels), which are used to govern the prioritization of audiovisual inputs and outputs.

Channels can either be in the foreground or background. At any given time, only one channel can be in the foreground and have focus. If multiple channels are active, you need to respect the following priority order: Dialog > Alerts > Content. When a channel that is in the foreground becomes inactive, the next active channel in the priority order moves into the foreground.

Focus management is not specific to Capability Agents or Directive Handlers, and can be used by non-Alexa related agents as well. This allows all agents using the AFML to have a consistent focus across a device.

**Capability Agents**: Handle Alexa-driven interactions; specifically directives and events. Each capability agent corresponds to a specific interface exposed by the AVS API. These interfaces include:

* [SpeechRecognizer](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechrecognizer) - The interface for speech capture.
* [SpeechSynthesizer](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speechsynthesizer) - The interface for Alexa speech output.
* [Alerts](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/alerts) - The interface for setting, stopping, and deleting timers and alarms.
* [AudioPlayer](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/audioplayer) - The interface for managing and controlling audio playback.  
* [Notifications](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/notifications) - The interface for displaying notifications indicators.
* [PlaybackController](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/playbackcontroller) - The interface for navigating a playback queue via GUI or buttons.
* [Speaker](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/speaker) - The interface for volume control, including mute and unmute.
* [System](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/system) - The interface for communicating product status/state to AVS.  
* [TemplateRuntime](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/templateruntime) - The interface for rendering visual metadata.  

### Important Considerations

* Review the AVS [Terms & Agreements](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/support/terms-and-agreements).
* The earcons associated with the sample project are for **prototyping purposes** only. For implementation and design guidance for commercial products, please see [Designing for AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/designing-for-the-alexa-voice-service) and [AVS UX Guidelines](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/alexa-voice-service-ux-design-guidelines).
* Please use the contact information below to-
  * [Contact Sensory](http://www.sensory.com/support/contact/us-sales/) for information on TrulyHandsFree licensing.
  * [Contact KITT.AI](mailto:snowboy@kitt.ai) for information on SnowBoy licensing.
* **IMPORTANT**: The Sensory wake word engine referenced in this document is time-limited: code linked against it will stop working when the library expires. The library included in this repository will, at all times, have an expiration date that is at least 120 days in the future. See [Sensory's GitHub ](https://github.com/Sensory/alexa-rpi#license)page for more information.


### Release Notes and Known Issues

**Note**: Features, updates, and resolved issues from previous releases are available to view in [CHANGELOG.md](https://github.com/alexa/alexa-client-sdk/blob/master/CHANGELOG.md).

v1.1 released 10/02/2017:  

**Enhancements**

* The SDK now supports TuneIn, Audible, Pandora, SiriusXM, Kindle Books, and Amazon Music.
* Added a capability agent for Settings, which allows a client to notify Alexa of locale changes.
* Added a capability agent for PlaybackController, which exposes these transport controls: play, pause, next, and previous.
* Added `PlaylistParser`, which is used to parse m3u and pls formatted playlists.
* Added new unit tests for `AudioPlayer`.
* Refactored the Alerts capability agent to support named timers and reminders.  

**Bug Fixes**  

* Bug fix to address AIP getting stuck in the `LISTENING` or `THINKING` state and refusing user input during a network outage.   
* Bug fix for the sample app crashing when it runs for more than 5 minutes following a network disconnect.  
* Bug fix for user utterances being ignored after long Alexa TTS.  
* Bug fix for GStreamer errors that appear on `SampleApp` exit that result from accessing the pipeline before it has been setup.  
* Fix crash when playing URLs one after another.  
* Bug fix for overrun in `AlertsRenderer`.
* SampleApp crashing when issuing "Alexa skip" command with iHeartRadio. (https://github.com/alexa/avs-device-sdk/issues/153)
* [`HTTP2Transport` network thread triggering a join on itself.](https://github.com/alexa/avs-device-sdk/issues/127)
* [`HTTP2Stream` request handling truncating exception messages.](https://github.com/alexa/avs-device-sdk/issues/67)
* [`AudioPlayer` was attempting an incorrect state transition from `STOPPED` to `PLAYING` through a `playbackResumed`.](https://github.com/alexa/avs-device-sdk/issues/138)


**Known Issues**

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
