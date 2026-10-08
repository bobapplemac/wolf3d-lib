FROM ubuntu:22.04

ARG LLVM_MINGW_RELEASE=20260908
ARG LLVM_MINGW_CRT=ucrt
ARG LLVM_MINGW_SHA256

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install --no-install-recommends -y \
       ca-certificates cmake curl git make ninja-build python3 xz-utils \
       binutils-mingw-w64-i686 binutils-mingw-w64-x86-64 \
    && rm -rf /var/lib/apt/lists/*

RUN test -n "${LLVM_MINGW_SHA256}" \
    && archive="llvm-mingw-${LLVM_MINGW_RELEASE}-${LLVM_MINGW_CRT}-ubuntu-22.04-x86_64.tar.xz" \
    && curl -fL \
       "https://github.com/mstorsjo/llvm-mingw/releases/download/${LLVM_MINGW_RELEASE}/${archive}" \
       -o "/tmp/${archive}" \
    && echo "${LLVM_MINGW_SHA256}  /tmp/${archive}" | sha256sum -c - \
    && mkdir -p /opt/llvm-mingw \
    && tar -xJf "/tmp/${archive}" --strip-components=1 -C /opt/llvm-mingw \
    && rm -f "/tmp/${archive}"

ENV PATH=/opt/llvm-mingw/bin:${PATH}
WORKDIR /src
