# Final patch series

The published sibling repositories already contain this series on `pbrp-a17`:

| Checkout path | Repository |
| --- | --- |
| `bootable/recovery` | [android_bootable_recovery](https://github.com/pbrp-alioth/android_bootable_recovery/tree/pbrp-a17) |
| `vendor/pb` | [vendor_pb](https://github.com/pbrp-alioth/vendor_pb/tree/pbrp-a17) |
| `external/f2fs-tools` | [android_external_f2fs-tools](https://github.com/pbrp-alioth/android_external_f2fs-tools/tree/pbrp-a17) |
| `system/sepolicy` | [android_system_sepolicy](https://github.com/pbrp-alioth/android_system_sepolicy/tree/pbrp-a17) |

Do not apply patches again when using these branches. The instructions below
are an alternative for reproducing the changes from clean upstream bases.

Apply once to clean sibling repositories using `git am`, in filename order.
Do not apply to the already-patched development checkout.

| Directory | Repository | Baseline |
|---|---|---|
| recovery | bootable/recovery | 4ea56986534da92b29c73ea1907e10e3c0d5ef50 |
| vendor-pb | vendor/pb | 2124e85c72c4d4ff9ef18a7303950d0480257e34 |
| system-sepolicy | system/sepolicy | dd91f58a018a43d70c40abb86dddb85368015b96 |
| f2fs-tools | external/f2fs-tools | a7424d458d4b924be8205986c7b7829c934127d8 |

Recovery patches 0001–0005 retain TeamWin contributors' authorship.
0006 contains the final Alioth adaptation, including GUI/frame pacing,
Magisk fallback, supported partitions and both active-slot ramdisk installers.
The recovery series was replayed from its baseline and its final Git tree
matched the built source exactly. Superseded Alioth patches are removed.

The first vendor patch allows recovery adbd to change context. The initial
system policy patch provides the upstream recovery allowlist; the later 0002
patches below narrow the final policy to the shared recovery domain only.
Android policy and neverallow checks stay enabled.

Recovery 0007 removes the FBE `/data/user/0` bind mount before Data unmount
and verifies/removes the `userdata` metadata-encryption mapping before raw
formatting. Failed normal teardown aborts formatting.

The f2fs-tools patch gives `sload_f2fs` the shared C++ runtime used by its
Android shared dependencies, fixing the startup `std::bad_cast` crash.
Global-permissive diagnostic images are temporary and are not release artifacts.

Recovery 0008 opens existing haptic controls without requesting sysfs file
creation or truncation, and reports failed/short writes. It addresses the
observed activate_mode create denial without widening sysfs permissions.

October 7 enforcement: vendor-pb/0002 removes the six infrastructure
permissive declarations. system-sepolicy/0002 limits the recovery user-build
allowlist to recovery only. Recovery/0009 supplies logd's init-owned control
socket. Device policy adds measured rootfs, keyring, scheduling and exact USB
mode access. Recovery's shared UI/decryption domain remains permissive; do
not describe this as a fully enforcing recovery.

Recovery 0010 preserves the recovery-as-boot ramdisk /system runtime when
mounting Android at /system_root. It also avoids unmounting recovery's /system.
This prevents a mounted Android System from hiding recovery linker/shell/services.

Recovery 0011 prevents the UI/startup non-SAR mount-point switch from moving
Android System to /system on recovery-as-boot. This closes the direct-mount path
that bypassed 0010 during password/UI transitions.

Recovery 0012 runs bundled Magisk in a private child mount namespace. Its own
System mounts cannot hide the parent recovery runtime. Namespace setup failure
aborts before running the updater; the Magisk ZIP remains unmodified.
