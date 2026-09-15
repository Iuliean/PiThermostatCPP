readonly BUILD_DIR=spdlog-build


cmake -S . -B $BUILD_DIR \
    -DCMAKE_INSTALL_PREFIX=$INSTALL_PATH \
    -DSPDLOG_USE_STD_FORMAT=ON \
    -DCMAKE_BUILD_TYPE="Release"

cmake --build $BUILD_DIR
cmake --install $BUILD_DIR