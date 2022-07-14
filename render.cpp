#include <vector>
#include "BelaParallelComm/BelaParallelComm.h"

#ifndef BELA_MASTER
#define BELA_MASTER 0
#endif

#define VERBOSE 1

bool gBelaIsMaster = (bool)BELA_MASTER;

std::vector<unsigned int> gCommPins{ 0, 1, 2, 3 };

BelaParallelComm parallelComm;

const unsigned int kNumBlocksComm = 1;  // This example works only for one block of data, needs revision for
                                        // multiple blocks
const unsigned int kHeaderSizeComm = 0; // No header, all data

unsigned int gCommBlockCount = 0;
unsigned int gCommCount = 0;
unsigned int gCommBlockSpan = 689; // ~ 0.25 sec @44.1k (16 block size)

// Sensor analog pin
// unsigned int gSensorPin = 0;

/*** SETUP ***/
bool setup(BelaContext* context, void* userData) {
    // If Bela is master...
    if (gBelaIsMaster) {

        parallelComm.setup(gCommPins.data(), gCommPins.size(),
                           /* headerSize */ kHeaderSizeComm,
                           /* nBlocks */ kNumBlocksComm);
        parallelComm.printDetails();
        parallelComm.prepareTx(context, 0);
        parallelComm.printBuffers();
    }
    // If Bela is not master...
    else {

        parallelComm.setup(gCommPins.data(), gCommPins.size(),
                           /* headerSize */ kHeaderSizeComm,
                           /* nBlocks */ kNumBlocksComm);
        parallelComm.printDetails();
        parallelComm.prepareRx(context, 0);
        parallelComm.printBuffers();
    }

    // // Check if analog channels are enabled --> set AnalogChannels to 4 in the IDE
    // if (context->analogFrames == 0 || context->analogFrames > context->audioFrames) {
    //     rt_printf("Error: this example needs analog enabled, with 4 channels\n");
    //     return false;
    // }

    return true;
}

/*** RENDER ***/
void render(BelaContext* context, void* userData) {

    /* AUDIO LOOP */
    for (unsigned int n = 0; n < context->audioFrames; n++) {

        // Read sensor value
        // float sensorValue = analogRead(context, n, gSensorPin);

        // If Bela is master..
        if (gBelaIsMaster) {
            // If first frame (beginning of block) and specific number of blocks have elapsed...
            if (n == 0 && ++gCommBlockCount > gCommBlockSpan) {
                gCommBlockCount = 0;                           // reset block count
                parallelComm.prepareDataToSend(0, gCommCount); // write count to buffer
#ifdef VERBOSE

                // unsigned int samplesElapsed = n + context->audioFramesElapsed;
                // rt_printf("---- samples elapsed = %d ----\n", samplesElapsed);
                // rt_printf("---- sensor value = %d ----\n", sensorValue);
                parallelComm.printBuffers(true);
#endif                                                           /* VERBOSE */
                parallelComm.sendData(context, n);               // send buffer
                if (++gCommCount > parallelComm.getMaxDataVal()) // increment count and check if it has changed
                    gCommCount = 0;
            }
        } else {
            parallelComm.readData(context, n);
            if (parallelComm.isReady() && parallelComm.hasChanged()) {
#ifdef VERBOSE
                unsigned int samplesElapsed = n + context->audioFramesElapsed;
                rt_printf("---- samples elapsed = %d ----\n", samplesElapsed);
                // rt_printf("---- sensor value = %f ----\n", sensorValue);

                parallelComm.printBuffers(true);
#endif /* VERBOSE */
                if (parallelComm.getBufferVal() >= -1)
                    gCommCount = parallelComm.getBufferVal();
            }
        }
    }
    /* END OF AUDIO LOOP */
}

void cleanup(BelaContext* context, void* userData) {}
