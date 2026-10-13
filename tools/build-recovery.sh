#!/usr/bin/env bash
set -eo pipefail
cd "$(dirname -- "${BASH_SOURCE[0]}")/../../../.."
source build/envsetup.sh
export AERA_BUILD_TYPE=Alpha
# WLAN probes need the standalone resolver; Android toybox uses absent netd.
export AERA_USE_BUSYBOX_BINARY=1
lunch twrp_alioth-bp2a-user
m -j"${JOBS:-8}" adbd bootimage
