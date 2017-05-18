#
# Setup the Keyword Detector type and compiler options.
#
# To build with a Keyword Detector, run the following command with a keyword detector type of AMAZON_KEY_WORD_DETECTOR,
# KITTAI_KEY_WORD_DETECTOR, or SENSORY_KEY_WORD_DETECTOR:
#     cmake <path-to-source> 
#       -DAMAZON_KEY_WORD_DETECTOR=ON 
#           -DAMAZON_KEY_WORD_DETECTOR_LIB_PATH=<path-to-amazon-lib> 
#           -DAMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-amazon-include-dir>
#       -DKITTAI_KEY_WORD_DETECTOR=ON 
#           -DKITTAI_KEY_WORD_DETECTOR_LIB_PATH=<path-to-kittai-lib>
#           -DKITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-kittai-include-dir>
#       -DSENSORY_KEY_WORD_DETECTOR=ON 
#           -DSENSORY_KEY_WORD_DETECTOR_LIB_PATH=<path-to-sensory-lib>
#           -DSENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR=<path-to-sensory-include-dir>
#

option(AMAZON_KEY_WORD_DETECTOR "Enable Amazon keyword detector." OFF)
option(KITTAI_KEY_WORD_DETECTOR "Enable KittAi keyword detector." OFF)
option(SENSORY_KEY_WORD_DETECTOR "Enable Sensory keyword detector." OFF)

if(NOT AMAZON_KEY_WORD_DETECTOR AND NOT KITTAI_KEY_WORD_DETECTOR AND NOT SENSORY_KEY_WORD_DETECTOR)
    message("No keyword detector type specified, skipping build of keyword detector.")
    return()
endif()

if(AMAZON_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: Amazon")
    if(NOT AMAZON_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of Amazon KeywordDetector!")
    endif()
    if(NOT AMAZON_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of Amazon KeywordDetector!")
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_AMAZON)
endif()

if(KITTAI_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: KittAi")
    if(NOT KITTAI_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of Kitt.ai KeywordDetector!")
    endif()
    if(NOT KITTAI_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of Kitt.ai KeywordDetector!")
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_KITTAI)
endif()

if(SENSORY_KEY_WORD_DETECTOR)
    message("Creating ${PROJECT_NAME} with keyword detector type: Sensory")
    if(NOT SENSORY_KEY_WORD_DETECTOR_LIB_PATH)
        message(FATAL_ERROR "Must pass library path of Sensory KeywordDetector!")
    endif()
    if(NOT SENSORY_KEY_WORD_DETECTOR_INCLUDE_DIR)
        message(FATAL_ERROR "Must pass include dir path of Sensory KeywordDetector!")
    endif()
    add_definitions(-DKWD)
    add_definitions(-DKWD_SENSORY)
endif()
