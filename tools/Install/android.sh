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

# Defaults
# TODO: Extract API / Architecture:
# adb shell cat /system/build.prop
# ro.build.version.sdk
# ro.product.cpu.abi
# TODO: Remove automake dependency.

# The android API level.
API_LEVEL=${API_LEVEL:?"Variable hasn't been set."}

# System where the sample app shall run.
TARGET_SYSTEM=${TARGET_SYSTEM:?"Variable hasn't been set."}

# Path on the device where the sdk will be installed.
DEVICE_INSTALL_PATH=${DEVICE_INSTALL_PATH:?"Variable hasn't been set."}
DEVICE_BIN_PATH=${DEVICE_INSTALL_PATH}/bin

# Build type. By default, use Debug build.
BUILD_TYPE=${BUILD_TYPE:?"Variable hasn't been set."}

# Path where android keeps its CA certificates
ANDROID_CA_PATH=${ANDROID_CA_PATH:-"/system/etc/security/cacerts/"}

# Path where we are going to generate CA bundle in the PEM format for curl
CURL_CA_BUNDLE=${CURL_CA_BUNDLE:-"${DEVICE_INSTALL_PATH}/cacert.pem"}

# Path to the start script that may be used to start the sample app.
START_SCRIPT="${INSTALL_BASE}/startsample.sh"

# Path to the sdk sqllite databases.
CONFIG_DB_PATH="${DEVICE_INSTALL_PATH}/databases"

NDK_PACKAGE=${NDK_PACKAGE:-"android-ndk-r16"}

# Path to run ADB. If not set, it will assume that adb is available on PATH.
ADB=${ADB:-"adb"}

# CMake parameters used to build the SDK.
set_cmake_var() {
  CMAKE_PLATFORM_SPECIFIC=(-DANDROID="ON" \
        -DANDROID_LOGGER="OFF" \
        -DANDROID_DEVICE_INSTALL_PREFIX="${DEVICE_INSTALL_PATH}" \
        -DANDROID_NDK="${ANDROID_NDK}" \
        -DANDROID_ABI="${ANDROID_ABI}" \
        -DANDROID_ALLOW_UNDEFINED_SYMBOLS="${ANDROID_ALLOW_UNDEFINED_SYMBOLS}" \
        -DANDROID_ARM_MODE="${ANDROID_ARM_MODE}" \
        -DANDROID_ARM_NEON="${ANDROID_ARM_NEON}" \
        -DANDROID_CPP_FEATURES="${ANDROID_CPP_FEATURES}" \
        -DANDROID_DISABLE_FORMAT_STRING_CHECKS="${ANDROID_DISABLE_FORMAT_STRING_CHECKS}" \
        -DANDROID_DISABLE_NO_EXECUTE="${ANDROID_DISABLE_NO_EXECUTE}" \
        -DANDROID_DISABLE_RELRO="${ANDROID_DISABLE_RELRO}" \
        -DANDROID_PIE="${ANDROID_PIE}" \
        -DANDROID_PLATFORM="${ANDROID_PLATFORM}" \
        -DANDROID_STL="${ANDROID_STL}" \
        -DANDROID_TOOLCHAIN="${ANDROID_TOOLCHAIN}" \
        -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
        -DCMAKE_CROSSCOMPILING="ON" \
        -DCMAKE_EXPORT_COMPILE_COMMANDS="ON" \
        -DCMAKE_INSTALL_PREFIX="${INSTALL_TARGET}" \
        -DCMAKE_PREFIX_PATH="${INSTALL_TARGET}" \
        -DCMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake" \
        -DCURL_INCLUDE_DIR="${CURL_INCLUDE_DIR}" \
        -DCURL_LIBRARY="${CURL_LIBRARY}" \
        -DLOCAL_MODULE_RELATIVE_PATH="asan" \
        -DFFMPEG_LIB_PATH=${INSTALL_TARGET}/lib \
        -DFFMPEG_INCLUDE_DIR=${INSTALL_TARGET}/include)
        #-DBUILD_SHARED_LIBS="ON" \
}

run_os_specifics() {
  set_cmake_var
}

make_install() {
  pushd "${BUILD_PATH}"
  removeSymbolsFromRelObjFiles .
  cp "${CXX_SHARED_LIBRARY}" "${INSTALL_TARGET_LIB}"
  find . -name lib\*.so -exec cp {} "${INSTALL_TARGET_LIB}" \;
  make install -j2
  popd
}

generate_start_script() {
  printInfo "Installing AVS Device SDK on the target device."
  make_install > /dev/null
  adb_push
  CONFIG_BASENAME=$(basename ${OUTPUT_CONFIG_FILE})
  cat << EOF > "${START_SCRIPT}"

  ${ADB} shell LD_LIBRARY_PATH=${DEVICE_INSTALL_PATH}/lib \
      ${DEVICE_BIN_PATH}/SampleApp \
      ${DEVICE_INSTALL_PATH}/${CONFIG_BASENAME} DEBUG9
EOF
}

printInfo() {
    echo "================================================================================"
    local ARGUMENT
    for ARGUMENT in "$@"; do
        echo "= $ARGUMENT"
    done
}

removeSymbolsFromRelObjFiles() {
    local STRIP_ACTION=""
    case "${BUILD_TYPE}" in
        "MinSizeRel" | "Release" )
            STRIP_ACTION="--strip-debug"
            ;;
        * )
            ;;
    esac
    if [ -n "${STRIP_ACTION}" ]; then
        find "$1" -name lib\*.a -print0 | xargs -0 -I % ${STRIP} ${STRIP_ACTION} -o % %
        find "$1" -name lib\*.so -print0 | xargs -0 -I % ${STRIP} ${STRIP_ACTION} -o % %
    fi
}

