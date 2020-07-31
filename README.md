### What is the Alexa Voice Service (AVS)?

The Alexa Voice Service (AVS) enables developers to integrate Alexa directly into their products, bringing the convenience of voice control to any connected device. AVS provides developers with access to a suite of resources to build Alexa-enabled products, including APIs, hardware development kits, software development kits, and documentation.

[Learn more »](https://developer.amazon.com/alexa-voice-service)

### What is the AVS Device SDK

The Alexa Voice Service (AVS) Device SDK provides you with a set of C ++ libraries to build an Alexa Built-in product, meaning your device has direct access to cloud-based Alexa capabilities to receive voice responses instantly. Your device can be almost anything – a smartwatch, a speaker, headphones – the choice is yours.

[Learn more »](https://developer.amazon.com/docs/alexa/avs-device-sdk/overview.html)

### Release Notes and Known Issues

Feature enhancements, updates, and resolved issues from all releases are available on the [Amazon developer portal](https://developer.amazon.com/docs/alexa/avs-device-sdk/release-notes.html).

### Get Started

You can set up the SDK on the Raspberry Pi following these instructions:
* [Raspberry Pi](https://github.com/alexa/avs-device-sdk/wiki/Raspberry-Pi-Quick-Start-Guide-with-Script) (Raspbian Stretch)

You can also prototype with a third party development kit:
* [XMOS VocalFusion 2-Mic (XVF3510) Dev Kit](https://github.com/xmos/vocalfusion-avs-setup) - Learn More [Here](https://www.xmos.com/xvf3510avs)

Or if you prefer, you can start with our [SDK API Documentation](https://alexa.github.io/avs-device-sdk/).

### Learn More About The AVS Device SDK

[Watch this tutorial](https://youtu.be/F5DixCPJYo8) to learn about the how this SDK works and the set up process.

[![Tutorial](https://img.youtube.com/vi/F5DixCPJYo8/0.jpg)](https://www.youtube.com/watch?v=F5DixCPJYo8)

### SDK Architecture

The SDK is modular and abstract. It provides [separate components](https://developer.amazon.com/docs/alexa/avs-device-sdk/overview.html#sdk-architecture) to handle necessary Alexa functionality including processing audio, maintaining persistent connections, and managing Alexa interactions. Each component exposes [Alexa APIs](https://developer.amazon.com/docs/alexa/alexa-voice-service/api-overview.html) to customize your device integrations as needed. The SDK also includes a Sample App, so you can  test interactions before integration.

[Learn more »](https://developer.amazon.com/docs/alexa/avs-device-sdk/overview.html#sdk-architecture)

### API References

View the [C++ API References](https://alexa.github.io/avs-device-sdk/) for detailed information about implementation.

### Security Best Practices and Important Considerations

All Alexa products should adopt the [Security Best Practices for Alexa](https://developer.amazon.com/docs/alexa/alexa-voice-service/security-best-practices.html).

When building Alexa with the SDK, you should also adhere to the [following security principles](https://developer.amazon.com/docs/alexa/avs-device-sdk/overview.html#security-best-practices).

