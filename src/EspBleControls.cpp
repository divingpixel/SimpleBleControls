#include <EspBleControls.h>

// --------------------------------------------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------------------------------------------

const std::string bytesToConsole(uint8_t* bytes, size_t length) {
    std::string result = "";
    for (int index = 0; index < length; index++) {
        uint8_t c = *bytes;
        result += intToString(c, 2, 16) + ' ';
        bytes++;
    }
    return result;
}

const bool hasTimePassed(uint32_t fromTimeStamp, uint16_t durationSeconds) {
    return (millis() - fromTimeStamp) >= (durationSeconds * 1000);
}

const int getIntervalIndex(uint32_t currentTime, uint16_t dayDivisions) {
    uint32_t currentDaySeconds = currentTime % (DAY_MINUTES * 60);
    uint16_t divisionSeconds = (DAY_MINUTES / dayDivisions) * 60;
    uint16_t currentIndex = currentDaySeconds / divisionSeconds;
    return currentIndex;
}

const std::string getCharParamValue(const std::string uuid, uint8_t position) {
    std::string result = uuid;
    if (position <= CHARID) {
        switch (position) {
            case PREFIX: result = uuid.substr(0, 8); break;
            case PARAM1: result = uuid.substr(9, 4); break;
            case PARAM2: result = uuid.substr(14, 4); break;
            case PARAM3: result = uuid.substr(19, 4); break;
            case SUFFIX: result = uuid.substr(24, 12); break;
            case CHARID: result = uuid.substr(24, 10); break;
        }
    }
    return result;
}

const bool isNotSaveExcluded(std::string controlId) {
    return (controlId != ((std::string)CLOCK_UUID_SUFFIX).append("01")) && 
        (controlId.substr(0,10) != ((std::string)MOMNT_UUID_SUFFIX).substr(0,10));
};

// --------------------------------------------------------------------------------------------------------------------

const char CharacteristicCallback::getValueType() {
    if (m_pIntFunc != nullptr) return 'i';
    if (m_pFloatFunc != nullptr) return 'f';
    if (m_pStringFunc != nullptr) return 's';
    if (m_pBoolsFunc != nullptr) return 'b';
    return 'x';
}

void CharacteristicCallback::saveValues(void* params) {
    SaveDataParams* data = (SaveDataParams*) params;
    Preferences m_preferences;
    m_preferences.begin(PREFERENCES_ID, false);
    const std::string controlId = getCharParamValue(data->pChar->getUUID().toString(), SUFFIX);
    uint8_t* m_byteArray = data->pChar->getData();
    if (isNotSaveExcluded(controlId) && data->pChar != nullptr) {
        ;
        if (data->type == 'f') {
            float_t floatValue = bytesToFloat(m_byteArray);
            m_preferences.putFloat(controlId.c_str(), floatValue);
            //printf("Saved %g float to %s\n", m_preferences.getFloat(controlId.c_str()), controlId.c_str());
        }
        if (data->type == 'i') {
            int32_t intValue = bytesToIntegerType<int32_t>(m_byteArray);
            m_preferences.putInt(controlId.c_str(), intValue);
            //printf("Saved %i int to %s\n", m_preferences.getInt(controlId.c_str()), controlId.c_str());
        }
        if (data->type == 's') {
            std::string stringValue = data->pChar->getValue();
            m_preferences.putString(controlId.c_str(), stringValue.c_str());
            uint8_t savedStringValue[512];
            m_preferences.getString(controlId.c_str(), (char*)&savedStringValue[0], sizeof(savedStringValue));
            //printf("Saved %s string to %s\n", (char*)savedStringValue, controlId.c_str());
        }
        if (data->type == 'b') {
            size_t m_dataSize = data->pChar->getLength();
            m_preferences.putBytes(controlId.c_str(), m_byteArray, m_dataSize);
            uint8_t savedValue[m_dataSize];
            m_preferences.getBytes(controlId.c_str(), savedValue, m_dataSize);
            //printf("Saved %s bytes to %s\n", bytesToConsole(savedValue, m_dataSize).c_str(), controlId.c_str());
        }
        m_preferences.end();
    }
    vTaskDelete(NULL);
}

