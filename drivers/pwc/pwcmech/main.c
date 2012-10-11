/*
 * main.c
 *
 *  Created on: Jul 26, 2011
 *      Author: raul
 */

#include "usbcom.h"
#include "pwcblock.h"
#include <usbif/usbif.h>
#include <usbif/streamif.h>
#include <devif/devif.h>

#include <devif/com.h>

#include "pwcmech_if.h"

#include <stdio.h>

void finish(struct pwcmech * pwcmech)
{
	usb_exit(pwcmech->com);

	ifstop_pwcblock();
	ifstop_usbcom();
}

int main(int argc, char * argv[])
{
	struct pwcmech * pwcmech;
	pwcmech->dev = ifinit_pwcblock();
	pwcmech->com = ifinit_usbcom();

	ifbind_pwcblock(pwcmech->dev, pwcmech->com);

	//use driver
	usb_init(pwcmech->com);
	if ( usb_open(pwcmech->com) )
	{
		finish(pwcmech);
		return 0;
	}

	//should work seamlessly
	usb_if_claim(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.cameraStreamConf);

	setVideoMode(pwcmech, 9);

	setLeds(pwcmech, 10,5);

	usb_release_interface(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.cameraStreamConf);

	usb_close(pwcmech->com);

	finish(pwcmech);

	return 0;
}
