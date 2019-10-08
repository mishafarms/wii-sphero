/*
 * wii-sphero written for IdeaFabLabs SpheroMaze
 * this code is all open-source, feel free to use or abuse it in anyway
 * you want.
 */

/*
 * This code will connect to many WiiMotes and will allow the control of up to N Sphero's
 * I used the wii accel data or the nunchuk joystick or a balance board to control the 
 * sphero. I have not really written in C++ in a while, so I am playing with difference
 * concepts to learn and play.
 */

#include <errno.h>
#include <inttypes.h>
#include <cmath>
#include <ncurses.h>
#include <stdarg.h>
#include <stdbool.h>
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <sys/select.h>
#include <iostream>
#include <fstream>
#include <termios.h>

#include <boost/thread.hpp>

#include "xwiimote.h"

#include "vec.h"

#include "wiimote.h"
#include <regex>
#include <wiringPi.h>

#include "sphero/bluetooth/bluez_adaptor.h"
#include "sphero/Sphero.hpp"
#include "sphero/packets/Constants.hpp"
//Test
#include <sphero/packets/answer/ColorStruct.hpp>
#include <sphero/packets/answer/BTInfoStruct.hpp>

//--------------------------------------------------------- Local includes
#include "spheromanager.h"

/**/
static SpheroManager sm;

std::vector<string> spheroAddresses;

string address[] = { "68:86:E7:06:A7:60",
		     "68:86:E7:06:75:50",
		     "68:86:E7:08:D9:F8",
		     "68:86:E7:08:13:91"};
    
int spheroAddressIndex = -1;
		
/**/

using namespace std;

#define WAIT_TIME 200  // we only want one every 100ms or 10 per second

#define MIN_WEIGHT 1
#define MIN_VELOCITY 1
#define MIN_MAGNITUDE .1

#define COS_45 .7071
#define SIN_45 .7071

const int calButPin = 0;

static bool calibrateMode = false;

bool switchSphero = true;

bool errReport = false;
bool logReport = false;

uint8_t percent = 100;  // we will multiply the velocity by this divided by 100

bool noColorChange = false;

#define LOG(fmt, ...) { if (logReport){ printf(fmt, ##__VA_ARGS__); } }
#define ERROR(fmt, ...) { if (errReport){ fprintf(stderr, fmt, ##__VA_ARGS__); } }

void setCalibrateMode(bool calibrate);

long int elapseTime(struct timeval *lastTime, struct timeval *nowTime)
{
    long int secs = nowTime->tv_sec - lastTime->tv_sec;
    long int msecs = (nowTime->tv_usec - lastTime->tv_usec) / 1000;

    if (msecs < 0)
    {
	msecs += 1000;  // add one second in msec
	secs--;
    }

    return((secs * 1000) + msecs);
}

/**
 * @brief isConnected : Checks if a Sphero is connected
 */
static bool isConnected()
{
	if(sm.getSphero() == NULL)
	{
	    ERROR("Please connect first\n");
	    return false;
	}
	return true;
}

void spheroSendRoll(float magnitude, float angle)
{
    struct timeval nowTime;
    static struct timeval lastTime = {0, 0};

    if(lastTime.tv_sec == 0)
    {
	// looks like we haven't read the time yet

	gettimeofday(&lastTime, NULL);
    }
        
    // get the current time, we really only want usecs

    gettimeofday(&nowTime, NULL);
   
    if (isConnected() && (elapseTime(&lastTime, &nowTime) >= WAIT_TIME))
    {
	lastTime = nowTime; // so we will wait again

	magnitude *= (float) percent / 100.0;  // scale the magnitude
	
	if (magnitude > 1.0)
	{
	    magnitude = 1.0;
	}
      
	if (magnitude >= MIN_MAGNITUDE)
	{
	    uint8_t velocity = (uint8_t) (255 * magnitude) % 256;

	    if (calibrateMode)
	    {
		velocity = 0;
		angle += 180.0;
	    }

	    if (angle < 0.0)
	    {
		angle += 360.0;
	    }
		
	    LOG("sending sphero roll %d %d\n",
		velocity, (uint16_t) angle );
	    sm.getSphero()->roll(velocity, (uint16_t) angle);
	}
	else
	{
	    sm.getSphero()->roll((uint8_t) 0, (uint16_t) 0);
	}
    }
}

