#!/usr/bin/env bash

readonly _BUILD_TYPE=RelWithDebInfo
readonly _BUILD_DIR=xAuto
readonly _GENERATOR="Unix Makefiles"
readonly _RESET_OBS=1

readonly _OBS_TAG=26.1.1
readonly _QT_VERSION=5.15.1
readonly _VLC_VERSION=3.0.8
readonly _CEF_VERSION=75.1.14+gc81164e+chromium-75.0.3770.100
# readonly _CEF_VERSION=85.3.12+g3e94ebf+chromium-85.0.4183.121

readonly _CFLAGS="-Wno-unused-variable -Wno-unused-parameter \
  -Wno-typedef-redefinition -Wno-enum-conversion -Wno-deprecated \
  -Wno-unused-private-field -Wno-sign-compare -Wno-vla"
readonly _CXXFLAGS="${CFLAGS} -Wno-pragmas"

#declare -xr MACOSX_DEPLOYMENT_TARGET=10.12
#declare -xr CMAKE_OSX_ARCHITECTURES=arm64;x86_64
declare -xr CMAKE_OSX_ARCHITECTURES=x86_64
declare -xr CMAKE_BUILD_TYPE=${BUILD_TYPE:-${_BUILD_TYPE}}
declare -xr BUILD_TYPE=${CMAKE_BUILD_TYPE}
_NUM_CORES="$(sysctl -n hw.logicalcpu)"
declare -xri NUM_CORES=${_NUM_CORES}
declare -xri CMAKE_BUILD_PARALLEL_LEVEL=${NUM_CORES}
declare -xr CMAKE_GENERATOR=${CMAKE_GENERATOR:-${_GENERATOR}}
declare -x GENERATOR=${GENERATOR:-${CMAKE_GENERATOR}}
export NINJA_PATH="${DEV_DIR}/depot_tools/ninja"
# XCODE_SELECT="$(xcode-select -p)"
# if [ "${XCODE_SELECT}" = "/Applications/Xcode.app/Contents/Developer" ]; then CURRENT_XCODE=true; fi

declare -xr BUILD_BROWSER=${BROWSER:-ON}
declare -xr BROWSER=${BUILD_BROWSER}
declare -xr ENABLE_SCRIPTING=${SCRIPTING:-ON}
declare -xr DISABLE_PYTHON=${DISABLE_PYTHON:-ON}

declare -x OBS_TAG=${OBS_TAG:-${_OBS_TAG}}
declare -xr QT_VERSION=${QT_VERSION:-${_QT_VERSION}}
declare -xr VLC_VERSION=${VLC_VERSION:-${_VLC_VERSION}}
declare -xr CEF_VERSION=${CEF_VERSION:-${_CEF_VERSION}}
declare -xr CEF_BUILD_VERSION=${CEF_BUILD_VERSION:-${CEF_VERSION}}
REFRESH_OBS=${REFRESH_OBS:=${_RESET_OBS}}
declare -xr RESET_OBS=${RESET_OBS:=${REFRESH_OBS}}

readonly _OBSAGENTS_ROOT="$(pwd)"
declare -xr OBSAGENTS_ROOT=${_OBSAGENTS_ROOT}
cd ../../.. || exit
readonly _OBS_ROOT="$(pwd)"
declare -xr OBS_ROOT=${_OBS_ROOT}
cd .. || exit
readonly _DEV_DIR="$(pwd)"
declare -xr DEV_DIR="${DEV_DIR:-${_DEV_DIR}}"

declare -xr BUILD_DIR="${BUILD_DIR:-${_BUILD_DIR}}"
declare -xr BUILD_ROOT="${OBS_ROOT}/${BUILD_DIR}"

declare -xr OBSDEPS="${OBSDEPS:-${DEV_DIR}/obsdeps}"
declare -xr DepsPath="${OBSDEPS}"
declare -xr X264_INCLUDE_DIR="${X264_INCLUDE_DIR:-${OBSDEPS}/include}"
# declare -xr CURL_INCLUDE_DIR="${CURL_INCLUDE_DIR:-/usr/include}"
# declare -xr CURL_INCLUDE_DIR="${CURL_INCLUDE_DIR:-/usr/local/opt/curl-openssl/include}"
declare -xr VLCPath="${DEV_DIR}/vlc-${VLC_VERSION}"
# declare -xr QTDIR="${QTDIR:-/usr/local/opt/qt}"
declare -xr QTDIR="${QTDIR:-${OBSDEPS}}"
readonly _CEF_DIR="${CEF:-${DEV_DIR}/cef_binary_${CEF_BUILD_VERSION}_macosx64}"
declare -xr CEF_ROOT="${CEF_ROOT:-${_CEF_DIR}}"
declare -xr CEF_ROOT_DIR="${CEF_ROOT_DIR:-${CEF_ROOT}}"
declare -xr BOOST_ROOT="${BOOST_ROOT:-/usr/local/opt/boost}"
readonly _OPENSSL_DIR="${OPENSSL:-/usr/local/opt/openssl@1.1}"
declare -xr OPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR:-${_OPENSSL_DIR}}"
readonly _WEBRTC_DIR="${WEBRTC:-${DEV_DIR}/webrtc}"
declare -xr WEBRTC_ROOT_DIR="${WEBRTC_ROOT_DIR:-${_WEBRTC_DIR}}"

