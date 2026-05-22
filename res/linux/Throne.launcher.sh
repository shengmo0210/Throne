#!/bin/sh
# Launcher installed to /usr/bin/Throne by the .deb package.
# Ensures /opt/Throne is on PATH so the GUI can spawn ThroneCore.
exec env PATH="/opt/Throne:$PATH" /opt/Throne/Throne "$@"
