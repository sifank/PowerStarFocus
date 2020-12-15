# PowerHub
INDI drivers for Greg's PowerHub

REQUIRES: 
INDI-core

BUILDING:

CAUTION:  Raspberry Pi's HID library has issues, this may not work on an RPI

In a work directory of your choosing on the RPI 
or (linux) system that the PowerHub is plugged into:

git clone https://github.com/sifank/PowerHub.git

cd PowerHub;
make;
sudo make install

