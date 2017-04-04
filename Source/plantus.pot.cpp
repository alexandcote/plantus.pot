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
#define INFO_PRINTX(DEBUG, x) if(INFO) {pc.printf(x);}
#define INFO_PRINTXNL(DEBUG, x) if(INFO) {pc.printf(x);               pc.printf("\r\n");}
#define INFO_PRINTXY(DEBUG, x, y) if(INFO) {pc.printf(x, y);}
#define INFO_PRINTXYNL(DEBUG, x, y) if(INFO) {pc.printf(x, y);         pc.printf("\r\n");}
#define INFO_PRINTXYZ(DEBUG, x, y, z) if(INFO) {pc.printf(x, y, z);}
#define INFO_PRINTXYZNL(DEBUG, x, y, z) if(INFO) {pc.printf(x, y, z);  pc.printf("\r\n");}

DigitalOut waterPump(p21);
DigitalOut LEDs[4] = {
    DigitalOut(LED1), DigitalOut(LED2), DigitalOut(LED3), DigitalOut(LED4)
};
LocalFileSystem local("local");
Thread eventQueueThread;
EventQueue eventQueue(32 * EVENTS_EVENT_SIZE); // holds 32 events
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, XBEE_BAUD_RATE);

char globalOperationId[FRAME_PREFIX_LENGTH + OPERATION_ID_MAX_LENGTH];

void ReadCaptors(TSL2561 *tsl2561) {
    uint16_t readLuminosity = tsl2561->getLuminosity(CHANNEL_0);
    INFO_PRINTXYNL(INFO, "read Luminosity = '0x%X'", readLuminosity);
    uint16_t readAmbianteTemperature = ReadAmbiantTemperature(); 
    INFO_PRINTXYNL(INFO, "read Ambiant Temperature = '0x%X'", readAmbianteTemperature);
    uint16_t readSoilHumidity = ReadSoilHumidity();
    INFO_PRINTXYNL(INFO, "read Soil Humidity = '0x%X'", readSoilHumidity);
    uint16_t readWaterLevel = ReadWaterLevel(); 
    INFO_PRINTXYNL(INFO, "read Water level = '%i'", readWaterLevel);    

            
    luminosity[0] = readLuminosity >> 8;         // high luminosity
    luminosity[1] = readLuminosity & 0xFF;       // low luminosity
    ambiantTemperature[0] = readAmbianteTemperature; 
    soilHumidity[0] = readSoilHumidity;
    waterLevel[0] = readWaterLevel;

    CreateDataFrame();
}

void CreateDataFrame(void) {
    frameData[0] = FRAME_PREFIX_NEW_DATA;
    frameData[1] = luminosity[0];        // High luminosity
    frameData[2] = luminosity[1];        // Low luminosity
    frameData[3] = ambiantTemperature[0];
    frameData[4] = soilHumidity[0];
    frameData[5] = waterLevel[0]; 
    SendFrameToCoordinator(frameData, DATA_LENGTH);
}

uint16_t ReadSoilHumidity(void) {
    // TODO read analog
    return 0xFF;
}

uint16_t ReadAmbiantTemperature(void) {
    // TODO read ambiant Humidity
    return 0xAA;
}

uint16_t ReadWaterLevel(void) {
    //TODO read water level
    return 0xAB;
}

void SendFrameToCoordinator(char frame[], uint16_t frameLength) {  
    DEBUG_PRINTXNL(DEBUG, "sending frame :");
    for(int i = 0; i < frameLength; i++) {
        DEBUG_PRINTXYZNL(DEBUG, "data at index '%i' = '0x%X'", i, frame[i]);
    }
    
    TxStatus txStatus = xBee.send_data_to_coordinator((const uint8_t *)frame, frameLength);
    if (txStatus == TxStatusSuccess) {
        INFO_PRINTXNL(INFO, "Data was sent to coordinator successfully!\r\n");
    } else {
        INFO_PRINTXYNL(INFO, "Failed to send to coordinator with error '%d', look in XBee.h for more details\r\n", (int) txStatus);
    }
}

void SendPotIdentifierToCoordinator(void) {
    INFO_PRINTXNL(INFO, "Sending pot Identifier to coordinator...");
    char frame[FRAME_PREFIX_LENGTH + POT_IDENTIFIER_LENGTH];
    PrepareFrameToSend(frame, potIdentifier, FRAME_PREFIX_ADD_POT_IDENTIFIER);
    SendFrameToCoordinator(frame, strlen(frame));
}