void CharacteristicCallback::executeCallback(BLECharacteristic* pChar, bool shouldSaveValues = false) {
    if (m_pFloatFunc != nullptr) m_pFloatFunc(bytesToFloat(pChar->getData()));
    if (m_pIntFunc != nullptr) m_pIntFunc(bytesToIntegerType<int32_t>(pChar->getData()));
    if (m_pStringFunc != nullptr) m_pStringFunc(pChar->getValue());
    if (m_pBoolsFunc != nullptr) m_pBoolsFunc(bytesToBools(pChar->getData(), pChar->getLength()));
    m_saveDataParams = { pChar, getValueType() };
    if (shouldSaveValues) xTaskCreate(saveValues, "saveValues", 8192, (void *) &m_saveDataParams, 10, &saveValuesTask);
}

CharacteristicCallback::CharacteristicCallback(
    std::function<void(int32_t)> func = nullptr,
    const std::string charDescription = "intCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pIntFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
    m_saveDataParams = {nullptr, 'x'};
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(float_t)> func = nullptr,
    const std::string charDescription = "floatCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pFloatFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
    m_saveDataParams = {nullptr, 'x'};
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(std::string)> func = nullptr,
    const std::string charDescription = "stringCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pStringFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
    m_saveDataParams = {nullptr, 'x'};
};

CharacteristicCallback::CharacteristicCallback(
    std::function<void(std::vector<char>)> func = nullptr,
    const std::string charDescription = "boolsCharId",
    bool* isDeviceAuthorised = nullptr
) {
    m_pBoolsFunc = func;
    m_callerName = charDescription;
    m_pIsDeviceAuthorised = isDeviceAuthorised;
    m_saveDataParams = {nullptr, 'x'};
};

// --------------------------------------------------------------------------------------------------------------------

IntervalControl::IntervalControl(
    std::string description,
    const uint16_t checkDelaySeconds,
    bool* isDeviceAuthorised,
    std::function<void(bool)> onIntervalToggle
){
    m_description = description;
    m_checkDelaySeconds = checkDelaySeconds;
    m_isDeviceAuthorised = isDeviceAuthorised;
    m_onIntervalToggle = onIntervalToggle;
    m_callback = [&](std::vector<char> intervals){
         m_intervals = intervals;
         m_lastUpdateTimeStamp = millis() - m_checkDelaySeconds * 1000;
         update();
    };
}

void IntervalControl::update() {
    if (m_checkDelaySeconds != 0 && hasTimePassed(m_lastUpdateTimeStamp, m_checkDelaySeconds)) {
        if (m_onIntervalToggle != nullptr && (m_intervals.size() >= DAY_HOURS)) {
            size_t intervalIndex = getIntervalIndex(espClock.getEpoch(), m_intervals.size());
            //printf ("Interval %i is %i\n", intervalIndex, m_intervals[intervalIndex]);
            m_onIntervalToggle(m_intervals[intervalIndex] == 1);
        }
        m_lastUpdateTimeStamp = millis();
    }
}

// --------------------------------------------------------------------------------------------------------------------

ClockControl::ClockControl(
    std::string description,
    uint32_t initialValue,
    const uint16_t notifyDelaySeconds,
    bool* isDeviceAuthorised,
    std::function<void(uint32_t)> onTimeSet
){
    m_description = description;
    m_notifyDelaySeconds = notifyDelaySeconds;
    m_isDeviceAuthorised = isDeviceAuthorised;
    m_onTimeSet = onTimeSet;
    m_callback = [&](uint32_t time) { 
        espClock.setTime(time);
        m_onTimeSet(time);
        update();
    };
    espClock.setTime(initialValue);
}

