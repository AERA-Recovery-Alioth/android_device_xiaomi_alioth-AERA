# Alioth GPU recovery support

Adreno EGL/GLES and a650 firmware are copied unchanged from the current Evo
sm8250-common proprietary tree. They target the Adreno 650 / KGSL 4.19 ABI.
The allocator is built from Evo display commit
385a1c2fa09b9f1ab1d566afa14727e54b6f57dc and vendor display commit
74a5c56b6229dbe518b43cf781e4c52e1a5ecb71.
Kernel UAPI headers come from the actual prebuilt kernel revision a67e83add1fa (unchanged from ac2a025a05e6).
Original copyrights and source histories are retained.

Android 16 uses its own nativewindow/libui. Its Soong lacks exported C flags;
Alioth keeps Evo's reserved-size gralloc handle ABI with explicit compiler flags. Android 17 composer AIDL APIs are
not imported because their framework interfaces do not exist on Android 16.
Runtime GPU initialization and measured frame delivery are checked separately
from compilation and the requested 120 Hz display mode.
