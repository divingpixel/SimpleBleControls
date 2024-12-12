#ifndef EspBleControls_h
#define EspBleControls_h

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <ESP32Time.h>
#include <bitset>
#include <Preferences.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define SERVICE_UUID    "e5932b1e-c0de-da7a-7472-616e73666572" // SHOULD USE THIS SERVICE UUID OTHERWISE THE APP WILL FILTER OUT THE DEVICE
#define PREFERENCES_ID  "control_values"
#define DAY_MINUTES     1440
#define DAY_HOURS       24

// The characteristic descriptor contains the label of the control
// The UUID should describe the control type and parameters, following these rules: 
// The first part, let's call it ID, is "e5932b1e" should be at the start of all characteristics (32 bits) (I should find a better use of these 32 bits)
// The middle 3 groups (of 16 bits each) can store parameters of the control (ex. for slider min/max/steps)
// The last part, let's call it CID, helps the app identify the control type
// The last byte represents the number of instances of that control

#define CHAR_UUID_PREFIX       "e5932b1e"

#define CLRPF_UUID_SUFFIX      "636c727066" // ID for a unique characteristic that is used to clear preferences and reset
#define CLOCK_UUID_SUFFIX      "636c6f636b" // ID-updateInterval-0000-0000-CID+count
#define INTRV_UUID_SUFFIX      "696e747276" // ID-divisions-updateInterval-0000-CID+count -> divisions multiple of 24, min 24, max 1440
#define SWTCH_UUID_SUFFIX      "7377746368" // ID-0000-0000-0000-CID+count
#define SLIDR_UUID_SUFFIX      "736c696472" // ID-minValue-maxValue-steps-CID+count -> min/max between -32767..32767
#define STRNG_UUID_SUFFIX      "7374726e67" // ID-size-0000-0000-CID+count -> size between 1..512
#define INTGR_UUID_SUFFIX      "696e746772" // ID-minValue-maxValue-0000-CID+count -> min/max between -32767..32767 if min/max 0 full 32bit int
#define FLOAT_UUID_SUFFIX      "666c6f6174" // ID-minValue-maxValue-0000-CID+count -> min/max between -32767..32767 if min/max 0 full 32bit float
#define ANGLE_UUID_SUFFIX      "616e676c65" // ID-isCompass-0000-0000-CID+count
#define MOMNT_UUID_SUFFIX      "6d6f6d6e74" // ID-type-0000-0000-CID+count -> type 0 for NO 1 for NC
#define COLOR_UUID_SUFFIX      "636f6c6f72" // ID-0000-0000-0000-CID+count

enum UuidSection {
    PREFIX, PARAM1, PARAM2, PARAM3, SUFFIX, CHARID
};

enum CallbackType {
    NONE, INTEGER, FLOAT, STRING, VECTOR 
};

// -----------------------------------------------------> CHARACTERISTIC CALLBACK CLASS <---------------------------------------------------
// TODO : It would be nice to have the one generic constructor to create the callback

class CharacteristicCallback : public BLECharacteristicCallbacks {
public:
    CharacteristicCallback(std::function<void(long)>, bool* isDeviceAuthorised);
    CharacteristicCallback(std::function<void(float)>, bool* isDeviceAuthorised);
    CharacteristicCallback(std::function<void(std::string)>, bool* isDeviceAuthorised);
    CharacteristicCallback(std::function<void(std::vector<char>)>, bool* isDeviceAuthorised);

    const CallbackType getValueType();
    void executeCallback(BLECharacteristic* pChar, bool saveValues);

    void onWrite(BLECharacteristic* pChar) override {
        if (*m_pIsDeviceAuthorised) executeCallback(pChar, true);
    }

private: 
    struct SaveDataParams {
        BLECharacteristic* pChar;
        CallbackType type;
    } m_saveDataParams;
    static void saveValuesTask(void* params);
    TaskHandle_t saveValuesTaskHandle = NULL;
    std::function<void(long)> m_pIntFunc= nullptr;
    std::function<void(float)> m_pFloatFunc = nullptr;
    std::function<void(std::string)> m_pStringFunc = nullptr;
    std::function<void(std::vector<char>)> m_pVectFunc = nullptr;
    bool* m_pIsDeviceAuthorised;
};

// -----------------------------------------------------> CONTOL OBSERVER CLASS <-----------------------------------------------------------

class BLEControl {
public:
    virtual void update() = 0;
    virtual void setCharacteristic(BLECharacteristic* bleCharacteristic) = 0;
    virtual BLECharacteristic* getCharacteristic() = 0;
    virtual CharacteristicCallback* getCallback() = 0;
};

// -----------------------------------------------------> CONTROL PUBLISHER CLASS <-----------------------------------------------------------------

