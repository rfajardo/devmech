/*
 * classed_xml.c
 *
 *  Created on: Jun 24, 2011
 *      Author: raul
 */

#include <stdio.h>
#include <libusb.h>

#include "classed_xml.h"

#include "devtree.h"
#include "usbmisc.h"
#include "std_xml.h"
#include "getusb.h"
#include "names.h"
#include "lsusb.h"


static int do_report_desc = 1;


/* ---------------------------------------------------------------------- */

/*
 * Audio Class descriptor dump
 */

struct bmcontrol {
	const char *name;
	unsigned int bit;
};

static const struct bmcontrol uac2_interface_header_bmcontrols[] = {
	{ "Latency control",	0 },
	{ NULL }
};

static const struct bmcontrol uac_fu_bmcontrols[] = {
	{ "Mute",		0 },
	{ "Volume",		1 },
	{ "Bass",		2 },
	{ "Mid",		3 },
	{ "Treble",		4 },
	{ "Graphic Equalizer",	5 },
	{ "Automatic Gain",	6 },
	{ "Delay",		7 },
	{ "Bass Boost",		8 },
	{ "Loudness",		9 },
	{ "Input gain",		10 },
	{ "Input gain pad",	11 },
	{ "Phase inverter",	12 },
	{ NULL }
};

static const struct bmcontrol uac2_input_term_bmcontrols[] = {
	{ "Copy Protect",	0 },
	{ "Connector",		1 },
	{ "Overload",		2 },
	{ "Cluster",		3 },
	{ "Underflow",		4 },
	{ "Overflow",		5 },
	{ NULL }
};

static const struct bmcontrol uac2_output_term_bmcontrols[] = {
	{ "Copy Protect",	0 },
	{ "Connector",		1 },
	{ "Overload",		2 },
	{ "Underflow",		3 },
	{ "Overflow",		4 },
	{ NULL }
};

static const struct bmcontrol uac2_mixer_unit_bmcontrols[] = {
	{ "Cluster",		0 },
	{ "Underflow",		1 },
	{ "Overflow",		2 },
	{ NULL }
};

static const struct bmcontrol uac2_extension_unit_bmcontrols[] = {
	{ "Enable",		0 },
	{ "Cluster",		1 },
	{ "Underflow",		2 },
	{ "Overflow",		3 },
	{ NULL }
};

static const struct bmcontrol uac2_clock_source_bmcontrols[] = {
	{ "Clock Frequency",	0 },
	{ "Clock Validity",	1 },
	{ NULL }
};

static const struct bmcontrol uac2_clock_selector_bmcontrols[] = {
	{ "Clock Selector",	0 },
	{ NULL }
};

static const struct bmcontrol uac2_clock_multiplier_bmcontrols[] = {
	{ "Clock Numerator",	0 },
	{ "Clock Denominator",	1 },
	{ NULL }
};

static const struct bmcontrol uac2_selector_bmcontrols[] = {
	{ "Selector",	0 },
	{ NULL }
};

static const char * const chconfig_uac2[] = {
	"Front Left (FL)", "Front Right (FR)", "Front Center (FC)", "Low Frequency Effects (LFE)",
	"Back Left (BL)", "Back Right (BR)", "Front Left of Center (FLC)", "Front Right of Center (FRC)", "Back Center (BC)",
	"Side Left (SL)", "Side Right (SR)",
	"Top Center (TC)", "Top Front Left (TFL)", "Top Front Center (TFC)", "Top Front Right (TFR)", "Top Back Left (TBL)",
	"Top Back Center (TBC)", "Top Back Right (TBR)", "Top Front Left of Center (TFLC)", "Top Front Right of Center (TFRC)",
	"Left Low Frequency Effects (LLFE)", "Right Low Frequency Effects (RLFE)",
	"Top Side Left (TSL)", "Top Side Right (TSR)", "Bottom Center (BC)",
	"Back Left of Center (BLC)", "Back Right of Center (BRC)"
};



static const struct bmcontrol uac2_as_interface_bmcontrols[] = {
	{ "Active Alternate Setting",	0 },
	{ "Valid Alternate Setting",	1 },
	{ NULL }
};



static const struct bmcontrol uac2_audio_endpoint_bmcontrols[] = {
	{ "Pitch",		0 },
	{ "Data Overrun",	1 },
	{ "Data Underrun",	2 },
	{ NULL }
};


static void xml_audio_bmcontrols(const char *prefix, int bmcontrols, const struct bmcontrol *list, int protocol)
{
	while (list->name) {
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (bmcontrols & (1 << list->bit))
				printf("%s%s Control\n", prefix, list->name);

			break;

		case USB_AUDIO_CLASS_2: {
			const char * const ctrl_type[] = { "read-only", "ILLEGAL (0b10)", "read/write" };
			int ctrl = (bmcontrols >> (list->bit * 2)) & 0x3;

			if (ctrl)
				printf("%s%s Control (%s)\n", prefix, list->name, ctrl_type[ctrl-1]);

			break;
		}

		} /* switch */

		list++;
	}
}

