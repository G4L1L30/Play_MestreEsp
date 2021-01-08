#include <ETH.h>
#include "time.h"
#include "string.h"
#include "WiFi.h" PROGMEM
#include <ESP32WebServer.h> PROGMEM
#include "SSD1306.h"
#include <Update.h>
#include "esp_wps.h"
#include "WatchDog.h"

/*Variaveis*/
static bool eth_connected = false;
int ip[] = {0, 0, 0, 0};
int gateway[] = {0, 0, 0, 0};
int subnet[] = {0, 0, 0, 0};

const char *ssid = "WIFICABONNET";
const char *password = "forcaf123";

String sinalWifi;
ESP32WebServer server(80);

struct tm *dataNow;
String StatusWifi;
SemaphoreHandle_t httpMutex = xSemaphoreCreateMutex();
hw_timer_t *timer = NULL;

void loopWifiServer()
{
    try
    {
        server.handleClient();
    }
    catch (...)
    {
        resetModule();
    }
}

void WiFiEvent(WiFiEvent_t event)
{
    switch (event)
    {
    case SYSTEM_EVENT_ETH_START:
        Serial.println("ETH Started");
        //set eth hostname here
        ETH.setHostname("esp32-ethernet");
        break;
    case SYSTEM_EVENT_ETH_CONNECTED:
        Serial.println("ETH Connected");
        break;
    case SYSTEM_EVENT_ETH_GOT_IP:
        Serial.print("ETH MAC: ");
        Serial.print(ETH.macAddress());
        Serial.print(", IPv4: ");
        Serial.print(ETH.localIP());
        if (ETH.fullDuplex())
        {
            Serial.print(", FULL_DUPLEX");
        }
        Serial.print(", ");
        Serial.print(ETH.linkSpeed());
        Serial.println("Mbps");
        eth_connected = true;
        break;
    case SYSTEM_EVENT_ETH_DISCONNECTED:
        Serial.println("ETH Disconnected");
        eth_connected = false;
        break;
    case SYSTEM_EVENT_ETH_STOP:
        Serial.println("ETH Stopped");
        eth_connected = false;
        break;
    default:
        break;
    }
}

void handleRoot()
{
    try
    {
        xSemaphoreTake(httpMutex, portMAX_DELAY);

        String Cmd = "";

        if (server.arg("cmd") == "reset")
        {
            server.sendHeader("Connection", "close");
            server.send(200, "text/html", "OK");
            delay(2000);
            resetModule();
        }

        String serverIndex =
            "<br> Milisegundos:  " + String(millis()) + " "
                                                        "<br> Clock:  " +
            String(clock()) + " "
                              "<br>Comando :  (" +
            Cmd + ")"

                  "<br><script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"

                  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
                  "<input type='file' name='update'>"
                  "<input type='submit' value='Update'>"
                  "</form>"
                  "<div id='prg'>progress: 0%</div>"
                  "<script>"
                  "$('form').submit(function(e){"
                  "e.preventDefault();"
                  "var form = $('#upload_form')[0];"
                  "var data = new FormData(form);"
                  " $.ajax({"
                  "url: '/update',"
                  "type: 'POST',"
                  "data: data,"
                  "contentType: false,"
                  "processData:false,"
                  "xhr: function() {"
                  "var xhr = new window.XMLHttpRequest();"
                  "xhr.upload.addEventListener('progress', function(evt) {"
                  "if (evt.lengthComputable) {"
                  "var per = evt.loaded / evt.total;"
                  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
                  "}"
                  "}, false);"
                  "return xhr;"
                  "},"
                  "success:function(d, s) {"
                  "console.log('success!')"
                  "},"
                  "error: function (a, b, c) {"
                  "}"
                  "});"
                  "});"
                  "</script>";

        xSemaphoreGive(httpMutex);
        server.sendHeader("Connection", "close");
        server.send(200, "text/html", serverIndex);
    }
    catch (...)
    {
        resetModule();
    }
}

void setupWifiServer()
{
    try
    {
        WiFi.onEvent(WiFiEvent);
        ETH.begin();
        // define IP fixo
        if (subnet[0] > 0)
        {
            IPAddress _ip(ip[0], ip[1], ip[2], ip[3]);
            IPAddress _gateway(gateway[0], gateway[1], gateway[2], gateway[3]);
            IPAddress _subnet(subnet[0], subnet[1], subnet[2], subnet[3]);
            ETH.config(_ip, _gateway, _subnet);
        }

        time_t timeout = millis() + 10000;
        server.on("/", handleRoot);
        /*handling uploading firmware file */
        server.on(
            "/update", HTTP_POST, []() {

      Serial.printf(" timerDetachInterrupt ");

      server.sendHeader("Connection", "close");
      server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      esp_wifi_wps_disable(); ESP.restart(); }, []() {
      HTTPUpload& upload = server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        timerAlarmDisable(timer);

        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      }
      else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        }
        else {
          Update.printError(Serial);
        }
      } });

        // start the server
        server.begin();
    }
    catch (...)
    {
        resetModule();
    }
}