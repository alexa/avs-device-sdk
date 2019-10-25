### What is the Alexa Voice Service (AVS)?

The Alexa Voice Service (AVS) enables developers to integrate Alexa directly into their products, bringing the convenience of voice control to any connected device. AVS provides developers with access to a suite of resources to build Alexa-enabled products, including APIs, hardware development kits, software development kits, and documentation.

[Learn more »](https://developer.amazon.com/alexa-voice-service)

### Overview of the AVS Device SDK

The AVS Device SDK provides C++-based (11 or later) libraries that leverage the AVS API to create device software for Alexa-enabled products. It's modular and abstracted, providing components for handling discrete functions such as speech capture, audio processing, and communications, with each component exposing the APIs that you can use and customize for your integration. It also includes a sample app, which demonstrates the interactions with AVS.

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
* [Synaptics AudioSmart 2-Mic Development Kit for Amazon AVS with NXP SoC](https://www.nxp.com/docs/en/user-guide/Quick-Start-Guide-for-Arrow-AVS-kit.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/synaptics-2-mic)
* [Intel Speech Enabling Developer Kit](https://avs-dvk-workshop.github.io) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/intel-speech-enabling/)
* [Amlogic A113X1 Far-Field Development Kit for Amazon AVS](http://openlinux2.amlogic.com/download/doc/A113X1_Usermanual.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/amlogic-6-mic)
* [Allwinner SoC-Only 3-Mic Far-Field Development Kit for Amazon AVS](http://www.banana-pi.org/images/r18avs/AVS-quickstartguide.pdf) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/allwinner-3-mic)
* [DSPG HDClear 3-Mic Development Kit for Amazon AVS](https://www.dspg.com/dspg-hdclear-3-mic-development-kit-for-amazon-avs/#documentation) - Learn More [Here](https://developer.amazon.com/alexa-voice-service/dev-kits/dspg-3-mic)

Or if you prefer, you can start with our [SDK API Documentation](https://alexa.github.io/avs-device-sdk/).

### Learn More About The AVS Device SDK

[Watch this tutorial](https://youtu.be/F5DixCPJYo8) to learn about the how this SDK works and the set up process.

[![Tutorial](https://img.youtube.com/vi/F5DixCPJYo8/0.jpg)](https://www.youtube.com/watch?v=F5DixCPJYo8)

### SDK Architecture

This diagram illustrates the data flows between components that comprise the AVS Device SDK for C++.

![SDK Architecture Diagram](https://m.media-amazon.com/images/G/01/mobile-apps/dex/avs/Alexa_Device_SDK_Architecture.png)

**Audio Signal Processor (ASP)** - Third-party software that applies signal processing algorithms to both input and output audio channels. The applied algorithms produce clean audio through features including, acoustic echo cancellation (AEC), beam forming (fixed or adaptive), voice activity detection (VAD), and dynamic range compression (DRC). If a multi-microphone array is present, the ASP constructs and outputs a single audio stream for the array.

**Shared Data Stream (SDS)** - A single producer, multi-consumer buffer that allows for the transport of data between a single writer and one or more readers. SDS performs two key tasks:

1. Passes audio data between the audio front end (or Audio Signal Processor), the wake word engine, and the Alexa Communications Library (ACL) before sending to AVS
2. Passes data attachments sent by AVS to specific capability agents via the ACL

SDS uses a ring buffer on a product-specific (or user-specified) memory segment, allowing for interprocess communication. Keep in mind, the writer and reader(s) might be in different threads or processes.

**Wake Word Engine (WWE)** - Software that spots wake words in an input stream. Two binary interfaces make up the WWE. The first handles wake word spotting (or detection), and the second handles specific wake word models (in this case "Alexa"). Depending on your implementation, the WWE might run on the system on a chip (SOC) or dedicated chip, like a digital signal processor (DSP).

**Audio Input Processor (AIP)** - Handles audio input sent to AVS via the ACL. These include on-device microphones, remote microphones, an other audio input sources.

The AIP also includes the logic to switch between different audio input sources. AVS can receive one audio input source at a given time.

**Alexa Communications Library (ACL)** - Serves as the main communications channel between a client and AVS. The ACL performs two key functions:

1. Establishes and maintains long-lived persistent connections with AVS. ACL adheres to the messaging specification detailed in [Managing an HTTP/2 Connection with AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/managing-an-http-2-connection).
2. Provides message sending and receiving capabilities, which includes support JSON-formatted text, and binary audio content. For more information, see [Structuring an HTTP/2 Request to AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/docs/avs-http2-requests).

**Alexa Directive Sequencer Library (ADSL)**: Manages the order and sequence of directives from AVS, as detailed in the [AVS Interaction Model](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/interaction-model#channels). This component manages the lifecycle of each directive, and informs the Directive Handler (which might be a Capability Agent) to handle the message.

**Activity Focus Manager Library (AFML)**: Provides centralized management of audiovisual focus for the device. Focus uses channels, as detailed in the [AVS Interaction Model](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/reference/interaction-model#channels), to govern the prioritization of audiovisual inputs and outputs.

Channels can either be in the foreground or background. At any given time, one channel can be in the foreground and have focus. If more than one channels are active, you need to respect the following priority order: Dialog > Alerts > Content. When a channel that's in the foreground becomes inactive, the next active channel in the priority order moves into the foreground.

Focus management isn't specific to Capability Agents or Directive Handlers, and non-Alexa related agents can also use it. This allows all agents using the AFML to have a consistent focus across a device.

**Capability Agents**: Handle Alexa-driven interactions; specifically directives and events. Each capability agent corresponds to a specific interface exposed by the AVS API. These interfaces include:

* [Alerts](https://developer.amazon.com/docs/alexa-voice-service/alerts.html) - The interface for setting, stopping, and deleting timers and alarms.
* [AudioPlayer](https://developer.amazon.com/docs/alexa-voice-service/audioplayer.html) - The interface for managing and controlling audio playback.
* [Bluetooth](https://developer.amazon.com/docs/alexa-voice-service/bluetooth.html) - The interface for managing Bluetooth connections between peer devices and Alexa-enabled products.
* [DoNotDisturb](https://developer.amazon.com/docs/alexa-voice-service/donotdisturb.html) - The interface for enabling the do not disturb feature.
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

All Alexa products should adopt the [Security Best Practices for Alexa](https://developer.amazon.com/docs/alexa-voice-service/security-best-practices.html). When building Alexa with the SDK, you should adhere to the following security principles.

* Protect configuration parameters, such as those found in the AlexaClientSDKCOnfig.json file, from tampering and inspection.
* Protect executable files and processes from tampering and inspection.
* Protect storage of the SDK's persistent states from tampering and inspection.
* Your C++ implementation of AVS Device SDK interfaces must not retain locks, crash, hang, or throw exceptions.
* Use exploit mitigation flags and memory randomization techniques when you compile your source code to prevent vulnerabilities from exploiting buffer overflows and memory corruptions.

### Important Considerations

* Review the AVS [Terms & Agreements](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/support/terms-and-agreements).
* The earcons associated with the sample project are for **prototyping purposes**. For implementation and design guidance for commercial products, please see [Designing for AVS](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/designing-for-the-alexa-voice-service) and [AVS UX Guidelines](https://developer.amazon.com/public/solutions/alexa/alexa-voice-service/content/alexa-voice-service-ux-design-guidelines).
* Please use the contact information below to-
  * [Contact Sensory](http://www.sensory.com/support/contact/us-sales/) for information on TrulyHandsFree licensing.
  * [Contact KITT.AI](mailto:snowboy@kitt.ai) for information on SnowBoy licensing.
* **IMPORTANT**: The Sensory wake word engine referenced in this document is time-limited: code linked against it will stop working when the library expires. The library included in this repository will, at all times, have an expiry date of at least 120 days in the future. See [Sensory's GitHub ](https://github.com/Sensory/alexa-rpi#license)page for more information.

### Release Notes and Known Issues

**Note**: Feature enhancements, updates, and resolved issues from previous releases are available to view in [CHANGELOG.md](https://github.com/alexa/alexa-client-sdk/blob/master/CHANGELOG.md).

### v1.16.0 released 10/25/2019:

**Enhancements** 

- Added support for SpeechSynthesizer v.1.2 which includes the new `playBehaviour` directive.  For more information, see [SpeechSynthesizer v1.2](https://github.com/alexa/avs-device-sdk/wiki/SpeechSynthesizer-Interface-v1.2).
- Added support for pre-buffering in [AudioPlayer](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1capability_agents_1_1audio_player_1_1_audio_player.html). You can optionally choose the number of instances MediaPlayer uses in the [AlexaClientSDKconfig.json](https://github.com/alexa/avs-device-sdk/blob/master/Integration/AlexaClientSDKConfig.json). Important: the contract for [MediaPlayerInterface](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1avs_common_1_1utils_1_1media_player_1_1_media_player_interface.html) has changed. You must now make sure that the `SourceId` value returned by `setSource()` is unique across all instances.
- AudioPlayer is now licensed under the Amazon Software License instead of the Apache Software License.

**Bug Fixes**

- Fixed Android issue that caused the build script to ignore PKG_CONFIG_PATH. This sometimes caused the build to use a preinstalled dependency instead of the specific version downloaded by the Android script (e.g - openssl).
- Fixed Android issue that prevented the Sample app from running at the same time as other applications using the microphone. Android doesn't inherently allow two applications to use the microphone. Pressing the mute button now temporarily stops Alexa from accessing the microphone.
- Added 'quit' (– q) to the settings sub menu.
- Fixed outdated dependencies issue in the Windows install script.
- Fixed reminders issue that caused Notification LEDs to stay on, even after dismissing the alert.

**Known Issues**

* Music playback history isn't displayed in the Alexa app for certain account and device types.
* When using Gnu Compiler Collection 8+ (GCC 8+), `-Wclass-memaccess` triggers warnings. You can ignore these, they don't cause the build to fail.
* Android error `libDefaultClient.so not found` might occur. Resolve this by upgrading to ADB version 1.0.40.
* If a device loses a network connection, the lost connection status isn't returned via local TTS.
* ACL encounters issues if it receives audio attachments but doesn't consume them.
* `SpeechSynthesizerState` uses `GAINING_FOCUS` and `LOSING_FOCUS` as a  workaround for handling intermediate states.
* Media steamed through Bluetooth might abruptly stop. To restart playback, resume the media in the source application or toggle next/previous.
* If a connected Bluetooth device is inactive, the Alexa app might indicates that audio is playing.
* The Bluetooth agent assumes that the Bluetooth adapter is always connected to a power source. Disconnecting from a power source during operation isn't yet supported.
* When using some products, interrupted Bluetooth playback might not resume if other content is locally streamed.
* `make integration` isn't available for Android. To run Android integration tests, manually upload the test binary and input file and run ADB.
* Alexa might truncate the beginning of speech when responding to text-to-speech (TTS) user events. This only impacts Raspberry Pi devices running Android Things with HDMI output audio.
* A reminder TTS message doesn't play if the sample app restarts and loses a network connection. Instead, the default alarm tone plays twice.
* `ServerDisconnectIntegratonTest` tests are disabled until they are updated to reflect new service behavior.
* Bluetooth initialization must complete before connecting devices, otherwise devices are ignored.
* The `DirectiveSequencerTest.test_handleBlockingThenImmediatelyThenNonBockingOnSameDialogId` test fails intermittently.
* On some devices, Alexa gets stuck in a permanent listening state. Pressing `t` and `h` in the Sample App doesn't exit the listening state.
* Exiting the `settings` menu doesn't provide a message to indicate that you are back in the main menu.
