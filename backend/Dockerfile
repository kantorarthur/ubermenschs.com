# Stages:
#   deps   the compiler, vcpkg and all libraries (cached; rebuilt only when vcpkg.json changes)
#   build  compiles the server
#   tests  compiles and runs the tests  (docker compose --profile test run --rm backend-tests)
#   (last) the final image: only the finished server, so it stays small

# ---------- deps ----------
FROM ubuntu:24.04 AS deps

# autoconf, automake and libtool: vcpkg builds libsodium with them.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl zip unzip tar pkg-config ca-certificates \
        autoconf autoconf-archive automake libtool \
    && rm -rf /var/lib/apt/lists/*

# vcpkg, pinned to a release so every build gets the same library versions.
ARG VCPKG_TAG=2026.10.16
RUN git clone --depth 1 --branch ${VCPKG_TAG} https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /src

# Install the libraries first. Docker caches this step, so it reruns only when vcpkg.json changes.
COPY vcpkg.json .
RUN /opt/vcpkg/vcpkg install --triplet x64-linux --x-install-root=/src/vcpkg_installed

# System packages added later, in their own step so the libraries above stay cached:
#   tzdata: the time zone database the reminders check their time zones against (the tests need it too);
#   libssl-dev, libcurl4-openssl-dev: OpenSSL and libcurl for sending push notifications.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        tzdata libssl-dev libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

ENV CMAKE_ARGS="-G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
    -DVCPKG_INSTALLED_DIR=/src/vcpkg_installed -DVCPKG_MANIFEST_INSTALL=OFF"

# ---------- build ----------
FROM deps AS build
COPY CMakeLists.txt .
COPY src ./src
RUN cmake -B build ${CMAKE_ARGS} -DBUILD_TESTS=OFF && cmake --build build --target server

# ---------- tests ----------
FROM deps AS tests
COPY CMakeLists.txt .
COPY src ./src
COPY tests ./tests
RUN cmake -B build ${CMAKE_ARGS} -DBUILD_TESTS=ON && cmake --build build --target tests
CMD ["ctest", "--test-dir", "build", "--output-on-failure"]

# ---------- run ----------
FROM ubuntu:24.04

# tzdata: the time zone database (e.g. "Europe/Bucharest") for the reminders.
# libssl3t64, libcurl4t64, ca-certificates: sending push notifications over HTTPS.
RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends \
        tzdata libssl3t64 libcurl4t64 ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# The server does not need to run as root.
RUN useradd --system --no-create-home app
COPY --from=build /src/build/server /usr/local/bin/server
USER app

EXPOSE 8080
CMD ["server"]
