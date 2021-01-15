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
// TQL - tempo que define a quebra de lotes
int id_prxlote = 0, priFila = 0, TQL = 5000, limiteVetor = 500;
long int loops = 0;
String StatusWifi, s_aux = "", sinalWifi;
String lotes[500] = {"", "", ""};
bool dLote = true; // controla a quebra do lote para que data ini e data fim estejam no mesmo dia

const char *ssid = "WIFICABONNET";
const char *password = "forcaf123";
const char *c_aux = "";

ESP32WebServer server(80);
clock_t tConfirmLote, tLoop, ttimeUltLote = 0;
time_t timeServerAtu, timeServerAtuRn, timeNow, timeServerDif, timeServer, timeServerResetNullptr, timeFimLote, timeIniLote = 0;
struct tm *dataNow;
SemaphoreHandle_t httpMutex = xSemaphoreCreateMutex();


time_t getTime_t()
{
    return timeServer + time(nullptr) - timeServerResetNullptr;
}

void loopWifiServer()
{
    try
    {
        server.handleClient(); //Resposta do servidor
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

//inicializa os lotes com '.'
void setupParametros()
{
    try
    {
        int i = 0;
        for (i = 0; i < limiteVetor; i++)
        {
            lotes[i] = ".";
        }
        id_prxlote = 0;
        priFila = 0;
    }
    catch (...)
    {
        resetModule();
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
                              "<br> Dado: " +
            String(apontamentos) + " "

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

//pagina que retorna a lista de lotes pendentes de envio
void handlegetLotes()
{
    try
    {
        String html = "", s_aux1 = "";
        //const char* c_aux;
        int i = 0;
        timeServerAtu = server.arg("d").toInt();
        timeServerAtuRn = time(nullptr);
        timeServerDif = getTime_t() - timeServerAtu;

        if (server.arg("tql") != "")
        {
            TQL = server.arg("tql").toInt() * 1000;
        }

        if (timeServer == 0)
        {
            timeServer = server.arg("d").toInt();
            timeServerResetNullptr = time(nullptr);
            s_aux = "|Data Atualizada:" + String(timeServer);
            c_aux = s_aux.c_str();
        }
        else
        {
            if (server.arg("at").toInt() == 1)
            {
                timeServer = server.arg("d").toInt();
                timeServerResetNullptr = time(nullptr);
            }
        }
        // codigo do lote
        xSemaphoreTake(httpMutex, portMAX_DELAY);
        for (i = 0; i < limiteVetor; i++)
        {
            if (lotes[i] != ".")
            {
                html += lotes[i];
            }
        }
        xSemaphoreGive(httpMutex);
        if (html == "") //nao existe lotes e o confirma lotes sera acionado  do contrario o confirma lotes sao acionados quando existe uma delecao de registros assim aumentando a seguranca do metodo contingencia
        {
            tConfirmLote = clock(); // esta variavel indica se o sistema esta sem comunicacao e serve para entrar em modo critico de operacao e criacao de lotes
        }
        // envia logs no fim da cominucacao
        html += "logs#" + String(((temprature_sens_read() - 32) / 1.8)) + "#" + String(hallRead()) + "#" + sinalWifi;
        //hallRead - MEDE A INTERFERENCIA ELETRO MAGNETICA
        server.setContentLength(html.length());
        server.send(200, "text/html", html);
    }
    catch (...)
    {
        resetModule();
    }
}

void handleconfirmLotes()
{
    try
    {
        String s_aux = server.arg("l");
        if (s_aux != "")
        {
            String s_aux1 = "";
            int l = s_aux.length();
            int x = 0;
            for (x = 0; x < l; x++)
            {
                if (s_aux[x] == '.')
                {
                    if (s_aux1 != "")
                    {
                        // apaga arquivo
                        lotes[s_aux1.toInt()] = ".";
                        xSemaphoreTake(httpMutex, portMAX_DELAY);
                        priFila = s_aux1.toInt() + 1;
                        xSemaphoreGive(httpMutex);
                        s_aux1 = "";
                    }
                }
                else
                {
                    s_aux1 += s_aux[x];
                }
            }
        }
        else
        {
            s_aux = "Zero lotes enviados";
        }
        tConfirmLote = clock();
        server.setContentLength(s_aux.length());
        server.send(200, "text/html", s_aux);
    }
    catch (...)
    {
        resetModule();
    }
}

void gravaLote()
{
    struct tm *dattime;
    try
    {
        //if (timeServerDif > 0 && CD1 <= 0 ) { // a data do  ESP e maior entao devemos controlar a geracao dos lotes ate a data sincronizar
        if (timeServerDif > 0)
        { // a data do  ESP e maior entao devemos controlar a geracao dos lotes ate a data sincronizar
            if (timeServerDif > (TQL / 1000))
                timeServer = timeServer - (TQL / 1000);
            else
                timeServer = timeServer - timeServerDif;
            //return false;
        }

        if (timeServer != 0)
        {                         // grava lote pois esta com data atualizada
            if (timeFimLote == 0) // siginifica que a data atualizou a variavel timeServer pore ainda nao atualizou a variavel timeFimLote   ai vou calcular as datas para nao mandar 1970 porem todo o tempo que o esp ficar desligado nao tera atualizacao
                timeIniLote = timeServer;
            else
                timeIniLote = timeFimLote;

            //      if (timeServerDif< 0 && CD1 <= 0 ) { // a data do schedule esta maior entao e sinal que a data do esp e menor e o ultimo lote tem data menor podendo ser atualizado
            if (timeServerDif < 0)
            { // a data do schedule esta maior entao e sinal que a data do esp e menor e o ultimo lote tem data menor podendo ser atualizado
                timeServerResetNullptr = timeServerAtuRn;
                timeServer = timeServerAtu;
                timeServerDif = 0;
            }
            timeFimLote = getTime_t();
            dattime = localtime(&timeIniLote);
            // limite do vetor cheio, porem primeiro vetor liberado, aponta novamente para o primeiro vetor
            if (id_prxlote == limiteVetor)
            {
                id_prxlote = 0;
            }
            dattime = localtime(&timeFimLote);
            String s_aux1 = apontamentos + "|";
            //Procurar nos lotes se ja tem o apontamento.
            //if(sizeof(lotes[id_prxlote]) - sizeof(s_aux1) < 0 || (lotes[id_prxlote] != s_aux1 && lotes[id_prxlote].substring(sizeof(lotes[id_prxlote]) - sizeof(s_aux1), sizeof(lotes[id_prxlote])) != s_aux1))//verificar substring
            if(lotes[id_prxlote] != s_aux1)
            {
              if(lotes[id_prxlote] == ".")
              {
                lotes[id_prxlote] = s_aux1;
              }
              else
              {
                lotes[id_prxlote] += s_aux1;
              }
              
              if ((clock() - tConfirmLote < TQL) || sizeof(lotes[id_prxlote]) + sizeof(s_aux1) > 255)
              {//Nao esta em contigencia, ou atingiu o tamanho limite do lote
                id_prxlote = id_prxlote + 1;
              }
            }
            
        }
    }
    catch (...)
    {
        resetModule();
    }
}

void handlelog()
{
    try
    {
        xSemaphoreTake(httpMutex, portMAX_DELAY);
        String html = "";
        int i = 0;
        html += "<h1><BR>VARIAVEIS... </h1>";
        /*########################################### VARIAVEIS DE CONFIGURACAO ################################################*/
        html += "  millis()             =" + String(millis()) + "<br>";
        //html += "  ipMaquina            =" + String(ETH.localIP()) + "<br>";
        html += "  TQL                  =" + String(TQL) + "<br>";
        html += "  c_aux                =" + String(c_aux) + "<br>";
        html += "  s_aux                =" + String(s_aux) + "<br>";
        html += "  id_prxlote           =" + String(id_prxlote) + "<br>";
        html += " priFila               =" + String(priFila) + "<br>";
        html += " loops                 =" + String(loops) + "<br>";
        html += " tConfirmLote          =" + String(tConfirmLote) + "<br>";
        html += " ttimeUltLote          =" + String(ttimeUltLote) + "<br>";
        html += " timeServer            =" + String(timeServer) + "<br>";
        html += " timeNow               =" + String(timeNow) + "<br>";
        html += " timeFimLote           =" + String(timeFimLote) + "<br>";
        html += " timeIniLote           =" + String(timeIniLote) + "<br>";
        //html += " StatusWifi            =" + String(StatusWifi) + "<br>";
        //html += " sinalWifi            =" + String(sinalWifi) + "<br>";
        //VETORES
        for (i = 0; i < limiteVetor; i++)
        {
            if (lotes[i] != ".")
            {
                html += "lote[" + String(i) + "]    = " + lotes[i] + "<br>";
            }
        }

        server.setContentLength(html.length());
        server.send(20000, "text/html", html);

        xSemaphoreGive(httpMutex);
    }
    catch (...)
    {
        resetModule();
    }
}

void handleResetSlave()
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
            "<br> Quantidade de resete slave 0: " + String(log_reset[0]) + " "
                                                                           "<br> Quantidade de resete slave 1: " +
            String(log_reset[1]) + " ";

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

        //time_t timeout = millis() + 10000;
        server.on("/", handleRoot);
        server.on("/getLotes", handlegetLotes);
        server.on("/confirmLotes", handleconfirmLotes);
        server.on("/log", handlelog);
        server.on("/reset", handleResetSlave);
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