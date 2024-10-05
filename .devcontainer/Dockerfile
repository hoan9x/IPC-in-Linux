# Base image (Ubuntu)
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && \
    apt-get install -y \
    build-essential \   # GCC, G++, Make, etc.
    cmake \             # CMake
    git \               # Git
    gdb \               # Debugger
    python3 \
    && apt-get clean

# Set up a default workspace directory
WORKDIR /workspace

# Expose the container's workspace for VS Code
VOLUME /workspace
