#include <EspBleControls.h>
#include <bitset>

// --------------------------------------------------------------------------------------------------------------------

const bool hasTimePassed(uint32_t fromTimeStamp, uint16_t durationSeconds) {
    return (millis() - fromTimeStamp) >= (durationSeconds * 1000);
}

const int getIntervalIndex(uint32_t currentTime, uint16_t dayDivisions) {
    uint32_t currentDaySeconds = currentTime % (DAY_MINUTES * 60);
    uint16_t divisionSeconds = (DAY_MINUTES / dayDivisions) * 60;
    uint16_t currentIndex = currentDaySeconds / divisionSeconds;
    return currentIndex;
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

const std::vector<char> bytesToBools(uint8_t* bytes, size_t length) {
    std::vector<char> result;
    for (int index = 0; index < length; index++) {
        uint8_t c = *bytes;
        auto bits = std::bitset<8>(c);
        for (int bit = 7; bit >= 0; bit--) {
            result.push_back(bits[bit]);
        }
        bytes++;
    }
    return result;
}

const std::string bytesToConsole(uint8_t* bytes, size_t length) {
    std::string result = "";
    for (int index = 0; index < length; index++) {
        uint8_t c = *bytes;
        result += intToString(c, 2, 16) + ' ';
        bytes++;
    }
    return result;
}

const std::string getCharParamValue(const std::string uuid, uint8_t position) {
    std::string result = uuid;
    if (position <= SUFFIX) {
        switch (position) {
            case PREFIX: result = uuid.substr(0, 8); break;
            case PARAM1: result = uuid.substr(9, 4); break;
            case PARAM2: result = uuid.substr(14, 4); break;
            case PARAM3: result = uuid.substr(19, 4); break;
            case SUFFIX: result = uuid.substr(24, 12); break;
        }
    }
    return result;
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

// --------------------------------------------------------------------------------------------------------------------

const char CharacteristicCallback::getValueType() {
    if (m_pIntFunc != nullptr) return 'i';
    if (m_pUIntFunc != nullptr) return 'u';
    if (m_pFloatFunc != nullptr) return 'f';
    if (m_pStringFunc != nullptr) return 's';
    if (m_pBoolsFunc != nullptr) return 'b';
    return 'x';
}

void CharacteristicCallback::executeCallback(BLECharacteristic* pChar, bool saveValues = false) {
    Preferences m_preferences;
    m_preferences.begin(PREFERENCES_ID, false);
    const std::string controlId = getCharParamValue(pChar->getUUID().toString(), SUFFIX);
    uint8_t* m_byteArray = pChar->getData();
    if (m_pFloatFunc != nullptr) {
        float_t floatValue = bytesToFloat(m_byteArray);
        printf("Received %g float from %s\n", floatValue, m_callerName.c_str());
        if (saveValues) {
            m_preferences.putFloat(controlId.c_str(), floatValue);
            printf("Saved %g float to %s\n", m_preferences.getFloat(controlId.c_str()), controlId.c_str());
        }
        m_pFloatFunc(floatValue);
    }
    if (m_pUIntFunc != nullptr) {
        uint32_t intValue = bytesToIntegerType<uint32_t>(m_byteArray);
        printf("Received %i Uint from %s\n", intValue, m_callerName.c_str());
        if (saveValues && controlId != ((std::string)CLOCK_UUID_SUFFIX).append("01")) {
            m_preferences.putUInt(controlId.c_str(), intValue);
            printf("Saved %i Uint to %s\n", m_preferences.getUInt(controlId.c_str()), controlId.c_str());
        }
        m_pUIntFunc(intValue);
    }
    if (m_pIntFunc != nullptr) {
        int32_t intValue = bytesToIntegerType<int32_t>(m_byteArray);
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
        size_t m_dataSize = pChar->getLength();
        printf("Received %i bytes as %s from %s\n", m_dataSize, bytesToConsole(m_byteArray, m_dataSize).c_str(), m_callerName.c_str());
        if (saveValues) {
            m_preferences.putBytes(controlId.c_str(), m_byteArray, m_dataSize);
            uint8_t savedValue[m_dataSize];
            m_preferences.getBytes(controlId.c_str(), savedValue, m_dataSize);
            printf("Saved %s bytes to %s\n", bytesToConsole(savedValue, m_dataSize).c_str(), controlId.c_str());
        }
        m_pBoolsFunc(bytesToBools(m_byteArray, m_dataSize));
    }
    m_preferences.end();
}

CharacteristicCallback::CharacteristicCallback(
    std::function<void(uint32_t)> func = nullptr,
    const std::string charDescription = "intCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pUIntFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(int32_t)> func = nullptr,
    const std::string charDescription = "intCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pIntFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(float_t)> func = nullptr,
    const std::string charDescription = "floatCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pFloatFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(std::string)> func = nullptr,
    const std::string charDescription = "stringCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pStringFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(std::vector<char>)> func = nullptr,
    const std::string charDescription = "stringCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pBoolsFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
};

// --------------------------------------------------------------------------------------------------------------------

void IntervalControl::checkIntervalChange() {
    if (m_checkDelaySeconds != 0 && hasTimePassed(m_lastNotificationTimeStamp, m_checkDelaySeconds)) {
        if (m_onIntervalToggle != nullptr && (m_intervals.size() >= DAY_HOURS)) {
            size_t intervalIndex = getIntervalIndex(espClock.getEpoch(), m_intervals.size());
            printf ("Interval %i is %i\n", intervalIndex, m_intervals[intervalIndex]);
            m_onIntervalToggle(m_intervals[intervalIndex] == 1);
        }
        m_lastNotificationTimeStamp = millis();
    }
}

CharacteristicCallback* IntervalControl::setCallback(const std::string charDescription, bool* isDeviceAuthorised){
    std::function<void(std::vector<char>)> onIntervalsChanged = [&](std::vector<char> intervals){
         m_intervals = intervals;
         m_lastNotificationTimeStamp = millis() - m_checkDelaySeconds * 1000;
         checkIntervalChange();
    };
    return new CharacteristicCallback(onIntervalsChanged, charDescription, isDeviceAuthorised);
}

void IntervalControl::setup(BLECharacteristic* bleCharacteristic, const uint16_t checkDelaySeconds, std::function<void(bool)> onIntervalToggle){
    m_checkDelaySeconds = checkDelaySeconds;
    m_onIntervalToggle = onIntervalToggle;
    m_bleCharacteristic = bleCharacteristic;
}

// --------------------------------------------------------------------------------------------------------------------

void ClockControl::updateTime() {
    if (m_notifyDelaySeconds != 0 && hasTimePassed(m_lastNotificationTimeStamp, m_notifyDelaySeconds)) {
      uint32_t timeValue = espClock.getEpoch();
      m_bleCharacteristic->setValue(timeValue);
      m_bleCharacteristic->notify();
      m_lastNotificationTimeStamp = millis();
    }
}

CharacteristicCallback* ClockControl::setCallback(const std::string charDescription, bool* isDeviceAuthorised) {
    std::function<void(uint32_t)> onTimeSet = [&](uint32_t time) { 
        espClock.setTime(time);
        m_onTimeSet(time);
        updateTime();
    };
    return new CharacteristicCallback(onTimeSet, charDescription, isDeviceAuthorised);
}

void ClockControl::setup(BLECharacteristic* bleCharacteristic, const uint16_t notifyDelaySeconds, uint32_t initialValue, std::function<void(uint32_t)> onTimeSet){
    m_notifyDelaySeconds = notifyDelaySeconds;
    m_bleCharacteristic = bleCharacteristic;
    m_onTimeSet = onTimeSet;
    espClock.setTime(initialValue);
}

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

void EspBleControls::loopCallbacks() {
    if (m_controls.size() > 0 && m_isDeviceAuthorised && m_isDeviceConnected) {
        for (std::map<std::string, ControlCallback*>::iterator it = m_controls.begin(); it!=m_controls.end(); ++it) {
            it->second->update();
        };
    }
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

void EspBleControls::restoreValue(BLECharacteristic* characteristic, const std::string uuid, CharacteristicCallback* callback) {
    Preferences m_preferences;
    m_preferences.begin(PREFERENCES_ID, false);
    const std::string controlId = getCharParamValue(uuid, SUFFIX);
    if (m_preferences.isKey(controlId.c_str())) {
        char valueType = callback->getValueType();
        if (valueType == 'i') {
            int value = m_preferences.getInt(controlId.c_str());
            characteristic->setValue(value);
        }
        if (valueType == 'u' && controlId != ((std::string)CLOCK_UUID_SUFFIX).append("01")) {
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
            size_t valueSize = stoi(getCharParamValue(uuid, PARAM1), 0, 16) / 8;
            uint8_t value[valueSize];
            m_preferences.getBytes(controlId.c_str(), value, valueSize);
            characteristic->setValue(value, valueSize);
        }
        callback->executeCallback(characteristic);
    }
    m_preferences.end();
}

template <typename ValueType> BLECharacteristic* EspBleControls::createCharacteristic(
    const std::string uuid,
    const std::string description,
    ValueType initialValue,
    const boolean shouldNotify,
    CharacteristicCallback* callback
) { 
    uint32_t properties = BLECharacteristic::PROPERTY_READ;
    if (shouldNotify) properties = BLECharacteristic::PROPERTY_NOTIFY;
    if (callback != nullptr) properties = properties | BLECharacteristic::PROPERTY_WRITE;
    
    BLECharacteristic* characteristic = m_pService->createCharacteristic(BLEUUID(uuid), properties);
    if (m_pin != 0) characteristic->setAccessPermissions(ESP_GATT_PERM_READ_ENCRYPTED | ESP_GATT_PERM_WRITE_ENCRYPTED);
    if (shouldNotify) {
        BLE2902* cccd;
        cccd = new BLE2902();
        cccd->setNotifications(true);
        characteristic->addDescriptor(cccd);
    }

    BLEDescriptor* cudd;
    cudd = new BLEDescriptor((uint16_t)0x2901);
    cudd->setValue(description);
    characteristic->addDescriptor(cudd);
    
    characteristic->setValue(initialValue);

    restoreValue(characteristic, uuid, callback);

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

ClockControl* EspBleControls::createClockControl(
    const std::string description,
    const uint32_t initialValue,
    uint16_t notifyDelaySeconds,
    std::function<void(uint32_t)> onTimeSet
) {
    if (characteristicCounterExists(CLOCK_UUID_SUFFIX)) {
        createStringControl(description, 256, "Only one Clock control instance can be created!", false, nullptr);
        return nullptr;
    } else {
        const std::string newUuid = generateCharUuid(CLOCK_UUID_SUFFIX, notifyDelaySeconds);
        ClockControl* clockControl = new ClockControl();
        CharacteristicCallback* callback = clockControl->setCallback(description, &m_isDeviceAuthorised);
        BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, true, callback);
        clockControl->setup(bleCharacteristic, notifyDelaySeconds, initialValue, onTimeSet);
        m_controls[getCharParamValue(newUuid , SUFFIX)] = clockControl;
        return clockControl;
    }
}

BLECharacteristic* EspBleControls::createSwitchControl(
    std::string description,
    std::string initialValue,
    boolean shouldNotify,
    std::function<void(std::string)> onSwitchToggle
) {
    CharacteristicCallback* callback = nullptr;
    if (onSwitchToggle != nullptr) callback = new CharacteristicCallback(onSwitchToggle, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(SWTCH_UUID_SUFFIX), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createMomentaryControl(
    std::string description,
    std::string initialValue,
    boolean shouldNotify,
    std::function<void(std::string)> onButtonPressed
) {
    CharacteristicCallback* callback = nullptr;
    if (onButtonPressed != nullptr) callback = new CharacteristicCallback(onButtonPressed, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(MOMNT_UUID_SUFFIX), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createSliderControl(
    std::string description,
    short minValue,
    short maxValue,
    uint16_t steps,
    int initialValue,
    boolean shouldNotify,
    std::function<void(int32_t)> onSliderMoved
) {
    CharacteristicCallback* callback = nullptr;
    if (onSliderMoved != nullptr) callback = new CharacteristicCallback(onSliderMoved, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(SLIDR_UUID_SUFFIX, minValue, maxValue, steps), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createIntControl(
    std::string description,
    short minValue,
    short maxValue,
    int initialValue,
    boolean shouldNotify,
    std::function<void(int32_t)> onIntReceived
) {
    CharacteristicCallback* callback = nullptr;
    if (onIntReceived != nullptr) callback = new CharacteristicCallback(onIntReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(INTGR_UUID_SUFFIX, minValue, maxValue), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createFloatControl(
    std::string description,
    short minValue,
    short maxValue,
    float_t initialValue,
    boolean shouldNotify,
    std::function<void(float_t)> onFloatReceived
) {
    CharacteristicCallback* callback = nullptr;
    if (onFloatReceived != nullptr) callback = new CharacteristicCallback(onFloatReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(FLOAT_UUID_SUFFIX, minValue, maxValue), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createStringControl(
    std::string description,
    uint16_t maxLength,
    std::string initialValue,
    boolean shouldNotify,
    std::function<void(std::string)> onTextReceived
) {
    CharacteristicCallback* callback = nullptr;
    if (onTextReceived != nullptr) callback = new CharacteristicCallback(onTextReceived, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(STRNG_UUID_SUFFIX, maxLength), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createAngleControl(
    std::string description,
    uint32_t initialValue,
    boolean isComapss,
    boolean shouldNotify,
    std::function<void(uint32_t)> onAngleChanged
) {
    CharacteristicCallback* callback = nullptr;
    if (onAngleChanged != nullptr) callback = new CharacteristicCallback(onAngleChanged, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(ANGLE_UUID_SUFFIX, isComapss), description, initialValue, shouldNotify, callback);
}

BLECharacteristic* EspBleControls::createColorControl(
    std::string description,
    std::string initialValue,
    boolean shouldNotify,
    std::function<void(std::string)> onColorChanged
) {
    CharacteristicCallback* callback = nullptr;
    if (onColorChanged != nullptr) callback = new CharacteristicCallback(onColorChanged, description, &m_isDeviceAuthorised);
    return createCharacteristic(generateCharUuid(COLOR_UUID_SUFFIX), description, initialValue, shouldNotify, callback);
}

IntervalControl* EspBleControls::createIntervalControl(
    const std::string description,
    const uint16_t divisions,
    const uint16_t checkDelaySeconds,
    std::function<void(bool)> onIntervalToggle
) {
    if (characteristicCounterExists(CLOCK_UUID_SUFFIX)) {
        const std::string newUuid = generateCharUuid(INTRV_UUID_SUFFIX, divisions);
        const std::string initialValue(divisions / 8, 0);
        IntervalControl* intervalControl = new IntervalControl();
        CharacteristicCallback* callback = intervalControl->setCallback(description, &m_isDeviceAuthorised);
        BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, false, callback);
        intervalControl->setup(bleCharacteristic, checkDelaySeconds, onIntervalToggle);
        m_controls[getCharParamValue(newUuid , SUFFIX)] = intervalControl;
        return intervalControl;
    } else {
        createStringControl(description, 256, "There is no Clock control defined!\nPlease add one before creating an Interval control!", false, nullptr);
        return nullptr;
    }
}
