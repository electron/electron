FROM arm32v7/ubuntu:18.04

RUN groupadd --gid 1000 builduser \
  && useradd --uid 1000 --gid builduser --shell /bin/bash --create-home builduser

# Set up TEMP directory
ENV TEMP=/tmp
RUN chmod a+rwx /tmp

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get install -y \
 bison \
 build-essential \
 clang \
 curl \
 gperf \
 git \
 libasound2 \
 libasound2-dev \
 libcap-dev \
 libcups2-dev \
 libdbus-1-dev \
 libgconf-2-4 \
 libgconf2-dev \
 libgnome-keyring-dev \
 libgtk2.0-0 \
 libgtk2.0-dev \
 libgtk-3-0 \
 libgtk-3-dev \
 libnotify-bin \
 libnss3 \
 libnss3-dev \
 libxss1 \
 libxtst-dev \
 libxtst6 \
 lsb-release \
 locales \
 nano \
 python-setuptools \
 python-pip \
 python-dbusmock \
 sudo \
 unzip \
 wget \
 xvfb \
&& rm -rf /var/lib/apt/lists/*

# Install Node.js
RUN curl -sL https://deb.nodesource.com/setup_10.x | bash - \
  && DEBIAN_FRONTEND=noninteractive apt-get install -y nodejs \
  && rm -rf /var/lib/apt/lists/*

# crcmod is required by gsutil, which is used for filling the gclient git cache
RUN pip install -U crcmod

ADD tools/xvfb-init.sh /etc/init.d/xvfb
RUN chmod a+x /etc/init.d/xvfb

RUN usermod -aG sudo builduser
RUN echo 'builduser ALL=(ALL:ALL) NOPASSWD:ALL' >> /etc/sudoers

WORKDIR /home/builduser
