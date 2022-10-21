#include <BelaParallelComm/BelaParallelComm.h>

BelaParallelComm::BelaParallelComm(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, bool lsb, bool grayCode) {
    setup(belaId, digitalPins, nDigitals, lsb, grayCode);
}

int BelaParallelComm::setup(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, bool lsb, bool grayCode) {
    _belaId = belaId;
    _lsb
      = lsb;
    _grayCode = grayCode;
    _nDigitals = nDigitals;
    _digitalPins.resize(nDigitals);
    for (unsigned int i = 0; i < nDigitals; i++)
        _digitalPins[i] = digitalPins[i];
    _dataBuffer.resize(nDigitals);

    return 0;
}

void BelaParallelComm::prepareTx(BelaContext* context, unsigned int n) {
    for (unsigned int i = 0; i < _nDigitals; i++)
        pinMode(context, n, _digitalPins[i], OUTPUT);
    _ready = true;
    _mode = TX;
}

void BelaParallelComm::prepareRx(BelaContext* context, unsigned int n) {
    for (unsigned int i = 0; i < _nDigitals; i++)
        pinMode(context, n, _digitalPins[i], INPUT);
    _mode = RX;
}

int BelaParallelComm::prepareDataToSend(unsigned int data) {
    if (!_ready)
        return -1;
    intToBitArray(data, _dataBuffer.data(), _dataBuffer.size(), _lsb, _grayCode);
    return 0;
}

void BelaParallelComm::intToBitArray(int value, unsigned int* bitArray, int nBits, bool lsb, bool grayCode) {
    unsigned int val = grayCode ? grayToBinary(value) : value;
    for (unsigned int i = 0; i < nBits; i++) {
        unsigned int idx = (lsb) ? nBits - i - 1 : i;
        bitArray[idx] = (val >> i) & 1;
    }
}

unsigned int BelaParallelComm::bitArrayToInt(unsigned int* bitArray, int nBits, bool lsb, bool grayCode) {
    int ret = 0;
    for (unsigned int i = 0; i < nBits; i++) {
        unsigned int idx = (lsb) ? nBits - i - 1 : i;
        ret += (1 << i) * bitArray[idx];
    }
    if (grayCode)
        ret = binaryToGray(ret);
    return ret;
}

int BelaParallelComm::grayToBinary(int value) {
    return value ^ (value >> 1);
}

int BelaParallelComm::binaryToGray(int value) {
    int ret = value;
    while (value > 0) {
        value >>= 1;
        ret ^= value;
    }
    return ret;
}

void BelaParallelComm::sendData(BelaContext* context, unsigned int n, bool persistent) {
    // Unset ready flag
    _ready = false;

    // Write data
    for (unsigned int i = 0; i < _nDigitals; i++) {
        if (persistent)
            digitalWrite(context, n, _digitalPins[i], _dataBuffer[i]);
        else
            digitalWriteOnce(context, n, _digitalPins[i], _dataBuffer[i]);
    }

    _ready = true;
}

void BelaParallelComm::readData(BelaContext* context, unsigned int n) {
    _hasChanged = false;

    // Unset ready flag
    _ready = false;

    // Read data
    for (unsigned int i = 0; i < _nDigitals; i++) {
        unsigned int dRead = digitalRead(context, n, _digitalPins[i]);
        if (dRead != _dataBuffer[i]) _hasChanged = true;
        _dataBuffer[i] = dRead;
    }

    _ready = true;
}

void BelaParallelComm::printBuffers(bool rt) {
    if (rt) {
        rt_printf("-- %s --\n", _belaId.c_str());

        rt_printf("Data Buffer [%d]:", getBufferVal());
        for (unsigned int i = 0; i < _dataBuffer.size(); i++)
            rt_printf(" %d", _dataBuffer[i]);
        rt_printf("\n");
    } else {
        printf("Data Buffer [%d]:", getBufferVal());
        for (unsigned int i = 0; i < _dataBuffer.size(); i++)
            printf(" %d", _dataBuffer[i]);
        printf("\n");
    }
}

void BelaParallelComm::printDetails() {
    printf("Digital pins [%d]: \n", _nDigitals);
    for (unsigned int i = 0; i < _digitalPins.size(); i++)
        printf(" %d", _digitalPins[i]);
    printf("\n");
    printf("Number of data bits: %d\n", getNumDataBits());
    printf("Max val: %d\n", getMaxDataVal());
}
