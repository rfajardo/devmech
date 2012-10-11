/*
 * pwcmech.c
 *
 *  Created on: Aug 24, 2011
 *      Author: raul
 */

#include "pwcmech_if.h"

#include <devif/devif.h>
#include <usbif/usbif.h>
#include <usbif/streamif.h>

#include "pwcblock.h"
#include "usbcom.h"

#include "pwcmechdebug.h"

enum transfer_error _setAgcMode(struct pwcmech * pwcmech, unsigned int mode)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->agcMode_agcMode, mode);
	if ( ret )
		return ret;

	if ( mode == GETNUM(pwcblock, auto) )
	{
		PWCMECH_DEBUG_MECH("Setting AGC mode to automatic.\n");
		disableRegFile(pwcmech->dev->agcFixedRegFileActive);
		enableRegFile(pwcmech->dev->agcAutoRegFileActive);
	}
	else if ( mode == GETNUM(pwcblock, fixed) )
	{
		PWCMECH_DEBUG_MECH("Setting AGC mode to manual.\n");
		disableRegFile(pwcmech->dev->agcAutoRegFileActive);
		enableRegFile(pwcmech->dev->agcFixedRegFileActive);
	}
	return TRANSFER_OK;
}

enum transfer_error _getAgcMode(struct pwcmech * pwcmech, unsigned int * mode)
{
	enum transfer_error ret;
	*mode = getField(pwcmech->dev, pwcmech->dev->agcMode_agcMode);
	PWCMECH_DEBUG_MECH("Retrieving AGC mode: %x.\n", *mode);
	ret = getTransferError();
	if ( ret == TRANSFER_OK )
	{
		if ( *mode == GETNUM(pwcblock, auto) )
		{
			PWCMECH_DEBUG_MECH("Setting AGC mode to automatic.\n");
			disableRegFile(pwcmech->dev->agcFixedRegFileActive);
			enableRegFile(pwcmech->dev->agcAutoRegFileActive);
		}
		else if ( *mode == GETNUM(pwcblock, fixed) )
		{
			PWCMECH_DEBUG_MECH("Setting AGC mode to manual.\n");
			disableRegFile(pwcmech->dev->agcAutoRegFileActive);
			enableRegFile(pwcmech->dev->agcFixedRegFileActive);
		}
	}
	return ret;
}


enum transfer_error _setShutterMode(struct pwcmech * pwcmech, unsigned int mode)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->shutterMode_shutterMode, mode);
	if ( ret )
		return ret;
	if ( mode == GETNUM(pwcblock, shutterMode_shutterMode_auto) )
	{
		PWCMECH_DEBUG_MECH("Setting Shutter mode to automatic.\n");
		disableRegFile(pwcmech->dev->shutterFixedRegFileActive);
	}
	else if ( mode == GETNUM(pwcblock, shutterMode_shutterMode_fixed) )
	{
		PWCMECH_DEBUG_MECH("Setting Shutter mode to manual.\n");
		enableRegFile(pwcmech->dev->shutterFixedRegFileActive);
	}
	return TRANSFER_OK;
}

enum transfer_error _getShutterMode(struct pwcmech * pwcmech, unsigned int * mode)
{
	enum transfer_error ret;
	*mode = getField(pwcmech->dev, pwcmech->dev->shutterMode_shutterMode);
	PWCMECH_DEBUG_MECH("Retrieving Shutter mode: %x.\n", *mode);
	ret = getTransferError();
	if ( ret == TRANSFER_OK )
	{
		if ( *mode == GETNUM(pwcblock, shutterMode_shutterMode_auto) )
		{
			PWCMECH_DEBUG_MECH("Setting Shutter mode to automatic.\n");
			disableRegFile(pwcmech->dev->shutterFixedRegFileActive);
		}
		else if ( *mode == GETNUM(pwcblock, shutterMode_shutterMode_fixed) )
		{
			PWCMECH_DEBUG_MECH("Setting Shutter mode to manual.\n");
			enableRegFile(pwcmech->dev->shutterFixedRegFileActive);
		}
	}
	return ret;
}


