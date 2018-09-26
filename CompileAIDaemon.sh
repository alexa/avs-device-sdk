#!/bin/sh

DIR=`pwd`

cd ../../sdk-build

cmake ${DIR} \
    -DACSDK_DEBUG_LOG_ENABLED=ON \
    -DCMAKE_BUILD_TYPE=DEBUG \
    -DOBIGO_AIDAEMON=ON \
    -DSENSORY_KEY_WORD_DETECTOR=OFF \
    -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
    -DPORTAUDIO_LIB_PATH=${DIR}/../../third-party/portaudio/lib/.libs/libportaudio.a \
    -DPORTAUDIO_INCLUDE_DIR=${DIR}/../../third-party/portaudio/include

make VERBOSE=1
