#! /bin/sh

# Note: this file is currently written in some kind of weird, undocument psuedo-
#       code that only makes sense to one person on earth (this file primarily
#       documents the commands that need to be run)

../../SRE-Test-FPS-camera -p test.ui_events

$acc = 0.0 # threshold before a pixel is different (0 fails if any difference)
$pix = 0.0 # percentage of pixels allowed to be different
bool pass = true;
for ($i = 1 to 6) {
    if(../../../bin/imgcmp -v -o diff$i.png -t $acc -e $pix% gold_files/Test$i.png Test$i.png == -1) {
        pass = false; 
    }
}
return pass;
