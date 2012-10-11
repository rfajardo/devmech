/*
 * frame_constructor_if.h
 *
 *  Created on: Apr 15, 2010
 *      Author: rfajardo
 */

#ifndef FRAME_CONSTRUCTOR_IF_H_
#define FRAME_CONSTRUCTOR_IF_H_


struct libusb_transfer;
struct frameBuf;
struct buffering;

//
// Dealing with Communication
//
int compactPacket(struct libusb_transfer * transfer,		//returns 0 when iso_transfer fully compacted
		unsigned char ** p_packet, unsigned int * len);		//return not zero if function finds end of frame
void fillFrame(struct frameBuf * frame_buffer, const unsigned char * p_packet, unsigned int len);
void finishFrame(struct buffering * fbufs, const unsigned char * p_packet, unsigned int len);


#endif /* FRAME_CONSTRUCTOR_IF_H_ */
