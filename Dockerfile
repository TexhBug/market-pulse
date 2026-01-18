# Build stage
FROM ubuntu:22.04 AS builder

# Avoid prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libwebsockets-dev \
    libssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source
WORKDIR /app
COPY CMakeLists.txt ./
COPY src/ ./src/
COPY include/ ./include/
COPY tests/ ./tests/

# Build
RUN mkdir build && cd build && \
    cmake .. && \
    make orderbook

# Runtime stage
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libwebsockets16 \
    libssl3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /app/build/bin/orderbook ./orderbook

# Render uses PORT env variable
ENV PORT=8080
EXPOSE 8080

CMD ["./orderbook", "--headless", "--auto-start"]
