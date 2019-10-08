#include <iostream>
#include <poll.h>
#include <string.h>
#include <fcntl.h>
#include <stdio.h>

#include <boost/thread.hpp>
#include <boost/bind.hpp>

#include "wiimote.h"

#define RAD_TO_DEGREE(r) ((r * 180.0f) / M_PI)
#define DEGREE_TO_RAD(d) ((d * M_PI) / 180.0f)


vector <class WiiMote *> WiiMote::wiiMotes;
struct xwii_monitor *WiiMote::monitor = (struct xwii_monitor *) NULL; // poll and use udev

WiiMote::WiiSignal WiiMote::wiiSignal;   // When Wii's come and go
WiiMote::WiiSignal WiiMote::wiiChangeSignal;  // When Wii's change unit number


int WiiMote::openIfaces(void)
{
    unsigned int newIfaces;

    // what interfaces are available

    cacheIfaces = xwii_iface_available(iFace);

    cout << "We have " << hex << cacheIfaces << " ifaces" << endl;

    // if we have a balance board open it.
    // if we have a nunchuck open it.
    // if we have a core, then open it
	
    if (cacheIfaces & XWII_IFACE_BALANCE_BOARD)
    {
	newIfaces = XWII_IFACE_BALANCE_BOARD;
    }
    else if (cacheIfaces & XWII_IFACE_NUNCHUK)
    {
	newIfaces = XWII_IFACE_NUNCHUK;
    }
    else if (cacheIfaces & XWII_IFACE_ACCEL)
    {
	newIfaces = XWII_IFACE_ACCEL;
    }

    if (cacheIfaces & XWII_IFACE_CORE)
    {
	newIfaces |= XWII_IFACE_CORE;
    }

    if (newIfaces)
    {
	unsigned int oldIfaces;
	unsigned int tmpFaces;
	
	oldIfaces = xwii_iface_opened(iFace);

	// get the difference between the old and new ignore the writable
	
	tmpFaces = (oldIfaces & ~newIfaces) & ~XWII_IFACE_WRITABLE;
	
	if (tmpFaces)
	{
	    // these are the things I will close

	    xwii_iface_close(iFace, tmpFaces);
	}
	
	newIfaces |= XWII_IFACE_WRITABLE;
	
	if (xwii_iface_open(iFace, newIfaces) < 0)
	{
	    cout << "We failed to open iface " << hex << newIfaces << endl;
	    return -1;
	}
    }

    // so at this point we have another xwii_iface 

    faces = newIfaces;
    return 0;
}

void WiiMote::rumbleOff(int usec)
{
    usleep(usec);
 
    if (xwii_iface_rumble(iFace, false) < 0)
    {
	cout << "We failed to rumble iface " << endl;
    }
}

int WiiMote::setLeds(void)
{
    int x;
    
    if ((unitId < 0) || (unitId > 3))
    {
	return -1;
    }

    for (x = 0 ; x < 4 ; x++)
    {
	bool on;
	
	// ok so we are going to clear all LEDS except the one we want

	if (x == unitId)
	{
	    on = true;
	}
	else
	{
	    on = false;
	}

	cout << "Setting Led " << x << " to " << on << endl;

	// the LED is one based
	xwii_iface_set_led(iFace, x + 1, on);
    }

    // now the interfaces are open let's play a little

    if (xwii_iface_rumble(iFace, true) < 0)
    {
	cout << "We failed to rumble iface " << endl;
	return -1;
    }

    boost::thread t(boost::bind(&WiiMote::rumbleOff, this, 500000));

    t.detach();

    return 0;
}

void WiiMote::readCalData(void)
{
    string fileName;
    int fd;
    unsigned char buf[10];
    int ret;
    
    // we need to create the filename

    fileName = "/sys/kernel/debug/hid/" + name + "/eeprom";

    fd = open(fileName.c_str(), O_RDONLY);

    // set the default value now and if I don't read anything then we are done

    cal_zero.x = 0;
    cal_zero.y = 0;
    cal_zero.z = 0;

    cal_g.x = 110;
    cal_g.y = 110;
    cal_g.z = 110;

    if (fd >= 0)
    {
	if (lseek(fd, 0x16, SEEK_CUR) >= 0)
	{
	    // we are at the correct location, read 10 bytes

	    ret = read(fd, buf, sizeof(buf));

	    if (ret == 10)
	    {
		int sum;
		
		// we got 10 bytes, we can put in the calibration info
		cout << "We got 10 bytes" << endl;

		// see if they are valid

		for (int x = 0 ; x < 9 ; x++)
		{
		    sum += buf[x];
		}

		sum += 0x55;
		sum &= 0xff;

		if (sum == buf[9])
		{
		    // valid calibration data

		    cal_zero.x = (buf[0] * 4) + ((buf[3] >> 4) & 0x3);
		    cal_zero.y = (buf[1] * 4) + ((buf[3] >> 2) & 0x3);
		    cal_zero.z = (buf[2] * 4) + (buf[3] & 0x3);

		    cal_g.x = (buf[4] * 4) + ((buf[7] >> 4) & 0x3);
		    cal_g.y = (buf[5] * 4) + ((buf[7] >> 2) & 0x3);
		    cal_g.z = (buf[6] * 4) + (buf[7] & 0x3);

		    cal_g.x -= cal_zero.x;
		    cal_g.y -= cal_zero.y;
		    cal_g.z -= cal_zero.z;

		    // the kernel subtracts 512 from all axis for us

		    cal_zero.x -= 512;
		    cal_zero.y -= 512;
		    cal_zero.z -= 512;
		    
		    cout << "Yeah" << endl;
		}
	    }
	}
    }    
}

