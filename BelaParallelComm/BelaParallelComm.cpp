#include <BelaParallelComm/BelaParallelComm.h>

BelaParallelComm::BelaParallelComm(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, unsigned int headerSize, unsigned int nBlocks, bool lsb) {
    setup(belaId, digitalPins, nDigitals, headerSize, nBlocks);
}

int BelaParallelComm::setup(std::string belaId, unsigned int* digitalPins, unsigned int nDigitals, unsigned int headerSize, unsigned int nBlocks, bool lsb) {
    _belaId = belaId;
    _lsb
      = lsb;
    _nDigitals = nDigitals;
    _digitalPins.resize(nDigitals);
    for (unsigned int i = 0; i < nDigitals; i++)
        _digitalPins[i] = digitalPins[i];
    _headerSize = (headerSize < nDigitals - 1) ? headerSize : 2;
    _nBlocks = (nBlocks > 0) ? nBlocks : 1;

    _dataHeader.resize(headerSize);
    unsigned int nDataBits = nDigitals * nBlocks - headerSize - nBlocks;
    _dataBuffer.resize(nDataBits);
    _grayTable = generateGray(nDataBits);
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
        bitArray[i] = _grayTable[value][i];
    }
}

unsigned int BelaParallelComm::bitArrayToInt(unsigned int* bitArray, int nBits, bool lsb) {
    // find value in table
    unsigned int value;
    for (unsigned int i = 0; i < _grayTable.size(); i++) {
        for (unsigned int j = 0; j < nBits; j++) {
            if (_grayTable[i][j] != bitArray[j]) {
                break;
            } else if (j == 2) { // if all numbers in bitArray are equal to _grayTable[i], match
                value = i;
            }
        }
    }
    return value;
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
    if (rt) { // real-time safe prints
        rt_printf("-- %s --\n", _belaId.c_str());

        rt_printf("Data Header [%d]:", getHeaderVal());
        for (unsigned int i = 0; i < _dataHeader.size(); i++)
            rt_printf(" %d", _dataHeader[i]);
        rt_printf("\n");

        rt_printf("Data Buffer [%d]:", getBufferVal());
        for (unsigned int i = 0; i < _dataBuffer.size(); i++)
            rt_printf(" %d", _dataBuffer[i]);
        rt_printf("\n");
    } else {
        printf("-- %s --\n", _belaId.c_str());

        printf("Data Header [%d]:", getHeaderVal());
        for (unsigned int i = 0; i < _dataHeader.size(); i++)
            printf(" %d", _dataHeader[i]);
        printf("\n");

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
    printf("Header size (bits): %d\n", _headerSize);
    printf("Number of blocks: %d\n", _nBlocks);
    printf("Number of data bits: %d\n", getNumDataBits());
    printf("Max val: %d\n", getMaxDataVal());
}

std::vector<std::vector<int>> BelaParallelComm::generateGray(int n) {
    // Base case
    if (n <= 0)
        return { { 0 } };

    if (n == 1) {
        return { { 0 }, { 1 } };
    }

    //Recursive case
    std::vector<std::vector<int>> recAns = generateGray(n - 1);
    std::vector<std::vector<int>> mainAns;

    // Append 0 to the first half
    for (int i = 0; i < recAns.size(); i++) {
        std::vector<int> s = recAns[i];
        s.insert(s.begin(), 0);

        mainAns.push_back(s);
    }

    // Append 1 to the second half
    for (int i = recAns.size() - 1; i >= 0; i--) {
        std::vector<int> s = recAns[i];
        s.insert(s.begin(), 1);

        mainAns.push_back(s);
    }
    return mainAns;
}
