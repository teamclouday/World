#!/bin/bash
# change the build type here for different usages (Debug, Release)
# target platform will always be x86
buildtype=Debug
if [ ! -d "build" ]; then
    echo Creating "build" folder
    mkdir build
fi
if [ ! -f "external/external.zip" ]; then
    git pull origin master
fi
if [ ! -d "external/glfw" ]; then
    echo Extracting external.zip
    unzip external/external.zip -d external/
fi
cd build
echo Running Cmake File
echo Current Build Type is "${buildtype}"
cmake -DCMAKE_BUILD_TYPE=${buildtype} ..
echo Cmake File Generated
echo Building
cmake --build .
cd ..
echo Compiling All Shaders
cd shaders
for folder in *; do
    if [[ -d "$folder" && ! -L "$folder" ]]; then
        cd $folder
        ./compile.sh
        cd ..
    fi
done
cd ..
echo Project Built