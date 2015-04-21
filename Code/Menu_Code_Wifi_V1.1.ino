
//I did not create this project, only repackage it it and use a smaller oled display.
//See the original at:
//http://thisoldgeek.blogspot.ca/2015/01/esp8266-weather-display.html
//
//
//Arduino IDE 1.0.6 is needed for the Json library
//Json Library: https://github.com/bblanchon/ArduinoJson

#include <Wire.h>
#include "font.h"
#include <avr/pgmspace.h>
#include <ArduinoJson.h>

#define OLED_address  0x3c  //OLED I2C bus address

#define SSID "YOUR WIFI SSID" // insert your SSID
#define PASS "YOUR WIFI PASSWORD" // insert your password

#define LOCATIONID "YOUR LOCATION"
#define DST_IP "23.222.152.140" //api.wunderground.com
#define DEG 2

int ESP_CP_PD = 12;   //cut wire
int ESP_RST = 4;

const int buffer=300;
char* conds[]={"\"city\":","\"weather\":","\"temp_c\":","\"relative_humidity\":","\"wind_dir\":","\"wind_kph\":","\"pressure_in\":"};
int num_elements = 7;  // number of conditions you are retrieving, count of elements in conds

char close_brace ='}';
char comma = ',';

int wifiConnected = 0;

char tmp[8];


void setup() 
{
  Serial.begin(9600);
  Serial1.begin(9600);
  
  pinMode(ESP_CP_PD, OUTPUT); 
  pinMode(ESP_RST, OUTPUT); 
  
  //Initialize I2C and OLED Display
  Wire.begin();
  init_OLED();
  reset_display();
  displayOff();
  setXY(0,0);
  clear_display();
  displayOn();
  sendStrXY(" Weather Info",0,1);
  delay(1000);
}

void loop() 
{
    reset();
    delay(2000);  //ESP8266 takes a while to restart
    if(!wifiConnected) 
    {
     connectWiFi();
    }

    if(!wifiConnected) 
    {
     delay(500);
     return;
    }
  
  
  static unsigned long thisMicros = 0;
  static unsigned long lastMicros = 0;
 
  Serial1.println("AT+CIPMUX=0"); //set to single connection mode
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += DST_IP;
  cmd += "\",80";
  Serial1.println(cmd);
  Serial.print(cmd);
 
  if(Serial1.find("ERROR"))
  { 
   Serial.println("Error!"); 
   return;
  }
  else
  {
   
  }
  
  cmd = "GET /api/YOUR KEY/conditions/q/";  //my API key
  cmd += LOCATIONID;
  cmd +=".json";
  cmd += " HTTP/1.1\r\nHost: api.wunderground.com\r\n\r\n";   
  Serial1.print(F("AT+CIPSEND="));
  Serial1.println(cmd.length());
  if(Serial1.find(">")){
    Serial.println("Connected to wunderground");  
 
      sendStrXY("-Getting Info",6,0);
      delay(500);
      
      clear_display();
  }else{
    Serial1.println("AT+CIPCLOSE");
    Serial.println("Connection Timed Out"); 
    delay(1000);
    return;
  }
  Serial1.print(cmd);
  Serial.print(cmd);
  
  unsigned int i = 0;    //timeout counter
  char json[buffer]="{"; // array for Json parsing
  int n = 1;             // character counter for json  
  
  for (int j=0;j<num_elements;j++){
    while (!Serial1.find(conds[j])){} // find the part we are interested in.
  
    String Str1 = conds[j];
  
    for (int l=0; l<(Str1.length());l++)
        {json[n] = Str1[l];
         n++;}   
    while (i<6000) {
      if(Serial1.available()) {
        char c = Serial1.read();
         
          if(c==',') break;
          json[n]=c;
          n++;
          i=0;
        }
        i++;
      }
     if (j==num_elements-1)
        {json[n]=close_brace;}
     else   
        {json[n]=comma;}
     n++;   
  }

  parseJSON(json);

  //Done with processing for now - close connection
  Serial1.println("AT+CIPCLOSE");

  delay(1000);  //delay(5000);

 // Only check weather every 15 minutes
 // So you don't go over quota on wunderground (for free api license)
  delay(900000);  //delay(900000);
}




