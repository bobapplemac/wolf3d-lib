FROM debian:10-slim@sha256:bb3dc79fddbca7e8903248ab916bb775c96ec61014b3d02b4f06043b604726dc

LABEL org.opencontainers.image.title="wolf3dgeneric Debian 10 portable builder" \
      org.opencontainers.image.description="Project-local glibc 2.28 Linux release toolchain"

ARG CMAKE_VERSION=3.31.6
ARG CMAKE_SHA256=5a1133ff103c71eb5120e2cc3de922733e7d8a26a98ae716397e8676adb367bf
ARG WAYLAND_VERSION=1.18.0
ARG WAYLAND_SHA256=102d5556db1524a7737ab76e358b03e5b92a7718f1ac1520bf9686bfd09c8e4a

# Debian 10 has moved from the normal mirrors to archive.debian.org. Its
# glibc 2.28 userspace is intentional: every staged ELF must remain usable on
# that ABI or newer. CMake is only a build tool and does not affect that floor.
RUN printf '%s\n' \
        'deb http://archive.debian.org/debian buster main' \
        'deb http://archive.debian.org/debian buster-updates main' \
        'deb http://archive.debian.org/debian-security buster/updates main' \
        > /etc/apt/sources.list \
    && printf '%s\n' \
        'Acquire::Check-Valid-Until "false";' \
        > /etc/apt/apt.conf.d/99archive \
    && apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        binutils \
        build-essential \
        ca-certificates \
        curl \
        file \
        git \
        libasound2-dev \
        libdrm-dev \
        libegl1-mesa-dev \
        libexpat1-dev \
        libffi-dev \
        libgbm-dev \
        libgl1-mesa-dev \
        libpulse-dev \
        libudev-dev \
        libxml2-dev \
        libx11-dev \
        libxcursor-dev \
        libxext-dev \
        libxfixes-dev \
        libxi-dev \
        libxinerama-dev \
        libxkbcommon-dev \
        libxrandr-dev \
        libxss-dev \
        libxtst-dev \
        pkg-config \
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

# SDL 3.4 needs Wayland 1.18 headers and wayland-scanner, while Debian 10
# supplies 1.16. Build only that toolchain against Debian 10's libc. SDL owns
# the protocol XML it generates, and dynamically loads the destination's
# Wayland client libraries, so no Wayland library is added to the package.
RUN curl -fsSLo /tmp/wayland.tar.gz \
        "https://deb.debian.org/debian/pool/main/w/wayland/wayland_${WAYLAND_VERSION}.orig.tar.gz" \
    && printf '%s  %s\n' "${WAYLAND_SHA256}" /tmp/wayland.tar.gz \
        | sha256sum --check --strict - \
    && mkdir -p /tmp/wayland-source \
    && tar -xzf /tmp/wayland.tar.gz \
        -C /tmp/wayland-source --strip-components=1 \
    && cd /tmp/wayland-source \
    && ./configure \
        --prefix=/opt/wayland \
        --libdir=/opt/wayland/lib \
        --disable-documentation \
    && make -j2 \
    && make install \
    && rm -rf /tmp/wayland.tar.gz /tmp/wayland-source

RUN curl -fsSLo /tmp/cmake.tar.gz \
        "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    && printf '%s  %s\n' "${CMAKE_SHA256}" /tmp/cmake.tar.gz \
        | sha256sum --check --strict - \
    && mkdir -p /opt/cmake \
    && tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1 \
    && ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake \
    && ln -s /opt/cmake/bin/ctest /usr/local/bin/ctest \
    && rm -f /tmp/cmake.tar.gz

ENV PATH="/opt/wayland/bin:${PATH}"
ENV PKG_CONFIG_PATH="/opt/wayland/lib/pkgconfig"
ENV LD_LIBRARY_PATH="/opt/wayland/lib"

WORKDIR /src
