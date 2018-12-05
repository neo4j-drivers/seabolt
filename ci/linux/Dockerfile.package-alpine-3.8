FROM alpine:3.8
RUN apk add --no-cache ca-certificates cmake make g++ openssl-dev git
ARG SEABOLT_VERSION
ARG SEABOLT_VERSION_HASH
ENV SEABOLT_VERSION=$SEABOLT_VERSION
ENV SEABOLT_VERSION_HASH=$SEABOLT_VERSION_HASH
ADD . /tmp/seabolt
WORKDIR /tmp/seabolt/build-docker
RUN cmake -D CMAKE_BUILD_TYPE=Release /tmp/seabolt \
    && cmake --build . --target package
CMD tar -czf - dist-package/seabolt*
