#include <BelaParallelComm/BelaParallelComm.h>

BelaParallelComm::BelaParallelComm(unsigned int* digitalPins, unsigned int nDigitals, unsigned int headerSize, unsigned int nBlocks, bool lsb) {
    setup(digitalPins, nDigitals, headerSize, nBlocks);
}

int BelaParallelComm::setup(unsigned int* digitalPins, unsigned int nDigitals, unsigned int headerSize, unsigned int nBlocks, bool lsb) {
    _lsb = lsb;
    _nDigitals = nDigitals;
    _digitalPins.resize(nDigitals);
    for (unsigned int i = 0; i < nDigitals; i++)
        _digitalPins[i] = digitalPins[i];
    _headerSize = (headerSize < nDigitals - 1) ? headerSize : 2;
    _nBlocks = (nBlocks > 0) ? nBlocks : 1;

    _dataHeader.resize(headerSize);
    _dataBuffer.resize(nDigitals * nBlocks - headerSize - nBlocks);

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

int BelaParallelComm::prepareDataToSend(unsigned int header, unsigned int data) {
    if (!_ready)
        return -1;
    intToBitArray(header, _dataHeader.data(), _dataHeader.size(), _lsb);
    intToBitArray(data, _dataBuffer.data(), _dataBuffer.size(), _lsb);
    _blockCount = 0;
    return 0;
}

void BelaParallelComm::intToBitArray(int value, unsigned int* bitArray, int nBits, bool lsb) {
    for (unsigned int i = 0; i < nBits; i++) {
        unsigned int idx = (lsb) ? nBits - i - 1 : i;
        bitArray[idx] = (value >> i) & 1;
    }
}

unsigned int BelaParallelComm::bitArrayToInt(unsigned int* bitArray, int nBits, bool lsb) {
    int ret = 0;
    for (unsigned int i = 0; i < nBits; i++) {
        unsigned int idx = (lsb) ? nBits - i - 1 : i;
        ret += (1 << i) * bitArray[idx];
    }
    return ret;
}

void BelaParallelComm::sendData(BelaContext* context, unsigned int n, bool persistent) {
    // If there are any blocks left to send
    if (_blockCount >= 0 && _blockCount <= _nBlocks - 1) {
        // Unset ready flag
        if (_nBlocks != 1)
            _ready = false;

        unsigned int lastIndex = 0;
        // If first block...
        if (_blockCount == 0) {
            // Set last digital to 1
            if (persistent)
                digitalWrite(context, n, _digitalPins[lastIndex], 1);
            else
                digitalWriteOnce(context, n, _digitalPins[lastIndex], 1);
            // Write header
            for (unsigned int i = 0; i < _headerSize; i++) {
                if (persistent)
                    digitalWrite(context, n, _digitalPins[++lastIndex], _dataHeader[i]);
                else
                    digitalWriteOnce(context, n, _digitalPins[++lastIndex], _dataHeader[i]);
            }
        } else {
            // Set last digital to 0
            if (persistent)
                digitalWrite(context, n, _digitalPins[lastIndex], 0);
            else
                digitalWriteOnce(context, n, _digitalPins[lastIndex], 0);
        }
        // Write data
        for (unsigned int i = lastIndex + 1; i < _nDigitals; i++) {
            unsigned int dataIndex = i - (lastIndex + 1) + (_blockCount * (_nDigitals - 1));
            if (_blockCount != 0) dataIndex -= _headerSize;
            if (persistent)
                digitalWrite(context, n, _digitalPins[i], _dataBuffer[dataIndex]);
            else
                digitalWriteOnce(context, n, _digitalPins[i], _dataBuffer[dataIndex]);
        }
        _blockCount++;
    } else // If no blocks left
    {
        // Set block count to -1
        _blockCount = -1;
        // Set ready flag, new data can be sent/received
        _ready = true;
    }
}

void BelaParallelComm::readData(BelaContext* context, unsigned int n) {
    unsigned int lastIndex = 0;
    _hasChanged = false;
    // Read first digital
    unsigned int flagBit = digitalRead(context, n, _digitalPins[lastIndex]);
    if (flagBit != _flagBit) _hasChanged = true;
    _flagBit = flagBit;

    // If first digital is 1...
    if (_flagBit == 1) {
        // ... first block
        _blockCount = 0;
        _ready = false;

        // Read header
        for (unsigned int i = 0; i < _headerSize; i++) {
            unsigned int dRead = digitalRead(context, n, _digitalPins[++lastIndex]);
            if (dRead != _dataHeader[i]) _hasChanged = true;
            _dataHeader[i] = dRead;
        }
        // Read data
        for (unsigned int j = 0; j < _nDigitals - 1 - _headerSize; j++) {
            unsigned int dRead = digitalRead(context, n, _digitalPins[++lastIndex]);
            if (dRead != _dataBuffer[j]) _hasChanged = true;
            _dataBuffer[j] = dRead;
        }
        // Increment block counter
        ++_blockCount;
    } else {
        // If is not first block...
        if (_blockCount > 0) {
            // Read data
            unsigned int startIndex = _nDigitals - 1 - _headerSize;
            startIndex += (_blockCount - 1) * _nDigitals;
            for (unsigned int i = 0; i < _nDigitals; i++) {
                unsigned int dRead = digitalRead(context, n, _digitalPins[++lastIndex]);
                if (dRead != _dataBuffer[startIndex + i]) _hasChanged = true;
                _dataBuffer[startIndex + i] = dRead;
            }

            ++_blockCount;
            if (_blockCount >= _nBlocks) {
                _ready = true;
                _blockCount = -1;
            }
        }
    }
}

void BelaParallelComm::printBuffers(bool rt) {
    if (rt) {
        if (_mode == TX) {
            rt_printf("--TX--\n");
        } else if (_mode == RX) {
            rt_printf("--RX--\n");
        }
        rt_printf("Data Header [%d]:", getHeaderVal());
        for (unsigned int i = 0; i < _dataHeader.size(); i++)
            rt_printf(" %d", _dataHeader[i]);
        rt_printf("\n");

        rt_printf("Data Buffer [%d]:", getBufferVal());
        for (unsigned int i = 0; i < _dataBuffer.size(); i++)
            rt_printf(" %d", _dataBuffer[i]);
        rt_printf("\n");
        rt_printf("--\n");
    } else {
        if (_mode == TX) {
            printf("--TX--\n");
        } else if (_mode == RX) {
            printf("--RX--\n");
        }
        printf("Data Header [%d]:", getHeaderVal());
        for (unsigned int i = 0; i < _dataHeader.size(); i++)
            printf(" %d", _dataHeader[i]);
        printf("\n");

        printf("Data Buffer [%d]:", getBufferVal());
        for (unsigned int i = 0; i < _dataBuffer.size(); i++)
            printf(" %d", _dataBuffer[i]);
        printf("\n");
        printf("--\n");
    }
}

void BelaParallelComm::printDetails() {
    printf("Digital pins [%d]: \n", _nDigitals);
    for (unsigned int i = 0; i < _digitalPins.size(); i++)
        printf(" %d", _digitalPins[i]);
    printf("\n");
    printf("Header size (bits): %d\n", _headerSize);
    printf("Number of blocks: %d\n", _nBlocks);
    printf("Number of data bits: %d\n", getNumDataBits());
    printf("Max val: %d\n", getMaxDataVal());
}
