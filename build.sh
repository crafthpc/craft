#!/bin/bash
#
# Helper script for installing and building CRAFT the first time.
#

# environment variables
CRAFT_ROOT="$(pwd)"
EXTERN_ROOT="$CRAFT_ROOT/extern"
DYNINST_ROOT="$EXTERN_ROOT/dyninst"
PIN_ROOT="$EXTERN_ROOT/pin-2.14-71313-gcc.4.4.7-linux"
PIN_FILE="pin-2.14-71313-gcc.4.4.7-linux.tar.gz"
PLATFORM="x86_64-unknown-linux2.4"
SETUP_SH="env-setup.sh"

# create extern folder
mkdir -p "$EXTERN_ROOT"

# clone the Dyninst repository
( cd "$EXTERN_ROOT" && \
    git clone https://github.com/dyninst/dyninst.git dyninst)

# build a known compatible version of Dyninst
( cd "$DYNINST_ROOT" && \
    git checkout v9.1.0 && \
    cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo -DCMAKE_INSTALL_PREFIX=$DYNINST_ROOT/$PLATFORM . && \
    make install -j16 )

# download and extract Pin (for XED)
( cd "$EXTERN_ROOT" && \
    curl -O https://software.intel.com/sites/landingpage/pintool/downloads/$PIN_FILE && \
    tar -xf "$PIN_FILE" && \
    rm "$PIN_FILE" )

# add XED location to CRAFT makefile
XED_PATH="$(sed -e "s:\/:\\\/:g" <<< "$PIN_ROOT")\/extras\/xed-intel64"
sed -i -e "s/^XED_KIT=.*$/XED_KIT=$XED_PATH/" Makefile

# build CRAFT
export DYNINST_ROOT
export PLATFORM
make -j16

# build viewer (requires Java 1.6 or higher)
( cd viewer && make )

# create environment setup script
echo "#!/bin/bash" >"$SETUP_SH"
echo "export PLATFORM=\"$PLATFORM\"" >>"$SETUP_SH"
echo "export DYNINST_ROOT=\"$DYNINST_ROOT\"" >>"$SETUP_SH"
echo "export PATH=\"$CRAFT_ROOT/$PLATFORM:$CRAFT_ROOT/scripts:\$PATH\"" >>"$SETUP_SH"
echo "export LD_LIBRARY_PATH=\"$CRAFT_ROOT/$PLATFORM/:$DYNINST_ROOT/$PLATFORM/lib:.:\$LD_LIBRARY_PATH\"" >>"$SETUP_SH"
echo "export DYNINSTAPI_RT_LIB=\"$DYNINST_ROOT/$PLATFORM/lib/libdyninstAPI_RT.so\"" >>"$SETUP_SH"
chmod +x "$SETUP_SH"

echo "Build complete. Don't forget to 'source $SETUP_SH' before you run anything."

