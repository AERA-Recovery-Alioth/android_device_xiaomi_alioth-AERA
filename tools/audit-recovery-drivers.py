#!/usr/bin/env python3
"""Local-only Alioth init, firmware, kernel and compiled-policy audit."""
import argparse
import gzip
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys
import tempfile

if sys.flags.optimize:
    raise RuntimeError("Run the driver audit without Python optimization; its assertions are required.")

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--checkout', type=Path, default=Path(__file__).resolve().parents[4])
parser.add_argument('--output', type=Path)
args = parser.parse_args()
root = args.checkout.resolve()
device = root / 'device/xiaomi/alioth'
staged = root / 'out/target/product/alioth/recovery/root'
verifier = root / 'out/host/linux-x86/bin/host_init_verifier'
rcs = sorted((device / 'recovery/root').glob('init*.rc'))
result = {'device_tests': False, 'init_scripts': [], 'policy_permissions': [], 'firmware': []}
for rc in rcs:
    subprocess.run([str(verifier), str(rc)], check=True)
    relative = rc.relative_to(device / 'recovery/root')
    assert (staged / relative).read_bytes() == rc.read_bytes(), f'Stale staged init: {relative}'
    for line in rc.read_text().splitlines():
        fields = shlex.split(line, comments=True)
        if fields and fields[0] == 'import':
            assert len(fields) == 2 and (staged / fields[1].lstrip('/')).is_file(), line
    result['init_scripts'].append(str(relative))

health_rc = device / 'health/android.hardware.health-service.alioth_recovery.rc'
subprocess.run([str(verifier), str(health_rc)], check=True)
assert (staged / 'system/etc/init' / health_rc.name).read_bytes() == health_rc.read_bytes()
result['init_scripts'].append('system/etc/init/' + health_rc.name)

ueventd = device / 'recovery/root/ueventd.qcom.rc'
assert (staged / 'ueventd.qcom.rc').read_bytes() == ueventd.read_bytes()
for line in ueventd.read_text().splitlines():
    fields = shlex.split(line, comments=True)
    if not fields:
        continue
    if fields[0].startswith('/dev/'):
        assert len(fields) in (4, 5), line
        int(fields[1], 8)
    elif fields[0].startswith('/sys/'):
        assert len(fields) in (5, 6), line
        int(fields[2], 8)
    else:
        assert fields[0] == 'firmware_directories' and len(fields) > 1, line
        assert all(path.endswith('/') for path in fields[1:]), 'ueventd concatenates firmware paths directly: ' + line
result['ueventd'] = 'syntax and staged copy checked; first API 30 loads legacy qcom file'
properties = (staged / 'prop.default').read_text()
assert re.search(r'^ro.product.first_api_level=30$', properties, re.M)

# Audio frontend, bootstrap and amplifier protection must ship together.
assert (staged / 'system/bin/aera-audio-bridge').is_file()
assert (staged / 'system/bin/aera-alioth-pd-mapper').is_file()
assert (staged / 'system/bin/aera-audio-bootstrap').read_bytes() == (device / 'recovery/root/system/bin/aera-audio-bootstrap').read_bytes()
for name in ['cs35l41-dsp1-spk-prot.wmfw', 'cs35l41-dsp1-spk-prot.bin',
             'cs35l41-dsp1-spk-prot-RCV.bin', 'cs35l41-dsp1-spk-prot-revb2.wmfw',
             'cs35l41-dsp1-spk-prot-revb2.bin', 'cs35l41-dsp1-spk-prot-RCV-revb2.bin',
             'spk1_playback_delta.txt', 'spk2_playback_delta.txt']:
    assert (staged / 'system/etc/aera/audio/firmware' / name).read_bytes() == (device / 'recovery/root/system/etc/aera/audio/firmware' / name).read_bytes()
result['audio_configuration'] = 'Bundled protected CS35L41 TDM route, built-in drivers, read-only ADSP firmware; hardware playback requires separate acceptance'

assert 'AERA_SUPPORT_INPUT_FF_HAPTICS := true' in (device / 'BoardConfig.mk').read_text()
variables = json.loads((root / 'out/soong/soong.twrp_alioth.variables').read_text())
assert variables['VendorVars']['twrpVarsPlugin']['TW_SUPPORT_INPUT_FF_HAPTICS'] == 'true'
assert (staged / 'system/bin/busybox').is_file() or (staged / 'sbin/busybox').is_file(), 'Missing WLAN probe utility'

