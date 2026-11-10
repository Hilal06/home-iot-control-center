# ==========================================
# STAGE 1: Build Application Binary
# ==========================================
FROM rust:1.88-slim AS builder

WORKDIR /app

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    pkg-config \
    libssl-dev \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Cache dependency layer
COPY Cargo.toml Cargo.lock ./
RUN mkdir -p src init-db && \
    echo 'fn main() {}' > src/main.rs && \
    touch init-db/01-schema.sql && \
    cargo build --release && \
    rm -rf src

# Copy real source code and migrations
COPY src ./src
COPY init-db ./init-db

# Touch main.rs so cargo detects source change and rebuilds release binary
RUN touch src/main.rs && cargo build --release

# ==========================================
# STAGE 2: Minimal Runtime Image
# ==========================================
FROM debian:bookworm-slim AS runtime

WORKDIR /app

# Install minimal runtime certificates
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy compiled binary from builder stage
COPY --from=builder /app/target/release/home-iot-backend /usr/local/bin/home-iot-backend

# Copy database init scripts for reference or runtime fallback
COPY init-db /app/init-db

# Default environment configuration
ENV PORT=8080 \
    HOST=0.0.0.0 \
    RUST_LOG=home_iot_backend=info,tower_http=debug

EXPOSE 8080

HEALTHCHECK --interval=15s --timeout=3s --start-period=5s --retries=3 \
    CMD curl -f http://localhost:8080/health || exit 1

ENTRYPOINT ["home-iot-backend"]
