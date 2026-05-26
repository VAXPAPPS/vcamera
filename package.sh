#!/bin/bash

# Exit on any error
set -e

# Package details
APP_NAME="vcamera"
VERSION="1.0.0"
ARCH="amd64"
MAINTAINER="Developer"
DESCRIPTION="A native and clean GTK4 Camera Application with QR support."

# Directories
BUILD_DIR="build_deb"
PKG_NAME="${APP_NAME}_${VERSION}_${ARCH}"
PKG_DIR="${BUILD_DIR}/${PKG_NAME}"

echo "🔨 Building the project with meson/ninja..."
meson setup build --reconfigure || meson setup build
meson compile -C build

echo "🧹 Cleaning previous build..."
rm -rf ${BUILD_DIR}
mkdir -p ${PKG_DIR}/DEBIAN
mkdir -p ${PKG_DIR}/usr/bin
mkdir -p ${PKG_DIR}/usr/share/applications
mkdir -p ${PKG_DIR}/usr/share/icons/hicolor/scalable/apps

echo "📦 Copying binary..."
cp build/src/vcamera ${PKG_DIR}/usr/bin/vcamera
chmod +x ${PKG_DIR}/usr/bin/vcamera

echo "🎨 Copying icon..."
if [ -f "data/ui/vcamera.svg" ]; then
    cp data/ui/vcamera.svg ${PKG_DIR}/usr/share/icons/hicolor/scalable/apps/${APP_NAME}.svg
else
    echo "⚠️  Warning: Icon not found at data/ui/vcamera.svg!"
fi

echo "📝 Creating desktop entry..."
cat <<EOF > ${PKG_DIR}/usr/share/applications/${APP_NAME}.desktop
[Desktop Entry]
Name=VCamera
Comment=Take photos and record videos
Exec=vcamera
Icon=vcamera
Terminal=false
Type=Application
Categories=AudioVideo;Video;Graphics;Photography;GNOME;GTK;
Keywords=Camera;Video;Photo;Record;Webcam;QR;Barcode;Scanner;كاميرا;فيديو;صور;تسجيل;باركود;
MimeType=image/jpeg;image/png;video/mp4;video/webm;
EOF

echo "📄 Creating control file..."
cat <<EOF > ${PKG_DIR}/DEBIAN/control
Package: ${APP_NAME}
Version: ${VERSION}
Section: video
Priority: optional
Architecture: ${ARCH}
Maintainer: ${MAINTAINER} <dev@example.com>
Description: ${DESCRIPTION}
Depends: libgtk-4-1, libgstreamer1.0-0, libgstreamer-plugins-base1.0-0
EOF

echo "📦 Building .deb package..."
dpkg-deb --build ${PKG_DIR}

echo "✅ Package created successfully: ${BUILD_DIR}/${PKG_NAME}.deb"
