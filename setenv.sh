export JAM_PATH=$(pwd)/build
export BIN_PATH=$(pwd)/bin
export PATH=$JAM_PATH:$BIN_PATH:$PATH
chmod a+x $JAM_PATH/jam
if [ ! -d "lib" ]; then
    tar zxf deps/lib.tar.gz
fi
if [ ! -d "include" ]; then
    tar zxf deps/include.tar.gz 
fi
echo "Linux Jam Build System is OK"