void xml_audiocontrol_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const chconfig[] = {
		"Left Front (L)", "Right Front (R)", "Center Front (C)", "Low Freqency Enhancement (LFE)",
		"Left Surround (LS)", "Right Surround (RS)", "Left of Center (LC)", "Right of Center (RC)",
		"Surround (S)", "Side Left (SL)", "Side Right (SR)", "Top (T)"
	};
	static const char * const clock_source_attrs[] = {
		"External", "Internal fixed", "Internal variable", "Internal programmable"
	};
	unsigned int i, chcfg, j, k, N, termt, subtype;
	char chnames[128], term[128], termts[128];

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      AudioControl Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);

	/*
	 * This is an utter mess - UAC2 defines some bDescriptorSubtype differently, so we have to do some ugly remapping here:
	 *
	 * bDescriptorSubtype		UAC1			UAC2
	 * ------------------------------------------------------------------------
	 * 0x07				PROCESSING_UNIT		EFFECT_UNIT
	 * 0x08				EXTENSION_UNIT		PROCESSING_UNIT
	 * 0x09				-			EXTENSION_UNIT
	 *
	 */

	if (protocol == USB_AUDIO_CLASS_2)
		switch(buf[2]) {
		case 0x07: subtype = 0xf0; break; /* effect unit */
		case 0x08: subtype = 0x07; break; /* processing unit */
		case 0x09: subtype = 0x08; break; /* extension unit */
		default: subtype = buf[2]; break; /* everything else is identical */
		}
	else
		subtype = buf[2];

	switch (subtype) {
	case 0x01:  /* HEADER */
		printf("(HEADER)\n");
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (buf[0] < 8+buf[7])
				printf("      Warning: Descriptor too short\n");
			printf("        bcdADC              %2x.%02x\n"
			       "        wTotalLength        %5u\n"
			       "        bInCollection       %5u\n",
			       buf[4], buf[3], buf[5] | (buf[6] << 8), buf[7]);
			for (i = 0; i < buf[7]; i++)
				printf("        baInterfaceNr(%2u)   %5u\n", i, buf[8+i]);
			xml_junk(buf, "        ", 8+buf[7]);
			break;
		case USB_AUDIO_CLASS_2:
			if (buf[0] < 9)
				printf("      Warning: Descriptor too short\n");
			printf("        bcdADC              %2x.%02x\n"
			       "        bCategory           %5u\n"
			       "        wTotalLength        %5u\n"
			       "        bmControl            0x%02x\n",
			       buf[4], buf[3], buf[5], buf[6] | (buf[7] << 8), buf[8]);
			xml_audio_bmcontrols("          ", buf[8], uac2_interface_header_bmcontrols, protocol);
			break;
		}
		break;

	case 0x02:  /* INPUT_TERMINAL */
		printf("(INPUT_TERMINAL)\n");
		termt = buf[4] | (buf[5] << 8);
		get_audioterminal_string(termts, sizeof(termts), termt);

		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			get_string(dev, chnames, sizeof(chnames), buf[10]);
			get_string(dev, term, sizeof(term), buf[11]);
			if (buf[0] < 12)
				printf("      Warning: Descriptor too short\n");
			chcfg = buf[8] | (buf[9] << 8);
			printf("        bTerminalID         %5u\n"
			       "        wTerminalType      0x%04x %s\n"
			       "        bAssocTerminal      %5u\n"
			       "        bNrChannels         %5u\n"
			       "        wChannelConfig     0x%04x\n",
			       buf[3], termt, termts, buf[6], buf[7], chcfg);
			for (i = 0; i < 12; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig[i]);
			printf("        iChannelNames       %5u %s\n"
			       "        iTerminal           %5u %s\n",
			       buf[10], chnames, buf[11], term);
			xml_junk(buf, "        ", 12);
			break;
		case USB_AUDIO_CLASS_2:
			get_string(dev, chnames, sizeof(chnames), buf[13]);
			get_string(dev, term, sizeof(term), buf[16]);
			if (buf[0] < 17)
				printf("      Warning: Descriptor too short\n");
			chcfg = buf[9] | (buf[10] << 8) | (buf[11] << 16) | (buf[12] << 24);
			printf("        bTerminalID         %5u\n"
			       "        wTerminalType      0x%04x %s\n"
			       "        bAssocTerminal      %5u\n"
			       "        bCSourceID          %5d\n"
			       "        bNrChannels         %5u\n"
			       "        bmChannelConfig   0x%08x\n",
			       buf[3], termt, termts, buf[6], buf[7], buf[8], chcfg);
			for (i = 0; i < 26; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig_uac2[i]);
			printf("        bmControls    0x%04x\n", buf[14] | (buf[15] << 8));
			xml_audio_bmcontrols("          ", buf[14] | (buf[15] << 8), uac2_input_term_bmcontrols, protocol);
			printf("        iChannelNames       %5u %s\n"
			       "        iTerminal           %5u %s\n",
			       buf[13], chnames, buf[16], term);
			xml_junk(buf, "        ", 17);
			break;
		} /* switch (protocol) */

		break;

	case 0x03:  /* OUTPUT_TERMINAL */
		printf("(OUTPUT_TERMINAL)\n");
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			get_string(dev, term, sizeof(term), buf[8]);
			termt = buf[4] | (buf[5] << 8);
			get_audioterminal_string(termts, sizeof(termts), termt);
			if (buf[0] < 9)
				printf("      Warning: Descriptor too short\n");
			printf("        bTerminalID         %5u\n"
			       "        wTerminalType      0x%04x %s\n"
			       "        bAssocTerminal      %5u\n"
			       "        bSourceID           %5u\n"
			       "        iTerminal           %5u %s\n",
			       buf[3], termt, termts, buf[6], buf[7], buf[8], term);
			xml_junk(buf, "        ", 9);
			break;
		case USB_AUDIO_CLASS_2:
			get_string(dev, term, sizeof(term), buf[11]);
			termt = buf[4] | (buf[5] << 8);
			get_audioterminal_string(termts, sizeof(termts), termt);
			if (buf[0] < 12)
				printf("      Warning: Descriptor too short\n");
			printf("        bTerminalID         %5u\n"
			       "        wTerminalType      0x%04x %s\n"
			       "        bAssocTerminal      %5u\n"
			       "        bSourceID           %5u\n"
			       "        bCSourceID          %5u\n"
			       "        bmControls         0x%04x\n",
			       buf[3], termt, termts, buf[6], buf[7], buf[8], buf[9] | (buf[10] << 8));
			xml_audio_bmcontrols("          ", buf[9] | (buf[10] << 8), uac2_output_term_bmcontrols, protocol);
			printf("        iTerminal           %5u %s\n", buf[11], term);
			xml_junk(buf, "        ", 12);
			break;
		} /* switch (protocol) */

		break;

	case 0x04:  /* MIXER_UNIT */
		printf("(MIXER_UNIT)\n");

		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			j = buf[4];
			k = buf[j+5];
			if (j == 0 || k == 0) {
				printf("      Warning: mixer with %5u input and %5u output channels.\n", j, k);
				N = 0;
			} else {
				N = 1+(j*k-1)/8;
			}
			get_string(dev, chnames, sizeof(chnames), buf[8+j]);
			get_string(dev, term, sizeof(term), buf[9+j+N]);
			if (buf[0] < 10+j+N)
				printf("      Warning: Descriptor too short\n");
			chcfg = buf[6+j] | (buf[7+j] << 8);
			printf("        bUnitID             %5u\n"
			       "        bNrInPins           %5u\n",
			       buf[3], buf[4]);
			for (i = 0; i < j; i++)
				printf("        baSourceID(%2u)      %5u\n", i, buf[5+i]);
			printf("        bNrChannels         %5u\n"
			       "        wChannelConfig     0x%04x\n",
			       buf[5+j], chcfg);
			for (i = 0; i < 12; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig[i]);
			printf("        iChannelNames       %5u %s\n",
			       buf[8+j], chnames);
			for (i = 0; i < N; i++)
				printf("        bmControls         0x%02x\n", buf[9+j+i]);
			printf("        iMixer              %5u %s\n", buf[9+j+N], term);
			xml_junk(buf, "        ", 10+j+N);
			break;

		case USB_AUDIO_CLASS_2:
			j = buf[4];
			k = buf[0] - 13 - j;
			get_string(dev, chnames, sizeof(chnames), buf[10+j]);
			get_string(dev, term, sizeof(term), buf[12+j+k]);
			chcfg =  buf[6+j] | (buf[7+j] << 8) | (buf[8+j] << 16) | (buf[9+j] << 24);

			printf("        bUnitID             %5u\n"
			       "        bNrPins             %5u\n",
			       buf[3], buf[4]);
			for (i = 0; i < j; i++)
				printf("        baSourceID(%2u)      %5u\n", i, buf[5+i]);
			printf("        bNrChannels         %5u\n"
			       "        bmChannelConfig    0x%08x\n", buf[5+j], chcfg);
			for (i = 0; i < 26; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig_uac2[i]);
			printf("        iChannelNames       %5u %s\n", buf[10+j], chnames);

			N = 0;
			for (i = 0; i < k; i++)
				N |= buf[11+j+i] << (i * 8);

			xml_bytes(buf+11+j, k);

			printf("        bmControls         %02x\n", buf[11+j+k]);
			xml_audio_bmcontrols("          ", buf[11+j+k], uac2_mixer_unit_bmcontrols, protocol);

			printf("        iMixer             %5u %s\n", buf[12+j+k], term);
			xml_junk(buf, "        ", 13+j+k);
			break;
		} /* switch (protocol) */
		break;

	case 0x05:  /* SELECTOR_UNIT */
		printf("(SELECTOR_UNIT)\n");
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (buf[0] < 6+buf[4])
				printf("      Warning: Descriptor too short\n");
			get_string(dev, term, sizeof(term), buf[5+buf[4]]);

			printf("        bUnitID             %5u\n"
			       "        bNrInPins           %5u\n",
			       buf[3], buf[4]);
			for (i = 0; i < buf[4]; i++)
				printf("        baSource(%2u)        %5u\n", i, buf[5+i]);
			printf("        iSelector           %5u %s\n",
			       buf[5+buf[4]], term);
			xml_junk(buf, "        ", 6+buf[4]);
			break;
		case USB_AUDIO_CLASS_2:
			if (buf[0] < 7+buf[4])
				printf("      Warning: Descriptor too short\n");
			get_string(dev, term, sizeof(term), buf[6+buf[4]]);

			printf("        bUnitID             %5u\n"
			       "        bNrInPins           %5u\n",
			       buf[3], buf[4]);
			for (i = 0; i < buf[4]; i++)
				printf("        baSource(%2u)        %5u\n", i, buf[5+i]);
			printf("        bmControls           0x%02x\n", buf[5+buf[4]]);
			xml_audio_bmcontrols("          ", buf[5+buf[4]], uac2_selector_bmcontrols, protocol);
			printf("        iSelector           %5u %s\n",
			       buf[6+buf[4]], term);
			xml_junk(buf, "        ", 7+buf[4]);
			break;
		} /* switch (protocol) */

		break;

	case 0x06:  /* FEATURE_UNIT */
		printf("(FEATURE_UNIT)\n");

		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			j = buf[5];
			if (!j)
				j = 1;
			k = (buf[0] - 7) / j;
			if (buf[0] < 7+buf[5]*k)
				printf("      Warning: Descriptor too short\n");
			get_string(dev, term, sizeof(term), buf[6+buf[5]*k]);
			printf("        bUnitID             %5u\n"
			       "        bSourceID           %5u\n"
			       "        bControlSize        %5u\n",
			       buf[3], buf[4], buf[5]);
			for (i = 0; i < k; i++) {
				chcfg = buf[6+buf[5]*i];
				if (buf[5] > 1)
					chcfg |= (buf[7+buf[5]*i] << 8);
				for (j = 0; j < buf[5]; j++)
					printf("        bmaControls(%2u)      0x%02x\n", i, buf[6+buf[5]*i+j]);

				xml_audio_bmcontrols("          ", chcfg, uac_fu_bmcontrols, protocol);
			}
			printf("        iFeature            %5u %s\n", buf[6+buf[5]*k], term);
			xml_junk(buf, "        ", 7+buf[5]*k);
			break;
		case USB_AUDIO_CLASS_2:
			if (buf[0] < 10)
				printf("      Warning: Descriptor too short\n");
			k = (buf[0] - 6) / 4;
			printf("        bUnitID             %5u\n"
			       "        bSourceID           %5u\n",
			       buf[3], buf[4]);
			for (i = 0; i < k; i++) {
				chcfg = buf[5+(4*i)] |
					buf[6+(4*i)] << 8 |
					buf[7+(4*i)] << 16 |
					buf[8+(4*i)] << 24;
				printf("        bmaControls(%2u)      0x%08x\n", i, chcfg);
				xml_audio_bmcontrols("          ", chcfg, uac_fu_bmcontrols, protocol);
			}
			get_string(dev, term, sizeof(term), buf[5+(k*4)]);
			printf("        iFeature            %5u %s\n", buf[5+(k*4)], term);
			xml_junk(buf, "        ", 6+(k*4));
			break;
		} /* switch (protocol) */

		break;

	case 0x07:  /* PROCESSING_UNIT */
		printf("(PROCESSING_UNIT)\n");

		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			j = buf[6];
			k = buf[11+j];
			get_string(dev, chnames, sizeof(chnames), buf[10+j]);
			get_string(dev, term, sizeof(term), buf[12+j+k]);
			chcfg = buf[8+j] | (buf[9+j] << 8);
			if (buf[0] < 13+j+k)
				printf("      Warning: Descriptor too short\n");
			printf("        bUnitID             %5u\n"
			       "        wProcessType        %5u\n"
			       "        bNrPins             %5u\n",
			       buf[3], buf[4] | (buf[5] << 8), buf[6]);
			for (i = 0; i < j; i++)
				printf("        baSourceID(%2u)      %5u\n", i, buf[7+i]);
			printf("        bNrChannels         %5u\n"
			       "        wChannelConfig     0x%04x\n", buf[7+j], chcfg);
			for (i = 0; i < 12; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig[i]);
			printf("        iChannelNames       %5u %s\n"
			       "        bControlSize        %5u\n", buf[10+j], chnames, buf[11+j]);
			for (i = 0; i < k; i++)
				printf("        bmControls(%2u)       0x%02x\n", i, buf[12+j+i]);
			if (buf[12+j] & 1)
				printf("          Enable Processing\n");
			printf("        iProcessing         %5u %s\n"
			       "        Process-Specific    ", buf[12+j+k], term);
			xml_bytes(buf+(13+j+k), buf[0]-(13+j+k));
			break;
		case USB_AUDIO_CLASS_2:
			j = buf[6];
			k = buf[0] - 17 - j;
			get_string(dev, chnames, sizeof(chnames), buf[12+j]);
			get_string(dev, term, sizeof(term), buf[15+j+k]);
			chcfg =  buf[8+j] |
				(buf[9+j] << 8) |
				(buf[10+j] << 16) |
				(buf[11+j] << 24);

			printf("        bUnitID             %5u\n"
			       "        wProcessType        %5u\n"
			       "        bNrPins             %5u\n",
			       buf[3], buf[4] | (buf[5] << 8), buf[6]);
			for (i = 0; i < j; i++)
				printf("        baSourceID(%2u)      %5u\n", i, buf[5+i]);
			printf("        bNrChannels         %5u\n"
			       "        bmChannelConfig    0x%08x\n", buf[7+j], chcfg);
			for (i = 0; i < 26; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig_uac2[i]);
			printf("        iChannelNames       %5u %s\n"
			       "        bmControls        0x%04x\n", buf[12+j], chnames, buf[13+j] | (buf[14+j] << 8));
			if (buf[12+j] & 1)
				printf("          Enable Processing\n");
			printf("        iProcessing         %5u %s\n"
			       "        Process-Specific    ", buf[15+j], term);
			xml_bytes(buf+(16+j), k);
			break;
		} /* switch (protocol) */

		break;

	case 0x08:  /* EXTENSION_UNIT */
		printf("(EXTENSION_UNIT)\n");

		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			j = buf[6];
			k = buf[11+j];
			get_string(dev, chnames, sizeof(chnames), buf[10+j]);
			get_string(dev, term, sizeof(term), buf[12+j+k]);
			chcfg = buf[8+j] | (buf[9+j] << 8);
			if (buf[0] < 13+j+k)
				printf("      Warning: Descriptor too short\n");
			printf("        bUnitID             %5u\n"
			       "        wExtensionCode      %5u\n"
			       "        bNrPins             %5u\n",
			       buf[3], buf[4] | (buf[5] << 8), buf[6]);
			for (i = 0; i < j; i++)
				printf("        baSourceID(%2u)      %5u\n", i, buf[7+i]);
			printf("        bNrChannels         %5u\n"
			       "        wChannelConfig      %5u\n", buf[7+j], chcfg);
			for (i = 0; i < 12; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig[i]);
			printf("        iChannelNames       %5u %s\n"
			       "        bControlSize        %5u\n", buf[10+j], chnames, buf[11+j]);
			for (i = 0; i < k; i++)
				printf("        bmControls(%2u)       0x%02x\n", i, buf[12+j+i]);
			if (buf[12+j] & 1)
				printf("          Enable Processing\n");
			printf("        iExtension          %5u %s\n",
			       buf[12+j+k], term);
			xml_junk(buf, "        ", 13+j+k);
			break;
		case USB_AUDIO_CLASS_2:
			j = buf[6];
			get_string(dev, chnames, sizeof(chnames), buf[13+j]);
			get_string(dev, term, sizeof(term), buf[15+j]);
			chcfg = buf[9+j] | (buf[10+j] << 8) | (buf[11+j] << 16) | (buf[12+j] << 24);
			if (buf[0] < 16+j)
				printf("      Warning: Descriptor too short\n");
			printf("        bUnitID             %5u\n"
			       "        wExtensionCode      %5u\n"
			       "        bNrPins             %5u\n",
			       buf[3], buf[4] | (buf[5] << 8), buf[6]);
			for (i = 0; i < j; i++)
				printf("        baSourceID(%2u)      %5u\n", i, buf[7+i]);
			printf("        bNrChannels         %5u\n"
			       "        wChannelConfig      %5u\n", buf[7+j], chcfg);
			for (i = 0; i < 26; i++)
				if ((chcfg >> i) & 1)
					printf("          %s\n", chconfig[i]);
			printf("        iChannelNames       %5u %s\n"
			       "        bmControls        0x%02x\n", buf[13+j], chnames, buf[14+j]);
			xml_audio_bmcontrols("          ", buf[14+j], uac2_extension_unit_bmcontrols, protocol);

			printf("        iExtension          %5u %s\n",
			       buf[15+j+k], term);
			xml_junk(buf, "        ", 16+j);
			break;
		} /* switch (protocol) */

		break;

	case 0x0a:  /* CLOCK_SOURCE */
		printf ("(CLOCK_SOURCE)\n");
		if (protocol != USB_AUDIO_CLASS_2)
			printf("      Warning: CLOCK_SOURCE descriptors are illegal for UAC1\n");

		if (buf[0] < 8)
			printf("      Warning: Descriptor too short\n");

		printf("        bClockID            %5u\n"
		       "        bmAttributes         0x%02x %s Clock %s\n",
		       buf[3], buf[4], clock_source_attrs[buf[4] & 3],
		       (buf[4] & 4) ? "(synced to SOF)" : "");

		printf("        bmControls           0x%02x\n", buf[5]);
		xml_audio_bmcontrols("          ", buf[5], uac2_clock_source_bmcontrols, protocol);

		get_string(dev, term, sizeof(term), buf[7]);
		printf("        bAssocTerminal      %5u\n", buf[6]);
		printf("        iClockSource        %5u %s\n", buf[7], term);
		xml_junk(buf, "        ", 8);
		break;

	case 0x0b:  /* CLOCK_SELECTOR */
		printf("(CLOCK_SELECTOR)\n");
		if (protocol != USB_AUDIO_CLASS_2)
			printf("      Warning: CLOCK_SELECTOR descriptors are illegal for UAC1\n");

		if (buf[0] < 7+buf[4])
			printf("      Warning: Descriptor too short\n");
		get_string(dev, term, sizeof(term), buf[6+buf[4]]);

		printf("        bUnitID             %5u\n"
		       "        bNrInPins           %5u\n",
		       buf[3], buf[4]);
		for (i = 0; i < buf[4]; i++)
			printf("        baCSourceID(%2u)     %5u\n", i, buf[5+i]);
		printf("        bmControls           0x%02x\n", buf[5+buf[4]]);
		xml_audio_bmcontrols("          ", buf[5+buf[4]], uac2_clock_selector_bmcontrols, protocol);

		printf("        iClockSelector      %5u %s\n",
		       buf[6+buf[4]], term);
		xml_junk(buf, "        ", 7+buf[4]);
		break;

	case 0x0c:  /* CLOCK_MULTIPLIER */
		printf("(CLOCK_MULTIPLIER)\n");
		if (protocol != USB_AUDIO_CLASS_2)
			printf("      Warning: CLOCK_MULTIPLIER descriptors are illegal for UAC1\n");

		if (buf[0] < 7)
			printf("      Warning: Descriptor too short\n");

		printf("        bClockID            %5u\n"
		       "        bCSourceID          %5u\n",
		       buf[3], buf[4]);

		printf("        bmControls           0x%02x\n", buf[5]);
		xml_audio_bmcontrols("          ", buf[5], uac2_clock_multiplier_bmcontrols, protocol);

		get_string(dev, term, sizeof(term), buf[6]);
		printf("        iClockMultiplier    %5u %s\n", buf[6], term);
		xml_junk(buf, "        ", 7);
		break;

	case 0x0d:  /* SAMPLE_RATE_CONVERTER_UNIT */
		printf("(SAMPLE_RATE_CONVERTER_UNIT)\n");
		if (protocol != USB_AUDIO_CLASS_2)
			printf("      Warning: SAMPLE_RATE_CONVERTER_UNIT descriptors are illegal for UAC1\n");

		if (buf[0] < 8)
			printf("      Warning: Descriptor too short\n");

		get_string(dev, term, sizeof(term), buf[7]);
		printf("        bUnitID             %5u\n"
		       "        bSourceID           %5u\n"
		       "        bCSourceInID        %5u\n"
		       "        bCSourceOutID       %5u\n"
		       "        iSRC                %5u %s\n",
		       buf[3], buf[4], buf[5], buf[6], buf[7], term);
		xml_junk(buf, "        ", 8);
		break;

	case 0xf0:  /* EFFECT_UNIT - the real value is 0x07, see above for the reason for remapping */
		printf("(EFFECT_UNIT)\n");

		if (buf[0] < 16)
			printf("      Warning: Descriptor too short\n");
		k = (buf[0] - 16) / 4;
		get_string(dev, term, sizeof(term), buf[15+(k*4)]);
		printf("        bUnitID             %5u\n"
		       "        wEffectType         %5u\n"
		       "        bSourceID           %5u\n",
		       buf[3], buf[4] | (buf[5] << 8), buf[6]);
		for (i = 0; i < k; i++) {
			chcfg = buf[7+(4*i)] |
				buf[8+(4*i)] << 8 |
				buf[9+(4*i)] << 16 |
				buf[10+(4*i)] << 24;
			printf("        bmaControls(%2u)      0x%08x\n", i, chcfg);
			/* TODO: parse effect-specific controls */
		}
		printf("        iEffect             %5u %s\n", buf[15+(k*4)], term);
		xml_junk(buf, "        ", 16+(k*4));
		break;

	default:
		printf("(unknown)\n"
		       "        Invalid desc subtype:");
		xml_bytes(buf+3, buf[0]-3);
		break;
	}
}