void ClockControl::update() {
    if (m_notifyDelaySeconds != 0 && hasTimePassed(m_lastUpdateTimeStamp, m_notifyDelaySeconds)) {
      uint32_t timeValue = espClock.getEpoch();
      m_bleCharacteristic->setValue(timeValue);
      if (*m_isDeviceAuthorised) m_bleCharacteristic->notify();
      m_lastUpdateTimeStamp = millis();
    }
}

// --------------------------------------------------------------------------------------------------------------------

BooleanControl::BooleanControl(
    const std::string charDescription,
    Publisher<std::string>* publisher,
    bool* isDeviceAuthorised,
    std::function<void(std::string)> onChange
){
    m_description = charDescription;
    m_publisher = publisher;
    m_isDeviceAuthorised = isDeviceAuthorised;
    m_onChange = onChange;
    m_callback = [&](std::string value) {
        //printf("%s setCallback received %s\n", m_description.c_str(), value.c_str());
        if (m_onChange != nullptr) m_onChange(value);
        if (m_publisher != nullptr) m_publisher->setValue(value, this);
    };
}

void BooleanControl::update() {
    if (m_publisher != nullptr && m_bleCharacteristic != nullptr){
        std::string currentValue = (m_publisher->getValue().length() > 0) ? m_publisher->getValue() : "OFF";
        //printf("%s updateControl received %s\n", m_description.c_str(), currentValue.c_str());
        m_bleCharacteristic->setValue(currentValue);
        if (*m_isDeviceAuthorised) m_bleCharacteristic->notify();
        m_lastNotificationTimeStamp = millis();
    }
}

// --------------------------------------------------------------------------------------------------------------------

IntControl::IntControl(
        const std::string charDescription,
        Publisher<int32_t>* publisher,
        bool* isDeviceAuthorised,
        std::function<void(int32_t)> onChange
){
    m_description = charDescription;
    m_publisher = publisher;
    m_isDeviceAuthorised = isDeviceAuthorised;
    m_onChange = onChange;
    m_callback = [&](int32_t value) {
        //printf("%s setCallback received %i\n", m_description.c_str(), value);
        if (m_onChange != nullptr) m_onChange(value);
        if (m_publisher != nullptr) m_publisher->setValue(value, this);
    };
}

void IntControl::update() {
    if (m_publisher != nullptr && m_bleCharacteristic != nullptr) {
        int currentValue = m_publisher->getValue();
        //printf("%s updateControl received %i\n", m_description.c_str(), currentValue);
        m_bleCharacteristic->setValue(currentValue);
        if (*m_isDeviceAuthorised) m_bleCharacteristic->notify();
        m_lastNotificationTimeStamp = millis();
    }
}

// --------------------------------------------------------------------------------------------------------------------

FloatControl::FloatControl(
        const std::string charDescription,
        Publisher<float_t>* publisher,
        bool* isDeviceAuthorised,
        std::function<void(float_t)> onChange
){
    m_description = charDescription;
    m_publisher = publisher;
    m_isDeviceAuthorised = isDeviceAuthorised;
    m_onChange = onChange;
    m_callback = [&](float_t value) {
        //printf("%s setCallback received %g\n", m_description.c_str(), value);
        if (m_onChange != nullptr) m_onChange(value);
        if (m_publisher != nullptr) m_publisher->setValue(value, this);
    };
}

void FloatControl::update() {
    if (m_publisher != nullptr && m_bleCharacteristic != nullptr) {
        float_t currentValue = m_publisher->getValue();
        //printf("%s updateControl received %g\n", m_description.c_str(), currentValue);
        m_bleCharacteristic->setValue(currentValue);
        if (*m_isDeviceAuthorised) m_bleCharacteristic->notify();
        m_lastNotificationTimeStamp = millis();
    }
}

// --------------------------------------------------------------------------------------------------------------------