void WiiMote::connectInThread(char *xwiiName)
{
    size_t pos;
    
    // if we come here from a plugin we need a delay

    usleep(700000);

    // keep track of the name, I really only want the last part

    name = string(xwiiName);

    pos = name.find_last_of("/");
    
    name = name.substr(pos+1);

    // let's read the calibration data

    readCalData();
    
    // so create a new xwii_iface

    if (xwii_iface_new(&iFace, xwiiName) < 0)
    {
	cout << "We failed to get a new iface " << errno << endl;
	return;
    }
    else
    {
	// tell us about hotPlug

	if (xwii_iface_watch(iFace, true) < 0)
	{
	    cout << "We failed to watch iface " << endl;
	    return;
	}

	openIfaces();

	setLeds();
    }

    // I think we can consider this wii connected

    connected = true;
    
    // we can now let someone know about this

    wiiSignal(this, 1);
}

int WiiMote::connect(char *xwiiName)
{
    boost::thread t(boost::bind(&WiiMote::connectInThread, this, xwiiName));

    // we don't care when this ends, but don't leak
    
    t.detach();

    return 0;
}

void WiiMote::calcGforce(int32_t x, int32_t y, int32_t z)
{
    float xg, yg, zg;

    xg = cal_g.x;
    yg = cal_g.y;
    zg = cal_g.z;

    gForce.x = (x - cal_zero.x) / xg;
    gForce.y = (y - cal_zero.y) / yg;
    gForce.z = (z - cal_zero.z) / zg;

//    cout <<  fixed << "X = " << gForce.x << " Y = " << gForce.y << " Z = " << gForce.z << endl;  
}

void WiiMote::calcOrientation(void)
{
    float x, y, z;

    /*
     *	roll	- use atan(z / x)		[ ranges from -180 to 180 ]
     *	pitch	- use atan(z / y)		[ ranges from -180 to 180 ]
     *	yaw		- impossible to tell without IR
     */

    /* yaw - set to 0, IR will take care of it if it's enabled */

    orientation.yaw = 0.0f;

    /* find out how much it actually moved and normalize to +/- 1g */

    x = gForce.x;
    y = gForce.y;
    z = gForce.z;

    /* make sure x,y,z are between -1 and 1 for the tan functions */
    if (x < -1.0f)
	x = -1.0f;
    else if (x > 1.0f)
	x = 1.0f;
    if (y < -1.0f)
	y = -1.0f;
    else if (y > 1.0f)
	y = 1.0f;
    if (z < -1.0f)
	z = -1.0f;
    else if (z > 1.0f)
	z = 1.0f;

    // if it is over 1g then it is probably accelerating and the gravity vector cannot be identified

    if (abs(gForce.x) < 1.1) {
	/* roll */
	x = -RAD_TO_DEGREE(atan2f(x, z));

	orientation.roll = x + 90;
    }

    if (abs(gForce.y) < 1.1) {
	/* pitch */
	y = RAD_TO_DEGREE(atan2f(y, z));

	orientation.pitch = y + 90;
    }

//    cout <<  fixed << "Pitch = " << orientation.pitch << " Roll = " << orientation.roll;
//    cout << " Yaw = " << orientation.yaw << endl;  
}

void WiiMote::accelEvent(xwii_event *event)
{
    calcGforce((int32_t) event->v.abs[0].x,
	       (int32_t) event->v.abs[0].y,
	       (int32_t) event->v.abs[0].z);

    calcOrientation();

    // we should call someone who cares now that we know things.

    accelSig(this);
}

