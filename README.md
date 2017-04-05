# wii-sphero
This is a raspberry pi hid-wiimote design using the kernel mods
These instructions have been updated to include setting up the LCD display and touchscreen.

Sphero with Wii control

This was all done on a raspberry PI 3. Here are my install instructions for the dependancies including my personal preferences

The Raspberry Pi now comes with VNC. Go to the pi preferences and set SSH and VNC to enabled.

You should change the password of the pi account to something other the "raspberry" you will see a message if you do not.

The LCD screen is the elecrow 5 inch.
```
git clone https://github.com/goodtft/LCD-show.git
chmod -R 755 LCD-show
cd LCD-show/
```

Then we have to make a change. The file usr/cmdline.txt has the wrong root. So look at the original file in /boot/cmdline.txt
and see what the root=, mine is root=/dev/mmcblk0p7, so modify the usr/cmdline.txt

Then type
```
./LCD5-show
```
and the system should reboot.
now do the following.

```
cd LCD-show
sudo dpkg -i -B xserver-xorg-input-evdev_1%3a2.10.3-1_armhf.deb
sudo cp -rf /usr/share/X11/xorg.conf.d/10-evdev.conf /usr/share/X11/xorg.conf.d/45-evdev.conf
sudo reboot
```

To get the touchscreen working I did the following.

```
cd LCD-show
sudo dpkg -i -B xinput-calibrator_0.7.5_armhf.deb
```
then execute the calbrator. You should not need to set this (it should be set) but your DISPLAY=0.0

```
xinput-calibrator
```
and calibrate the touchscreen.

To get the right mouse click working. create an /etc/X11/xorg.conf file with this in it.

```
Section "InputClass"
	Identifier "calibration"
	Driver "evdev"
	MatchProduct "ADS7846 Touchscreen"

	Option "EmulateThirdButton" "1"
	Option "EmulateThirdButtonTimeout" "750"
	Option "EmulateThirdButtonMoveThreshold" "30"
EndSection
```
```
sudo reboot
 
sudo apt-get install bluez blueman libbluetooth-dev git libreadline6-dev libncurses5 libncurses5-dev cmake

sudo apt-get install emacs
```

The Sphero's need to be paired with the raspberry. You can use blueman should be at the top right
side of the main desktop. to pair and trust the Sphero's. 

##### This is the way to  go with wii use the hid-wiimote.

##### using hid-wiimote kernel mod

sudo modprobe hid-wiimote

sudo apt-get install xwiimote

###### go here to get more info https://wiki.archlinux.org/index.php/XWiimote

```
sudo bluetoothctl
power on
agent on
<press red sync button>
scan on
pair <MAC of the found wiimote, use TAB for autocompletion>           # note: we do not explicitly connect, we just pair!
connect <MAC of the wiimote>                                          # there seems to be a pretty short timeout, so execute thisimmediately after the pairing command
trust <MAC of the wiimote>
disconnect <MAC of the wiimote>
```

```
git clone https://github.com/dvdhrm/xwiimote.git
cd xwiimote
./autogen.sh
make
sudo make install
```
####### may need autoconf
```
sudo apt-get install autoconf libtool libudev-dev libboost-dev
```

then in this directory you can type 'cmake .' then 'make'.

You should get a wii-sphero executeable. It needs to be run as root. So if you do sudo make install it will be installed into /home/pi/bin/wii-sphero with the proper permissions.

I put this into the /etc/rc.local to get this program to run every time we reboot.

```
if [ -e /home/pi/bin/wii-sphero ]; then
   cd /home/pi
   /home/pi/bin/wii-sphero -p 60 > /dev/null &
fi
```
```
wii-sphero [-cel] [-s #] [-p #]
-c - don't connect to sphero. used mostly for debugging Wii code
-e - turn on error printing
-l - turn on logging. used mostly with -c above to debug Wii code
-s # - in our case # is 0 or 1 and is which Sphero we want to connect to
-p # - # can be between 30 and 200 and is the percent we multiply the Wii input by in order to make it less or more
      sensitive.
````
If you just have the Wii mote and no Nunchuk then I use the orientation of the Wii controller to drive the Sphero. With the 1 & 2 
buttons to the right. and the Wii facing up. The Sphero should roll forward when you tip the Wii forward and backward when you
tip the Wii backwards. Tip left to roll left and you guessed it tip right to roll right. It will roll to all the directions in
between as well and the more you tip the Wii the faster the Sphero will roll.

If you need to calibrate the Sphero, Then hold down the 1 button, the Wii should rumble and the backlight on the Sphero
should come on. Now when you control the Wii it is setting the heading of the Sphero. Let go of the 1 button before you move
the Wii controller or you will not set the heading correctly.

If the Nunchuk is inserted, then we will use the joystick on the Nunchuk to control the Sphero. You can still use the 1 button
on the Wii to calibrate the Sphero. You can add or remove the Nunchuk at any time.

If you push the home button, the Sphero will go to sleep and we will try to connect to the next Sphero in the list

So to turn off the system, Push the home button on the Wii (without another awake Sphero) and then push the power button
on all the Wii's that are connected. 

If you push power on the Wii controlling the Sphero, we will try to promote the next Wii. In order to use the Wi Balance Board
You should calibrate with another Wii, then connect the Wii Balance the light on the Balance Board should be light when it is
controlling the Sphero.
