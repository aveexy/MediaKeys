/*
 Name:		MediaKeys.ino
 Created:	1/14/2018 8:14:13 PM
 Author:	Av_
*/

#include <HID.h>

#define REMOTE_CLEAR 0
#define VOLUME_UP 1
#define VOLUME_DOWN 2
#define REMOTE_PLAYPAUSE 4
#define REMOTE_NEXT 8
#define REMOTE_PREVIOUS 16

static const u8 _hidReportDescriptor[] PROGMEM = {

	0x05, 0x0c, //	Usage Page (Consumer Devices)
	0x09, 0x01, //	Usage (Consumer Control)
	0xa1, 0x01, //	Collection (Application)
	0x85, 0x04, //	REPORT_ID (4)
	0x15, 0x00, //	Logical Minimum (0)
	0x25, 0x01, //	Logical Maximum (1)

	0x09, 0xe9, //	Usage (Volume Up)
	0x09, 0xea, //	Usage (Volume Down)
	0x75, 0x01, //	Report Size (1)
	0x95, 0x02, //	Report Count (2)
	0x81, 0x06, //	Input (Data, Variable, Relative)

	0x09, 0xcd, //	Usage (Play/Pause)
	0x95, 0x01, //	Report Count (1)
	0x81, 0x06, //	Input (Data, Variable, Relative)

	0x09, 0xb5, //	Usage (Next)
	0x95, 0x01, //	Report Count (1)
	0x81, 0x06, //	Input (Data, Variable, Relative)

	0x09, 0xb6, //	Usage (Previous)
	0x95, 0x01, //	Report Count (1)
	0x81, 0x06, //	Input (Data, Variable, Relative)

	0x95, 0x03, //	Report Count (3) Number of bits remaining in byte
	0x81, 0x07, //	Input (Constant, Variable, Relative) 
	0xc0        //	End Collection
};

inline boolean extractState(unsigned int state) {
	return !!(state & 0x0100);
}
inline word debounce(word prevstate, boolean data) {
	register word newstate = ((prevstate >> 1) & 0x7F7F) + ((prevstate >> 2) & 0x3F3F) + (data ? 65 : 0);
	newstate = newstate + ((newstate << 6) & 0x3F00) + ((prevstate & 0x0100) << 1);
	newstate = (newstate & ~0x0100) | (prevstate & 0x0100 ? (newstate > 0x7000 ? 0x0100 : 0) : (newstate > 0x9000 ? 0x0100 : 0));
	if ((prevstate ^ newstate) & 0x0100) {
		if (newstate & 0x0100) newstate = 0xFFFF;
		else newstate = 0;
	}
	return newstate;
}

unsigned int most_recent_sampleButton_t[5];
word filter[5];
boolean lastState[5];
boolean GetButtonClick(int pinNr, int persistenceId) {
	int ms = ((int)millis()) & ~0x03;
	if (ms != most_recent_sampleButton_t[persistenceId]) {
		most_recent_sampleButton_t[persistenceId] = ms;
		boolean buttonState = !digitalRead(pinNr);
		filter[persistenceId] = debounce(filter[persistenceId], buttonState);
	}

	boolean state = extractState(filter[persistenceId]);

	if (state) {
		if (!lastState[persistenceId]) {
			lastState[persistenceId] = true;
			return true;
		}
	} else {
		lastState[persistenceId] = false;
	}

	return false;
}

void SendReport(u8 id) {
	u8 m[2];
	m[0] = id;
	m[1] = 0;
	HID().SendReport(4, m, 2);

	m[0] = REMOTE_CLEAR;
	m[1] = 0;
	HID().SendReport(4, m, 2);
}

void setup() {
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);

	for (u8 i = 2; i < 8; i++) {
		pinMode(i, INPUT_PULLUP);
	}

	if (digitalRead(7)) {
		static HIDSubDescriptor node(_hidReportDescriptor, sizeof(_hidReportDescriptor));
		HID().AppendDescriptor(&node);
		HID().begin();
		digitalWrite(LED_BUILTIN, LOW);
	}
}

void loop() {
	if (GetButtonClick(2, 0)) {
		SendReport(VOLUME_DOWN);
	}

	if (GetButtonClick(3, 1)) {
		SendReport(VOLUME_UP);
	}

	if (GetButtonClick(4, 2)) {
		SendReport(REMOTE_PREVIOUS);
	}

	if (GetButtonClick(5, 3)) {
		SendReport(REMOTE_PLAYPAUSE);
	}

	if (GetButtonClick(6, 4)) {
		SendReport(REMOTE_NEXT);
	}
}
