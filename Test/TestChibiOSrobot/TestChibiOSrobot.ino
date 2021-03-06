//////////////////////////////////////////////////////////////////////////////////
// CONFIGURAZIONE DEL SISTEMA                  ///////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#pragma region CONFIGURAZIONE DEL SISTEMA   

#include <MyRobotLibs/SystemConfig.h>

#include <MyRobotLibs/dbg.h>


#pragma endregion


// ////////////////////////////////////////////////////////////////////////////////////////////
// ///																						///
// ///       LIBRERIE 																		///
// ///		Aggiungere ciascun percorso nelle proprietÓ del progetto in Visual Studio 		///
// ///		Configuration Properties >C++ > Path											///
// ////////////////////////////////////////////////////////////////////////////////////////////
#pragma region LIBRERIE
#include <digitalWriteFast.h>
#include <ChibiOS_AVR.h>
#include <util/atomic.h>




#if 1

#include <TinyGPS++.h>
//static const int RXPin = 4, TXPin = 3;
//static const uint32_t GPSBaud = SERIAL_GPS_BAUD_RATE;

// The serial connection to the GPS device
//SoftwareSerial SerialGps(RXPin, TXPin);


TinyGPSPlus gps;

#endif


//#include <FrequencyTimer2\FrequencyTimer2.h>	
//#include <TimerThree\TimerThree.h>
//#include <FlexiTimer2\FlexiTimer2.h>
//#include <encoder/Encoder.h>
#include <Servo.h> //deve restare qui altrimenti il linker s'incazza (??)
#include <Newping\NewPing.h>
#include <PWM\PWM.h>
//#include <avr/wdt.h>
#include <MyRobotLibs/robot.h>
#include "speak.h"


#pragma endregion

// ////////////////////////////////////////////////////////////////////////////////////////////
//  CREAZIONE OGGETTI GLOBALI
// ////////////////////////////////////////////////////////////////////////////////////////////
#pragma region CREAZIONE OGGETTI GLOBALI

#if OPT_SERVOSONAR
	// va messo prima dell'istanza del robot
 	Servo servoSonar;
	NewPing Sonar(Pin_SonarTrig,Pin_SonarEcho);
#endif

 struct robot_c robot;
// ////////////////////////////////////////////////////////////////////////////////////////////


// ////////////////////////////////////////////////////////////////////////////////////////////
//  CmdMessenger object to the default Serial port
// ////////////////////////////////////////////////////////////////////////////////////////////
#include <CmdMessenger2/CmdMessenger2.h>
static CmdMessenger2 cmdMMI = CmdMessenger2(SERIAL_MMI);
static CmdMessenger2 cmdWiFi = CmdMessenger2(SERIAL_WIFI);
#include <robot/RobotInterfaceCommands2.h>

//------------------------------------------------------------------------------
#pragma region DEFINIZIONE MAILBOX VOICE
// mailbox size and memory pool object count
const size_t MBOX_COUNT = 6;

// type for a memory pool object
struct mboxObject_t {
	char* name;
	char str[100];
	int size;
};
// array of memory pool objects
mboxObject_t msgObjArray[MBOX_COUNT];

// memory pool structure
MEMORYPOOL_DECL(memPool, MBOX_COUNT, 0);

// slots for mailbox messages
msg_t letter[MBOX_COUNT];

// mailbox structure
MAILBOX_DECL(mailVoice, &letter, MBOX_COUNT);

#pragma endregion

// ////////////////////////////////////////////////////////////////////////////////////////////

#pragma endregion



// ///////////////////////////////////////////////////////////////////////////////
//  CHARARRAY STRING LIB					     ///////////////////////////////// 
// ///////////////////////////////////////////////////////////////////////////////
#pragma region Funzioni di manipolazione stringhe di tipo CHARARRAY
void strtrim(char* str) {
	int start = 0; // number of leading spaces
	char* buffer = str;
	while (*str && *str++ == ' ') ++start;
	while (*str++); // move to end of string
	int end = str - buffer - 1;
	while (end > 0 && buffer[end - 1] == ' ') --end; // backup over trailing spaces
	buffer[end] = 0; // remove trailing spaces
	if (end <= start || start == 0) return; // exit if no leading spaces or string is now empty
	str = buffer + start;
	while ((*buffer++ = *str++));  // remove leading spaces: K&R
}
template<size_t charCount>
void strtrim_safe(char(&output)[charCount]) {
	char *ptr = output;
	size_t n = charCount;
	size_t start = 0;
	size_t end = 0;

	// Find the start and end position of trimmed string
	while (n-- != 0 && *ptr != 0) {
		if (*ptr == 32) {
			if (end == 0) {
				start++;
			}
			else {
				break;
			}
		}
		else {
			end++;
		}

		ptr++;
	}

	// Shift the char array 
	for (int i = start, j = 0; i < end + start && j < charCount; i++, j++) {
		output[j] = output[i];
	}
	output[end] = 0;
}

