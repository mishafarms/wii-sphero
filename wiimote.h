#ifndef _WII_MOTE_H
#define _WII_MOTE_H

#include <vector>
#include <string>
#include <cmath>
#include <unistd.h>

#include "xwiimote.h"
#include <boost/signals2.hpp>
#include <boost/bind.hpp>

using namespace boost;
using namespace std;

#include "vec.h"

class WiiMote
{
protected:
    static vector <class WiiMote *> wiiMotes;
    static struct xwii_monitor *monitor;  // this is the monitor object
    unsigned int cacheIfaces;  // which interfaces we thought we had
    bool connected;

    void connectInThread(char *);

public:
    int unitId;                // this will be which LED is on
    struct xwii_iface *iFace;  // this is where we store the xwii_iface
    int faces;                 // keep track of Wii interfaces available for this Wii
    string name;
    Vec3<int16_t> cal_zero;
    Vec3<uint16_t> cal_g;
    Vec3<float> gForce;       // place to store what the G forces are
    Angle3<float> orientation;// how are we oriented in the world
    Vec2<int16_t> joyStick;   // this is the nunchuk joystick values if there is one
    int16_t bbCal[4];         // amount to remove from each weight
    int16_t bbWeights[5];     // each entry is one sensor on the balance board tr, br, tl, bl
                              // the last entry is the total
    bool calibrate;           // should we use the current data to fill in the bbCal
    typedef signals2::signal<void (WiiMote *, int)> WiiSignal;

    static WiiSignal wiiSignal;
    static WiiSignal wiiChangeSignal;
    
    typedef signals2::signal<void (WiiMote *)> AccelSignal;
    typedef signals2::signal<void (WiiMote *)> JoyStickSignal;
    typedef signals2::signal<void (WiiMote *)> BalanceSignal;
    typedef signals2::signal<void (WiiMote *, unsigned int, unsigned int)> KeySignal;

    AccelSignal accelSig;
    JoyStickSignal joyStickSig;
    BalanceSignal balanceSig;
    KeySignal keySig;
    
    WiiMote()
    {
	wiiMotes.push_back(this);
	unitId = wiiMotes.size() - 1;  // your unitId is always your index
	connected = false;
    }
    
    WiiMote(char *name)
    {
	wiiMotes.push_back(this);
	unitId = wiiMotes.size() - 1;  // your unitId is always your index
	connected = false;

	// we should do the new and get things started

	connect(name);
    }
    
    ~WiiMote()
    {
	vector <class WiiMote *>::iterator temp;

	// unreference the xwii_iface

	xwii_iface_unref(iFace);

	// now remove the WiiMote

	for (temp = wiiMotes.begin() ; temp != wiiMotes.end() ;)
	{
	    if (*temp == this)
	    {
		wiiMotes.erase(temp);
		// that moved the interator
	    }
	    else
	    {
		// make sure the unitId matches the index

		if ((*temp)->unitId != distance(wiiMotes.begin(), temp))
		{
		    // we should call a signal and if the results are 0
		    // then update and if not, then leave it alone
		    
		    (*temp)->unitId = distance(wiiMotes.begin(), temp);
		    (*temp)->setLeds();

		    // signal the change

		    wiiChangeSignal((*temp), (*temp)->unitId);
		}
		
		temp++;
	    }
	}
    }

    int openIfaces(void);

    int setLeds(void);

    int connect(char *name);

    void readCalData(void);

    void calcGforce(int32_t x, int32_t y, int32_t z);

    void calcOrientation(void);

    void keyEvent(xwii_event *);

    void accelEvent(xwii_event *);
    
    void nunchukMove(xwii_event *);

    void balanceBoard(xwii_event *);
    
    void rumbleOff(int);
    
    static class WiiMote *index(size_t position)
    {
	// I just want to return the wiiMote from our vector

	return WiiMote::wiiMotes[position];
    }

    static int count(void)
    {
	return WiiMote::wiiMotes.size();
    }
    
    static int getCurrent(string type)
    {
	char *xwiiName;

	if (WiiMote::monitor == (struct xwii_monitor *) NULL)
	{
	    WiiMote::monitor = xwii_monitor_new(true, false); // poll and use udev
	}
	
	// this is our guy so poll

	while((xwiiName = xwii_monitor_poll(monitor)) != NULL)
	{
	    class WiiMote *wii;
	
	    cout << "We just saw " << type << " Wii " << xwiiName << endl;
	    
	    wii = new WiiMote(xwiiName);

	    if (wii == NULL)
	    {
		cout << "This can never happen" << endl;
	    }
	}

	return wiiMotes.size();
    }

    static int fillInPoll(struct pollfd *fds, int maxFds)
    {
	vector<class WiiMote *>::iterator i;
	int fds_num;
	int x = 0;
    
	memset(fds, 0, sizeof(*fds) * maxFds);
	fds_num = 0;
	
	for (i = wiiMotes.begin() ;
	     (i != wiiMotes.end()) && (fds_num < (maxFds - 1)) ; ++i, ++x)
	{
	    if ((*i)->connected)
	    {
		fds[fds_num].fd = xwii_iface_get_fd((*i)->iFace);
		fds[fds_num].events = POLLIN;
		fds_num++;
	    }
	}

	fds[fds_num].fd = xwii_monitor_get_fd(monitor, false);
	fds[fds_num].events = POLLIN;
	fds_num++;

	return fds_num;
    }

    static void init(void);
    static void loop(void);
};

#endif /* _WII_MOTE_H */