red=$'\e[1;31m'
# grn=$'\e[1;32m'
# blu=$'\e[1;34m'
# mag=$'\e[1;35m'
# cyn=$'\e[1;36m'
bold=$'\e[1m'
reset=$'\e[0m'

exists() {
  command -v "$1" >/dev/null 2>&1
}

hr() {
  echo "────────────────────────────────────────────────────────────────"
  echo "$1"
  [ -n "$2" ] && echo "$2"
  [ -n "$3" ] && printf "$3" "$4" "$5" "$6"
  echo "────────────────────────────────────────────────────────────────"
}

install_ninja() {
  export CFLAGS="${CFLAGS} ${_CFLAGS}"
  export CXXFLAGS=${_CXXFLAGS}
  if ! exists ninja; then
    echo "Ninja not found in PATH"
    if [ -f "${NINJA_PATH}" ]; then
      echo "Ninja found: ${NINJA_PATH}. Adding to PATH"
      _DEPOT_TOOLS=$(dirname "${NINJA_PATH}")
      export DEPOT_TOOLS=${_DEPOT_TOOLS}
      export PATH="${DEPOT_TOOLS}:${PATH}"
    else
      echo "Ninja not found. Installing from brew"
      brew install ninja
    fi
  else
    echo "Ninja found in PATH"
  fi
}