void wiiAccelEvent(WiiMote *wii)
{
    // only Wii unitId 0 can control the sphero

    if (wii->unitId != 0)
    {
	return;
    }
    
    // ok so now actually control the sphero
    // here goes more math, I think it works like this
    // visualize a 1 meter stick coming straight up out of the ground
    // this stick is at 90 degrees. if we rotate this stick so that it is
    // sticking 45 degrees, we would take the sin(45) and the cos(45) to
    // the X,Y value of where the endpoint of the 1 meter stick would be
    // then we rotate the stick again about the roll angle with 90 being
    // straight up. so we take the X value
    // to rotate in pitch get the sin of the angle and the cos of the
    // angle. Then use the sin value and get the angle of the roll and
    // multiply the sin of the roll by it and the cos of the roll by it
    // this gives us a new Y and Z values.
    // now we use the X and Z values to represent a magnitude and
    // direction which we use to control the Sphero

    float x, y, z;
    float angle, magnitude;
    
    x = cos(wii->orientation.pitch * M_PI/180);
    y = sin(wii->orientation.pitch * M_PI/180);
    z = y * cos(wii->orientation.roll * M_PI/180);

    magnitude = sqrt((x * x) + (z * z));
    angle = atan2(x, z) * 180 / M_PI;

    LOG("Mag = %f direction = %f\n", magnitude, angle);

    spheroSendRoll(magnitude, angle);
}

void wiiJoyStickEvent(WiiMote *wii)
{
    // only Wii unitId 0 can control the sphero

    if (wii->unitId != 0)
    {
	return;
    }
    
    // ok so now actually control the sphero
    // this time we just do the normalize then find the vector magnitude and direction
    //

    
    float x, y;
    float angle, magnitude;

    x = (float) wii->joyStick.x;

    // this will limit x to -100 >= x <= 100
    
    x = (x > 100.0) ? 100.0 : x;
    x = (x < -100.0) ? -100.0 : x;
    
    y = (float) wii->joyStick.y;

    // this will limit y to -100 >= y <= 100
    
    y = (y > 100.0) ? 100.0 : y;
    y = (y < -100.0) ? -100.0 : y;

    // normalize the values

    x /= 100.0;
    y /= 100.0;
 
    magnitude = sqrt((x * x) + (y * y));
    angle = atan2(x, y) * 180 / M_PI;

    LOG("Mag = %f direction = %f\n", magnitude, angle);

    spheroSendRoll(magnitude, angle);
}

void wiiBalanceEvent(WiiMote *wii)
{
    float total;
    float weights[4];
    float magnitude;
    float angle;
    float vecX, vecY;
    
    // only Wii unitId 0 can control the sphero

    if (wii->unitId != 0)
    {
	return;
    }

    total = wii->bbWeights[4];
    
    for(int x = 0 ; x < 4 ; x++)
    {
	weights[x] = wii->bbWeights[x] / total;
    }

    // here is the math as I understand it. I get 4 vectors (one for each sensor) they are at
    // 45, 135, -45, -135 degrees
    // if I multiply the weight on each sensor by the cos of the angle I get the X value and
    // it is also the Y in these cases.
    // instead of doing cos(45), I am just going to multiply by the value .7071, This will
    // give me the vectors in X,Y then I can add them
    // all up and get a new X,Y. this resultant vector is the angle and magnitude of what I want,
    // where is the weight positioned.

    printf("tot %3.3f tr %3.3f br %3.3f tl %3.3f bl %3.3f\n", total, weights[0], weights[1],
	weights[2], weights[3]);

    vecX = 0.0;
    vecY = 0.0;

    vecX += (COS_45 * weights[0]);
    vecY += (SIN_45 * weights[0]);
	
    vecX += (COS_45 * weights[1]);
    vecY += (-SIN_45 * weights[1]);

    vecX += (-COS_45 * weights[2]);
    vecY += (SIN_45 * weights[2]);

    vecX += (-COS_45 * weights[3]);
    vecY += (-SIN_45 * weights[3]);

    LOG("vecX = %3.3f vecY = %3.3f\n", vecX, vecY);

    // so now we have a new vector of how the weight is distributed get the angle and magnitude

    // magnitude is sqrt(vecX2 + vecY2)

    magnitude = sqrt((vecX * vecX) + (vecY * vecY));

    angle = atan2(vecX, vecY) * 180 / M_PI;
    
    LOG("Mag = %f direction = %f\n", magnitude, angle);

    spheroSendRoll(magnitude, angle);
}