configure_host() {
  BACKUP_EXTENSION=".BAK"

  BUILD_SYSTEM=$(uname)
  case "${BUILD_SYSTEM}" in
      "Darwin" )
          ANDROID_HOME="${HOME}/Library/Android"
          DISABLE_RPATH="--disable-rpath"
          NDK_PACKAGE_FILE="${NDK_PACKAGE}-darwin-x86_64.zip"
          SED_IN_PLACE_BACKUP_OPTION="-i ${BACKUP_EXTENSION}"
          TOOLCHAIN_BUILD="x86_64-apple-darwin"
          ;;
      "Linux" )
          ANDROID_HOME="${HOME}/Android"
          DISABLE_RPATH=""
          NDK_PACKAGE_FILE="${NDK_PACKAGE}-linux-x86_64.zip"
          SED_IN_PLACE_BACKUP_OPTION="--in-place=${BACKUP_EXTENSION}"
          TOOLCHAIN_BUILD="x86_64-linux-gnu"
          ;;
      * ) echo "$0: Unknown build system: ${BUILD_SYSTEM}"
          exit 1
          ;;
  esac

  case $(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]') in
      "debug" )
          BUILD_TYPE="Debug"
          ;;
      "minsizerel" )
          BUILD_TYPE="MinSizeRel"
          ;;
      "release" )
          BUILD_TYPE="Release"
          ;;
      "relwithdebinfo" )
          BUILD_TYPE="RelWithDebInfo"
          ;;
      * ) echo "$0: Unknown build type: ${BUILD_TYPE}"
          exit 1
          ;;
  esac

  case "${TARGET_SYSTEM}" in
      "arm" | "armeabi-v7a" )
          TARGET_SYSTEM="arm"
          ;;
      "x86" )
          TARGET_SYSTEM="x86"
          ;;
      * ) echo "$0: Unknown host system: ${TARGET_SYSTEM}"
          exit 1
          ;;
  esac

  if [ "${TARGET_SYSTEM}" == "x86" ]; then
      ANDROID_ABI="${TARGET_SYSTEM}"
  else
      if [ "${TARGET_SYSTEM}" == "arm" ]; then
          ANDROID_ABI="armeabi-v7a"
      else
          ANDROID_ABI="arm64-v8a"
      fi
  fi
  mkdir -p "${ANDROID_HOME}/ndk"
  ANDROID_NDK="${ANDROID_HOME}/ndk/ndk-bundle/${NDK_PACKAGE}"
  ANDROID_PLATFORM_LEVEL="${API_LEVEL}"
  ANDROID_SYSROOT_ABI="${TARGET_SYSTEM}"

  ANDROID_ALLOW_UNDEFINED_SYMBOLS="FALSE"
  ANDROID_ARM_MODE="thumb"
  ANDROID_ARM_NEON="FALSE"
  ANDROID_CPP_FEATURES="rtti exceptions"
  ANDROID_DISABLE_FORMAT_STRING_CHECKS="FALSE"
  ANDROID_DISABLE_NO_EXECUTE="FALSE"
  ANDROID_DISABLE_RELRO="FALSE"
  ANDROID_PIE="ON"
  ANDROID_PLATFORM="android-${ANDROID_PLATFORM_LEVEL}"
  ANDROID_STL="c++_shared"
  ANDROID_TOOLCHAIN="clang"

  # Standalone toolchain
  if [ "${TARGET_SYSTEM}" == "x86" ]; then
      TOOLCHAIN_HOST="i686-linux-android"
  else
      if [ "${TARGET_SYSTEM}" == "arm" ]; then
          TOOLCHAIN_HOST="arm-linux-androideabi"
      else
          TOOLCHAIN_HOST="aarch64-linux-android"
      fi
  fi
  TOOLCHAIN="${ANDROID_HOME}/ndk/toolchains/${NDK_PACKAGE}/toolchain-${ANDROID_ABI}/${ANDROID_PLATFORM}"
  if [ ! -d "${TOOLCHAIN}" ]; then
      if [ ! -d "${ANDROID_NDK}" ]; then
          mkdir -p "${ANDROID_HOME}/ndk/ndk-bundle"
          pushd "${ANDROID_HOME}/ndk/ndk-bundle"
          if [ ! -f "${NDK_PACKAGE_FILE}" ]; then
              if [ $(which wget) ]; then
                  wget "https://dl.google.com/android/repository/${NDK_PACKAGE_FILE}"
              elif [ $(which curl) ]; then
                  curl --output "${NDK_PACKAGE_FILE}" --url "https://dl.google.com/android/repository/${NDK_PACKAGE_FILE}"
              else
                  echo "Error: Cannot download package ${NDK_PACKAGE_FILE}. Please install 'wget' or 'curl'."
                  exit 1
              fi
          fi
          unzip -a "${NDK_PACKAGE_FILE}"
          popd
      fi
      MAKE_STANDALONE_TOOLCHAIN_ARCH="${ANDROID_SYSROOT_ABI}"
      case "${NDK_PACKAGE}" in
          "android-ndk-r16" )
              "${ANDROID_NDK}/build/tools/make_standalone_toolchain.py" \
                      --arch="${MAKE_STANDALONE_TOOLCHAIN_ARCH}" \
                      --api="${ANDROID_PLATFORM_LEVEL}" \
                      --stl="libc++" \
                      --force \
                      --install-dir="${TOOLCHAIN}"
              ;;
          "android-ndk-r15c" )
              "${ANDROID_NDK}/build/tools/make_standalone_toolchain.py" \
                      --arch="${MAKE_STANDALONE_TOOLCHAIN_ARCH}" \
                      --api="${ANDROID_PLATFORM_LEVEL}" \
                      --stl="libc++" \
                      --unified-headers \
                      --force \
                      --install-dir="${TOOLCHAIN}"
              ;;
          * ) echo "Unsupported NDK package: ${NDK_PACKAGE}"
              exit 1
              ;;
      esac
      if [ "$?" -ne 0 ]; then
          echo "Error: make standalone toolchain failed!"
          exit 1
      fi
  fi
  CMAKE_TOOLCHAIN_FILE="${ANDROID_NDK}/build/cmake/android.toolchain.cmake"
  CXX_SHARED_LIBRARY="${TOOLCHAIN}/${TOOLCHAIN_HOST}/lib/lib${ANDROID_STL}.so"
  export AR="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-ar"
  export CC="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-clang"
  export CPP="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-cpp"
  export CROSS_SYSROOT="${TOOLCHAIN}/sysroot"
  export CXX="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-clang++"
  export LD="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-ld"
  export LINK="${CXX}"
  export PATH="${TOOLCHAIN}/bin:${PATH}"
  export RANLIB="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-ranlib"
  export READELF="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-readelf"
  export STRIP="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-strip"
  export SYSROOT="${TOOLCHAIN}/sysroot"

  # Out-of-source build
  BUILD_TYPE_LOWER=$(echo "${BUILD_TYPE}" | tr '[:upper:]' '[:lower:]')
  BUILD_PLATFORM="${INSTALL_BASE}/${BUILD_TYPE_LOWER}/${ANDROID_PLATFORM}"
  BUILD_TARGET="${BUILD_PLATFORM}/${ANDROID_ABI}"
  INSTALL_TARGET="${BUILD_TARGET}/install"
  INSTALL_TARGET_INCLUDE="${INSTALL_TARGET}/include"
  INSTALL_TARGET_LIB="${INSTALL_TARGET}/lib"
  [ ! -d "${INSTALL_TARGET_INCLUDE}" ] && mkdir -p "${INSTALL_TARGET_INCLUDE}"
  [ ! -d "${INSTALL_TARGET_LIB}" ] && mkdir -p "${INSTALL_TARGET_LIB}"

  # Symbolic links for Android JNI convenience
  JNILIBS="${BUILD_PLATFORM}/jniLibs"
  [ ! -d "${JNILIBS}" ] && mkdir -p "${JNILIBS}"
  [ ! -L "${JNILIBS}/${ANDROID_ABI}" ] && ln -sf "${INSTALL_TARGET_LIB}/" "${JNILIBS}/${ANDROID_ABI}"

  # Print installation summary
  printInfo   "ANDROID_HOME:         ${ANDROID_HOME}" \
              "ANDROID_NDK:          ${ANDROID_NDK}" \
              "CMAKE_TOOLCHAIN_FILE: ${CMAKE_TOOLCHAIN_FILE}" \
              "CXX:                  ${CXX}" \
              "READELF:              ${READELF}" \
              "STRIP:                ${STRIP}" \
              "SYSROOT:              ${SYSROOT}" \
              "TOOLCHAIN:            ${TOOLCHAIN}" \
              "TOOLCHAIN_HOST:       ${TOOLCHAIN_HOST}" \
              "INSTALL_PATH:         ${INSTALL_TARGET}" \
              "DEVICE_INSTALL_PATH:  ${DEVICE_INSTALL_PATH}"
  echo ""

}

