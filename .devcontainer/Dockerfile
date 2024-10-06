# Base image (Ubuntu)
FROM ubuntu:22.04

# Install required packages
RUN apt-get update && apt-get install -y build-essential cmake git gdb python3 && apt-get clean

# Set up a default workspace directory
WORKDIR /workspaces

# Expose the container's workspace for VS Code
VOLUME /workspaces
