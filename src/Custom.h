#ifndef ESPEASY_CUSTOM_H
#define ESPEASY_CUSTOM_H

#ifdef USE_SECRETS_H
#include "../../secrets.h"
// Below lines are defined in secrets.h - file MUST be in .gitignore
// #warning "Secrets included in this firmware"
// #define DEFAULT_SSID                         "xxxxxxxx"         // Enter your network SSID
// #define DEFAULT_KEY                          "xxxxxxxx"         // Enter your network WPA key
// #define DEFAULT_ADMIN_USERNAME               "admin"
// #define DEFAULT_ADMIN_PASS                   "xxxx"
// #define DEFAULT_LATITUDE                     xx.xxxxxx          // Default Latitude  
// #define DEFAULT_LONGITUDE                    xx.xxxxxxf         // Default Longitude
#endif


// #include <Arduino.h>

/*
	To modify the stock configuration without changing the EspEasy.ino file :

	1) rename this file to "Custom.h" (It is ignored by Git)
	2) define your own settings below
    3) define USE_CUSTOM_H as a build flags. ie : export PLATFORMIO_BUILD_FLAGS="'-DUSE_CUSTOM_H'"
*/

// #define FEATURE_SETTINGS_ARCHIVE 1
#define FEATURE_I2CMULTIPLEXER 0
// #define FEATURE_TRIGONOMETRIC_FUNCTIONS_RULES 1
// #define PLUGIN_USES_ADAFRUITGFX // Used by Display plugins using Adafruit GFX library
// #define ADAGFX_ARGUMENT_VALIDATION  0 // Disable argument validation in AdafruitGFX_helper
// #define ADAGFX_SUPPORT_7COLOR  0 // Disable the support of 7-color eInk displays by AdafruitGFX_helper
// #define FEATURE_SEND_TO_HTTP 1 // Enable availability of the SendToHTTP command
// #define FEATURE_POST_TO_HTTP 1 // Enable availability of the PostToHTTP command
#define FEATURE_I2C_DEVICE_CHECK 0 // Disable the I2C Device check feature
#define FEATURE_I2C_GET_ADDRESS 0 // Disable fetching the I2C address from I2C plugins. Will be enabled when FEATURE_I2C_DEVICE_CHECK is enabled


// make the compiler show a warning to confirm that this file is inlcuded
// #warning "**** Using Settings from Custom.h File ***"

/*
 *  https://www.letscontrolit.com/wiki/index.php/Tutorial_Arduino_Firmware_Upload
 *  For 512k boards (like the blue colored ESP-01 or the ESP-12 or ESP-201): Select 64k SPIFFS.
 *  For 1M boards: Select 128k SPIFFS
 *  For 2M boards: Select 256k SPIFFS
 *  For 4M boards: Select 1M SPIFFS.
 *
 *  Max OTA filesize on 1MB = 616448 bytes.
 *
 */

/*
#######################################################################################################
Your Own Default Settings
#######################################################################################################

	You can basically ovveride ALL macro defined in ESPEasy.ino.
	Don't forget to first #undef each existing #define that you add below.
    But since this Custom.h is included before other defines are made, you don't have to undef a lot of defines.
*/

#define DATEREV "20240713a"
// #define BUILDDATEREV(pre, post) pre " - " DATEREV " - " post
#define BUILDDATEREV(pre, post) pre " - " DATEREV
// #warning "Custom config is being used"

#ifdef BUILD_GIT
# undef BUILD_GIT
#endif // ifdef BUILD_GIT

#ifdef ESP32
#define BUILD_GIT BUILDDATEREV("Custom IR for AC", "(ca 1275 kB)")
#else
#define BUILD_GIT BUILDDATEREV("Custom IR for AC", "(ca 880 kB)")
#endif

#define DEFAULT_NAME        "ESPeasy"                              // Enter your device friendly name
#define UNIT                0                                      // Unit Number
#define DEFAULT_DELAY       60                                     // Sleep Delay in seconds
#define DEFAULT_AP_KEY      "configesp"                            // Enter network WPA key for AP (config) mode


// --- Wifi Client Mode -----------------------------------------------------------------------------
#define DEFAULT_IP_BLOCK_LEVEL               0                     // 0: ALL_ALLOWED  1: LOCAL_SUBNET_ALLOWED  2:
#define DEFAULT_PROTOCOL                     1                     // Protocol used for controller communications
#define DEFAULT_WIFI_INCLUDE_HIDDEN_SSID     true                  // Allow to connect to hidden SSID APs
#define DEFAULT_SYNC_UDP_PORT                8266                  // Used for ESPEasy p2p. (IANA registered port: 8266)
#define DEFAULT_SYSLOG_FACILITY              22                    // local6
#define DEFAULT_SYSLOG_LEVEL                 4                     // Syslog Log Level WARN
#define DEFAULT_IPRANGE_LOW                  "0.0.0.0"               // Allowed IP range to access webserver
#define DEFAULT_IPRANGE_HIGH                 "255.255.255.255"       // Allowed IP range to access webserver

