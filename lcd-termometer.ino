#include "resources.h"

#include <ESP8266WiFi.h>
#include <DNSServer.h> 
#include <ESP8266WebServer.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS D3

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

const int rs = D8, en = D9, d4 = D4, d5 = D5, d6 = D6, d7 = D7;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

const char* SSID_NAME = "Termometer";

const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(172, 0, 0, 1); // Gateway

unsigned long lastTick=0;
float tempC1, tempC2;

float ts1[600], ts2[600];
int tp1, tp2;

DNSServer dnsServer; 
ESP8266WebServer webServer(80);

String chart(String ind, float temp, String min, String max) {
  return "<div id=\"container"+ind+"\" style=\"width:450px;\"></div>\r\n  <script>\r\n    Highcharts.chart('container"+ind+"', {\r\n      chart: {\r\n        type: 'gauge',\r\n        plotBorderWidth: 1,\r\n       plotBackgroundColor: {\r\n          linearGradient: { x1: 0, y1: 0, x2: 0, y2: 1 },\r\n         stops: [\r\n            [0, '#EFEFFF'],\r\n           [0.3, '#FFFFFF'],\r\n           [1, '#CCCCFF']\r\n          ]\r\n       },\r\n        plotBackgroundImage: null,\r\n        height: 340,\r\n      },\r\n\r\n      title: {\r\n        text: 'TermoProbe "+ind+"',\r\n\r\n     },\r\n\r\n      pane: [{\r\n        startAngle: -90,\r\n        endAngle: 90,\r\n       background: null,\r\n       center: ['50%', '90%'],\r\n       size: 360\r\n     }],\r\n\r\n     tooltip: {\r\n        enabled: false\r\n      },\r\n\r\n      yAxis: [{\r\n       min: "+min+",\r\n       max: "+max+",\r\n       minorTickPosition: 'outside',\r\n       tickPosition: 'outside',\r\n        labels: {\r\n         rotation: 'auto',\r\n         distance: 20\r\n        },\r\n        plotBands: [{\r\n         from: -50,\r\n          to: 0,\r\n          color: 'blue',\r\n          innerRadius: '97%',\r\n         outerRadius: '99%'\r\n        }, {\r\n          from: 0,\r\n          to: 20,\r\n         color: 'green',\r\n         innerRadius: '97%',\r\n         outerRadius: '99%'\r\n        }, {\r\n          from: 20,\r\n         to: 100,\r\n          color: 'yellow',\r\n          innerRadius: '97%',\r\n         outerRadius: '99%'\r\n        },\r\n        {\r\n         from: 100,\r\n          to: 150,\r\n          color: 'red',\r\n         innerRadius: '97%',\r\n         outerRadius: '99%'\r\n        }],\r\n       pane: 0,\r\n        title: {\r\n          text: '<b style=\"font-size:16px; font-weight:bolder;\">"+temp+"</b>',\r\n          y: -30\r\n        }\r\n     }],\r\n\r\n     plotOptions: {\r\n        gauge: {\r\n          dataLabels: {\r\n           enabled: false\r\n          },\r\n          dial: {\r\n           radius: '102%'\r\n          }\r\n       }\r\n     },\r\n\r\n\r\n      series: [{\r\n        data: ["+String(temp)+"],\r\n       yAxis: 0\r\n      }]\r\n\r\n    }, function (chart) {\r\n\r\n     function upData() {\r\n       fetch('./t"+ind+"').then(res => res.json()).then(res => {\r\n         chart.update({ series: { data: res } }, true, true);\r\n\r\n        }).finally(() => { setTimeout(() => { upData() }, 1000); });\r\n      }\r\n     setTimeout(() => { upData() }, 1000);\r\n   });\r\n\r\n </script>";
}
String html(String body) {
  return String("<html>\r\n<head>")
    + String("\r\n\t<script src=\"./jquery\"></script>")
    + String("\r\n\t<script src=\"./highstock\"></script>")
    + String("\r\n\t<script src=\"./highcharts\"></script>")
    + String("\r\n\t<title>Termometer</title>\r\n</head>\r\n<body>\r\n" +body+ " \r\n</body>\r\n</html>")
  ;
}
String index() {
  String min = "0";
  String max = "50";
  if(tempC1<0 || tempC1>50 || tempC2<0 || tempC2>50) {
    min = "-50";
    max = "150";
  }
  String charts = chart("1", tempC1, min, max) + chart("2", tempC2, min, max);
  return html("<div style=\"display:flex; justify-content: space-evenly;\" >"+charts+"</div>");
}


void handleNotFound()
{ 
    webServer.sendHeader("Location", "http://172.0.0.1/",true); //Redirect to our html web page 
    webServer.send(302, "text/plane",""); 
}
void printTemps() {
  // put your main code here, to run repeatedly:
  sensors.requestTemperatures();
  
  tempC1 = sensors.getTempCByIndex(0);
  tempC2 = sensors.getTempCByIndex(1);

  lcd.clear();
  if(tempC1 != DEVICE_DISCONNECTED_C) {
    lcd.print("T1: ");
    lcd.print(tempC1);
    lcd.print(" C");
  }
  else {
    lcd.print("ERR");
  }

  lcd.setCursor(0,2);
  
  if(tempC2 != DEVICE_DISCONNECTED_C) {
    lcd.print("T2: ");
    lcd.print(tempC2);
    lcd.print(" C");
  }
  else {
    lcd.print("ERR");
  }
}
void setup() {
  // put your setup code here, to run once:
  lcd.begin(16,2);
  lcd.print("Termometer :)");
  lcd.setCursor(0,2);
  lcd.print("Nikola Nestorov");

  sensors.begin();

  delay(3000);

  lcd.clear();
  lcd.print("Making WiFi ...");
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  
  lcd.setCursor(0,2);
  lcd.print("Starting Server");
  
  dnsServer.start(DNS_PORT, "*", APIP); // DNS spoofing (Only for HTTP)
  webServer.on("/",[]() { webServer.send(HTTP_CODE, "text/html", index()); });
  webServer.on("/jquery",[]() { webServer.send(HTTP_CODE, "application/javascript", jquery); });
  webServer.on("/highstock",[]() { webServer.send(HTTP_CODE, "application/javascript", highstock); });
  webServer.on("/highcharts",[]() { webServer.send(HTTP_CODE, "application/javascript", highcharts); });

  webServer.on("/t1",[]() { webServer.send(HTTP_CODE, "application/json", "["+String(tempC1)+"]"); });
  webServer.on("/t2",[]() { webServer.send(HTTP_CODE, "application/json", "["+String(tempC2)+"]"); });
  
  webServer.onNotFound(handleNotFound);
  webServer.begin();
}
void loop() {
  if ((millis() - lastTick) > TICK_TIMER) {
    lastTick = millis();
    printTemps();
  } 
  dnsServer.processNextRequest(); 
  webServer.handleClient(); 

  //int aValue = analogRead(A0);
  //lcd.setCursor(0,2);
  //lcd.print(aValue);
}
