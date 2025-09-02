FROM ubuntu:22.04

# Avoid interactive prompts during package installation
ENV DEBIAN_FRONTEND=noninteractive

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    python3 \
    python3-pip \
    wget \
    curl \
    unzip \
    tar \
    bzip2 \
    git \
    && rm -rf /var/lib/apt/lists/*

# Install yq separately from GitHub releases
RUN wget -qO /usr/local/bin/yq https://github.com/mikefarah/yq/releases/latest/download/yq_linux_amd64 \
    && chmod +x /usr/local/bin/yq

# Set working directory
WORKDIR /workspace

# Copy the project files
COPY . .

# Make scripts executable
RUN chmod +x make_scripts/*.sh

# Set default command
CMD ["/bin/bash"]