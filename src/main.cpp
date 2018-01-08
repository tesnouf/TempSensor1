/**
This update has been done as a test - TE


 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0: Henrik EKblad
 * Version 1.1 - 2016-07-20: Converted to MySensors v2.0 and added various improvements - Torben Woltjen (mozzbozz)
 *
 * DESCRIPTION
 * This sketch provides an example of how to implement a humidity/temperature
 * sensor using a DHT11/DHT-22.
 *
 * For more information, please visit:
 * http://www.mysensors.org/build/humidity
 *


 */

// Enable debug prints
#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RF24

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
//#define MY_RS485

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

// Include Libraries
#include <SPI.h>
#include <MySensors.h>
#include <DHT.h>
#include <Arduino.h>


// Set a bunch of stuff for the DHT
// Set this to the pin you connected the DHT's data pin to
#define DHT_DATA_PIN 2

// Set this offset if the sensor has a permanent small offset to the real temperatures
#define SENSOR_TEMP_OFFSET 0

// Sleep time between sensor updates (in milliseconds)
// Must be >1000ms for DHT22 and >2000ms for DHT11
static const uint64_t UPDATE_INTERVAL = 3000;
float lastTemp;
float lastHum;
bool metric = true;

// Temp Light timer Variables
unsigned long PreviousTempInterval = 0;
const long ReadingInterval = 60000; // 60secs by default

// Define the LED settings

#define LED_PIN 5      // Arduino pin attached to MOSFET Gate pin
#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)

static int16_t currentLevel = 0;  // Current dim level...
MyMessage dimmerMsg(0, V_DIMMER);
MyMessage lightMsg(0, V_LIGHT);

// // Pin connected to
// #define LED_TESTING_PIN 5
// #define RELAY_1  3  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
// #define NUMBER_OF_RELAYS 1 // Total number of attached relays
// #define RELAY_ON 1  // GPIO value to write to turn on attached relay
// #define RELAY_OFF 0 // GPIO value to write to turn off attached relay

// A bunch of stuff for the Controller/Gateway
#define CHILD_ID_HUM 30
#define CHILD_ID_TEMP 31
#define CHILD_ID_LED 32



MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
DHT dht;




void presentation()
{
  // Send the sketch version information to the gateway
  sendSketchInfo("TemperatureAndHumidityLED", "1.1");

  // Register all sensors to gw (they will be created as child devices)
  present(CHILD_ID_HUM, S_HUM);
  present(CHILD_ID_TEMP, S_TEMP);
  // present(CHILD_ID_LED,S_BINARY);
  present( CHILD_ID_LED, V_DIMMER );

  metric = getControllerConfig().isMetric;
}


void setup()
{
  dht.setup(DHT_DATA_PIN); // set data pin of DHT sensor
  if (UPDATE_INTERVAL <= dht.getMinimumSamplingPeriod()) {
    Serial.println("Warning: UPDATE_INTERVAL is smaller than supported by the sensor!");
  }
  // Sleep for the time of the minimum sampling period to give the sensor time to power up
  // (otherwise, timeout errors might occure for the first reading)
  sleep(dht.getMinimumSamplingPeriod());

  // Set up the LED for use cycle it on and off to ensure harware is working
  // This occurs at effectively the same time the first message is being sent
  // pinMode(LED_TESTING_PIN, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
}


void loop()
{
// digitalWrite(LED_BUILTIN, LOW);
// digitalWrite(LED_TESTING, LOW);
unsigned long CurrentTempInterval = millis();
if (CurrentTempInterval - PreviousTempInterval >= ReadingInterval){
  PreviousTempInterval = CurrentTempInterval;
  // Force reading sensor, so it works also after sleep()
//  dht.readSensor(true);

  // Get temperature from DHT library
  float temperature = dht.getTemperature();
  if (isnan(temperature)) {
    Serial.println("Failed reading temperature from DHT!");
  } else {
    lastTemp = temperature;
    temperature += SENSOR_TEMP_OFFSET;
    send(msgTemp.set(temperature, 1));
    // Add a LED to show when a message is being sent
    // digitalWrite(LED_TESTING, LOW);

    #ifdef MY_DEBUG
    Serial.print("T: ");
    Serial.println(temperature);
    #endif
  }


  // Get humidity from DHT library
  float humidity = dht.getHumidity();
  if (isnan(humidity)) {
    Serial.println("Failed reading humidity from DHT");
  } else {
    // Only send humidity if it changed since the last measurement or if we didn't send an update for n times
    lastHum = humidity;
    send(msgHum.set(humidity, 1));

    // Add a LED to show when a message is being sent
    // digitalWrite(LED_BUILTIN, LOW);

    #ifdef MY_DEBUG
    Serial.print("H: ");
    Serial.println(humidity);
    #endif
  }
}

}

// void receive(const MyMessage &message) {
//   // digitalWrite(message.LED_TESTING_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
//   digitalWrite(LED_TESTING_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
//   Serial.print("Incoming change for sensor:");
//   Serial.print(message.sensor);
//   Serial.print(", New status: ");
//   Serial.println(message.getBool());
// }

/***
 *  This method provides a graceful fade up/down effect
 */
void fadeToLevel( int toLevel )
{

    int delta = ( toLevel - currentLevel ) < 0 ? -1 : 1;

    while ( currentLevel != toLevel ) {
        currentLevel += delta;
        analogWrite( LED_PIN, (int)(currentLevel / 100. * 255) );
        delay( FADE_DELAY );
    }
}

void receive(const MyMessage &message)
{
    if (message.type == V_LIGHT || message.type == V_DIMMER) {

        //  Retrieve the power or dim level from the incoming request message
        int requestedLevel = atoi( message.data );

        // Adjust incoming level if this is a V_LIGHT variable update [0 == off, 1 == on]
        requestedLevel *= ( message.type == V_LIGHT ? 100 : 1 );

        // Clip incoming level to valid range of 0 to 100
        requestedLevel = requestedLevel > 100 ? 100 : requestedLevel;
        requestedLevel = requestedLevel < 0   ? 0   : requestedLevel;

        Serial.print( "Changing level to " );
        Serial.print( requestedLevel );
        Serial.print( ", from " );
        Serial.println( currentLevel );

        fadeToLevel( requestedLevel );

        // Inform the gateway of the current DimmableLED's SwitchPower1 and LoadLevelStatus value...
        send(lightMsg.set(currentLevel > 0));

        // hek comment: Is this really nessesary?
        send( dimmerMsg.set(currentLevel) );


    }
}
