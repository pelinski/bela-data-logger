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
#define BELA_ID 0
#endif

#ifndef NUM_INPUTS
#define NUM_INPUTS 2
#endif

// Config: Bela role, Id, and number of Analog pins to log
bool gBelaIsMaster = (bool)BELA_MASTER;
std::string gBelaId;
unsigned int gNumAnalogPins = (unsigned int) NUM_INPUTS; // Number of analog pins used (number of sensors connected to the Bela, assumes they are connected in ascending order, e.g., if gNumAnalogPins is 5, then pins 0, 1, 2, 3, 4 are used)

// BelaParallelComm config 
BelaParallelComm parallelComm;
std::vector<unsigned int>
  gCommPins = { 10 }; // Digital pins to be connected between TX and RXs
unsigned int gCommBlockCount = 0;
unsigned int gCommCount = 1;       // starts at 1 because otherwise it does not detect the first bit
unsigned int gCommBlockSpan = 689; // ~ 0.25 sec @44.1k (16 block size)

// WriteFile config
WriteFile sensorLog;
WriteFile syncLog;

unsigned int gAudioFramesPerAnalogFrame;

bool setup(BelaContext* context, void* userData) {

    if (context->analogFrames)
        gAudioFramesPerAnalogFrame
          = context->audioFrames / context->analogFrames;

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
    sensorLog.setEchoInterval(0);
    sensorLog.setFileType(kBinary);
    sensorLog.setHeader("");
    sensorLog.setFooter("");

    syncLog.setup((gBelaId + "-sync.log").c_str());
    syncLog.setEchoInterval(0);
    syncLog.setFileType(kBinary);
    syncLog.setHeader("");
    syncLog.setFooter("");

    // Setup parallel communication

    parallelComm.setup(gBelaId, gCommPins.data(), gCommPins.size(), true, false);
    parallelComm.printDetails();

    if (gBelaIsMaster) {
        parallelComm.prepareTx(context, 0);
    } else {
        parallelComm.prepareRx(context, 0);
    }

    return true;
}

void render(BelaContext* context, void* userData) {
    for (unsigned int nAudioFrames = 0; nAudioFrames < context->audioFrames; nAudioFrames++) {
        // Convert nAudioFrames (which we are looping over) to nAnalogFrames, used for reading the sensor values
        unsigned int nAnalogFrames = nAudioFrames / gAudioFramesPerAnalogFrame;
        // Timestamp
        unsigned int analogFramesElapsed = (nAudioFrames + context->audioFramesElapsed) / gAudioFramesPerAnalogFrame;

        // only log sensor data when it's read (at each analogFrame)
        if (nAudioFrames % gAudioFramesPerAnalogFrame == gAudioFramesPerAnalogFrame - 1) {
            //Log timestamp and sensor values into sensorLog
            sensorLog.log(analogFramesElapsed);                                                         // timestamp
            sensorLog.log(&(context->analogIn[nAnalogFrames * context->analogFrames]), gNumAnalogPins); // sensor values
        }
        //Parallel communication
        if (gBelaIsMaster) {
            // If first frame (beginning of block) and specific number of blocks have elapsed...
            if (nAudioFrames == 0 && ++gCommBlockCount > gCommBlockSpan) {
                gCommBlockCount = 0;                                // reset block count
                parallelComm.prepareDataToSend(gCommCount);         // write count to buffer
                parallelComm.sendData(context, nAudioFrames, true); // send buffer. digital data is written and read at audioFrameRate. persistent needs to be set as true, otherwise a 0 is read after every bit
                parallelComm.printBuffers(true);

                // Log every timestamp and block sent to syncLog
                syncLog.log(analogFramesElapsed);
                syncLog.log(gCommCount);

                rt_printf("analogFramesElapsed = %d --\n", analogFramesElapsed);

                if (++gCommCount > parallelComm.getMaxDataVal()) // increment count and check if it has changed
                    gCommCount = 0;
            }
        } else {
            parallelComm.readData(context, nAudioFrames); // digital data is written and read at
            if (parallelComm.isReady() && parallelComm.hasChanged()) {
                gCommCount = parallelComm.getBufferVal();
                parallelComm.printBuffers(true);

                // Log timestamp and block received to syncLog
                syncLog.log(analogFramesElapsed);
                syncLog.log(gCommCount);

                rt_printf("analogFramesElapsed = %d --\n", analogFramesElapsed);
            }
        }
    }
}

void cleanup(BelaContext* context, void* userData) {}
