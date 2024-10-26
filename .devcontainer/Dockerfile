# Base image (Ubuntu)
FROM ubuntu:22.04

# Add the ASCII banner to the .bashrc
RUN echo "GREEN='\033[0;32m'\n\
NC='\033[0m' # No Color\n\
echo -e \"\${GREEN}---------------------------------------------------------------------------------------------\${NC}\"\n\
echo -e \"\${GREEN} \${NC}\"\n\
echo -e \"\${GREEN}8888888 8888888b.   .d8888b.       d8b               888      d8b                            \${NC}\"\n\
echo -e \"\${GREEN}  888   888   Y88b d88P  Y88b      Y8P               888      Y8P                            \${NC}\"\n\
echo -e \"\${GREEN}  888   888    888 888    888                        888                                     \${NC}\"\n\
echo -e \"\${GREEN}  888   888   d88P 888             888 88888b.       888      888 88888b.  888  888 888  888 \${NC}\"\n\
echo -e \"\${GREEN}  888   8888888P   888             888 888  88b      888      888 888  88b 888  888  Y8bd8P \${NC}\"\n\
echo -e \"\${GREEN}  888   888        888    888      888 888  888      888      888 888  888 888  888   X88K   \${NC}\"\n\
echo -e \"\${GREEN}  888   888        Y88b  d88P      888 888  888      888      888 888  888 Y88b 888 .d8  8b. \${NC}\"\n\
echo -e \"\${GREEN}8888888 888          Y8888P        888 888  888      88888888 888 888  888   Y88888 888  888 \${NC}\"\n\
echo -e \"\${GREEN} \${NC}\"\n\
echo -e \"\${GREEN} Author        :  Nguyen Dinh Hoan\${NC}\"\n\
echo -e \"\${GREEN} Repo          :  https://github.com/hoan9x/IPC-in-Linux\${NC}\"\n\
echo -e \"\${GREEN} Description   :  Working environment to demonstrate IPC in Linux\${NC}\"\n\
echo -e \"\${GREEN}---------------------------------------------------------------------------------------------\${NC}\"" >> ~/.bashrc

# Install required packages
RUN apt-get update && apt-get install -y build-essential cmake git gdb python3 && apt-get clean

# Set up a default workspace directory
WORKDIR /workspaces

# Expose the container's workspace for VS Code
VOLUME /workspaces
