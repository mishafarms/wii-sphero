# wii-sphero
This is a raspberry pi hid-wiimote design using the kernel mods
Sphero with Wii control

This was all done on a raspberry PI 3. Here are my install instructions for the dependancies including my personal preferences

sudo apt-get vino dconf-editor

use dconf-editor to set no-prompt and no-encryption

create /home/pi/.config/autostart/

write the following to vino.desktop in the /home/pi/.config/autostart/ directory 
```
[Desktop Entry]
Encoding=UTF-8
Type=Application
Name=VINO
Comment=
Exec=/usr/lib/vino/vino-server
StartupNotify=false
Terminal=false
Hidden=false
```

then reboot and you should have a VNC to the desktop available.
in order to get a larger screen headless add the following to /boot/config.txt
##### this will give a 1920x1080 screen size with no monitor connected hdmi_group = 1 and hdmi_mode =16

```
hdmi_force_hotplug=1
hdmi_ignore_edid=0xa5000080
hdmi_group=1
hdmi_mode=16
start_x=1
gpu_mem=128
```

sudo apt-get install bluez blueman libbluetooth-dev git libreadline6-dev libncurses5 libncurses5-dev cmake

git clone https://github.com/slock83/sphero-linux-api.git

sudo apt-get install emacs

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

git clone https://github.com/dvdhrm/xwiimote.git

####### may need autoconf
```
sudo apt-get install autoconf
sudo apt-get install libtool
sudo apt-get install libudev-dev
sudo apt-get install libboost-dev
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