download_dependency() {
  local LIB_NAME=$1
  local LIB_PATH=$2
  local LIB_URL=$3
  local ORIGINAL_PATH=$4
  if [ -z "${!LIB_NAME:-}" ]; then # indirect expansion
      DIRECTORY="${INSTALL_BASE}/${LIB_PATH}"
      PARENT=$(dirname "${DIRECTORY}")
      LIB_DIR_NAME=$(basename "${LIB_PATH}")
      LIB_FILE=$(basename "${LIB_URL}")
      eval ${LIB_NAME}=\${DIRECTORY}
      mkdir -p "${PARENT}"
      pushd "${PARENT}" > /dev/null
      if [ ! -f ${LIB_FILE} ]; then
          if [ $(which wget) ]; then
              wget ${LIB_URL}
          elif [ $(which curl) ]; then
              curl --output ${LIB_FILE} --url ${LIB_URL}
          else
              echo "Error: Cannot download package ${LIB_URL}. Please install 'wget' or 'curl'."
              exit 1
          fi
      fi
      if [ ! -d ${LIB_DIR_NAME} ]; then
          tar -zxf ${LIB_FILE}
          mv ${ORIGINAL_PATH} ${LIB_DIR_NAME}
      fi
      popd > /dev/null
      [ ! -d "${DIRECTORY}" ] && (echo "${DIRECTORY}: No such directory"; exit 1)
      DIRECTORY=$(cd "${DIRECTORY}" && pwd)
  else
      DIRECTORY=${!LIB_DIR_NAME:-${LIB_PATH}}
      [ ! -d "${DIRECTORY}" ] && (echo "${DIRECTORY}: No such directory"; exit 1)
      DIRECTORY=$(cd "${DIRECTORY}" && pwd)
  fi
}

