./configure --disable-shared --prefix=$INSTALL_PATH
make -j$(nproc)
make install