enum transfer_error _setAwbMode(struct pwcmech * pwcmech, unsigned int mode)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->awbMode_awbMode, mode);
	if ( ret )
		return ret;
	if ( mode == GETNUM(pwcblock, awbMode_awbMode_auto) )
	{
		PWCMECH_DEBUG_MECH("Setting AWB mode to automatic.\n");
		disableRegFile(pwcmech->dev->awbOthersRegFileActive);
		enableRegFile(pwcmech->dev->awbAutoRegFileActive);
	}
	else
	{
		PWCMECH_DEBUG_MECH("Setting AWB mode to manual.\n");
		disableRegFile(pwcmech->dev->awbAutoRegFileActive);
		enableRegFile(pwcmech->dev->awbOthersRegFileActive);
	}
	return TRANSFER_OK;
}

enum transfer_error _getAwbMode(struct pwcmech * pwcmech, unsigned int * mode)
{
	enum transfer_error ret;
	*mode = getField(pwcmech->dev, pwcmech->dev->awbMode_awbMode);
	PWCMECH_DEBUG_MECH("Retrieving AWB mode: %x.\n", *mode);
	ret = getTransferError();
	if ( ret == TRANSFER_OK )
	{
		if ( *mode == GETNUM(pwcblock, awbMode_awbMode_auto) )
		{
			PWCMECH_DEBUG_MECH("Setting AWB mode to automatic.\n");
			disableRegFile(pwcmech->dev->awbOthersRegFileActive);
			enableRegFile(pwcmech->dev->awbAutoRegFileActive);
		}
		else
		{
			PWCMECH_DEBUG_MECH("Setting AWB mode to manual.\n");
			disableRegFile(pwcmech->dev->awbAutoRegFileActive);
			enableRegFile(pwcmech->dev->awbOthersRegFileActive);
		}
	}
	return ret;
}


enum transfer_error _setContourMode(struct pwcmech * pwcmech, unsigned int mode)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->contourMode_contourMode, mode);
	if ( ret )
		return ret;

	if ( mode == GETNUM(pwcblock, contourMode_contourMode_auto) )
	{
		PWCMECH_DEBUG_MECH("Setting Contour mode to automatic.\n");
		disableRegFile(pwcmech->dev->contourFixedRegFileActive);
	}
	else if ( mode == GETNUM(pwcblock, contourMode_contourMode_fixed) )
	{
		PWCMECH_DEBUG_MECH("Setting Contour mode to manual.\n");
		enableRegFile(pwcmech->dev->contourFixedRegFileActive);
	}
	return TRANSFER_OK;
}

enum transfer_error _getContourMode(struct pwcmech * pwcmech, unsigned int * mode)
{
	enum transfer_error ret;
	*mode = getField(pwcmech->dev, pwcmech->dev->contourMode_contourMode);
	PWCMECH_DEBUG_MECH("Retrieving Contour mode: %x.\n", *mode);
	ret = getTransferError();
	if ( ret == TRANSFER_OK )
	{
		if ( *mode == GETNUM(pwcblock, contourMode_contourMode_auto) )
		{
			PWCMECH_DEBUG_MECH("Setting Contour mode to automatic.\n");
			disableRegFile(pwcmech->dev->contourFixedRegFileActive);
		}
		else if ( *mode == GETNUM(pwcblock, contourMode_contourMode_fixed) )
		{
			PWCMECH_DEBUG_MECH("Setting Contour mode to manual.\n");
			enableRegFile(pwcmech->dev->contourFixedRegFileActive);
		}
	}
	return ret;
}


enum transfer_error _getBrightness(struct pwcmech * pwcmech, unsigned int * value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->brightness);
	PWCMECH_DEBUG_MECH("Retrieving Brightness register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}

enum transfer_error _setBrightness(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->brightness, value);
	PWCMECH_DEBUG_MECH("Setting Brightness register to: 0x%x.\n", value);
	return ret;
}

/* CONTRAST */
enum transfer_error _getContrast(struct pwcmech * pwcmech, unsigned int * value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->contrast);
	PWCMECH_DEBUG_MECH("Retrieving Contrast register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}

enum transfer_error _setContrast(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->contrast, value);
	PWCMECH_DEBUG_MECH("Setting Contrast register to: 0x%x.\n", value);
	return ret;
}

