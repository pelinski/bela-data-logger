// Based on @adanlbenito example for the BelaParallelComm library
#include <libraries/WriteFile/WriteFile.h>
#include <string>
#include <vector>
#include "BelaParallelComm/BelaParallelComm.h"

// Cpp flags default values
#ifndef BELA_MASTER
#define BELA_MASTER 0
#endif

#ifndef BELA_ID
#if BELA_MASTER
#define BELA_ID 0
#else
#define BELA_ID 1
#endif
#endif

#ifndef DIGITAL_PINS
#define DIGITAL_PINS \
    { 0, 1 }
#endif

#ifndef NUM_ANALOG_PINS
#define NUM_ANALOG_PINS 5
#endif

bool gBelaIsMaster = (bool)BELA_MASTER;
std::string gBelaId;

unsigned int _gCommPins[2] = DIGITAL_PINS;
  std::vector<unsigned int>
    gCommPins; // Digital pins to be connected between TX and RXs

int gNumAnalogPins = (int)NUM_ANALOG_PINS; // Number of analog pins used (number of sensors connected to the Bela, assumes they are connected in ascending order, e.g., if gNumAnalogPins is 5, then pins 0, 1, 2, 3, 4 are used)

BelaParallelComm parallelComm;

const unsigned int kNumBlocksComm = 1;  // This example works only for one block of data, needs revision for multiple blocks
const unsigned int kHeaderSizeComm = 0; // No header, all data

unsigned int gCommBlockCount = 0;
unsigned int gCommCount = 0;
unsigned int gCommBlockSpan = 689; // ~ 0.25 sec @44.1k (16 block size)

WriteFile sensorLog;
WriteFile syncLog;

int gAudioFramesPerAnalogFrame;

bool setup(BelaContext* context, void* userData) {

    gCommPins = { _gCommPins[0], _gCommPins[1] }; // cast into std::vector<unsigned int>

    if (context->analogFrames)
        gAudioFramesPerAnalogFrame = context->audioFrames / context->analogFrames;

    // Setup data logger

    if ((!context->flags) & BELA_FLAG_INTERLEAVED) {
        fprintf(stderr, "data-logger requires interleaved buffers\n");
        return false;
    }

    if (gBelaIsMaster) {
        gBelaId = "TX" + std::to_string((unsigned int)BELA_ID);
    } else {
        gBelaId = "RX" + std::to_string((unsigned int)BELA_ID);
    }

    sensorLog.setup((gBelaId + "-data.log").c_str()); // set the file name to write to
    sensorLog.setEchoInterval(0);                     // only print to the console every 10000 calls to log
    sensorLog.setFileType(kBinary);
    sensorLog.setHeader("");
    sensorLog.setFooter("");

    syncLog.setup((gBelaId + "-sync.log").c_str());
    syncLog.setEchoInterval(0);
    syncLog.setFileType(kBinary);
    syncLog.setHeader("");
    syncLog.setFooter("");

    // Setup parallel communication

    parallelComm.setup(gCommPins.data(), gCommPins.size(), kHeaderSizeComm, kNumBlocksComm);
    parallelComm.printDetails();
    // parallelComm.printBuffers(true);

    if (gBelaIsMaster) {
        parallelComm.prepareTx(context, 0);
    } else {
        parallelComm.prepareRx(context, 0);
    }

    return true;
}

void render(BelaContext* context, void* userData) {
    for (unsigned int n = 0; n < context->analogFrames; n++) { // ! analogFrames here instead of audioFrames

        // Timestamp
        unsigned int framesElapsed = n + context->audioFramesElapsed / gAudioFramesPerAnalogFrame; // convert audio to analog frames elapsed

        //Log timestamp and sensor values into sensorLog
        sensorLog.log(framesElapsed);                                                   // timestamp
        sensorLog.log(&(context->analogIn[n * context->analogFrames]), gNumAnalogPins); // sensor values

        //Parallel communication
        if (gBelaIsMaster) {
            // If first frame (beginning of block) and specific number of blocks have elapsed...
            if (n == 0 && ++gCommBlockCount > gCommBlockSpan) {
                gCommBlockCount = 0;                           // reset block count
                parallelComm.prepareDataToSend(0, gCommCount); // write count to buffer
                parallelComm.sendData(context, n);             // send buffer
                parallelComm.printBuffers(true);

                // Log every timestamp and block sent to syncLog
                syncLog.log(framesElapsed);
                syncLog.log(gCommCount);

                rt_printf("master -- %d, %d\n", framesElapsed, gCommCount);

                if (++gCommCount > parallelComm.getMaxDataVal()) // increment count and check if it has changed
                    gCommCount = 0;
            }
        } else {
            parallelComm.readData(context, n);
            if (parallelComm.isReady() && parallelComm.hasChanged() && (parallelComm.getBufferVal() >= -1)) {
                gCommCount = parallelComm.getBufferVal();
                parallelComm.printBuffers(true);

                // Log timestamp and block received to syncLog
                syncLog.log(framesElapsed);
                syncLog.log(gCommCount);

                rt_printf("rx -- %d, %d\n", framesElapsed, gCommCount);
            }
        }
    }
}

void cleanup(BelaContext* context, void* userData) {}