void parseJSON(char json[300])
{
  StaticJsonBuffer<buffer> jsonBuffer;
 JsonObject& root = jsonBuffer.parseObject(json);
 
 if (!root.success())
{
  Serial.println("fparseObject() failed");
  return;
}

 const char* city = root["city"];
 const char* weather = root["weather"];
 //double temp_c = root["temp_c"]; 
 //int temp_c = root["temp_c"]; 
 double temp_c = root["temp_c"]; //Good for Windsor!
 const char* humidity = root["relative_humidity"];
 const char* wind_dir = root["wind_dir"];
 //double wind_kph = root["wind_kph"];
 //int wind_kph = root["wind_kph"];
 double wind_kph = root["wind_kph"];
 const char* pressure_in = root["pressure_in"];
 sendStrXY(" Weather Info",0,0);
  
  // Location of conditions
  Serial.print("City: ");
  Serial.println(city);
  sendStrXY("City: ",1,0);
  char a[12];
  sprintf(a, "%s ", city);
  sendStrXY(a, 1,6);

   
  // Conditions: Sunny, Cloudy, Fog, Rain, etc. 
  Serial.print("Weather: ");
  Serial.println(weather);
  sendStrXY("Type: ",2,0);
  char b[12];
  sprintf(b, "%s ", weather);
  sendStrXY(b, 2,6);
  //oledWriteString(b);

  
  // Temperature  
  Serial.print("Temp: ");
  Serial.println(temp_c);  
   sendStrXY("Temp: ",3,0);  
   char c[12];
   sprintf(c, "%d ", temp_c);
   dtostrf(temp_c,3,1,c);
   sendStrXY(c, 3, 6);
   
   for(int i=0;i<8;i++)     // print degree simbol
    SendChar(pgm_read_byte(myDregree+i));
  sendStrXY("C",3,11);
  
   
  


  // Humidity 
  Serial.print("Humidity: ");
  Serial.println(humidity);
   sendStrXY("Humidity: ",4,0);
   char d[12];
   sprintf(d, "%s ", humidity);
   sendStrXY(d, 4, 10);

  // Wind Direction
  Serial.print("Wind Direction: ");
  Serial.println(wind_dir);
   sendStrXY("Wind Dir:", 5, 0);
   char e[12];
   sprintf(e, "%s ", wind_dir);
   sendStrXY(e, 5, 10);

  // Wind Speed
  Serial.print("Wind Speed: ");
  Serial.println(wind_kph);
   sendStrXY("Wind Spd:", 6, 0);
   char f[12];
   sprintf(f, "%d ", wind_kph);
   dtostrf(wind_kph,3,1,f);
   sendStrXY(f, 6, 10);
   sendStrXY("Km",6,14);
 
  // Barometric Pressure
  Serial.print("Barometric Pressure: ");
  Serial.println(pressure_in);
   sendStrXY("Baro:", 7, 0);
   char g[32];
   sprintf(g, "%s ", pressure_in);
   sendStrXY(g, 7,6);
   delay(1000);
 
}


void reset()
{
  digitalWrite(ESP_RST,LOW);
  delay(100);
  digitalWrite(ESP_RST,HIGH); 
}



boolean connectWiFi()
{
  Serial1.println("AT+RST");  //Reset wifi
  sendStrXY("-Resetting Wifi",2,0);
  delay(1000);
  
  Serial1.println("AT+CWMODE=3");
  sendStrXY("-AT+CWMODE=3",3,0); 
  String cmd="AT+CWJAP=\"";
  cmd+=SSID;
  cmd+="\",\"";
  cmd+=PASS;
  cmd+="\"";
  Serial1.println(cmd);
  Serial.println(cmd);
  sendStrXY("-SSID & Password",4,0);
  //delay(8000);  //varies, can take from 5 seconds to 15 seconds after SSID & PASSWD entered for a resonse  
  
  for(int i = 0; i < 20; i++) 
  {
   if(Serial1.find("OK"))
   {
    Serial.println("Connected to WiFi...");
    sendStrXY("WiFi Good",5,0);
    wifiConnected = 1;
    delay(200);     // Allow display to print
    break;
  }
  
  if(Serial1.find("ERROR"))
  {
   break;
  }
  delay(50);
 }
}





//==========================================================//
// Resets display depending on the actual mode.
static void reset_display(void)
{
  displayOff();
  clear_display();
  displayOn();
}



//==========================================================//
// Turns display on.
void displayOn(void)
{
  sendcommand(0xaf);        //display on
}

//==========================================================//
// Turns display off.
void displayOff(void)
{
  sendcommand(0xae);		//display off
}

//==========================================================//
// Clears the display by sendind 0 to all the screen map.
static void clear_display(void)
{
  unsigned char i,k;
  for(k=0;k<8;k++)
  {	
    setXY(k,0);    
    {
      for(i=0;i<128;i++)     //clear all COL
      {
        SendChar(0);         //clear all COL
        //delay(10);
      }
    }
  }
}



//==========================================================//
// Actually this sends a byte, not a char to draw in the display. 
// Display's chars uses 8 byte font the small ones and 96 bytes
// for the big number font.
static void SendChar(unsigned char data) 
{
  //if (interrupt && !doing_menu) return;   // Stop printing only if interrupt is call but not in button functions
  
  Wire.beginTransmission(OLED_address); // begin transmitting
  Wire.write(0x40);//data mode
  Wire.write(data);
  Wire.endTransmission();    // stop transmitting
}

