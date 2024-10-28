#include <EspBleControls.h>
#include <bitset>

// --------------------------------------------------------------------------------------------------------------------

uint32_t getCurrentTimeStamp() {
  return esp_timer_get_time() / 1000;
}

uint32_t getTimeLapsed(uint32_t timeStamp) {
  return getCurrentTimeStamp() - timeStamp;
}

const std::string intToString(int number, int digitCount, int base) {
    int sum = number;
    int digit;
    std::string output = "";
    if (digitCount == 0 || base < 2) return output;
    for (int i = 0; i < digitCount; i++) output += '0';
    output[digitCount] = '\0';
    int index = digitCount - 1;
    while (index >= 0) {
        digit = sum % base;
        sum /= base;
        if (digit < 0xA) output[index] = '0' + digit;
        else output[index] = 'A' + digit - 0xA;
        index--;
    }
    return output;
}

const std::string shortToHex(short number) {
    return intToString((uint16_t)number, 4, 16);
}

template <typename IntegerType>
const IntegerType bytesToIntegerType(uint8_t* bytes, bool big_endian = false) {
    IntegerType result = 0;
    if (!big_endian)
        for (int n = sizeof(result); n >= 0; n--)
            result = (result << 8) + bytes[n];
    else
        for (unsigned n = 0; n < sizeof(result); n++)
            result = (result << 8) + bytes[n];
    return result;
}

const float bytesToFloat(uint8_t* bytes, bool big_endian = false) {
    float result = 0;
    uint8_t* floatPointer = (uint8_t*)&result;
    if (!big_endian) {
        floatPointer[3] = bytes[3];
        floatPointer[2] = bytes[2];
        floatPointer[1] = bytes[1];
        floatPointer[0] = bytes[0];
    } else {
        floatPointer[3] = bytes[0];
        floatPointer[2] = bytes[1];
        floatPointer[1] = bytes[2];
        floatPointer[0] = bytes[3];
    }
    return result;
}

const std::vector<bool> bytesToBools(uint8_t* bytes, size_t length) {
    std::vector<bool> result;
    for (int index = 0; index<length; index++) {
       	uint8_t c = *bytes;
        auto bits = std::bitset<8>(c);
        for (int bit = 0; bit < 8; bit ++) {
            result.push_back(bits[bit]);
        }
		bytes++;    
    }
    return result;
}

const std::string bytesToConsole(uint8_t* bytes, size_t length) {
    std::string result = "";
    for (int index = 0; index<length; index++) {
       	uint8_t c = *bytes;
        result += intToString(c, 2, 16) + ' ';
		bytes++;    
    }
    return result;
}

// --------------------------------------------------------------------------------------------------------------------

const char CharacteristicCallback::getValueType() {
    if (m_pIntFunc != nullptr) return 'i';
    if (m_pUIntFunc != nullptr) return 'u';
    if (m_pFloatFunc != nullptr) return 'f';
    if (m_pStringFunc != nullptr) return 's';
    if (m_pBoolsFunc != nullptr) return 'b';
    return 'x';
}

