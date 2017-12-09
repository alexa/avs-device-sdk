#ifndef ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_JSONHEADER_H_
#define ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_JSONHEADER_H_

// Todo ACSDK-443: Move the JSON to text file
/// This is a basic synchronize JSON message which may be used to initiate a connection with AVS.
// clang-format off
static const std::string SYNCHRONIZE_STATE_JSON =
    "{"
        "\"context\":[{"
            "\"header\":{"
                "\"name\":\"SpeechState\","
                "\"namespace\":\"SpeechSynthesizer\""
            "},"
            "\"payload\":{"
                "\"playerActivity\":\"FINISHED\","
                "\"offsetInMilliseconds\":0,"
                "\"token\":\"\""
            "}"
        "}],"
        "\"event\":{"
            "\"header\":{"
                "\"messageId\":\"00000000-0000-0000-0000-000000000000\","
                "\"name\":\"SynchronizeState\","
                "\"namespace\":\"System\""
            "},"
            "\"payload\":{"
            "}"
        "}"
    "}";
// clang-format on

#endif  // ALEXA_CLIENT_SDK_INTEGRATION_INCLUDE_INTEGRATION_JSONHEADER_H_