StringControl::StringControl(
        const std::string charDescription,
        Publisher<std::string>* publisher,
        bool* isDeviceAuthorised,
        std::function<void(std::string)> onChange
){
    m_description = charDescription;
    m_publisher = publisher;
    m_isDeviceAuthorised = isDeviceAuthorised;
    m_onChange = onChange;
    m_callback = [&](std::string value) {
        //printf("%s setCallback received %g\n", m_description.c_str(), value);
        if (m_onChange != nullptr) m_onChange(value);
        if (m_publisher != nullptr) m_publisher->setValue(value, this);
    };
}

void StringControl::update() {
    if (m_publisher != nullptr && m_bleCharacteristic != nullptr) {
        std::string currentValue = m_publisher->getValue();
        //printf("%s updateControl received %g\n", m_description.c_str(), currentValue);
        m_bleCharacteristic->setValue(currentValue);
        if (*m_isDeviceAuthorised) m_bleCharacteristic->notify();
        m_lastNotificationTimeStamp = millis();
    }
}

// --------------------------------------------------------------------------------------------------------------------

EspBleControls::EspBleControls(const std::string deviceName, const uint32_t passkey) {

    m_isDeviceConnected = false;
    m_isDeviceAuthorised = false;
    m_shouldNotifyDevice = false;
    m_pin = passkey;

    BLEDevice::init(deviceName);

    if (m_pin == 0) {
        m_isDeviceAuthorised = true;
    } else {
        BLEDevice::setEncryptionLevel(ESP_BLE_SEC_ENCRYPT);
        BLESecurityCallbacks* secCallback = new SecurityCallback(
            [&](bool isDeviceAuthorised) -> void { 
                m_isDeviceAuthorised = isDeviceAuthorised; 
                if (!isDeviceAuthorised) m_pServer->removePeerDevice(m_pServer->getConnId(), true);
                notifyOnConnection();
            }
        );
        BLEDevice::setSecurityCallbacks(secCallback);
    }

    m_pServer = BLEDevice::createServer();
    BLEServerCallbacks* serverCallback = new ServerCallback(
        [&](bool isDeviceConnected) -> void { 
            m_isDeviceConnected = isDeviceConnected;
            if (!isDeviceConnected) m_isDeviceAuthorised = false;
            notifyOnConnection();
        }
    );
    m_pServer->setCallbacks(serverCallback);
    m_pService = m_pServer->createService(BLEUUID(SERVICE_UUID), 127U, 0);
}

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
    uint16_t charCount = getCharCounterIndex(suffix) + 1;
    uuid.append(intToString(charCount, 2, 16));
    return uuid;
}

const uint16_t EspBleControls::getCharCounterIndex(const std::string charId) {
    const std::map<std::string, uint16_t>::iterator charIterator = m_charsCounter.find(charId);
    if (charIterator != m_charsCounter.end()) {
        return m_charsCounter.at(charId);
    } else {
        m_charsCounter[charId] = 0;
        return 0;
    }
}

const boolean EspBleControls::doesCharCounterExists(const std::string charId) {
    const std::map<std::string, uint16_t>::iterator charIterator = m_charsCounter.find(charId);
    return charIterator != m_charsCounter.end();
}

void EspBleControls::createClearPrefsAndResetControl() {
    std::function<void(int32_t)> action = [&](int32_t shouldClear) {
        if (shouldClear == 1) {
            Preferences m_preferences;
            m_preferences.begin(PREFERENCES_ID, false);
            m_preferences.clear();
            m_preferences.end();
        }
        delay(1000);
        esp_restart();
    };
    CharacteristicCallback* callback = new CharacteristicCallback(action , "Clear values", &m_isDeviceAuthorised);
    createCharacteristic(generateCharUuid(CLRPF_UUID_SUFFIX), "Clear values", 0, false, callback);
}

void EspBleControls::notifyOnConnection(){
    if (m_isDeviceAuthorised && m_isDeviceConnected) { 
        m_shouldNotifyDevice = true;
        m_deviceConnectionTimeStamp = millis();
    }
};

