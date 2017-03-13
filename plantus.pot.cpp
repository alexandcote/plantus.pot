#include "plantus.pot.h"
/* TODO list:
    Read soil moisture lvl
    Read ambiant moisture lvl
    Receive actions from coordinator
    Activate pump until humidity lvl is adequate or set of time?
*/
Serial pc(USBTX, USBRX);   // tx, rx
#define DEBUG_PRINTX(DEBUG, x) if(DEBUG) {pc.printf(x);}
#define DEBUG_PRINTXNL(DEBUG, x) if(DEBUG) {pc.printf(x);               pc.printf("\r\n");}
#define DEBUG_PRINTXY(DEBUG, x, y) if(DEBUG) {pc.printf(x, y);}
#define DEBUG_PRINTXYNL(DEBUG, x, y) if(DEBUG) {pc.printf(x, y);         pc.printf("\r\n");}
#define DEBUG_PRINTXYZ(DEBUG, x, y, z) if(DEBUG) {pc.printf(x, y, z);}
#define DEBUG_PRINTXYZNL(DEBUG, x, y, z) if(DEBUG) {pc.printf(x, y, z);  pc.printf("\r\n");}

DigitalOut waterPump(p21);
DigitalOut LEDs[4] = {
    DigitalOut(LED1), DigitalOut(LED2), DigitalOut(LED3), DigitalOut(LED4)
};
LocalFileSystem local("local");

EventQueue eventQueue(32 * EVENTS_EVENT_SIZE); // holds 32 events
Thread eventQueueThread;
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, XBEE_BAUD_RATE);

// TODO more dynamic?
void ReadCaptors(TSL2561 *tsl2561) {
    uint16_t readLuminosity = tsl2561->getLuminosity(CHANNEL_0);
    DEBUG_PRINTXYNL(DEBUG, "read Luminosity = '0x%X'", readLuminosity);
    uint16_t readSoilHumidity = ReadSoilHumidity();
    DEBUG_PRINTXYNL(DEBUG, "read Soil Humidity = '0x%X'", readSoilHumidity);
    uint16_t readAmbianteHumidity = ReadAmbiantHumidity(); 
    DEBUG_PRINTXYNL(DEBUG, "read Ambiant Humidity = '0x%X'", readAmbianteHumidity);
    uint16_t readWaterPumpState = waterPump; 
    DEBUG_PRINTXYNL(DEBUG, "read Water Pump State = '%i'", readWaterPumpState);    

    soilHumidity[0] = readSoilHumidity;        
    luminosity[0] = readLuminosity >> 8;         // high luminosity
    luminosity[1] = readLuminosity & 0xFF;       // low luminosity
    ambiantHumidity[0] = readAmbianteHumidity;    
    waterPumpState[0] = readWaterPumpState;      
    // TODO add CreateFrame in the event queue?
}

void CreateDataFrame(void) {
    frameData[0] = macAdr[0];
    frameData[1] = macAdr[1];
    frameData[2] = macAdr[2];
    frameData[3] = macAdr[3];
    frameData[4] = macAdr[4];
    frameData[5] = macAdr[5]; 
    frameData[6] = nodeID[0];            // High node ID
    frameData[7] = nodeID[1];            // Low node ID
    frameData[8] = luminosity[0];        // High luminosity
    frameData[9] = luminosity[1];        // Low luminosity
    frameData[10] = ambiantHumidity[0];    
    frameData[11] = waterPumpState[0]; 
    SendDataToCoordinator(frameData);
}

uint16_t ReadSoilHumidity(void) {
    // TODO read analog
    return 0xFF;
}

uint16_t ReadAmbiantHumidity(void) {
    // TODO read ambiant Humidity
    return 0xAA;
}

void FlashLed(uint16_t led) {
    LEDs[led] = !LEDs[led];
}

void SetLedTo(uint16_t led, bool state) {
    LEDs[led] = state;
}

void GetMacAddress(char *macAdr) {
    mbed_mac_address(macAdr);
    #if DEBUG
    DEBUG_PRINTX(DEBUG, "\r\nmBed Mac Adress = ");
     for(int i=0; i<MAC_ADR_LENGTH; i++) {
        DEBUG_PRINTXY(DEBUG, "%02X ", macAdr[i]);      
    }   
    DEBUG_PRINTXNL(DEBUG, "\n\r");
    #endif
}

