#define _TASK_WDT_IDS
#undef __STRICT_ANSI__

#include <Adafruit_NeoPixel.h>
#include <ArduinoOTA.h>
#include <Wifi.h>
#include <mDNS.h>
#include <TaskScheduler.h>
#include <WiFiUDP.h>
#include <string.h>
#include <arduino.h>

#define numtasks 25
#define maxcolors 20
#define udpport 9100
#define commandlen 128

const char *ssid = "TRNNET-2G";
const char *password = "ripcord1";

void parse_command();
void check_udp();
void t1_callback();

WiFiUDP Udp;
WiFiUDP Udp1;
Adafruit_NeoPixel strip = Adafruit_NeoPixel(numtasks, 27, NEO_GRB + NEO_KHZ800);
Scheduler runner;

Task *tasks[numtasks];

unsigned int numcolors[numtasks], currcolor[numtasks];
unsigned long colors[numtasks][maxcolors];
unsigned long steps[numtasks][maxcolors], currstep[numtasks];
unsigned char first[numtasks];
char command[commandlen+1];

void connect() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println(WiFi.localIP());
  Udp.begin(udpport);
  ArduinoOTA.begin();
}

  IRAM_ATTR void reset (){
   //void reset(){
   Serial.println("reset");

    memset(command,0,commandlen+1);
    strcpy(command,"0 500 000000");
    parse_command();

    Udp1.beginPacket("192.168.1.255",4100);
    Udp1.print("11 500 000000");
    Udp1.endPacket();
}

void check_udp() {

  int packetsize = Udp.parsePacket();
 if (packetsize>0) {
     Serial.println(packetsize);
     Serial.println("Last");
 }
  if (packetsize) {

    memset(command,0,commandlen+1);
    Udp.read(command, 128);
    Serial.println((int) command[packetsize-1]);
    parse_command();
    Udp.beginPacket(Udp.remoteIP(), Udp.remotePort());
    Udp.println("ok");
    Udp.endPacket();
  }
}

void parse_command() {

  unsigned ledId, pause;
  char *ptr, *ptr1;
  char *sav, *sav1;

  Serial.println(command);
  Serial.println(strlen(command));
  ptr = strtok_r(command, " ", &sav);
  ledId = atoi(ptr);
  (*tasks[ledId]).disable();

  currcolor[ledId] = 0;
  currstep[ledId] = 0;
  ptr = strtok_r(NULL, " ", &sav);

  if (strcmp(ptr, "+")) {
    numcolors[ledId] = 0;
    pause = atoi(ptr);
    (*tasks[ledId]).setInterval(pause);
  }

  while (ptr != NULL) {
    ptr = strtok_r(NULL, " ", &sav);
    if (ptr != NULL) {

      ptr = strupr(ptr);
      if (strchr(ptr, 'X') != NULL) {
        ptr1 = strtok_r(ptr, "X", &sav1);
        steps[ledId][numcolors[ledId]] = atol(ptr1);
        ptr1 = strtok_r(NULL, "X", &sav1);
    } else {
        steps[ledId][numcolors[ledId]] = 1;
        ptr1=ptr;
    }
      colors[ledId][numcolors[ledId]] = strtol(ptr1, NULL, 16);
      numcolors[ledId]++;
    }
  }
  strip.setPixelColor(ledId, colors[ledId][currcolor[ledId]]);
  strip.show();
  first[ledId] = 1;
  (*tasks[ledId]).enable();
}

void t1_callback() {
  int ledId;

  Task &taskRef = runner.currentTask();
  ledId = taskRef.getId();
  
  if (steps[ledId][currcolor[ledId]] != 0 and not first[ledId]) {
    if (currstep[ledId] == steps[ledId][currcolor[ledId]] - 1) {

      if (currcolor[ledId] < numcolors[ledId] - 1)
        currcolor[ledId]++;
      else
        currcolor[ledId] = 0;

      currstep[ledId] = 0;
      strip.setPixelColor(ledId, colors[ledId][currcolor[ledId]]);
      strip.show();
    } else
      currstep[ledId]++;
  }
  first[ledId] = 0;
}
void setup(void) {

  Serial.begin(9600);
  connect();
  ArduinoOTA.begin();

  //pinMode(D1,INPUT);
  //attachInterrupt(digitalPinToInterrupt(D1),reset,RISING);

  for (int i = 0; i <= numtasks-1; i++) {
    numcolors[i] = 1;
    currcolor[i] = 0;
    currstep[i] = 0;
    first[i] = 0;
    for (int j = 0; j <= 19; j++) {
      colors[i][j] = 0;
      steps[i][j] = 1;
    }
  }

  runner.init();
  for (int i = 0; i <= numtasks-1; i++) {
    tasks[i] = new Task(500, TASK_FOREVER, &t1_callback);
    (*tasks[i]).setId(i);

    runner.addTask((*tasks[i]));
    (*tasks[i]).enable();
  }
  //pinMode(D5, INPUT_PULLUP);
  strip.begin();
}

void loop(void) {
  runner.execute();
  ArduinoOTA.handle();
  // if (WiFi.status() != WL_CONNECTED)
  // connect();
  check_udp();
}