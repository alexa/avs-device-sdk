### What is the Alexa Voice Service (AVS)?

The Alexa Voice Service (AVS) enables developers to integrate Alexa directly into their products, bringing the convenience of voice control to any connected device. AVS provides developers with access to a suite of resources to quickly and easily build Alexa-enabled products, including APIs, hardware development kits, software development kits, and documentation.

[Learn more »](https://developer.amazon.com/alexa-voice-service)

### Overview of the AVS Device SDK

The AVS Device SDK provides C++-based (11 or later) libraries that leverage the AVS API to create device software for Alexa-enabled products. It is modular and abstracted, providing components for handling discrete functions such as speech capture, audio processing, and communications, with each component exposing the APIs that you can use and customize for your integration. It also includes a sample app, which demonstrates the interactions with AVS.

### Get Started

You can set up the SDK on the following platforms:
* [Ubuntu Linux](https://github.com/alexa/avs-device-sdk/wiki/Ubuntu-Linux-Quick-Start-Guide)
* [Raspberry Pi](https://github.com/alexa/avs-device-sdk/wiki/Raspberry-Pi-Quick-Start-Guide-with-Script) (Raspbian Stretch)
* [macOS](https://github.com/alexa/avs-device-sdk/wiki/macOS-Quick-Start-Guide)
* [Windows 64-bit](https://github.com/alexa/avs-device-sdk/wiki/Windows-Quick-Start-Guide-with-Script)
* [Generic Linux](https://github.com/alexa/avs-device-sdk/wiki/Linux-Reference-Guide)
* [Android](https://github.com/alexa/avs-device-sdk/wiki/Android-Quick-Start-Guide)

You can also prototype with a third party development kit:
* [XMOS VocalFusion 4-Mic Kit](https://github.com/xmos/vocalfusion-avs-setup) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/xmos-vocal-fusion)
* [Synaptics AudioSmart 2-Mic Dev Kit for Amazon AVS with NXP SoC](https://www.nxp.com/docs/en/user-guide/Quick-Start-Guide-for-Arrow-AVS-kit.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/synaptics-2-mic)
* [Intel Speech Enabling Developer Kit](https://avs-dvk-workshop.github.io) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/intel-speech-enabling/)
* [Amlogic A113X1 Far-Field Dev Kit for Amazon AVS](http://openlinux2.amlogic.com/download/doc/A113X1_Usermanual.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/amlogic-6-mic)
* [Allwinner SoC-Only 3-Mic Far-Field Dev Kit for Amazon AVS](http://www.banana-pi.org/images/r18avs/AVS-quickstartguide.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/allwinner-3-mic)
* [DSPG HDClear 3-Mic Development Kit for Amazon AVS](https://www.dspg.com/dspg-hdclear-3-mic-development-kit-for-amazon-avs/#documentation) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/dspg-3-mic)

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
* [Bluetooth](https://developer.amazon.com/docs/alexa-voice-service/bluetooth.html) - The interface for managing Bluetooth connections between peer devices and Alexa-enabled products.
* [EqualizerController](https://developer.amazon.com/docs/alexa-voice-service/equalizercontroller.html) - The interface for adjusting equalizer settings, such as decibel (dB) levels and modes.

### Security Best Practices

In addition to adopting the [Security Best Practices for Alexa](https://developer.amazon.com/docs/alexa-voice-service/security-best-practices.html), when building the SDK:

* Protect configuration parameters, such as those found in the AlexaClientSDKCOnfig.json file, from tampering and inspection.
* Protect executable files and processes from tampering and inspection.
* Protect storage of the SDK's persistent states from tampering and inspection.
* Your C++ implementation of AVS Device SDK interfaces must not retain locks, crash, hang, or throw exceptions.
* Use exploit mitigation flags and memory randomization techniques when you compile your source code, in order to prevent vulnerabilities from exploiting buffer overflows and memory corruptions.

### Important Considerations

* Review the AVS [Terms & Agreements](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/support/terms-and-agreements).
* The earcons associated with the sample project are for **prototyping purposes** only. For implementation and design guidance for commercial products, please see [Designing for AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/designing-for-the-alexa-voice-service) and [AVS UX Guidelines](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/alexa-voice-service-ux-design-guidelines).
* Please use the contact information below to-
  * [Contact Sensory](http://www.sensory.com/support/contact/us-sales/) for information on TrulyHandsFree licensing.
  * [Contact KITT.AI](mailto:snowboy@kitt.ai) for information on SnowBoy licensing.
* **IMPORTANT**: The Sensory wake word engine referenced in this document is time-limited: code linked against it will stop working when the library expires. The library included in this repository will, at all times, have an expiration date that is at least 120 days in the future. See [Sensory's GitHub ](https://github.com/Sensory/alexa-rpi#license)page for more information.

### Release Notes and Known Issues

**Note**: Feature enhancements, updates, and resolved issues from previous releases are available to view in [CHANGELOG.md](https://github.com/alexa/alexa-client-sdk/blob/master/CHANGELOG.md).

v1.10.0 released 10/24/2018:

**Enhancements**

* New optional configuration for [EqualizerController](https://github.com/alexa/avs-device-sdk/blob/v1.10.0/Integration/AlexaClientSDKConfig.json#L154). The EqualizerController interface allows you to adjust equalizer settings on your product, such as decibel (dB) levels and modes.
* Added reference implementation of the EqualizerController for GStreamer-based (MacOS, Linux, and Raspberry Pi) and OpenSL ES-based (Android) MediaPlayers. Note: In order to use with Android, it must support OpenSL ES.
* New optional configuration for the [TemplateRuntime display card value](https://github.com/alexa/avs-device-sdk/blob/v1.10.0/Integration/AlexaClientSDKConfig.json#L144).
* A configuration file generator script, `genConfig.sh` is now included with the SDK in the **tools/Install** directory. `genConfig.sh` and it's associated arguments populate `AlexaClientSDKConfig.json` with the data required to authorize with LWA.
* Added Bluetooth A2DP source and AVRCP target support for Linux.
* Added Alexa for Business (A4B) support, which includes support for handling the new [RevokeAuthorization](https://developer.amazon.com/docs/alexa-voice-service/system.html#revokeauth) directive in the Settings interface. A new CMake option has been added to enable A4B within the SDK, `-DA4B`.
* Added locale support for IT and ES.
* The Alexa Communication Library (ACL), `CBLAUthDelegate`, and sample app have been enhanced to detect de-authorization using the new `z` command.
* Added `ExternalMediaPlayerObserver`, which receives notification of player state, track, and username changes.
* `HTTP2ConnectionInterface` was factored out of `HTTP2Transport` to enable unit testing of `HTTP2Transport` and re-use of `HTTP2Connection` logic.

**Bug Fixes**

* Fixed a bug in which `ExternalMediaPlayer` adapter playback wasn't being recognized by AVS.
* [Issue 973](https://github.com/alexa/avs-device-sdk/issues/973) - Fixed issues related to `AudioPlayer` where progress reports were being sent out of order or with incorrect offsets.
* An `EXPECTING`, state has been added to `DialogUXState` in order to handle `EXPECT_SPEECH` state for hold-to-talk devices.
* [Issue 948](https://github.com/alexa/avs-device-sdk/issues/948) - Fixed a bug in which the sample app was stuck in a listening state.
* Fixed a bug where there was a delay between receiving a `DeleteAlert` directive, and deleting the alert.
* [Issue 839](https://github.com/alexa/avs-device-sdk/issues/839) - Fixed an issue where speech was being truncated due to the `DialogUXStateAggregator` transitioning between a `THINKING` and `IDLE` state.
* Fixed a bug in which the `AudioPlayer` attempted to play when it wasn't in the `FOREGROUND` focus.
* `CapabilitiesDelegateTest` now works on Android.
* [Issue 950](https://github.com/alexa/avs-device-sdk/issues/950) - Improved Android Media Player audio quality.
* [Issue 908](https://github.com/alexa/avs-device-sdk/issues/908) - Fixed compile error on g++ 7.x in which includes were missing.

**Known Issues**

* On GCC 8+, issues related to `-Wclass-memaccess` will trigger warnings. However, this won't cause the build to fail, and these warnings can be ignored.
* In order to use Bluetooth source and sink PulseAudio, you must manually load and unload PulseAudio modules after the SDK starts.
* When connecting a new device to AVS, *currently connected devices must be manually disconnected*. For example, if a user says "Alexa, connect my phone", and an Alexa-enabled speaker is already connected, there is no indication to the user a device is already connected.
* The `ACL` may encounter issues if audio attachments are received but not consumed.
* `SpeechSynthesizerState` currently uses `GAINING_FOCUS` and `LOSING_FOCUS` as a workaround for handling intermediate state. These states may be removed in a future release.
* The Alexa app doesn't always indicate when a device is successfully connected via Bluetooth.
* Connecting a product to streaming media via Bluetooth will sometimes stop media playback within the source application. Resuming playback through the source application or toggling next/previous will correct playback.
* When a source device is streaming silence via Bluetooth, the Alexa app indicates that audio content is streaming.
* The Bluetooth agent assumes that the Bluetooth adapter is always connected to a power source. Disconnecting from a power source during operation is not yet supported.
* On some products, interrupted Bluetooth playback may not resume if other content is locally streamed.
* `make integration` is currently not available for Android. In order to run integration tests on Android, you'll need to manually upload the test binary file along with any input file. At that point, the adb can be used to run the integration tests.
* On Raspberry Pi running Android Things with HDMI output audio, beginning of speech is truncated when Alexa responds to user TTS.
* When the sample app is restarted and network connection is lost, Alerts don't play.
* When network connection is lost, lost connection status is not returned via local TTS.
