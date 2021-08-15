FROM ghcr.io/electron/build:afff0fe88a106dfd7cd7839388fd4a8944ab9640

RUN rm -rf /home/builduser/.electron_build_tools && \
    git clone https://github.com/electron/build-tools.git /home/builduser/.electron_build_tools && \
    cd /home/builduser/.electron_build_tools && \
    npx --yes yarn && \
    sudo locale-gen "en_US.UTF-8"

COPY vnc-setup.sh /tmp/library-scripts/
RUN sudo apt update && export DEBIAN_FRONTEND=noninteractive \
	&& sudo bash /tmp/library-scripts/vnc-setup.sh \
	&& sudo add-apt-repository ppa:mozillateam/ppa \
	&& sudo apt -y install firefox-esr \
	&& sudo apt clean -y && sudo rm -rf /var/lib/apt/lists/* /tmp/library-scripts/

ENV DBUS_SESSION_BUS_ADDRESS="autolaunch:" \
	VNC_RESOLUTION="1440x768x16" \
	VNC_DPI="96" \
	VNC_PORT="5901" \
	NOVNC_PORT="6080" \
	DISPLAY=":1" \
	LANG="en_US.UTF-8" \
	LANGUAGE="en_US.UTF-8"

RUN echo export PATH=\"\$PATH:/home/builduser/.electron_build_tools/src\" >> ~/.bashrc
RUN echo "cd /workspaces/gclient/src/electron" >> ~/.bashrc
RUN echo export LC_ALL=\"en_US.UTF-8\" >> ~/.bashrc

ENTRYPOINT ["/usr/local/share/desktop-init.sh"]
