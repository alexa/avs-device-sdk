### IMPORTANT NOTE
If you are updating from v1.3 or earlier to v1.6, you must update your `AlexaClientSDKConfig.json` to include a Notifications database. An updated sample is available in the quickstart guides for Ubuntu Linux, Raspberry Pi, macOS, and Generic Linux.

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

You can also prototype with a third party development kit:
* [XMOS VocalFusion 4-Mic Kit](https://github.com/xmos/vocalfusion-avs-setup) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/xmos-vocal-fusion)
* [Synaptics AudioSmart 2-Mic Dev Kit for Amazon AVS with NXP SoC](https://www.nxp.com/docs/en/user-guide/Quick-Start-Guide-for-Arrow-AVS-kit.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/synaptics-2-mic)
* [Intel Speech Enabling Developer Kit](https://avs-dvk-workshop.github.io) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/intel-speech-enabling/)
* [Amlogic A113X1 Far-Field Dev Kit for Amazon AVS](http://openlinux2.amlogic.com/download/doc/A113X1_Usermanual.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/amlogic-6-mic)
* [Allwinner SoC-Only 3-Mic Far-Field Dev Kit for Amazon AVS](http://www.banana-pi.org/images/r18avs/AVS-quickstartguide.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/allwinner-3-mic)

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

**Note**: Feature enhancements, updates, and resolved issues from previous releases are available to view in [CHANGELOG.md](https://github.com/alexa/alexa-client-sdk/blob/master/CHANGELOG.md).

v1.6.0 released 03/08/2018:

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
