## Build Instructions

### Prerequisites 
**macOS (Homebrew):**
```bash
brew install opencv
```
**Windows (vcpkg):**

```bash
vcpkg install opencv:x64-windows
```
#### Then configure: 

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
```

**Linux:**
```bash
sudo apt-get install libopencv-dev
```

### Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### Run
```bash
./Airchestra_artefacts/Airchestra
```

# Install all these on Windows
winget install -e --id Kitware.CMake

# Build instructions with build.bat
- Full setup and build:
`./build.bat build-all`
- Compile existing build:
`./build.bat compile`
- Run compiled exe:
`./build.bat run`
- Compile and run:
`./build.bat compile-and-run`
- Remove the build folder:
`./build.bat clean`

# Ultevis
ICT Project for course II1305 at KTH. 

Project Members:

Alex Ryström\
Petros Efraim Koukios\
Raghav Khurana\
Sachin Prabhu Ram\
Sebastian Tryscien\
Stanislaw Jerzy Rybak\
Dajue Qiu