void CharacteristicCallback::executeCallback(BLECharacteristic* pChar, bool saveValues = false){
            Preferences m_preferences;
            m_preferences.begin(PREFERENCES_ID, false);
            const std::string controlId = pChar->getUUID().toString().substr(24,12);
            uint8_t* byteArrayValue = pChar->getData();
            if (m_pFloatFunc != nullptr) {
                float_t floatValue = bytesToFloat(byteArrayValue);
                printf("Received %g float from %s\n", floatValue, m_callerName.c_str());
                if (saveValues) { 
                    m_preferences.putFloat(controlId.c_str(), floatValue);
                    printf("Saved %g float to %s\n", m_preferences.getFloat(controlId.c_str()), controlId.c_str());
                }
                m_pFloatFunc(floatValue);
            }
            if (m_pUIntFunc != nullptr) {
                uint32_t intValue = bytesToIntegerType<uint32_t>(byteArrayValue);
                printf("Received %i Uint from %s\n", intValue, m_callerName.c_str());
                if (saveValues) {
                    m_preferences.putUInt(controlId.c_str(), intValue);
                    printf("Saved %i Uint to %s\n", m_preferences.getUInt(controlId.c_str()), controlId.c_str());
                }
                m_pUIntFunc(intValue);
            }
            if (m_pIntFunc != nullptr) {
                int32_t intValue = bytesToIntegerType<int32_t>(byteArrayValue);
                printf("Received %i int from %s\n", intValue, m_callerName.c_str());
                if (saveValues) {
                    m_preferences.putInt(controlId.c_str(), intValue);
                    printf("Saved %i int to %s\n", m_preferences.getInt(controlId.c_str()), controlId.c_str());
                }
                m_pIntFunc(intValue);
            }
            if (m_pStringFunc != nullptr) {
                std::string stringValue = pChar->getValue();
                printf("Received %s string from %s\n", stringValue.c_str(), m_callerName.c_str());
                if (saveValues) {
                    m_preferences.putString(controlId.c_str(), stringValue.c_str());
                    uint8_t savedStringValue[512];
                    m_preferences.getString(controlId.c_str(), (char*)&savedStringValue[0], sizeof(savedStringValue));
                    printf("Saved %s string to %s\n", (char*)savedStringValue, controlId.c_str());
                }
                m_pStringFunc(stringValue);
            }
            if (m_pBoolsFunc != nullptr) {
                printf("Received %i bytes as %s from %s\n", m_dataSize, bytesToConsole(byteArrayValue, m_dataSize).c_str(), m_callerName.c_str());
                if (saveValues) {
                    m_preferences.putBytes(controlId.c_str(), byteArrayValue, m_dataSize);
                    uint8_t savedValue[m_dataSize];
                    m_preferences.getBytes(controlId.c_str(), savedValue, m_dataSize);
                    printf("Saved %s bytes to %s\n", bytesToConsole(savedValue, m_dataSize).c_str(), controlId.c_str());
                }
                m_pBoolsFunc(bytesToBools(byteArrayValue, m_dataSize));
            }
            m_preferences.end();
}

CharacteristicCallback::CharacteristicCallback(
    void (*func)(uint32_t) = nullptr,
    const std::string charDescription = "intCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pUIntFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    void (*func)(int32_t) = nullptr,
    const std::string charDescription = "intCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pIntFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    void (*func)(float_t) = nullptr,
    const std::string charDescription = "floatCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pFloatFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    void (*func)(std::string) = nullptr,
    const std::string charDescription = "stringCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pStringFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    void (*func)(std::vector<bool>) = nullptr,
    const size_t dataSize = 3U,
    const std::string charDescription = "stringCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_dataSize = dataSize;
    m_pBoolsFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

// --------------------------------------------------------------------------------------------------------------------

EspBleControls::EspBleControls(const std::string deviceName, const uint32_t passkey) {

    m_isDeviceConnected = false;
    m_isDeviceAuthorised = false;
    m_pin = passkey;

    BLEDevice::init(deviceName);

    if (m_pin == 0) {
        m_isDeviceAuthorised = true;
    } else {
        BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
        BLESecurityCallbacks* secCallback = new SecurityCallback(
            [&](bool isDeviceAuthorised) -> void { m_isDeviceAuthorised = isDeviceAuthorised; },
            m_pServer
        );
        BLEDevice::setSecurityCallbacks(secCallback);
    }

    m_pServer = BLEDevice::createServer();
    BLEServerCallbacks* serverCallback = new ServerCallback(
        [&](bool isDeviceConnected) -> void { m_isDeviceConnected = isDeviceConnected; },
        [&](bool isDeviceAuthorised) -> void { m_isDeviceAuthorised = isDeviceAuthorised; }
    );
    m_pServer->setCallbacks(serverCallback);
    m_pService = m_pServer->createService(BLEUUID(SERVICE_UUID), 128, 0);
}

bool EspBleControls::isConnected() {
    return m_isDeviceConnected;
}

bool EspBleControls::isAuthorised() {
    return m_isDeviceAuthorised;
}

void EspBleControls::startService() {
    m_pService->start();
    startAdvertising();
    if (m_pin != 0) setBleSecurity();
}

void EspBleControls::clearPrefs() {
    Preferences m_preferences;
    m_preferences.begin(PREFERENCES_ID, false);
    m_preferences.clear();
    m_preferences.end();
}

void EspBleControls::setBleSecurity() {
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND;
    esp_ble_io_cap_t iocap = ESP_IO_CAP_OUT;
    uint8_t key_size = 16;
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t auth_option = ESP_BLE_ONLY_ACCEPT_SPECIFIED_AUTH_DISABLE;
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_STATIC_PASSKEY, &m_pin, sizeof(uint32_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_ONLY_ACCEPT_SPECIFIED_SEC_AUTH, &auth_option, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t));
    esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t));
}

