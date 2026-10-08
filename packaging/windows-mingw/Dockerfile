FROM debian:12-slim

RUN apt-get update \
    && apt-get install --no-install-recommends -y \
       ca-certificates cmake git make ninja-build \
       binutils-mingw-w64-i686 binutils-mingw-w64-x86-64 \
       gcc-mingw-w64-i686-win32 gcc-mingw-w64-x86-64-win32 \
       g++-mingw-w64-i686-win32 g++-mingw-w64-x86-64-win32 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
