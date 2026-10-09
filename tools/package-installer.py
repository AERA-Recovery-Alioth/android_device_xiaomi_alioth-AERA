#!/usr/bin/env python3
"""Package a PBRP ramdisk installer; never flash during packaging."""
from pathlib import Path
import hashlib, subprocess, zipfile
root = Path(__file__).resolve().parents[4]
out = root / "out/release-candidate"
host = root / "out/host/linux-x86"
java = root / "prebuilts/jdk/jdk17/linux-x86/bin/java"
signing_jar = host / "framework/signapk.jar"
signing_library = host / "lib64/libconscrypt_openjdk_jni.so"
for required in (java, signing_jar, signing_library):
    if not required.is_file():
        raise SystemExit(f"Missing recovery-local signing tool: {required}; build signapk first")
image = out / "recovery_boot.img"
unsigned = out / "recovery-unsigned.zip"
with zipfile.ZipFile(unsigned, "w", zipfile.ZIP_DEFLATED) as z:
    z.write(image, "recovery_boot.img")
    tool = root / "external/magisk-prebuilt/prebuilt/magiskboot_arm64"
    z.write(tool, "magiskboot")
    z.writestr("recovery_boot.sha256", hashlib.sha256(image.read_bytes()).hexdigest() + "  recovery_boot.img\n" + hashlib.sha256(tool.read_bytes()).hexdigest() + "  magiskboot\n")
    entry = zipfile.ZipInfo("META-INF/com/google/android/update-binary")
    entry.external_attr = 0o100755 << 16
    z.writestr(entry, (Path(__file__).parent / "recovery-installer.sh").read_bytes())
    z.writestr("README.txt", "PBRP ramdisk installer. Preserves current kernel, replaces active Boot ramdisk. No decrypted storage or persistent backup required. Reinstall Magisk afterward. Not a RAM-only launch.\n")
subprocess.run([str(java),
    "-Djava.library.path=" + str(host / "lib64"), "-jar",
    str(signing_jar), "-w",
    str(root / "build/make/target/product/security/testkey.x509.pem"),
    str(root / "build/make/target/product/security/testkey.pk8"),
    str(unsigned), str(out / "recovery.zip")], check=True)
with zipfile.ZipFile(out / "recovery.zip") as z:
    bad_entry = z.testzip()
    if bad_entry is not None:
        raise SystemExit(f"Corrupt signed ZIP entry: {bad_entry}")
    if z.read("recovery_boot.img") != image.read_bytes():
        raise SystemExit("Signed ZIP recovery image does not match candidate")
    for line in z.read("recovery_boot.sha256").decode().splitlines():
        expected, name = line.split("  ", 1)
        if hashlib.sha256(z.read(name)).hexdigest() != expected:
            raise SystemExit(f"Signed ZIP payload checksum mismatch: {name}")
(out / "SHA256SUMS").write_text("".join(hashlib.sha256((out / name).read_bytes()).hexdigest() + "  " + name + "\n" for name in ["recovery_boot.img", "recovery.zip"]))
unsigned.unlink()
print(out / "recovery.zip")
