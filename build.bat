@ECHO OFF
:: change the build type here for different usages
:: target platform will always be x86
SET buildtype=Debug
IF NOT EXIST build (
    ECHO Creating "build" folder
    MKDIR build
)
CD build
ECHO Running Cmake File
ECHO Current Build Type is %buildtype%
cmake -A Win32 -DCMAKE_BUILD_TYPE=%buildtype% ..
ECHO Cmake File Generated
ECHO Building
cmake --build .
CD ..
ECHO Project Successfully Built
PAUSE