void setSpheroColor( uint8_t r, uint8_t g, uint8_t b)
{
    if (isConnected())
    {
	sm.getSphero()->setColor(r, g, b);
    }
}

void setSpheroPercentColor(uint8_t spheroPercent)
{
    if (spheroPercent <= 30)
    {
	// set the color to Bright Purple

	setSpheroColor(0x00, 0xff, 0xff);
    }
    else if (spheroPercent == 100)
    {
	// set the color to Bright Green

	setSpheroColor(0x00, 0xff, 0x00);
    }
    else if (spheroPercent == 200)
    {
 	// set the color to Bright Yellow

	setSpheroColor(0xff, 0xff, 0x00);
    }
    else
    {
	// show some amount of white
		    
	setSpheroColor(spheroPercent, spheroPercent, spheroPercent);
    }
}

void wiiKeyEvent(WiiMote *wii, int code, int state)
{
    switch(code)
    {
	case XWII_KEY_ONE:
	{
	    if (state != 0)
	    {
		// set the rumble on
		if (xwii_iface_rumble(wii->iFace, true) < 0)
		{
		}
		setCalibrateMode(true);
	    }
	    else
	    {
		// set the rumble off
		if (xwii_iface_rumble(wii->iFace, false) < 0)
		{
		    // maybe do something
		}

		setCalibrateMode(false);
	    }

	    break;
	}

	case XWII_KEY_HOME:
	{
	    if (state != 0)
	    {
	    }
	    else
	    {
		// let's switch sphero
		switchSphero = true;
	    }

	    break;
	}

	case XWII_KEY_PLUS:
	{
	    if (state != 0)
	    {
		// key down add to percent if we are less then 200

		if ((percent + 5) >= 200)
		{
		    percent = 200;
		}
		else
		{
		    percent += 5;
		}

		setSpheroPercentColor(percent);
		noColorChange = true;
	    }
	    else
	    {
		noColorChange = false;
	    }

	    break;
	}

	case XWII_KEY_MINUS:
	{
	    if (state != 0)
	    {
		// key down subtract 5 from percent if we are greater then 30

		if ((percent - 5) <= 30)
		{
		    percent = 30;
		}
		else
		{
		    percent -= 5;
		}   

		setSpheroPercentColor(percent);
		noColorChange = true;
	    }
	    else
	    {
		noColorChange = false;
	    }

	    break;
	}

	default:
	{
	    cout << wii->unitId << ": Key event " << code << " is " << state << endl;
	    break;
	}
    }
}

void wiiEvent(WiiMote *wii, int add)
{
    cout << "We just got an ";

    if (add)
    {
	cout << "Add ";
    }
    else
    {
	cout << "Remove ";
    }

    cout <<  "WiiEvent " << wii->unitId << endl;

    // here is where we should connect to the wiiMotes events

    wii->accelSig.connect(&wiiAccelEvent);
    wii->joyStickSig.connect(&wiiJoyStickEvent);
    wii->balanceSig.connect(&wiiBalanceEvent);
    wii->keySig.connect(&wiiKeyEvent);
}

void wiiChangeEvent(WiiMote *wii, int id)
{
    (void) wii;
    (void) id;
    
    cout << "We just got a change event" << endl;
}

void initSpheroAddresses(void)
{
    ifstream spheroIStream ("sphero.cfg");

    if (spheroIStream.is_open())
    {
	string line;
	
	// read each line and assume it is in MAC format xx:xx:xx:xx:xx:xx

	while (getline(spheroIStream, line))
	{
	    // I guess we should test for proper format
	    // we are going to use regex.
	    // so ^([0-9A-Fa-f]{2}[:-]){5}([0-9A-Fa-f]{2})$
	    // is our format

	    if (regex_match(line, regex("^([0-9A-Fa-f]{2}:){5}([0-9A-Fa-f]{2})$")))
	    {
		// this one passes muster we can put it in the list

		spheroAddresses.push_back(line);
	    }
	    else
	    {
		cout << line << " is not the right format" << endl;
	    }	    
	}

	spheroIStream.close();
    }

    if (spheroAddresses.size() == 0)
    {
	spheroAddresses.push_back(address[0]);
	spheroAddresses.push_back(address[1]);
    }
}