void xml_audiostreaming_interface(libusb_device_handle *dev, const unsigned char *buf, int protocol)
{
	static const char * const fmtItag[] = {
		"TYPE_I_UNDEFINED", "PCM", "PCM8", "IEEE_FLOAT", "ALAW", "MULAW" };
	static const char * const fmtIItag[] = { "TYPE_II_UNDEFINED", "MPEG", "AC-3" };
	static const char * const fmtIIItag[] = {
		"TYPE_III_UNDEFINED", "IEC1937_AC-3", "IEC1937_MPEG-1_Layer1",
		"IEC1937_MPEG-Layer2/3/NOEXT", "IEC1937_MPEG-2_EXT",
		"IEC1937_MPEG-2_Layer1_LS", "IEC1937_MPEG-2_Layer2/3_LS" };
	unsigned int i, j, fmttag;
	const char *fmtptr = "undefined";
	char name[128];

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      AudioStreaming Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01: /* AS_GENERAL */
		printf("(AS_GENERAL)\n");

		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (buf[0] < 7)
				printf("      Warning: Descriptor too short\n");
			fmttag = buf[5] | (buf[6] << 8);
			if (fmttag <= 5)
				fmtptr = fmtItag[fmttag];
			else if (fmttag >= 0x1000 && fmttag <= 0x1002)
				fmtptr = fmtIItag[fmttag & 0xfff];
			else if (fmttag >= 0x2000 && fmttag <= 0x2006)
				fmtptr = fmtIIItag[fmttag & 0xfff];
			printf("        bTerminalLink       %5u\n"
			       "        bDelay              %5u frames\n"
			       "        wFormatTag          %5u %s\n",
			       buf[3], buf[4], fmttag, fmtptr);
			xml_junk(buf, "        ", 7);
			break;
		case USB_AUDIO_CLASS_2:
			if (buf[0] < 16)
				printf("      Warning: Descriptor too short\n");
			printf("        bTerminalLink       %5u\n"
			       "        bmControls           0x%02x\n",
			       buf[3], buf[4]);
			xml_audio_bmcontrols("          ", buf[4], uac2_as_interface_bmcontrols, protocol);

			printf("        bFormatType         %5u\n", buf[5]);
			fmttag = buf[6] | (buf[7] << 8) | (buf[8] << 16) | (buf[9] << 24);
			printf("        bmFormats           %5u\n", fmttag);
			for (i=0; i<5; i++)
				if ((fmttag >> i) & 1)
					printf("          %s\n", fmtItag[i+1]);

			j = buf[11] | (buf[12] << 8) | (buf[13] << 16) | (buf[14] << 24);
			printf("        bNrChannels         %5u\n"
			       "        bmChannelConfig   0x%08x\n",
			       buf[10], j);
			for (i = 0; i < 26; i++)
				if ((j >> i) & 1)
					printf("          %s\n", chconfig_uac2[i]);

			get_string(dev, name, sizeof(name), buf[15]);
			printf("        iChannelNames       %5u %s\n", buf[15], name);
			xml_junk(buf, "        ", 16);
			break;
		} /* switch (protocol) */

		break;

	case 0x02: /* FORMAT_TYPE */
		printf("(FORMAT_TYPE)\n");
		switch (protocol) {
		case USB_AUDIO_CLASS_1:
			if (buf[0] < 8)
				printf("      Warning: Descriptor too short\n");
			printf("        bFormatType         %5u ", buf[3]);
			switch (buf[3]) {
			case 0x01: /* FORMAT_TYPE_I */
				printf("(FORMAT_TYPE_I)\n");
				j = buf[7] ? (buf[7]*3+8) : 14;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        bNrChannels         %5u\n"
				       "        bSubframeSize       %5u\n"
				       "        bBitResolution      %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4], buf[5], buf[6], buf[7], buf[7] ? "Discrete" : "Continuous");
				if (!buf[7])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[8] | (buf[9] << 8) | (buf[10] << 16), buf[11] | (buf[12] << 8) | (buf[13] << 16));
				else
					for (i = 0; i < buf[7]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[8+3*i] | (buf[9+3*i] << 8) | (buf[10+3*i] << 16));
				xml_junk(buf, "        ", j);
				break;

			case 0x02: /* FORMAT_TYPE_II */
				printf("(FORMAT_TYPE_II)\n");
				j = buf[8] ? (buf[7]*3+9) : 15;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        wMaxBitRate         %5u\n"
				       "        wSamplesPerFrame    %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4] | (buf[5] << 8), buf[6] | (buf[7] << 8), buf[8], buf[8] ? "Discrete" : "Continuous");
				if (!buf[8])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[9] | (buf[10] << 8) | (buf[11] << 16), buf[12] | (buf[13] << 8) | (buf[14] << 16));
				else
					for (i = 0; i < buf[8]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[9+3*i] | (buf[10+3*i] << 8) | (buf[11+3*i] << 16));
				xml_junk(buf, "        ", j);
				break;

			case 0x03: /* FORMAT_TYPE_III */
				printf("(FORMAT_TYPE_III)\n");
				j = buf[7] ? (buf[7]*3+8) : 14;
				if (buf[0] < j)
					printf("      Warning: Descriptor too short\n");
				printf("        bNrChannels         %5u\n"
				       "        bSubframeSize       %5u\n"
				       "        bBitResolution      %5u\n"
				       "        bSamFreqType        %5u %s\n",
				       buf[4], buf[5], buf[6], buf[7], buf[7] ? "Discrete" : "Continuous");
				if (!buf[7])
					printf("        tLowerSamFreq     %7u\n"
					       "        tUpperSamFreq     %7u\n",
					       buf[8] | (buf[9] << 8) | (buf[10] << 16), buf[11] | (buf[12] << 8) | (buf[13] << 16));
				else
					for (i = 0; i < buf[7]; i++)
						printf("        tSamFreq[%2u]      %7u\n", i,
						       buf[8+3*i] | (buf[9+3*i] << 8) | (buf[10+3*i] << 16));
				xml_junk(buf, "        ", j);
				break;

			default:
				printf("(unknown)\n"
				       "        Invalid desc format type:");
				xml_bytes(buf+4, buf[0]-4);
			}

			break;

		case USB_AUDIO_CLASS_2:
			printf("        bFormatType         %5u ", buf[3]);
			switch (buf[3]) {
			case 0x01: /* FORMAT_TYPE_I */
				printf("(FORMAT_TYPE_I)\n");
				if (buf[0] < 6)
					printf("      Warning: Descriptor too short\n");
				printf("        bSubslotSize        %5u\n"
				       "        bBitResolution      %5u\n",
				       buf[4], buf[5]);
				xml_junk(buf, "        ", 6);
				break;

			case 0x02: /* FORMAT_TYPE_II */
				printf("(FORMAT_TYPE_II)\n");
				if (buf[0] < 8)
					printf("      Warning: Descriptor too short\n");
				printf("        wMaxBitRate         %5u\n"
				       "        wSlotsPerFrame      %5u\n",
				       buf[4] | (buf[5] << 8),
				       buf[6] | (buf[7] << 8));
				xml_junk(buf, "        ", 8);
				break;

			case 0x03: /* FORMAT_TYPE_III */
				printf("(FORMAT_TYPE_III)\n");
				if (buf[0] < 6)
					printf("      Warning: Descriptor too short\n");
				printf("        bSubslotSize        %5u\n"
				       "        bBitResolution      %5u\n",
				       buf[4], buf[5]);
				xml_junk(buf, "        ", 6);
				break;

			case 0x04: /* FORMAT_TYPE_IV */
				printf("(FORMAT_TYPE_IV)\n");
				if (buf[0] < 4)
					printf("      Warning: Descriptor too short\n");
				printf("        bFormatType         %5u\n", buf[3]);
				xml_junk(buf, "        ", 4);
				break;

			default:
				printf("(unknown)\n"
				       "        Invalid desc format type:");
				xml_bytes(buf+4, buf[0]-4);
			}

			break;
		} /* switch (protocol) */

		break;

	case 0x03: /* FORMAT_SPECIFIC */
		printf("(FORMAT_SPECIFIC)\n");
		if (buf[0] < 5)
			printf("      Warning: Descriptor too short\n");
		fmttag = buf[3] | (buf[4] << 8);
		if (fmttag <= 5)
			fmtptr = fmtItag[fmttag];
		else if (fmttag >= 0x1000 && fmttag <= 0x1002)
			fmtptr = fmtIItag[fmttag & 0xfff];
		else if (fmttag >= 0x2000 && fmttag <= 0x2006)
			fmtptr = fmtIIItag[fmttag & 0xfff];
		printf("        wFormatTag          %5u %s\n", fmttag, fmtptr);
		switch (fmttag) {
		case 0x1001: /* MPEG */
			if (buf[0] < 8)
				printf("      Warning: Descriptor too short\n");
			printf("        bmMPEGCapabilities 0x%04x\n",
			       buf[5] | (buf[6] << 8));
			if (buf[5] & 0x01)
				printf("          Layer I\n");
			if (buf[5] & 0x02)
				printf("          Layer II\n");
			if (buf[5] & 0x04)
				printf("          Layer III\n");
			if (buf[5] & 0x08)
				printf("          MPEG-1 only\n");
			if (buf[5] & 0x10)
				printf("          MPEG-1 dual-channel\n");
			if (buf[5] & 0x20)
				printf("          MPEG-2 second stereo\n");
			if (buf[5] & 0x40)
				printf("          MPEG-2 7.1 channel augmentation\n");
			if (buf[5] & 0x80)
				printf("          Adaptive multi-channel prediction\n");
			printf("          MPEG-2 multilingual support: ");
			switch (buf[6] & 3) {
			case 0:
				printf("Not supported\n");
				break;

			case 1:
				printf("Supported at Fs\n");
				break;

			case 2:
				printf("Reserved\n");
				break;

			default:
				printf("Supported at Fs and 1/2Fs\n");
				break;
			}
			printf("        bmMPEGFeatures       0x%02x\n", buf[7]);
			printf("          Internal Dynamic Range Control: ");
			switch ((buf[7] << 4) & 3) {
			case 0:
				printf("not supported\n");
				break;

			case 1:
				printf("supported but not scalable\n");
				break;

			case 2:
				printf("scalable, common boost and cut scaling value\n");
				break;

			default:
				printf("scalable, separate boost and cut scaling value\n");
				break;
			}
			xml_junk(buf, "        ", 8);
			break;

		case 0x1002: /* AC-3 */
			if (buf[0] < 10)
				printf("      Warning: Descriptor too short\n");
			printf("        bmBSID         0x%08x\n"
			       "        bmAC3Features        0x%02x\n",
			       buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24), buf[9]);
			if (buf[9] & 0x01)
				printf("          RF mode\n");
			if (buf[9] & 0x02)
				printf("          Line mode\n");
			if (buf[9] & 0x04)
				printf("          Custom0 mode\n");
			if (buf[9] & 0x08)
				printf("          Custom1 mode\n");
			printf("          Internal Dynamic Range Control: ");
			switch ((buf[9] >> 4) & 3) {
			case 0:
				printf("not supported\n");
				break;

			case 1:
				printf("supported but not scalable\n");
				break;

			case 2:
				printf("scalable, common boost and cut scaling value\n");
				break;

			default:
				printf("scalable, separate boost and cut scaling value\n");
				break;
			}
			xml_junk(buf, "        ", 8);
			break;

		default:
			printf("(unknown)\n"
			       "        Invalid desc format type:");
			xml_bytes(buf+4, buf[0]-4);
		}
		break;

	default:
		printf("        Invalid desc subtype:");
		xml_bytes(buf+3, buf[0]-3);
		break;
	}
}

