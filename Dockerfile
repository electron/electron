FROM ubuntu:18.04

RUN groupadd --gid 1000 builduser \
  && useradd --uid 1000 --gid builduser --shell /bin/bash --create-home builduser

# Set up TEMP directory
ENV TEMP=/tmp
RUN chmod a+rwx /tmp

# Install Linux packages
ADD build/install-build-deps.sh /setup/install-build-deps.sh
RUN curl -sL https://deb.nodesource.com/setup_10.x | bash -
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    libnotify-bin \
    locales \
    lsb-release \
    nodejs \
    python-dbusmock \
    python-pip \
    python-setuptools \
    vim-nox \
    wget \
  && /setup/install-build-deps.sh --syms --no-prompt --no-chromeos-fonts --no-nacl \
  && rm -rf /var/lib/apt/lists/*

RUN pip install -U crcmod

RUN mkdir /tmp/workspace
RUN chown builduser:builduser /tmp/workspace

# Add xvfb init script
ADD tools/xvfb-init.sh /etc/init.d/xvfb
RUN chmod a+x /etc/init.d/xvfb

USER builduser
WORKDIR /home/builduser
