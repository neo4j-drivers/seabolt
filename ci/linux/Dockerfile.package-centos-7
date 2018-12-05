FROM centos:centos7
RUN yum -y install openssl-devel openssl-static wget pkg-config ca-certificates make gcc-c++ pkgconfig git rpm-build \
        && (mkdir -p /cmake && wget --no-verbose --output-document=- https://cmake.org/files/v3.12/cmake-3.12.3-Linux-x86_64.tar.gz | tar xvz -C /cmake --strip 1) \
        && yum -y clean all
ARG SEABOLT_VERSION
ARG SEABOLT_VERSION_HASH
ENV SEABOLT_VERSION=$SEABOLT_VERSION
ENV SEABOLT_VERSION_HASH=$SEABOLT_VERSION_HASH
ADD . /tmp/seabolt
WORKDIR /tmp/seabolt/build-docker
RUN /cmake/bin/cmake -D CMAKE_BUILD_TYPE=Release /tmp/seabolt \
    && /cmake/bin/cmake --build . --target package
CMD tar -czf - dist-package/seabolt*
