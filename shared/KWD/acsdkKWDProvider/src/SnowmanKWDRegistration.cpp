#include "KWDProvider/KeywordDetectorProvider.h"
#include "KWDProvider/SnowboyWrapper.h"

#include <cstdlib>
#include <thread>
#include <atomic>
#include <unordered_map>
#include <cmath>
#include <memory>

using namespace alexaClientSDK;
using namespace alexaClientSDK::kwd;

static const std::string TAG("SnowmanKeyWordDetector");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) avsCommon::utils::logger::LogEntry(TAG, event)

/** The number of hertz per kilohertz. */
static const size_t HERTZ_PER_KILOHERTZ = 1000;

class SnowmanKeywordDetector : public acsdkKWDImplementations::AbstractKeywordDetector {
public:

    /**
     * Creates a @c SnowmanKeywordDetector.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @return A new @c SnowmanKeywordDetector, or @c nullptr if the operation failed.
     */
    static std::unique_ptr<acsdkKWDImplementations::AbstractKeywordDetector> create(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers) {
        if (!stream) {
            ACSDK_ERROR(LX("createFailed").d("reason", "nullStream"));
            return nullptr;
        }
        // TODO: ACSDK-249 - Investigate cpu usage of converting bytes between endianness and if it's not too much, do it.
        if (isByteswappingRequired(audioFormat)) {
            ACSDK_ERROR(LX("createFailed").d("reason", "endianMismatch"));
            return nullptr;
        }

        /* TODO: defaults, default to alexa.pmdl? */
        const char *resourceFilePath = std::getenv("SNOWMAN_RESOURCE_PATH");
        const char *modelPaths = std::getenv("SNOWMAN_MODEL_PATHS");
        const char *sensitivities = std::getenv("SNOWMAN_SENSITIVITIES") ?: "1";
        if (!modelPaths || !resourceFilePath) {
            ACSDK_ERROR(LX(__func__).m("SNOWMAN_MODEL_PATHS or SNOWMAN_RESOURCE_PATH not set"));
            return nullptr;
        }

        int numModels = 1, numSensitivities = 1;
        for (const char *ptr = modelPaths; ptr; ptr = strchr(ptr + 1, SNOWMAN_DELIMITER))
            numModels++;
        for (const char *ptr = sensitivities; ptr; ptr = strchr(ptr + 1, SNOWMAN_DELIMITER))
            numSensitivities++;

        if (numSensitivities < numModels) {
            /* Note some model files may contain multiple hotword models */
            ACSDK_ERROR(LX(__func__).m("The numbers of entries in SNOWMAN_MODEL_PATHS and SNOWMAN_SENSITIVITIES must match"));
            return nullptr;
        }

        bool applyFrontEnd = !std::getenv("SNOWMAN_NO_FRONTENDS");

        /* TODO: Optimally this should be runtime configurable */
        char *endp;
        float audioGain = strtof(std::getenv("SNOWMAN_MIC_GAIN") ?: "2", &endp);
        if (*endp || audioGain <= 0.0f || !std::isnormal(audioGain)) {
            ACSDK_ERROR(LX(__func__).m("Invalid SNOWMAN_MIC_GAIN value"));
            return nullptr;
        }

        std::unique_ptr<SnowmanKeywordDetector> keywordDetector(new SnowmanKeywordDetector(
            stream,
            audioFormat,
            keyWordObservers,
            keyWordDetectorStateObservers,
            resourceFilePath,
            modelPaths,
            sensitivities,
            audioGain,
            applyFrontEnd));
        if (!keywordDetector->init(audioFormat)) {
            ACSDK_ERROR(LX("createFailed").d("reason", "initDetectorFailed"));
            return nullptr;
        }
        return keywordDetector;
    }

    /** Destructor. */
    ~SnowmanKeywordDetector(void) override {
        m_isShuttingDown = true;
        if (m_detectionThread.joinable()) {
            m_detectionThread.join();
        }
    }

private:
    /** The timeout to use for read calls to the SharedDataStream. */
    const std::chrono::milliseconds TIMEOUT_FOR_READ_CALLS = std::chrono::milliseconds(1000);

    /** The delimiter for Snowman engine constructor parameters */
    static const char SNOWMAN_DELIMITER = ',';

    /** The Snowman compatible audio encoding of LPCM. */
    const avsCommon::utils::AudioFormat::Encoding COMPATIBLE_ENCODING =
        avsCommon::utils::AudioFormat::Encoding::LPCM;

    /** The Snowman compatible endianness which is little endian. */
    const avsCommon::utils::AudioFormat::Endianness COMPATIBLE_ENDIANNESS =
        avsCommon::utils::AudioFormat::Endianness::LITTLE;

    /** Snowman returns -2 if silence is detected. */
    static const int SNOWMAN_SILENCE_DETECTION_RESULT = -2;