/* GAMMA */
enum transfer_error _getGamma(struct pwcmech * pwcmech, unsigned int * value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->gamma);
	PWCMECH_DEBUG_MECH("Retrieving Gamma register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}

enum transfer_error _setGamma(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->gamma, value);
	PWCMECH_DEBUG_MECH("Setting Gamma register to: 0x%x.\n", value);
	return ret;
}


/* SATURATION */
enum transfer_error _getSaturation(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->saturation);
	PWCMECH_DEBUG_MECH("Retrieving Saturation register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}

enum transfer_error _setSaturation(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->saturation, value);
	PWCMECH_DEBUG_MECH("Setting Saturation register to: 0x%x.\n", value);
	return ret;
}

/* AGC */

enum transfer_error _setAgc(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->agcFixed.agc, value);
	PWCMECH_DEBUG_MECH("Setting AGC register to: 0x%x.\n", value);
	return ret;
}

enum transfer_error _getAgc(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	unsigned int mode;

	ret = _getAgcMode(pwcmech, &mode);
	if ( ret )
		return ret;

	if (mode == GETNUM(pwcblock, auto) )
	{
		*value = getReg(pwcmech->dev, pwcmech->dev->agcAuto.agc);
		PWCMECH_DEBUG_MECH("AGC automatic mode - retrieving AGC register for current automatic level: 0x%x.\n", *value);
	}
	else
	{
		*value = getReg(pwcmech->dev, pwcmech->dev->agcFixed.agc);
		PWCMECH_DEBUG_MECH("AGC manual mode - retrieving AGC manual set register: 0x%x.\n", *value);
	}

	ret = getTransferError();
	return ret;
}

enum transfer_error _setShutterSpeed(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->shutterFixed.shutterSpeed, value);
	PWCMECH_DEBUG_MECH("Setting Shutter speed register to: 0x%x.\n", value);
	return ret;
}
/* This function is not exported to v4l1, so output values between 0 -> 256 */
enum transfer_error _getShutterSpeed(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->shutter);
	PWCMECH_DEBUG_MECH("Retrieving Shutter speed register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setRedGain(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->awbOthers.redGain, value);
	PWCMECH_DEBUG_MECH("Setting RedGain register to: 0x%x.\n", value);
	return ret;
}

enum transfer_error _getRedGain(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	unsigned int mode;

	ret = _getAwbMode(pwcmech, &mode);
	if ( ret )
		return ret;

	if (mode == GETNUM(pwcblock, awbMode_awbMode_auto) )
	{
		*value = getReg(pwcmech->dev, pwcmech->dev->awbAuto.redGain);
		PWCMECH_DEBUG_MECH("AWB automatic mode - retrieving RedGain register for current automatic level: 0x%x.\n", *value);
	}
	else
	{
		*value = getReg(pwcmech->dev, pwcmech->dev->awbOthers.redGain);
		PWCMECH_DEBUG_MECH("AWB manual mode - retrieving RedGain manual set register: 0x%x.\n", *value);
	}

	ret = getTransferError();
	return ret;
}


enum transfer_error _setBlueGain(struct pwcmech * pwcmech, unsigned int value)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->awbOthers.blueGain, value);
	PWCMECH_DEBUG_MECH("Setting BlueGain register to: 0x%x.\n", value);
	return ret;
}

enum transfer_error _getBlueGain(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	unsigned int mode;

	ret = _getAwbMode(pwcmech, &mode);
	if ( ret )
		return ret;

	if (mode == GETNUM(pwcblock, awbMode_awbMode_auto) )
	{
		*value = getReg(pwcmech->dev, pwcmech->dev->awbAuto.blueGain);
		PWCMECH_DEBUG_MECH("AWB automatic mode - retrieving BlueGain register for current automatic level: 0x%x.\n", *value);
	}
	else
	{
		*value = getReg(pwcmech->dev, pwcmech->dev->awbOthers.blueGain);
		PWCMECH_DEBUG_MECH("AWB manual mode - retrieving BlueGain manual set register: 0x%x.\n", *value);
	}

	ret = getTransferError();
	return ret;
}


enum transfer_error _setWbSpeed(struct pwcmech * pwcmech, unsigned int speed)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->wbSpeed, speed);
	PWCMECH_DEBUG_MECH("Setting WbSpeed register to: 0x%x.\n", speed);
	return ret;
}

