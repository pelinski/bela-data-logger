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
    BelaParallelComm(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, bool lsb = true, bool grayCode = false);
    int setup(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, bool lsb = true, bool grayCode = false);
    ~BelaParallelComm(){};

    void prepareTx(BelaContext* context, unsigned int n = 0);
    void prepareRx(BelaContext* context, unsigned int n = 0);

    bool isReady() { return _ready; };
    bool hasChanged() { return _hasChanged; };

    int prepareDataToSend(unsigned int data);

    void intToBitArray(int value, unsigned int* bitArray, int nBits, bool lsb, bool grayCode);

    unsigned int bitArrayToInt(unsigned int* bitArray, int nBits, bool lsb, bool grayCode);

    int grayToBinary(int value);
    int binaryToGray(int value);

    void sendData(BelaContext* context, unsigned int n = 0, bool persistent = false);

    void readData(BelaContext* context, unsigned int n);

    void printBuffers(bool rt = false);

    void printDetails();

    int getBufferVal() { return (_ready) ? bitArrayToInt(_dataBuffer.data(), _dataBuffer.size(), _lsb, _grayCode) : -1; };

    unsigned int getNumDataBits() { return _nDigitals; };
    unsigned int getMaxDataVal() { return pow(2.0, getNumDataBits()) - 1; };

  private:
    std::string _belaId;

    std::vector<unsigned int> _digitalPins;
    std::vector<unsigned int> _dataBuffer;
    unsigned int _nDigitals;
    bool _ready = false;
    bool _hasChanged = false;
    bool _lsb = true;
    bool _grayCode = false;
    Mode _mode = UNKNOWN;
};
