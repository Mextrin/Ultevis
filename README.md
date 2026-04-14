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

**Linux:**
```bash
sudo apt-get install libopencv-dev
```

#### Then configure: 

```bash
cmake .. -DCMAKE_TOOLCHAIN_FILE=path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
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
