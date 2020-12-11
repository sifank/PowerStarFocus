# Power*Star
INDI drivers for Greg's Power*Star, also known as FeatherTouch Power*Star

REQUIRES: 
INDI-core

CONTENTS:
- INDI driver and xml files
  - Only the focus functions are implemented so far
- TUI (text user interface) called 'pstui'
  - This allows complete manual control from a terminal window
    of all functions and capabilities

INSTALLING:
In a work directory of your choosing on the RPI 
or (linux) system that the Power*Star is plugged into:

git clone https://github.com/sifank/PowerStar.git

Binary files for the Raspberry Pi are included:
cd PowerStar
sudo make install

To compile from scratch:
cd PowerStar;
make clean; make
sudo make install

NOTES:
- If it can not find it's config file, it will tell you rerun the TUI as root
- Run pstui first to setup focus motor, power options, etc.
- Initial configuration is set for a Unipolar motor, if you have a bipolar motor or not sure of your Unipolar, then do not connect to
your focus motor until after to run the TUI to set it up.

