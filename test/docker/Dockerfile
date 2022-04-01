FROM ubuntu:20.04

RUN apt -y update && \
    apt -y upgrade && \
    apt -y install make \
                   clang-12 \
                   python3 \
                   netcat-traditional \
                   uuid-runtime

RUN mkdir -p /build

COPY . /build
WORKDIR /build

CMD make test

