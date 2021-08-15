FROM ghcr.io/electron/build:afff0fe88a106dfd7cd7839388fd4a8944ab9640

RUN rm -rf /home/builduser/.electron_build_tools && \
    git clone https://github.com/electron/build-tools.git /home/builduser/.electron_build_tools && \
    cd /home/builduser/.electron_build_tools && \
    npx --yes yarn && \
    sudo locale-gen "en_US.UTF-8"

RUN echo export PATH=\"\$PATH:/home/builduser/.electron_build_tools/src\" >> ~/.bashrc
RUN echo "cd /workspaces/gclient/src/electron" >> ~/.bashrc
RUN echo export LC_ALL=\"en_US.UTF-8\" >> ~/.bashrc
