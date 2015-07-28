/*
  Modified version of LOG's Temperature monito program
  Jim Thomas
*/

/*********************************************************************
 * L.O.G. receiving data from NRF24L01-DS18B20 modules
 * and displaying them on LCD5110

 */
/*
 Copyright (C) 2012 James Coliz, Jr. <maniacbug@ymail.com>
 */
/*********************************************************************
This is an example sketch for our Monochrome Nokia 5110 LCD Displays

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/338

These displays use SPI to communicate, 4 or 5 pins are required to
interface

Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
*********************************************************************/
#include <Adafruit_GFX.h>
#include <Adafruit_PCD8544.h>
#include <RF24Network.h>
#include <RF24.h>
#include <SPI.h>
#include <OneWire.h>
#include <DallasTemperature.h>

// DS18B20 is plugged into Arduino D4 
#define ONE_WIRE_BUS 4
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(ONE_WIRE_BUS);
// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);
// This is DS18B20 #20 address
DeviceAddress Therm1 = { 0x28, 0x44, 0x15, 0x5D, 0x05, 0x00, 0x00, 0x69 };  //unit #3

bool centigrade = false;
int i;
float temp1C;
float temp1F;

unsigned long startTime;
unsigned long clearTime = 240000;	// in milliseconds

// Number of Temperature sensors
#define NumNodes 3
// nRF24L01(+) radio attached 
RF24 radio(9,10);
// Network uses that radio
RF24Network network(radio);

// Channel of our node
const uint16_t channel = 90;
// Address of our node
const uint16_t this_node = 0;

// Structure of our payload
struct payload_t
{
  unsigned long counter;
  float tempC;
  float tempF;
  float Vcc;		// 4 bytes

};

// Since nodes start at 1, I added 1 to arrays to make it easier
unsigned long NodeCounter[NumNodes+1];
float NodeTempC[NumNodes+1];
float NodeTempF[NumNodes+1];
float NodeVcc[NumNodes+1];

// LCD5110 connections
// pin 3 - Serial clock out (SCLK)
// pin 2 - Serial data out (DIN)
// pin 17 - Data/Command select (D/C)
// pin 18 - LCD chip select (CS)
// pin 19 - LCD reset (RST)
//  Adafruit_PCD8544(SCLK, DIN, DC, CS, RST);
Adafruit_PCD8544 display = Adafruit_PCD8544(3, 2, 17, 18, 19);

//Prototypes for utility functions
void getTemperature(DeviceAddress);	// getTemperature
void displayData();					// display data
void getRadioData();				// get Radio data
void clearTemperatures();			// clear temps to make sure modules are active

void setup(){
	// Start up the library
	sensors.begin();
    // set the Temp resolution to 12 bit 
	sensors.setResolution(Therm1, 12);

	Serial.begin(9600);
	Serial.println("Temperature Data");
 
	SPI.begin();
	// Radio setup
	radio.begin();
	// network.begin(/*channel*/, /*node address*/);
	network.begin(channel, this_node);
	
	display.begin();
	display.setContrast(67);	// Choose best contrast - default 55
	display.display(); 			// show splashscreen
	delay(20);
	display.clearDisplay();   	// clears the screen and buffer
	
	startTime = millis();		// start the timer

}

void loop() {
	// Pump the network regularly
	network.update();
	getTemperature(Therm1);	
	// Is there anything ready for us?
	while ( network.available() ){
		// If so, grab it and print it out
		getRadioData();
	}

	displayData();
	delay(100);
	if ( (millis() - startTime) > clearTime){ 
		clearTemperatures();  
		startTime = millis();		// restart the timer
	}
}

//////////////////////////////////////////////////////////////////
// getTemperature
//////////////////////////////////////////////////////////////////
void getTemperature(DeviceAddress deviceAddress)
{
	sensors.requestTemperatures();
	temp1C = sensors.getTempC(deviceAddress);
	temp1F = sensors.getTempF(deviceAddress);
}

//////////////////////////////////////////////////////////////////////////////////
// displayData()
//////////////////////////////////////////////////////////////////////////////////
void displayData(){
// Lcd5110 will display 14 characters x 6 rows
	display.clearDisplay();   	// clears the screen and buffer
	display.setTextSize(1);
	display.setCursor(0,0);
	display.setTextColor(BLACK);
	
	for (i=1; i <= NumNodes; i++){
		display.print(i);
		display.print(" ");
		if(centigrade){
			display.print(NodeTempC[i], 1);
			display.print("C ");
		}else{
			display.print(NodeTempF[i], 1);
			display.print("F ");
		}
		display.print(NodeVcc[i],1);
		display.println("V");
	}

	display.print("Local: ");
	if(centigrade){
		display.print(temp1C, 1);
		display.print("C ");
	}else{
		display.print(temp1F, 1);
		display.print("F ");
	}
	display.display();

}

//////////////////////////////////////////////////////////////////////////////////
// getRadioData()					// get Network data
//////////////////////////////////////////////////////////////////////////////////
void getRadioData(){
	RF24NetworkHeader header;
	payload_t payload;
	bool done = false;
      
	while (!done){

		done = 	network.read(header,&payload,sizeof(payload));

		NodeCounter[header.from_node] = payload.counter;
		NodeTempC[header.from_node] = payload.tempC;
		NodeTempF[header.from_node] = payload.tempF;
		NodeVcc[header.from_node] = payload.Vcc;
		
		Serial.print("Received packet #");
		Serial.print(NodeCounter[header.from_node]);
 
		Serial.print(" from node#: ");
		Serial.print(header.from_node);
		
		Serial.print(" TempC: ");
		Serial.print(NodeTempC[header.from_node], 1);
		Serial.print(" TempF: ");
		Serial.print(NodeTempF[header.from_node], 1);
		Serial.print(" Vcc: ");
		Serial.println(NodeVcc[header.from_node], 1);
		Serial.print(" Local: Temp C:");
                Serial.print(temp1C, 1);
                Serial.print(" Temp F:");
		Serial.println(temp1F, 1);
	}
}

//////////////////////////////////////////////////////////////////////////////////
// clearTemperatures()					// clear temps to make sure modules are active
//////////////////////////////////////////////////////////////////////////////////

void clearTemperatures(){
	for (i=1; i <= NumNodes; i++){
		NodeTempC[i] = 0.0;
		NodeTempF[i] = 0.0;
		NodeVcc[i] = 0.0;
	}
}
