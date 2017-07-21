XVFB=/usr/bin/Xvfb
XVFBARGS="$DISPLAY -ac -screen 0 1024x768x16 +extension RANDR"
PIDFILE=/var/xvfb_${DISPLAY:1}.pid
case "$1" in
  start)
    echo -n "Starting virtual X frame buffer: Xvfb"
    /sbin/start-stop-daemon --start --quiet --pidfile $PIDFILE --make-pidfile --background --exec $XVFB -- $XVFBARGS
    echo "."
    ;;
  stop)
    echo -n "Stopping virtual X frame buffer: Xvfb"
    /sbin/start-stop-daemon --stop --quiet --pidfile $PIDFILE
    echo "."
    ;;
  restart)
    $0 stop
    $0 start
    ;;
  *)
  echo "Usage: /etc/init.d/xvfb {start|stop|restart}"
  exit 1
esac
exit 0