enum transfer_error _getWbSpeed(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->wbSpeed);
	PWCMECH_DEBUG_MECH("Retrieving WbSpeed register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setWbDelay(struct pwcmech * pwcmech, unsigned int delay)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->wbDelay, delay);
	PWCMECH_DEBUG_MECH("Setting WbDelay register to: 0x%x.\n", delay);
	return ret;
}

enum transfer_error _getWbDelay(struct pwcmech * pwcmech, unsigned int *value)
{
	enum transfer_error ret;
	*value = getReg(pwcmech->dev, pwcmech->dev->wbDelay);
	PWCMECH_DEBUG_MECH("Retrieving WbDelay register: 0x%x.\n", *value);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setLeds(struct pwcmech * pwcmech, unsigned int on_period, unsigned int off_period)
{
	enum transfer_error ret;
	ret = delayedSetField(pwcmech->dev->on_period, on_period);
	if ( ret )
		return ret;
	ret = delayedSetField(pwcmech->dev->off_period, off_period);
	if ( ret )
		return ret;
	ret = delayedSetReg(pwcmech->dev, pwcmech->dev->led);
	PWCMECH_DEBUG_MECH("Setting LED registers. Set on period to: %d. Set off period to: %d .\n", on_period, off_period);
	return ret;
}

enum transfer_error _getLeds(struct pwcmech * pwcmech, unsigned int *on_period, unsigned int *off_period)
{
	enum transfer_error ret;

	*on_period = getField(pwcmech->dev, pwcmech->dev->on_period);
	ret = getTransferError();
	if ( ret )
		return ret;

	*off_period = getField(pwcmech->dev, pwcmech->dev->off_period);
	ret = getTransferError();
	PWCMECH_DEBUG_MECH("Retrieving LED registers. On period set to: %d. Off period set to: %d .\n", *on_period, *off_period);

	return ret;
}


enum transfer_error _setContour(struct pwcmech * pwcmech, unsigned int contour)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->contourFixed.contour, contour);
	PWCMECH_DEBUG_MECH("Setting Contour register to: 0x%x.\n", contour);
	return ret;
}

enum transfer_error _getContour(struct pwcmech * pwcmech, unsigned int *contour)
{
	enum transfer_error ret;
	*contour = getReg(pwcmech->dev, pwcmech->dev->contourFixed.contour);
	PWCMECH_DEBUG_MECH("Retrieving Contour register: 0x%x.\n", *contour);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setBacklight(struct pwcmech * pwcmech, unsigned int backlight)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->backlight_backlight, backlight);
	PWCMECH_DEBUG_MECH("Setting Backlight register to: 0x%x.\n", backlight);
	return ret;
}

enum transfer_error _getBacklight(struct pwcmech * pwcmech, unsigned int *backlight)
{
	enum transfer_error ret;
	*backlight = getField(pwcmech->dev, pwcmech->dev->backlight_backlight);
	PWCMECH_DEBUG_MECH("Retrieving Backlight register: 0x%x.\n", *backlight);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setColourMode(struct pwcmech * pwcmech, unsigned int colour)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->colourMode_colourMode, colour);
	PWCMECH_DEBUG_MECH("Setting ColourMode register to: 0x%x.\n", colour);
	return ret;
}

enum transfer_error _getColourMode(struct pwcmech * pwcmech, unsigned int *colour)
{
	enum transfer_error ret;
	*colour = getField(pwcmech->dev, pwcmech->dev->colourMode_colourMode);
	PWCMECH_DEBUG_MECH("Retrieving ColourMode register: 0x%x.\n", *colour);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setFlickerMode(struct pwcmech * pwcmech, unsigned int flicker)
{
	enum transfer_error ret;
	ret = setField(pwcmech->dev, pwcmech->dev->flickerMode_flickerMode, flicker);
	PWCMECH_DEBUG_MECH("Setting FlickerMode register to: 0x%x.\n", flicker);
	return ret;
}

enum transfer_error _getFlickerMode(struct pwcmech * pwcmech, unsigned int *flicker)
{
	enum transfer_error ret;
	*flicker = getField(pwcmech->dev, pwcmech->dev->flickerMode_flickerMode);
	PWCMECH_DEBUG_MECH("Retrieving FlickerMode register: 0x%x.\n", *flicker);
	ret = getTransferError();
	return ret;
}


enum transfer_error _setDynamicNoise(struct pwcmech * pwcmech, unsigned int noise)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->dynamicNoiseControl, noise);
	PWCMECH_DEBUG_MECH("Setting DynamicNoise register to: 0x%x.\n", noise);
	return ret;
}