install_dependencies() {

  download_dependency "CURL_LIBRARY_SOURCE" "Libraries/curl" "https://github.com/curl/curl/releases/download/curl-7_61_0/curl-7.61.0.tar.gz" "curl-7.61.0"
  download_dependency "NGHTTP2_LIBRARY_SOURCE" "Libraries/nghttp2" "https://github.com/nghttp2/nghttp2/releases/download/v1.32.0/nghttp2-1.32.0.tar.gz" "nghttp2-1.32.0"
  download_dependency "OPENSSL_LIBRARY_SOURCE" "Libraries/openssl" "https://github.com/openssl/openssl/archive/OpenSSL_1_1_0h.tar.gz" "openssl-OpenSSL_1_1_0h"
  download_dependency "SQLITE3_LIBRARY_SOURCE" "Libraries/sqlite3" "https://sqlite.org/2018/sqlite-autoconf-3240000.tar.gz" "sqlite-autoconf-3240000"
  download_dependency "FFMPEG_LIBRARY_SOURCE" "Libraries/ffmpeg" "https://www.ffmpeg.org/releases/ffmpeg-4.0.tar.gz" "ffmpeg-4.0"

  ##################################################
  # Download packages
  ##################################################
  printInfo "Download packages"

  # Default num threads to use for make commands
  NUM_THREADS="1"

  # Override on mac or linux - use number of logic cores
  if [ "$(uname)" == "Darwin" ]; then
      # on mac
      NUM_THREADS=$(sysctl -n hw.ncpu)
  elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
      # on linux
      NUM_THREADS=$(grep -c processor /proc/cpuinfo)
  fi

  # Some breathing room
  if (( ${NUM_THREADS} > 1 )); then
      NUM_THREADS=$(($NUM_THREADS - 1))
  fi

  ##################################################
  # Build openssl
  ##################################################
  OPENSSL_ARCH="${ANDROID_ABI}"
  OPENSSL_CROSS_COMPILE="${TOOLCHAIN_HOST}-"
  if [ "${TARGET_SYSTEM}" == "x86" ]; then
      # android-x86
      OPENSSL_MACHINE="i686"
      OPENSSL_TARGET="android-x86"
  else
      # android-armeabi
      OPENSSL_MACHINE="armv7"
      OPENSSL_TARGET="android-armeabi"
  fi
  OPENSSL_RELEASE="2.6.37"
  OPENSSL_SYSTEM="android"

  OPENSSL_BUILD_TARGET="${BUILD_TARGET}/openssl"
  DONE_FILE="${OPENSSL_BUILD_TARGET}/.done"
  if [ -d "${OPENSSL_LIBRARY_SOURCE}" ] && [ ! -f "${DONE_FILE}" ]; then
      if [ ! -f "${OPENSSL_BUILD_TARGET}/Makefile" ]; then
          mkdir -p "${OPENSSL_BUILD_TARGET}"
          pushd "${OPENSSL_BUILD_TARGET}"
          sed ${SED_IN_PLACE_BACKUP_OPTION} "s/-mandroid//" "${OPENSSL_LIBRARY_SOURCE}/Configurations/10-main.conf"
          "${OPENSSL_LIBRARY_SOURCE}/Configure" \
                  "${OPENSSL_TARGET}" \
                  -DARCH="${OPENSSL_ARCH}" \
                  -DCROSS_COMPILE="${OPENSSL_CROSS_COMPILE}" \
                  -DMACHINE="${OPENSSL_MACHINE}" \
                  -DRELEASE="${OPENSSL_RELEASE}" \
                  -DSYSTEM="${OPENSSL_SYSTEM}" \
                  no-asm \
                  no-comp \
                  no-dso \
                  no-dtls \
                  no-engine \
                  no-hw \
                  no-idea \
                  no-nextprotoneg \
                  no-psk \
                  no-srp \
                  no-ssl3 \
                  no-weak-ssl-ciphers \
                  --prefix="${INSTALL_TARGET}" \
                  --openssldir="${INSTALL_TARGET}/ssl" \
                  -D_FORTIFY_SOURCE="2" -fstack-protector-strong
          if [ -f "${OPENSSL_LIBRARY_SOURCE}/Configurations/10-main.conf${BACKUP_EXTENSION}" ]; then
              mv "${OPENSSL_LIBRARY_SOURCE}/Configurations/10-main.conf${BACKUP_EXTENSION}" "${OPENSSL_LIBRARY_SOURCE}/Configurations/10-main.conf"
          fi
          rm -fr "${INSTALL_TARGET_LIB}/engines-"*
          rm -f "${INSTALL_TARGET_LIB}/libcrypto."*
          popd
      fi
      pushd "${OPENSSL_BUILD_TARGET}"
      make depend
      make all -j ${NUM_THREADS}
      removeSymbolsFromRelObjFiles .
      make install_sw
      popd

      echo "Built on $(date)" > ${DONE_FILE}
  fi

  ##################################################
  # Build nghttp2
  ##################################################
  NGHTTP2_BUILD_TARGET="${BUILD_TARGET}/nghttp2"
  DONE_FILE="${NGHTTP2_BUILD_TARGET}/.done"
  if [ -d "${NGHTTP2_LIBRARY_SOURCE}" ] && [ ! -f "${DONE_FILE}" ]; then
      if [ ! -f "${NGHTTP2_BUILD_TARGET}/Makefile" ]; then
          mkdir -p "${NGHTTP2_BUILD_TARGET}"
          pushd "${NGHTTP2_BUILD_TARGET}"
          if [ ! -f "${NGHTTP2_LIBRARY_SOURCE}/configure" ]; then
              pushd "${NGHTTP2_LIBRARY_SOURCE}"
              autoreconf -i
              automake
              autoconf
              popd
          fi
          "${NGHTTP2_LIBRARY_SOURCE}/configure" \
                  ${DISABLE_RPATH} \
                  --prefix="${INSTALL_TARGET}" \
                  --host="${TOOLCHAIN_HOST}" \
                  --build="${TOOLCHAIN_BUILD}" \
                  --enable-static="YES" \
                  --enable-shared="YES" \
                  CPPFLAGS="-fPIE -D_FORTIFY_SOURCE=2 -fstack-protector-strong" \
                  LDFLAGS="-fPIE -pie" \
                  PKG_CONFIG_LIBDIR="${INSTALL_TARGET_LIB}/pkgconfig"
          rm -f "${INSTALL_TARGET_LIB}/libnghttp2."*
          popd
      fi
      pushd "${NGHTTP2_BUILD_TARGET}"
      make -j ${NUM_THREADS}
      removeSymbolsFromRelObjFiles .
      make install
      popd

      echo "Built on $(date)" > ${DONE_FILE}
  fi

  ##################################################
  # Build curl
  ##################################################
  CURL_BUILD_TARGET="${BUILD_TARGET}/curl"
  CURL_INCLUDE_DIR="${INSTALL_TARGET_INCLUDE}"

  CURL_LIBRARY="${INSTALL_TARGET_LIB}/libcurl.so"
  DONE_FILE="${CURL_BUILD_TARGET}/.done"
  if [ -d "${CURL_LIBRARY_SOURCE}" ] && [ ! -f "${DONE_FILE}" ]; then
      if [ ! -f "${CURL_BUILD_TARGET}/Makefile" ]; then
          mkdir -p "${CURL_BUILD_TARGET}"
          pushd "${CURL_BUILD_TARGET}"
          if [ ! -f "${CURL_LIBRARY_SOURCE}/configure" ]; then
              pushd "${CURL_LIBRARY_SOURCE}"
              autoreconf -i
              automake
              autoconf
              popd
          fi
          CFLAGS="-fstack-protector-strong" \
          CPPFLAGS="-D_FORTIFY_SOURCE=2 -fstack-protector-strong -I\"${INSTALL_TARGET_INCLUDE}\"" \
          LDFLAGS="-L\"${INSTALL_TARGET_LIB}\"" "${CURL_LIBRARY_SOURCE}/configure" \
                  ${DISABLE_RPATH} \
                  --prefix="${INSTALL_TARGET}" \
                  --with-sysroot="${SYSROOT}" \
                  --host="${TOOLCHAIN_HOST}" \
                  --build="${TOOLCHAIN_BUILD}" \
                  --enable-optimize \
                  --enable-hidden-symbols \
                  --disable-largefile \
                  --disable-static \
                  --disable-ftp \
                  --disable-file \
                  --disable-ldap \
                  --disable-rtsp \
                  --disable-proxy \
                  --disable-dict \
                  --disable-telnet \
                  --disable-tftp \
                  --disable-pop3 \
                  --disable-imap \
                  --disable-smb \
                  --disable-smtp \
                  --disable-gopher \
                  --disable-manual \
                  --disable-verbose \
                  --disable-sspi \
                  --disable-crypto-auth \
                  --disable-tls-srp \
                  --disable-unix-sockets \
                  --enable-cookies \
                  --without-zlib \
                  --with-ssl="${INSTALL_TARGET}" \
                  --with-ca-bundle="${CURL_CA_BUNDLE}" \
                  --with-nghttp2="${INSTALL_TARGET}"
          rm -f "${INSTALL_TARGET_LIB}/libcurl-d."*
          rm -f "${INSTALL_TARGET_LIB}/libcurl."*
          popd
      fi
      pushd "${CURL_BUILD_TARGET}"
      make -j ${NUM_THREADS}
      removeSymbolsFromRelObjFiles .
      make install
      popd

      echo "Built on $(date)" > ${DONE_FILE}
  fi

  ##################################################
  # Build sqlite3
  ##################################################
  SQLITE3_BUILD_TARGET="${BUILD_TARGET}/sqlite3"
  DONE_FILE="${SQLITE3_BUILD_TARGET}/.done"
  if [ -d "${SQLITE3_LIBRARY_SOURCE}" ] && [ ! -f "${DONE_FILE}" ]; then
      if [ ! -f "${SQLITE3_BUILD_TARGET}/Makefile" ]; then
          mkdir -p "${SQLITE3_BUILD_TARGET}"
          pushd "${SQLITE3_BUILD_TARGET}"
          "${SQLITE3_LIBRARY_SOURCE}/configure" \
                  ${DISABLE_RPATH} \
                  --prefix="${INSTALL_TARGET}" \
                  --with-sysroot="${SYSROOT}" \
                  --host="${TOOLCHAIN_HOST}" \
                  --build="${TOOLCHAIN_BUILD}" \
                  --disable-static
          rm -f "${INSTALL_TARGET_LIB}/libsqlite3."*
          popd
      fi
      pushd "${SQLITE3_BUILD_TARGET}"
      if [ "${ANDROID_PLATFORM_LEVEL}" -ge "21" ]; then
          make -j ${NUM_THREADS}
      else
          make -j ${NUM_THREADS} CFLAGS="-D_FILE_OFFSET_BITS=32"
      fi
      removeSymbolsFromRelObjFiles .
      make install
      popd

      echo "Built on $(date)" > ${DONE_FILE}
  fi

  ##################################################
  # Build FFmpeg
  ##################################################
  if [ "${TARGET_SYSTEM}" == "x86" ]; then
      FFMPEG_ARCH="x86"
  else
      FFMPEG_ARCH="armv7-a"
  fi

  FFMPEG_DEBUG_OPS=""
  if [ "${BUILD_TYPE}" == "debug" ]; then
      FFMPEG_DEBUG_OPS="--disable-optimizations --disable-mmx --disable-stripping
      --enable-debug "
  fi

  FFMPEG_BUILD_TARGET="${BUILD_TARGET}/ffmpeg"
  DONE_FILE="${FFMPEG_BUILD_TARGET}/.done"
  if [ -d "${FFMPEG_LIBRARY_SOURCE}" ] && [ ! -f "${DONE_FILE}" ]; then
      if [ ! -f "${FFMPEG_BUILD_TARGET}/Makefile" ]; then
          mkdir -p "${FFMPEG_BUILD_TARGET}"
          pushd "${FFMPEG_BUILD_TARGET}"
          # TODO: Prune unused options
          CPPFLAGS="-I${INSTALL_TARGET_INCLUDE}"
          LDFLAGS="-L${INSTALL_TARGET_LIB} -lssl"
          "${FFMPEG_LIBRARY_SOURCE}/configure" \
                  ${DISABLE_RPATH} \
                  --prefix="${INSTALL_TARGET}" \
                  --extra-cflags="-I${TOOLCHAIN}/include $CPPFLAGS -fPIE -pie" \
                  --extra-ldflags="-L${TOOLCHAIN}/lib $LDFLAGS -fPIE -pie" \
                  --arch="${FFMPEG_ARCH}" \
                  --sysroot="${SYSROOT}" \
                  --cross-prefix="${TOOLCHAIN}/bin/${TOOLCHAIN_HOST}-" \
                  --target-os="android" \
                  --enable-cross-compile \
                  --cc=clang \
                  --cxx=clang++ \
                  --disable-static \
                  --disable-encoders \
                  --disable-ffplay \
                  --disable-ffprobe \
                  --disable-ffmpeg \
                  --disable-muxers \
                  --disable-indevs \
                  --disable-bsfs \
                  --disable-filters \
                  --disable-asm \
                  --disable-doc \
                  ${FFMPEG_DEBUG_OPS} \
                  --disable-linux-perf \
                  --enable-network \
                  --enable-openssl \
                  --enable-swscale \
                  --enable-shared \
                  --enable-parsers \
                  --enable-decoders \
                  --enable-demuxers \
                  --enable-protocols \
                  --pkg-config=true # Hack to allow us to use openssl.
          popd
      fi
      pushd "${FFMPEG_BUILD_TARGET}"
      make -j ${NUM_THREADS}
      removeSymbolsFromRelObjFiles .
      make install
      popd

      echo "Built on $(date)" > ${DONE_FILE}
  fi

  echo "Done"

}