#define DEFAULT_USE_RULES                       true             // (true|false) Enable Rules?
#define DEFAULT_RULES_OLDENGINE                 true             // used for old builds

#define FEATURE_AUTO_DARK_MODE              1
#define FEATURE_RULES_EASY_COLOR_CODE       1
#define FEATURE_PLUGIN_STATS  				1    // Support collecting historic data + computing stats on historic data
// #define FEATURE_CHART_JS  1        // Support for drawing charts, like PluginStats historic data

#ifdef ESP8266
#  define PLUGIN_STATS_NR_ELEMENTS 16
#define FEATURE_TIMING_STATS                0
#endif // ifdef ESP8266
# ifdef ESP32
#  define PLUGIN_STATS_NR_ELEMENTS 64
#endif // ifdef ESP32

// #define WEBSERVER_NEW_UI    true
#define FEATURE_AUTO_DARK_MODE           1
#define SHOW_SYSINFO_JSON 1  //Enables the sysinfo_json page (by default is enabled when WEBSERVER_NEW_UI is enabled too)

/*
 #######################################################################################################
   Your Own selection of plugins and controllers
 #######################################################################################################
 */

#define CONTROLLER_SET_NONE
#define NOTIFIER_SET_NONE
#define PLUGIN_SET_NONE


/*
 #######################################################################################################
 ###########     Plugins
 #######################################################################################################
 */

// #define FEATURE_SERVO  1   // Uncomment and set to 0 to explicitly disable SERVO support


#define USES_P001   // Switch
#define USES_P002   // ADC
#define USES_P003   // Pulse
#define USES_P004   // Dallas
#define USES_P005   // DHT
// #define USES_P006   // BMP085
// #define USES_P007   // PCF8591
// #define USES_P008   // RFID
// #define USES_P009   // MCP

#define USES_P010   // BH1750
// #define USES_P011   // PME
// #define USES_P012   // LCD
#define USES_P013   // HCSR04
#define USES_P014   // SI7021
#define USES_P015   // TSL2561
// #define USES_P017   // PN532
// #define USES_P018   // Dust
// #define USES_P019   // PCF8574

#define USES_P020   // Ser2Net
// #define USES_P021   // Level
// #define USES_P022   // PCA9685
// #define USES_P023   // OLED
// #define USES_P024   // MLX90614
// #define USES_P025   // ADS1115
#define USES_P026   // SysInfo
// #define USES_P027   // INA219
#define USES_P028   // BME280
#define USES_P029   // Output

// #define USES_P031   // SHT1X
// #define USES_P032   // MS5611
#define USES_P033   // Dummy
// #define USES_P034   // DHT12
// #define USES_P036   // FrameOLED
// #define USES_P037   // MQTTImport
//   #define P037_MAPPING_SUPPORT 1 // Enable Value mapping support
//   #define P037_FILTER_SUPPORT  1 // Enable filtering support
//   #define P037_JSON_SUPPORT    1 // Enable Json support
// #define USES_P038   // NeoPixel
// #define USES_P039   // Environment - Thermocouple

// #define USES_P040   // RFID - ID12LA/RDM6300
// #define USES_P041   // NeoClock
// #define USES_P042   // Candle
// #define USES_P043   // ClkOutput
// #define USES_P044   // P1WifiGateway
// #define USES_P045   // MPU6050
// #define USES_P046   // VentusW266
// #define USES_P047   // I2C_soil_misture
// #define USES_P048   // Motoshield_v2
#define USES_P049      // MHZ19

// #define USES_P050   // TCS34725 RGB Color Sensor with IR filter and White LED
// #define USES_P051   // AM2320
// #define USES_P052   // SenseAir
// #define USES_P053   // PMSx003
// #define USES_P054   // DMX512
// #define USES_P055   // Chiming
// #define USES_P056   // SDS011-Dust
// #define USES_P057   // HT16K33_LED
// #define USES_P058   // HT16K33_KeyPad
// #define USES_P059   // Encoder

// #define USES_P060   // MCP3221
// #define USES_P061   // Keypad
// #define USES_P062   // MPR121_KeyPad
// #define USES_P063   // TTP229_KeyPad
// #define USES_P064   // APDS9960 Gesture
// #define USES_P065   // DRF0299
// #define USES_P066   // VEML6040
// #define USES_P067   // HX711_Load_Cell
// #define USES_P068   // SHT3x
// #define USES_P069   // LM75A

// #define USES_P070   // NeoPixel_Clock
// #define USES_P071   // Kamstrup401
// #define USES_P072   // HDC1080
// #define USES_P073   // 7DG
// #define USES_P074   // TSL2591
// #define USES_P075   // Nextion
// #define USES_P076   // HWL8012   in POW r1
// #define USES_P077   // CSE7766   in POW R2
// #define USES_P078   // Eastron Modbus Energy meters
// #define USES_P079   // Wemos Motoshield