enum transfer_error _getDynamicNoise(struct pwcmech * pwcmech, unsigned int *noise)
{
	enum transfer_error ret;
	*noise = getReg(pwcmech->dev, pwcmech->dev->dynamicNoiseControl);
	PWCMECH_DEBUG_MECH("Retrieving DynamicNoise register: 0x%x.\n", *noise);
	ret = getTransferError();
	return ret;
}


enum transfer_error _getSensor(struct pwcmech * pwcmech, unsigned int *sensor)
{
	enum transfer_error ret;
	*sensor = getReg(pwcmech->dev, pwcmech->dev->sensorType);
	PWCMECH_DEBUG_MECH("Retrieving SensorType register: 0x%x.\n", *sensor);
	ret = getTransferError();
	return ret;
}


/* POWER */
enum transfer_error _setPower(struct pwcmech * pwcmech, unsigned int power)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->powerMode, power);
	PWCMECH_DEBUG_MECH("Setting Power register to: 0x%x.\n", power);
	return ret;
}

enum transfer_error _setMotor(struct pwcmech * pwcmech, unsigned int pan, unsigned int tilt)
{
	enum transfer_error ret;
	ret = delayedSetField(pwcmech->dev->pan, pan);
	if ( ret )
		return ret;
	ret = delayedSetField(pwcmech->dev->tilt, tilt);
	if ( ret )
		return ret;
	ret = delayedSetReg(pwcmech->dev, pwcmech->dev->motor);
	PWCMECH_DEBUG_MECH("Setting motor register. Set pan to: %d. Set tilt to: %d.\n", pan, tilt);
	return ret;
}

enum transfer_error _resetMotor(struct pwcmech * pwcmech, bool pan, bool tilt)
{
	enum transfer_error ret;
	ret = delayedSetField(pwcmech->dev->resetPan, pan ? 1 : 0);
	if ( ret )
		return ret;
	ret = delayedSetField(pwcmech->dev->resetTilt, tilt ? 1 : 0);
	if ( ret )
		return ret;
	ret = delayedSetReg(pwcmech->dev, pwcmech->dev->resetMotor);
	if ( pan || tilt )
		PWCMECH_DEBUG_MECH("Resetting motor register.\n");
	if ( pan )
		PWCMECH_DEBUG_MECH("Resetting pan.\n");
	if ( tilt )
		PWCMECH_DEBUG_MECH("Resetting tilt.\n");
	return ret;
}


enum transfer_error _restoreUser(struct pwcmech * pwcmech)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->restoreUser, 0);
	PWCMECH_DEBUG_MECH("Restoring user configuration. Setting restoreUser strobe register to: 0x%x.\n", 0);
	return ret;
}

enum transfer_error _saveUser(struct pwcmech * pwcmech)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->saveUser, 0);
	PWCMECH_DEBUG_MECH("Storing user configuration. Setting saveUser strobe register to: 0x%x.\n", 0);
	return ret;
}

enum transfer_error _restoreFactory(struct pwcmech * pwcmech)
{
	enum transfer_error ret;
	ret = setReg(pwcmech->dev, pwcmech->dev->restoreFactory, 0);
	PWCMECH_DEBUG_MECH("Restoring factory configuration. Setting restoreFactory strobe register to: 0x%x.\n", 0);
	return ret;
}


enum usb_error _getMotorStatus(struct pwcmech * pwcmech, uint8_t * buf, size_t len)
{
	enum usb_error ret;
	ret = transfer_setup(pwcmech->com, pwcmech->com->setup_packets.getPtStatus, buf, len);
	PWCMECH_DEBUG_MECH("Acquiring motor status of the camera, pan and tilt.\n");
	return ret;
}

enum usb_error _sendVideoCommand(struct pwcmech * pwcmech, uint8_t * buf, size_t len)
{
	enum usb_error ret;
	ret = transfer_setup(pwcmech->com, pwcmech->com->setup_packets.setVideoMode, buf, len);
	PWCMECH_DEBUG_MECH("Seding video command. This configures the picture mode of the camera. It has to be adapted to the selected alternate interface.\n");
	return ret;
}