void xml_midistreaming_interface(libusb_device_handle *dev, const unsigned char *buf)
{
	static const char * const jacktypes[] = {"Undefined", "Embedded", "External"};
	char jackstr[128];
	unsigned int j, tlength, capssize;
	unsigned long caps;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      MIDIStreaming Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01:
		printf("(HEADER)\n");
		if (buf[0] < 7)
			printf("      Warning: Descriptor too short\n");
		tlength = buf[5] | (buf[6] << 8);
		printf("        bcdADC              %2x.%02x\n"
		       "        wTotalLength        %5u\n",
		       buf[4], buf[3], tlength);
		xml_junk(buf, "        ", 7);
		break;

	case 0x02:
		printf("(MIDI_IN_JACK)\n");
		if (buf[0] < 6)
			printf("      Warning: Descriptor too short\n");
		get_string(dev, jackstr, sizeof(jackstr), buf[5]);
		printf("        bJackType           %5u %s\n"
		       "        bJackID             %5u\n"
		       "        iJack               %5u %s\n",
		       buf[3], buf[3] < 3 ? jacktypes[buf[3]] : "Invalid",
		       buf[4], buf[5], jackstr);
		xml_junk(buf, "        ", 6);
		break;

	case 0x03:
		printf("(MIDI_OUT_JACK)\n");
		if (buf[0] < 9)
			printf("      Warning: Descriptor too short\n");
		printf("        bJackType           %5u %s\n"
		       "        bJackID             %5u\n"
		       "        bNrInputPins        %5u\n",
		       buf[3], buf[3] < 3 ? jacktypes[buf[3]] : "Invalid",
		       buf[4], buf[5]);
		for (j = 0; j < buf[5]; j++) {
			printf("        baSourceID(%2u)      %5u\n"
			       "        BaSourcePin(%2u)     %5u\n",
			       j, buf[2*j+6], j, buf[2*j+7]);
		}
		j = 6+buf[5]*2; /* midi10.pdf says, incorrectly: 5+2*p */
		get_string(dev, jackstr, sizeof(jackstr), buf[j]);
		printf("        iJack               %5u %s\n",
		       buf[j], jackstr);
		xml_junk(buf, "        ", j+1);
		break;

	case 0x04:
		printf("(ELEMENT)\n");
		if (buf[0] < 12)
			printf("      Warning: Descriptor too short\n");
		printf("        bElementID          %5u\n"
		       "        bNrInputPins        %5u\n",
		       buf[3], buf[4]);
		for (j = 0; j < buf[4]; j++) {
			printf("        baSourceID(%2u)      %5u\n"
			       "        BaSourcePin(%2u)     %5u\n",
			       j, buf[2*j+5], j, buf[2*j+6]);
		}
		j = 5+buf[4]*2;
		printf("        bNrOutputPins       %5u\n"
		       "        bInTerminalLink     %5u\n"
		       "        bOutTerminalLink    %5u\n"
		       "        bElCapsSize         %5u\n",
		       buf[j], buf[j+1], buf[j+2], buf[j+3]);
		capssize = buf[j+3];
		caps = 0;
		for (j = 0; j < capssize; j++)
			caps |= (buf[j+9+buf[4]*2] << (8*j));
		printf("        bmElementCaps  0x%08lx\n", caps);
		if (caps & 0x01)
			printf("          Undefined\n");
		if (caps & 0x02)
			printf("          MIDI Clock\n");
		if (caps & 0x04)
			printf("          MTC (MIDI Time Code)\n");
		if (caps & 0x08)
			printf("          MMC (MIDI Machine Control)\n");
		if (caps & 0x10)
			printf("          GM1 (General MIDI v.1)\n");
		if (caps & 0x20)
			printf("          GM2 (General MIDI v.2)\n");
		if (caps & 0x40)
			printf("          GS MIDI Extension\n");
		if (caps & 0x80)
			printf("          XG MIDI Extension\n");
		if (caps & 0x100)
			printf("          EFX\n");
		if (caps & 0x200)
			printf("          MIDI Patch Bay\n");
		if (caps & 0x400)
			printf("          DLS1 (Downloadable Sounds Level 1)\n");
		if (caps & 0x800)
			printf("          DLS2 (Downloadable Sounds Level 2)\n");
		j = 9+2*buf[4]+capssize;
		get_string(dev, jackstr, sizeof(jackstr), buf[j]);
		printf("        iElement            %5u %s\n", buf[j], jackstr);
		xml_junk(buf, "        ", j+1);
		break;

	default:
		printf("\n        Invalid desc subtype: ");
		xml_bytes(buf+3, buf[0]-3);
		break;
	}
}

/*
 * Video Class descriptor dump
 */

