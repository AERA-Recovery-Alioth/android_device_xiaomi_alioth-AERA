# Alioth recovery kernel prebuilts

Source: PocoF3Releases/kernel_xiaomi_sm8250, aosp-17,
commit ac2a025a05e6f6d9cf12b020c739019fafb33987.
Built in the Android 17 Alioth checkout; copied from the matching
KERNEL_OBJ Image, product dtb.img and dtbo.img outputs.

BoardConfig uses Image, dtbs/alioth.dtb and dtbo.img.
SHA256SUMS records the exact shipped bytes. No KernelSU integration.
These files were used for the recovery with successful Format Data tests
on Alioth. Aliothin physical validation remains outstanding.