//==========================================================//
// Prints a display char (not just a byte) in coordinates X Y,
// being multiples of 8. This means we have 16 COLS (0-15) 
// and 8 ROWS (0-7).
static void sendCharXY(unsigned char data, int X, int Y)
{
  setXY(X, Y);
  Wire.beginTransmission(OLED_address); // begin transmitting
  Wire.write(0x40);//data mode
  
  for(int i=0;i<8;i++)
    Wire.write(pgm_read_byte(myFont[data-0x20]+i));
    
  Wire.endTransmission();    // stop transmitting
}

//==========================================================//
// Used to send commands to the display.
static void sendcommand(unsigned char com)
{
  Wire.beginTransmission(OLED_address);     //begin transmitting
  Wire.write(0x80);                          //command mode
  Wire.write(com);
  Wire.endTransmission();                    // stop transmitting
}

//==========================================================//
// Set the cursor position in a 16 COL * 8 ROW map.
static void setXY(unsigned char row,unsigned char col)
{
  sendcommand(0xb0+row);                //set page address
  sendcommand(0x00+(8*col&0x0f));       //set low col address
  sendcommand(0x10+((8*col>>4)&0x0f));  //set high col address
}


//==========================================================//
// Prints a string regardless the cursor position.
static void sendStr(unsigned char *string)
{
  unsigned char i=0;
  while(*string)
  {
    for(i=0;i<8;i++)
    {
      SendChar(pgm_read_byte(myFont[*string-0x20]+i));
    }
    *string++;
  }
}

//==========================================================//
// Prints a string in coordinates X Y, being multiples of 8.
// This means we have 16 COLS (0-15) and 8 ROWS (0-7).
static void sendStrXY( char *string, int X, int Y)
{
  setXY(X,Y);
  unsigned char i=0;
  while(*string)
  {
    for(i=0;i<8;i++)
    {
      SendChar(pgm_read_byte(myFont[*string-0x20]+i));
    }
    *string++;
  }
}


//==========================================================//
// Inits oled and draws logo at startup
static void init_OLED(void)
{
  sendcommand(0xae);		//display off
  sendcommand(0xa6);            //Set Normal Display (default)
    // Adafruit Init sequence for 128x64 OLED module
    sendcommand(0xAE);             //DISPLAYOFF
    sendcommand(0xD5);            //SETDISPLAYCLOCKDIV
    sendcommand(0x80);            // the suggested ratio 0x80
    sendcommand(0xA8);            //SSD1306_SETMULTIPLEX
    sendcommand(0x3F);
    sendcommand(0xD3);            //SETDISPLAYOFFSET
    sendcommand(0x0);             //no offset
    sendcommand(0x40 | 0x0);      //SETSTARTLINE
    sendcommand(0x8D);            //CHARGEPUMP
    sendcommand(0x14);
    sendcommand(0x20);             //MEMORYMODE
    sendcommand(0x00);             //0x0 act like ks0108
    
    //sendcommand(0xA0 | 0x1);      //SEGREMAP   //Rotate screen 180 deg
    sendcommand(0xA0);
    
    //sendcommand(0xC8);            //COMSCANDEC  Rotate screen 180 Deg
    sendcommand(0xC0);
    
    sendcommand(0xDA);            //0xDA
    sendcommand(0x12);           //COMSCANDEC
    sendcommand(0x81);           //SETCONTRAS
    sendcommand(0xCF);           //
    sendcommand(0xd9);          //SETPRECHARGE 
    sendcommand(0xF1); 
    sendcommand(0xDB);        //SETVCOMDETECT                
    sendcommand(0x40);
    sendcommand(0xA4);        //DISPLAYALLON_RESUME        
    sendcommand(0xA6);        //NORMALDISPLAY             

  clear_display();
  sendcommand(0x2e);            // stop scroll
  //----------------------------REVERSE comments----------------------------//
  //  sendcommand(0xa0);		//seg re-map 0->127(default)
  //  sendcommand(0xa1);		//seg re-map 127->0
  //  sendcommand(0xc8);
  //  delay(1000);
  //----------------------------REVERSE comments----------------------------//
  // sendcommand(0xa7);  //Set Inverse Display  
  // sendcommand(0xae);		//display off
  sendcommand(0x20);            //Set Memory Addressing Mode
  sendcommand(0x00);            //Set Memory Addressing Mode ab Horizontal addressing mode
  //  sendcommand(0x02);         // Set Memory Addressing Mode ab Page addressing mode(RESET)  
  
   setXY(0,0);
  
  for(int i=0;i<128*8;i++)     // show 128* 64 Logo
  {
    SendChar(pgm_read_byte(logo+i));
  }
  sendcommand(0xaf);		//display on
  
  //sendStrXY("Miker",7,5);
  delay(5000); 
}

