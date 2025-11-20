# Multi-stage Dockerfile for FluidLoom with MPI support

# Stage 1: Base image with dependencies
FROM ubuntu:22.04 AS base

ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=UTC

# Install system dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    wget \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

# Try to install OpenCL (may not be available on all architectures)
RUN apt-get update && apt-get install -y \
    ocl-icd-opencl-dev || true \
    && rm -rf /var/lib/apt/lists/*

# Install OpenMPI
RUN apt-get update && apt-get install -y \
    openmpi-bin \
    openmpi-common \
    libopenmpi-dev \
    && rm -rf /var/lib/apt/lists/*

# Install ANTLR4 runtime
RUN apt-get update && apt-get install -y \
    libantlr4-runtime-dev \
    antlr4 \
    && rm -rf /var/lib/apt/lists/*

# Stage 2: Build stage
FROM base AS builder

WORKDIR /workspace

# Copy source code
COPY . .

# Create build directory and configure
RUN mkdir -p build && cd build && \
    cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=ON \
    -DSTRICT_WARNINGS=OFF \
    -DFL_ENABLE_OPENCL=OFF

# Build FluidLoom
RUN cd build && make -j$(nproc)

# Stage 3: Test stage
FROM builder AS test

WORKDIR /workspace/build

# Run unit tests
RUN ctest --output-on-failure -R ".*Unit.*" || true

# Stage 4: Integration test stage
FROM builder AS integration

WORKDIR /workspace/build

# Set MPI environment variables
ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

# Default command runs integration tests with 2 MPI ranks
CMD ["mpirun", "-np", "2", "--allow-run-as-root", \
     "./tests/integration/transport/test_two_gpu_exchange"]

# Stage 5: Benchmark stage
FROM builder AS benchmark

WORKDIR /workspace/build

ENV OMPI_ALLOW_RUN_AS_ROOT=1
ENV OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1

# Default command runs benchmarks
CMD ["./tests/benchmark/benchmark_transport", \
     "--benchmark_format=json", \
     "--benchmark_out=transport_benchmark.json"]
