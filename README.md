# TxPPM

This software is usable for all RC Transmitters with the PPM modulation
(mostly all transmitters with trainer/simulator connector).

Purpose of this one is to convert the signal from your transmitter
and make it available as joystick in your operating system.
You don't have to buy anything else except your transmitter
adapter + mono/stereo extension cord for your sound card input.
Or you can make your own cable for direct connection into your transmitter
with specified TX connector (e.g. mini din 4 for Esky Hobby) and mono jack.
For example, DX6i transmitter need only mono or stereo jack (from both sides) cable.

As result you can play all simulators and games which requires joystick interface,
for example: Heli-X, CRRCsim, FlightGear, ClearView (with wine, ...), etc.

# Installation

## Dependencies

This project requires the ```portaudio``` development files.
On a debian derivative this can be installed with
```
$ sudo apt-get install portaudio19-dev
```

## Kernel Module installation

Build the kernel module in the ```module``` directory and install the kernel module.

```
$ cd module
$ make
$ sudo make install
```

## Build and install the user program

If you want to ubild the graphical user interface set the environment variable ```WITH_GUI``` to 1.

Build the client program in the ```software``` directory and start it.

```
cd software
make
./ppm2tx
```

Now you should be able to use a joystick device that represents your transmitter in your simulator.

# Credits

This is a fork of the work Tomas Jedrzejek provided. It worked perfectly
until a recent update of my system. That's why I chose to fork his project and fix the compilation issues.

# Disclaimer

This program comes with ABSOLUTELY NO WARRANTY.
The author of this software can not be held liable for any damage caused by this program or instructions given.
For details check the LICENSE file bundled with this software.

As I don't have a Windows PC or Mac for development, I can not guarantee that this
project compiles on a Windows PC or Mac. If you are intereted in maintaining these
platforms please contact me.
