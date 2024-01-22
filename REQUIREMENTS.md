# Requirements

## Hardware

Note that building WinMut requires a lot of time (maybe *hours*), and the experiments need a lot of computation resources.
For reference, our experiment reproducing VMWare image has 16GB of memory, 4 Intel-I7 cores and 60GB of disk space, which is capable of reproduction.
The docker image requires at least 20GB of free disk space.

## Software

We tested our artifact on Ubuntu 18.04 LTS with glibc 2.27. It contains assembly code and only works on x86-64 platform.
The following scripts are based on Ubuntu 18.04 LTS.
### Dependencies
#### Dependencies for building WinMut
You need a C++17 compatible compiler and cmake 3.13.0+ to build WinMut.
You can install them with the following script
```shell
apt install build-essential curl git
## You need cmake 3.13.0+ to build WinMut.
curl -L https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-linux-x86_64.sh | bash
```
#### Dependencies for the subjects
The building and the testing of the subject softwares requires the following dependencies.
```
apt install automake dejagnu libreadline-dev texinfo yasm zlib1g-dev
```
#### Dependencies for executing the evaluation scripts
You need python3.7+ to execute the scripts.
```
apt install python3.8
```

## Docker image
You can also read this [Dockerfile](docker/ns/Dockerfile) to see how we build WinMut.