template <typename T>
class ControlPublisher {
private:
    T m_value;
    std::vector<BLEControl*> m_observers;
    std::function<void(T)> m_action;
    BLEControl* m_sender;
    
public:
    void subscribe(BLEControl* observer) {
        m_observers.push_back(observer);
        observer->update();
    }

    T getValue() {
        return m_value;
    }

    void doOnSet(std::function<void(T)> action) {
        m_action = action;
    }

    void setValue(T value, BLEControl* sender) {
        if (value != m_value) {
            m_value = value;
            m_sender = sender;
            for (BLEControl* observer : m_observers) {
                if(observer != m_sender) observer->update();
            }
            if (m_action != nullptr) m_action(value);
        }
    }
};

// -----------------------------------------------------> INTERVAL CONTROL CLASS <----------------------------------------------------------

class IntervalControl : public BLEControl {
public:
    IntervalControl(const uint16_t checkDelaySeconds, bool* isDeviceAuthorised, std::function<void(bool)> onIntervalToggle);
    CharacteristicCallback* getCallback() override { return new CharacteristicCallback(m_callback, m_isDeviceAuthorised); };
    void setCharacteristic(BLECharacteristic* bleCharacteristic) override { m_bleCharacteristic = bleCharacteristic; };
    BLECharacteristic* getCharacteristic() override { return m_bleCharacteristic; };
    void update() override;
private:
    ESP32Time espClock;
    BLECharacteristic* m_bleCharacteristic;
    std::vector<char> m_intervals;
    bool* m_isDeviceAuthorised;
    uint16_t m_checkDelaySeconds;
    uint32_t m_lastUpdateTimeStamp;
    std::function<void(bool)> m_onIntervalToggle;
    std::function<void(std::vector<char>)> m_callback;
};

// ------------------------------------------------------> CLOCK CONTROL CLASS <------------------------------------------------------------

class ClockControl : public BLEControl {
public:
    ClockControl(uint32_t initialValue, const uint16_t notifyDelaySeconds, bool* isDeviceAuthorised, std::function<void(uint32_t)> onTimeSet);
    CharacteristicCallback* getCallback() override { return new CharacteristicCallback(m_callback, m_isDeviceAuthorised); };
    void setCharacteristic(BLECharacteristic* bleCharacteristic) override { m_bleCharacteristic = bleCharacteristic; };
    BLECharacteristic* getCharacteristic() override { return m_bleCharacteristic; };
    void update() override;
private:
    ESP32Time espClock;
    BLECharacteristic* m_bleCharacteristic;
    uint16_t m_notifyDelaySeconds;
    uint32_t m_lastUpdateTimeStamp;
    bool* m_isDeviceAuthorised;
    std::function<void(int32_t)> m_onTimeSet, m_callback;
};

// ------------------------------------------------------> BOOLEAN CONTROL CLASS <----------------------------------------------------------

class BooleanControl : public BLEControl {
public:
    BooleanControl(ControlPublisher<std::string>* publisher, bool* isDeviceAuthorised, std::function<void(std::string)> onChange);
    CharacteristicCallback* getCallback() override { return new CharacteristicCallback(m_callback, m_isDeviceAuthorised); };
    void setCharacteristic(BLECharacteristic* bleCharacteristic) override { 
        m_bleCharacteristic = bleCharacteristic;
        if (m_publisher != nullptr) m_publisher->subscribe(this); 
    };
    BLECharacteristic* getCharacteristic() override { return m_bleCharacteristic; };
    void update() override;
 private:
    BLECharacteristic* m_bleCharacteristic;
    ControlPublisher<std::string>* m_publisher;
    std::string m_initialValue;
    uint32_t m_lastNotificationTimeStamp;
    bool* m_isDeviceAuthorised;
    std::function<void(std::string)> m_onChange, m_callback;
};

// ------------------------------------------------------> INT CONTROL CLASS <--------------------------------------------------------------
// TODO : It would be nice to have the bool, int, float, string classes merged into one generic class

class IntControl : public BLEControl {
public:
    IntControl(ControlPublisher<int32_t>* publisher, bool* isDeviceAuthorised, std::function<void(int32_t)> onChange);
    CharacteristicCallback* getCallback() override { return new CharacteristicCallback(m_callback, m_isDeviceAuthorised); };
    void setCharacteristic(BLECharacteristic* bleCharacteristic) override { 
        m_bleCharacteristic = bleCharacteristic;
        if (m_publisher != nullptr) m_publisher->subscribe(this); 
    };
    BLECharacteristic* getCharacteristic() override { return m_bleCharacteristic; };
    void update() override;
 private:
    BLECharacteristic* m_bleCharacteristic;
    ControlPublisher<int32_t>* m_publisher;
    uint32_t m_lastNotificationTimeStamp;
    bool* m_isDeviceAuthorised;
    std::function<void(int32_t)> m_onChange, m_callback;
};

