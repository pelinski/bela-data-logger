#pragma once
#include <Bela.h>
#include <math.h>
#include <string>
#include <vector>

/* 
 * Small library for handling a parallel digital communication between two Bela devices using
 * the Bela digital GPIO API.
 *
 * Januray 2022
 * Author: Adan L. Benito
 *
 */

class BelaParallelComm {
  public:
    typedef enum {
        UNKNOWN = -1,
        RX = 0,
        TX = 1
    } Mode;

    BelaParallelComm(){};
    BelaParallelComm(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, unsigned int headerSize = 2, unsigned int nBlocks = 1, bool lsb = true);
    int setup(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, unsigned int headerSize = 2, unsigned int nBlocks = 1, bool lsb = true);
    ~BelaParallelComm(){};

    void prepareTx(BelaContext* context, unsigned int n = 0);
    void prepareRx(BelaContext* context, unsigned int n = 0);

    bool isReady() { return _ready; };
    bool hasChanged() { return _hasChanged; };

    int prepareDataToSend(unsigned int header, unsigned int data);

    void intToBitArray(int value, unsigned int* bitArray, int nBits, bool lsb = true);

    unsigned int bitArrayToInt(unsigned int* bitArray, int nBits, bool lsb = true);

    void sendData(BelaContext* context, unsigned int n = 0, bool persistent = false);

    void readData(BelaContext* context, unsigned int n);

    void printBuffers(bool rt = false);

    void printDetails();

    int getHeaderVal() { return (_ready) ? bitArrayToInt(_dataHeader.data(), _dataHeader.size(), _lsb) : -1; };
    int getBufferVal() { return (_ready) ? bitArrayToInt(_dataBuffer.data(), _dataBuffer.size(), _lsb) : -1; };

    unsigned int getNumDataBits() { return _nDigitals * _nBlocks - _headerSize - _nBlocks; };
    unsigned int getMaxDataVal() { return pow(2.0, getNumDataBits()) - 1; };

  private:
    std::string _belaId;
    std::vector<unsigned int> _digitalPins;
    std::vector<unsigned int> _dataBuffer;
    std::vector<unsigned int> _dataHeader;
    unsigned int _nDigitals;
    unsigned int _headerSize;
    unsigned int _nBlocks;
    unsigned int _flagBit = 0;
    int _blockCount = -1;
    bool _ready = false;
    bool _hasChanged = false;
    bool _lsb = true;
    Mode _mode = UNKNOWN;
};
