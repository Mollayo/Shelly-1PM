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
volatile uint8_t maxBrightness = 100;
volatile uint8_t brightness = 0;
uint8_t publishedBrightness = 0;      // The last brigthness value published to MQTT
uint8_t wattage = 0;

// For the auto-off timer
uint16_t autoOffDuration = 0;         // In seconds
volatile unsigned long lastLightOnTime = 0;
volatile bool lightAutoTurnOffDisable = false;

// For blinking
unsigned long startBlinkingTime = 0;
uint16_t blinkingTimerDuration = 5;  // In seconds
bool blinking = false;

// For the blinking pattern
unsigned long lastBlinkingLightStateTime = 0;
bool blinkingLightState = false;
uint16_t blinkingPattern[10] = {500, 500, 0, 0, 0, 0, 0, 0, 0, 0};   // in ms; if 0, no blinking



uint8_t &getWattage() {
  return wattage;
}

void STM32reset()
{
}


void setBlinkingDuration(const char* durationStr)
{
  uint16_t newDuration=5;
  if (helpers::convertToInteger(durationStr, newDuration))
  {
    if (blinkingTimerDuration != newDuration)
    {
      logging::getLogStream().printf("light: change blinking duration to %d\n", newDuration);
      blinkingTimerDuration = newDuration;
    }
  }
  else
  {
    logging::getLogStream().printf("light: failed to change blinking duration to %s\n", durationStr);
    blinkingTimerDuration = 5;
  }

}

void startBlinking()
{
  logging::getLogStream().printf("light: start blinking\n");
  // start blinking
  blinkingLightState = false;              // light should be switched on
  lastBlinkingLightStateTime = millis();   // reset blinking timer
  startBlinkingTime = millis();            // Save the stating time of blinking
  blinking = true;
}

void stopBlinking()
{
  logging::getLogStream().printf("light: stop blinking\n");
  // stopping blinking
  // Comme back to the initial brightness level
  if ((brightness - minBrightness) < (maxBrightness - brightness))
    digitalWrite(LIGHT_RELAY, LOW);
  else
    digitalWrite(LIGHT_RELAY, HIGH);
  blinking = false;
}

void setBlinkingPattern(const char *payload)
{
  // payload should contain a sequence of int for the pattern of the blink (duration in ms for the light on, light off, etc)
  uint8_t nbPattern = 0;
  if (payload!=nullptr)
  {
    logging::getLogStream().printf("light: setting pattern to %s\n",payload);
    uint16_t strl = strlen(payload);
    memset(blinkingPattern, 0x00, sizeof(blinkingPattern));
    int i=0, j=0;
    char temp[10];
    memset(temp,0x00,sizeof(temp));
    for (i = 0; i <= strl; i++)
    {
      if (i == strl || payload[i]<'0' || payload[i]>'9')
      {
        if (helpers::isInteger(temp, sizeof(temp) - 1))
        {
          if (nbPattern < 10)
          {
            blinkingPattern[nbPattern] = atoi(temp)*100;
            logging::getLogStream().printf("light: adding pattern %d\n",blinkingPattern[nbPattern]);
            if (blinkingPattern[nbPattern]<200)
            {
              logging::getLogStream().printf("light: pattern duration to short. Set to 200\n");
              blinkingPattern[nbPattern]=200;
            }
            nbPattern++;
          }
        }
        j = 0;
        memset(temp,0x00,sizeof(temp));
      }
      else
      {
        if (j < sizeof(temp) - 1)
        {
          temp[j] = payload[i];
          j++;
        }
      }
    }
    logging::getLogStream().printf("light: new blinking pattern %d %d %d %d %d %d %d %d %d %d\n",
                                    blinkingPattern[0],blinkingPattern[1],blinkingPattern[2],blinkingPattern[3],blinkingPattern[4],
                                    blinkingPattern[5],blinkingPattern[6],blinkingPattern[7],blinkingPattern[8],blinkingPattern[9]);
  }
  if (nbPattern<2)
  {
    // If the blinking pattern is malformed (i.e. sequence smaller than 2)
    logging::getLogStream().printf("light: blinking pattern to short or not defined. Set back to default value\n");
    memset(blinkingPattern,0x00,sizeof(blinkingPattern));
    blinkingPattern[0]=500;
    blinkingPattern[1]=500;
  }
}

void mqttCallback(const char* paramID, const char* payload)
{
  if (strcmp(paramID, "subMqttLightOn") == 0 || strcmp(paramID, "subMqttLightAllOn") == 0)
  {
    lightOn();
  }
  else if (strcmp(paramID, "subMqttLightToggle") == 0)
  {
    lightToggle();
  }
  else if (strcmp(paramID, "subMqttLightOff") == 0 || strcmp(paramID, "subMqttLightAllOff") == 0)
    lightOff();
  else if (strcmp(paramID, "subMqttBlinkingPattern") == 0)
  {
    setBlinkingPattern(payload);
    // Start blinking with the new pattern
    startBlinking();
  }
  else if (strcmp(paramID, "subMqttBlinkingDuration") == 0)
  {
    setBlinkingDuration(payload);
  }
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
    autoOffDuration = 0;
    return;
  }
  autoOffDuration = atoi (str);
}

