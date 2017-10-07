/*
  WiFi UDP Send and Receive String

  This sketch wait an UDP packet on localPort using a WiFi shield.
  When a packet is received an Acknowledge packet is sent to the client on port remotePort

  Circuit:
   WiFi shield attached

  created 30 December 2012
  by dlf (Metodo2 srl)

  modified 6 October 2017
  by Eccepiente (aka Piero Processing)

*/


#include <SPI.h>
#include <WiFi101.h>
#include <WiFiUdp.h>
#include <OSCMessage.h>
//#include <stdio.h>

int status = WL_IDLE_STATUS;
#include "arduino_secrets.h"

///////please enter your sensitive data in the Secret tab/arduino_secrets.h

char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)
int keyIndex = 0;            // your network key Index number (needed only for WEP)



int incomingValue = 0;
float fraction;
short int indice;
char incomingData;
boolean checkValue = false;
boolean setValue = false;
short int multiplier;

unsigned int localPort = 2390;      // local port to listen on
//const IPAddress outIp (192, 168, 1, 199);
//unsigned int localPort = 2390;      // local port to listen on

char packetBuffer[255]; //buffer to hold incoming packet
char  ReplyBuffer[] = "acknowledged";       // a string to send back

WiFiUDP Udp;

void setup() {

  //Initialize serial and wait for port to open:
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // check for the presence of the shield:
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi shield not present");
    // don't continue:
    while (true);
  }

  // attempt to connect to WiFi network:
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(1000);
  }
  Serial.println("Connected to wifi");
  printWiFiStatus();

  Serial.println("\nStarting connection to server...");

  // if you get a connection, report back via serial:
  // this is the port to point if you send a message
  Udp.begin(localPort);
}
//////////////////////////
// the array to store the incoming data from serial port
char oscData[80];

char typeTagInt [] = {',', 'i', 0, 0};
char typeTagFloat [] = {',', 'f', 0, 0};