void xml_videocontrol_interface(libusb_device_handle *dev, const unsigned char *buf)
{
	static const char * const ctrlnames[] = {
		"Brightness", "Contrast", "Hue", "Saturation", "Sharpness", "Gamma",
		"White Balance Temperature", "White Balance Component", "Backlight Compensation",
		"Gain", "Power Line Frequency", "Hue, Auto", "White Balance Temperature, Auto",
		"White Balance Component, Auto", "Digital Multiplier", "Digital Multiplier Limit",
		"Analog Video Standard", "Analog Video Lock Status"
	};
	static const char * const camctrlnames[] = {
		"Scanning Mode", "Auto-Exposure Mode", "Auto-Exposure Priority",
		"Exposure Time (Absolute)", "Exposure Time (Relative)", "Focus (Absolute)",
		"Focus (Relative)", "Iris (Absolute)", "Iris (Relative)", "Zoom (Absolute)",
		"Zoom (Relative)", "PanTilt (Absolute)", "PanTilt (Relative)",
		"Roll (Absolute)", "Roll (Relative)", "Reserved", "Reserved", "Focus, Auto",
		"Privacy"
	};
	static const char * const stdnames[] = {
		"None", "NTSC - 525/60", "PAL - 625/50", "SECAM - 625/50",
		"NTSC - 625/50", "PAL - 525/60" };
	unsigned int i, ctrls, stds, n, p, termt, freq;
	char term[128], termts[128];

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      VideoControl Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bDescriptorSubtype  %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01:  /* HEADER */
		printf("(HEADER)\n");
		n = buf[11];
		if (buf[0] < 12+n)
			printf("      Warning: Descriptor too short\n");
		freq = buf[7] | (buf[8] << 8) | (buf[9] << 16) | (buf[10] << 24);
		printf("        bcdUVC              %2x.%02x\n"
		       "        wTotalLength        %5u\n"
		       "        dwClockFrequency    %5u.%06uMHz\n"
		       "        bInCollection       %5u\n",
		       buf[4], buf[3], buf[5] | (buf[6] << 8), freq / 1000000,
		       freq % 1000000, n);
		for (i = 0; i < n; i++)
			printf("        baInterfaceNr(%2u)   %5u\n", i, buf[12+i]);
		xml_junk(buf, "        ", 12+n);
		break;

	case 0x02:  /* INPUT_TERMINAL */
		printf("(INPUT_TERMINAL)\n");
		get_string(dev, term, sizeof(term), buf[7]);
		termt = buf[4] | (buf[5] << 8);
		n = termt == 0x0201 ? 7 : 0;
		get_videoterminal_string(termts, sizeof(termts), termt);
		if (buf[0] < 8 + n)
			printf("      Warning: Descriptor too short\n");
		printf("        bTerminalID         %5u\n"
		       "        wTerminalType      0x%04x %s\n"
		       "        bAssocTerminal      %5u\n",
		       buf[3], termt, termts, buf[6]);
		printf("        iTerminal           %5u %s\n",
		       buf[7], term);
		if (termt == 0x0201) {
			n += buf[14];
			printf("        wObjectiveFocalLengthMin  %5u\n"
			       "        wObjectiveFocalLengthMax  %5u\n"
			       "        wOcularFocalLength        %5u\n"
			       "        bControlSize              %5u\n",
			       buf[8] | (buf[9] << 8), buf[10] | (buf[11] << 8),
			       buf[12] | (buf[13] << 8), buf[14]);
			ctrls = 0;
			for (i = 0; i < 3 && i < buf[14]; i++)
				ctrls = (ctrls << 8) | buf[8+n-i-1];
			printf("        bmControls           0x%08x\n", ctrls);
			for (i = 0; i < 19; i++)
				if ((ctrls >> i) & 1)
					printf("          %s\n", camctrlnames[i]);
		}
		xml_junk(buf, "        ", 8+n);
		break;

	case 0x03:  /* OUTPUT_TERMINAL */
		printf("(OUTPUT_TERMINAL)\n");
		get_string(dev, term, sizeof(term), buf[8]);
		termt = buf[4] | (buf[5] << 8);
		get_audioterminal_string(termts, sizeof(termts), termt);
		if (buf[0] < 9)
			printf("      Warning: Descriptor too short\n");
		printf("        bTerminalID         %5u\n"
		       "        wTerminalType      0x%04x %s\n"
		       "        bAssocTerminal      %5u\n"
		       "        bSourceID           %5u\n"
		       "        iTerminal           %5u %s\n",
		       buf[3], termt, termts, buf[6], buf[7], buf[8], term);
		xml_junk(buf, "        ", 9);
		break;

	case 0x04:  /* SELECTOR_UNIT */
		printf("(SELECTOR_UNIT)\n");
		p = buf[4];
		if (buf[0] < 6+p)
			printf("      Warning: Descriptor too short\n");
		get_string(dev, term, sizeof(term), buf[5+p]);

		printf("        bUnitID             %5u\n"
		       "        bNrInPins           %5u\n",
		       buf[3], p);
		for (i = 0; i < p; i++)
			printf("        baSource(%2u)        %5u\n", i, buf[5+i]);
		printf("        iSelector           %5u %s\n",
		       buf[5+p], term);
		xml_junk(buf, "        ", 6+p);
		break;

	case 0x05:  /* PROCESSING_UNIT */
		printf("(PROCESSING_UNIT)\n");
		n = buf[7];
		get_string(dev, term, sizeof(term), buf[8+n]);
		if (buf[0] < 10+n)
			printf("      Warning: Descriptor too short\n");
		printf("        bUnitID             %5u\n"
		       "        bSourceID           %5u\n"
		       "        wMaxMultiplier      %5u\n"
		       "        bControlSize        %5u\n",
		       buf[3], buf[4], buf[5] | (buf[6] << 8), n);
		ctrls = 0;
		for (i = 0; i < 3 && i < n; i++)
			ctrls = (ctrls << 8) | buf[8+n-i-1];
		printf("        bmControls     0x%08x\n", ctrls);
		for (i = 0; i < 18; i++)
			if ((ctrls >> i) & 1)
				printf("          %s\n", ctrlnames[i]);
		stds = buf[9+n];
		printf("        iProcessing         %5u %s\n"
		       "        bmVideoStandards     0x%2x\n", buf[8+n], term, stds);
		for (i = 0; i < 6; i++)
			if ((stds >> i) & 1)
				printf("          %s\n", stdnames[i]);
		break;

	case 0x06:  /* EXTENSION_UNIT */
		printf("(EXTENSION_UNIT)\n");
		p = buf[21];
		n = buf[22+p];
		get_string(dev, term, sizeof(term), buf[23+p+n]);
		if (buf[0] < 24+p+n)
			printf("      Warning: Descriptor too short\n");
		printf("        bUnitID             %5u\n"
		       "        guidExtensionCode         %s\n"
		       "        bNumControl         %5u\n"
		       "        bNrPins             %5u\n",
		       buf[3], get_guid(&buf[4]), buf[20], buf[21]);
		for (i = 0; i < p; i++)
			printf("        baSourceID(%2u)      %5u\n", i, buf[22+i]);
		printf("        bControlSize        %5u\n", buf[22+p]);
		for (i = 0; i < n; i++)
			printf("        bmControls(%2u)       0x%02x\n", i, buf[23+p+i]);
		printf("        iExtension          %5u %s\n",
		       buf[23+p+n], term);
		xml_junk(buf, "        ", 24+p+n);
		break;

	default:
		printf("(unknown)\n"
		       "        Invalid desc subtype:");
		xml_bytes(buf+3, buf[0]-3);
		break;
	}
}


