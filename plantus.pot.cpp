#include "plantus.station.mobile.h"
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
ConfigFile cfg;
EventQueue eventQueue(32 * EVENTS_EVENT_SIZE); // holds 32 events
Thread thread;
XBeeZB xBee = XBeeZB(p13, p14, p8, NC, NC, BAUD_RATE);

char frameData[DATA_LENGTH];

// TODO more dynamic?
void ReadCaptors(TSL2561 *tsl2561) {
    uint16_t luminosity = tsl2561->getLuminosity(CHANNEL_0);
    DEBUG_PRINTXYNL(DEBUG, "Luminosity = %i", luminosity);
    
    frameData[2] = readSoilHumidity();          
    frameData[3] = luminosity >> 8;         // high luminosity
    frameData[4] = luminosity & 0xFF;       // low luminosity
    frameData[5] = readAmbiantHumidity();    
    frameData[6] = waterPump;               // pump state

    sendDataToCoordinator(frameData);
}

uint16_t readSoilHumidity(void) {
    // TODO read analog
    return 0xFF;
}

uint16_t readAmbiantHumidity(void) {
    // TODO read ambiant Humidity
    return 0xAA;
}

void setWaterPumpTo(bool state) {
    //TODO activate pump until soil humidity is good or for a set time?
    // waterPump = state;
    waterPump = !waterPump; // for test
}

void FlashLED(uint16_t led) {
    LEDs[led] = !LEDs[led];
}

void sendDataToCoordinator(char data[])
{
    #if DEBUG
    DEBUG_PRINTXNL(DEBUG, "sending data :");
    for(int i = 0; i < DATA_LENGTH; i++) {
        DEBUG_PRINTXYZNL(DEBUG, "data at index %i is: 0x%X", i, data[i]);
    }
    #endif

    TxStatus txStatus = xBee.send_data_to_coordinator((const uint8_t *)data, DATA_LENGTH);
    if (txStatus == TxStatusSuccess) {
        DEBUG_PRINTXNL(DEBUG, "Data was sent to coordinator successfully!");
    } else {
        DEBUG_PRINTXYNL(DEBUG, "Failed to send to coordinator with error %d", (int) txStatus);
    }
}

void readConfigFile(uint16_t *periode, uint16_t *panID) {
    DEBUG_PRINTXNL(DEBUG, "reading configuration file...");
    char configurationValue[BUFSIZ];
    cfg.read(cfgPath);

    cfg.getValue(periodeKey, configurationValue, BUFSIZ);
    *periode = atoi(configurationValue);
    DEBUG_PRINTXYNL(DEBUG, "Periode: %ims", *periode);

    cfg.getValue(panIDKey, configurationValue, BUFSIZ);
    *panID = strtol(configurationValue, NULL, HEXA_BASE);
    DEBUG_PRINTXYNL(DEBUG, "Pan ID: 0x%X", *panID);

    cfg.getValue(nodeIdKey, configurationValue, BUFSIZ);
    frameData[0] = configurationValue[0];
    frameData[1] = configurationValue[1];
    DEBUG_PRINTXYNL(DEBUG, "node ID: %s", configurationValue);
    DEBUG_PRINTXNL(DEBUG, "Reading configuration file finished.");
}

void setupXBee(uint16_t panID) {
    DEBUG_PRINTXNL(DEBUG, "Initiating XBee...");
    RadioStatus radioStatus = xBee.init();
    MBED_ASSERT(radioStatus == Success);

    radioStatus = xBee.set_panid(panID);
    MBED_ASSERT(radioStatus == Success);

    DEBUG_PRINTXYNL(DEBUG, "Initialization successful, waiting for device to join the network with panID 0x%X", panID);
    while (!xBee.is_joined()) {
        wait_ms(1000);
        DEBUG_PRINTX(DEBUG, ".");
    }
    DEBUG_PRINTXNL(DEBUG, "\r\ndevice joined network successfully!");
}

void startCaptorsThread(TSL2561 *tsl2561, uint16_t periode) {
    DEBUG_PRINTXYNL(DEBUG, "Starting thread with periode %ims", periode);

    eventQueue.call_every(periode, ReadCaptors, tsl2561);
    eventQueue.call_every(periode, FlashLED, 3);  // just so we can see the mbed still runs
    eventQueue.call_every(33, setWaterPumpTo, true);   // test
    thread.start(callback(&eventQueue, &EventQueue::dispatch_forever));

    DEBUG_PRINTXNL(DEBUG, "Thread started sucessfully.");
}

int main() {
    DEBUG_PRINTXNL(DEBUG, "Mobile node started");
    LEDs[0] = 1; // Init LED
    TSL2561 tsl2561(p9, p10);  // luminosity captor

    readConfigFile(&periode, &panID);
    setupXBee(panID);
    startCaptorsThread(&tsl2561, periode);
    LEDs[0] = 0; // Init LED
    while (true) {
        Thread::wait(osWaitForever);
    }
}

