/**
 * AudioInputProcessor.cpp
 *
 * Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <sstream>

#include <AVSCommon/JSON/JSONUtils.h>
#include <AVSCommon/SDKInterfaces/FocusState.h>
#include <AVSCommon/AVS/Message.h>
#include <AVSUtils/Logging/Logger.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>

#include "AIP/AudioInputProcessor.h"
#include "AIP/MessageRequest.h"
#include "AIP/ObserverInterface.h"

namespace alexaClientSDK {
namespace capabilityAgent {
namespace aip {

/// The name of the @c FocusManager channel used by @c AudioInputProvider.
static const std::string CHANNEL_NAME = "Dialog";

/// The activityId string used with @c FocusManager by @c AudioInputProvider.
static const std::string ACTIVITY_ID = "SpeechRecognizer.Recognize";

/// The namespace for this capability agent.
static const std::string NAMESPACE = "SpeechRecognizer";

/// The StopCapture directive signature.
static const avsCommon::avs::NamespaceAndName STOP_CAPTURE{NAMESPACE, "StopCapture"};

/// The ExpectSpeech directive signature.
static const avsCommon::avs::NamespaceAndName EXPECT_SPEECH{NAMESPACE, "ExpectSpeech"};

/// The SpeechRecognizer context state signature.
static const avsCommon::avs::NamespaceAndName RECOGNIZER_STATE{NAMESPACE, "RecognizerState"};

std::string AudioInputProcessor::stateToString(State state) {
    switch (state) {
        case State::IDLE:
            return "IDLE";
        case State::EXPECTING_SPEECH:
            return "EXPECTING_SPEECH";
        case State::RECOGNIZING:
            return "RECOGNIZING";
        case State::BUSY:
            return "BUSY";
    }
    avsUtils::Logger::log(
            "stateToString failed: unknown state " +
            std::to_string(static_cast<int>(state)) + ".");
    return "Unknown State";
}

std::shared_ptr<AudioInputProcessor> AudioInputProcessor::create(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender,
        AudioProvider defaultAudioProvider) {
    if (!directiveSequencer) {
        avsUtils::Logger::log("create failed: Invalid nullptr directiveSequencer parameter.");
        return nullptr;
    } else if (!messageSender) {
        avsUtils::Logger::log("create failed: Invalid nullptr messageSender parameter.");
        return nullptr;
    } else if (!contextManager) {
        avsUtils::Logger::log("create failed: Invalid nullptr contextManager parameter.");
        return nullptr;
    } else if (!focusManager) {
        avsUtils::Logger::log("create failed: Invalid nullptr focusManager parameter.");
        return nullptr;
    } else if (!exceptionSender) {
        avsUtils::Logger::log("create failed: Invalid nullptr exceptionSender parameter.");
        return nullptr;
    }

    auto aip = std::shared_ptr<AudioInputProcessor>(new AudioInputProcessor(
            directiveSequencer,
            messageSender,
            contextManager,
            focusManager,
            exceptionSender,
            defaultAudioProvider));

    contextManager->setStateProvider(RECOGNIZER_STATE, aip);

    return aip;
}

avsCommon::avs::DirectiveHandlerConfiguration AudioInputProcessor::getConfiguration() {
    avsCommon::avs::HandlerAndPolicy handlerAndPolicy{
            shared_from_this(),
            avsCommon::avs::BlockingPolicy::NON_BLOCKING};
    avsCommon::avs::DirectiveHandlerConfiguration configuration;
    configuration[STOP_CAPTURE] = handlerAndPolicy;
    configuration[EXPECT_SPEECH] = handlerAndPolicy;
    return configuration;
}

void AudioInputProcessor::addObserver(std::shared_ptr<ObserverInterface> observer) {
    if (nullptr == observer) {
        avsUtils::Logger::log("addObserver failed: Invalid nullptr observer parameter.");
        return;
    }
    m_executor.submit([this, observer] () { m_observers.insert(observer); });
}

void AudioInputProcessor::removeObserver(std::shared_ptr<ObserverInterface> observer) {
    if (nullptr == observer) {
        avsUtils::Logger::log("removeObserver failed: Invalid nullptr observer parameter.");
        return;
    }
    m_executor.submit([this, observer] () { m_observers.erase(observer); });
}

std::future<bool> AudioInputProcessor::recognize(
        AudioProvider audioProvider,
        Initiator initiator,
        avsCommon::sdkInterfaces::AudioInputStream::Index begin,
        avsCommon::sdkInterfaces::AudioInputStream::Index keywordEnd,
        std::string keyword) {
    return m_executor.submit(
        [this, audioProvider, initiator, begin, keywordEnd, keyword] () {
            return executeRecognize(audioProvider, initiatorToString(initiator), begin, keywordEnd, keyword);
        }
    );
}

std::future<bool> AudioInputProcessor::stopCapture() {
    return m_executor.submit(
        [this] () {
            return executeStopCapture();
        }
    );
}

std::future<void> AudioInputProcessor::resetState() {
    return m_executor.submit(
        [this] () {
            executeResetState();
        }
    );
}

void AudioInputProcessor::provideState(unsigned int stateRequestToken) {
    m_executor.submit(
        [this, stateRequestToken] () {
            executeProvideState(true, stateRequestToken);
        }
    );
}

void AudioInputProcessor::onContextAvailable(const std::string& jsonContext) {
    m_executor.submit(
        [this, jsonContext] () {
            executeOnContextAvailable(jsonContext);
        }
    );
}

void AudioInputProcessor::onContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    m_executor.submit(
        [this, error] () {
            executeOnContextFailure(error);
        }
    );
}

void AudioInputProcessor::handleDirectiveImmediately(const DirectiveAndResultInterface& directiveAndResult) {
    handleDirective(directiveAndResult);
}

void AudioInputProcessor::preHandleDirective(const DirectiveAndResultInterface& directiveAndResult) {
}

void AudioInputProcessor::handleDirective(const DirectiveAndResultInterface& directiveAndResult) {
    if (directiveAndResult.directive->getName() == STOP_CAPTURE.name) {
        handleStopCaptureDirective(directiveAndResult);
    } else if (directiveAndResult.directive->getName() == EXPECT_SPEECH.name) {
        handleExpectSpeechDirective(directiveAndResult);
    } else {
        std::string errorMessage =
                "unexpected directive " +
                directiveAndResult.directive->getNamespace() + ":" + directiveAndResult.directive->getName();
        m_exceptionSender->sendExceptionEncountered(
            directiveAndResult.directive->getUnparsedDirective(),
            avsCommon::ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED,
            errorMessage);
        if (directiveAndResult.resultInterface) {
            directiveAndResult.resultInterface->setFailed(errorMessage);
        }
        avsUtils::Logger::log(std::string("handleDirective failed: ") + errorMessage);
    }
}

void AudioInputProcessor::cancelDirective(const DirectiveAndResultInterface& directiveAndResult) {
    resetState();
    removeDirective(directiveAndResult.directive->getMessageId());
}

void AudioInputProcessor::onDeregistered() {
    resetState();
}

void AudioInputProcessor::onFocusChanged(avsCommon::sdkInterfaces::FocusState newFocus) {
    m_executor.submit(
        [this, newFocus] () {
            executeOnFocusChanged(newFocus);
        }
    );
}

AudioInputProcessor::AudioInputProcessor(
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender,
        std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<avsCommon::sdkInterfaces::FocusManagerInterface> focusManager,
        std::shared_ptr<avsCommon::ExceptionEncounteredSenderInterface> exceptionSender,
        AudioProvider defaultAudioProvider) :
        CapabilityAgent{NAMESPACE},
        m_directiveSequencer{directiveSequencer},
        m_messageSender{messageSender},
        m_contextManager{contextManager},
        m_focusManager{focusManager},
        m_exceptionSender{exceptionSender},
        m_defaultAudioProvider{defaultAudioProvider},
        m_lastAudioProvider{AudioProvider::null()},
        m_state{State::IDLE},
        m_focusState{avsCommon::sdkInterfaces::FocusState::NONE} {
}

std::future<bool> AudioInputProcessor::expectSpeechTimedOut() {
    return m_executor.submit(
        [this] () {
            return executeExpectSpeechTimedOut();
        }
    );
}

void AudioInputProcessor::handleStopCaptureDirective(const DirectiveAndResultInterface& directiveAndResult) {
    m_executor.submit(
        [this, directiveAndResult] () {
            bool stopImmediately = true;
            executeStopCapture(stopImmediately, directiveAndResult.directive, directiveAndResult.resultInterface);
        }
    );
}

void AudioInputProcessor::handleExpectSpeechDirective(const DirectiveAndResultInterface& directiveAndResult) {
    int64_t timeout;
    bool found = avsCommon::jsonUtils::lookupInt64Value(
            directiveAndResult.directive->getPayload(),
            "timeoutInMilliseconds",
            &timeout);

    if (!found) {
        static const char * errorMessage = "missing/invalid timeoutInMilliseconds";
        m_exceptionSender->sendExceptionEncountered(
                directiveAndResult.directive->getUnparsedDirective(),
                avsCommon::ExceptionErrorType::UNSUPPORTED_OPERATION,
                errorMessage);
        if (directiveAndResult.resultInterface) {
            directiveAndResult.resultInterface->setFailed(errorMessage);
        }
        avsUtils::Logger::log(std::string("handleExpectSpeechDirective failed: ") + errorMessage);
        return;
    }

    std::string initiator;
    avsCommon::jsonUtils::lookupStringValue(directiveAndResult.directive->getPayload(), "initiator", &initiator);
        
    m_executor.submit(
        [this, timeout, initiator, directiveAndResult] () {
            executeExpectSpeech(
                std::chrono::milliseconds{timeout},
                initiator,
                directiveAndResult.directive,
                directiveAndResult.resultInterface);
        }
    );
}

bool AudioInputProcessor::executeRecognize(
        AudioProvider provider,
        const std::string& initiatorType,
        avsCommon::sdkInterfaces::AudioInputStream::Index begin,
        avsCommon::sdkInterfaces::AudioInputStream::Index end,
        const std::string& keyword) {
    if (nullptr == provider.stream) {
        avsUtils::Logger::log("executeRecognize failed: provider.stream must not be nullptr.");
        return false;
    }

    if (provider.format.encoding != avsCommon::AudioFormat::Encoding::LPCM) {
        avsUtils::Logger::log("executeRecognize failed: unsupported audio format encoding.");
        return false;
    }
    else if (provider.format.endianness != avsCommon::AudioFormat::Endianness::LITTLE) {
        avsUtils::Logger::log("executeRecognize failed: unsupported audio format endianness.");
        return false;
    }
    else if (provider.format.sampleSizeInBits != 16) {
        avsUtils::Logger::log("executeRecognize failed: unsupported audio format sample size.");
        return false;
    }
    else if (provider.format.sampleRateHz != 16000) {
        avsUtils::Logger::log("executeRecognize failed: unsupported audio format sample rate.");
        return false;
    }
    else if (provider.format.numChannels != 1) {
        avsUtils::Logger::log("executeRecognize failed: unsupported audio format number of channels.");
        return false;
    }

    // Make sure we have a keyword if this is a wakeword initiator.
    bool initiatorIsWakeword = (initiatorType == initiatorToString(Initiator::WAKEWORD));
    if (initiatorIsWakeword && keyword.empty()) {
        avsUtils::Logger::log("executeRecognize failed: WAKEWORD initiator requires a keyword.");
        return false;
    }

    // Do any cleanup needed if this Recognize is barging in.
    switch (m_state) {
        case State::IDLE:
        case State::EXPECTING_SPEECH:
            break;
        case State::RECOGNIZING:
            // Barge-in is only permitted if the audio providers have compatible policies.
            if (!m_lastAudioProvider.canBeOverridden) {
                avsUtils::Logger::log("executeRecognize failed: Active audio provider can not be overridden.");
                return false;
            }
            if (!provider.canOverride) {
                avsUtils::Logger::log("executeRecognize failed: New audio provider can not override.");
                return false;
            }
            // Close out the current Recognize so that a new one can start.
            executeResetState();
            break;
        case State::BUSY:
            avsUtils::Logger::log("executeRecognize failed: Barge-in is not permitted while busy.");
            return false;
    }
    setState(State::RECOGNIZING);

    //  Start assembling the context; we'll service the callback after assembling our Recognize event.
    m_contextManager->getContext(shared_from_this());

    // Stop the ExpectSpeech timer so we don't get a timeout.
    m_expectingSpeechTimer.stop();

    // 500ms preroll.
    avsCommon::sdkInterfaces::AudioInputStream::Index preroll = provider.format.sampleRateHz / 2;

    // Check if we have everything we need to enable false wakeword detection.
    // TODO: Consider relaxing the hard requirement for a full 500ms preroll - ACSDK-276.
    bool falseWakewordDetection = 
            initiatorIsWakeword &&
            begin != INVALID_INDEX &&
            begin >= preroll &&
            end != INVALID_INDEX;

    // Update state if we're changing wakewords.
    if (initiatorIsWakeword && m_wakeword != keyword) {
            m_wakeword = keyword;
            executeProvideState();
    }

    // If we will be enabling false wakeword detection, add preroll and build the initiator payload.
    std::ostringstream initiatorPayloadJson;
    // TODO: Consider reworking this code to use RapidJSON - ACSDK-279.
    if (falseWakewordDetection) {
        begin -= preroll;
        initiatorPayloadJson
                << R"("wakeWordIndices":{)"
                       R"("startIndexInSamples":)" << preroll << R"(,)"
                       R"("endIndexInSamples":)" << preroll + end - begin
                << R"(})";
    }

    // If we have an initiatorType, build the initiator json.
    std::ostringstream initiatorJson;
    if (!initiatorType.empty()) {
        initiatorJson
                << R"(,"initiator":{)"
                       R"("type":")" << initiatorType << R"(",)"
                       R"("payload":{)"
                <<         initiatorPayloadJson.str()
                <<     R"(})"
                   R"(})";
    }

    // Set up an attachment reader for the event.
    // TODO: There is a small delay from when the original recognize() or expectSpeech() call occurred until we
    // reach this point, and the audio during that delay is discarded.  Moving the creation of the reader out to
    // the recognize() and expectSpeech() calls and then passing it down through the executor would eliminate this
    // lost audio (ACSDK-253).
    avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeIndex offset = 0;
    avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference reference =
            avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference::BEFORE_WRITER;
    if (INVALID_INDEX != begin) {
        offset = begin;
        reference = avsCommon::avs::attachment::InProcessAttachmentReader::SDSTypeReader::Reference::ABSOLUTE;
    }
    m_reader = avsCommon::avs::attachment::InProcessAttachmentReader::create(
            avsCommon::avs::attachment::AttachmentReader::Policy::NON_BLOCKING,
            provider.stream,
            offset,
            reference);
    if (!m_reader) {
        avsUtils::Logger::log("executeRecognize failed: Failed to create attachment reader.");
        executeResetState();
        return false;
    }

    // Record provider as the last-used AudioProvider so it can be used in the event of an ExpectSpeech directive.
    m_lastAudioProvider = provider;

    // Assemble the event payload.
    std::ostringstream payload;
    payload << R"({)"
                   R"("profile":")" << provider.profile << R"(",)"
            <<     R"("format":"AUDIO_L16_RATE_16000_CHANNELS_1")"
            <<     initiatorJson.str()
            << R"(})";
    m_payload = payload.str();

    // We can't assemble the MessageRequest until we receive the context.
    m_request.reset();

    return true;
}

void AudioInputProcessor::executeOnContextAvailable(const std::string jsonContext) {
    // Should already be RECOGNIZING if we get here.
    if (m_state != State::RECOGNIZING) {
        avsUtils::Logger::log("onContextAvailable failed: not permitted in " + stateToString(m_state) + "state.");
        return;
    }

    // Should already have a reader and payload.
    if (!m_reader || m_payload.empty()) {
        avsUtils::Logger::log("onContextAvailable failed: Missing reader or payload.");
        executeResetState();
        return;
    }

    // Start acquiring the channel right away; we'll service the callback after assembling our Recognize event.
    if (!m_focusManager->acquireChannel(CHANNEL_NAME, shared_from_this(), ACTIVITY_ID)) {
        avsUtils::Logger::log("onContextAvailable failed: Unable to acquire channel.");
        executeResetState();
        return;
    }

    // Assemble the MessageRequest.  It will be sent by executeOnFocusChanged when we acquire the channel.
    auto dialogRequestId = avsCommon::utils::uuidGeneration::generateUUID();
    m_directiveSequencer->setDialogRequestId(dialogRequestId);
    auto json = buildJsonEventString(
            "Recognize",
            dialogRequestId,
            m_payload,
            jsonContext);
    m_request = std::make_shared<MessageRequest>(shared_from_this(), json, m_reader);
}

void AudioInputProcessor::executeOnContextFailure(const avsCommon::sdkInterfaces::ContextRequestError error) {
    avsUtils::Logger::log("executeOnContextFailure: Context failure.");
    executeResetState();
}

void AudioInputProcessor::executeOnFocusChanged(avsCommon::sdkInterfaces::FocusState newFocus) {
    // If we're losing focus, stop using the channel.
    if (newFocus != avsCommon::sdkInterfaces::FocusState::FOREGROUND) {
        avsUtils::Logger::log("executeOnFocusChanged: Lost focus.");
        executeResetState();
        return;
    }

    // Note new focus state.
    m_focusState = newFocus;

    // We're not losing the channel (m_focusState == avsCommon::sdkInterfaces::FocusState::FOREGROUND).  For all
    // states except RECOGNIZING, there's nothing more to do here.
    if (m_state != State::RECOGNIZING) {
        return;
    }

    // For a focus change to FOREGROUND in the Recognizing state, we may have a message queued up to send.  If we do,
    // we can safely send it now.
    if (m_request) {
        m_messageSender->sendMessage(m_request);
        m_request.reset();
    }
}

bool AudioInputProcessor::executeStopCapture(
        bool stopImmediately,
        std::shared_ptr<avsCommon::AVSDirective> directive,
        std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) {
    if (m_state != State::RECOGNIZING) {
        static const char * errorMessage = "StopCapture only allowed in RECOGNIZING state.";
        if (result) {
            result->setFailed(errorMessage);
        }
        if (directive) {
            removeDirective(directive->getMessageId());
        }
        avsUtils::Logger::log(std::string("executeStopCapture failed: ") + errorMessage);
        return false;
    }

    if (stopImmediately) {
        m_reader->close(avsCommon::avs::attachment::AttachmentReader::ClosePoint::IMMEDIATELY);
    } else {
        m_reader->close(avsCommon::avs::attachment::AttachmentReader::ClosePoint::AFTER_DRAINING_CURRENT_BUFFER);
    }

    m_reader.reset();
    setState(State::BUSY);

    //TODO: Need an additional API (maybe just resetState()) which is called by a UX component, and changes the state
    //  from BUSY back to IDLE - ACSDK-282
    m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    m_focusState = avsCommon::sdkInterfaces::FocusState::NONE;
    setState(State::IDLE);

    if (result) {
        result->setCompleted();
    }
    if (directive) {
        removeDirective(directive->getMessageId());
    }
    return true;
}

void AudioInputProcessor::executeResetState() {
    // Irrespective of current state, clean up and go back to idle.
    m_expectingSpeechTimer.stop();
    if (m_reader) {
        m_reader->close();
    }
    m_reader.reset();
    m_request.reset();
    if (m_focusState != avsCommon::sdkInterfaces::FocusState::NONE) {
        m_focusManager->releaseChannel(CHANNEL_NAME, shared_from_this());
    }
    m_focusState = avsCommon::sdkInterfaces::FocusState::NONE;
    setState(State::IDLE);
}

bool AudioInputProcessor::executeExpectSpeech(
    std::chrono::milliseconds timeout,
    std::string initiator,
    std::shared_ptr<avsCommon::AVSDirective> directive,
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) {

    if (m_state != State::IDLE && m_state != State::BUSY) {
        static const char * errorMessage = "ExpectSpeech only allowed in IDLE or BUSY state.";
        if (result) {
            result->setFailed(errorMessage);
        }
        removeDirective(directive->getMessageId());
        avsUtils::Logger::log(std::string("executeExpectSpeech failed: ") + errorMessage);
        return false;
    }

    // Start the ExpectSpeech timer.
    if (!m_expectingSpeechTimer.start(timeout, std::bind(&AudioInputProcessor::expectSpeechTimedOut, this)).valid()) {
        avsUtils::Logger::log("executeExpectSpeech failed: Unable to start timer.");
    }
    setState(State::EXPECTING_SPEECH);
    if (result) {
        result->setCompleted();
    }
    removeDirective(directive->getMessageId());

    // If possible, start recognizing immediately.
    if (m_lastAudioProvider && m_lastAudioProvider.alwaysReadable) {
        return executeRecognize(m_lastAudioProvider, initiator);
    } else if (m_defaultAudioProvider && m_defaultAudioProvider.alwaysReadable) {
        return executeRecognize(m_defaultAudioProvider, initiator);
    }

    return true;
}

bool AudioInputProcessor::executeExpectSpeechTimedOut() {
    if (m_state != State::EXPECTING_SPEECH) {
        avsUtils::Logger::log("expectSpeechTimedOut failed: Only allowed in EXPECTING_SPEECH state.");
        return false;
    }

    auto json = buildJsonEventString("ExpectSpeechTimedOut");
    auto request = std::make_shared<MessageRequest>(shared_from_this(), json, m_reader);
    m_messageSender->sendMessage(request);
    setState(State::IDLE);
    avsUtils::Logger::log("executeExpectSpeech failed: Timed out.");
    return true;
}

void AudioInputProcessor::executeProvideState(bool sendToken, unsigned int stateRequestToken) {
    std::ostringstream context;
    context << R"({"wakeword" : ")" << m_wakeword << R"("})";
    if (sendToken) {
        m_contextManager->setState(
                RECOGNIZER_STATE,
                context.str(),
                avsCommon::sdkInterfaces::StateRefreshPolicy::NEVER,
                stateRequestToken);
    } else {
        m_contextManager->setState(
                RECOGNIZER_STATE,
                context.str(),
                avsCommon::sdkInterfaces::StateRefreshPolicy::NEVER);
    }
}

void AudioInputProcessor::setState(State state) {
    m_state = state;
    for (auto observer: m_observers) {
        observer->onStateChanged(m_state);
    }
}

} // namespace aip
} // namespace capabilityagent
} // namespace alexaClientSDK
