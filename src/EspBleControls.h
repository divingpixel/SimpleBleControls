#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Arduino.h>
#include <Preferences.h>

#define SERVICE_UUID    "e5932b1e-c0de-da7a-7472-616e73666572" // SHOULD USE THIS SERVICE UUID OTHERWISE THE APP WILL FILTER OUT THE DEVICE
#define PREFERENCES_ID  "control_values"
#define DAY_MINUTES 1440

// The characteristic descriptor contains the label of the control
// The UUID should describe the control type and parameters, following these rules: 
// The first part, let's call it SID, is "e5932b1e" should be at the start of all characteristics (32 bits),
// The middle 3 groups (of 16 bits each) should store parameters of the control (ex. for slider min/max/steps)
// The last part, let's call it CID, helps the app identify the control type
// The last byte is 0x00 but in case of multiple controls of the same type the last byte should represent the number of instances

#define CHAR_UUID_PREFIX       "e5932b1e"
// NICE TO HAVE
//      DEPAD (D-pad):intensity 0..255 -> joystick like, sends 2 values : direction in degrees and intensity (float/string like 270,255)
//      DRVNG (driving controls): -> control for vehicle driving (3 gradients vertically) sends direction left/right (-255..255), acceleration forward/backward (255..-255), brake (0..255)
//      DIPSW (dip switch):2-16 -> 1 switch/bit
//      BATRY (battery):0..100 -> Battery Indicator
//      SENTR (sensor value + trigger): -> Display a sensor value and a slider to set a threshold value. Find a way to combine 2 values (a string?)
#define CLOCK_UUID_SUFFIX      "636c6f636b" // SID-0000-0000-0000-CID+count
#define INTRV_UUID_SUFFIX      "696e747276" // SID-divisions-0000-0000-CID+count -> divisions multiple of 24, min 24, max 1440
#define SWTCH_UUID_SUFFIX      "7377746368" // SID-0000-0000-0000-CID+count
#define SLIDR_UUID_SUFFIX      "736c696472" // SID-minValue-maxValue-steps-CID+count -> min/max between -32767..32767 if min/max 0 full 32bit int
#define STRNG_UUID_SUFFIX      "7374726e67" // SID-size-0000-0000-CID+count -> size between 1..512
#define INTGR_UUID_SUFFIX      "696e746772" // SID-minValue-maxValue-0000-CID+count -> min/max between -32767..32767 if min/max 0 full 32bit int
#define FLOAT_UUID_SUFFIX      "666c6f6174" // SID-minValue-maxValue-0000-CID+count -> min/max between -32767..32767 if min/max 0 full 32bit float
#define ANGLE_UUID_SUFFIX      "616e676c65" // SID-isCompass-0000-0000-CID+count
#define MOMNT_UUID_SUFFIX      "6d6f6d6e74" // SID-0000-0000-0000-CID+count
#define COLOR_UUID_SUFFIX      "636f6c6f72" // SID-0000-0000-0000-CID+count

// --------------------------------------------------------------------------------------------------------------------

//TODO here the Observer classes

// --------------------------------------------------------------------------------------------------------------------

class CharacteristicCallback : public BLECharacteristicCallbacks {
public:
    CharacteristicCallback(void (*func)(uint32_t), const std::string charDescription, bool* isDeviceAuthorised);
    CharacteristicCallback(void (*func)(int32_t), const std::string charDescription, bool* isDeviceAuthorised);
    CharacteristicCallback(void (*func)(float_t), const std::string charDescription, bool* isDeviceAuthorised);
    CharacteristicCallback(void (*func)(std::string), const std::string charDescription, bool* isDeviceAuthorised);
    CharacteristicCallback(void (*func)(std::vector<bool>), const size_t dataSize, const std::string charDescription, bool* isDeviceAuthorised);

    const char getValueType();
    void executeCallback(BLECharacteristic* pChar, bool saveValues);

    void onWrite(BLECharacteristic* pChar) override {
         if (*m_pIsDeviceAuthorised) executeCallback(pChar, true);
    }

private: 
    void (*m_pUIntFunc)(uint32_t) = nullptr;
    void (*m_pIntFunc)(int32_t) = nullptr;
    void (*m_pFloatFunc)(float_t) = nullptr;
    void (*m_pStringFunc)(std::string) = nullptr;
    void (*m_pBoolsFunc)(std::vector<bool>) = nullptr;
    size_t m_dataSize;
    std::string m_callerName = "";
    bool* m_pIsDeviceAuthorised;
};

// --------------------------------------------------------------------------------------------------------------------

class EspBleControls {
public:
    EspBleControls(const std::string deviceName, const uint32_t passkey = 0);

    bool isConnected();
    bool isAuthorised();
    void startService();
    void clearPrefs();
    uint32_t getLastNotificationTimeStamp(const std::string charUuid); // Gets the last time a notification was sent for the corresponding characteristic

