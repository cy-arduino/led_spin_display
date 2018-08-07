#include "test.h"

//let pin13(onboard led) usable
#define BLINKLED 99
#define BLINKLED_ON()
#define BLINKLED_OFF()

/////////////////////////////////////////////////
//config
#define DBG_SERIAL 0
#define BAUD_RATE 38400

//debounce time(us)
#define ISR_DEBOUNCE 600
//#define ISR_DETECT_RATIO 0.3

#define RESOLUTION 200
#define INT_PIN 2

#define LED_NUM 16
#define LED_START 0
#define LED_LOW_ACTIVE 1
/////////////////////////////////////////////////

volatile unsigned int led_data[RESOLUTION];
volatile bool reset = false;

volatile unsigned long lastestIntTime;
volatile unsigned long diffIntTime;
volatile unsigned long avgIntTime;
volatile unsigned long realIntTime;
volatile unsigned long lastestRealIntTime;

volatile unsigned long curTime;

//LED GPIO
//bit 0~3: d4~d7(portD 4~7)
//bit 4~7: d8~d11(portB 0~3)
//bit 8~9: d12~d13(portB 4~5)
//bit 10~15: A0~A5(portC 0~5)

void setup() {
	Serial.begin(BAUD_RATE);
	//while (!Serial) {
	// wait for serial port to connect. Needed for native USB port only
	//}

	ledPinSetup();
	interruptSetup();

	lastestIntTime = micros();

	initLedData();

	ledShow(0);
}

void handleCmd() {
	int opcode;
	String cmd;
	String reply;

	Serial.println("===CMD_MODE===");
	ledShow(0);

	while (true) {
		if (Serial.available()) {
			cmd = Serial.readStringUntil(13);//return when input enter or timeout occur
			cmd.toUpperCase();

			if (cmd.startsWith("PING")) {
				reply = "PONG";
				Serial.println(reply);
			}
			else if (cmd.startsWith("APPLY")) {
				return;
			}
			else if (cmd.startsWith("SET_LED")) {
				handleCmd_SET_LED();
			}
			else {
				Serial.print("Unsupported command: ");
				Serial.println(cmd);
			}
		}
	}
}

void handleCmd_SET_LED() {
	String cmd;
	Serial.println("===CMD_MODE:SET_LED in===");

	int addr_serial = 0;

	while (true) {
		if (Serial.available()) {
			cmd = Serial.readStringUntil(13);//return when input enter or timeout occur
			cmd.toUpperCase();

			if (cmd.startsWith("DONE")) {
				Serial.println("===CMD_MODE:SET_LED out===");
				return;
			}
			else if (cmd.startsWith("CLEAR")) {
				for (int i = 0; i < RESOLUTION; i++) {
					led_data[i] = 0;
				}
				Serial.println("OK");
			}
			else if (cmd.startsWith("DEFAULT")) {
				initLedData();
				Serial.println("OK");
			}
			else if (cmd.startsWith("SET")) {
				//format: SET <addr(0~65535)> <data(0~65535)> 
				//example: address=1, data=0xffff
				//	SET 00001 65535

				bool success = false;
				unsigned int addr, data;//2 byte in arduino!

				if (cmd.length() >= 15) {

					addr = cmd.substring(4, 9).toInt();
					data = cmd.substring(10, 15).toInt();

					Serial.print("###SET: ");
					Serial.print(addr);
					Serial.print(" ");
					Serial.print(data);
					Serial.println();

					if (addr >= 0 && addr < RESOLUTION) {
						success = true;
						led_data[addr] = data;
					}
				}

				if (success)
					Serial.println("OK");
				else
					Serial.println("FAIL");

			}
			else if (cmd.startsWith("SERIAL")) {
				//format: SERIAL <data(0~65535)> 
				//example: address=0-2, data=0xffff
				//	SERIAL 65535
				//  SERIAL 65535
				//  SERIAL 65535

				bool success = false;
				unsigned int data;//2 byte in arduino!

				
				if (cmd.length() >= 12) {

					data = cmd.substring(7, 12).toInt();
					
					Serial.print("###SERIAL: ");
					Serial.print(addr_serial);
					Serial.print(" ");
					Serial.print(data);
					Serial.println();

					if (addr_serial >= 0 && addr_serial < RESOLUTION) {
						success = true;
						led_data[addr_serial] = data;

						addr_serial++;
					}
				}

				if (success)
					Serial.println("OK");
				else
					Serial.println("FAIL");

			}
			else {
				Serial.print("Unsupported command: ");
				Serial.println(cmd);
			}
		}
	}

}



