#! /bin/sh

../../test/SRE-Test-FPS-camera -p test.ui_events

#if (../bin/imgcmp -v -o diff1.png -t 0.0 -e 0.0% Test0_base.png Test0.png == -1) {
#    fail = true
#}
