/*
 * pwcmech_if.h
 *
 *  Created on: Aug 24, 2011
 *      Author: raul
 */

#ifndef PWCMECH_IF_H_
#define PWCMECH_IF_H_

#include <devif/types.h>

struct pwcmech
{
	struct efsm *		efsm;		//this pointer's name must be efsm
	struct pwcblock * 	dev;
	struct usbcom * 	com;
};
typedef struct pwcmech DevMech;		//this structure must be typedefed to DevMech

//we should export enumerations from pwcblock.h and usbcom.h
//use the ones they have and rename them to something meaningful for mode setting

enum transfer_error;
enum usb_error;
enum cb_ret;

//register configuration
enum transfer_error _setAgcMode(struct pwcmech * pwcmech, unsigned int mode);
enum transfer_error _getAgcMode(struct pwcmech * pwcmech, unsigned int * mode);

enum transfer_error _setShutterMode(struct pwcmech * pwcmech, unsigned int mode);
enum transfer_error _getShutterMode(struct pwcmech * pwcmech, unsigned int * mode);

enum transfer_error _setAwbMode(struct pwcmech * pwcmech, unsigned int mode);
enum transfer_error _getAwbMode(struct pwcmech * pwcmech, unsigned int * mode);

enum transfer_error _setContourMode(struct pwcmech * pwcmech, unsigned int mode);
enum transfer_error _getContourMode(struct pwcmech * pwcmech, unsigned int * mode);

enum transfer_error _getBrightness(struct pwcmech * pwcmech, unsigned int * value);
enum transfer_error _setBrightness(struct pwcmech * pwcmech, unsigned int value);

enum transfer_error _getContrast(struct pwcmech * pwcmech, unsigned int * value);
enum transfer_error _setContrast(struct pwcmech * pwcmech, unsigned int value);

enum transfer_error _getGamma(struct pwcmech * pwcmech, unsigned int * value);
enum transfer_error _setGamma(struct pwcmech * pwcmech, unsigned int value);

enum transfer_error _getSaturation(struct pwcmech * pwcmech, unsigned int *value);
enum transfer_error _setSaturation(struct pwcmech * pwcmech, unsigned int value);

enum transfer_error _setAgc(struct pwcmech * pwcmech, unsigned int value);
enum transfer_error _getAgc(struct pwcmech * pwcmech, unsigned int *value);

enum transfer_error _setShutterSpeed(struct pwcmech * pwcmech, unsigned int value);
enum transfer_error _getShutterSpeed(struct pwcmech * pwcmech, unsigned int *value);

enum transfer_error _setRedGain(struct pwcmech * pwcmech, unsigned int value);
enum transfer_error _getRedGain(struct pwcmech * pwcmech, unsigned int *value);

enum transfer_error _setBlueGain(struct pwcmech * pwcmech, unsigned int value);
enum transfer_error _getBlueGain(struct pwcmech * pwcmech, unsigned int *value);

enum transfer_error _setWbSpeed(struct pwcmech * pwcmech, unsigned int speed);
enum transfer_error _getWbSpeed(struct pwcmech * pwcmech, unsigned int *value);

enum transfer_error _setWbDelay(struct pwcmech * pwcmech, unsigned int delay);
enum transfer_error _getWbDelay(struct pwcmech * pwcmech, unsigned int *value);

enum transfer_error _setLeds(struct pwcmech * pwcmech, unsigned int on_period, unsigned int off_period);
enum transfer_error _getLeds(struct pwcmech * pwcmech, unsigned int *on_period, unsigned int *off_period);

enum transfer_error _setContour(struct pwcmech * pwcmech, unsigned int contour);
enum transfer_error _getContour(struct pwcmech * pwcmech, unsigned int *contour);

enum transfer_error _setBacklight(struct pwcmech * pwcmech, unsigned int backlight);
enum transfer_error _getBacklight(struct pwcmech * pwcmech, unsigned int *backlight);

enum transfer_error _setColourMode(struct pwcmech * pwcmech, unsigned int colour);
enum transfer_error _getColourMode(struct pwcmech * pwcmech, unsigned int *colour);

enum transfer_error _setFlickerMode(struct pwcmech * pwcmech, unsigned int flicker);
enum transfer_error _getFlickerMode(struct pwcmech * pwcmech, unsigned int *flicker);

enum transfer_error _setDynamicNoise(struct pwcmech * pwcmech, unsigned int noise);
enum transfer_error _getDynamicNoise(struct pwcmech * pwcmech, unsigned int *noise);

enum transfer_error _getSensor(struct pwcmech * pwcmech, unsigned int *sensor);

enum transfer_error _setPower(struct pwcmech * pwcmech, unsigned int power);

enum transfer_error _setMotor(struct pwcmech * pwcmech, unsigned int pan, unsigned int tilt);
enum transfer_error _resetMotor(struct pwcmech * pwcmech, bool pan, bool tilt);

enum transfer_error _restoreUser(struct pwcmech * pwcmech);
enum transfer_error _saveUser(struct pwcmech * pwcmech);
enum transfer_error _restoreFactory(struct pwcmech * pwcmech);

enum usb_error _getMotorStatus(struct pwcmech * pwcmech, uint8_t * buf, size_t len);
enum usb_error _sendVideoCommand(struct pwcmech * pwcmech, uint8_t * buf, size_t len);

//stream configuration
enum usb_error _setVideoMode(struct pwcmech * pwcmech, unsigned int mode);

enum usb_error _assignVideoBuffer(struct pwcmech * pwcmech, uint8_t * buf, size_t len);
enum usb_error _unassignVideoBuffers(struct pwcmech * pwcmech);

enum usb_error _registerVideoCallback(struct pwcmech * pwcmech, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context);
enum usb_error _acknowledgeVideoCallback(struct pwcmech * pwcmech);
enum usb_error _releaseVideoCallback(struct pwcmech * pwcmech);

enum usb_error _assignPhotoBuffer(struct pwcmech * pwcmech, uint8_t * buf, size_t len);
enum usb_error _unassignPhotoBuffers(struct pwcmech * pwcmech);

enum usb_error _registerPhotoCallback(struct pwcmech * pwcmech, enum cb_ret (*func)(void * context, uint8_t * buf, size_t len, size_t * p_packet_len, size_t nr_of_packets), void * context);
enum usb_error _acknowledgePhotoCallback(struct pwcmech * pwcmech);
enum usb_error _releasePhotoCallback(struct pwcmech * pwcmech);


#endif /* PWCMECH_IF_H_ */
