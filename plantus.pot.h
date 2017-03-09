#pragma once

#include "mbed.h"
#include "LPC17xx.h"
#include "ConfigFile.h"
#include "XBeeLib.h"
#include "config.h"
#include "TSL2561.h"

using namespace XBeeLib;

#define DEBUG true
#define NODE_ID_LENGTH 2
#define SOIL_HUMIDITY_DATA_LENGTH 1
#define TSL2561_DATA_LENGTH 2
#define AMBIANT_HUMIDITY_DATA_LENGTH 1
#define PUMP_STATE 1
#define DATA_LENGTH NODE_ID_LENGTH + SOIL_HUMIDITY_DATA_LENGTH + TSL2561_DATA_LENGTH + AMBIANT_HUMIDITY_DATA_LENGTH + PUMP_STATE
#define CHANNEL_0 0
#define BAUD_RATE 115200
#define HEXA_BASE 16

// configuration file
char cfgPath[] = "/local/config.cfg"; 
char periodeKey[] = "PERIODE";
char panIDKey[] = "PAN_ID";
char nodeIdKey[] = "NODE_ID";
uint16_t periode;
uint16_t panID; 

// peripherals
extern Serial pc;
extern DigitalOut LEDs[4];
extern LocalFileSystem local;
extern ConfigFile cfg;
extern TSL2561 tsl2561;
extern DigitalOut waterPump;

// prototypes
void ReadCaptors(TSL2561 *tsl2561);
void FlashLED(uint16_t led);
void CreateFrame(uint16_t data[]);
void SendFrameToXBee(uint16_t frame[]);
void sendDataToCoordinator(char data[]);
void readConfigFile(uint16_t *periode, uint16_t *panID);
void setupXBee(uint16_t panID);
void startCaptorsThread(TSL2561 *tsl2561, uint16_t periode);
uint16_t readSoilHumidity(void);
uint16_t readAmbiantHumidity(void);
void setWaterPumpTo(bool state);