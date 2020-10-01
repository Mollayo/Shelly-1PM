#include <Arduino.h>

#include "mqtt.h"
#include "wifi.h"
#include "logging.h"
#include "config.h"
#include "light.h"
#include "switches.h"



namespace light {

// The light parameters
volatile uint8_t minBrightness = 0;   // brightness values in %
volatile uint8_t maxBrightness = 50;
volatile uint8_t brightness = 0;
uint8_t publishedBrightness = 0;      // The last brigthness value published to MQTT
uint8_t wattage = 0;

// For the auto-off timer
uint16_t autoOffDuration = 0;         // In seconds
unsigned long lastLightOnTime = 0;

// For blinking
unsigned long lastBlinkingTime = 0;
uint16_t blinkingDuration = 0;   // in ms; if 0, no blinking
bool blinkingLightState = false;


uint8_t &getWattage() {
  return wattage;
}


void sendCmdGetVersion() 
{
}

void sendCmdGetState() 
{
}

void setMinBrightness(const char* str)
{
  if (!helpers::isInteger(str, 3))
    return;

  // Make the conversion
  minBrightness = atoi (str);

  // Check if minBrightness between 0 and 200
  if (minBrightness < 0)
    minBrightness = 0;
  if (minBrightness > 20)
    minBrightness = 20;
}

void setMaxBrightness(const char* str)
{
  if (!helpers::isInteger(str, 3))
    return;

  // Make the conversion
  maxBrightness = atoi (str);

  // Check if maxBrightness between minBrightness and 1000
  if (maxBrightness < minBrightness)
    maxBrightness = minBrightness;
  if (maxBrightness > 100)
    maxBrightness = 100;
}

void setAutoOffTimer(const char* str)
{
  if (!helpers::isInteger(str, 3))
  {
    autoOffDuration=0;
    return;
  }

  autoOffDuration = atoi (str);
}

// To keep compatibility with the Dimmer 2
ICACHE_RAM_ATTR void setBrightness(volatile uint8_t b)
{
}
  
void lightOn()
{
  digitalWrite(LIGHT_RELAY, HIGH);
  brightness = maxBrightness;
}

void lightOff()
{
  digitalWrite(LIGHT_RELAY, LOW);
  brightness = minBrightness;
}

ICACHE_RAM_ATTR void lightToggle()
{
  // Current brighness closer to minBrightness than maxBrighness
  if ((brightness - minBrightness) < (maxBrightness - brightness))
  {
    digitalWrite(LIGHT_RELAY, HIGH);
    brightness = maxBrightness;
  }
  else
  {
    digitalWrite(LIGHT_RELAY, LOW);
    brightness = minBrightness;
  }
}

bool lightIsOn()
{
  return brightness>minBrightness;
}

void setBlinkingDuration(uint16_t duration)
{
  if (blinkingDuration!=duration)
  {
    logging::getLogStream().printf("light: change blinking duration to %d\n", duration);
    blinkingDuration=duration;
    if (duration == 0)
    {
      // stopping blinking
      // Comme back to the initial brightness level
      if ((brightness - minBrightness) < (maxBrightness - brightness))
        digitalWrite(LIGHT_RELAY, LOW);
      else
        digitalWrite(LIGHT_RELAY, HIGH);
    }
    else
    {
      // start blinking
      blinkingLightState=true;    // start with light on
      lastBlinkingTime=0;         // reset blinking timer
    }
  }
}

void setup() 
{
  pinMode(LIGHT_RELAY, OUTPUT);
}

void updateParams()
{
  logging::getLogStream().printf("light: updateParams\n");
  setMinBrightness(wifi::getParamValueFromID("minBrightness"));
  setMaxBrightness(wifi::getParamValueFromID("maxBrightness"));
  setAutoOffTimer(wifi::getParamValueFromID("autoOffTimer"));
}

void resetSTM32()
{
  
}

void handle()
{
  unsigned long currTime;

  // Check if there is new brightness value of brightness
  if (publishedBrightness != brightness)
  {
    publishedBrightness=brightness;

    // Set the auto-off timer for the brightness change
    if (brightness > minBrightness)
      lastLightOnTime  = millis();
    else
      lastLightOnTime = 0;

    // Publish the new value of the brightness
    const char* topic=wifi::getParamValueFromID("pubMqttBrightnessLevel");
    // If no topic, we do not publish
    if (topic!=NULL)
    {  
      char payload[5];
      sprintf(payload,"%d",brightness);
      mqtt::publishMQTT(topic,payload);
    }
  }
  
  // For blinking,  blinkingDuration==0 -> no blinking
  if (blinkingDuration>0)
  {
    currTime = millis();
    if (currTime - lastBlinkingTime > blinkingDuration)
    {
      lastBlinkingTime = currTime;
      // alternate on/off
      if (blinkingLightState)
      {
        logging::getLogStream().printf("light: light on for blinking\n");
        digitalWrite(LIGHT_RELAY, HIGH);
      }
      else
      {
        logging::getLogStream().printf("light: light off for blinking\n");
        digitalWrite(LIGHT_RELAY, LOW);
      }
      blinkingLightState=!blinkingLightState;
    }
  }

  // For the auto-off light
  if (autoOffDuration>0 && lastLightOnTime>0)
  {
    currTime = millis();
    // Make the conversion from ms to s
    if (currTime - lastLightOnTime > (autoOffDuration*1000))
    {
      logging::getLogStream().printf("light: auto-off light\n");
      lightOff();
      lastLightOnTime = 0;
    }
  }
}

} // namespace dimmer