// #define USES_P080   // iButton Sensor  DS1990A
// #define USES_P081   // Cron
// #define USES_P082   // GPS
// #define USES_P083   // SGP30
// #define USES_P084   // VEML6070
// #define USES_P085   // AcuDC24x
// #define USES_P086   // Receiving values according Homie convention. Works together with C014 Homie controller
// #define USES_P087   // Serial Proxy
#define USES_P088      // HeatpumpIR
// #define USES_P089   // Ping

// #define USES_P090   // CCS811
// #define USES_P091   // SerSwitch
// #define USES_P092   // DLbus
#define USES_P093      // MitsubishiHP
// #define USES_P094   // CULReader
// #define USES_P095   // ILI9341
// #define USES_P096   // eInk
// #define USES_P097   // ESP32Touch
// #define USES_P098   // 
// #define USES_P099   // XPT2046 touchscreen

// #define USES_P100   // DS2423 counter
// #define USES_P101   // WakeOnLan
// #define USES_P102   // PZEM004Tv3
// #define USES_P103   // Atlas Scientific EZO Sensors (pH, ORP, EZO, DO)
// #define USES_P104   // MAX7219 dotmatrix
// #define USES_P105   // AHT10/20/21
#define USES_P106      // BME680
// #define USES_P107   // Si1145
// #define USES_P109   // ThermoOLED

// #define USES_P110   // VL53L0X Time of Flight sensor
// #define USES_P111   // RF522 RFID reader
// #define USES_P112   // AS7265x
// #define USES_P113   // VL53L1X ToF
// #define USES_P114   // VEML6075
// #define USES_P115   // MAX1704x
// #define USES_P116   // ST77xx
// #define USES_P117   // SCD30
// #define USES_P118   // Itho
// #define USES_P119   // ITG3205 Gyro
// #define USES_P120   // ADXL345 I2C Acceleration / Gravity
// #define USES_P124   // I2C MultiRelay
// #define USES_P125   // ADXL345 SPI Acceleration / Gravity
// #define USES_P126   // 74HC595 Shift register
// #define USES_P127   // CDM7160
// #define USES_P129   // 74HC165 Input shiftregisters
// #define USES_P131   // NeoMatrix
// #define USES_P132   // INA3221
// #define USES_P133   // LTR390 UV
// #define USES_P134   // A02YYUW
// #define USES_P135   // SCD4x
// #define P135_FEATURE_RESET_COMMANDS  1 // Enable/Disable quite spacious (~950 bytes) 'selftest' and 'factoryreset' subcommands
// #define USES_P138   // IP5306
// #define USES_P141   // PCD8544 Nokia 5110 LCD
// #define USES_P143   // I2C Rotary encoders
// #define P143_FEATURE_INCLUDE_M5STACK      0 // Enabled by default, can be turned off here
// #define P143_FEATURE_INCLUDE_DFROBOT      0 // Enabled by default, can be turned off here
// #define P143_FEATURE_COUNTER_COLORMAPPING 0 // Enabled by default, can be turned off here

// #define USES_P128   // NeoPixelBusFX
// #define P128_USES_GRB  // Default
// #define P128_USES_GRBW // Select 1 option, only first one enabled from this list will be used
// #define P128_USES_RGB
// #define P128_USES_RGBW
// #define P128_USES_BRG
// #define P128_USES_RBG
// #define P128_ENABLE_FAKETV 1 // Enable(1)/Disable(0) FakeTV effect, disabled by default on ESP8266 (.bin size issue), enabled by default on ESP32


// #define USES_P108   // DDS238-x ZN Modbus energy meters


/*
 #######################################################################################################
 ###########     Controllers
 #######################################################################################################
 */


#define USES_C001   // Domoticz HTTP
#define USES_C002   // Domoticz MQTT
// #define USES_C003   // Nodo telnet
// #define USES_C004   // ThingSpeak
#define USES_C005   // Home Assistant (openHAB) MQTT
#define USES_C006   // PiDome MQTT
// #define USES_C007   // Emoncms
#define USES_C008   // Generic HTTP
// #define USES_C009   // FHEM HTTP
// #define USES_C010   // Generic UDP
#define USES_C011   // Generic HTTP Advanced
// #define USES_C012   // Blynk HTTP
#define USES_C013   // ESPEasy P2P network
#define USES_C014   // homie 3 & 4dev MQTT
// #define USES_C015   // Blynk
// #define USES_C016   // Cache controller
// #define USES_C017   // Zabbix
// #define USES_C018   // TTN/RN2483


/*
 #######################################################################################################
 ###########     Notifiers
 #######################################################################################################
 */


// #define USES_N001   // Email
// #define USES_N002   // Buzzer



#endif // ESPEASY_CUSTOM_H
