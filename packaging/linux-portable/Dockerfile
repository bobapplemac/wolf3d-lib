FROM debian:10-slim@sha256:bb3dc79fddbca7e8903248ab916bb775c96ec61014b3d02b4f06043b604726dc

LABEL org.opencontainers.image.title="wolf3d-lib Debian 10 portable builder" \
      org.opencontainers.image.description="Library-only glibc 2.28 Linux release toolchain"

ARG CMAKE_VERSION=3.31.6
ARG CMAKE_SHA256=5a1133ff103c71eb5120e2cc3de922733e7d8a26a98ae716397e8676adb367bf
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
        xz-utils \
    && rm -rf /var/lib/apt/lists/*

RUN curl -fsSLo /tmp/cmake.tar.gz \
        "https://github.com/Kitware/CMake/releases/download/v${CMAKE_VERSION}/cmake-${CMAKE_VERSION}-linux-x86_64.tar.gz" \
    && printf '%s  %s\n' "${CMAKE_SHA256}" /tmp/cmake.tar.gz \
        | sha256sum --check --strict - \
    && mkdir -p /opt/cmake \
    && tar -xzf /tmp/cmake.tar.gz -C /opt/cmake --strip-components=1 \
    && ln -s /opt/cmake/bin/cmake /usr/local/bin/cmake \
    && ln -s /opt/cmake/bin/ctest /usr/local/bin/ctest \
    && rm -f /tmp/cmake.tar.gz

WORKDIR /src
