FROM debian:oldstable
WORKDIR /fteqw
RUN apt-get update \
 && apt-get install -y \
    build-essential \
    cmake \
    # gcc \
    # make \
    # libfreetype6-dev \
    # libgnutls28-dev \
    libpng-dev \
    libgl1-mesa-dev \
    mesa-common-dev \
    # subversion \
    # zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*
COPY trunk/ /fteqw/
RUN mkdir /tmp/build \
 && cd /tmp/build \
 && cmake /fteqw/ \
 && make
# RUN cd /fteqw/engine/ \
#  && make sv-rel -j`nproc` \
#  && make gl-rel -j`nproc` \
#  && make gl-rel FTE_TARGET=win64 -j`nproc`