void loop() {
#if 1
	SHOW_LED:
			Serial.println("===SHOW_LED===");
			reset = false;
			while (true) {

			INT_TRIGGER:

				if (Serial.available())
					goto CMD_MODE;

				if (reset) {

					reset = 0;
					for (int i = 0; i < RESOLUTION; i++) {
						ledShow(led_data[i]);

						while (micros() < lastestRealIntTime + realIntTime * i / RESOLUTION) {
							if (reset)
								goto INT_TRIGGER;
						}
					}
				}
			}

		CMD_MODE:
			handleCmd();

#endif

			//for check interrupt
#if 0 
			while (true) {
				if (reset) {
					reset = false;
					if (DBG_SERIAL) Serial.println("GO!");
					ledShow(0xffff);
					delayMicroseconds(10);
					ledShow(0);
				}
			}
#endif

			//for test led
#if 0 
			unsigned int i, data;
			while (true) {
				//delayMicroseconds(10000);
				data = 1 << ++i % 16;
				ledShow(data);
				//if(DBG_SERIAL) Serial.println(data);
				delay(20);
			}
#endif

}

void isr() {
	diffIntTime = micros() - lastestIntTime;
	avgIntTime = (avgIntTime + diffIntTime) / 2;
	lastestIntTime += diffIntTime;

	if (DBG_SERIAL) Serial.print("interrupt: diffIntTime=");
	if (DBG_SERIAL) Serial.println(diffIntTime);

	//debounce
	if (diffIntTime > ISR_DEBOUNCE /*&& diffIntTime < (diffIntTime * ISR_DETECT_RATIO)*/) {
		lastestRealIntTime = lastestIntTime;
		if (DBG_SERIAL) Serial.println("take");
		reset = true;
		realIntTime = (realIntTime + diffIntTime) / 2;
	}
}

void interruptSetup() {
	pinMode(INT_PIN, INPUT_PULLUP);
	attachInterrupt(digitalPinToInterrupt(INT_PIN), isr, FALLING);//pin2 is INT0, falling edge trigger
}

void ledPinSetup() {

	//set gpio to output
	//bit 0~3: d4~d7(portD 4~7)
	DDRD |= B11110000;
	//bit 4~7: d8~d11(portB 0~3)
	//bit 8~9: d12~d13(portB 4~5)
	DDRB |= B00111111;
	//bit 10~15: A0~A5(portC 0~5)
	DDRC |= B00111111;
}

void ledShow(unsigned int val) {

	val = val << LED_START;

	if (LED_LOW_ACTIVE)
		val = ~val;

	byte PD = PORTD;
	byte PB = PORTB;
	byte PC = PORTC;

	//analog to digital out
	pinMode(A0, OUTPUT);
	pinMode(A1, OUTPUT);
	pinMode(A2, OUTPUT);
	pinMode(A3, OUTPUT);
	pinMode(A4, OUTPUT);
	pinMode(A5, OUTPUT);

	//set output
	//bit 0~3: d4~d7(portD 4~7)
	PORTD = (PD & 0x0F) | (val & 0x000F) << 4;

	//bit 4~7: d8~d11(portB 0~3)
	//bit 8~9: d12~d13(portB 4~5)
	PORTB = (PB & 0xC0) | (val & 0x03F0) >> 4;

	//bit 10~15: A0~A5(portC 0~5)
	PORTC = (PC & 0xC0) | (val & 0xFC00) >> 10;

	//set initial value

}

void initLedData() {
	//test data
#if 0
	//black & white
	unsigned testData = 0xAAAA;
	for (int i = 0; i < RESOLUTION; i++) {
		led_data[i] = (i % 2) ? testData : ~testData;
	}
#endif

	//chinese text demo
#if 1
	for (int i = 0; i < sizeof(textChinese) / 16; i++) {
		if (i > RESOLUTION * 0.8)
			break;

		unsigned int d = 0;
		for (int j = 15; j >= 0; j--) {
			d = d << 1 | textChinese[i * 16 + j];
		}
		led_data[i] = d;
	}
#endif
}
