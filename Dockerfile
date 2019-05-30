FROM electronbuilds/libchromiumcontent:0.0.4

USER root

# Set up HOME directory
ENV HOME=/home/builduser

# Install node.js
RUN curl -sL https://deb.nodesource.com/setup_10.x | bash -
RUN apt-get install -y nodejs

# Install wget used by crash reporter
RUN apt-get install -y wget

# Install python-dbus
RUN apt-get install -y python-dbus
RUN pip install python-dbusmock

# Install libnotify
RUN apt-get install -y libnotify-bin

# Add xvfb init script
ADD tools/xvfb-init.sh /etc/init.d/xvfb
RUN chmod a+x /etc/init.d/xvfb

COPY .circleci/fix-known-hosts.sh /home/builduser/fix-known-hosts.sh
RUN chmod a+x /home/builduser/fix-known-hosts.sh

USER builduser
WORKDIR /home/builduser
