FROM ubuntu:25.04

ENV DEBIAN_FRONTEND=noninteractive
RUN apt update && apt upgrade --yes && apt install --no-install-recommends --yes \
    bc \
    build-essential \
    ca-certificates \
    ccache \
    cpio \
    file \
    git \
    rsync \
    screen \
    unzip \
    wget

USER ubuntu
