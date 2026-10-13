#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
set -eu
base=$(CDPATH= cd -- "$(dirname -- "$0")/../audio" && pwd)
work=$(mktemp -d)
trap 'rm -f "$work/test" "$work/mapper-test" "$work/qmi.o" "$work/servreg.o" "$work/logging.o"; rmdir "$work"' EXIT HUP INT TERM
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -UNDEBUG \
    -fsanitize=address,undefined -ffunction-sections -fdata-sections \
    -I"$base/tinyalsa/include" "$base/bridge_test.cpp" -pthread \
    -Wl,--gc-sections \
    -Wl,--wrap=getsockopt,--wrap=pcm_open,--wrap=pcm_is_ready,--wrap=pcm_close,--wrap=pcm_get_error,--wrap=pcm_writei,--wrap=pcm_get_poll_fd,--wrap=mixer_open,--wrap=mixer_close,--wrap=mixer_get_ctl_by_name,--wrap=mixer_ctl_get_num_values,--wrap=mixer_ctl_set_value,--wrap=mixer_ctl_set_enum_by_string \
    -o "$work/test"
"$work/test"

checkout=$(CDPATH= cd -- "$base/../../../.." && pwd)
${CC:-cc} -fsanitize=address,undefined -I"$base/../wifi/qrtr/include" -c "$base/../wifi/qrtr/lib/qmi.c" -o "$work/qmi.o"
${CC:-cc} -fsanitize=address,undefined -I"$base/../wifi/qrtr/include" -c "$base/../wifi/qrtr/lib/logging.c" -o "$work/logging.o"
${CC:-cc} -fsanitize=address,undefined -I"$base/../wifi/qrtr/include" -c "$base/pd-mapper/servreg_loc.c" -o "$work/servreg.o"
${CXX:-c++} -std=c++17 -Wall -Wextra -UNDEBUG \
    -fsanitize=address,undefined -ffunction-sections -fdata-sections \
    -I"$base/../wifi/qrtr/include" -I"$checkout/external/jsoncpp/include" \
    "$base/pd_mapper_test.cpp" \
    "$checkout/external/jsoncpp/src/lib_json/json_reader.cpp" \
    "$checkout/external/jsoncpp/src/lib_json/json_value.cpp" \
    "$checkout/external/jsoncpp/src/lib_json/json_writer.cpp" \
    "$work/qmi.o" "$work/servreg.o" "$work/logging.o" -Wl,--gc-sections -o "$work/mapper-test"
"$work/mapper-test"
