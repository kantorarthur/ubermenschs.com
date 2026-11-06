# Two stages: the first compiles the server with all its tools,
# the second contains only the finished program, so the final image stays small.

# ---------- build ----------
FROM ubuntu:24.04 AS build

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential cmake ninja-build git curl zip unzip tar pkg-config ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# vcpkg, pinned to a release so every build gets the same library versions.
ARG VCPKG_TAG=2026.10.16
RUN git clone --depth 1 --branch ${VCPKG_TAG} https://github.com/microsoft/vcpkg.git /opt/vcpkg \
    && /opt/vcpkg/bootstrap-vcpkg.sh -disableMetrics

WORKDIR /src

# Install the libraries first. Docker caches this step, so it reruns only when vcpkg.json changes.
COPY vcpkg.json .
RUN /opt/vcpkg/vcpkg install --triplet x64-linux --x-install-root=/src/vcpkg_installed

COPY CMakeLists.txt .
COPY src ./src
RUN cmake -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=/opt/vcpkg/scripts/buildsystems/vcpkg.cmake \
        -DVCPKG_INSTALLED_DIR=/src/vcpkg_installed \
        -DVCPKG_MANIFEST_INSTALL=OFF \
    && cmake --build build

# ---------- run ----------
FROM ubuntu:24.04

# The server does not need to run as root.
RUN useradd --system --no-create-home app
COPY --from=build /src/build/server /usr/local/bin/server
USER app

EXPOSE 8080
CMD ["server"]