// this is the kind of message you want to send
char message[72] = {
  '/', 'l', 'a', 'y',
  'e', 'r', '1', '/',
  'v', 'i', 'd', 'e',
  'o', '/', 'o', 'p',
  'a', 'c', 'i',  't',
  'y', '/', 'v',  'a',
  'l', 'u', 'e', 's',
  ',', 'f',  0,  0,
  '?',  0,  0,   0
};
uint32_t hex2int(char *hex) {
  uint32_t val = 0;
  while (*hex) {
    // get current character then increment
    uint8_t byte = *hex++;
    // transform hex character to the 4bit equivalent number, using the ascii table indexes
    if (byte >= '0' && byte <= '9') byte = byte - '0';
    else if (byte >= 'a' && byte <= 'f') byte = byte - 'a' + 10;
    else if (byte >= 'A' && byte <= 'F') byte = byte - 'A' + 10;
    // shift 4 to make space for new digit, and add the 4 bits of the new digit
    val = (val << 4) | (byte & 0xF);
  }
  return val;
}
//////////////////////////
void loop() {

  // clear index before starting the count

  indice = 0;
  incomingValue = 0;
  fraction = 0;
  checkValue = false;
  setValue = false;
  multiplier = 0;
  // if there's data available, read a packet
  /*
     TO DO:
     Use the parsePacket method to set all the machine you want to comunicate with
     Pseudo Code:
     Send a message with Udp to set different IPAdress and than use them to share
     different OSC message
  */

  // here the feedback when talking to machines via wifi

  int packetSize = Udp.parsePacket();
  if (packetSize)
  {
    Serial.print("Received packet of size ");
    Serial.println(packetSize);
    Serial.print("From ");
    IPAddress remoteIp = Udp.remoteIP();
    Serial.print(remoteIp);
    Serial.print(", port ");
    Serial.println(Udp.remotePort());
    //
    //    // read the packet into packetBufffer
    int len = Udp.read(packetBuffer, 255);
    if (len > 0) packetBuffer[len] = 0;
    Serial.println("Contents:");
    Serial.println(packetBuffer);

    // send a reply, to the IP address and port that sent us the packet we received
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.write(ReplyBuffer);
    Udp.endPacket();
  }
  // here start the loop to send messages via serial
  if (Serial.available() > 0) {

    //loop to get a string from serial

    while (1) {

      incomingData = Serial.read();

      if (incomingData == '\n' ) break;

      // the space in you message means you are sending int or float
      if (isSpace(incomingData)) {
        checkValue = !checkValue;
      }

      if (checkValue && isDigit(incomingData)) {

        incomingValue *= 10 ;  // shift left 1 decimal place
        incomingValue += int(incomingData - 48);

        if (setValue) {
          multiplier++; // to have fraction in your data
        }

      } else if (checkValue && isPunct(incomingData)) {
        setValue = true;
      } else if (isAlpha(incomingData) || incomingData == '/' || isDigit(incomingData)) {

        oscData[indice] = incomingData;
        indice++;

      } else  {
        continue;
      }
    }
  }
  if (strlen (oscData) != 0) {
    /*
      // osc formatting see http://opensoundcontrol.org/spec-1_0
      you need always 4 data each row so this is the algorithm to achieve that
      // create a message multiple of 4;
    */
    float len = strlen (oscData);
    //    Serial.print("len of osc data serial: ");
    //    Serial.println(len);
    int row = int(ceil(len / 4.00));
    //    Serial.print("row of data osc: ");
    //    Serial.println(row);
    int patternByte = row * 4;
    //    Serial.print("patternByte len of new array: ");
    //    Serial.println(patternByte);
    char oscPattern [patternByte];

    // oscPattern is sure to be a multiple of 4 array with the osc data from serial!
    // to do: faster way to copy.

    for (int i = 0; i < patternByte; i++) {
      oscPattern[i] = oscData[i];
    }

    Serial.print("incoming value from serial: ");
    Serial.println(incomingValue);
    //    Serial.print("multiply: ");
    //    Serial.println(multiplier);
    //    Serial.print("setValue: ");
    //    Serial.println(setValue);

    // create the float part of osc message
    int lenCharVal;

    if (setValue) {
      fraction = incomingValue / (pow(10, multiplier));
    }

    // here the system to store the float or int message part of the osc.
    char charVal[8];

    Serial.print("fraction: ");
    Serial.println(fraction);

    //here the conversion from float to long int

    unsigned int ui;
    memcpy(&ui, &fraction, sizeof (ui));

    if (setValue) lenCharVal = snprintf(charVal, 9, "%x", ui);
    else lenCharVal = snprintf(charVal, 9, "%i", incomingValue);

    //    Serial.println("char of value: ");
    //    for (int i = 0; i < 8; i++) {
    //      Serial.println(charVal[i]);
    //
    //    }

    // here the assignmet of the array in 4 coupled data than the conversion of each couple into an int
    char  oscValue [4];
    for (int i = 0; i < 8; i += 2) {
      char holdHex[2];
      holdHex[0] = charVal[i];
      holdHex[1] = charVal[i + 1];

      char * convert = holdHex;
      //      Serial.print("value of hold hex: ");
      //      Serial.println(holdHex);
      oscValue [i / 2] = hex2int(convert);
      //      Serial.print("value of convert: ");
      //      Serial.println(oscValue [i / 2]);
    }

    Serial.print("oscVal: ");
    Serial.println(oscValue);

    int oscLen = patternByte + 4 + 4;
    Serial.print("oscLen: ");
    Serial.println(oscLen);


    // assemble the osc message maybe there is a fastest and elegant way

    char oscMessage [oscLen];

    memcpy( oscMessage, oscPattern, sizeof( oscPattern ) );

    int limit = patternByte + 4;
    if (setValue) {
      for (int i = patternByte; i < limit; i++) {
        oscMessage[i] = typeTagFloat[i - patternByte];
      }
    } else {
      for (int i = patternByte; i < limit; i++) {
        oscMessage[i] = typeTagInt[i - patternByte];
      }
    }
    for (int i = limit; i < limit + 4; i++) {
      oscMessage[i] = oscValue[i - limit];
    }


    Serial.println(oscMessage);
    /*
      TODO
      I don't know if i have to clear or delete all the array i created before.
    */

    /*
      TO DO:
      Use the parsePacket method to set all the machine you want to comunicate with
      to know the ip adress on the run.
      Pseudo Code:
      Send a message with Udp to set different IPAdress and than use them to share
      different OSC message to different machines
    */
    Udp.beginPacket(Udp.remoteIP(), 9999);
    Udp.write(oscMessage, oscLen);
    Udp.endPacket();

  }
  // how to clear an array
  memset(oscData, 0, sizeof(oscData));
}


void printWiFiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your WiFi shield's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}




