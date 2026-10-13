#!/usr/bin/env bash
# Alioth AERA bring-up has not passed on-device acceptance.
export AERA_BUILD_TYPE=Alpha
# WLAN probes need the standalone resolver; Android toybox uses absent netd.
export AERA_USE_BUSYBOX_BINARY=1
