# Alioth recovery WLAN MAC provider

Adapted from Daniel210191's official AERA Infiniti commit
01c7a838c71ca76999ca4106b550240101c91c1f. The unmodified import is a separate
commit. Upstream linux-msm/qrtr BSD licensing is retained.

The Alioth service answers DMS2/v1 WLAN MAC0x005c requests on QRTR. It reads
Xiaomi wlan0 provisioning from the read-only persist mount, then existing
qcacld MAC firmware files. Existing serial fallback is retained only if valid
factory data is unavailable. It writes no calibration, Android storage or boot
partitions. A real DMS provider takes precedence; unsupported operations are
not reported as successful modem operations. Service failure retains the
driver's existing MAC-file fallback.

Starts after QRTR on boot, before CNSS is requested. Android logcat records
provider readiness/replies without MAC values. No assumption is made about
Infiniti's byte reversal: Alioth must retain its observed factory address.

Prior Alioth evidence includes an approximately18-second DMS thread join and
another approximately11-second firmware MAC wait. Timing improvement and
identity retention require the rebuilt image's runtime logs.