void SendDataToCoordinator(char data[])
{
    /*
    #if DEBUG
    DEBUG_PRINTXNL(DEBUG, "sending data :");
    for(int i = 0; i < DATA_LENGTH; i++) {
        DEBUG_PRINTXYZNL(DEBUG, "data at index '%i' = '0x%X'", i, data[i]);
    }
    #endif
    */
    TxStatus txStatus = xBee.send_data_to_coordinator((const uint8_t *)data, DATA_LENGTH);
    if (txStatus == TxStatusSuccess) {
        DEBUG_PRINTXNL(DEBUG, "Data was sent to coordinator successfully!\r\n");
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Failed to send to coordinator with error '%d', look in XBee.h for more details\r\n", (int) txStatus);
    }
}

void NewFrameReceivedHandler(const RemoteXBeeZB &remoteNode, bool broadcast, const uint8_t *const data, uint16_t len)
{
    uint64_t remote64Adress = remoteNode.get_addr64();
    uint32_t highAdr = remote64Adress >> 32;
    uint32_t lowAdr = remote64Adress;
    DEBUG_PRINTXYZNL(DEBUG, "\r\nGot a %s RX packet of length '%d'", broadcast ? "BROADCAST" : "UNICAST", len);

    DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
    DEBUG_PRINTXYZ(DEBUG, "64 bit remote address is: '0x%X%X'", highAdr, lowAdr);
    DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");

    DEBUG_PRINTX(DEBUG, "Data is: ");
    for (int i = 0; i < len; i++)
        DEBUG_PRINTXY(DEBUG, "0x%X ", data[i]);
    DEBUG_PRINTXNL(DEBUG, "\r\n");

    // TODO make something better than else if...
    if(remoteNode.get_addr16() == COORDINATOR_16BIT_ADDRESS) {
        DEBUG_PRINTXNL(DEBUG, "Processing Coordinator command...");
        if(data[0] == TURN_WATER_PUMP_ON) {
            DEBUG_PRINTXNL(DEBUG, "Setting water pump to true");
            waterPump = true;
        } else if (data[0] == TURN_WATER_PUMP_OFF) {
            DEBUG_PRINTXNL(DEBUG, "Setting water pump to false");
            waterPump = false;           
        } else if (data[0] == ALTERNATE_WATER_PUMP_STATE) { // debug
            DEBUG_PRINTXNL(DEBUG, "Alternating water pump state");
            waterPump = !waterPump;
        }
        DEBUG_PRINTXNL(DEBUG, "Processing Coordinator command finished!\r\n");
    } else {
        DEBUG_PRINTX(DEBUG, "Received packet from a non-Coordinator device, packet is ignored...\r\n");
    }
}

void ReadConfigFile(uint16_t *periode, uint16_t *panID, char *nodeID, char *nodeIdentifier) {
    DEBUG_PRINTXNL(DEBUG, "\r\nreading configuration file...");

    char configurationValue[BUFSIZ];
    cfg.read(CFG_PATH);

    cfg.getValue(CFG_KEY_PERIOD, configurationValue, BUFSIZ);
    *periode = atoi(configurationValue);
    DEBUG_PRINTXYNL(DEBUG, "Periode = '%ims'", *periode);

    cfg.getValue(CFG_KEY_PAN_ID, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    DEBUG_PRINTXYNL(DEBUG, "Pan ID = '0x%X'", *panID);

    cfg.getValue(CFG_KEY_NODE_ID, configurationValue, BUFSIZ);
    nodeID[0] = configurationValue[0];
    nodeID[1] = configurationValue[1];
    DEBUG_PRINTXYNL(DEBUG, "node ID = '%s'", configurationValue);

    cfg.getValue(CFG_KEY_NODE_IDENTIFIER, configurationValue, BUFSIZ);
    int i = 0;
    while(configurationValue[i] != '\0') {
        nodeIdentifier[i] = configurationValue[i];
        i++;
    }
    DEBUG_PRINTXYNL(DEBUG, "node Identifier = '%s'", nodeIdentifier);

    DEBUG_PRINTXNL(DEBUG, "Reading configuration file finished.\r\n");
}

// Not sure if it works fully, need more testing, maybe more XBees in the network?
void discovery_function(const RemoteXBeeZB &remote, char const * const node_id)
{
    const uint64_t remote64Adress = remote.get_addr64();
    DEBUG_PRINTXYNL(DEBUG, "HEY, I HAVE FOUND A DEVICE! node ID = '%s'", node_id);
    DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remote.get_addr16(), remote.is_valid_addr16b() ? "valid" : "invalid");
    DEBUG_PRINTXYZ(DEBUG, "64 bit address is:  High = '0x%X' Low = '0x%X'", UINT64_HI32(remote64Adress), UINT64_LO32(remote64Adress));
    DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remote.is_valid_addr64b() ? "valid" : "invalid");

    if (remoteNodesCount < MAX_NODES) {
        remoteNodesInNetwork[remoteNodesCount] = remote;
        remoteNodesCount++;
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Found more nodes than maximum configured, max is '%i'", MAX_NODES);
    } 
}

