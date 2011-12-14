# Utilities for B2G RIL Connection

This repo contains utilities for forwarding the rild socket connection
out to non radio account owned processes.

# Desktop RIL development using rilproxyd

In order to use desktop browsers for RIL development (as happens with
the b2g-js-ril project, http://www.github.com/philikon/b2g-js-ril),
the rilproxy needs to listen on a different socket so that the b2g
browser does not take the connection. To do this, run the following
command in an adb shell:

touch /data/local/rilproxyd

Then:

killall rilproxy

This will restart the rilproxy daemon to listen on
/dev/socket/rilproxyd instead of /dev/socket/rilproxy. Note that the
/data/local/rilproxyd file is deleted when rilproxy comes up again, so
the next time rilproxy is killed or restarted, it will connect to
/dev/socket/rilproxy once again.
