FROM debian:12-slim

ARG OPEN_WATCOM_RELEASE=2026-10-01-Build
ARG OPEN_WATCOM_INSTALLER=open-watcom-2_0-c-linux-x64
ARG OPEN_WATCOM_SHA256=a961f3e02ce27bcd88428345dc57483457a623b6e718b6cee52c48d24ab8065f

RUN apt-get update \
    && apt-get install --no-install-recommends -y ca-certificates curl unzip \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /opt/watcom \
    && curl -fL \
       "https://github.com/open-watcom/open-watcom-v2/releases/download/${OPEN_WATCOM_RELEASE}/${OPEN_WATCOM_INSTALLER}" \
       -o /tmp/open-watcom-installer \
    && echo "${OPEN_WATCOM_SHA256}  /tmp/open-watcom-installer" \
       | sha256sum -c - \
    && unzip -q /tmp/open-watcom-installer -d /opt/watcom \
    && chmod a+x /opt/watcom/binl/* /opt/watcom/binl64/* \
    && rm -f /tmp/open-watcom-installer

ENV WATCOM=/opt/watcom
ENV PATH=/opt/watcom/binl64:/opt/watcom/binl:${PATH}
ENV EDPATH=/opt/watcom/eddat
ENV INCLUDE=/opt/watcom/h:/opt/watcom/h/dos

WORKDIR /src