void EspBleControls::startService() {
    m_pService->start();
    startAdvertising();
    if (m_pin != 0) setBleSecurity();
    createClearPrefsAndResetControl();
}

void EspBleControls::updateControls() {
    if (m_selfUpdatingControls.size() > 0) {
        for (std::map<std::string, BLEControl*>::iterator control = m_selfUpdatingControls.begin(); control != m_selfUpdatingControls.end(); ++control) {
            control->second->update();
        };
    }
    if (m_shouldNotifyDevice && hasTimePassed(m_deviceConnectionTimeStamp, CHAR_NOTIFICATION_DELAY) && m_notifyingControls.size() > 0) {
        for (std::map<std::string, BLEControl*>::iterator control = m_notifyingControls.begin(); control != m_notifyingControls.end(); ++control) {
            control->second->update();
        };
        m_shouldNotifyDevice = false;
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
    if (isNotSaveExcluded(controlId) && m_preferences.isKey(controlId.c_str())) {
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
    } else {
        characteristic->setValue(initialValue);
    }

    BLEDescriptor* cudd;
    cudd = new BLEDescriptor((uint16_t)0x2901);
    cudd->setValue(description);
    characteristic->addDescriptor(cudd);

    restoreValue(characteristic, uuid, callback);

    characteristic->setCallbacks(callback);

    return characteristic;
};

ClockControl* EspBleControls::createClockControl(
    const std::string description,
    const uint32_t initialValue,
    uint16_t notifyDelaySeconds,
    std::function<void(uint32_t)> onTimeSet
) {
    if (doesCharCounterExists(CLOCK_UUID_SUFFIX)) {
        createStringControl(description, 256, "Only one Clock control instance can be created!", nullptr, nullptr);
        return nullptr;
    } else {
        const std::string newUuid = generateCharUuid(CLOCK_UUID_SUFFIX, notifyDelaySeconds);
        ClockControl* clockControl = new ClockControl(description, initialValue, notifyDelaySeconds, &m_isDeviceAuthorised, onTimeSet);
        BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, true, clockControl->getCallback());
        clockControl->setCharacteristic(bleCharacteristic);
        m_selfUpdatingControls[getCharParamValue(newUuid , SUFFIX)] = clockControl;
        return clockControl;
    }
}

IntervalControl* EspBleControls::createIntervalControl(
    const std::string description,
    const uint16_t divisions,
    const uint16_t checkDelaySeconds,
    std::function<void(bool)> onIntervalToggle
) {
    if (doesCharCounterExists(CLOCK_UUID_SUFFIX)) {
        const std::string newUuid = generateCharUuid(INTRV_UUID_SUFFIX, divisions, checkDelaySeconds);
        const std::string initialValue(divisions / 8, 0);
        IntervalControl* intervalControl = new IntervalControl(description, checkDelaySeconds, &m_isDeviceAuthorised, onIntervalToggle);
        BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, false, intervalControl->getCallback());
        intervalControl->setCharacteristic(bleCharacteristic);
        m_selfUpdatingControls[getCharParamValue(newUuid , SUFFIX)] = intervalControl;
        return intervalControl;
    } else {
        createStringControl(description, 256, "There is no Clock control defined!\nPlease add one before creating an Interval control!", nullptr, nullptr);
        return nullptr;
    }
}

BooleanControl* EspBleControls::createSwitchControl(
    std::string description,
    std::string initialValue,
    Publisher<std::string>* publisher,
    std::function<void(std::string)> onSwitchToggle
) {
    const std::string newUuid = generateCharUuid(SWTCH_UUID_SUFFIX);
    BooleanControl* switchControl = new BooleanControl(description, publisher, &m_isDeviceAuthorised, onSwitchToggle);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, switchControl->getCallback());
    switchControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = switchControl;
    return switchControl;
}

