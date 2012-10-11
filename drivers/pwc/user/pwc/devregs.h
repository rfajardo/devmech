/*
 * devregs.h
 *
 *  Created on: Mar 18, 2010
 *      Author: rfajardo
 */

#ifndef DEVREGS_H_
#define DEVREGS_H_


#define DEVREG_VID							0x046d
#define DEVREG_PID							0x08b1

#define DEVREG_DATA_PIPE					5
#define DEVREG_INTERFACE					0
#define DEVREG_ALT_SETTING					9


#define VIDEO_MODE_STRING_LEN				12
static const unsigned char video_mode_string[VIDEO_MODE_STRING_LEN] =
							{ 0x24, 0xF4, 0x70, 0x18, 0x9C, 0x15,
							0x7C, 0x03, 0x48, 0xBC, 0x03, 0x80 };


#define SET_EP_STREAM_CTL 					0x07
#define VIDEO_OUTPUT_CONTROL_FORMATTER 		0x0100


#endif /* DEVREGS_H_ */