configure_target() {
  # Create a certificate bundle for curl.
  # If running on emulator, start emulator with -writable-system.
  printInfo "Wait until device is online. Please make sure the device is " \
  "connected."
  ${ADB} wait-for-device

  printInfo "Make sure device intall target exists and has write permission"
  ${ADB} shell mkdir -p ${DEVICE_INSTALL_PATH}
  ${ADB} shell mkdir -p ${DEVICE_BIN_PATH}
  ${ADB} shell mkdir -p ${CONFIG_DB_PATH}

  printInfo "Generate CA bundle"
  ${ADB} shell "echo > ${CURL_CA_BUNDLE}"
  ${ADB} shell \
      "for f in \$(ls ${ANDROID_CA_PATH}); do \
          cat ${ANDROID_CA_PATH}\$f | grep 'END CERTIFICATE' -B 100 >> ${CURL_CA_BUNDLE}; \
      done;"
}

##################################################
# Push files through adb
##################################################

adb_push() {
  configure_target

  ${ADB} push ${INSTALL_TARGET_LIB} ${DEVICE_INSTALL_PATH}
  ${ADB} push ${BUILD_PATH}/SampleApp/src/SampleApp ${DEVICE_BIN_PATH}
  ${ADB} push ${OUTPUT_CONFIG_FILE} ${DEVICE_INSTALL_PATH}
}

configure_host
