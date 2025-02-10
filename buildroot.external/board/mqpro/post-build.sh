#!/bin/sh -eu

# add automount for the second partition
mkdir -p ${TARGET_DIR}/mnt/photos
if ! grep -q "^/dev/mmcblk0p2" ${TARGET_DIR}/etc/fstab; then
  echo "/dev/mmcblk0p2	/mnt/photos	auto	nofail,noatime,rw,user	0	0" >> ${TARGET_DIR}/etc/fstab
fi

# replace getty with direct login without authorization
sed -i 's#\(.*\)::respawn:/sbin/getty.*#\1::respawn:/bin/login -f root#' ${TARGET_DIR}/etc/inittab
# add autologin for tty0
if ! grep -q "^tty0::respawn" ${TARGET_DIR}/etc/inittab; then
  sed -i '/.*::respawn:.*/a tty0::respawn:/bin/login -f root' ${TARGET_DIR}/etc/inittab
fi

# setup network
if [ -n "${WIFI_SSID+x}" -a -n "${WIFI_PSWD+x}" ]; then
  wpa_passphrase "${WIFI_SSID}" "${WIFI_PSWD}" | sed '/^\s*#/d' > ${TARGET_DIR}/etc/wpa_supplicant.conf
fi
