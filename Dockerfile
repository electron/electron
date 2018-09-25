FROM ubuntu:18.04

RUN groupadd --gid 1000 builduser \
  && useradd --uid 1000 --gid builduser --shell /bin/bash --create-home builduser

# Set up TEMP directory
ENV TEMP=/tmp
RUN chmod a+rwx /tmp

# Install Linux packages
ADD build/install-build-deps.sh /setup/install-build-deps.sh
RUN echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections
RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
    curl \
    libnotify-bin \
    locales \
    lsb-release \
    nano \
    python-dbusmock \
    python-pip \
    python-setuptools \
    sudo \
    vim-nox \
    wget \
  && /setup/install-build-deps.sh --syms --no-prompt --no-chromeos-fonts --lib32 --arm \
  && rm -rf /var/lib/apt/lists/*

# Install Node.js
RUN curl -sL https://deb.nodesource.com/setup_10.x | bash - \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y nodejs \
  && rm -rf /var/lib/apt/lists/*

# crcmod is required by gsutil, which is used for filling the gclient git cache
RUN pip install -U crcmod

RUN mkdir /tmp/workspace
RUN chown builduser:builduser /tmp/workspace

# Add xvfb init script
ADD tools/xvfb-init.sh /etc/init.d/xvfb
RUN chmod a+x /etc/init.d/xvfb

USER builduser
WORKDIR /home/builduser
