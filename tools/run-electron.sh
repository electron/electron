export DISPLAY=":99.0"
sh -e /etc/init.d/xvfb start
cd /workspace
./electron --version
