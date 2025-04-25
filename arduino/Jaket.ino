/*

Program Description: This program will place the Jaket in auto mode as was. 
The user can optionally adjust the range that enables each power mode 
through a Bluetooth enabled application on their smartphone.
The user can also put the Jaket in manual mode.

*/

#include <EEPROM.h> // Library to read and write to the EEPROM on Arduino Micro.

#define sensorOut A0 // Temperature sensor pin.
#define heatControl 9 // PWM pin for driver (default frequency).

char Incoming_value = 0; // Stores the incoming Bluetooth value.

bool findingRange = false; // Used to find the range sent by the phone app.
bool mode_auto = true; // Auto mode is enabled by default.
bool temp_request = false; // Used for sending temperature data to the phone app.

int temperature = 0; // Stores the temperature from the TMP36 sensor.

// Integers needed for writing/reading range into/from EEPROM.
int range_high = 0;
int addr_high = 0;
int range_medium = 0;
int addr_med = 2;
int range_low = 0;
int addr_low = 4;

// PWM values measured for desired Wattage.
int PWM_off = 0;
int PWM_high = 233; // 10.40 Volts, ~25 Watts.
int PWM_medium = 182; // 8.05 Volts, ~15 Watts.
int PWM_low = 107; // 4.66 Volts, ~5 Watts.

void setup() {

  Serial1.begin(9600); // Initializing the RX and TX serial communication.
  Serial.begin(9600); // Initializing USB serial communication (for debugging).

  pinMode(heatControl, OUTPUT); // PWM pin as output.
}

void loop() {

  // If statement to receive data from phone application.
  // Incoming values were chosen at random, but they are the same
  // values sent by the phone application when connected through
  // Bluetooth.
  if(Serial1.available() > 0)
  {
    Incoming_value = Serial1.read();      
    if(Incoming_value == '0') {
      analogWrite(heatControl, PWM_off);
      mode_auto = false;
    }
    else if(Incoming_value == '5') {
      analogWrite(heatControl, PWM_low);
      mode_auto = false; 
    }
    else if(Incoming_value == '8') {
      analogWrite(heatControl, PWM_medium);
      mode_auto = false;
    }
    else if(Incoming_value == 'b') {
      analogWrite(heatControl, PWM_high);
      mode_auto = false; 
    }
    else if(Incoming_value == 't') {
      temp_request = true;
    }
    else if(Incoming_value == 'r') {
      findingRange = true;
      StoreRange();
      mode_auto = true;
    }
    else if(Incoming_value = 'a') {
      mode_auto = true;      
    }
    else if(Incoming_value = 'm') {
      mode_auto = false;      
    }    
  } 

  // Reading temperature data for sensor.
  // Read twice to improve accuracy.
  int readSensorOut = analogRead(sensorOut);
  readSensorOut = analogRead(sensorOut);

  // Convert the readings into voltages.
  float sensorOutVoltage = readSensorOut * 5.0;
  sensorOutVoltage /= 1024.0;

  // Convert the voltages into the temperature in Celsius.
  float sensorOutTempC = (sensorOutVoltage - 0.5) * 100;

  // Convert temperature from Celcius to Fahrenheit.
  float sensorOutTempF = (sensorOutTempC * 9.0 / 5.0) + 32.0;
  temperature = int(sensorOutTempF);  
  
  if(temp_request) { // Sends the temperature through the TX pin
      Serial1.print(temperature);
      Serial1.print(" \xC2\xB0");
      Serial1.println(" F");
      temp_request = false;
  }

  if(mode_auto) { // Handles the auto mode of the Jaket.

    EEPROM.get(addr_high, range_high); // Reading stored range values from EEPROM
    EEPROM.get(addr_med, range_medium);
    EEPROM.get(addr_low, range_low);
    
    if(temperature >= range_high) {
      analogWrite(heatControl,PWM_off);
    }
    else if((range_high > temperature) && (temperature >= range_medium)) {
      analogWrite(heatControl,PWM_low);
    }
    else if((range_medium > temperature) && (temperature >= range_low)) {
      analogWrite(heatControl,PWM_medium);
    }
    else if(range_low > temperature) {
      analogWrite(heatControl,PWM_high);  
    }
  }
}

/*
This function stores the range sent by the phone app into the EEPROM.
Data is only stored if the values are different, since the EEPROM has
a write limit of 100,000 writes.
*/
void StoreRange() {

  int comma_found = 0;
  int found_num = 0;
  int num_digit = 0;
  int number = 0;

  while(findingRange) {
    if(Serial1.available() > 0) {
        Incoming_value = Serial1.read(); // Read BT stream
          
        if(Incoming_value == ',') {
          comma_found++;
          found_num = comma_found;
        }

        // Numbers are sent in decimal format with the LSD arriving first.
        if(isDigit(Incoming_value)) {
          number = (Incoming_value - 48) * pow(10,num_digit) + number;
          num_digit++;
        }

        if(found_num == 1) {
          range_high = number;
          EEPROM.put(addr_high, number);
          number = 0;
          num_digit = 0; 
          found_num = 0;           
        }

        if(found_num == 2) {
          range_medium = number;
          EEPROM.put(addr_med, number);
          number = 0;      
          num_digit = 0;   
          found_num = 0;    
        }

        if(found_num == 3) {
          range_low = number;
          EEPROM.put(addr_low, number);
          number = 0;     
          num_digit = 0;  
          found_num = 0;   
          findingRange = false;   
        }  
      }    
  }  
}
