# Final patch series

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

The vendor patch allows recovery adbd to change context. The system policy
patch permits only PBRP's seven upstream permissive recovery service domains
in user recovery builds. Android policy and neverallow checks stay enabled.

Recovery 0007 removes the FBE `/data/user/0` bind mount before Data unmount
and verifies/removes the `userdata` metadata-encryption mapping before raw
formatting. Failed normal teardown aborts formatting.

The f2fs-tools patch gives `sload_f2fs` the shared C++ runtime used by its
Android shared dependencies, fixing the startup `std::bad_cast` crash.
Global-permissive diagnostic images are temporary and are not release artifacts.

Recovery 0008 opens existing haptic controls without requesting sysfs file
creation or truncation, and reports failed/short writes. It addresses the
observed activate_mode create denial without widening sysfs permissions.