for name in ['system/bin/aera-alioth-wlan-mac', 'system/bin/snapuserd', 'system/bin/qrtr-ns', 'system/bin/cnss-daemon', 'system/bin/wpa_supplicant',
             'system/bin/android.hardware.keymaster@4.0-service-qti',
             'system/bin/android.hardware.gatekeeper@1.0-service-qti',
             'system/bin/hw/android.hardware.health-service.alioth_recovery',
             'system/bin/vendor.qti.hardware.display.allocator-service']:
    assert (staged / name).is_file(), f'Missing required driver service: {name}'
template = device / 'recovery/root/system/etc/wifi/wpa_supplicant.conf'
assert (staged / 'system/etc/wifi/wpa_supplicant.conf').read_bytes() == template.read_bytes()
assert 'update_config=0' in template.read_text()
assert re.search(r'^ro.aera.wlan.supplicant_conf=/tmp/recovery/wpa_supplicant\.conf$', properties, re.M)
assert not (staged / 'system/bin/aera-wifi-check.sh').exists(), 'Obsolete diagnostic service is staged'
qcom_init = (device / 'recovery/root/init.recovery.qcom.rc').read_text()
wifi_init = (device / 'recovery/root/init.recovery.wifi.rc').read_text()
assert '/tmp/recovery/wpa_supplicant.conf' in wifi_init
assert 'u:r:vendor_alioth_recovery_supplicant:s0' in wifi_init
assert 'u:r:vendor_alioth_recovery_allocator:s0' in qcom_init
result['wifi_configuration'] = 'Read-only template, RAM configuration and dedicated init service domains'

for name in ['aw8697_haptic.bin', 'aw8697_rtp_1.bin', 'a650_gmu.bin', 'a650_sqe.fw',
             'a650_zap.elf', 'a650_zap.mdt', 'a650_zap.b00', 'a650_zap.b01', 'a650_zap.b02',
             'wlan/qca_cld/qca6390/WCNSS_qcom_cfg.ini']:
    assert (staged / 'vendor/firmware' / name).is_file(), name
    result['firmware'].append(name)
image = (device / 'prebuilt/Image').read_bytes()
start, end = image.index(b'IKCFG_ST') + 8, image.index(b'IKCFG_ED')
config = gzip.decompress(image[start:end]).decode()
required = ['QCA_CLD_WLAN', 'CNSS2', 'QRTR', 'TOUCHSCREEN_FOCALTECH', 'INPUT_AW8697_HAPTIC',
            'DM_USER', 'DM_SNAPSHOT', 'QCOM_KGSL', 'USB_CONFIGFS', 'USB_CONFIGFS_F_FS', 'FS_ENCRYPTION', 'INCREMENTAL_FS']
for symbol in required:
    assert f'CONFIG_{symbol}=y\n' in config, f'Kernel driver not built in: {symbol}'
result['built_in_kernel_drivers'] = required

