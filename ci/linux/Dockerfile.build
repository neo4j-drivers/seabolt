FROM debian:stretch AS build
RUN apt update \
        && apt install -y ca-certificates make g++ libssl-dev git wget pkg-config \
        && (mkdir -p /cmake && wget --no-verbose --output-document=- https://cmake.org/files/v3.12/cmake-3.12.3-Linux-x86_64.tar.gz | tar xvz -C /cmake --strip 1) \
        && rm -rf /var/lib/apt/lists/*
ADD . /tmp/seabolt
WORKDIR /tmp/seabolt/build-docker
RUN /cmake/bin/cmake -D CMAKE_BUILD_TYPE=Debug -D CMAKE_INSTALL_PREFIX=dist /tmp/seabolt \
    && /cmake/bin/cmake --build . --target install

FROM openjdk:11-jdk
RUN apt update \
    && apt install -y bash python3 python3-pip openjdk-8-jdk \
    && python3 -m pip install boltkit==1.2.0 \
    && rm -rf /var/lib/apt/lists/* \
    && mkdir /java \
    && ln -s /usr/lib/jvm/java-11-openjdk-amd64 /java/jdk-4.0 \
    && ln -s /usr/lib/jvm/java-8-openjdk-amd64 /java/jdk-3.5 \
    && ln -s /usr/lib/jvm/java-8-openjdk-amd64 /java/jdk-3.4 \
    && ln -s /usr/lib/jvm/java-8-openjdk-amd64 /java/jdk-3.3 \
    && ln -s /usr/lib/jvm/java-8-openjdk-amd64 /java/jdk-3.2
ENV NEOCTRLARGS="-e 3.4"
ENV TEAMCITY_HOST="" TEAMCITY_USER="" TEAMCITY_PASSWORD=""
ENV BOLT_HOST="localhost" BOLT_PORT="7687" HTTP_PORT="7474" HTTPS_PORT="7473"
ENV BOLT_PASSWORD="password"
ADD run_tests.sh /seabolt/
COPY --from=build /tmp/seabolt/build-docker/bin /seabolt/build/bin/
COPY --from=build /tmp/seabolt/build-docker/dist /seabolt/build/dist/
WORKDIR /seabolt
CMD PYTHON=python3 JAVA_HOME=/java/jdk-`echo $NEOCTRLARGS | sed -E 's/-e\s*//g' | cut -d. -f1,2` ./run_tests.sh
