FROM ubuntu:18.04
ARG DEBIAN_FRONTEND="noninteractive"
ENV TZ=America/New_York
RUN apt update && apt install -y \
  build-essential \
  automake \
  curl \
  dejagnu \
  libreadline-dev \
  git \
  texinfo \
  python3.8 \
  yasm \
  zlib1g-dev \
  && rm -rf /var/lib/apt/lists/*

RUN curl -L -o cmake.sh https://github.com/Kitware/CMake/releases/download/v3.21.1/cmake-3.21.1-linux-x86_64.sh \
  && chmod +x cmake.sh \
  && bash ./cmake.sh --skip-license \
  && rm cmake.sh

RUN useradd -m -s /bin/bash build

USER build

RUN git clone https://github.com/winmutase21/WinMutASE21Artifact /home/build/WinMutASE21Artifact \
  && cd /home/build/WinMutASE21Artifact \
  && cp -r experiments ../ \
  && mkdir cmake-build-release-tmp \
  && cd cmake-build-release-tmp \
  && cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=True -DLLVM_ENABLE_CXX1Z=True -DCMAKE_INSTALL_PREFIX:PATH=/home/build/winmut \
  && make -j`nproc` \
  && make install \
  && cd /home/build \
  && rm -r WinMutASE21Artifact

ENV WINMUT_BASE_DIR=/home/build/winmut
# RUN cd /home/build/experiments/new-subjects \
#  && ./setup.sh \
#  && ./buildall.sh grep libpng binutils coreutils gmp libsodium lz4 pcre2 lua \
#  && find . -name '*.ll' | xargs -n1 rm

WORKDIR /home/build