main() {
  set -e
  echo
  echo "${bold}Building           Sidekick${reset}"
  echo "${red}BUILD_TYPE:        ${BUILD_TYPE}${reset}"
  echo "GENERATOR:         ${GENERATOR}"
  echo "OBS_TAG:           ${OBS_TAG}"
  echo
  echo "OBSAGENTS_ROOT:    ${OBSAGENTS_ROOT}"
  echo "BUILD_ROOT:        ${BUILD_ROOT}"
  echo "OBS_ROOT:          ${OBS_ROOT}"
  echo "DEV_DIR:           ${DEV_DIR}"
  echo
  echo "DepsPath:          ${DepsPath}"
  echo "CURL_INCLUDE_DIR:  ${CURL_INCLUDE_DIR}"
  echo "X264_INCLUDE_DIR:  ${X264_INCLUDE_DIR}"
  echo "VLCPath:           ${VLCPath}"
  echo "QTDIR:             ${QTDIR}"
  echo "CEF_ROOT_DIR:      ${CEF_ROOT_DIR}"
  echo "BOOST_ROOT:        ${BOOST_ROOT}"
  echo "OPENSSL_ROOT_DIR:  ${OPENSSL_ROOT_DIR}"
  echo "WEBRTC_ROOT_DIR:   ${WEBRTC_ROOT_DIR}"
  echo "RESET OBS:         ${RESET_OBS}"
  echo

  sudo rm -rf "/Library/Application Support/obs-studio/sidekick"
  sudo mkdir -p "/Library/Application Support/obs-studio/sidekick"
  sudo chmod 775 "/Library/Application Support/obs-studio/sidekick"

  declare -i start_ts
  start_ts=$(date +%s)
  start="$(date '+%Y-%m-%d %H:%M:%S')"
  hr "Build Started:     ${start}"

  if [ "${GENERATOR}" = "Ninja" ]; then install_ninja; fi

  cd "${OBSAGENTS_ROOT}"
  # "${OBSAGENTS_ROOT}/scripts/build-deps-full.sh" skip_build_tools

  export CFLAGS="-I${OBSDEPS}/include"
  export LDFLAGS="-L${OBSDEPS}/lib"
  export PKG_CONFIG_PATH="${OBSDEPS}/lib/pkgconfig"

  cd "${OBS_ROOT}"
  if [ ${RESET_OBS} -eq 1 ]; then
    echo "-- obs-studio hard reset - initiating"
    rm -rf ./plugins/enc-amf > /dev/null
    git reset --hard > /dev/null
    git submodule foreach git reset --hard > /dev/null
    # if [ "${CURRENT_XCODE}" = true ]; then
    #   hr "Checking out obs-studio pull request #2264"
    #   git fetch origin +refs/pull/2264/merge > /dev/null
    #   OBS_TAG=FETCH_HEAD
    # fi
    git fetch origin > /dev/null
    git checkout ${OBS_TAG} > /dev/null
    git submodule update --init --recursive > /dev/null
    git reset --hard > /dev/null
    echo "-- obs-studio hard reset - done"
  else
    echo "-- obs-studio hard reset - skipping"
  fi

  # Append add_subdirectory(plugins/MyFreeCams) to ${OBS_ROOT}/CMakeLists.txt
  echo "-- Editing ${OBS_ROOT}/CMakeLists.txt"
  if grep -Fxq "add_subdirectory(plugins/MyFreeCams)" "${OBS_ROOT}/CMakeLists.txt"; then
    echo "-- Editing ${OBS_ROOT}/CMakeLists.txt - already modified"
  else
    sed -i '' -e '/add_subdirectory(libobs)/a\'$'\n''add_subdirectory(plugins/MyFreeCams)'$'\n''' "${OBS_ROOT}/CMakeLists.txt"
    echo "-- Editing ${OBS_ROOT}/CMakeLists.txt - success"
  fi

  # Create CMakeLists.txt in ${OBS_ROOT}/plugins/MyFreeCams
  echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt"
  if [ -f "${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt" ]; then
    echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt - file exists"
  else
    echo -e "add_subdirectory(Sidekick)\n" > "${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt"
    echo "-- Creating ${OBS_ROOT}/plugins/MyFreeCams/CMakeLists.txt - success"
  fi

  cd "${OBS_ROOT}"
  if [ -d "${BUILD_ROOT}" ]; then
    mv "${BUILD_DIR}" "${BUILD_DIR}-$(date +'%Y%m%d_%H%M%S')"
  fi
  mkdir -p "${BUILD_DIR}"
  cd "${BUILD_DIR}"

  cmake \
    -G "${GENERATOR}" \
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}" \
    -DCMAKE_BUILD_PARALLEL_LEVEL=${CMAKE_BUILD_PARALLEL_LEVEL} \
    -DCMAKE_OSX_ARCHITECTURES="${CMAKE_OSX_ARCHITECTURES}" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${MACOSX_DEPLOYMENT_TARGET}" \
    -DDepsPath="${OBSDEPS}" \
    -DX264_INCLUDE_DIR="${X264_INCLUDE_DIR}" \
    -DVLCPath="${VLCPath}" \
    -DQTDIR="${QTDIR}" \
    -DQt5Core_DIR="${QTDIR}/lib/cmake/Qt5Core" \
    -DQt5Gui_DIR="${QTDIR}/lib/cmake/Qt5Gui" \
    -DQt5MacExtras_DIR="${QTDIR}/lib/cmake/Qt5MacExtras" \
    -DQt5Svg_DIR="${QTDIR}/lib/cmake/Qt5Svg" \
    -DQt5Widgets_DIR="${QTDIR}/lib/cmake/Qt5Widgets" \
    -DCEF_ROOT="${CEF_ROOT}" \
    -DCEF_ROOT_DIR="${CEF_ROOT_DIR}" \
    -DBOOST_ROOT="${BOOST_ROOT}" \
    -DOPENSSL_ROOT_DIR="${OPENSSL_ROOT_DIR}" \
    -DWEBRTC_ROOT_DIR="${WEBRTC_ROOT_DIR}" \
    -DBUILD_BROWSER="${BUILD_BROWSER}" \
    -DBROWSER_DEPLOY="${BUILD_BROWSER}" \
    -DWITH_RTMPS=ON \
    -DDISABLE_PYTHON="${DISABLE_PYTHON}" \
    -DENABLE_SCRIPTING="${ENABLE_SCRIPTING}" \
    ..

  cmake --build . --config ${BUILD_TYPE}

  end=$(date '+%Y-%m-%d %H:%M:%S')
  declare -i end_ts
  end_ts=$(date +%s)
  declare -i runtime=$((end_ts-start_ts))
  declare -i hours=$((runtime / 3600))
  declare -i minutes=$(( (runtime % 3600) / 60 ))
  declare -i seconds=$(( (runtime % 3600) % 60 ))

  echo
  echo   "BUILD_TYPE:      ${BUILD_TYPE}"
  echo
  echo   "Start:           ${start}"
  echo   "End:             ${end}"
  printf "Elapsed:         (hh:mm:ss) %02d:%02d:%02d\n" ${hours} ${minutes} ${seconds}
}

main "$@"
