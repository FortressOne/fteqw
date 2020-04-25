FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive
WORKDIR /fteqw/
RUN apt-get update \
 && apt-get install -y \
    build-essential \
    cmake \
    libasound2-dev \
    libbz2-dev \
    libegl1-mesa-dev \
    # libbullet-dev \
    libfreetype6-dev \
    libgnutls28-dev \
    libjpeg-dev \
    libpng-dev \
    libvorbis-dev \
    libvulkan-dev \
    libwayland-dev \
    libxcursor-dev \
    libxkbcommon-dev \
    libxrandr-dev \
    libavdevice-dev \
 && rm -rf /var/lib/apt/lists/*
COPY entrypoint.sh/ /tmp/
CMD ["/tmp/entrypoint.sh"]