// ------------------------------------------------------> FLOAT CONTROL CLASS <------------------------------------------------------------

class FloatControl : public BLEControl {
public:
    FloatControl(ControlPublisher<float_t>* publisher, bool* isDeviceAuthorised, std::function<void(float_t)> onChange);
    CharacteristicCallback* getCallback() override { return new CharacteristicCallback(m_callback, m_isDeviceAuthorised); };
    void setCharacteristic(BLECharacteristic* bleCharacteristic) override { 
        m_bleCharacteristic = bleCharacteristic;
        if (m_publisher != nullptr) m_publisher->subscribe(this); 
    };
    BLECharacteristic* getCharacteristic() override { return m_bleCharacteristic; };
    void update() override;
 private:
    BLECharacteristic* m_bleCharacteristic;
    ControlPublisher<float_t>* m_publisher;
    uint32_t m_lastNotificationTimeStamp;
    bool* m_isDeviceAuthorised;
    std::function<void(float_t)> m_onChange, m_callback;
};

// ------------------------------------------------------> STRING CONTROL CLASS <------------------------------------------------------------

class StringControl : public BLEControl {
public:
    StringControl(ControlPublisher<std::string>* publisher, bool* isDeviceAuthorised, std::function<void(std::string)> onChange);
    CharacteristicCallback* getCallback() override { return new CharacteristicCallback(m_callback, m_isDeviceAuthorised); };
    void setCharacteristic(BLECharacteristic* bleCharacteristic) override { 
        m_bleCharacteristic = bleCharacteristic;
        if (m_publisher != nullptr) m_publisher->subscribe(this); 
    };
    BLECharacteristic* getCharacteristic() override { return m_bleCharacteristic; };
    void update() override;
 private:
    BLECharacteristic* m_bleCharacteristic;
    ControlPublisher<std::string>* m_publisher;
    uint32_t m_lastNotificationTimeStamp;
    bool* m_isDeviceAuthorised;
    std::function<void(std::string)> m_onChange, m_callback;
};

// ------------------------------------------------------> ESP BLE CONTROLS FACTORY CLASS <-------------------------------------------------

class EspBleControlsFactory {
public:
    EspBleControlsFactory(const std::string deviceName, const uint32_t passkey = 0);
    void startService();
    void updateControls();

    //A control that displays the microcontroller RTC value. Data is sent as long, received as long (unix epoch time).
    //It can have only one instance, and it's reccomended to have a method to set the RTC of the microcontroller onValueReceived.
    //If onTimeSet function is nullptr then the value will be read only.
    ClockControl* createClockControl(
        const std::string description,
        uint32_t initialValue,
        uint16_t notifyDelaySeconds,
        std::function<void(uint32_t)> onTimeSet
    );
    
    //24 hours ON/OFF interval setter with binary value for each division.
    //Divisions must be multiple of 24, with minimum value 24 and maximum value 1440.
    //If checkDelaySeconds is 0 the onIntervalToggle function will not be executed
    IntervalControl* createIntervalControl(
        const std::string description,
        const uint16_t divisions, 
        const uint16_t checkDelaySeconds,
        std::function<void(bool)> onIntervalToggle
    );

    //A switch where data is sent and received as string with the values "ON"/"OFF"
    //If onSwitchToggle function is nullptr then the value will be read only.
    BooleanControl* createSwitchControl(
        const std::string description,
        const std::string initialValue,
        ControlPublisher<std::string>* publisher,
        std::function<void(std::string)> onSwitchToggle
    );
    
    //A momentary button, sends "ON" if NO or "OFF" if NC when pressed and "OFF" if NO and "ON" if NC when released.
    //Initial value determines if is NO or NC. If the initial value is "OFF" it will be NO and NC if is "ON".
    //If onValueReceived function is nullptr then the value will be read only.
    BooleanControl* createMomentaryControl(
        const std::string description,
        const std::string initialValue,
        bool isNC, 
        ControlPublisher<std::string>* publisher,
        std::function<void(std::string)> onButtonPressed
    );
    
    //Will display a text field with an integer value. If the minimum and maximum values are set to 0 the value will be unconstrained (32 bits).
    //If onValueReceived function is nullptr then the value will be read only.
    IntControl* createIntControl(
        const std::string description,
        const short minValue,
        const short maxValue,
        const int initialValue,
        ControlPublisher<int32_t>* publisher,
        std::function<void(int32_t)> onIntReceived
    );

