#include <Wire.h>
#include "uRTCLib.h"
#include <avr/pgmspace.h>
#include "U8glib.h"
#include <avr/wdt.h>

//define pins
#define PUMP 4
#define MENU_BUTTON 7
#define RELAY_1 6
#define RELAY_2 8
#define RELAY_3 9
#define RELAY_4 10
#define MOISTURE_1 A0
#define MOISTURE_2 A1
#define MOISTURE_3 A2
#define MOISTURE_4 A3
#define RESET_ESP A4

U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NONE);    // I2C
uRTCLib rtc(0x68);

//Globals for ESP8266 WiFi module
const int OK = 0;
const int ERROR = 1;
const int FAIL = 2;
const char* CR_LF = "\r\n";
char buffer[65];
size_t nbytes = 0;
size_t bindex = 0;
char global_res_buffer[350];


//RTC days of week array
char* daysOfTheWeek[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

//moisture sensor values
int moisture_values[4] = { 0, 0, 0, 0 };

//lower and upper moisture thresholds
unsigned char moisture_lower[4] = { 30, 30, 30, 30 };
unsigned char moisture_upper[4] = { 55, 55, 55, 55 };

//menu state and button globals
int menuState = 0;
unsigned long lastPress = 0;

//pump state    1:on   0:off
unsigned char pump_state_flag = 0;

//relay state flags bitmap    1:valve open   0:valve closed
unsigned char relay_state_flags = 0;

//override flags bitmap
unsigned char override_flags = 0;

unsigned long override_timestamps[4] = { 0, 0, 0, 0 };

unsigned char* water_params[5] = { "0", "1", "2", "3", "all" };


//water by moisture or time IE mode flag bitmap
unsigned char mode_flags = 0;

//keep track of whether plant has been watered for the day (relevant for time mode only)
uint8_t today = 0;
unsigned char watered_flags = 0;
unsigned long watered_timestamps[4] = { 0, 0, 0, 0 };

//hour and minute to water a plant in time mode
unsigned char hour[4] = { 0, 0, 0, 0 };
unsigned char minute[4] = { 0, 0, 0, 0 };

unsigned char reconfigure_flag = 1;


const unsigned char bender[] PROGMEM = {
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfb, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x7f, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0x7f, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x7f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x7f, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x7f, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0x7f, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xfd, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0x0f, 0xf8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xe3, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x80, 0xe3, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f,
   0x3f, 0xc8, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xc7, 0xff, 0x1f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf7, 0xff, 0x7f, 0xfe, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xf3,
   0xff, 0xff, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xf9, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0xff, 0xff, 0xf9, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0xfe,
   0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0x7f, 0xff, 0xff, 0xff, 0xf3, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff, 0xff, 0xff, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbf, 0xff,
   0xff, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xbf, 0xff, 0xff, 0xff, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0x00, 0x00, 0x00, 0xf8, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0xff, 0xff,
   0xff, 0xf3, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xcf, 0x00, 0x00, 0x00, 0xc6, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0x6f, 0x00, 0x00, 0x00, 0x9c, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x27, 0x00, 0x00,
   0x00, 0xb8, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x37, 0x00, 0x00, 0x00, 0x38, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0x17, 0x00, 0x00, 0x00, 0x78, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x17, 0x10, 0x8f,
   0xe3, 0x79, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x17, 0x18, 0xdf, 0xe3, 0x7b, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0x37, 0xf8, 0xdf, 0xff, 0x7b, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x27, 0xf8, 0xdf,
   0xff, 0x79, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x6f, 0xf8, 0x8f, 0xff, 0x7c, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xcf, 0x01, 0x00, 0x00, 0x3e, 0xf7, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xff, 0xff,
   0xff, 0xbf, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0x7f, 0xfc, 0xff, 0xff, 0x9f, 0xf7, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe1, 0xff, 0xff, 0xc7, 0xf3, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x0f, 0x00,
   0x00, 0xf0, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff, 0xff, 0xff, 0xfb, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0xff,
   0xff, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xdf, 0x01, 0x00, 0xff, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x74, 0x5f, 0xf8, 0xfb, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x9f, 0x76,
   0xdf, 0xe3, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0x3f, 0x76, 0xdf, 0xcb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xda, 0xfb, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x76,
   0xdf, 0xc0, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0x76, 0xdf, 0xdb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x00, 0x00, 0xc0, 0xfb, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x76,
   0xdf, 0xdb, 0xfb, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0x76, 0xdf, 0xcb, 0xf9, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x00, 0x00, 0xe0, 0xfd, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x3f, 0xff,
   0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0x9f, 0xff, 0xff, 0xff, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xdf, 0x01, 0x00, 0xf0, 0xfd, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x1f, 0xfc,
   0xff, 0x87, 0xfd, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xcf, 0xff, 0xff, 0x3f, 0xfc, 0xff, 0xff, 0xff, 0xff, 0xff,
   0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xe7, 0xff, 0xff, 0xff, 0xf9, 0xff,
   0xff, 0xff, 0xff, 0xff
};

// good flower
const unsigned char bitmap_good[] PROGMEM = {

  0x00, 0x42, 0x4C, 0x00, 0x00, 0xE6, 0x6E, 0x00, 0x00, 0xAE, 0x7B, 0x00, 0x00, 0x3A, 0x51, 0x00,
  0x00, 0x12, 0x40, 0x00, 0x00, 0x02, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00, 0x00, 0x06, 0x40, 0x00,
  0x00, 0x04, 0x60, 0x00, 0x00, 0x0C, 0x20, 0x00, 0x00, 0x08, 0x30, 0x00, 0x00, 0x18, 0x18, 0x00,
  0x00, 0xE0, 0x0F, 0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x02, 0xC1, 0x00, 0x00, 0x0E, 0x61, 0x00,
  0x00, 0x1C, 0x79, 0x00, 0x00, 0x34, 0x29, 0x00, 0x00, 0x28, 0x35, 0x00, 0x00, 0x48, 0x17, 0x00,
  0x00, 0xD8, 0x1B, 0x00, 0x00, 0x90, 0x1B, 0x00, 0x00, 0xB0, 0x09, 0x00, 0x00, 0xA0, 0x05, 0x00,
  0x00, 0xE0, 0x07, 0x00, 0x00, 0xC0, 0x03, 0x00
};

// bad flower
const unsigned char bitmap_bad[] PROGMEM = {
  0x00, 0x80, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0xE0, 0x0D, 0x00, 0x00, 0xA0, 0x0F, 0x00,
  0x00, 0x20, 0x69, 0x00, 0x00, 0x10, 0x78, 0x02, 0x00, 0x10, 0xC0, 0x03, 0x00, 0x10, 0xC0, 0x03,
  0x00, 0x10, 0x00, 0x01, 0x00, 0x10, 0x80, 0x00, 0x00, 0x10, 0xC0, 0x00, 0x00, 0x30, 0x60, 0x00,
  0x00, 0x60, 0x30, 0x00, 0x00, 0xC0, 0x1F, 0x00, 0x00, 0x60, 0x07, 0x00, 0x00, 0x60, 0x00, 0x00,
  0x00, 0x60, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00,
  0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0xC7, 0x1C, 0x00,
  0x80, 0x68, 0x66, 0x00, 0xC0, 0x33, 0x7B, 0x00, 0x40, 0xB6, 0x4D, 0x00, 0x00, 0xE8, 0x06, 0x00,
  0x00, 0xF0, 0x03, 0x00, 0x00, 0xE0, 0x00, 0x00
};


void reset_arduino()
{
  wdt_enable(WDTO_1S);
  while(1) continue;
}

void setup()
{
  pinMode(RESET_ESP, OUTPUT);
  digitalWrite(RESET_ESP, LOW);
  drawbender();
  Serial1.setTimeout(0);
  Serial1.begin(9600);
  wifi_init(global_res_buffer, sizeof(global_res_buffer));
  wifi_configure(global_res_buffer, sizeof(global_res_buffer)); 
  wifi_reconnect(global_res_buffer, sizeof(global_res_buffer));
  Wire.begin();
  URTCLIB_WIRE.begin();
  rtc.set(0, 56, 12, 5, 13, 1, 22);
  // declare relay as output
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(RELAY_3, OUTPUT);
  pinMode(RELAY_4, OUTPUT);
  // declare pump as output
  pinMode(PUMP, OUTPUT);
  // declare switch as input
  pinMode(MENU_BUTTON, INPUT);

  wdt_enable(WDTO_4S); //prevent drowning a plant in the case of a hang, such as the nodejs server crashing mid-transmission  
  reconfigure_flag = iot_push(reconfigure_flag); //pulls configuration
  wdt_reset();
  if(!reconfigure_flag) //configuration pull was successful
    reconfigure_flag = iot_push(reconfigure_flag); //sets rtc (configuration pull queues sync command on nodejs side)
  wdt_reset();

  attachInterrupt(digitalPinToInterrupt(MENU_BUTTON), &toggle_state, FALLING);
  
}

void loop()
{
  rtc.refresh();
  unsigned long current_millis = millis();
  check_overrides(current_millis);
  check_day();
  read_moisture();
  water_flower(current_millis);
  render_display();
  reconfigure_flag = iot_push(reconfigure_flag);
  if(current_millis < lastPress || (current_millis - lastPress > 100))
    attachInterrupt(digitalPinToInterrupt(MENU_BUTTON), &toggle_state, FALLING);
  wdt_reset(); //prevent drowning a plant in the case of a hang, such as the nodejs server crashing mid-transmission
}

unsigned char iot_push(unsigned char configure)
{
  int result = 0;
  {
    char payload[180];
    {
      char body[100];
      char buffer[5];
      memset(payload, '\0', sizeof(payload));
      memset(body, '\0', sizeof(body));
      strcat(body, "{\"a\":[");
      int scratch = 0;
      for(int i = 0; i < 4; i++)
      {
        itoa(moisture_values[i], buffer, 10);
        strcat(body, buffer);
        strcat(body, ",");
        if((relay_state_flags >> i & 1) == 0)
          strcat(body, "0");
        else
          strcat(body, "1");
        strcat(body, ",");
        if((mode_flags >> i & 1) == 0)
          strcat(body, "0");
        else
          strcat(body, "1");
        strcat(body, ",");
        scratch = (int)moisture_lower[i];
        itoa(scratch, buffer, 10);
        strcat(body, buffer);
        strcat(body, ",");
        scratch = (int)moisture_upper[i];
        itoa(scratch, buffer, 10);
        strcat(body, buffer);
        strcat(body, ",");
        scratch = (int)hour[i];
        itoa(scratch, buffer, 10);
        strcat(body, buffer);
        strcat(body, ",");
        scratch = (int)minute[i];
        itoa(scratch, buffer, 10);
        strcat(body, buffer);
        strcat(body, ",");
      }      
      if(pump_state_flag == 0)
        strcat(body, "0");
      else
        strcat(body, "1");
      strcat(body, "]}");
      if(!configure)
        strcat(payload, "POST");
      else
        strcat(payload, "GET");
      strcat(payload, " /elecrow/report HTTP/1.1");
      if(!configure)
      {
        strcat(payload, "\r\nContent-Type: application/json\r\nContent-Length: ");
        itoa(strlen(body), buffer, 10);
        strcat(payload, buffer);
      }
      strcat(payload, "\r\n\r\n");
      if(!configure)
        strcat(payload, body);
    }
    result = wifi_send(global_res_buffer, sizeof(global_res_buffer), "SERVER_IP_HERE", "SERVER_PORT_HERE", payload, strlen(payload), 1);
  }
  if(result != OK)
    return 1;
  char* body = strstr(global_res_buffer, "\r\n\r\n");
  if(body == NULL)
    return 1;
  body += 4;
  char buffer[50];
  if(get_value(buffer, sizeof(buffer), body, "command") == NULL)
    return 0;
  if(strcmp(buffer, "sync") == 0)
  {
    if(get_value(buffer, sizeof(buffer), body, "param") == NULL)
      return 0;
    int args[7];
    char* start = buffer;
    char* end = NULL;
    bool valid = true;
    for(int i = 0; i < 7; i++)
    {
      end = strchr(start, ' ');
      if(end == NULL && i != 6)
      {
        valid = false;
        break;
      }
      if(end != NULL)
        *end = '\0';
      args[i] = atoi(start);
      start = end + 1;
    }
    if(valid)
      rtc.set(args[0], args[1], args[2], args[3], args[4], args[5], args[6]);
    return 0;
  }
  else if(strcmp(buffer, "mode") == 0)
  {
    if(get_value(buffer, sizeof(buffer), body, "param") == NULL)
      return 0;
    int i = 0;
    for(; i < 5; i++)
    {
      if(strcmp(buffer, water_params[i]) == 0)
        break;
    }
    char a[5];
    char b[5];
    char c[5];
    if(get_value(a, sizeof(a), body, "a") == NULL || get_value(b, sizeof(b), body, "b") == NULL || get_value(c, sizeof(c), body, "c") == NULL)
      return 0;
    unsigned char mode = (unsigned char)atoi(a);
    unsigned char arg1 = (unsigned char)atoi(b);
    unsigned char arg2 = (unsigned char)atoi(c);
    switch(i)
    {
      case 0:
      case 1:
      case 2:
      case 3:
        watered_flags &= ~(1 << i);
        if(mode == 0) //moisture
        {
          mode_flags &= ~(1 << i);
          moisture_lower[i] = arg1;
          moisture_upper[i] = arg2;
        }
        else //time
        {
          mode_flags |= 1 << i;
          hour[i] = arg1;
          minute[i] = arg2;
        }
        break;
      case 4:
        watered_flags = 0;
        for(i = 0; i < 4; i++)
        {
          if(mode == 0)
          {
            mode_flags &= ~(1 << i);
            moisture_lower[i] = arg1;
            moisture_upper[i] = arg2;
          }
          else
          {
            mode_flags |= 1 << i;
            hour[i] = arg1;
            minute[i] = arg2;
          }
        }
        break;
      case 5:
        break;
    }
  }
  else if(strcmp(buffer, "water") == 0)
  {
    if(get_value(buffer, sizeof(buffer), body, "param") == NULL)
      return 0;
    int i = 0;
    for(; i < 5; i++)
    {
      if(strcmp(buffer, water_params[i]) == 0)
        break;
    }
    switch(i)
    {
      case 0:
      case 1:
      case 2:
      case 3:
        override_flags |= 1 << i;
        override_timestamps[i] = millis();
        break;
      case 4:
        unsigned long ts = millis();
        for(i = 0; i < 4; i++)
        {
          override_flags |= 1 << i;
          override_timestamps[i] = ts;
        }
        break;
      case 5:
        break;
    }
  }
  else if(strcmp(buffer, "conf") == 0)
  {
    char key[2] = "a\0";
    int i = 0;
    unsigned char scratch = 0;
    for(; i < 4; i++)
    {
      if(get_value(buffer, sizeof(buffer), body, key) == NULL)
        return 1;
      key[0]++;
      scratch = (unsigned char)atoi(buffer);
      mode_flags |= scratch << i;
    }
    for(i = 0; i < 4; i++)
    {
      if(get_value(buffer, sizeof(buffer), body, key) == NULL)
        return 1;
      key[0]++;
      scratch = (unsigned char)atoi(buffer);
      moisture_lower[i] = scratch;
    }
    for(i = 0; i < 4; i++)
    {
      if(get_value(buffer, sizeof(buffer), body, key) == NULL)
        return 1;
      key[0]++;
      scratch = (unsigned char)atoi(buffer);
      moisture_upper[i] = scratch;
    }
    for(i = 0; i < 4; i++)
    {
      if(get_value(buffer, sizeof(buffer), body, key) == NULL)
        return 1;
      key[0]++;
      scratch = (unsigned char)atoi(buffer);
      hour[i] = scratch;
    }
    for(i = 0; i < 4; i++)
    {
      if(get_value(buffer, sizeof(buffer), body, key) == NULL)
        return 1;
      key[0]++;
      scratch = (unsigned char)atoi(buffer);
      minute[i] = scratch;
    }
  }
  return 0;
}

void toggle_state()
{
  detachInterrupt(digitalPinToInterrupt(MENU_BUTTON));
  lastPress = millis();
  menuState = !menuState;
}

void render_display()
{
  if(!menuState) {
    u8g.firstPage();
    do {
      drawTH();
      drawflower(); 
    } while ( u8g.nextPage() );
  } else {
    u8g.firstPage();
    do {
      printtime();
    } while ( u8g.nextPage() );
  }
}

void check_overrides(unsigned long current_millis)
{
  for(int i = 0; i < 4; i++)
  {
    if((override_flags >> i & 1) == 0)
      continue;
    if((current_millis < override_timestamps[i]) || (current_millis - override_timestamps[i] > 7000))
      override_flags &= ~(1 << i);
  }
}

void check_day()
{
  uint8_t current = rtc.day();
  if(today != current)
  {
    today = current;
    watered_flags = 0;
  }
}

void read_moisture()
{
  int mapped;
  int average;
  uint8_t pin[4] = { MOISTURE_1, MOISTURE_2, MOISTURE_3, MOISTURE_4 };
  for(char i = 0; i < 4; i++) {
    average = 0;
    for(char j = 0; j < 5; j++) {
      mapped = map(analogRead(pin[i]), 590, 360, 0, 100);
      //tried using constrain() after mapping but was still getting values > 100 and < 0
      //thus, we revert to good ol' reliable ternary statements
      mapped = mapped > 100 ? mapped = 100 : mapped;
      mapped = mapped < 0 ? mapped = 0 : mapped;
      average += mapped;
      delay(20);
    }
    moisture_values[i] = average / 5;
  }
}

void water_flower(unsigned long current_millis)
{
  uint8_t relays[4] = { RELAY_1, RELAY_2, RELAY_3, RELAY_4 };
  for(char i = 0; i < 4; i++)
  {
    if((mode_flags >> i & 1) == 0) //moisture mode
    {
      if(((override_flags >> i & 1) != 0 || moisture_values[i] <= moisture_lower[i]) && (relay_state_flags >> i & 1) == 0)
      {
        if((override_flags >> i & 1) == 0)
          watered_timestamps[i] = current_millis; //so if user switches to time mode while watering
        else
          watered_timestamps[i] = override_timestamps[i]; //so if user switches to time mode while watering
        digitalWrite(relays[i], HIGH);
        relay_state_flags |= 1 << i;
      }
      else if(((override_flags >> i & 1) != 1 && moisture_values[i] >= moisture_upper[i]) && (relay_state_flags >> i & 1) == 1)
      {
        digitalWrite(relays[i], LOW);
          relay_state_flags &= ~(1 << i);
      }
    }
    else //time mode
    {
      if(((override_flags >> i & 1) != 0 || (watered_flags >> i & 1) == 0 && hour[i] == rtc.hour() && minute[i] == rtc.minute()) && (relay_state_flags >> i & 1) == 0)
      {
        if((override_flags >> i & 1) == 0)
        {
          watered_flags |= 1 << i;
          watered_timestamps[i] = current_millis;
        }
        else
        {
          watered_timestamps[i] = override_timestamps[i];
        }
        digitalWrite(relays[i], HIGH);
        relay_state_flags |= 1 << i;
      }
      else if(((override_flags >> i & 1) != 1 && ((current_millis < watered_timestamps[i]) || (current_millis - watered_timestamps[i] > 7000))) && (relay_state_flags >> i & 1) == 1)
      {
        digitalWrite(relays[i], LOW);
        relay_state_flags &= ~(1 << i);
      }
    }
  }
  if(relay_state_flags == 0)
  {
    if(pump_state_flag == 1)
    {
      digitalWrite(PUMP, LOW);
      pump_state_flag = 0;
    }
  }
  else
  {
    if(pump_state_flag == 0)
    {
      digitalWrite(PUMP, HIGH);
      pump_state_flag = 1;
    }
  }
}


void drawbender(void)
{
  u8g.firstPage();
  do
  {
    u8g.drawXBMP(0, 0, 128, 64, bender);
  } while ( u8g.nextPage() );
  delay(3000);
}


void drawflower(void)
{
  u8g_uint_t x_flower[4] = { 0, 32, 64, 96 };
  for(char i = 0; i < 4; i++)
  {
    if(moisture_values[i] < moisture_lower[i])
      u8g.drawXBMP(x_flower[i], 0, 32, 30, bitmap_bad);
    else
      u8g.drawXBMP(x_flower[i], 0, 32, 30, bitmap_good);
  }
}

void drawTH(void)
{
  u8g_uint_t x_one_digit[4] = { 14, 46, 78, 110 };
  u8g_uint_t x_two_digit[4] = { 7, 39, 71, 103 };
  u8g_uint_t x_three_digit[4] = { 2, 32, 66, 98 };
  u8g_uint_t x_percent[4] = { 23, 54, 87, 119 };
  u8g_uint_t x_name[4] = { 9, 41, 73, 105 };
  char* name[4] = { "A0", "A1", "A2", "A3" };
  u8g.setFont(u8g_font_7x14);
  for(char i = 0; i < 4; i++)
  {
    u8g.setPrintPos(x_name[i], 60);
    u8g.print(name[i]);
    u8g.setPrintPos(x_percent[i], 45);
    u8g.print("%");
    if(moisture_values[i] < 10)
      u8g.setPrintPos(x_one_digit[i], 45);
    else if(moisture_values[i] < 100)
      u8g.setPrintPos(x_two_digit[i], 45);
    else
      u8g.setPrintPos(x_three_digit[i], 45);
    u8g.print(moisture_values[i], DEC);
  }
}

void printtime()
{
  uint8_t value = rtc.dayOfWeek();
  u8g.setPrintPos(36 ,20);
  u8g.print(daysOfTheWeek[value-1]);
  u8g.setPrintPos(36, 35);
  uint8_t (uRTCLib::*functions[])() = { &uRTCLib::month, &uRTCLib::day, &uRTCLib::year, &uRTCLib::hour, &uRTCLib::minute, &uRTCLib::second };
  for(char i = 0; i < 6; i++)
  {
    value = (rtc.*functions[i])();
    if(i != 2 && value < 10) //not rtc.year and value < 10, for brevity - technically shouldn't matter since year will never be < 10
      u8g.print("0");
    u8g.print((int)value, DEC);
    switch(i)
    {
      case 0:
      case 1:
        u8g.print('/');
        break;
      case 2:
        u8g.setPrintPos(36, 50);
        break;
      case 3:
      case 4:
        u8g.print(':');
        break;
      case 5:
        break;
    }
  }
}

//if delim is NULL, count is used instead
int receive_until(char* response_buffer, size_t response_buffer_length, char* delim, size_t count, size_t timeout)
{
  char* eol = NULL;
  size_t delim_index = 0;
  size_t length = 0;
  int overflow = 0;
  uint64_t timeout_us = timeout * 1000000;
  uint64_t us_elapsed = 0;
  memset(response_buffer, '\0', response_buffer_length);
  while(1)
  {
    while(bindex < nbytes)
    {
      size_t tindex = 0;
      while(bindex + tindex < nbytes)
      {
        if(delim == NULL)
        {
          if((count - delim_index) > (nbytes - bindex))
          {
            delim_index += (nbytes - bindex);
            break;
          }
          //calculate and set eol
          eol = &buffer[bindex + (count - delim_index)];
          break;  
        }
        if(buffer[bindex + tindex] == delim[delim_index])
        {
          delim_index++;
          tindex++;
          if(delim[delim_index] == '\0')
          {
            eol = &buffer[bindex + tindex];
            delim_index = 0;
            break;
          }
          continue;
        }
        delim_index = 0;
        if(buffer[bindex + tindex] == delim[0])
          continue;          
        tindex++;
      }
      size_t tlen;
      if(eol == NULL)
      {
        if(overflow)
          break;
        tlen = nbytes - bindex;
      }
      else
      {
        if(overflow)
          return 0;
        tlen = eol - &buffer[bindex];
      }
      if((length + tlen) > (response_buffer_length - 1))
      {
        memset(response_buffer, '\0', response_buffer_length);
        if(eol != NULL)        
          return 0;
        overflow = 1;
        break;
      }
      memcpy(&response_buffer[length], &buffer[bindex], tlen);
      length += tlen;      
      response_buffer[length] = '\0';
      if(eol == NULL)
        break;
      bindex = eol - buffer;
      return length;
    }
    while((nbytes = Serial1.readBytes(buffer, 64)) == 0)
    {
      delayMicroseconds(5);
      us_elapsed += 14;
      if(us_elapsed > timeout_us)
      {
        return -1;
      }
      continue;      
    }
    buffer[nbytes] = '\0';
    bindex = 0;
  }
}

int wifi_query(char* response, size_t length, char* command, size_t timeout)
{
  int echo = 0;
  size_t clen = strlen(command);
  int eoc = 0;
  int res_len = 0;
  int return_code = 0;
  char line[128];
  size_t total_len = 0;
  memset(response, '\0', length);
  Serial1.println(command);
  while(!eoc)
  {
    res_len = receive_until(line, sizeof(line), CR_LF, 0, timeout);
    if(res_len <= 0)
    {
      memset(response, '\0', length);
      return_code = -1;
      break;
    }
    if(!echo)
    {
      if(strncmp(line, command, clen) == 0)
        echo = 1;
      continue;
    }
    if(strcmp(line, "OK\r\n") == 0)
    {
      eoc = 1;
      return_code = OK;
    }
    else if(strcmp(line, "FAIL\r\n") == 0)
    {
      eoc = 1;
      return_code = FAIL;
    }
    else if(strcmp(line, "ERROR\r\n") == 0)
    {
      eoc = 1;
      return_code = ERROR;
    }
    total_len += strlen(line);
    if(total_len > (length - 1))
    {
      memset(response, '\0', length);
      return_code = -1;
      break;
    }
    strcat(response, line);
  }
  return return_code;
}

void wifi_init(char* response, size_t length) {
  int ready = 0;
  int res_len = 0;
  digitalWrite(RESET_ESP, LOW);
  Serial.println("Resetting ESP8266...");
  nbytes = 0;
  bindex = 0;
  delay(2000);
  digitalWrite(RESET_ESP, HIGH);
  while(!ready)
  {
    res_len = receive_until(response, length, CR_LF, 0, 15);
    if(res_len <= 0)
      reset_arduino();
    if(strcmp(response, "ready\r\n") == 0)
      ready = 1;
  }
}

void wifi_configure(char* response, size_t length)
{
  char* commands[] = { "AT+CWMODE=1", "AT+CIPMUX=0", "AT+CIPMODE=0", NULL };
  int i = 0;
  int return_code = 0;
  while(commands[i] != NULL)
  {
    return_code = wifi_query(response, length, commands[i], 15);
    if(return_code != OK)
    {
      wifi_init(response, length);
      i = 0;
      continue;
    }
    i++;
  }
}

void wifi_reconnect(char* response, size_t length)
{
  char* commands[] = { "AT+CWQAP", "AT+CWJAP?", "AT+CWJAP=\"AP_NAME_HERE\",\"AP_PASS_HERE\"", "AT+CWJAP?", NULL };
  char* needle[] = { NULL, "No AP", NULL, "+CWJAP:\"AP_NAME_HERE\"" };
  int i = 0;
  int return_code = 0;
  while(commands[i] != NULL)
  {
    return_code = wifi_query(response, length, commands[i], 15);
    if(return_code != OK || needle[i] != NULL && strstr(response, needle[i]) == NULL)
    {
      wifi_init(response, length);
      wifi_configure(response, length);
      i = 0;
      continue;
    }
    i++;
  }
}

int wifi_send(char* response, size_t length, char* ip, char* port, char* payload, size_t p_length, size_t max_retry)
{
  char command[50];
  char* colon = NULL;
  int return_code = 0;
  int res_len = 0;
  size_t ipd_len = 0;
  char itoa_buff[12];
  size_t retries = 0;
  while(1)
  {
    if(retries == max_retry)
    {
      response[0] = '\0';          
      return ERROR;
    }
    return_code = wifi_query(response, length, "AT+CWJAP?", 15);
    if(return_code != OK || strstr(response, "No AP") != NULL)
    {
      wifi_reconnect(response, length);
      continue;
    }
    memset(command, '\0', sizeof(command));
    strcat(command, "AT+CIPSTART=\"TCP\",\"");
    strcat(command, ip);
    strcat(command, "\",");
    strcat(command, port);
    return_code = wifi_query(response, length, command, 15);
    if(return_code != OK)
    {
      retries++;
      continue;
    }
    memset(command, '\0', sizeof(command));
    strcat(command, "AT+CIPSEND=");
    itoa((int)p_length, itoa_buff, 10);
    strcat(command, itoa_buff);
    Serial1.println(command);
    memset(response, '\0', length);
    res_len = receive_until(response, length, ">", 0, 15);
    if(res_len <= 0)
    {
      wifi_reconnect(response, length);
      continue;
    }
    Serial1.print(payload);
    memset(response, '\0', length);
    res_len = receive_until(response, length, "SEND OK\r\n", 0, 15);
    if(res_len <= 0)
    {
      wifi_reconnect(response, length);
      continue;
    }
    memset(response, '\0', length);
    res_len = receive_until(response, length, "+IPD,", 0, 15);
    if(res_len <= 0)
    {
      retries++;
      wifi_reconnect(response, length);
      continue;
    }
    memset(response, '\0', length);
    res_len = receive_until(response, length, ":", 0, 15);
    if(res_len <= 0)
    {
      retries++;
      wifi_reconnect(response, length);
      continue;
    }
    colon = strchr(response, ':');
    *colon = '\0';
    ipd_len = (size_t)atoi(response);
    if(ipd_len == 0)
      continue;
    memset(response, '\0', length);
    res_len = receive_until(response, length, NULL, ipd_len, 15);
    if(res_len < 0)
    {
      retries++;
      wifi_reconnect(response, length);
      continue;
    }
    char line[30];
    return_code = wifi_query(line, sizeof(line), "AT+CIPCLOSE", 15);
    if(return_code < 0)
    {
      wifi_reconnect(response, length);
      continue;
    }
    break;
  }
  return OK;
}

char* get_value(char* buffer, size_t length, char* json, char* key)
{
  char scratch[30];
  memset(scratch, '\0', sizeof(scratch));
  strcat(scratch, "\"");
  strcat(scratch, key);
  strcat(scratch, "\":\"");
  char* start = strstr(json, scratch);
  if(start == NULL)
    return NULL;
  start += strlen(scratch);
  char* end = strchr(start, '\"');
  if(end == NULL)
    return NULL;
  *end = '\0';
  if(strlen(start) > (length - 1))
    return NULL;
  memset(buffer, '\0', length);
  strcat(buffer, start);
  *end = '\"';
  return buffer;
}
