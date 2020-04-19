FROM debian:oldstable
WORKDIR /fteqw
RUN apt-get update \
 && apt-get install -y \
    build-essential \
    cmake \
    libpng-dev \
    libgl1-mesa-dev \
    mesa-common-dev \
 && rm -rf /var/lib/apt/lists/*
COPY trunk/ /fteqw/
RUN mkdir /tmp/build \
 && cd /tmp/build \
 && cmake /fteqw/ \
 && make
