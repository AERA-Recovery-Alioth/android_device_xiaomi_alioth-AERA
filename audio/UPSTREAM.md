# Audio bridge provenance

Imported from AERA-Recovery/android_device_oneplus_instantnoodlep-AERA
at 48d7db6c3a4d8b47362a2cf1b4fe27cc11cc0b33. Bridge changes through 6922df5474e91e4c37057be04c9dd5e491d5744e.
Original author: Daniel210191 <daniel@danielspringer.at>.
TinyALSA notices and source licenses are retained.

## Alioth adaptation

Speaker controls follow the protected CS35L41 stereo TDM route in Evo
`device/xiaomi/alioth/audio/mixer_paths_overlay_static.xml`. The bundled
kernel includes Qualcomm audio and Cirrus drivers through techpack config;
these are not separate installed-ROM modules. ADSP PIL images come from the
private read-only slot modem and DSP mounts. Protection firmware and playback deltas
are bundled from the Alioth vendor tree, retaining their original contents.
Only the fixed AERA PCM protocol is accepted; plugins receive no ALSA nodes.
Local transport tests are in `tools/test-audio-bridge.sh`.

PD mapper source: https://github.com/andersson/pd-mapper/commit/5ecd2fe926aca7abfe40724177f63b942cff3947; upstream BSD-3-Clause license retained. Shared QRTR sources are the existing Alioth WLAN dependency.

Alioth uses the platform JsonCpp parser for read-only uncompressed `.jsn`
service maps. File size, nesting, strings, service count and instance values
are validated. It shares the recovery-local QRTR service and advertises the
ADSP audio protection domain before the kernel creates its sound-card children.