// Not sure if it works fully, need more testing, maybe more XBees in the network?
void DiscoverAllNodes(void) {
    DEBUG_PRINTX(DEBUG, "Discovering process started");
    xBee.start_node_discovery();
    do {
        xBee.process_rx_frames();
        wait_ms(1000);
        DEBUG_PRINTX(DEBUG, ".");
    } while(xBee.is_node_discovery_in_progress());
    DEBUG_PRINTXNL(DEBUG, "Discovering process finished!");
}

void CheckIfNewFrameIsPresent(void) {
    xBee.process_rx_frames();
}

void SetupXBee(uint16_t panID) {
    DEBUG_PRINTXNL(DEBUG, "\r\nInitiating XBee...");

    xBee.register_receive_cb(&NewFrameReceivedHandler);

    RadioStatus radioStatus = xBee.init();
    MBED_ASSERT(radioStatus == Success);
    uint64_t Adr64Bits = xBee.get_addr64();
    uint32_t highAdr = Adr64Bits >> 32;
    uint32_t lowAdr = Adr64Bits;
    DEBUG_PRINTXYZNL(DEBUG, "Primary initialization successful, device 64bit Adress = '0x%X%X'", highAdr, lowAdr);

    radioStatus = xBee.set_panid(panID);
    MBED_ASSERT(radioStatus == Success);
    DEBUG_PRINTXYNL(DEBUG, "Device PanID was set to '0x%X'", panID);

    radioStatus = xBee.set_node_identifier(nodeIdentifier);
    MBED_ASSERT(radioStatus == Success);   
    DEBUG_PRINTXYNL(DEBUG, "Device node identifier was set to '%s'", nodeIdentifier);

    DEBUG_PRINTXY(DEBUG, "Waiting for device to join the network '0x%X'", panID);
    while (!xBee.is_joined()) {
        wait_ms(1000);
        DEBUG_PRINTX(DEBUG, ".");
    }
    DEBUG_PRINTXYNL(DEBUG, "\r\ndevice joined network with panID '0x%X' successfully!\r\n", panID);

    /* Maybe drop this config if we dont use discovery feature...
    xBee.register_node_discovery_cb(&discovery_function);
    radioStatus = xBee.config_node_discovery(NODE_DISCOVERY_TIMEOUT_MS);
    MBED_ASSERT(radioStatus == Success);
    DEBUG_PRINTXYNL(DEBUG, "Configuration of node discovery successful with timeout at '%ims'", NODE_DISCOVERY_TIMEOUT_MS);
    */
    DEBUG_PRINTXNL(DEBUG, "XBee initialization finished successfully!");
}

void StartEventQueueThread(TSL2561 *tsl2561, uint16_t periode) {
    DEBUG_PRINTXYNL(DEBUG, "Starting event queue with periode '%ims'", periode);

    eventQueue.call_every(periode, ReadCaptors, tsl2561);
    eventQueue.call_every(periode, CreateDataFrame);
    eventQueue.call_every(periode, FlashLed, 3);  // just so we can see the event queue still runs
    eventQueue.call_every(100, CheckIfNewFrameIsPresent);
    eventQueueThread.start(callback(&eventQueue, &EventQueue::dispatch_forever));

    DEBUG_PRINTXNL(DEBUG, "Event queue thread started sucessfully.");
}

int main() {
    DEBUG_PRINTXNL(DEBUG, "Mobile node started");
    SetLedTo(0, true); // Init LED on

    waterPump = false;
    TSL2561 tsl2561(p9, p10);  // luminosity captor
    GetMacAddress(macAdr);
    ReadConfigFile(&periode, &panID, nodeID, nodeIdentifier);
    SetupXBee(panID);
    StartEventQueueThread(&tsl2561, periode);

    SetLedTo(0, false); // Init LED off
    while (true) {
        Thread::wait(osWaitForever);
    }
}

