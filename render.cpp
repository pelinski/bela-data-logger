
#include <libraries/WriteFile/WriteFile.h>
#include <vector>
#include "BelaParallelComm/BelaParallelComm.h"
// #include "WriteFile/WriteFile.h"

#ifndef BELA_MASTER
#define BELA_MASTER 0
#endif

bool gBelaIsMaster = (bool)BELA_MASTER;

std::vector<unsigned int> gCommPins{ 0, 1, 2, 3 };

BelaParallelComm parallelComm;

const unsigned int kNumBlocksComm = 1;  // This example works only for one block of data, needs revision for multiple blocks
const unsigned int kHeaderSizeComm = 0; // No header, all data

unsigned int gCommBlockCount = 0;
unsigned int gCommCount = 0;
unsigned int gCommBlockSpan = 689; // ~ 0.25 sec @44.1k (16 block size)

WriteFile dataLog;
WriteFile syncLog;

/*** SETUP ***/
bool setup(BelaContext* context, void* userData) {

    /** Setup data logger**/

    if ((!context->flags) & BELA_FLAG_INTERLEAVED) {
        fprintf(stderr, "DataLogger requires interleaved buffers\n");
        return false;
    }
    dataLog.setup("data.log");  // set the file name to write to
    dataLog.setEchoInterval(0); // only print to the console every 10000 calls to log
    dataLog.setFileType(kBinary);
    dataLog.setHeader("");
    dataLog.setFooter("");
    // dataLog.setFormat("%.0f %.4f %.4f %.4f %.4f\n"); // Output format. Use only %f (with modifiers). When in binary mode, this is used only for echoing the console.

    syncLog.setup("sync.log");
    syncLog.setEchoInterval(0);
    syncLog.setFileType(kBinary); // kText is not working properly in rx -- it misses the second logged value
    syncLog.setHeader("");
    syncLog.setFooter("");
    // syncLog.setFormat("%.0f %.0f\n");

    /**Setup parallel communication**/

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

/*** RENDER ***/
void render(BelaContext* context, void* userData) {
    for (unsigned int n = 0; n < context->analogFrames; n++) { // ! analogFrames here instead of audioFrames

        // Timestamp
        unsigned int framesElapsed = n + context->audioFramesElapsed;

        //**Log timestamp and sensor values into dataLog**//
        dataLog.log(framesElapsed);                                      // timestamp
        dataLog.log(&(context->analogIn[n * context->analogFrames]), 4); // sensor values

        //**Parallel communication**//
        // If Bela is master..
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
        } else { // If Bela is receiver...
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
