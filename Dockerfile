# File: Dockerfile
# Purpose: Build and run the dependency-free headless demonstration in a non-root container.
FROM ubuntu:24.04 AS build

RUN apt-get update \
    && DEBIAN_FRONTEND=noninteractive apt-get install --yes --no-install-recommends \
       ca-certificates cmake g++ ninja-build \
    && rm -rf /var/lib/apt/lists/*
WORKDIR /source
COPY . .
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build --target forge2d_arena forge2d_tests forge2d_benchmark --parallel \
    && ctest --test-dir build --output-on-failure \
    && ./build/forge2d_benchmark

FROM ubuntu:24.04 AS runtime
RUN useradd --create-home --uid 10001 forge2d \
    && mkdir /app /output \
    && chown forge2d:forge2d /output
WORKDIR /app
COPY --from=build /source/build/forge2d_arena /app/forge2d_arena
COPY --from=build /source/assets /app/assets
USER forge2d
VOLUME ["/output"]
ENTRYPOINT ["/app/forge2d_arena", "/app/assets/arena.scene", "/output"]