void WiiMote::keyEvent(xwii_event *event)
{
    // handle the key event

    // send to someone else

    keySig(this, event->v.key.code, event->v.key.state);

 #if 0
    // I guess I can call an event on key change

    switch(event->v.key.code)
    {
	case XWII_KEY_ONE:
	{
	    extern void setCalibrateMode(bool);

	    if (event->v.key.state != 0)
	    {
		// set the rumble on
		if (xwii_iface_rumble(iFace, true) < 0)
		{
		}
		setCalibrateMode(true);
	    }
	    else
	    {
		// set the rumble off
		if (xwii_iface_rumble(iFace, false) < 0)
		{
		    // maybe do something
		}

		setCalibrateMode(false);
	    }

	    break;
	}

	default:
	{
	    cout << "Key event " << event->v.key.code << " is ";
	    cout << event->v.key.state << endl;
	    break;
	}
    }
#endif
}

void WiiMote::nunchukMove(xwii_event *event)
{
    // handle nunchuk moves, this is both joystick and accel

    // for now print some data, 
    xwii_event_abs *pAbs = &(event->v.abs[0]);

    joyStick.x = pAbs[0].x;
    joyStick.y = pAbs[0].y;

    // Signal to all interested parties

    joyStickSig(this);
    
    // for now I will ignore the accel info
#if 0
    cout << dec;
    cout << "Nunchuk move x[0] = " << pAbs[0].x << " y[0] = " << pAbs[0].y << " z[0] = ";
    cout << pAbs[0].z << endl;
    cout << "x[1] = " << pAbs[1].x << " y[1] = " << pAbs[1].y << " z[1] = ";
    cout << pAbs[1].z << endl;
#endif
}

void WiiMote::balanceBoard(xwii_event *event)
{
    // get the measure at each corner
    
    xwii_event_abs *pAbs = &(event->v.abs[0]);

    // bbWeights[4] is the total, so clear it

    bbWeights[4] = 0;
    
    for (int i = 0 ; i < 4 ; i++)
    {
	bbWeights[i] = pAbs[i].x;
	bbWeights[4] += bbWeights[i];
    }

    // Signal to all interested parties

    balanceSig(this);
}

// this is the loop to process all wiiMote things. This will never return it should be runn
// in it's own thread

void WiiMote::loop(void)
{
    int fds_num;
    struct pollfd fds[10];

    // start by looking for existing wii's

    WiiMote::getCurrent("old");
    
    // we are going to do this forever

    while(true)
    {
	int results;
	
	// fill in a couple of poll objects
	
	fds_num = WiiMote::fillInPoll(&fds[0], 10);

	// poll for 100 ms
	
	results = poll(fds, fds_num, 100);

	if (results < 0)
	{
	    if (errno != EINTR)
	    {
		results = -errno;
		break;
	    }
	}
	else
	{
	    int x;
	
	    for (x = 0 ;  x < (fds_num - 1) ; x++)
	    {
		if (fds[x].revents & POLLIN) 
		{
		    struct xwii_event event;
		    
		    // this iface has something for us

		    if (xwii_iface_dispatch(WiiMote::index(x)->iFace, &event, sizeof(event)) < 0)
		    {
			if (errno != EAGAIN)
			{
			    cout << "Iface dispatch failed errno " << errno << endl;
			    continue;
			}

			continue;
		    }
		    else
		    {
			// we have an event

			// what is it ?

			if (event.type == XWII_EVENT_ACCEL)
			{
			    WiiMote::index(x)->accelEvent(&event);
			}
			else if (event.type == XWII_EVENT_WATCH)
			{
			    WiiMote::index(x)->openIfaces();
			}
			else if (event.type == XWII_EVENT_GONE)
			{
			    cout << "Gone event" << endl;

			    // looks like it all went away

			    delete WiiMote::index(x);
			}
			else if (event.type == XWII_EVENT_KEY)
			{
			    WiiMote::index(x)->keyEvent(&event);
			}
			else if (event.type == XWII_EVENT_NUNCHUK_KEY)
			{
			    WiiMote::index(x)->keyEvent(&event);
			}
			else if (event.type == XWII_EVENT_NUNCHUK_MOVE)
			{
			    // this looks like an accel from the nunchuk

			    WiiMote::index(x)->nunchukMove(&event);
			}
			else if (event.type == XWII_EVENT_BALANCE_BOARD)
			{
			    // this looks like a balance board

			    WiiMote::index(x)->balanceBoard(&event);
			}
			else
			{
			    cout << "We got an " << hex << event.type << endl;
			}
		    }
		}
	    }

	    // now test the monitor
	    
	    if (fds[x].revents & POLLIN) 
	    {
		WiiMote::getCurrent("a new");
	    }
	}
    }
}

void WiiMote::init(void)
{
    static bool _inited = false;

    // There can be only one
    
    if (!_inited)
    {
	// start a new thread for the wii motes

	boost::thread wiiLoop(&loop);
    }
}