void xml_videostreaming_interface(const unsigned char *buf)
{
	static const char * const colorPrims[] = { "Unspecified", "BT.709,sRGB",
		"BT.470-2 (M)", "BT.470-2 (B,G)", "SMPTE 170M", "SMPTE 240M" };
	static const char * const transferChars[] = { "Unspecified", "BT.709",
		"BT.470-2 (M)", "BT.470-2 (B,G)", "SMPTE 170M", "SMPTE 240M",
		"Linear", "sRGB"};
	static const char * const matrixCoeffs[] = { "Unspecified", "BT.709",
		"FCC", "BT.470-2 (B,G)", "SMPTE 170M (BT.601)", "SMPTE 240M" };
	unsigned int i, m, n, p, flags, len;

	if (buf[1] != USB_DT_CS_INTERFACE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 3)
		printf("      Warning: Descriptor too short\n");
	printf("      VideoStreaming Interface Descriptor:\n"
	       "        bLength                         %5u\n"
	       "        bDescriptorType                 %5u\n"
	       "        bDescriptorSubtype              %5u ",
	       buf[0], buf[1], buf[2]);
	switch (buf[2]) {
	case 0x01: /* INPUT_HEADER */
		printf("(INPUT_HEADER)\n");
		p = buf[3];
		n = buf[12];
		if (buf[0] < 13+p*n)
			printf("      Warning: Descriptor too short\n");
		printf("        bNumFormats                     %5u\n"
		       "        wTotalLength                    %5u\n"
		       "        bEndPointAddress                %5u\n"
		       "        bmInfo                          %5u\n"
		       "        bTerminalLink                   %5u\n"
		       "        bStillCaptureMethod             %5u\n"
		       "        bTriggerSupport                 %5u\n"
		       "        bTriggerUsage                   %5u\n"
		       "        bControlSize                    %5u\n",
		       p, buf[4] | (buf[5] << 8), buf[6], buf[7], buf[8],
		       buf[9], buf[10], buf[11], n);
		for (i = 0; i < p; i++)
			printf(
			"        bmaControls(%2u)                 %5u\n",
				i, buf[13+p*n]);
		xml_junk(buf, "        ", 13+p*n);
		break;

	case 0x02: /* OUTPUT_HEADER */
		printf("(OUTPUT_HEADER)\n");
		p = buf[3];
		n = buf[8];
		if (buf[0] < 9+p*n)
			printf("      Warning: Descriptor too short\n");
		printf("        bNumFormats                 %5u\n"
		       "        wTotalLength                %5u\n"
		       "        bEndpointAddress            %5u\n"
		       "        bTerminalLink               %5u\n"
		       "        bControlSize                %5u\n",
		       p, buf[4] | (buf[5] << 8), buf[6], buf[7], n);
		for (i = 0; i < p; i++)
			printf(
			"        bmaControls(%2u)             %5u\n",
				i, buf[9+p*n]);
		xml_junk(buf, "        ", 9+p*n);
		break;

	case 0x03: /* STILL_IMAGE_FRAME */
		printf("(STILL_IMAGE_FRAME)\n");
		n = buf[4];
		m = buf[5+4*n];
		if (buf[0] < 6+4*n+m)
			printf("      Warning: Descriptor too short\n");
		printf("        bEndpointAddress                %5u\n"
		       "        bNumImageSizePatterns             %3u\n",
		       buf[3], n);
		for (i = 0; i < n; i++)
			printf("        wWidth(%2u)                      %5u\n"
			       "        wHeight(%2u)                     %5u\n",
			       i, buf[5+4*i] | (buf[6+4*i] << 8),
			       i, buf[7+4*i] | (buf[8+4*i] << 8));
		printf("        bNumCompressionPatterns           %3u\n", n);
		for (i = 0; i < m; i++)
			printf("        bCompression(%2u)                %5u\n",
			       i, buf[6+4*n+i]);
		xml_junk(buf, "        ", 6+4*n+m);
		break;

	case 0x04: /* FORMAT_UNCOMPRESSED */
	case 0x10: /* FORMAT_FRAME_BASED */
		if (buf[2] == 0x04) {
			printf("(FORMAT_UNCOMPRESSED)\n");
			len = 27;
		} else {
			printf("(FORMAT_FRAME_BASED)\n");
			len = 28;
		}
		if (buf[0] < len)
			printf("      Warning: Descriptor too short\n");
		flags = buf[25];
		printf("        bFormatIndex                    %5u\n"
		       "        bNumFrameDescriptors            %5u\n"
		       "        guidFormat                            %s\n"
		       "        bBitsPerPixel                   %5u\n"
		       "        bDefaultFrameIndex              %5u\n"
		       "        bAspectRatioX                   %5u\n"
		       "        bAspectRatioY                   %5u\n"
		       "        bmInterlaceFlags                 0x%02x\n",
		       buf[3], buf[4], get_guid(&buf[5]), buf[21], buf[22],
		       buf[23], buf[24], flags);
		printf("          Interlaced stream or variable: %s\n",
		       (flags & (1 << 0)) ? "Yes" : "No");
		printf("          Fields per frame: %u fields\n",
		       (flags & (1 << 1)) ? 1 : 2);
		printf("          Field 1 first: %s\n",
		       (flags & (1 << 2)) ? "Yes" : "No");
		printf("          Field pattern: ");
		switch ((flags >> 4) & 0x03) {
		case 0:
			printf("Field 1 only\n");
			break;
		case 1:
			printf("Field 2 only\n");
			break;
		case 2:
			printf("Regular pattern of fields 1 and 2\n");
			break;
		case 3:
			printf("Random pattern of fields 1 and 2\n");
			break;
		}
		printf("          bCopyProtect                  %5u\n", buf[26]);
		if (buf[2] == 0x10)
			printf("          bVariableSize                 %5u\n", buf[27]);
		xml_junk(buf, "        ", len);
		break;

	case 0x05: /* FRAME UNCOMPRESSED */
	case 0x07: /* FRAME_MJPEG */
	case 0x11: /* FRAME_FRAME_BASED */
		if (buf[2] == 0x05) {
			printf("(FRAME_UNCOMPRESSED)\n");
			n = 25;
		} else if (buf[2] == 0x07) {
			printf("(FRAME_MJPEG)\n");
			n = 25;
		} else {
			printf("(FRAME_FRAME_BASED)\n");
			n = 21;
		}
		len = (buf[n] != 0) ? (26+buf[n]*4) : 38;
		if (buf[0] < len)
			printf("      Warning: Descriptor too short\n");
		flags = buf[4];
		printf("        bFrameIndex                     %5u\n"
		       "        bmCapabilities                   0x%02x\n",
		       buf[3], flags);
		printf("          Still image %ssupported\n",
		       (flags & (1 << 0)) ? "" : "un");
		if (flags & (1 << 1))
			printf("          Fixed frame-rate\n");
		printf("        wWidth                          %5u\n"
		       "        wHeight                         %5u\n"
		       "        dwMinBitRate                %9u\n"
		       "        dwMaxBitRate                %9u\n",
		       buf[5] | (buf[6] <<  8), buf[7] | (buf[8] << 8),
		       buf[9] | (buf[10] << 8) | (buf[11] << 16) | (buf[12] << 24),
		       buf[13] | (buf[14] << 8) | (buf[15] << 16) | (buf[16] << 24));
		if (buf[2] == 0x11)
			printf("        dwDefaultFrameInterval      %9u\n"
			       "        bFrameIntervalType              %5u\n"
			       "        dwBytesPerLine              %9u\n",
			       buf[17] | (buf[18] << 8) | (buf[19] << 16) | (buf[20] << 24),
			       buf[21],
			       buf[22] | (buf[23] << 8) | (buf[24] << 16) | (buf[25] << 24));
		else
			printf("        dwMaxVideoFrameBufferSize   %9u\n"
			       "        dwDefaultFrameInterval      %9u\n"
			       "        bFrameIntervalType              %5u\n",
			       buf[17] | (buf[18] << 8) | (buf[19] << 16) | (buf[20] << 24),
			       buf[21] | (buf[22] << 8) | (buf[23] << 16) | (buf[24] << 24),
			       buf[25]);
		if (buf[n] == 0)
			printf("        dwMinFrameInterval          %9u\n"
			       "        dwMaxFrameInterval          %9u\n"
			       "        dwFrameIntervalStep         %9u\n",
			       buf[26] | (buf[27] << 8) | (buf[28] << 16) | (buf[29] << 24),
			       buf[30] | (buf[31] << 8) | (buf[32] << 16) | (buf[33] << 24),
			       buf[34] | (buf[35] << 8) | (buf[36] << 16) | (buf[37] << 24));
		else
			for (i = 0; i < buf[n]; i++)
				printf("        dwFrameInterval(%2u)         %9u\n",
				       i, buf[26+4*i] | (buf[27+4*i] << 8) |
				       (buf[28+4*i] << 16) | (buf[29+4*i] << 24));
		xml_junk(buf, "        ", len);
		break;

	case 0x06: /* FORMAT_MJPEG */
		printf("(FORMAT_MJPEG)\n");
		if (buf[0] < 11)
			printf("      Warning: Descriptor too short\n");
		flags = buf[5];
		printf("        bFormatIndex                    %5u\n"
		       "        bNumFrameDescriptors            %5u\n"
		       "        bFlags                          %5u\n",
		       buf[3], buf[4], flags);
		printf("          Fixed-size samples: %s\n",
		       (flags & (1 << 0)) ? "Yes" : "No");
		flags = buf[9];
		printf("        bDefaultFrameIndex              %5u\n"
		       "        bAspectRatioX                   %5u\n"
		       "        bAspectRatioY                   %5u\n"
		       "        bmInterlaceFlags                 0x%02x\n",
		       buf[6], buf[7], buf[8], flags);
		printf("          Interlaced stream or variable: %s\n",
		       (flags & (1 << 0)) ? "Yes" : "No");
		printf("          Fields per frame: %u fields\n",
		       (flags & (1 << 1)) ? 2 : 1);
		printf("          Field 1 first: %s\n",
		       (flags & (1 << 2)) ? "Yes" : "No");
		printf("          Field pattern: ");
		switch ((flags >> 4) & 0x03) {
		case 0:
			printf("Field 1 only\n");
			break;
		case 1:
			printf("Field 2 only\n");
			break;
		case 2:
			printf("Regular pattern of fields 1 and 2\n");
			break;
		case 3:
			printf("Random pattern of fields 1 and 2\n");
			break;
		}
		printf("          bCopyProtect                  %5u\n", buf[10]);
		xml_junk(buf, "        ", 11);
		break;

	case 0x0a: /* FORMAT_MPEG2TS */
		printf("(FORMAT_MPEG2TS)\n");
		len = buf[0] < 23 ? 7 : 23;
		if (buf[0] < len)
			printf("      Warning: Descriptor too short\n");
		printf("        bFormatIndex                    %5u\n"
		       "        bDataOffset                     %5u\n"
		       "        bPacketLength                   %5u\n"
		       "        bStrideLength                   %5u\n",
		       buf[3], buf[4], buf[5], buf[6]);
		if (len > 7)
			printf("        guidStrideFormat                      %s\n",
			       get_guid(&buf[7]));
		xml_junk(buf, "        ", len);
		break;

	case 0x0d: /* COLORFORMAT */
		printf("(COLORFORMAT)\n");
		if (buf[0] < 6)
			printf("      Warning: Descriptor too short\n");
		printf("        bColorPrimaries                 %5u (%s)\n",
		       buf[3], (buf[3] <= 5) ? colorPrims[buf[3]] : "Unknown");
		printf("        bTransferCharacteristics        %5u (%s)\n",
		       buf[4], (buf[4] <= 7) ? transferChars[buf[4]] : "Unknown");
		printf("        bMatrixCoefficients             %5u (%s)\n",
		       buf[5], (buf[5] <= 5) ? matrixCoeffs[buf[5]] : "Unknown");
		xml_junk(buf, "        ", 6);
		break;

	default:
		printf("        Invalid desc subtype:");
		xml_bytes(buf+3, buf[0]-3);
		break;
	}
}

void xml_dfu_interface(const unsigned char *buf)
{
	if (buf[1] != USB_DT_CS_DEVICE)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 7)
		printf("      Warning: Descriptor too short\n");
	printf("      Device Firmware Upgrade Interface Descriptor:\n"
	       "        bLength                         %5u\n"
	       "        bDescriptorType                 %5u\n"
	       "        bmAttributes                    %5u\n",
	       buf[0], buf[1], buf[2]);
	if (buf[2] & 0xf0)
		printf("          (unknown attributes!)\n");
	printf("          Will %sDetach\n", (buf[2] & 0x08) ? "" : "Not ");
	printf("          Manifestation %s\n", (buf[2] & 0x04) ? "Tolerant" : "Intolerant");
	printf("          Upload %s\n", (buf[2] & 0x02) ? "Supported" : "Unsupported");
	printf("          Download %s\n", (buf[2] & 0x01) ? "Supported" : "Unsupported");
	printf("        wDetachTimeout                  %5u milliseconds\n"
	       "        wTransferSize                   %5u bytes\n",
	       buf[3] | (buf[4] << 8), buf[5] | (buf[6] << 8));

	/* DFU 1.0 defines no version code, DFU 1.1 does */
	if (buf[0] < 9)
		return;
	printf("        bcdDFUVersion                   %x.%02x\n",
			buf[8], buf[7]);
}

char *
xml_comm_descriptor(libusb_device_handle *dev, const unsigned char *buf, char *indent)
{
	int		tmp;
	char		str[128];
	char		*type;

	switch (buf[2]) {
	case 0:
		type = "Header";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC Header:\n"
		       "%s  bcdCDC               %x.%02x\n",
		       indent,
		       indent, buf[4], buf[3]);
		break;
	case 0x01:		/* call management functional desc */
		type = "Call Management";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC Call Management:\n"
		       "%s  bmCapabilities       0x%02x\n",
		       indent,
		       indent, buf[3]);
		if (buf[3] & 0x01)
			printf("%s    call management\n", indent);
		if (buf[3] & 0x02)
			printf("%s    use DataInterface\n", indent);
		printf("%s  bDataInterface          %d\n", indent, buf[4]);
		break;
	case 0x02:		/* acm functional desc */
		type = "ACM";
		if (buf[0] != 4)
			goto bad;
		printf("%sCDC ACM:\n"
		       "%s  bmCapabilities       0x%02x\n",
		       indent,
		       indent, buf[3]);
		if (buf[3] & 0x08)
			printf("%s    connection notifications\n", indent);
		if (buf[3] & 0x04)
			printf("%s    sends break\n", indent);
		if (buf[3] & 0x02)
			printf("%s    line coding and serial state\n", indent);
		if (buf[3] & 0x01)
			printf("%s    get/set/clear comm features\n", indent);
		break;
#if 0
	case 0x03:		/* direct line management */
	case 0x04:		/* telephone ringer */
	case 0x05:		/* telephone call and line state reporting */