string getNextSpheroAddress()
{
    string spheroAddress;
    
    // now we may have the list of good addresses, copy the first for our use

    if (spheroAddressIndex >= 0)
    {
	return (spheroAddresses[spheroAddressIndex]);
    }
    
    spheroAddress = spheroAddresses[0];

    spheroAddresses.erase(spheroAddresses.begin());
	
    // put this address at the back of the list

    spheroAddresses.push_back(spheroAddress);

    // now the list is in order for the next time, write it out

    ofstream spheroOStream("sphero.cfg.new", ios::trunc);

    if (spheroOStream.is_open())
    {
	for (std::vector<string>::iterator i = spheroAddresses.begin() ;
	     i < spheroAddresses.end() ; i++)
	{
	    spheroOStream << *i << endl;
	}

	spheroOStream.close();
    }

    // now delete the original file and rename the new one

    std::remove("sphero.cfg");
    std::rename("sphero.cfg.new", "sphero.cfg");

    return spheroAddress;
}

int init(void)
{
    // we need to register our functions for events.

    WiiMote::wiiSignal.connect(&wiiEvent);
    WiiMote::wiiChangeSignal.connect(&wiiChangeEvent);

    // this will start a thread and then return, You will only get signals
    
    WiiMote::init();

    // get the SpheroAddress ready

    initSpheroAddresses();
    return 0;
}

void setCalibrateMode(bool calibrate)
{
    if (isConnected())
    {
	if (calibrate && !calibrateMode)
	{
	    LOG("Enter calibrateMode\n");

	    // go into calibrate mode
	    calibrateMode = true;
	    sm.getSphero()->setBackLedOutput(255);
	}
	else if (!calibrate && calibrateMode)
	{
	    LOG("Exit calibrateMode\n");
	    sm.getSphero()->setBackLedOutput(0);
	    sm.getSphero()->setHeading(0);
	    calibrateMode = false;	
	}
    }
}

int wiiSpheroLoop(int argc, char** argv);

struct cmdArgs {
    int argc;
    char **argv;
};

void spheroDisconnect(void)
{
    cout << "Sphero disconnected, we should do something" << endl;
//    sm.getSphero()->disconnect();
}

void spheroCollision(CollisionStruct *info)
{
    int16_t iX = static_cast <int16_t> (info->impact_component_x);
    int16_t iY = static_cast <int16_t> (info->impact_component_y);
    int16_t sp = static_cast <int16_t> (info->speed);
    
    cout << "Bang" << endl;

    cout << dec;
    cout << "impact x/y (" << iX << "/" << iY;
    cout << ") axis = " << info->threshold_axis << "mag x/y (" << info->magnitude_component_x;
    cout << "/" << info->magnitude_component_y << ")" << "Sp = " << sp << endl;
//	x=" << xCoord << " y=" << yCoord << endl;
}

int main(int argc, char** argv)
{
    // setup the button pin(s)

//    wiringPiSetup () ;
//    pinMode(calButPin, INPUT);
//    pullUpDnControl(calButPin, PUD_UP);

    // call init which will start a thread for the wiiMotes

    init();

    boost::thread spheroLoop(&wiiSpheroLoop, argc, argv);

    spheroLoop.join();
}

bool mustConnect = true;

