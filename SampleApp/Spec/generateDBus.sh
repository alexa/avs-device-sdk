#!/bin/sh

gdbus-codegen --generate-c-code AIDaemon-dbus-generated \
--c-namespace AIDaemon \
--interface-prefix com.obigo.Nissan.AIDaemon \
com.obigo.Nissan.AIDaemon.xml

mv ./AIDaemon-dbus-generated.h ../include/AIDaemon/
mv ./AIDaemon-dbus-generated.c ../src/