#endif
	case 0x06:		/* union desc */
		type = "Union";
		if (buf[0] < 5)
			goto bad;
		printf("%sCDC Union:\n"
		       "%s  bMasterInterface        %d\n"
		       "%s  bSlaveInterface         ",
		       indent,
		       indent, buf[3],
		       indent);
		for (tmp = 4; tmp < buf[0]; tmp++)
			printf("%d ", buf[tmp]);
		printf("\n");
		break;
	case 0x07:		/* country selection functional desc */
		type = "Country Selection";
		if (buf[0] < 6 || (buf[0] & 1) != 0)
			goto bad;
		get_string(dev, str, sizeof str, buf[3]);
		printf("%sCountry Selection:\n"
		       "%s  iCountryCodeRelDate     %4d %s\n",
		       indent,
		       indent, buf[3], (buf[3] && *str) ? str : "(?\?)");
		for (tmp = 4; tmp < buf[0]; tmp += 2) {
			printf("%s  wCountryCode          0x%02x%02x\n",
				indent, buf[tmp], buf[tmp + 1]);
		}
		break;
	case 0x08:		/* telephone operational modes */
		type = "Telephone Operations";
		if (buf[0] != 4)
			goto bad;
		printf("%sCDC Telephone operations:\n"
		       "%s  bmCapabilities       0x%02x\n",
		       indent,
		       indent, buf[3]);
		if (buf[3] & 0x04)
			printf("%s    computer centric mode\n", indent);
		if (buf[3] & 0x02)
			printf("%s    standalone mode\n", indent);
		if (buf[3] & 0x01)
			printf("%s    simple mode\n", indent);
		break;
#if 0
	case 0x09:		/* USB terminal */
#endif
	case 0x0a:		/* network channel terminal */
		type = "Network Channel Terminal";
		if (buf[0] != 7)
			goto bad;
		get_string(dev, str, sizeof str, buf[4]);
		printf("%sNetwork Channel Terminal:\n"
		       "%s  bEntityId               %3d\n"
		       "%s  iName                   %3d %s\n"
		       "%s  bChannelIndex           %3d\n"
		       "%s  bPhysicalInterface      %3d\n",
		       indent,
		       indent, buf[3],
		       indent, buf[4], str,
		       indent, buf[5],
		       indent, buf[6]);
		break;
#if 0
	case 0x0b:		/* protocol unit */
	case 0x0c:		/* extension unit */
	case 0x0d:		/* multi-channel management */
	case 0x0e:		/* CAPI control management*/
#endif
	case 0x0f:		/* ethernet functional desc */
		type = "Ethernet";
		if (buf[0] != 13)
			goto bad;
		get_string(dev, str, sizeof str, buf[3]);
		tmp = buf[7] << 8;
		tmp |= buf[6]; tmp <<= 8;
		tmp |= buf[5]; tmp <<= 8;
		tmp |= buf[4];
		printf("%sCDC Ethernet:\n"
		       "%s  iMacAddress             %10d %s\n"
		       "%s  bmEthernetStatistics    0x%08x\n",
		       indent,
		       indent, buf[3], (buf[3] && *str) ? str : "(?\?)",
		       indent, tmp);
		/* FIXME dissect ALL 28 bits */
		printf("%s  wMaxSegmentSize         %10d\n"
		       "%s  wNumberMCFilters            0x%04x\n"
		       "%s  bNumberPowerFilters     %10d\n",
		       indent, (buf[9]<<8)|buf[8],
		       indent, (buf[11]<<8)|buf[10],
		       indent, buf[12]);
		break;
#if 0
	case 0x10:		/* ATM networking */
#endif
	case 0x11:		/* WHCM functional desc */
		type = "WHCM version";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC WHCM:\n"
		       "%s  bcdVersion           %x.%02x\n",
		       indent,
		       indent, buf[4], buf[3]);
		break;
	case 0x12:		/* MDLM functional desc */
		type = "MDLM";
		if (buf[0] != 21)
			goto bad;
		printf("%sCDC MDLM:\n"
		       "%s  bcdCDC               %x.%02x\n"
		       "%s  bGUID               %s\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, get_guid(buf + 5));
		break;
	case 0x13:		/* MDLM detail desc */
		type = "MDLM detail";
		if (buf[0] < 5)
			goto bad;
		printf("%sCDC MDLM detail:\n"
		       "%s  bGuidDescriptorType  %02x\n"
		       "%s  bDetailData         ",
		       indent,
		       indent, buf[3],
		       indent);
		xml_bytes(buf + 4, buf[0] - 4);
		break;
	case 0x14:		/* device management functional desc */
		type = "Device Management";
		if (buf[0] != 7)
			goto bad;
		printf("%sCDC Device Management:\n"
		       "%s  bcdVersion           %x.%02x\n"
		       "%s  wMaxCommand          %d\n",
		       indent,
		       indent, buf[4], buf[3],
		       indent, (buf[6] << 8) | buf[5]);
		break;
	case 0x15:		/* OBEX functional desc */
		type = "OBEX";
		if (buf[0] != 5)
			goto bad;
		printf("%sCDC OBEX:\n"
		       "%s  bcdVersion           %x.%02x\n",
		       indent,
		       indent, buf[4], buf[3]);
		break;
#if 0
	case 0x16:		/* command set functional desc */
	case 0x17:		/* command set detail desc */
	case 0x18:		/* telephone control model functional desc */
#endif
	default:
		/* FIXME there are about a dozen more descriptor types */
		printf("%sUNRECOGNIZED CDC: ", indent);
		xml_bytes(buf, buf[0]);
		return "unrecognized comm descriptor";
	}
	return 0;

bad:
	printf("%sINVALID CDC (%s): ", indent, type);
	xml_bytes(buf, buf[0]);
	return "corrupt comm descriptor";
}

/* ---------------------------------------------------------------------- */

/*
 * HID descriptor
 */

static void xml_report_desc(unsigned char *b, int l)
{
	unsigned int t, j, bsize, btag, btype, data = 0xffff, hut = 0xffff;
	int i;
	char *types[4] = { "Main", "Global", "Local", "reserved" };
	char indent[] = "                            ";

	printf("          Report Descriptor: (length is %d)\n", l);
	for (i = 0; i < l; ) {
		t = b[i];
		bsize = b[i] & 0x03;
		if (bsize == 3)
			bsize = 4;
		btype = b[i] & (0x03 << 2);
		btag = b[i] & ~0x03; /* 2 LSB bits encode length */
		printf("            Item(%-6s): %s, data=", types[btype>>2],
				names_reporttag(btag));
		if (bsize > 0) {
			printf(" [ ");
			data = 0;
			for (j = 0; j < bsize; j++) {
				printf("0x%02x ", b[i+1+j]);
				data += (b[i+1+j] << (8*j));
			}
			printf("] %d", data);
		} else
			printf("none");
		printf("\n");
		switch (btag) {
		case 0x04: /* Usage Page */
			printf("%s%s\n", indent, names_huts(data));
			hut = data;
			break;

		case 0x08: /* Usage */
		case 0x18: /* Usage Minimum */
		case 0x28: /* Usage Maximum */
			printf("%s%s\n", indent,
			       names_hutus((hut << 16) + data));
			break;

		case 0x54: /* Unit Exponent */
			printf("%sUnit Exponent: %i\n", indent,
			       (signed char)data);
			break;

		case 0x64: /* Unit */
			printf("%s", indent);
			xml_unit(data, bsize);
			break;

		case 0xa0: /* Collection */
			printf("%s", indent);
			switch (data) {
			case 0x00:
				printf("Physical\n");
				break;

			case 0x01:
				printf("Application\n");
				break;

			case 0x02:
				printf("Logical\n");
				break;

			case 0x03:
				printf("Report\n");
				break;

			case 0x04:
				printf("Named Array\n");
				break;

			case 0x05:
				printf("Usage Switch\n");
				break;

			case 0x06:
				printf("Usage Modifier\n");
				break;

			default:
				if (data & 0x80)
					printf("Vendor defined\n");
				else
					printf("Reserved for future use.\n");
			}
			break;
		case 0x80: /* Input */
		case 0x90: /* Output */
		case 0xb0: /* Feature */
			printf("%s%s %s %s %s %s\n%s%s %s %s %s\n",
			       indent,
			       data & 0x01 ? "Constant" : "Data",
			       data & 0x02 ? "Variable" : "Array",
			       data & 0x04 ? "Relative" : "Absolute",
			       data & 0x08 ? "Wrap" : "No_Wrap",
			       data & 0x10 ? "Non_Linear" : "Linear",
			       indent,
			       data & 0x20 ? "No_Preferred_State" : "Preferred_State",
			       data & 0x40 ? "Null_State" : "No_Null_Position",
			       data & 0x80 ? "Volatile" : "Non_Volatile",
			       data & 0x100 ? "Buffered Bytes" : "Bitfield");
			break;
		}
		i += 1 + bsize;
	}
}


void xml_hid_device(libusb_device_handle *dev,
			    const struct libusb_interface_descriptor *interface,
			    const unsigned char *buf)
{
	unsigned int i, len;
	unsigned int n;
	unsigned char dbuf[8192];

	if (buf[1] != LIBUSB_DT_HID)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 6+3*buf[5])
		printf("      Warning: Descriptor too short\n");
	printf("        HID Device Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bcdHID              %2x.%02x\n"
	       "          bCountryCode        %5u %s\n"
	       "          bNumDescriptors     %5u\n",
	       buf[0], buf[1], buf[3], buf[2], buf[4],
	       names_countrycode(buf[4]) ? : "Unknown", buf[5]);
	for (i = 0; i < buf[5]; i++)
		printf("          bDescriptorType     %5u %s\n"
		       "          wDescriptorLength   %5u\n",
		       buf[6+3*i], names_hid(buf[6+3*i]),
		       buf[7+3*i] | (buf[8+3*i] << 8));
	xml_junk(buf, "        ", 6+3*buf[5]);
	if (!do_report_desc)
		return;

	if (!dev) {
		printf("         Report Descriptors: \n"
		       "           ** UNAVAILABLE **\n");
		return;
	}

	for (i = 0; i < buf[5]; i++) {
		/* we are just interested in report descriptors*/
		if (buf[6+3*i] != LIBUSB_DT_REPORT)
			continue;
		len = buf[7+3*i] | (buf[8+3*i] << 8);
		if (len > (unsigned int)sizeof(dbuf)) {
			printf("report descriptor too long\n");
			continue;
		}
		if (libusb_claim_interface(dev, interface->bInterfaceNumber) == 0) {
			int retries = 4;
			n = 0;
			while (n < len && retries--)
				n = usb_control_msg(dev,
					 LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_STANDARD
						| LIBUSB_RECIPIENT_INTERFACE,
					 LIBUSB_REQUEST_GET_DESCRIPTOR,
					 (LIBUSB_DT_REPORT << 8),
					 interface->bInterfaceNumber,
					 dbuf, len,
					 CTRL_TIMEOUT);

			if (n > 0) {
				if (n < len)
					printf("          Warning: incomplete report descriptor\n");
				xml_report_desc(dbuf, n);
			}
			libusb_release_interface(dev, interface->bInterfaceNumber);
		} else {
			/* recent Linuxes require claim() for RECIP_INTERFACE,
			 * so "rmmod hid" will often make these available.
			 */
			printf("         Report Descriptors: \n"
			       "           ** UNAVAILABLE **\n");
		}
	}
}