    //A control that displays the microcontroller RTC value. Data is sent as long, received as long (unix epoch time).
    //It can have only one instance, and it's reccomended to have a method to set the RTC of the microcontroller onValueReceived.
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createClockControl(
        const std::string description,
        uint32_t initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const uint32_t)
    );
    
    //A switch where data is sent and received as string with the values "ON"/"OFF"
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createSwitchControl(
        const std::string description,
        const std::string initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const std::string)
    );
    
    //A momentary button, sends "ON" if NO or "OFF" if NC when pressed and "OFF" if NO and "ON" if NC when released.
    //Initial value determines if is NO or NC. If the initial value is "OFF" it will be NO and NC if is "ON".
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createMomentaryControl(
        const std::string description,
        const std::string initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const std::string)
    );
    
    //A slider for int values between 0 and 65535. The steps will divide the slider and the cursor will jump to these values.
    //For fine control leave the steps to 0.
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createSliderControl(
        const std::string description,
        const short minValue,
        const short maxValue,
        const uint16_t steps,
        const int initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const int32_t)
    );
    
    //Will display a text field with an integer value. If the minimum and maximum values are set to 0 the value will be unconstrained (32 bits).
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createIntControl(
        const std::string description,
        const short minValue,
        const short maxValue,
        const int initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const int32_t)
    );
    
    //Will display a text field with a float value. The minimum and maximum values are integers but will be displayed as floats.
    //If the minimum and maximum values are set to 0 the value will be unconstrained (32 bits).
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createFloatControl(
        const std::string description,
        const short minValue,
        const short maxValue,
        const float_t initialValue, 
        const boolean shouldNotify,
        void (onValueReceived)(const float_t)
    );
    
    //Will display a text field where text length can be constrained between 0 and 512 characters.
    //If the maxLength is left 0, the maxLength will be set to the length of the inital value.
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createStringControl(
        const std::string description,
        const uint16_t maxLength,
        const std::string initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const std::string)
    );
    
    //A control that displays and sets an angle with integer values from 0 to 359.
    //If isCompass is set to true will display the cardinal points also.
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createAngleControl(
        const std::string description,
        const uint32_t initialValue,
        const boolean isCompass,
        const boolean shouldNotify,
        void (onValueReceived)(const uint32_t)
    );
    
    //A control to set a color in RGB format. Sends and receives a string representing the hexadecimal value of the color.
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createColorControl(
        const std::string description,
        const std::string initialValue,
        const boolean shouldNotify,
        void (onValueReceived)(const std::string)
    );
    
    //24 hours ON/OFF interval setter with binary value for each division. Sends and receives a string where each bit is an division.
    //Divisions will be multiple of 24, with minimum value 24 and maximum value 1440.
    //If onValueReceived function is nullptr then the value will be read only.
    BLECharacteristic* createIntervalControl(
        const std::string description,
        const uint16_t divisions,
        const boolean shouldNotify,
        void (onValueReceived)(std::vector<bool>) //TODO this also should be a bool vector
    );

private:
    template <typename ValueType> BLECharacteristic* createCharacteristic(
        const std::string uuid,
        const std::string description,
        ValueType initialValue,
        const size_t valueSize,
        const boolean shouldNotify, // TODO : for notifications add a Publisher, and this should subscribe and notify when the Publisher is not null?
        CharacteristicCallback* callback
    );
    void setBleSecurity();
    void startAdvertising();
    void setLastNotificationTimeStamp(const std::string charUuid); 
    const std::string generateCharUuid(const std::string suffix, const int16_t val1, const int16_t val2, const int16_t val3);
    const uint16_t getCharIndex(const std::string charId);
    const boolean characteristicCounterExists(const std::string charId);
    std::map<std::string, uint16_t> m_charsCounter;
    std::map<std::string, uint32_t> m_charLastNotificationTimeStamp; // this will be used to store the last notification times for characteristics using the characteristic FULL uuid
    uint32_t m_pin;
    bool m_isDeviceAuthorised;
    bool m_isDeviceConnected;
    BLEServer* m_pServer;
    BLEService* m_pService;
};

// --------------------------------------------------------------------------------------------------------------------

class ServerCallback : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        m_onDeviceConnection(true);
        //TODO resend all the characteristic values???
        printf("ServerCallback - Device Connecting\n");
    };

    void onDisconnect(BLEServer* pServer) {
        m_onDeviceConnection(false);
        m_onDeviceAuthentication(false);
        printf("ServerCallback - Device Disconnected\n");
        vTaskDelay(500);
        BLEDevice::startAdvertising();
    }

public:
    ServerCallback(std::function<void(bool)> onDeviceConnection, std::function<void(bool)> onDeviceAuthentication);

private:
    std::function<void(bool)> m_onDeviceConnection;
    std::function<void(bool)> m_onDeviceAuthentication;
};

// --------------------------------------------------------------------------------------------------------------------

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
            printf("SecurityCallback - Authentication Success\n");
        } else {
            m_onDeviceAuthentication(false);
            printf("SecurityCallback - Authentication Failure\n");
            m_pServer->removePeerDevice(m_pServer->getConnId(), true);
            vTaskDelay(500);
            BLEDevice::startAdvertising();
        }
    };

public:
    SecurityCallback(std::function<void(bool)> onDeviceAuthentication, BLEServer* pServer);

private:
    BLEServer* m_pServer;
    std::function<void(bool)> m_onDeviceAuthentication;
};
