#include "BelaParallelComm/BelaParallelComm.h"
#include <vector>

#define BELA_MASTER 0
#define VERBOSE

bool gBelaIsMaster = false;
#if BELA_MASTER == 1
gBelaIsMaster = true;
#endif

std::vector<unsigned int> gCommPins{1, 2, 3, 4};

BelaParallelComm parallelComm;

const unsigned int kNumBlocksComm = 1;	// This example works only for one block of data, needs revision for
					// multiple blocks
const unsigned int kHeaderSizeComm = 0; // No header, all data

unsigned int gCommBlockCount = 0;
unsigned int gCommCount = 0;
unsigned int gCommBlockSpan = 689; // ~ 0.25 sec @44.1k (16 block size)

/*** SETUP ***/
bool setup(BelaContext *context, void *userData) {
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

	return true;
}

/*** RENDER ***/
void render(BelaContext *context, void *userData) {

	/* AUDIO LOOP */
	for (unsigned int n = 0; n < context->audioFrames; n++) {
		// If Bela is master..
		if (gBelaIsMaster) {
			// If first frame (beginning of block) and specific number of blocks have
			// elapsed...
			if (n == 0 && ++gCommBlockCount > gCommBlockSpan) {
				gCommBlockCount = 0;			       // reset block count
				parallelComm.prepareDataToSend(0, gCommCount); // write count to buffer
#ifdef VERBOSE
				parallelComm.printBuffers(true);
#endif										 /* VERBOSE */
				parallelComm.sendData(context, n);		 // send buffer
				if (++gCommCount > parallelComm.getMaxDataVal()) // increment count and check if it has changed
					gCommCount = 0;
			}
		} else {
			parallelComm.readData(context, n);
			if (parallelComm.isReady() && parallelComm.hasChanged()) {
#ifdef VERBOSE
				rt_printf("---- n = %d ----\n", n);
				parallelComm.printBuffers(true);
#endif /* VERBOSE */
				if (parallelComm.getBufferVal() >= -1)
					gCommCount = parallelComm.getBufferVal();
			}
		}
	}
	/* END OF AUDIO LOOP */
}

void cleanup(BelaContext *context, void *userData) {}
