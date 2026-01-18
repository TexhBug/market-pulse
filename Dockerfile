
# ---------- Build stage ----------
FROM ubuntu:22.04 AS builder
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    pkg-config \
    libwebsockets-dev \
    libssl-dev \
    libuv1-dev \
    libev-dev \
    zlib1g-dev \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy project
COPY CMakeLists.txt /app/
COPY src /app/src
COPY include /app/include
COPY tests /app/tests

# Build
RUN mkdir -p build && cd build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . --parallel

# ---------- Runtime stage ----------
FROM ubuntu:22.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    libwebsockets16 \
    libssl3 \
    libuv1 \
    libev4 \
    ca-certificates \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY --from=builder /app/build/bin/orderbook /app/orderbook

# Render typically provides PORT; set a default if your app reads it
ENV PORT=10000
EXPOSE 10000

CMD ["/app/orderbook", "--headless", "--auto-start"]
