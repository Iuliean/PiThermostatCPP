readonly LIB=i2c-tools
readonly VERSION=4.3
readonly CMAKE_BUILD_ROOT=$(pwd)
readonly TARGET_LIB=$LIB-$VERSION.tar.xz
readonly SOURCE_PATH=$CMAKE_BUILD_ROOT/$LIB/$LIB-$VERSION


mkdir -p $SOURCE_PATH

if [[ ! -f $TARGET_LIB ]]; then
    echo "Downloading $LIB..."
    wget --quiet https://www.kernel.org/pub/software/utils/i2c-tools/$LIB-$VERSION.tar.xz
    echo "Downloading $LIB...done"
fi

cd $CMAKE_BUILD_ROOT/$LIB
tar xf $CMAKE_BUILD_ROOT/$TARGET_LIB

cd $LIB-$VERSION
make install-include install-lib PREFIX=$INSTALL_PATH