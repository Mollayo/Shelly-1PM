#ifndef CONFIG
#define CONFIG


// For the Shelly Dimmer 2
/*
#define SHELLY_BUILTIN_LED 16
#define SHELLY_SW1 14
#define SHELLY_SW2 12
#define SHELLY_BUILTIN_SWITCH 13

#define STM_NRST_PIN 5
#define STM_BOOT0_PIN 4
*/

// For the Shelly 1PM
#define SHELLY_BUILTIN_LED 0
#define SHELLY_SW1 4
//#define SHELLY_SW2 -1           // Not applicable
//#define SHELLY_SW0 2            // Built-in switch -> quite unstable for the Shelly 1PM
#define LIGHT_RELAY 15            // Relay for swtiching on/off the light


  
namespace helpers {

  bool isInteger(const char* str, uint8_t maxLength=10);
  bool convertToInteger(const char* str, uint16_t &val, uint8_t maxLength=10);
  const char* hexToStr(const uint8_t *s, uint8_t len);
}

#endif