    //A slider for int values between -32767 and 32767. The steps will divide the slider and the cursor will jump to these values.
    //For fine control leave the steps to 0.
    //If onValueReceived function is nullptr then the value will be read only. I can't think of a use case for this but ... it is what it is.
    IntControl* createSliderControl(
        const std::string description,
        const short minValue,
        const short maxValue,
        const uint16_t steps,
        const int initialValue,
        ControlPublisher<int32_t>* publisher,
        std::function<void(int32_t)> onSliderMoved
    );

    //A control that displays and sets an angle with integer values from 0 to 359.
    //If isCompass is set to true will display the cardinal points also.
    //If onValueReceived function is nullptr then the value will be read only.
    IntControl* createAngleControl(
        const std::string description,
        const int initialValue,
        const boolean isCompass,
        ControlPublisher<int32_t>* publisher,  
        std::function<void(int32_t)> onAngleChanged
    );
    
    //Will display a text field with a float value. The minimum and maximum values are integers but will be displayed as floats.
    //If the minimum and maximum values are set to 0 the value will be unconstrained (32 bits).
    //If onValueReceived function is nullptr then the value will be read only.
    FloatControl* createFloatControl(
        const std::string description,
        const short minValue,
        const short maxValue,
        const float_t initialValue, 
        ControlPublisher<float_t>* publisher,
        std::function<void(float_t)> onFloatReceived
    );
    
    //Will display a text field where text length can be constrained between 0 and 512 characters.
    //If the maxLength is left 0, the maxLength will be set to the length of the inital value.
    //If onValueReceived function is nullptr then the value will be read only.
    StringControl* createStringControl(
        const std::string description,
        const uint16_t maxLength,
        const std::string initialValue,
        ControlPublisher<std::string>* publisher,
        std::function<void(std::string)> onTextReceived
    );
    
    //A control to set a color in RGB format. Sends and receives a string representing the hexadecimal value of the color.
    //If onValueReceived function is nullptr then the value will be read only.
    StringControl* createColorControl(
        const std::string description,
        const std::string initialValue,
        ControlPublisher<std::string>* publisher,
        std::function<void(std::string)> onColorChanged
    );

private:
    template <typename ValueType> BLECharacteristic* createCharacteristic(
        const std::string uuid,
        const std::string description,
        ValueType initialValue,
        const boolean shouldNotify,
        CharacteristicCallback* callback
    );
    void setBleSecurity();
    void startAdvertising();
    void createClearPrefsAndResetControl();
    void restoreValue(BLECharacteristic* characteristic, const std::string uuid, CharacteristicCallback* callback);
    const std::string generateCharUuid(const std::string suffix, const int16_t val1, const int16_t val2, const int16_t val3);
    const uint16_t getCharCounterIndex(const std::string charId);
    const boolean doesCharCounterExists(const std::string charId);
    std::map<std::string, uint16_t> m_charsCounter;
    std::vector<BLEControl*> m_selfUpdatingControls;
    uint32_t m_pin;
    bool m_isDeviceAuthorised;
    bool m_isDeviceConnected;
    BLEServer* m_pServer;
    BLEService* m_pService;
};

// -----------------------------------------------------> SERVER CALLBACK <----------------------------------------------------------------

class ServerCallback : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        m_onDeviceConnection(true);
    };

    void onDisconnect(BLEServer* pServer) {
        m_onDeviceConnection(false);
        vTaskDelay(500);
        BLEDevice::startAdvertising();
    }

public:
    ServerCallback(std::function<void(bool)> onDeviceConnection){
       m_onDeviceConnection = onDeviceConnection;
    };

private:
    std::function<void(bool)> m_onDeviceConnection;
};

// -----------------------------------------------------> SECURITY CALLBACK <---------------------------------------------------------------

class SecurityCallback : public BLESecurityCallbacks {
    uint32_t onPassKeyRequest() {
        return 0;
    }

    void onPassKeyNotify(uint32_t pass_key) {}

    bool onConfirmPIN(uint32_t pass_key) {
        vTaskDelay(5000);
        return true;
    }

    bool onSecurityRequest() {
        return true;
    }

    void onAuthenticationComplete(esp_ble_auth_cmpl_t cmpl) {
        if (cmpl.success) {
            m_onDeviceAuthentication(true);
        } else {
            m_onDeviceAuthentication(false);
            vTaskDelay(500);
            BLEDevice::startAdvertising();
        }
    };

public:
    SecurityCallback(std::function<void(bool)> onDeviceAuthentication){
        m_onDeviceAuthentication = onDeviceAuthentication;
    };

private:
    std::function<void(bool)> m_onDeviceAuthentication;
};

#endif