ptrdiff_t index_of(const char *string, char search) {
	const char *moved_string = strchr(string, search);
	/* If not null, return the difference. */
	if (moved_string) {
		return moved_string - string;
	}
	/* Character not found. */
	return -1;
}


void substring(char *buff, byte pos, byte len) {
	char subbuff[50];
	memcpy(subbuff, &buff[pos], len);
	subbuff[len] = '\0';
}

#pragma endregion



//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//                   /////////////////////////////////////////////////////////////
// PROCESSI CHIBIOS  /////////////////////////////////////////////////////////////
//                   /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// thread 1		- Esplora
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
static THD_WORKING_AREA(waThreadEsplora, 64);
static THD_FUNCTION(ThreadEsplora, arg) {
	const int FWDIST = 10;
	int alfa = 0;
	int cmDone = 0;
	int stuckCount = 0;
	// Flash led every 200 ms.

	while (1) 
	{
		if (digitalReadFast(Pin_SwitchTop) != robot.status.switchTop)
		{
			// switch cambiato
			//		SPEAK_OK
			robot.status.switchTop = digitalReadFast(Pin_SwitchTop);
			if (robot.status.switchTop) // Autonomo
			{
				robot.operatingMode = AUTONOMOUS;
				SPEAK("okei esploro")
			}
			else
			{
				SPEAK("okei agli ordini")

			}
		}
		if (robot.operatingMode== AUTONOMOUS)
		{
			robot.parameter.sonarStartAngle = 0;
			robot.parameter.sonarEndAngle = 180;
			robot.parameter.sonarStepAngle = 30;
			robot.parameter.sonarScanSweeps = 1;
			robot.parameter.sonarMedianSamples = 2;
			robot.parameter.sonarScanSpeed = map(analogRead(Pin_AnaPot1),0,1023, 10, 500);  //was = 30 ms di attesa tra due posizioni
			 
			robot.SonarScanBatch(&servoSonar, &Sonar);
			alfa = 90 - robot.parameter.SonarMaxDistAngle;

			// invia i dati Sonar
			kbSonarSendData(&cmdMMI);

	//		printf("1,Rot. %d ;", alfa);

			robot.rotateDeg(alfa);
			cmDone =robot.moveCm(FWDIST);	// avanti 
			if (cmDone < (FWDIST -1))	//ostacolo ?
			{ 
				//Serial.print("1,Obst!;");
				robot.moveCm(-FWDIST);	// torna indietro
				stuckCount++;
				if (stuckCount>2)
				{
					robot.rotateDeg(180); //inverto la direzione
					stuckCount = 0;
				}
			}
			else //nessun ostacolo, azzero il contatore
			{
				stuckCount = 0;
			}
		
			chThdSleepMilliseconds(500);	// Sleep for 150 milliseconds.

		}
		else   // non autonomo
		{
				


			chThdSleepMilliseconds(1000);	// Sleep for 50 milliseconds.
		}
	}
//  return 0;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// thread 2 - invio stato sensori
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

static THD_WORKING_AREA(waThreadSendSensorsHR, 64);
static THD_FUNCTION(ThreadSendSensorsHR, arg) {

	while (1) 
	{

		robot.readSensors();	//IR proxy, Gyro


		cmdWiFi.sendCmdStart(kbGetSensorsHRate);
		cmdWiFi.sendCmdArg(robot.status.irproxy.fw);	// IR proxy
		cmdWiFi.sendCmdArg(robot.status.irproxy.fwHL);	// IR proxy
		cmdWiFi.sendCmdArg(robot.status.irproxy.bk);	// IR proxy
		cmdWiFi.sendCmdArg(robot.status.pirDome);		// movimento

		cmdWiFi.sendCmdArg(robot.status.analog[0]);	//pot
		cmdWiFi.sendCmdArg(robot.status.analog[1]);	//batteria
		cmdWiFi.sendCmdArg(robot.status.analog[2]);	//light

		cmdWiFi.sendCmdArg(robot.getReleStatus(0));		//rele 1
		cmdWiFi.sendCmdArg(robot.getReleStatus(1));		//rel2

		cmdWiFi.sendCmdArg(digitalReadFast(Pin_MotENR));
		cmdWiFi.sendCmdArg(digitalReadFast(Pin_MotENL));

		cmdWiFi.sendCmdArg(digitalReadFast(Pin_BtOnOff));
		cmdWiFi.sendCmdArg(digitalReadFast(Pin_LaserOn));
		cmdWiFi.sendCmdArg(robot.readBattChargeLevel());

		cmdWiFi.sendCmdEnd();

		chThdSleepMilliseconds(1000);// Sleep for n milliseconds.



	}
//  return 0;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// thread 3 - Invio sensori a bassa frequenza
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

static THD_WORKING_AREA(waThreadSendSensorsLR, 200);
static THD_FUNCTION(ThreadSendSensorsLR, arg) {
	//SERIAL_MSG.begin(SERIAL_MSG_BAUD_RATE);
	cmdWiFi.printLfCr();   // Adds newline to every command 

	//loop ---------------------------------------------------------------
	while (1) {
		LEDTOP_ON
			chThdSleepMilliseconds(100);
		LEDTOP_OFF						// Turn LED off.	 

 
		chThdSleepMilliseconds(500);// Sleep for n milliseconds.
	}
//	return 0;
}




//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// thread 4 - gps
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
static THD_WORKING_AREA(waGPS, 64);
static THD_FUNCTION(GPS, arg) {
	// Setup ---------------------------------------------
	//SERIAL_GPS.begin(SERIAL_GPS_BAUD_RATE);

	while (1) {
		
		while (SERIAL_GPS.available() > 0)	gps.encode(SERIAL_GPS.read());
		if (gps.location.isUpdated()) {
			//printf("1,SATS= %l ;", gps.satellites.value());
			//printf("1,LAT= %l ;", gps.location.lat());
			//printf("1,LONG== %l ;", gps.location.lng());
			//printf("1,ALT== %f ;", gps.altitude.meters());
			SERIAL_MSG.print("1,");
			SERIAL_MSG.println(gps.satellites.value()); // Number of satellites in use (u32)
			SERIAL_MSG.print("LAT=");  SERIAL_MSG.print(gps.location.lat(), 6);
			SERIAL_MSG.print(", LONG="); SERIAL_MSG.print(gps.location.lng(), 6);
			SERIAL_MSG.print(", ALT=");  SERIAL_MSG.print(gps.altitude.meters());
			SERIAL_MSG.print(";");
		}
		// Sleep for n milliseconds.
		chThdSleepMilliseconds(2000);
	}
//  return 0;
}


//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
// thread   MMIcommands - ricezione comandi da MMI
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

static THD_WORKING_AREA(waMMIcommands, 256);
static THD_FUNCTION(MMIcommands,arg) {
//static msg_t MMIcommands(void *arg)


	// Setup CommandMessenger -----------------------------------------------------
	cmdMMI.printLfCr();   // Adds newline to every command 
//	cmdWiFi.printLfCr();   // Adds newline to every command 
	attachCommandCallbacks(&cmdMMI);// Attach my application's user-defined callback methods
	
	while (true)
	{
		//printf("1,>;");
		cmdMMI.feedinSerialData();
//		cmdWiFi.feedinSerialData();

		chThdSleepMilliseconds(200);
		//if (robot.operatingMode == SLAVE) 			{chThdSleepMilliseconds(200);}// Sleep for n milliseconds.
		//else if (robot.operatingMode == AUTONOMOUS)	{chThdSleepMilliseconds(500);}// Sleep for n milliseconds.
		//else										{chThdSleepMilliseconds(100);}// Sleep for n milliseconds.
	}
}
//////////////////////////////////////////////////////////////////////////////////
//  blinking LED       ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#pragma region  Processo blinking LED  
#define PIN_LED  13
// 64 byte stack beyond task switch and interrupt needs
static THD_WORKING_AREA(waFlashLed, 64);
static THD_FUNCTION(FlashLed, arg) {
	// Flash led every 200 ms.
	pinMode(PIN_LED, OUTPUT);		digitalWrite(PIN_LED, 0);	// led superiore

	while (1) {
		// Turn LED on.
		LEDTOP_ON
		//digitalWriteFast(PIN_LED, HIGH);

		// Sleep for 50 milliseconds.
		chThdSleepMilliseconds(50);

		// Turn LED off.
		LEDTOP_OFF
		//digitalWriteFast(PIN_LED, LOW);

		// Sleep for 150 milliseconds.
		chThdSleepMilliseconds(950);
	}
}
#pragma endregion 
//////////////////////////////////////////////////////////////////////////////////
// M O N I T O R	   ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#pragma region Processo di MONITOR
#define BATTERY_WARNING_LEVEL 40		//livello al disotto del quale avvisa 
#define BATTERY_CRITICAL_LEVEL 30		//livello al disotto del quale spegne la WEBCAM
volatile uint32_t count = 0;
volatile uint32_t maxDelay = 0;

static THD_WORKING_AREA(waThreadMonitor, 64);
static THD_FUNCTION(ThreadMonitor, arg) {

	while (true)
	{
		if (robot.readBattChargeLevel() < BATTERY_WARNING_LEVEL)
		{
			SPEAK("LIVELLO BATTERIA BASSO ")
		}
		if (robot.readBattChargeLevel() < BATTERY_CRITICAL_LEVEL)
		{
			WEBCAM_OFF
		}

		//Serial.print(F("    waFifoFeed unused stack: "));
		//Serial.println(chUnusedStack(waFifoFeed, sizeof(waFifoFeed)));
		//Serial.print(F("    overrun errors: "));
		//Serial.println( OverrunErrorCount);

		count++;
		//FIFO_SPEAK.push(int2str(count));
		uint32_t t = micros();
		// yield so other threads can run
		chThdYield();
		t = micros() - t;
		if (t > maxDelay) maxDelay = t;

		chThdSleepMilliseconds(3000);//	chThdYield();//	
//		SPEAK_CIAO
	}

}

#pragma endregion 

// ////////////////////////////////////////////////////////////////////////////////////////////
// OS Setup (non cambia se non si aggiungono task)
// ////////////////////////////////////////////////////////////////////////////////////////////
#pragma region [CHIBIOS RTOS]

extern unsigned int __bss_end;
extern unsigned int __heap_start;
extern void *__brkval;

uint16_t getFreeSram() {
	uint8_t newVariable;
	// heap is empty, use bss as start memory address 
	if ((uint16_t)__brkval == 0)
		return (((uint16_t)&newVariable) - ((uint16_t)&__bss_end));
	// use heap end as the start of the memory address 
	else
		return (((uint16_t)&newVariable) - ((uint16_t)__brkval));
};

void chSetup() {
 	Serial.println("robot chibios");
	// fill pool with msgObjArray array
	for (size_t i = 0; i < MBOX_COUNT; i++) {
		chPoolFree(&memPool, &msgObjArray[i]);
	}

	chThdCreateStatic(waFlashLed,sizeof(waFlashLed), NORMALPRIO +1, FlashLed, NULL);
	chThdCreateStatic(waMMIcommands,sizeof(waMMIcommands), NORMALPRIO +1, MMIcommands, NULL);
	chThdCreateStatic(waThreadEsplora,sizeof(waThreadEsplora), NORMALPRIO +1, ThreadEsplora, NULL);//-Esplora
	chThdCreateStatic(waThreadSendSensorsHR,sizeof(waThreadSendSensorsHR),	NORMALPRIO + 1, ThreadSendSensorsHR, NULL);
	chThdCreateStatic(waGPS,sizeof(waGPS),	NORMALPRIO + 1, GPS, NULL);
 	chThdCreateStatic(waThreadMonitor,sizeof(waThreadMonitor),	NORMALPRIO + 1, ThreadMonitor, NULL);



	dbg("all chThd started..")


}

void setup()
{
	LEDTOP_ON

	//	// inizializzazione ROBOT ---------------------
	robot.initServoAndSonar(&servoSonar, &Sonar);
	//robot.runIBIT(300, &servoSonar, &Sonar);

	WEBCAM_ON

	SERIAL_WIFI.begin(SERIAL_WIFI_BAUD_RATE);
	SERIAL_MMI.begin(SERIAL_MMI_BAUD_RATE);
	SERIAL_MSG.begin(SERIAL_MSG_BAUD_RATE);
	SERIAL_GPS.begin(SERIAL_GPS_BAUD_RATE);

	chBegin(chSetup);	while (1) {}
}


void loop() {
	// not used
}

#pragma endregion