    /** Snowman returns -1 if an error occurred. */
    static const int SNOWMAN_ERROR_DETECTION_RESULT = -1;

    /** Snowman returns 0 if no keyword was detected but audio has been heard. */
    static const int SNOWMAN_NO_DETECTION_RESULT = 0;

    /**
     * Constructor.
     *
     * @param stream The stream of audio data. This should be formatted in LPCM encoded with 16 bits per sample and
     * have a sample rate of 16 kHz. Additionally, the data should be in little endian format.
     * @param audioFormat The format of the audio data located within the stream.
     * @param keyWordObservers The observers to notify of keyword detections.
     * @param keyWordDetectorStateObservers The observers to notify of state changes in the engine.
     * @param resourceFilePath The path to the resource file.
     * @param modelPaths A comma-separated list of paths to model files for keywords.
     * @param sensitivities A comma-separated list of sensitivity values for each keyword.
     * @param audioGain This controls whether to increase (>1) or decrease (<1) input volume.
     * @param applyFrontEnd Whether to apply frontend audio processing.
     * @param msToPushPerIteration The amount of data in milliseconds to push to Snowman at a time. Smaller sizes will
     * lead to less delay but more CPU usage. Additionally, larger amounts of data fed into the engine per iteration
     * might lead longer delays before receiving keyword detection events. This has been defaulted to 200 milliseconds
     * as it is a good trade off between CPU usage and recognition delay.
     * @see https://github.com/seasalt-ai/snowboy for more information regarding @c audioGain and @c applyFrontEnd.
     */
    SnowmanKeywordDetector(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>>
            keyWordDetectorStateObservers,
        const char *resourceFilePath,
        const char *modelPaths,
        const char *sensitivities,
        float audioGain,
        bool applyFrontEnd,
        std::chrono::milliseconds msToPushPerIteration = std::chrono::milliseconds(200)) :
            acsdkKWDImplementations::AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers),
            m_stream{stream},
            m_maxSamplesPerPush{
                static_cast<size_t>((audioFormat.sampleRateHz / HERTZ_PER_KILOHERTZ) * msToPushPerIteration.count())} {
        m_engine = std::unique_ptr<SnowboyWrapper>(new SnowboyWrapper(resourceFilePath, modelPaths));
        m_engine->SetSensitivity(sensitivities);
        m_engine->SetAudioGain(audioGain);
        m_engine->ApplyFrontend(applyFrontEnd);

        const char *start = modelPaths, *end;
        for (unsigned int i = 1; true; start = end + 1) {
            end = strchrnul(start, SNOWMAN_DELIMITER);
            m_detectionResultsToKeyWords[i++] = std::string(start, end - start);
            if (!*end)
                break;
        }
    }

    /**
     * Initializes the stream reader and kicks off a thread to read data from the stream. This function should only be
     * called once with each new @c SnowmanKeywordDetector.
     *
     * @param audioFormat The format of the audio data located within the stream.
     * @return @c true if the engine was initialized properly and @c false otherwise.
     */
    bool init(avsCommon::utils::AudioFormat audioFormat) {
        if (!isAudioFormatCompatible(audioFormat)) {
            return false;
        }
        m_streamReader = m_stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::BLOCKING, true);
        if (!m_streamReader) {
            ACSDK_ERROR(LX("initFailed").d("reason", "createStreamReaderFailed"));
            return false;
        }
        m_isShuttingDown = false;
        m_detectionThread = std::thread(&SnowmanKeywordDetector::detectionLoop, this);
        return true;
    }

    /**
     * Checks to see if an @c avsCommon::utils::AudioFormat is compatible with Snowman.
     *
     * @param audioFormat The audio format to check.
     * @return @c true if the audio format is compatible and @c false otherwise.
     */
    bool isAudioFormatCompatible(avsCommon::utils::AudioFormat audioFormat) {
        if (audioFormat.numChannels != static_cast<unsigned int>(m_engine->NumChannels())) {
            ACSDK_ERROR(LX("isAudioFormatCompatibleFailed")
                            .d("reason", "numChannelsMismatch")
                            .d("snowmanNumChannels", m_engine->NumChannels())
                            .d("numChannels", audioFormat.numChannels));
            return false;
        }
        if (audioFormat.sampleRateHz != static_cast<unsigned int>(m_engine->SampleRate())) {
            ACSDK_ERROR(LX("isAudioFormatCompatibleFailed")
                            .d("reason", "sampleRateMismatch")
                            .d("snowmanSampleRate", m_engine->SampleRate())
                            .d("sampleRate", audioFormat.sampleRateHz));
            return false;
        }
        if (audioFormat.sampleSizeInBits != static_cast<unsigned int>(m_engine->BitsPerSample())) {
            ACSDK_ERROR(LX("isAudioFormatCompatibleFailed")
                            .d("reason", "sampleSizeInBitsMismatch")
                            .d("snowmanSampleSizeInBits", m_engine->BitsPerSample())
                            .d("sampleSizeInBits", audioFormat.sampleSizeInBits));
            return false;
        }
        if (audioFormat.endianness != COMPATIBLE_ENDIANNESS) {
            ACSDK_ERROR(LX("isAudioFormatCompatibleFailed")
                            .d("reason", "endiannessMismatch")
                            .d("snowmanEndianness", COMPATIBLE_ENDIANNESS)
                            .d("endianness", audioFormat.endianness));
            return false;
        }
        if (audioFormat.encoding != COMPATIBLE_ENCODING) {
            ACSDK_ERROR(LX("isAudioFormatCompatibleFailed")
                            .d("reason", "encodingMismatch")
                            .d("snowmanEncoding", COMPATIBLE_ENCODING)
                            .d("encoding", audioFormat.encoding));
            return false;
        }
        return true;
    }

    /** The main function that reads data and feeds it into the engine. */
    void detectionLoop() {
        notifyKeyWordDetectorStateObservers(avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
        int16_t audioDataToPush[m_maxSamplesPerPush];
        ACSDK_INFO(LX(__func__).m("Snowman Wake Word Engine is ready"));
        while (!m_isShuttingDown) {
            bool didErrorOccur;
            auto wordsRead = readFromStream(
                m_streamReader, m_stream, audioDataToPush, m_maxSamplesPerPush, TIMEOUT_FOR_READ_CALLS, &didErrorOccur);
            if (didErrorOccur) {
                break;
            } else if (wordsRead > 0) {
                /* Words were successfully read. */
                notifyKeyWordDetectorStateObservers(avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ACTIVE);
		/* TODO: catch exceptions */
                int detectionResult = m_engine->RunDetection(audioDataToPush, wordsRead);
                if (detectionResult > 0) {
                    /* > 0 indicates a keyword was found */
                    if (m_detectionResultsToKeyWords.find(detectionResult) == m_detectionResultsToKeyWords.end()) {
                        ACSDK_ERROR(LX("detectionLoopFailed").d("reason", "retrievingDetectedKeyWordFailed"));
                        notifyKeyWordDetectorStateObservers(
                            avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                        break;
                    } else {
                        ACSDK_INFO(LX(__func__).m("Keyword detected!"));
                        /*
                         * TODO: obtain beginIndex so that the keyword itself can be sent to the Alexa
                         * Confirmation API to reduce false positives.
                         */
                        notifyKeyWordObservers(
                            m_stream,
                            m_detectionResultsToKeyWords[detectionResult],
                            avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX,
                            m_streamReader->tell());
                    }
                } else {
                    switch (detectionResult) {
                        case SNOWMAN_ERROR_DETECTION_RESULT:
                            ACSDK_ERROR(LX("detectionLoopFailed").d("reason", "SnowmanEngineError"));
                            notifyKeyWordDetectorStateObservers(
                                avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                            didErrorOccur = true;
                            break;
                        case SNOWMAN_SILENCE_DETECTION_RESULT:
                            break;
                        case SNOWMAN_NO_DETECTION_RESULT:
                            break;
                        default:
                            ACSDK_ERROR(LX("detectionLoopEnded")
                                            .d("reason", "unexpectedDetectionResult")
                                            .d("detectionResult", detectionResult));
                            notifyKeyWordDetectorStateObservers(
                                avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface::KeyWordDetectorState::ERROR);
                            didErrorOccur = true;
                            break;
                    }
                    if (didErrorOccur) {
                        break;
                    }
                }
            }
        }
        m_streamReader->close();
    }

    /// Indicates whether the internal main loop should keep running.
    std::atomic<bool> m_isShuttingDown;

    /**
     * The Snowman engine emits a number to indicate which keyword was detected. The number corresponds to the model
     * passed in. Since we can pass in multiple models, it's up to us to keep track which return value corrresponds to
     * which keyword. This map maps each keyword to a return value.
     */
    std::unordered_map<unsigned int, std::string> m_detectionResultsToKeyWords;

    /// The stream of audio data.
    const std::shared_ptr<avsCommon::avs::AudioInputStream> m_stream;

    /// The reader that will be used to read audio data from the stream.
    std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> m_streamReader;

    /// Internal thread that reads audio from the buffer and feeds it to the Snowman engine.
    std::thread m_detectionThread;

    /// The Snowman engine instance.
    std::unique_ptr<SnowboyWrapper> m_engine;

    /**
     * The max number of samples to push into the underlying engine per iteration. This will be determined based on the
     * sampling rate of the audio data passed in.
     */
    const size_t m_maxSamplesPerPush;
};

KeywordDetectorProvider::KWDRegistration snowmanKWDRegistrar(&SnowmanKeywordDetector::create);
