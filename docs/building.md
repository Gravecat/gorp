# Build Instructions

## Compiling SFML

GORP requires the headers and compiled library files (.a or .lib) for [SFML 3](https://github.com/SFML/SFML). However you choose to acquire these files, you
should place the `include` folder in the `src/3rdparty/sfml` folder of the GORP repo, the library files (.a or .lib) in either `linux/lib` or `windows/lib`
(depending on your target platform), and if using Windows, the DLL files in the `windows/dll` folder.

If cross-compiling on Linux or WSL to create a 64-bit Windows build, an optional MinGW toolchain file has been provided in
[optional/mingw-w64-x86_64.cmake](https://github.com/Gravecat/gorp/blob/main/optional/mingw-w64-x86_64.cmake). Place this file in the root folder of your SFML
repo, and then:

`mkdir build`<br>
`cd build`<br>
`cmake -DCMAKE_TOOLCHAIN_FILE=../mingw-w64-x86_64.cmake -DBUILD_SHARED_LIBS=ON -DCMAKE_BUILD_TYPE=Release ..` (or `Debug` if making a debug build)<br>
`cmake --build .`

## Compiling GORP

### Windows

To compile from VSCode, ensure the VSCode CMake settings are set to x86_64, it should automatically build a Windows binary.

To manually create a Windows binary:<br>
`mkdir build`<br>
`cd build`<br>
`cmake .. -DCROSS_COMPILE_WINDOWS=1 -DCMAKE_BUILD_TYPE=Debug` (or `Release`, or `RelWithDebInfo`)<br>
`cmake --build .`

For Windows cross-compiling, you'll need the apt packages `mingw-w64` and `build-essential` (or equivalent packages for your repo).

### Linux

For Linux binaries, just `cmake .. -DCMAKE_BUILD_TYPE=Debug` (or `Release`, or `RelWithDebInfo`) will suffice.

### 32-Bit Builds

For 32-bit builds (on either platform), add `-DBUILD_I686`.
