# Installing WinMut

## Build from source
We hardcoded the path `${PROJECT_ROOT}/cmake-bulid-release` in our evaluation script. If you don't want to install the
library elsewhere, you can just run
```shell
mkdir cmake-build-release
cd cmake-build-release
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=True -DLLVM_ENABLE_CXX1Z=True
make
```

If you want to install the compiler and the library somewhere, please run
```shell
mkdir build
cd build
cmake .. -DCMAKE_INSTALL_PREFIX:PATH=<<<where you want to install the artifact>>> \
 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=True -DLLVM_ENABLE_CXX1Z=True
make && make install
```

**Caveat**: the current WinMut implementation contains the full LLVM 6.0 source tree. Building it from scratch may take a long time.
For your convenience, we provided the docker image.

## Test the build
To test if you have built WinMut successfully, you can run the script `test.sh` at `experiments/test-installation`. It would print
`Success` if WinMut works.

The following script tests the installation at `${PROJECT_ROOT}/cmake-build-release`. If you installed WinMut elsewhere, you should set the environment variable `WINMUT_BASE_DIR` for `test.sh`.
```shell
cd experiments/test-installation/
./test.sh
```

## Setting up the subjects
1. Go to the `experiment/new-subjects` directory
   ```shell
   cd experiments/new-subjects
   ```
2. Download the subject projects and patch them
   ```shell
   ./setup.sh
   ```
   The subject projects will be directly downloaded from the project websites.
   Some of them are patched to make it work with our compiler.
   You can read the patch files at `experiments/new-subjects`.
   - lz4 is patched to remove the -g flag passed to the compiler.
   - lua is patched to add the correct CFLAGS.
   - ffmpeg is patched to remove several test cases that won't work with our runtime.
3. You can build the subjects with [experiments/new-subjects/buildall.sh](experiments/new-subjects/buildall.sh).
   ```shell
   ./buildall.sh <<<names of the subjects to build>>>
   # The supported subject names are grep, libpng, binutils, coreutils, gmp, libsodium, lz4, pcre2, lua, ffmpeg
   ```
   The script should report success (exited normally) for most subjects, but it is okay to report a non-zero exit code
   for `binutils` under WinMut/AccMut algorithm.
   
**Caveat**: building ffmpeg takes a long time and requires tens of gigabytes of storage.

## Docker
We also provided docker images for WinMut.

The image `lsrcz/winmut:ns1.0.3` contains the WinMut compiler.
The image `lsrcz/winmut:nf1.0.3` contains the WinMut compiler and all subjects (except for ffmpeg) pre-built.
We did not include ffmpeg in the docker image because building it requires too much storage and
it's hard to remove the intermediate files.

`WINMUT_BASE_DIR` are already set in these images.

### Usage of the docker image
```shell
docker pull lsrcz/winmut:ns1.0.3
docker run -it lsrcz/winmut:ns1.0.3
cd experiments/new-subjects/
./run.sh pcre2 run WinMut 10
```

### Warning

For some unknown reasons (maybe our docker environment setting errors), *libsodium* may randomly failed on serveral tests in the first execution, but it can pass all the test in the second execution.