void EspBleControls::startAdvertising() {
    BLEAdvertisementData advData;
    advData.setCompleteServices(BLEUUID(SERVICE_UUID));
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->setScanResponse(false);
    pAdvertising->setMinPreferred(0x0);
    pAdvertising->setScanResponseData(advData);
    BLEDevice::startAdvertising();
}

template <typename ValueType> BLECharacteristic* EspBleControls::createCharacteristic(
    const std::string uuid,
    const std::string description,
    ValueType initialValue,
    size_t valueSize,
    const boolean shouldNotify,
    CharacteristicCallback* callback
) {
    uint32_t properties = BLECharacteristic::PROPERTY_READ;
    if (shouldNotify) {
        properties = BLECharacteristic::PROPERTY_NOTIFY;
    }
    if (callback != nullptr) {
        properties = properties | BLECharacteristic::PROPERTY_WRITE;
    }

    BLECharacteristic* characteristic = m_pService->createCharacteristic(BLEUUID(uuid), properties);
    characteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    
    if (shouldNotify) {
        BLE2902* cccd;
        cccd = new BLE2902();
        cccd->setNotifications(true);
        characteristic->addDescriptor(cccd);
    } else {
        characteristic->setValue(initialValue);
    }

    Preferences m_preferences;
    m_preferences.begin(PREFERENCES_ID, false);
    const std::string controlId = uuid.substr(24, 12);
    if (m_preferences.isKey(controlId.c_str())) {
        char valueType = callback->getValueType();
        if (valueType == 'i') {
            int value = m_preferences.getInt(controlId.c_str());
            characteristic->setValue(value);
        }
        if (valueType == 'u') {
            uint32_t value = m_preferences.getUInt(controlId.c_str());
            characteristic->setValue(value);
        }
        if (valueType == 'f') {
            float value = m_preferences.getFloat(controlId.c_str());
            characteristic->setValue(value);
        }
        if (valueType == 's') {
            uint8_t value[512];
            m_preferences.getString(controlId.c_str(), (char*)&value[0], sizeof(value));
            characteristic->setValue((char*)value);
        }
        if (valueType == 'b') {
            uint8_t value[valueSize];
            m_preferences.getBytes(controlId.c_str(), value, valueSize);
            characteristic->setValue(value, valueSize);
        }
        callback->executeCallback(characteristic);
    }
    m_preferences.end();

    BLEDescriptor* cudd;
    cudd = new BLEDescriptor((uint16_t)0x2901);
    cudd->setValue(description);
    characteristic->addDescriptor(cudd);
    characteristic->setCallbacks(callback);

    return characteristic;
};

const std::string EspBleControls::generateCharUuid(const std::string suffix, const int16_t val1 = 0, const int16_t val2 = 0, const int16_t val3 = 0) {
    std::string uuid = CHAR_UUID_PREFIX;
    uuid.append("-");
    uuid.append(shortToHex(val1));
    uuid.append("-");
    uuid.append(shortToHex(val2));
    uuid.append("-");
    uuid.append(shortToHex(val3));
    uuid.append("-");
    uuid.append(suffix);
    uint16_t charCount = getCharIndex(suffix) + 1;
    uuid.append(intToString(charCount, 2, 16));
    return uuid;
}

const uint16_t EspBleControls::getCharIndex(const std::string charId) {
    const std::map<std::string, uint16_t>::iterator charIterator = m_charsCounter.find(charId);
    if (charIterator != m_charsCounter.end()) {
        return m_charsCounter.at(charId);
    } else {
        m_charsCounter[charId] = 0;
        return 0;
    }
}

const boolean EspBleControls::characteristicCounterExists(const std::string charId) {
    const std::map<std::string, uint16_t>::iterator charIterator = m_charsCounter.find(charId);
    return charIterator != m_charsCounter.end();
}

