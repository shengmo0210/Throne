#!/bin/bash
set -e

VERSION="$1"
ARCH="$2"
SYSTEMQT_SUFFIX=$([[ $3 == "systemqt" ]] && echo "-system-qt")

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
RES_LINUX="$SCRIPT_DIR/../res/linux"

# stage the payload at /opt/Throne
mkdir -p Throne/opt
cp -r "linux-${ARCH}${SYSTEMQT_SUFFIX}" Throne/opt
mv "Throne/opt/linux-${ARCH}${SYSTEMQT_SUFFIX}" Throne/opt/Throne
rm Throne/opt/Throne/Throne.debug

# /usr/bin/Throne launcher (puts /opt/Throne on PATH so the GUI can spawn ThroneCore)
install -D -m 0755 "$RES_LINUX/Throne.launcher.sh" Throne/usr/bin/Throne

# .desktop
install -D -m 0644 "$RES_LINUX/org.throneproj.Throne.desktop" \
    Throne/usr/share/applications/org.throneproj.Throne.desktop

# AppStream metainfo (stamp version + build date)
install -d Throne/usr/share/metainfo
sed -e "s|@@VERSION@@|${VERSION}|g" \
    -e "s|@@DATE@@|$(date -u +%Y-%m-%d)|g" \
    "$RES_LINUX/org.throneproj.Throne.metainfo.xml" \
    > Throne/usr/share/metainfo/org.throneproj.Throne.metainfo.xml

# hicolor icons
for size in 16x16 32x32 48x48 64x64 128x128 256x256 512x512; do
    install -D -m 0644 \
        "$RES_LINUX/icons/hicolor/${size}/apps/org.throneproj.Throne.png" \
        "Throne/usr/share/icons/hicolor/${size}/apps/org.throneproj.Throne.png"
done

# DEBIAN control
mkdir -p Throne/DEBIAN
cat >Throne/DEBIAN/control <<-EOF
Package: Throne
Version: $VERSION
Architecture: $ARCH
Maintainer: Mahdi mahdi.pgitmail@gmail.com
Depends: desktop-file-utils$([[ $3 == "systemqt" ]] && echo ", libqt6core6, libqt6gui6, libqt6network6, libqt6widgets6, qt6-qpa-plugins, qt6-wayland, qt6-gtk-platformtheme, qt6-xdgdesktopportal-platformtheme, libxcb-cursor0, fonts-noto-color-emoji")
Description: Qt based cross-platform GUI proxy configuration manager (backend: sing-box)
EOF

# postinst: refresh desktop DB and clean up any stale file from older packages
cat >Throne/DEBIAN/postinst <<-'EOF'
#!/bin/sh
set -e
# Previous versions wrote /usr/share/applications/Throne.desktop from postinst;
# the new package ships org.throneproj.Throne.desktop instead.
rm -f /usr/share/applications/Throne.desktop
update-desktop-database -q || true
EOF
chmod 0755 Throne/DEBIAN/postinst

sudo dpkg-deb --build Throne
