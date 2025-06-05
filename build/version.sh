#!/bin/sh

version=$(git describe --tags --dirty --always 2>/dev/null || echo "v0.0.0-unknown")

cat <<EOF >../src/version.h
#ifndef VERSION_H
#define VERSION_H

#define APP_VERSION "${version}"

#endif
EOF
