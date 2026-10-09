#!/usr/bin/env python3
"""Package a final PBRP recovery; never flash."""
import argparse
import hashlib
import gzip
import subprocess
import shutil
import zipfile
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
metadata_source = parser.add_mutually_exclusive_group(required=True)
metadata_source.add_argument("--rom", type=Path, help="Target Alioth ROM ZIP")
metadata_source.add_argument("--metadata-boot", type=Path, help="Previously matched recovery Boot image; retain its OS/patch metadata")
parser.add_argument("--security-patch", help="Verified installed-ROM patch YYYY-MM; override retained Boot metadata")
args = parser.parse_args()
root = Path(__file__).resolve().parents[4]
product = root / "out/target/product/alioth"
staging = product / "recovery/root"
host = root / "out/host/linux-x86/bin"
output = root / "out/release-candidate"
output.mkdir(exist_ok=True)
# Fail rather than silently boot a stale theme from an incremental build.
for source, destination in (
    (root / "bootable/recovery/gui/theme/common/portrait.xml", staging / "twres/portrait.xml"),
    (root / "bootable/recovery/gui/theme/portrait_hdpi/extras.xml", staging / "twres/extras.xml"),
):
    if source.read_bytes() != destination.read_bytes():
        raise SystemExit(f"Stale recovery resource: {destination}; rebuild the theme before packaging")

if args.rom:
    with zipfile.ZipFile(args.rom) as archive:
        metadata = dict(line.split("=", 1) for line in archive.read("META-INF/com/android/metadata").decode().splitlines() if "=" in line)
    if not {"alioth", "aliothin"}.intersection(metadata.get("pre-device", "").split(",")):
        raise SystemExit("ROM does not target Alioth")
    os_version = metadata["post-build"].split(":", 1)[1].split("/", 1)[0]
    patch_level = metadata["post-security-patch-level"][:7]
else:
    import struct
    with args.metadata_boot.open("rb") as image:
        header = image.read(44)
    if len(header) != 44 or header[:8] != b"ANDROID!" or struct.unpack_from("<I", header, 40)[0] != 3:
        raise SystemExit("Metadata source must be a header-v3 Android Boot image")
    version = struct.unpack_from("<I", header, 16)[0]
    os_version = f"{(version >> 25) & 127}.{(version >> 18) & 127}.{(version >> 11) & 127}"
    month = version & 15
    if not version or not 1 <= month <= 12:
        raise SystemExit("Metadata source has missing/invalid OS or patch metadata")
    patch_level = f"{2000 + ((version >> 4) & 127):04d}-{month:02d}"
if args.security_patch:
    import re
    if not args.metadata_boot or not re.fullmatch(r"20[0-9]{2}-(0[1-9]|1[0-2])", args.security_patch):
        raise SystemExit("--security-patch requires --metadata-boot and valid YYYY-MM")
    patch_level = args.security_patch
policy = staging / "sepolicy"
domains = set(subprocess.check_output([str(host / "sepolicy-analyze"), str(policy), "permissive"], text=True).split())
expected = {"recovery"}
if domains != expected:
    raise SystemExit(f"Unexpected permissive domains: {sorted(domains)}")
# Refresh minuitwrp even when an old build graph omitted its relink dependency.
shutil.copy2(product / "system/lib64/libminuitwrp.so", staging / "system/lib64/libminuitwrp.so")
original = (staging / "sepolicy").read_bytes()
checksums = staging / "ramdisk-files.sha256sum"
original_checksums = checksums.read_bytes()
try:
    # Keep Flash Current's integrity check valid for the exact packaged ramdisk.
    entries = []
    for line in original_checksums.decode().splitlines():
        _, name = line.split("  ", 1)
        entries.append(hashlib.sha256((staging / name).read_bytes()).hexdigest() + "  " + name)
    checksums.write_text("\n".join(entries) + "\n")
    cpio = subprocess.check_output([str(host / "mkbootfs"), "-d", str(product), str(staging)])
finally:
    (staging / "sepolicy").write_bytes(original)
    checksums.write_bytes(original_checksums)
ramdisk = output / "ramdisk.gz"
ramdisk.write_bytes(gzip.compress(cpio, mtime=0))
image = output / "recovery_boot.img"
subprocess.run([str(host / "mkbootimg"), "--kernel", str(product / "kernel"), "--ramdisk", str(ramdisk), "--header_version", "3", "--os_version", os_version, "--os_patch_level", patch_level, "--cmdline", "twrpfastboot=1 printk.devkmsg=on", "--output", str(image)], check=True)
ramdisk.unlink()
print(f"PBRP RECOVERY: {image}; OS {os_version}, patch {patch_level}")