with tempfile.TemporaryDirectory(prefix='aera-driver-policy-') as directory:
    binary = Path(directory) / 'policy-access'
    subprocess.run(['cc', '-Wall', '-Wextra', '-Werror', str(device / 'tools/policy-access.c'),
                    '-o', str(binary), '-l:libsepol.a'], check=True)
    policy = staged / 'sepolicy'
    checks = [('ueventd', 'vendor_alioth_recovery_firmware_file', 'file', p)
              for p in ['read', 'open', 'getattr', 'map']]
    checks += [('ueventd', 'vendor_alioth_recovery_firmware_file', 'dir', p)
               for p in ['search', 'read', 'open']]
    checks += [('init', name, 'filesystem', 'relabelto') for name in
               ['vendor_alioth_recovery_firmware_file', 'vendor_alioth_recovery_bt_firmware_file',
                'vendor_alioth_recovery_spunvm_file']]
    checks += [(domain, name, 'filesystem', permission) for domain in ['init', 'recovery']
               for name in ['vendor_alioth_recovery_firmware_file', 'vendor_alioth_recovery_bt_firmware_file']
               for permission in ['mount', 'unmount', 'getattr', 'relabelfrom']]
    checks += [(name, name, 'filesystem', 'associate') for name in
               ['vendor_alioth_recovery_firmware_file', 'vendor_alioth_recovery_bt_firmware_file',
                'vendor_alioth_recovery_spunvm_file']]
    checks += [('recovery', 'recovery', 'qipcrtr_socket', permission)
               for permission in ['create', 'bind', 'read', 'write', 'getattr', 'getopt', 'setopt']]
    checks += [('snapuserd', 'rootfs', 'file', permission)
               for permission in ['read', 'open', 'getattr', 'map', 'execute', 'entrypoint']]
    checks += [('recovery', 'misc_block_device', 'blk_file', p) for p in ['read', 'write', 'open']]
    checks += [('recovery', 'vendor_alioth_recovery_drm_device', 'chr_file', p)
               for p in ['read', 'write', 'open', 'ioctl', 'map']]
    checks += [('init', 'vendor_alioth_recovery_usb_mode', 'file', 'write'),
               ('recovery', 'vendor_alioth_recovery_haptics', 'file', 'write')]
    health = 'vendor_alioth_recovery_health'
    checks += [('init', health, 'process', 'transition'),
               (health, 'rootfs', 'file', 'execute')]
    checks += [(health, 'vendor_alioth_recovery_health_exec', 'file', p)
               for p in ['entrypoint', 'execute', 'read', 'open']]
    checks += [(health, 'sysfs_batteryinfo', 'file', p) for p in ['read', 'open', 'getattr']]
    checks += [(health, 'sysfs_batteryinfo', 'dir', 'search'),
               (health, health, 'capability2', 'wake_alarm'),
               (health, health, 'capability2', 'block_suspend'),
               (health, 'kmsg_device', 'chr_file', 'write'),
               (health, 'hal_health_service', 'service_manager', 'add')]
    checks += [(domain, 'vendor_alioth_recovery_persist_file', 'filesystem', p)
               for domain in ['init', 'recovery'] for p in ['mount', 'unmount', 'getattr', 'relabelfrom', 'relabelto']]
    checks += [('vendor_alioth_recovery_persist_file', 'vendor_alioth_recovery_persist_file',
                'filesystem', 'associate')]
    checks += [('recovery', 'vendor_alioth_recovery_persist_file', 'file', p)
               for p in ['read', 'open', 'getattr']]
    allocator = 'vendor_alioth_recovery_allocator'
    supplicant = 'vendor_alioth_recovery_supplicant'
    for domain in [allocator, supplicant]:
        checks += [('init', domain, 'process', 'transition'),
                   (domain, domain + '_exec', 'file', 'entrypoint'),
                   (domain, 'rootfs', 'file', 'execute')]
    checks += [(supplicant, 'vendor_alioth_recovery_wifi_config_file', 'file', p)
               for p in ['read', 'open', 'write', 'getattr']]
    checks += [(supplicant, 'vendor_alioth_recovery_wifi_socket', 'sock_file', 'create'),
               ('recovery', 'vendor_alioth_recovery_wifi_socket', 'sock_file', 'write'),
               ('recovery', supplicant, 'unix_dgram_socket', 'sendto'),
               (supplicant, 'recovery', 'unix_dgram_socket', 'sendto')]
    checks += [('recovery', 'audio_device', 'chr_file', p) for p in ['read', 'write', 'open', 'ioctl', 'map']]
    checks += [('init', 'vendor_alioth_recovery_adsp_boot', 'file', 'write')]
    for check in checks:
        output = subprocess.check_output([str(binary), str(policy), *check], text=True).strip()
        result['policy_permissions'].append(output)
    # Every explicit fstab mount label must resolve to a compiled policy type.
    fstab = device / 'recovery/root/system/etc/recovery.fstab'
    assert (staged / 'system/etc/recovery.fstab').read_bytes() == fstab.read_bytes()
    for type_name in re.findall(r'context=u:object_r:([a-zA-Z0-9_]+):s0', fstab.read_text()):
        subprocess.run([str(binary), str(policy), 'init', type_name, 'filesystem', 'relabelto'], check=True)
result['scope'] = 'Local source, staged files and unconditional compiled allow rules; no hardware guarantee.'
text = json.dumps(result, indent=2) + '\n'
if args.output:
    args.output.write_text(text)
print(text)