void setBrightness(uint8_t b)
{
}

ICACHE_RAM_ATTR void lightOn(bool noLightAutoTurnOff)
{
  //logging::getLogStream().printf("light: switch on\n");
  if (noLightAutoTurnOff==true)
  {
    lightAutoTurnOffDisable =true;
    lastLightOnTime=0;
  }
  else
    // Reset auto turn off timer
    lastLightOnTime=millis();
  digitalWrite(LIGHT_RELAY, HIGH);
  brightness = maxBrightness;
}

ICACHE_RAM_ATTR void lightOff()
{
  //logging::getLogStream().printf("light: switch off\n");
  lastLightOnTime = 0;
  lightAutoTurnOffDisable =false;
  digitalWrite(LIGHT_RELAY, LOW);
  brightness = minBrightness;
}

ICACHE_RAM_ATTR void lightToggle(bool noLightAutoTurnOff)
{
  // If brighness different from minBrightness
  if (brightness == minBrightness)
  {
    if (noLightAutoTurnOff==true)
    {
      lightAutoTurnOffDisable =true;
      lastLightOnTime=0;
    }
    else
      // Reset auto turn off timer
      lastLightOnTime=millis();

    // Switch on the light
    digitalWrite(LIGHT_RELAY, HIGH);
    brightness = maxBrightness;
  }
  else
  {
    // Set the timer to 0
    lastLightOnTime = 0;
    lightAutoTurnOffDisable =false;
    // Switch off te light
    digitalWrite(LIGHT_RELAY, LOW);
    brightness = minBrightness;
  }
}

ICACHE_RAM_ATTR bool lightIsOn()
{
  return brightness != minBrightness;
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

void addWifiManagerCustomButtons()
{
}

void addWifiManagerCustomParams()
{
}

void bindServerCallback()
{
  // HTTP callback for controling the light
  wifi::getWifiManager().server.get()->on("/on", []()
                                {
                                  // Light on
                                  lightOn();
                                  // Send OK text
                                  wifi::getWifiManager().server.get()->send ( 200, "text/plain", "Ok");
                                }
                              );
  wifi::getWifiManager().server.get()->on("/off", []()
                                {
                                  // Light off
                                  lightOff();
                                  // Send OK text
                                  wifi::getWifiManager().server.get()->send ( 200, "text/plain", "Ok");
                                }
                              );
}

void handle()
{
  unsigned long currTime;

  // Check if there is new brightness value to publish
  if (publishedBrightness != brightness)
  {
    // Publish the new value of the brightness
    const char* topic = wifi::getParamValueFromID("pubMqttBrightnessLevel");
    // If no topic, we do not publish
    if (topic != NULL)
    {
      char payload[5];
      sprintf(payload, "%d", brightness);
      if (mqtt::publishMQTT(topic, payload))
        // If the new brightness value has been succeefully published
        publishedBrightness = brightness;
    }
  }

  // For blinking
  if (blinking)
  {
    currTime = millis();

    // Check if the blinking has to be stopped
    if (currTime - startBlinkingTime > blinkingTimerDuration*1000)
    {
      logging::getLogStream().printf("light: stop blinking\n");
      stopBlinking();
    }

    // Alternate on/off for the blinking
    if (blinking)
    {
      // Using the pattern, find the new light state
      long diff = currTime - lastBlinkingLightStateTime;
      uint8_t pc = 0;
      unsigned long sum = 0;
      while (diff > sum)
      {
        // If at the end of the pattern, we come back
        if (pc == 10)
        {
          lastBlinkingLightStateTime = lastBlinkingLightStateTime + sum;
          diff = currTime - lastBlinkingLightStateTime;
          logging::getLogStream().printf("light: loop over the pattern\n");
          pc = 0;
          sum = 0;
        }
        sum += blinkingPattern[pc];
        pc++;
      }
      
      bool newBlinkingLightState = ((pc-1)%2==0);
      if (blinkingLightState != newBlinkingLightState)
      {
        blinkingLightState = newBlinkingLightState;
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
      }
    }
  }

  // For the auto-off light
  if (autoOffDuration > 0 && lastLightOnTime > 0)
  {
    currTime = millis();
    // Make the conversion from ms to s
    if (currTime - lastLightOnTime > (autoOffDuration * 1000))
    {
      logging::getLogStream().printf("light: auto-off light\n");
      lightOff();
    }
  }
}

} // namespace dimmer
