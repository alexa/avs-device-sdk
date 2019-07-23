### What is the Alexa Voice Service (AVS)?

The Alexa Voice Service (AVS) enables developers to integrate Alexa directly into their products, bringing the convenience of voice control to any connected device. AVS provides developers with access to a suite of resources to quickly and easily build Alexa-enabled products, including APIs, hardware development kits, software development kits, and documentation.

[Learn more »](https://developer.amazon.com/alexa-voice-service)

### Overview of the AVS Device SDK

The AVS Device SDK provides C++-based (11 or later) libraries that leverage the AVS API to create device software for Alexa-enabled products. It is modular and abstracted, providing components for handling discrete functions such as speech capture, audio processing, and communications, with each component exposing the APIs that you can use and customize for your integration. It also includes a sample app, which demonstrates the interactions with AVS.

### Get Started

You can set up the SDK on the Raspberry Pi following these instructions:
* [Raspberry Pi](https://github.com/alexa/avs-device-sdk/wiki/Raspberry-Pi-Quick-Start-Guide-with-Script) (Raspbian Stretch)

You can also prototype with a third party development kit:
* [XMOS VocalFusion 4-Mic Kit](https://github.com/xmos/vocalfusion-avs-setup) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/xmos-vocal-fusion)

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

* [Alerts](https://developer.amazon.com/docs/alexa-voice-service/alerts.html) - The interface for setting, stopping, and deleting timers and alarms.
* [AudioPlayer](https://developer.amazon.com/docs/alexa-voice-service/audioplayer.html) - The interface for managing and controlling audio playback.
* [Bluetooth](https://developer.amazon.com/docs/alexa-voice-service/bluetooth.html) - The interface for managing Bluetooth connections between peer devices and Alexa-enabled products.
* [DoNotDisturb](https://developer.amazon.com/docs/alexa-voice-service//donotdisturb.html) - The interface for enabling the do not disturb feature.
* [EqualizerController](https://developer.amazon.com/docs/alexa-voice-service/equalizercontroller.html) - The interface for adjusting equalizer settings, such as decibel (dB) levels and modes.
* [InteractionModel](https://developer.amazon.com/docs/alexa-voice-service/interactionmodel-interface.html) - This interface allows a client to support complex interactions initiated by Alexa, such as Alexa Routines.
* [Notifications](https://developer.amazon.com/docs/alexa-voice-service/notifications.html) - The interface for displaying notifications indicators.
* [PlaybackController](https://developer.amazon.com/docs/alexa-voice-service/playbackcontroller.html) - The interface for navigating a playback queue via GUI or buttons.
* [Speaker](https://developer.amazon.com/docs/alexa-voice-service/speaker.html) - The interface for volume control, including mute and unmute.
* [SpeechRecognizer](https://developer.amazon.com/docs/alexa-voice-service/speechrecognizer.html) - The interface for speech capture.
* [SpeechSynthesizer](https://developer.amazon.com/docs/alexa-voice-service/speechsynthesizer.html) - The interface for Alexa speech output.
* [System](https://developer.amazon.com/docs/alexa-voice-service/system.html) - The interface for communicating product status/state to AVS.
* [TemplateRuntime](https://developer.amazon.com/docs/alexa-voice-service/templateruntime.html) - The interface for rendering visual metadata.

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

### v1.14.0 released 07/09/2019:

**Enhancements**

* AudioPlayer can now pre-buffer audio tracks in the Pre-Handle stage.

**Bug Fixes**

* Fixed an issue in the SQLite wrapper code where a `SQLiteStatement` caused a memory corruption issue.
* Fixed a race condition in SpeechSynthesizer that caused crashes.
* Fixed a `cmake` issue that specifies a dependency for Bluetooth incorrectly.
* Fixed a bug that caused Bluetooth playback to start automatically.
* Changed `supportedOperations` from a vector to a set in `ExternalMediaAdapterInterface`.
* Corrected an issue where a `VolumeChanged` event had previously been sent when the volume was unchanged after `setVolume` or `adjustVolume` had been called locally.
* Fixed issue with `IterativePlaylistParser` that prevented live stations on TuneIn from playing on Android.
* Corrected the spelling of "UNINITIALIZED".

**Known Issues**

* Music playback history isn't being displayed in the Alexa app for certain account and device types.
* On GCC 8+, issues related to `-Wclass-memaccess` will trigger warnings. However, this won't cause the build to fail and these warnings can be ignored.
* Android error ("libDefaultClient.so" not found) can be resolved by upgrading to ADB version 1.0.40
* When network connection is lost, lost connection status is not returned via local TTS.
* `ACL` may encounter issues if audio attachments are received but not consumed.
* `SpeechSynthesizerState` currently uses `GAINING_FOCUS` and `LOSING_FOCUS` as a workaround for handling intermediate state. These states may be removed in a future release.
* The Alexa app doesn't always indicate when a device is successfully connected via Bluetooth.
* Connecting a product to streaming media via Bluetooth will sometimes stop media playback within the source application. Resuming playback through the source application or toggling next/previous will correct playback.
* When a source device is streaming silence via Bluetooth, the Alexa app indicates that audio content is streaming.
* The Bluetooth agent assumes that the Bluetooth adapter is always connected to a power source. Disconnecting from a power source during operation is not yet supported.
* On some products, interrupted Bluetooth playback may not resume if other content is locally streamed.
* `make integration` is currently not available for Android. In order to run integration tests on Android, you'll need to manually upload the test binary file along with any input file. At that point, the adb can be used to run the integration tests.
* On Raspberry Pi running Android Things with HDMI output audio, beginning of speech is truncated when Alexa responds to user text-to-speech (TTS).
* When the sample app is restarted and the network connection is lost, the Reminder TTS message does not play. Instead, the default alarm tone will play twice.
* `ServerDisconnectIntegratonTest` tests have been disabled until they can be updated to reflect new service behavior.
* Devices connected before the Bluetooth CA is initialized are ignored.
* The `DirectiveSequencerTest.test_handleBlockingThenImmediatelyThenNonBockingOnSameDialogId` test fails intermittently.
