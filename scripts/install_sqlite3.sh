
if [[ -e $INSTALL_PATH/lib/libsqlite3.a ]]; then
    exit 0;
fi

./configure --disable-shared --prefix=$INSTALL_PATH
make -j$(nproc)
make install