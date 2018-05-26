#include <CurieBLE.h>
#include <Wire.h>
#include <Adafruit_ADS1015.h>
Adafruit_ADS1115 ads;  /* Use this for the 16-bit version */
float multiplier;
int16_t gain1;
float adsch1,adsch2;  
uint16_t analogData[NUM_ANALOG_INPUTS];
float adsch1Data[2];
float adsch2Data[2];
BLEService ledService("19B10000-E8F2-537E-4F6C-D104768A1214"); // BLE LED Service

// BLE LED Switch Characteristic - custom 128-bit UUID, read and writable by central
BLEUnsignedCharCharacteristic switchCharacteristic("19B10001-E8F2-537E-4F6C-D104768A1214", BLERead | BLEWrite | BLENotify);
BLECharacteristic  analogCharacteristic("19B10002-E8F2-537E-4F6C-D104768A1214",BLERead|BLENotify,sizeof(analogData));
BLECharacteristic  adsch1Characteristic("19B10003-E8F2-537E-4F6C-D104768A1214",BLERead|BLENotify,sizeof(adsch1Data));
BLECharacteristic  adsch2Characteristic("19B10004-E8F2-537E-4F6C-D104768A1214",BLERead|BLENotify,sizeof(adsch2Data));
BLEDescriptor analogDescriptor("2902","block");
BLEDescriptor adsch1Descriptor("2902","block");
BLEDescriptor adsch2Descriptor("2902","block");
const int ledPin = 13; // pin to use for the LED
void readAnalogInputs(void)
{
  for (int i=0; i<NUM_ANALOG_INPUTS; i++)
  {
    analogData[i] = analogRead(A0+i);
    //Serial.println(analogData[i] );
  }
}
void readAdsCh1(void)
{
  int16_t results;
  results = ads.readADC_Differential_0_1();  
    if (results == 32767){
      gain1 = gain1 + 1;
    }
    if (results == -32768){
      gain1 = gain1 + 1;
    }
    //Serial.print("--");Serial.println(gain1);
      
       if (gain1 == 5){
      ads.setGain(GAIN_TWOTHIRDS);  // 2/3x gain1 +/- 6.144V  1 bit = 3mV      0.1875mV (default)
      multiplier = 0.1875F; /* ADS1115  @ +/- 6.144V gain1 (16-bit results) */
    }
    if (gain1 == 4){
      ads.setGain(GAIN_ONE);        // 1x gain1   +/- 4.096V  1 bit = 2mV      0.125mV
      multiplier = 0.125F; 
    }
    if (gain1 == 3){
      ads.setGain(GAIN_TWO);        // 2x gain1   +/- 2.048V  1 bit = 1mV      0.0625mV
      multiplier = 0.0625F; 
    }
    if (gain1 == 2){
      ads.setGain(GAIN_FOUR);       // 4x gain1   +/- 1.024V  1 bit = 0.5mV    0.03125mV
      multiplier = 0.03125F;
    }
    if (gain1 == 1){
      ads.setGain(GAIN_EIGHT);      // 8x gain1   +/- 0.512V  1 bit = 0.25mV   0.015625mV
      multiplier = 0.015625F; 
    }
    if (gain1 == 0){
      ads.setGain(GAIN_SIXTEEN);    // 16x gain1  +/- 0.256V  1 bit = 0.125mV  0.0078125mV
      multiplier = 0.0078125F; 
    }
      
      
      if (abs(results * multiplier) <= 6144){
      gain1 = 5;
    }
    if (abs(results * multiplier) <= 4096){
      gain1 = 4;
    }
    if (abs(results * multiplier) <= 2048){
      gain1 = 3;
    }
    if (abs(results * multiplier) <= 1024){
      gain1 = 2;
    }
    if (abs(results * multiplier) <= 512){
      gain1 = 1;
    }
    if (abs(results * multiplier) <= 256){
      gain1 = 0;
    }

    results = ads.readADC_Differential_0_1();
    adsch1 = results * multiplier / 1000;
    adsch1Data[0] = gain1;
    adsch1Data[1] = adsch1;
    adsch2 = ads.readADC_Differential_2_3()* multiplier / 1000;
    adsch2Data[0] = gain1;
    adsch2Data[1] = adsch2;
    //Serial.print("*");Serial.print(gain1);Serial.print("  ");Serial.print(results); Serial.print("("); Serial.print(adsch1,3); Serial.println("mV)");
}

void setup() {
  Serial.begin(115200);

  // set LED pin to output mode
  pinMode(ledPin, OUTPUT);

  // begin initialization
  BLE.begin();

  // set advertised local name and service UUID:
  BLE.setLocalName("LED");
  BLE.setAdvertisedService(ledService);

  // add the characteristic to the service
  ledService.addCharacteristic(switchCharacteristic);
  ledService.addCharacteristic(analogCharacteristic);
  ledService.addCharacteristic(adsch1Characteristic);
  ledService.addCharacteristic(adsch2Characteristic);

  // add service
  BLE.addService(ledService);

  // set the initial value for the characeristic:
  switchCharacteristic.setValue(89);
  analogCharacteristic.setValue((unsigned char *)&analogData,sizeof(analogData));
  adsch1Characteristic.setValue((unsigned char *)&adsch1Data,sizeof(adsch1Data));
  adsch2Characteristic.setValue((unsigned char *)&adsch2Data,sizeof(adsch2Data));
  // start advertising
  BLE.advertise();

  Serial.println("Voltmeter Peripheral");
  ads.begin();
}

void loop() {
  // listen for BLE peripherals to connect:
  BLEDevice central = BLE.central();

  // if a central is connected to peripheral:
  if (central) {
    Serial.print("Connected to central: ");
    // print the central's MAC address:
    Serial.println(central.address());

    // while the central is still connected to peripheral:
    while (central.connected()) {
     // BLE.poll();
      //switchCharacteristic.setValue(1);
      // if the remote device wrote to the characteristic,
      // use the value to control the LED:
      readAnalogInputs();
      analogCharacteristic.setValue((unsigned char *)&analogData,sizeof(analogData));
      //Serial.println(analogRead(A0));
      readAdsCh1();
      adsch1Characteristic.setValue((unsigned char *)&adsch1Data,sizeof(adsch1Data));
      adsch2Characteristic.setValue((unsigned char *)&adsch2Data,sizeof(adsch2Data));

      if (switchCharacteristic.written()) {
        if (switchCharacteristic.value()) {   // any value other than 0
          Serial.println("LED on");
          digitalWrite(ledPin, HIGH);         // will turn the LED on
        } else {                              // a 0 value
          Serial.println(F("LED off"));
          digitalWrite(ledPin, LOW);          // will turn the LED off
        }
      }
    }

    // when the central disconnects, print it out:
    Serial.print(F("Disconnected from central: "));
    Serial.println(central.address());
  }
}
