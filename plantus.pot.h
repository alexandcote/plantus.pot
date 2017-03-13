#pragma once

#include "mbed.h"
#include "LPC17xx.h"
#include "ConfigFile.h"
#include "XBeeLib.h"
#include "config.h"
#include "TSL2561.h"

using namespace XBeeLib;

#define DEBUG false
#define MAC_ADR_LENGTH 6

// used for Frame
#define NODE_ID_LENGTH 2
#define SOIL_HUMIDITY_DATA_LENGTH 1
#define LUMINOSITY_DATA_LENGTH 2
#define AMBIANT_HUMIDITY_DATA_LENGTH 1
#define PUMP_DATA_LENGTH 1
#define DATA_LENGTH MAC_ADR_LENGTH + NODE_ID_LENGTH + SOIL_HUMIDITY_DATA_LENGTH + LUMINOSITY_DATA_LENGTH + AMBIANT_HUMIDITY_DATA_LENGTH + PUMP_DATA_LENGTH
#define TURN_WATER_PUMP_ON 0xBB
#define TURN_WATER_PUMP_OFF 0xAA
#define ALTERNATE_WATER_PUMP_STATE 0xFF // for debug purposes

// used for XBee
#define XBEE_BAUD_RATE 115200
#define NODE_IDENTIFIER_MAX_LENGTH  21
#define NODE_DISCOVERY_TIMEOUT_MS   5000
#define MAX_NODES 5
#define COORDINATOR_16BIT_ADDRESS 0x00
#define MAX_COORDINATOR_MESSAGES 10

// used for TSL2561
#define CHANNEL_0 0

// used for configuration file
#define HEXA_BASE 16
#define CFG_PATH "/local/config.cfg"
#define CFG_KEY_PERIOD "PERIOD"
#define CFG_KEY_PAN_ID "PAN_ID"
#define CFG_KEY_NODE_ID "NODE_ID"
#define CFG_KEY_NODE_IDENTIFIER "NODE_IDENTIFIER"

uint16_t periode;
uint16_t panID; 
uint8_t remoteNodesCount = 0;

// global variables
ConfigFile cfg;
RemoteXBeeZB remoteNodesInNetwork[MAX_NODES];
char nodeIdentifier[NODE_IDENTIFIER_MAX_LENGTH];
char macAdr[MAC_ADR_LENGTH];
char frameData[DATA_LENGTH];
char nodeID[NODE_ID_LENGTH];
char soilHumidity[SOIL_HUMIDITY_DATA_LENGTH];
char luminosity[LUMINOSITY_DATA_LENGTH];
char ambiantHumidity[AMBIANT_HUMIDITY_DATA_LENGTH];
char waterPumpState[PUMP_DATA_LENGTH];

// peripherals
extern Serial pc;
extern DigitalOut LEDs[4];
extern LocalFileSystem local;
extern ConfigFile cfg;
extern TSL2561 tsl2561;
extern DigitalOut waterPump;

// prototypes
extern "C" void mbed_mac_address(char *mac);
void ReadCaptors(TSL2561 *tsl2561);
void SetLedTo(uint16_t led, bool state);
void FlashLed(uint16_t led);
void CreateFrame(uint16_t data[]);
void SendDataToCoordinator(char data[]);
void ReadConfigFile(uint16_t *periode, uint16_t *panID, char *nodeID, char *nodeIdentifier);
void SetupXBee(uint16_t panID);
void StartEventQueueThread(TSL2561 *tsl2561, uint16_t periode);
uint16_t ReadSoilHumidity(void);
uint16_t ReadAmbiantHumidity(void);
void GetMacAddress(char *macAdr);
void CreateDataFrame(void);
void DiscoverAllNodes(void);
void CheckIfNewFrameIsPresent(void);
void NewFrameReceivedHandler(const RemoteXBeeZB &remoteNode, bool broadcast, const uint8_t *const data, uint16_t len);