void xml_audiostreaming_endpoint(const unsigned char *buf, int protocol)
{
	static const char * const lockdelunits[] = { "Undefined", "Milliseconds", "Decoded PCM samples", "Reserved" };
	unsigned int lckdelidx;

	if (buf[1] != USB_DT_CS_ENDPOINT)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < ((protocol == USB_AUDIO_CLASS_1) ? 7 : 8))
		printf("      Warning: Descriptor too short\n");
	printf("        AudioControl Endpoint Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bDescriptorSubtype  %5u (%s)\n"
	       "          bmAttributes         0x%02x\n",
	       buf[0], buf[1], buf[2], buf[2] == 1 ? "EP_GENERAL" : "invalid", buf[3]);

	switch (protocol) {
	case USB_AUDIO_CLASS_1:
		if (buf[3] & 1)
			printf("            Sampling Frequency\n");
		if (buf[3] & 2)
			printf("            Pitch\n");
		if (buf[3] & 128)
			printf("            MaxPacketsOnly\n");
		lckdelidx = buf[4];
		if (lckdelidx > 3)
			lckdelidx = 3;
		printf("          bLockDelayUnits     %5u %s\n"
		       "          wLockDelay          %5u %s\n",
		       buf[4], lockdelunits[lckdelidx], buf[5] | (buf[6] << 8), lockdelunits[lckdelidx]);
		xml_junk(buf, "        ", 7);
		break;

	case USB_AUDIO_CLASS_2:
		if (buf[3] & 128)
			printf("            MaxPacketsOnly\n");

		printf("          bmControls           0x%02x\n", buf[4]);
		xml_audio_bmcontrols("          ", buf[4], uac2_audio_endpoint_bmcontrols, protocol);

		lckdelidx = buf[5];
		if (lckdelidx > 3)
			lckdelidx = 3;
		printf("          bLockDelayUnits     %5u %s\n"
		       "          wLockDelay          %5u\n",
		       buf[5], lockdelunits[lckdelidx], buf[6] | (buf[7] << 8));
		xml_junk(buf, "        ", 8);
		break;
	} /* switch protocol */
}

void xml_midistreaming_endpoint(const unsigned char *buf)
{
	unsigned int j;

	if (buf[1] != USB_DT_CS_ENDPOINT)
		printf("      Warning: Invalid descriptor\n");
	else if (buf[0] < 5)
		printf("      Warning: Descriptor too short\n");
	printf("        MIDIStreaming Endpoint Descriptor:\n"
	       "          bLength             %5u\n"
	       "          bDescriptorType     %5u\n"
	       "          bDescriptorSubtype  %5u (%s)\n"
	       "          bNumEmbMIDIJack     %5u\n",
	       buf[0], buf[1], buf[2], buf[2] == 1 ? "GENERAL" : "Invalid", buf[3]);
	for (j = 0; j < buf[3]; j++)
		printf("          baAssocJackID(%2u)   %5u\n", j, buf[4+j]);
	xml_junk(buf, "          ", 4+buf[3]);
}

void xml_hub(const char *prefix, const unsigned char *p, int tt_type)
{
	unsigned int l, i, j;
	unsigned int offset;
	unsigned int wHubChar = (p[4] << 8) | p[3];

	printf("%sHub Descriptor:\n", prefix);
	printf("%s  bLength             %3u\n", prefix, p[0]);
	printf("%s  bDescriptorType     %3u\n", prefix, p[1]);
	printf("%s  nNbrPorts           %3u\n", prefix, p[2]);
	printf("%s  wHubCharacteristic 0x%04x\n", prefix, wHubChar);
	switch (wHubChar & 0x03) {
	case 0:
		printf("%s    Ganged power switching\n", prefix);
		break;
	case 1:
		printf("%s    Per-port power switching\n", prefix);
		break;
	default:
		printf("%s    No power switching (usb 1.0)\n", prefix);
		break;
	}
	if (wHubChar & 0x04)
		printf("%s    Compound device\n", prefix);
	switch ((wHubChar >> 3) & 0x03) {
	case 0:
		printf("%s    Ganged overcurrent protection\n", prefix);
		break;
	case 1:
		printf("%s    Per-port overcurrent protection\n", prefix);
		break;
	default:
		printf("%s    No overcurrent protection\n", prefix);
		break;
	}
	/* USB 3.0 hubs don't have TTs. */
	if (tt_type >= 1 && tt_type < 3) {
		l = (wHubChar >> 5) & 0x03;
		printf("%s    TT think time %d FS bits\n", prefix, (l + 1) * 8);
	}
	/* USB 3.0 hubs don't have port indicators.  Sad face. */
	if (tt_type != 3 && wHubChar & (1<<7))
		printf("%s    Port indicators\n", prefix);
	printf("%s  bPwrOn2PwrGood      %3u * 2 milli seconds\n", prefix, p[5]);

	/* USB 3.0 hubs report current in units of aCurrentUnit, or 4 mA */
	if (tt_type == 3)
		printf("%s  bHubContrCurrent   %4u milli Ampere\n",
				prefix, p[6]*4);
	else
		printf("%s  bHubContrCurrent    %3u milli Ampere\n",
				prefix, p[6]);

	if (tt_type == 3) {
		printf("%s  bHubDecLat          0.%1u micro seconds\n",
				prefix, p[7]);
		printf("%s  wHubDelay          %4u nano seconds\n",
				prefix, (p[8] << 4) +(p[7]));
		offset = 10;
	} else {
		offset = 7;
	}

	l = (p[2] >> 3) + 1; /* this determines the variable number of bytes following */
	if (l > HUB_STATUS_BYTELEN)
		l = HUB_STATUS_BYTELEN;
	printf("%s  DeviceRemovable   ", prefix);
	for (i = 0; i < l; i++)
		printf(" 0x%02x", p[offset+i]);

	if (tt_type != 3) {
		printf("\n%s  PortPwrCtrlMask   ", prefix);
		for (j = 0; j < l; j++)
			printf(" 0x%02x", p[offset+i+j]);
	}
	printf("\n");
}

void xml_ccid_device(const unsigned char *buf)
{
	unsigned int us;

	if (buf[0] < 54) {
		printf("      Warning: Descriptor too short\n");
		return;
	}
	printf("      ChipCard Interface Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bcdCCID             %2x.%02x",
	       buf[0], buf[1], buf[3], buf[2]);
	if (buf[3] != 1 || buf[2] != 0)
		fputs("  (Warning: Only accurate for version 1.0)", stdout);
	putchar('\n');

	printf("        nMaxSlotIndex       %5u\n"
		"        bVoltageSupport     %5u  %s%s%s\n",
		buf[4],
		buf[5],
	       (buf[5] & 1) ? "5.0V " : "",
	       (buf[5] & 2) ? "3.0V " : "",
	       (buf[5] & 4) ? "1.8V " : "");

	us = convert_le_u32 (buf+6);
	printf("        dwProtocols         %5u ", us);
	if ((us & 1))
		fputs(" T=0", stdout);
	if ((us & 2))
		fputs(" T=1", stdout);
	if ((us & ~3))
		fputs(" (Invalid values detected)", stdout);
	putchar('\n');

	us = convert_le_u32(buf+10);
	printf("        dwDefaultClock      %5u\n", us);
	us = convert_le_u32(buf+14);
	printf("        dwMaxiumumClock     %5u\n", us);
	printf("        bNumClockSupported  %5u\n", buf[18]);
	us = convert_le_u32(buf+19);
	printf("        dwDataRate        %7u bps\n", us);
	us = convert_le_u32(buf+23);
	printf("        dwMaxDataRate     %7u bps\n", us);
	printf("        bNumDataRatesSupp.  %5u\n", buf[27]);

	us = convert_le_u32(buf+28);
	printf("        dwMaxIFSD           %5u\n", us);

	us = convert_le_u32(buf+32);
	printf("        dwSyncProtocols  %08X ", us);
	if ((us&1))
		fputs(" 2-wire", stdout);
	if ((us&2))
		fputs(" 3-wire", stdout);
	if ((us&4))
		fputs(" I2C", stdout);
	putchar('\n');

	us = convert_le_u32(buf+36);
	printf("        dwMechanical     %08X ", us);
	if ((us & 1))
		fputs(" accept", stdout);
	if ((us & 2))
		fputs(" eject", stdout);
	if ((us & 4))
		fputs(" capture", stdout);
	if ((us & 8))
		fputs(" lock", stdout);
	putchar('\n');

	us = convert_le_u32(buf+40);
	printf("        dwFeatures       %08X\n", us);
	if ((us & 0x0002))
		fputs("          Auto configuration based on ATR\n", stdout);
	if ((us & 0x0004))
		fputs("          Auto activation on insert\n", stdout);
	if ((us & 0x0008))
		fputs("          Auto voltage selection\n", stdout);
	if ((us & 0x0010))
		fputs("          Auto clock change\n", stdout);
	if ((us & 0x0020))
		fputs("          Auto baud rate change\n", stdout);
	if ((us & 0x0040))
		fputs("          Auto parameter negotation made by CCID\n", stdout);
	else if ((us & 0x0080))
		fputs("          Auto PPS made by CCID\n", stdout);
	else if ((us & (0x0040 | 0x0080)))
		fputs("        WARNING: conflicting negotation features\n", stdout);

	if ((us & 0x0100))
		fputs("          CCID can set ICC in clock stop mode\n", stdout);
	if ((us & 0x0200))
		fputs("          NAD value other than 0x00 accpeted\n", stdout);
	if ((us & 0x0400))
		fputs("          Auto IFSD exchange\n", stdout);

	if ((us & 0x00010000))
		fputs("          TPDU level exchange\n", stdout);
	else if ((us & 0x00020000))
		fputs("          Short APDU level exchange\n", stdout);
	else if ((us & 0x00040000))
		fputs("          Short and extended APDU level exchange\n", stdout);
	else if ((us & 0x00070000))
		fputs("        WARNING: conflicting exchange levels\n", stdout);

	us = convert_le_u32(buf+44);
	printf("        dwMaxCCIDMsgLen     %5u\n", us);

	printf("        bClassGetResponse    ");
	if (buf[48] == 0xff)
		fputs("echo\n", stdout);
	else
		printf("  %02X\n", buf[48]);

	printf("        bClassEnvelope       ");
	if (buf[49] == 0xff)
		fputs("echo\n", stdout);
	else
		printf("  %02X\n", buf[48]);

	printf("        wlcdLayout           ");
	if (!buf[50] && !buf[51])
		fputs("none\n", stdout);
	else
		printf("%u cols %u lines\n", buf[50], buf[51]);

	printf("        bPINSupport         %5u ", buf[52]);
	if ((buf[52] & 1))
		fputs(" verification", stdout);
	if ((buf[52] & 2))
		fputs(" modification", stdout);
	putchar('\n');

	printf("        bMaxCCIDBusySlots   %5u\n", buf[53]);

	if (buf[0] > 54) {
		fputs("        junk             ", stdout);
		xml_bytes(buf+54, buf[0]-54);
	}
}

void xml_rc_interface(const unsigned char *buf)
{
	printf("      Radio Control Interface Class Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bcdRCIVersion       %2x.%02x\n",
	       buf[0], buf[1], buf[3], buf[2]);

}

void xml_wire_adapter(const unsigned char *buf)
{

	printf("      Wire Adapter Class Descriptor:\n"
	       "        bLength             %5u\n"
	       "        bDescriptorType     %5u\n"
	       "        bcdWAVersion        %2x.%02x\n"
	       "	 bNumPorts	     %5u\n"
	       "	 bmAttributes	     %5u\n"
	       "	 wNumRPRipes	     %5u\n"
	       "	 wRPipeMaxBlock	     %5u\n"
	       "	 bRPipeBlockSize     %5u\n"
	       "	 bPwrOn2PwrGood	     %5u\n"
	       "	 bNumMMCIEs	     %5u\n"
	       "	 DeviceRemovable     %5u\n",
	       buf[0], buf[1], buf[3], buf[2], buf[4], buf[5],
	       (buf[6] | buf[7] << 8),
	       (buf[8] | buf[9] << 8),
	       buf[10], buf[11], buf[12], buf[13]);
}
