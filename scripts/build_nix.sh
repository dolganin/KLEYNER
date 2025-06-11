#!/bin/bash
set -e
BUILD_DIR=build
INSTALL_DIR=bin
mkdir -p $BUILD_DIR
cd $BUILD_DIR
cmake -DCMAKE_BUILD_TYPE=Release -DCPP_STANDARD=20 ..
cmake --build . --config Release
mkdir -p ../$INSTALL_DIR
cp cleaner ../$INSTALL_DIR/ 2>/dev/null || cp Release/cleaner ../$INSTALL_DIR/
cd ..
rm -rf $BUILD_DIR
