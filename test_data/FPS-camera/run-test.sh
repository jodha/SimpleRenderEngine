#! /bin/sh

./FPS-camera -p test.log

if (../bin/imgcmp -v -o diff1.png -t 0.0 -e 0.0% Test0_base.png Test0.png == -1) {
    fail = true
}
