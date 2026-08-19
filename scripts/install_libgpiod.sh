readonly LIB=libgpiod
readonly VERSION=2.3
readonly CMAKE_BUILD_ROOT=$(pwd)
readonly TARGET_LIB=$LIB-$VERSION.tar.xz
readonly SOURCE_PATH=$CMAKE_BUILD_ROOT/$LIB/libgpiod-$VERSION
readonly BUILD_PATH=$CMAKE_BUILD_ROOT/$LIB/build

mkdir -p $SOURCE_PATH

if [[ ! -f  $TARGET_LIB ]]; then
    echo "Downloading libgpio $CMAKE_C_COMPILER $CC"
    wget --quiet https://mirrors.edge.kernel.org/pub/software/libs/libgpiod/$TARGET_LIB
fi


cd $LIB

tar xf $CMAKE_BUILD_ROOT/$TARGET_LIB

mkdir -p $BUILD_PATH && cd $BUILD_PATH
meson setup --prefix=$INSTALL_PATH --buildtype=release $SOURCE_PATH --cross-file=$MESON_TOOLCHAIN_FILE -Dtests=disabled
echo "Building $LIB-$VERSION..."
ninja
ninja install