//stream configuration
enum usb_error _setVideoMode(struct pwcmech * pwcmech, unsigned int mode)
{
	enum usb_error ret;
	AltIf * altif;
	altif = usb_get_alt_if(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.cameraStreamConf, mode);
	if ( altif == NULL )
	{
		PWCMECH_ERROR("Error searching for the specified alternate interface. Alternate interface not found.\n");
		return USB_ERROR_INVALID_PARAM;
	}
	else
	{
		ret = usb_set_alt_if(pwcmech->com, altif);
		PWCMECH_DEBUG_MECH("Alternate interface found. Returning alternate interface structure. Alternate interface number: %d\n", altif->bAlternateSetting);
	}
	return ret;
}


enum usb_error _assignVideoBuffer(struct pwcmech * pwcmech, uint8_t * buf, size_t len)
{
	enum usb_error ret;
	ret = register_dma_space(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Video, buf, len);
	PWCMECH_DEBUG_MECH("Assigning buffer to Isochronous endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Video->bEndpointAddress);
	return ret;
}

enum usb_error _unassignVideoBuffers(struct pwcmech * pwcmech)
{
	enum usb_error ret;
	ret = unregister_dma_spaces(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Video);
	PWCMECH_DEBUG_MECH("Deregistering buffer of Isochronous endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Video->bEndpointAddress);
	return ret;
}


enum usb_error _registerVideoCallback(struct pwcmech * pwcmech, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context)
{
	enum usb_error ret;
	ret = register_dma_interrupt(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Video, func, context);
	PWCMECH_DEBUG_MECH("Registering callback function to Isochronous endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Video->bEndpointAddress);
	return ret;
}

enum usb_error _acknowledgeVideoCallback(struct pwcmech * pwcmech)
{
	enum usb_error ret;
	ret = unlock_dma_spaces(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Video);
	PWCMECH_DEBUG_MECH("Re-issuing transfers of Isochronous endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Video->bEndpointAddress);
	return ret;
}

enum usb_error _releaseVideoCallback(struct pwcmech * pwcmech)
{
	enum usb_error ret;
	ret = unregister_dma_interrupt(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Video);
	PWCMECH_DEBUG_MECH("Disabling callback function of Isochronous endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Video->bEndpointAddress);
	return ret;
}


enum usb_error _assignPhotoBuffer(struct pwcmech * pwcmech, uint8_t * buf, size_t len)
{
	enum usb_error ret;
	ret = register_interrupt_buffer(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Button, buf, len);
	PWCMECH_DEBUG_MECH("Assigning buffer to Interrupt endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Button->bEndpointAddress);
	return ret;
}

enum usb_error _unassignPhotoBuffers(struct pwcmech * pwcmech)
{
	enum usb_error ret;
	ret = unregister_interrupt_buffers(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Button);
	PWCMECH_DEBUG_MECH("Deregistering buffer of Interrupt endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Button->bEndpointAddress);
	return ret;
}


enum usb_error _registerPhotoCallback(struct pwcmech * pwcmech, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context)
{
	enum usb_error ret;
	ret = register_interrupt(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Button, func, context);
	PWCMECH_DEBUG_MECH("Registering callback function to Interrupt endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Button->bEndpointAddress);
	return ret;
}

enum usb_error _acknowledgePhotoCallback(struct pwcmech * pwcmech)
{
	enum usb_error ret;
	ret = unlock_interrupt(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Button);
	PWCMECH_DEBUG_MECH("Re-issuing transfers of Interrupt endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Button->bEndpointAddress);
	return ret;
}

enum usb_error _releasePhotoCallback(struct pwcmech * pwcmech)
{
	enum usb_error ret;
	ret = unregister_dma_interrupt(pwcmech->com, pwcmech->com->standard_descriptors.highspeed.cameraStream.Button);
	PWCMECH_DEBUG_MECH("Disabling callback function of Interrupt endpoint number 0x%x.\n", pwcmech->com->standard_descriptors.highspeed.cameraStream.Button->bEndpointAddress);
	return ret;
}
