#
# Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
#
#  http://aws.amazon.com/apache2.0
#
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
#

if [ -z "$PLATFORM" ]; then
	echo "You should run the main script."
	exit 1
fi

LIB_SUFFIX="dll.a"
START_SCRIPT="$INSTALL_BASE/startsample.bat"
CMAKE_PLATFORM_SPECIFIC=(-G 'MSYS Makefiles' -Dgtest_disable_pthreads=ON \
    -DGSTREAMER_MEDIA_PLAYER=ON -DPORTAUDIO=ON \
    -DPORTAUDIO_LIB_PATH="$THIRD_PARTY_PATH/portaudio/lib/.libs/libportaudio.$LIB_SUFFIX" \
    -DPORTAUDIO_INCLUDE_DIR="$THIRD_PARTY_PATH/portaudio/include")
CONFIG_DB_PATH=`cygpath.exe -m $DB_PATH`

GSTREAMER_AUDIO_SINK="directsoundsink"

install_dependencies() {

  PACMAN_ARGS="--noconfirm --needed"

  # Build tools and make (mingw32-make fails building portAudio)
  pacman -S ${PACMAN_ARGS} git mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake msys/tar msys/make

  # required by the SDK
  pacman -S ${PACMAN_ARGS} mingw-w64-x86_64-sqlite3

  # MediaPlayer reference Implementation
  pacman -S ${PACMAN_ARGS} mingw64/mingw-w64-x86_64-gstreamer

  # MediaPlayer reference Implementation
  pacman -S ${PACMAN_ARGS} mingw64/mingw-w64-x86_64-gst-plugins-good mingw64/mingw-w64-x86_64-gst-plugins-base mingw64/mingw-w64-x86_64-gst-plugins-ugly

  # Music providers requirements
  pacman -S ${PACMAN_ARGS} mingw64/mingw-w64-x86_64-gst-plugins-bad mingw64/mingw-w64-x86_64-faad2

  # Install Portaudio
  pacman -S ${PACMAN_ARGS} mingw64/mingw-w64-x86_64-portaudio

}

run_os_specifics() {
  build_port_audio
}

generate_start_script() {
  cat << EOF > "$START_SCRIPT"
  set path=`cygpath.exe -m $MSYSTEM_PREFIX/bin`;%path%;
  cd `cygpath.exe -m $BUILD_PATH/bin`
  SampleApp.exe `cygpath.exe -m $OUTPUT_CONFIG_FILE` DEBUG9
  pause
EOF
}

generate_test_script() {
  cat << EOF > "${TEST_SCRIPT}"
  echo
  echo "==============> BUILDING Tests =============="
  echo

  cd ${BUILD_PATH}
  make all -j2
  make test
EOF
}