void WaterPlant(char operationId[]) {
    INFO_PRINTXYZNL(INFO, "Going to wet plant for %ums! operation id is '%s'", pumpActivationTime, operationId);
    // make global copy to use with the event queue
    memset(globalOperationId, 0, sizeof(globalOperationId)); // start with fresh values
    strcat(globalOperationId, operationId);
    waterPump = true;
    eventQueue.call_in(pumpActivationTime, SetWaterPumpToAndNotifyCoordinator, false, globalOperationId);
}

void SetWaterPumpToAndNotifyCoordinator(bool state, char operationId[]) {
    INFO_PRINTXYZNL(INFO, "Setting water pump to '%s' and operationId is '%s'", state ? "ON" : "OFF", operationId);
    waterPump = state;
    SendCompletedOperationToCoordinator(operationId);
}

void PrepareFrameToSend(char frame[], char data[], int framePrefix) {
    memset(frame, 0, sizeof(frame)); // start with fresh values
    frame[0] = framePrefix;
    strcat(frame, data);
}

void SendCompletedOperationToCoordinator(char operationId[]) {
    INFO_PRINTXYNL(INFO, "Sending Operation id '%s' completed to coordinator", operationId);
    char frame[FRAME_PREFIX_LENGTH + OPERATION_ID_MAX_LENGTH];
    PrepareFrameToSend(frame, operationId, FRAME_PREFIX_COMPLETED_OPERATION);
    SendFrameToCoordinator(frame, strlen(frame));
}

void AlternateWaterPump(char operationId[]) {
    INFO_PRINTXNL(INFO, "Alternating water pump state");
    waterPump = !waterPump;
    SendCompletedOperationToCoordinator(operationId);
}

void NewFrameReceivedHandler(const RemoteXBeeZB &remoteNode, bool broadcast, const uint8_t *const frame, uint16_t frameLength) {
    INFO_PRINTXNL(INFO, "New frame received!");

    #if DEBUG
        uint64_t remote64Adress = remoteNode.get_addr64();
        uint32_t highAdr = remote64Adress >> 32;
        uint32_t lowAdr = remote64Adress;
        DEBUG_PRINTXYZNL(DEBUG, "\r\nGot a %s RX packet of length '%d'", broadcast ? "BROADCAST" : "UNICAST", frameLength);

        DEBUG_PRINTXYZNL(DEBUG, "16 bit remote address is: '0x%X' and it is '%s'", remoteNode.get_addr16(), remoteNode.is_valid_addr16b() ? "valid" : "invalid");
        DEBUG_PRINTXYZ(DEBUG, "64 bit remote address is: '0x%X%X'", highAdr, lowAdr);
        DEBUG_PRINTXYNL(DEBUG, "and it is '%s'", remoteNode.is_valid_addr64b() ? "valid" : "invalid");

        DEBUG_PRINTX(DEBUG, "Frame is: ");
        for (int i = 0; i < frameLength; i++)
            DEBUG_PRINTXY(DEBUG, "0x%X ", frame[i]);
        DEBUG_PRINTXNL(DEBUG, "\r\n");
    #endif

    if(remoteNode.get_addr16() == COORDINATOR_16BIT_ADDRESS) {
        DEBUG_PRINTXYNL(DEBUG, "Frame prefix = '0x%X'", frame[0]);
        switch(frame[0]) {
            char operationId[OPERATION_ID_MAX_LENGTH];
            memset(operationId, 0, sizeof(operationId)); // start with fresh values

            case FRAME_PREFIX_TURN_WATER_PUMP_ON:
                INFO_PRINTXNL(INFO, "Water plant frame detected!");
                for(int i = 1; i < frameLength; i++) 
                    operationId[i-1] = frame[i];
                INFO_PRINTXYNL(INFO, "operation id is '%s'", operationId);
                WaterPlant(operationId);
                break;

            case FRAME_PREFIX_TURN_WATER_PUMP_OFF:
                INFO_PRINTXNL(INFO, "Turn off water pump frame detected!");
                for(int i = 1; i < frameLength; i++) 
                    operationId[i-1] = frame[i];
                SetWaterPumpToAndNotifyCoordinator(false, operationId);
                break; 

            case FRAME_PREFIX_ALTERNATE_WATER_PUMP_STATE:     
                INFO_PRINTXNL(INFO, "Alternate water pump state frame detected!");       
                for(int i = 1; i < frameLength; i++) 
                    operationId[i-1] = frame[i];
                AlternateWaterPump(operationId);
                break;

            default:
                INFO_PRINTXYNL(INFO, "Unknown frame '0x%X' detected, nothing will be done!", frame[0]);
                break;
        }
        INFO_PRINTXNL(INFO, "Processing Coordinator command finished!\r\n");
    } else {
        INFO_PRINTX(INFO, "Received packet from a non-Coordinator device, packet is ignored...\r\n");
    }
}

