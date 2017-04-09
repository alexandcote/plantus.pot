#pragma once

#include "mbed.h"
#include "LPC17xx.h"
#include "ConfigFile.h"
#include "XBeeLib.h"
#include "config.h"
#include "TSL2561.h"
#include <algorithm>

using namespace XBeeLib;

#define DEBUG false
#define INFO true
#define MAC_ADR_LENGTH 6

// used for Frame
#define OPERATION_ID_MAX_LENGTH 3
#define NODE_ID_LENGTH 2

// must be the same in station...
#define LUMINOSITY_DATA_LENGTH 1
#define TEMPERATURE_DATA_LENGTH 6
#define SOIL_HUMIDITY_DATA_LENGTH 1
#define WATER_LEVEL_DATA_LENGTH 1
#define FRAME_PREFIX_LENGTH 1
#define FRAME_DATA_LENGTH FRAME_PREFIX_LENGTH + LUMINOSITY_DATA_LENGTH + TEMPERATURE_DATA_LENGTH + SOIL_HUMIDITY_DATA_LENGTH + WATER_LEVEL_DATA_LENGTH

#define FRAME_PREFIX_WATER_PLANT 0xBB
#define FRAME_PREFIX_TURN_WATER_PUMP_OFF 0xAA
#define FRAME_PREFIX_ALTERNATE_WATER_PUMP_STATE 0xFF // for debug purposes
#define FRAME_PREFIX_NEW_DATA 0x00
#define FRAME_PREFIX_ADD_POT_IDENTIFIER 0x01
#define FRAME_PREFIX_COMPLETED_OPERATION 0x10

// used for XBee
#define XBEE_BAUD_RATE 115200
#define NODE_IDENTIFIER_MAX_LENGTH  21
#define NODE_DISCOVERY_TIMEOUT_MS   5000
#define MAX_NODES 5
#define COORDINATOR_16BIT_ADDRESS 0x00
#define MAX_COORDINATOR_MESSAGES 10
#define POT_IDENTIFIER_LENGTH 37

// used for TSL2561
#define CHANNEL_0 0
#define MAX_LUMINOSITY 0x134B // mesured max value

// used for configuration file
#define HEXA_BASE 16
#define CFG_PATH "/local/config.cfg"
#define CFG_KEY_PERIOD "PERIOD"
#define CFG_KEY_PAN_ID "PAN_ID"
#define CFG_POT_IDENTIFIER "POT_IDENTIFIER"

// global variables
uint16_t periode;
uint16_t panID; 
uint8_t remoteNodesCount = 0;
uint16_t pumpActivationTime = 5000; // ms
const float TMP36VoltageOffset = 0.500;
const char framePrefixNewData = {FRAME_PREFIX_NEW_DATA};
char potIdentifier[POT_IDENTIFIER_LENGTH];
char luminosity[LUMINOSITY_DATA_LENGTH];
char temperature[TEMPERATURE_DATA_LENGTH];
char soilHumidity[SOIL_HUMIDITY_DATA_LENGTH];
char waterLevel[WATER_LEVEL_DATA_LENGTH];
char globalOperationId[FRAME_PREFIX_LENGTH + OPERATION_ID_MAX_LENGTH];
RemoteXBeeZB remoteNodesInNetwork[MAX_NODES];
ConfigFile cfg;

// peripherals
extern Serial pc;
extern DigitalOut LEDs[4];
extern LocalFileSystem local;
extern ConfigFile cfg;
extern TSL2561 tsl2561;
extern DigitalOut waterPump;
extern AnalogIn tmp36;

// prototypes
extern "C" void mbed_mac_address(char *mac);
void ReadCaptors(void);
void SetLedTo(uint16_t led, bool state);
void FlashLed(uint16_t led);
void SendFrameToCoordinator(char frame[], uint16_t frameLength);
void ReadConfigFile(uint16_t *periode, uint16_t *panID);
void SetupXBee(uint16_t panID);
void StartEventQueue(uint16_t periode);
void GetMacAddress(char *macAdr);
void CreateDataFrameAndSendToCoordinator(void);
void CheckIfNewXBeeFrameIsPresent(void);
void NewFrameReceivedHandler(const RemoteXBeeZB &remoteNode, bool broadcast, const uint8_t *const data, uint16_t len);
void SendPotIdentifierToCoordinator(void);
void AlternateWaterPump(char operationId[]);
void WaterPlant(char operationId[]);
void SetWaterPumpToAndNotifyCoordinator(bool state, char operationId[]);
void SendCompletedOperationToCoordinator(char operationId[]);
void PrepareFrameToSend(char frame[], char data[], int framePrefix);
int InsertDataToFrame(int frameIndexOffset, const int maxIndex, const char data[], char frame[]);
float ReadTemperature(void);
uint16_t ReadSoilHumidity(void);
uint16_t ReadWaterLevel(void);
uint16_t ReadLuminosityPercent(void);