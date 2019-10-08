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

### v1.15.0 released 09/25/2019:

**Enhancements**

* Added `SystemSoundPlayer` to [ApplicationUtilities](https://alexa.github.io/avs-device-sdk/namespacealexa_client_s_d_k_1_1application_utilities.html). `SystemSoundPlayer` is a new class that plays pre-defined sounds. Sounds currently supported include the wake word notification and the end of speech tone. This change is internal and you don't need to update your code.
* Removed [Echo Spatial Perception (ESP)](https://developer.amazon.com/blogs/alexa/post/042be85c-5a62-4c55-a18d-d7a82cf394df/esp-moves-to-the-cloud-for-alexa-enabled-devices) functionality from the Alexa Voice Service (AVS) device SDK. Make sure you download and test your devices using the new AVS SDK sample app. If you're using an older version of the sample app, manually remove any references to ESP or errors occur during compile.
* Added `onNotificationReceived` to `NotificationsObserverInterface`. `onNotificationReceived` broadcasts when `NotificationsObserverInterface` receives a new notification, instead of only sending the indicator state. This is important if you support a feature that requires a distinct signal for each notification received. See [NotificationsObserverInterface](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1avs_common_1_1sdk_interfaces_1_1_notifications_observer_interface.html) for more details.
* Added support for [Multilingual Mode](https://developer.amazon.com/docs/alexa-voice-service/system.html#localecombinations). With this enabled, Alexa automatically detects what language a user speaks by analyzing the spoken wake word and proceeding utterances. Once Alexa identifies the language, all corresponding responses are in the same language. The current supported language pairs are:
 - `[ "en-US", "es-US" ]`
 - `[ "es-US", "en-US" ]`
 - `[ "en-IN", "hi-IN" ]`
 - `[ "hi-IN", "en-IN" ]`
 - `[ "en-CA", "fr-CA" ]`
 - `[ "fr-CA", "en-CA" ]`
<br/> **IMPORTANT**: Specify the locales your device supports in the [localeCombinations](https://developer.amazon.com/docs/alexa-voice-service/system.html#localecombinations) field in AlexaClientSDKConfig.json. This field can't be empty. If you don't set these values, the sample app fails to run.
* Added two new system settings, [Timezone](https://developer.amazon.com/docs/alexa-voice-service/system.html#settimezone) and [Locale](https://developer.amazon.com/docs/alexa-voice-service/system.html#locales).
    - Timezone: For example, you can set the `defaultTimezone` to `America/Vancouver`. If you don't set a value, `GMT` is set as the default value. If you set a new timezone, make sure that your AVS system settings and default timezone stay in sync. To handle this, use the new class `SystemTimeZoneInterface`. See [System Interface > SetTimeZone](https://developer.amazon.com/docs/alexa-voice-service/system.html#settimezone) for more information.
    - Locale: For example, you can set `defaultLocale` to `en-GB`, instead of the default `en-US`.
* The [SpeechRecognizer](https://developer.amazon.com/docs/alexa-voice-service/speechrecognizer.html) interface now supports the following functionalities.
  - Change wake word (`Alexa` supported for now).
  - Toggle start of request tone on/off.
  - Toggle End of request tone on/off.
* Deprecated the [CapabilityAgents](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1avs_common_1_1avs_1_1_capability_agent.html) `Settings{…}` library. `Settings {…}` now maps to an interface that's no longer supported. You might need to update your code to handle these changes. Read [Settings Interface](https://developer.amazon.com/docs/alexa-voice-service/per-interface-settings.html) for more details.
* Added support for three new locals: Spanish - United States (ES_US), Hindi - India (HI_IN), and Brazilian - Portuguese (PT_BR).
* Linked the atomic library to the sample app to prevent build errors on Raspberry Pi.

**Bug Fixes**

* Fixed resource leaking in [EqualizerCapabilityAgent](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1capability_agents_1_1equalizer_1_1_equalizer_capability_agent.html) after engine shutdown.
* [Issue 1391:](https://github.com/alexa/avs-device-sdk/pull/1391) Fixed an issue where [SQLiteDeviceSettingsStorage::open](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1settings_1_1storage_1_1_s_q_lite_device_setting_storage.html#a7733e56145916f7ff265c5c950add492) tries to acquire a mutex twice, resulting in deadlock.
* [Issue 1468:](https://github.com/alexa/avs-device-sdk/issues/1468) Fixed a bug in [AudioPlayer::cancelDirective](https://alexa.github.io/avs-device-sdk/classalexa_client_s_d_k_1_1capability_agents_1_1audio_player_1_1_audio_player.html#a2c710c16f3627790fcc3238d34da9361) that causes a crash.
* Fixed Windows install script that caused the sample app build to fail - removed pip, flask, requests, and commentjson dependencies from the mingw.sh helper script.
* Fixed issue: notifications failed to sync upon device initialization. For example, let's say you had two devices - one turned on and the other turned off. After clearing the notification on the first device, it still showed up on the second device after turning it on.
* Fixed issue: barging in on a reminder caused it to stick in an inconsistent state, blocking subsequent reminders. For example, if a reminder was going off and you interrupted it, the reminder would get persist indefinitely. You could schedule future reminders, but they wouldn't play. Saying “Alexa stop” or rebooting the device fixed the “stuck” reminder.

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
