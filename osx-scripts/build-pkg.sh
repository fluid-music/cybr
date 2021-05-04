#!/bin/sh

VERSION="0.3.1"
pkgbuild --root ../Builds/MacOSX/build/pkgroot/ \
         --identifier "edu.mit.media.cybr" \
         --version $VERSION \
         --install-location "/" \
         --sign "Developer ID Installer: Tod Machover (2Z44ZRUPZ4)" \
         "../Builds/MacOSX/build/cybr-${VERSION}.pkg"