int wiiSpheroLoop(int argc, char **argv)
{
    int opt;
    
    string spheroAddress;
    
    while ((opt = getopt(argc, argv, "celp:s:")) != -1)
    {
	switch (opt)
	{
	    case 'c':
		mustConnect = !mustConnect;
		break;
	    case 'e':
		errReport = !errReport;
		break;
	    case 'l':
		logReport = !logReport;
		break;
	    case 'p':
		int temp;
		
		temp = atoi(optarg);

		if ((temp >= 30) && (temp <= 200))
		{
		    // update global percent

		    percent = temp;
		}
		break;
	    case 's':
		int spheroIndex;

		spheroIndex = atoi(optarg);
		
		if ((spheroIndex < 0) || spheroIndex > 2)
		{
		    spheroIndex = 0;
		}
		spheroAddressIndex = spheroIndex;
		break;
	    default:
		fprintf(stderr, "Usage %s [-l logging] [-e error] [-c no sphero connect] [-p #] [-s #]",
			argv[0]);
		exit(-200);
	}
    }

    // since we have more then one Sphero and I want this headless, I want to read a file
    // with all the Sphero Addresses in it and use the first address, reorder the addresses by
    // putting the first address last and rewrite the file, this way we will sooner or later
    // try all the Sphero(s) and if we are connected to one Sphero, the next time we re-run
    // we will use the next Sphero.
   
    // we now should have any Wii's that where up already connected and we can look for a
    // sphero

    struct termios t;
    
    cout << "Setting ~ICANON" << std::endl;
    tcgetattr(0, &t); //get the current terminal I/O structure
    t.c_lflag &= ~ICANON; //Manipulate the flag bits to do what you want it to do
    tcsetattr(0, TCSANOW, &t); //Apply the new settings
     
    while(true)
    {
	static timeval startTime = {0, 0};
	struct timeval nowTime;
	int ch;
	char userInput;
//	static bool butWasDwn = false;
	
	// we should only due this every once in a while

	ch = std::cin.peek();

	if (ch > 0)
	{
	    std::cin.get(userInput);
//	    printf("ch1 = %x [%c]\n", userInput,userInput);

	    if (userInput == 'q' || userInput == 'Q')
	    {
		tcgetattr(0, &t); //get the current terminal I/O structure
		t.c_lflag |= ICANON; //Manipulate the flag bits to do what you want it to do
		tcsetattr(0, TCSANOW, &t); //Apply the new settings
		cout << "Exiting wii-sphero" << std::endl;
		exit(0);
	    }
	    else if (userInput == 'b' || userInput == 'B')
	    {
		// read the balance board and set the values as the base
            }
	    else if (userInput == 'c' || userInput == 'C')
	    {
		// toggle the sphero set heading mode
	    }
	}

	// if we want to connect a new sphero, we need to disconnect the old one

	if (switchSphero)
	{
	    switchSphero = false;

	    if (isConnected())
	    {
		// send it to sleep immediately
		
		sm.getSphero()->sleep(0, 0, 0);
		sm.disconnectSphero(0);
	    }
	}
	
	if (startTime.tv_sec == 0)
	{
	    // looks like we need to get the startTime

	    gettimeofday(&startTime, NULL);
	    // cheat to get a quick first try

	    startTime.tv_sec -= 6;
	}
	
	// get the current time, we really only want usecs

	gettimeofday(&nowTime, NULL);

	if(mustConnect && !isConnected() && WiiMote::count())
	{
	    if ((elapseTime(&startTime, &nowTime) / 1000) >= 5)
	    {
		// get the next sphero to try
		
		spheroAddress = getNextSpheroAddress();
		
		cout << "Trying address " << spheroAddress << endl;
		
		sm.connectSphero(spheroAddress);

		if (sm.getNbSpheros() != 0)
		{
		    sm.selectSphero(0);

		    if (isConnected())
		    {
			// turn on collision detection
			sm.getSphero()->enableCollisionDetection(80,40,80,60,100);
			sm.getSphero()->onCollision((callback_collision_t) &spheroCollision);

			// do something on disconnect
			sm.getSphero()->onDisconnect((callback_disconnect_t) &spheroDisconnect);
		    }
		}
		
		startTime = nowTime;
	    }
	}
	else
	{
	    if (isConnected() && ((elapseTime(&startTime, &nowTime) / 1000) >= 5))
	    {
		static uint32_t times = 0;

		//	    sm.getSphero()->roll((uint8_t) 60, (uint16_t) 0, 1);

		// if we are not stopping color changes then do it now
		
		if (!noColorChange)
		{
		    if (times++ % 2)
		    {
			sm.getSphero()->setColor(0xff, 0x00, 0x00);
		    }
		    else
		    {
			sm.getSphero()->setColor(0x00, 0x00, 0xff);
		    }
		}

		startTime = nowTime;
	    }
	}

	pthread_yield();
    }

    return 0;
}
