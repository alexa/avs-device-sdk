#include "KWDProvider/KeywordDetectorProvider.h"

#include <cstdlib>
#include <thread>

#include <pv_porcupine.h>

using namespace alexaClientSDK;
using namespace alexaClientSDK::kwd;

class PorcupineKeywordDetector : public acsdkKWDImplementations::AbstractKeywordDetector {
    public:
    PorcupineKeywordDetector(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        avsCommon::utils::AudioFormat audioFormat,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
        std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers
    ) : acsdkKWDImplementations::AbstractKeywordDetector(keyWordObservers, keyWordDetectorStateObservers)
    {
        std::thread listen([stream, this]() {
            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::cout << "Porcupine Wake Word Engine is ready" << std::endl;
            std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> reader = stream->createReader(avsCommon::avs::AudioInputStream::Reader::Policy::BLOCKING, true);
            avsCommon::avs::AudioInputStream::Index endIndex = avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX;
            avsCommon::avs::AudioInputStream::Index beginIndex = avsCommon::sdkInterfaces::KeyWordObserverInterface::UNSPECIFIED_INDEX;

            const char *access_key = std::getenv("PORCUPINE_ACCESS_KEY");
            if (!access_key) {
                std::cout << "Missing Porcupine access key! please set the environment variable." << std::endl;
                return;
            }
            const char *model_path = std::getenv("PORCUPINE_MODEL_PATH");
            if (!model_path) {
                std::cout << "Missing Porcupine model path! please set the environment variable." << std::endl;
                return;
            }
            const char *keyword_path = std::getenv("PORCUPINE_KEYWORD_PATH");
            if (!keyword_path) {
                std::cout << "Missing Porcupine keyword path! please set the environment variable." << std::endl;
                return;
            }
            const float sensitivity = 0.5f;

            pv_porcupine_t *handle = NULL;
            const pv_status_t initStatus = pv_porcupine_init(
                access_key,
                model_path,
                1,
                &keyword_path,
                &sensitivity,
                &handle);
            if (initStatus != PV_STATUS_SUCCESS) {
                // Insert error handling logic
                std::cout << "error in porcupine init!" << std::endl;
            }

            ssize_t nWords = pv_porcupine_frame_length(); // number of samples per frame
            int16_t *buf = new int16_t[nWords]; // word size must be 2 bytes, so we use int16.
            static constexpr std::chrono::milliseconds timeout{16};
            bool errorOccurred = false;
            ssize_t nWordsRead = 0;

            int32_t keyword_index = -1;
            pv_status_t status;
            while (true) {
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                nWordsRead += this->readFromStream2(
                    reader,
                    stream,
                    buf + nWordsRead,
                    nWords - nWordsRead,
                    timeout,
                    &errorOccurred
                );
                if (errorOccurred) {
                    std::cout << "error occurred" << std::endl;
                }
                if (nWordsRead == nWords) {
                    nWordsRead = 0;

                    status = pv_porcupine_process(handle, buf, &keyword_index);
                    if (status != PV_STATUS_SUCCESS) {
                        // error handling logic
                        std::cout << "pv error" << std::endl;
                        // TODO: on any kind of error, change the  state of the detector to "ERROR" and notify the KeyWordDetectorStateObservers.
                    }
                    if (keyword_index != -1) {
                        // Insert detection event callback
                        std::cout << "keyword detected!" << std::endl;

                        endIndex = reader->tell();
                        this->notifyKeyWordObservers2(stream, "porcupine", beginIndex, endIndex);
                    }
                }
            }
        });
        listen.detach();
    }

    // We should override some methods here.
    void notifyKeyWordObservers2(
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        std::string keyword,
        avsCommon::avs::AudioInputStream::Index beginIndex,
        avsCommon::avs::AudioInputStream::Index endIndex,
        std::shared_ptr<const std::vector<char>> KWDMetadata = nullptr
    ) const {
        std::cout << "notifying observers" << std::endl;
        return notifyKeyWordObservers(stream, keyword, beginIndex, endIndex, KWDMetadata);
    }

    ssize_t readFromStream2(
        std::shared_ptr<avsCommon::avs::AudioInputStream::Reader> reader,
        std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
        void* buf,
        size_t nWords,
        std::chrono::milliseconds timeout,
        bool* errorOccurred
    ) {
        return readFromStream(reader, stream, buf, nWords, timeout, errorOccurred);
    }
};

std::unique_ptr<acsdkKWDImplementations::AbstractKeywordDetector>
createPorcupineKeywordDetector(
    std::shared_ptr<avsCommon::avs::AudioInputStream> stream,
    avsCommon::utils::AudioFormat audioFormat,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordObserverInterface>> keyWordObservers,
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::KeyWordDetectorStateObserverInterface>> keyWordDetectorStateObservers
) {
    std::unique_ptr<PorcupineKeywordDetector> keywordDetector(new PorcupineKeywordDetector(stream, audioFormat, keyWordObservers, keyWordDetectorStateObservers));
    return keywordDetector;
}

KeywordDetectorProvider::KWDRegistration porcupineKWDRegistrar(&createPorcupineKeywordDetector);