BLECharacteristic* EspBleControls::createClockControl(
    const std::string description,
    const uint32_t initialValue,
    const boolean shouldNotify,
    void (onValueReceived)(uint32_t)
) {
    if (!characteristicCounterExists(CLOCK_UUID_SUFFIX)) {
        CharacteristicCallback* callback = nullptr;
        if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
        return createCharacteristic(generateCharUuid(CLOCK_UUID_SUFFIX), description, initialValue, sizeof(initialValue), shouldNotify, callback);
    } else {
        return m_pService->getCharacteristic(generateCharUuid(CLOCK_UUID_SUFFIX));
    }
}

BLECharacteristic* EspBleControls::createSwitchControl(
    std::string description,
    std::string initialValue,
    boolean shouldNotify,
    void (onValueReceived)(std::string)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(SWTCH_UUID_SUFFIX), description, initialValue, sizeof(initialValue), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createMomentaryControl(
    std::string description,
    std::string initialValue,
    boolean shouldNotify,
    void (onValueReceived)(std::string)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(MOMNT_UUID_SUFFIX), description, initialValue, sizeof(initialValue), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createSliderControl(
    std::string description,
    short minValue,
    short maxValue,
    uint16_t steps,
    int initialValue,
    boolean shouldNotify,
    void (onValueReceived)(int32_t)
) {    
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(SLIDR_UUID_SUFFIX, minValue, maxValue, steps), description, initialValue, sizeof(initialValue), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createIntControl(
    std::string description,
    short minValue,
    short maxValue,
    int initialValue,
    boolean shouldNotify,
    void (onValueReceived)(int32_t)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(INTGR_UUID_SUFFIX, minValue, maxValue), description, initialValue, sizeof(initialValue), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createFloatControl(
    std::string description,
    short minValue,
    short maxValue,
    float_t initialValue,
    boolean shouldNotify,
    void (onValueReceived)(float_t)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(FLOAT_UUID_SUFFIX, minValue, maxValue), description, initialValue, sizeof(initialValue), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createStringControl(
    std::string description,
    uint16_t maxLength,
    std::string initialValue,
    boolean shouldNotify,
    void (onValueReceived)(std::string)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);    
    return createCharacteristic(generateCharUuid(STRNG_UUID_SUFFIX, maxLength), description, initialValue, maxLength, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createAngleControl(
    std::string description,
    uint32_t initialValue,
    boolean isComapss,
    boolean shouldNotify,
    void (onValueReceived)(uint32_t)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(ANGLE_UUID_SUFFIX, isComapss), description, initialValue, sizeof(initialValue), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createColorControl(
    std::string description,
    std::string initialValue,
    boolean shouldNotify,
    void (onValueReceived)(std::string)
) {
    CharacteristicCallback* callback = nullptr;
    if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(COLOR_UUID_SUFFIX), description, initialValue, initialValue.length(), shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createIntervalControl(
    std::string description,
    uint16_t divisions,
    boolean shouldNotify,
    void (onValueReceived)(std::vector<bool>)
) {
    if (!characteristicCounterExists(CLOCK_UUID_SUFFIX)) { 
        return createStringControl(description, 256, "There is no Clock control defined!\nPlease add one before creating an Interval control!", false, nullptr);
    } else {
        CharacteristicCallback* callback = nullptr;
        std::string initialValue(divisions / 8, 0);
        if (onValueReceived != nullptr) callback = new CharacteristicCallback(onValueReceived, initialValue.length(), description, &m_isDeviceAuthorised);
        return createCharacteristic(generateCharUuid(INTRV_UUID_SUFFIX, divisions), description, initialValue, initialValue.length(), shouldNotify, callback);
    }
}

// --------------------------------------------------------------------------------------------------------------------

SecurityCallback::SecurityCallback(std::function<void(bool)> onDeviceAuthentication, BLEServer* pServer) {
    m_onDeviceAuthentication = onDeviceAuthentication;
    m_pServer = pServer;
};

// --------------------------------------------------------------------------------------------------------------------

ServerCallback::ServerCallback(std::function<void(bool)> onDeviceConnection, std::function<void(bool)> onDeviceAuthentication) {
    m_onDeviceConnection = onDeviceConnection;
    m_onDeviceAuthentication = onDeviceAuthentication;
}
