FROM alpine:3.20.10@sha256:d9e853e87e55526f6b2917df91a2115c36dd7c696a35be12163d44e6e2a4b6bc

LABEL org.opencontainers.image.title="wolf3d-lib Alpine musl builder" \
      org.opencontainers.image.description="Reproducible shared-library build against musl libc"

RUN apk add --no-cache \
        binutils \
        build-base \
        clang \
        cmake \
        file \
        make

WORKDIR /src
