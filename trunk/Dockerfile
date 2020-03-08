FROM ubuntu:18.04
WORKDIR /fteqw
RUN apt-get update \
 && apt-get install -y \
    curl \
    gcc \
    libgnutls28-dev \
    libpng-dev \
    make \
    mesa-common-dev \
    subversion \
    zlib1g-dev \
 && rm -rf /var/lib/apt/lists/*
COPY trunk/ fteqw/
RUN cd /fteqw/engine/ \
 && make sv-rel -j`nproc` \
 && make gl-rel -j`nproc` \
 && make gl-rel FTE_TARGET=win64 -j`nproc`