void ReadConfigFile(uint16_t *periode, uint16_t *panID) {
    INFO_PRINTXNL(INFO, "\r\nreading configuration file...");

    char configurationValue[BUFSIZ];
    cfg.read(CFG_PATH);

    cfg.getValue(CFG_KEY_PERIOD, configurationValue, BUFSIZ);
    *periode = atoi(configurationValue);
    INFO_PRINTXYNL(INFO, "Periode = '%ims'", *periode);

    cfg.getValue(CFG_KEY_PAN_ID, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    INFO_PRINTXYNL(INFO, "Pan ID = '0x%X'", *panID);

    cfg.getValue(CFG_POT_IDENTIFIER, configurationValue, BUFSIZ);
    int i = 0;
    while(configurationValue[i] != '\0') {
        potIdentifier[i] = configurationValue[i];
        i++;
    }
    INFO_PRINTXYNL(INFO, "pot Identifier = '%s'", potIdentifier);

    INFO_PRINTXNL(INFO, "Reading configuration file finished.\r\n");
}

void CheckIfNewXBeeFrameIsPresent(void) {
    xBee.process_rx_frames();
}

void SetupXBee(uint16_t panID) {
    INFO_PRINTXNL(INFO, "\r\nInitiating XBee...");

    xBee.register_receive_cb(&NewFrameReceivedHandler);

    RadioStatus radioStatus = xBee.init();
    MBED_ASSERT(radioStatus == Success);
    uint64_t Adr64Bits = xBee.get_addr64();
    uint32_t highAdr = Adr64Bits >> 32;
    uint32_t lowAdr = Adr64Bits;
    INFO_PRINTXYZNL(INFO, "Primary initialization successful, device 64bit Adress = '0x%X%X'", highAdr, lowAdr);

    radioStatus = xBee.set_panid(panID);
    MBED_ASSERT(radioStatus == Success);
    INFO_PRINTXYNL(INFO, "Device PanID was set to '0x%X'", panID);

    INFO_PRINTXY(DEBUG, "Waiting for device to join the network '0x%X'", panID);
    while (!xBee.is_joined()) {
        wait_ms(1000);
        INFO_PRINTX(INFO, ".");
    }
    INFO_PRINTXYNL(INFO, "\r\ndevice joined network with panID '0x%X' successfully!\r\n", panID);

    INFO_PRINTXNL(INFO, "Sending pot identifier to coordinator");
    SendPotIdentifierToCoordinator();

    INFO_PRINTXNL(INFO, "XBee initialization finished successfully!\r\n");
}

void StartEventQueue(TSL2561 *tsl2561, uint16_t periode) {
    INFO_PRINTXYNL(INFO, "Starting event queue, reading captors at '%ims'\r\n", periode);

    eventQueue.call_every(periode, ReadCaptors, tsl2561);
    eventQueue.call_every(1000, FlashLed, 3);  // just so we can see the event queue still runs
    eventQueue.call_every(100, CheckIfNewXBeeFrameIsPresent);
    eventQueue.call_every(30000, SendPotIdentifierToCoordinator);
    eventQueueThread.start(callback(&eventQueue, &EventQueue::dispatch_forever));

    DEBUG_PRINTXNL(DEBUG, "Event queue thread started sucessfully!\r\n");
}

int main() {
    INFO_PRINTXNL(INFO, "Pot node started");
    SetLedTo(0, true); // Init LED on

    waterPump = false;
    TSL2561 tsl2561(p9, p10);  // luminosity captor
    ReadConfigFile(&periode, &panID);
    SetupXBee(panID);
    StartEventQueue(&tsl2561, periode);

    SetLedTo(0, false); // Init LED off
    while (true) {
        Thread::wait(osWaitForever);
    }
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