BooleanControl* EspBleControls::createMomentaryControl(
    std::string description,
    std::string initialValue,
    Publisher<std::string>* publisher,
    std::function<void(std::string)> onButtonPressed
) {
    const std::string newUuid = generateCharUuid(MOMNT_UUID_SUFFIX);
    BooleanControl* momentaryControl = new BooleanControl(description, publisher, &m_isDeviceAuthorised, onButtonPressed);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, momentaryControl->getCallback());
    momentaryControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = momentaryControl;
    return momentaryControl;
}

IntControl* EspBleControls::createSliderControl(
    std::string description,
    short minValue,
    short maxValue,
    uint16_t steps,
    int initialValue,
    Publisher<int32_t>* publisher,
    std::function<void(int32_t)> onSliderMoved
) {
    const std::string newUuid = generateCharUuid(SLIDR_UUID_SUFFIX, minValue, maxValue, steps);
    IntControl* sliderControl = new IntControl(description, publisher, &m_isDeviceAuthorised, onSliderMoved);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, sliderControl->getCallback());
    sliderControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = sliderControl;
    return sliderControl;
}

IntControl* EspBleControls::createIntControl(
    std::string description,
    short minValue,
    short maxValue,
    int initialValue,
    Publisher<int32_t>* publisher,
    std::function<void(int32_t)> onIntReceived
) {
    const std::string newUuid = generateCharUuid(INTGR_UUID_SUFFIX, minValue, maxValue);
    IntControl* intControl = new IntControl(description, publisher, &m_isDeviceAuthorised, onIntReceived);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, intControl->getCallback());
    intControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = intControl;
    return intControl;
}

IntControl* EspBleControls::createAngleControl(
    std::string description,
    int initialValue,
    boolean isComapss,
    Publisher<int32_t>* publisher,
    std::function<void(int32_t)> onAngleChanged
) {
    const std::string newUuid = generateCharUuid(ANGLE_UUID_SUFFIX, isComapss);
    IntControl* angleControl = new IntControl(description, publisher, &m_isDeviceAuthorised, onAngleChanged);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, angleControl->getCallback());
    angleControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = angleControl;
    return angleControl;
}


FloatControl* EspBleControls::createFloatControl(
    std::string description,
    short minValue,
    short maxValue,
    float_t initialValue,
    Publisher<float_t>* publisher,
    std::function<void(float_t)> onFloatReceived
) {
    const std::string newUuid = generateCharUuid(FLOAT_UUID_SUFFIX, minValue, maxValue);
    FloatControl* floatControl = new FloatControl(description, publisher, &m_isDeviceAuthorised, onFloatReceived);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, floatControl->getCallback());
    floatControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = floatControl;
    return floatControl;
}

StringControl* EspBleControls::createStringControl(
    std::string description,
    uint16_t maxLength,
    std::string initialValue,
    Publisher<std::string>* publisher,
    std::function<void(std::string)> onTextReceived
) {
    const std::string newUuid = generateCharUuid(STRNG_UUID_SUFFIX, maxLength);
    StringControl* stringControl = new StringControl(description, publisher, &m_isDeviceAuthorised, onTextReceived);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, stringControl->getCallback());
    stringControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = stringControl;
    return stringControl;
}

StringControl* EspBleControls::createColorControl(
    std::string description,
    std::string initialValue,
    Publisher<std::string>* publisher,
    std::function<void(std::string)> onColorChanged
) {
    const std::string newUuid = generateCharUuid(COLOR_UUID_SUFFIX);
    StringControl* colorControl = new StringControl(description, publisher, &m_isDeviceAuthorised, onColorChanged);
    BLECharacteristic* bleCharacteristic = createCharacteristic(newUuid, description, initialValue, publisher != nullptr, colorControl->getCallback());
    colorControl->setCharacteristic(bleCharacteristic);
    if (publisher != nullptr) m_notifyingControls[getCharParamValue(newUuid , SUFFIX)] = colorControl;
    return colorControl;
}
