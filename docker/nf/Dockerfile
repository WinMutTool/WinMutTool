FROM winmut:ns1.0.3
RUN cd /home/build/experiments/new-subjects \
  && ./setup.sh \
  && ./buildall.sh grep libpng binutils coreutils gmp libsodium lz4 pcre2 lua \
  && find . -name '*.ll' | xargs -n1 rm

WORKDIR /home/build

