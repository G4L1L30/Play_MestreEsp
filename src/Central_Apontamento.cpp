#include "Apontamento.h"
extern "C" // MEDE TEMPERATURA INTERNA DO ESP32
{
  uint8_t temprature_sens_read();
}
#include "Conexao.h"
bool espera;
int result;
String aux_apt;

//TASC NO COR ZERO DO ESP
//void setupfileSistem(void * pvParameters){
void setupfileSistem()
{
  try
  {
    setupParametros();
    setupWifiServer();
  }
  catch (...)
  {
    resetModule();
  }
}

void setupcoreZero(void *pvParameters)
{
  try
  {
    String msg = "";
    StatusWifi = "Conecting....";
    setupfileSistem();
    for (;;)
    {
      if (ssid != "")
      {
        if (WiFi.status() != WL_CONNECTED && eth_connected == false)
        {
          StatusWifi = "Conecting....";
          setupWifiServer(); // conecta
          delay(1000);
          Serial.println(WiFi.localIP());
          Serial.println(WiFi.macAddress());
          Serial.println("Try conect");
        }
        else
        {
          loopWifiServer();
        }
      }
      delay(1);
    }
    vTaskDelete(NULL);
  }
  catch (...)
  {
    resetModule();
  }
}

void setup()
{
  try
  {
    inicia_PinMode();
    pinMode(ent_sensor, INPUT_PULLUP);
    Serial.begin(115200);
    pinMode(SDA, INPUT_PULLUP);
    pinMode(SCL, INPUT_PULLUP);
    Wire.begin(SDA, SCL);
    espera = false;
    xTaskCreatePinnedToCore(setupcoreZero, "setupcoreZero", 8192, NULL, 0, NULL, 0);
    // configura WatchDog
    setupWatchDog();
  }
  catch (...)
  {
    resetModule();
  }
}

void loop()
{
  try
  {
    tLoop = millis();
    loopWatchDog();
    if (loops == 0)
    {
      loops = millis();
    }
    timeNow = getTime_t();
    dataNow = localtime(&timeNow);
    val_sensor = digitalRead(ent_sensor);
    for (int i = 0; i < tam_slave; i++)
    {
      envia_Sensor(val_sensor, slave[i]);
    }
    if (!espera)
    {
      escravo(slave[0]);
      delay(100);
      escravo(slave[1]);
      delay(100);
    }
    if (val_sensor == 0 && apontamentos.length() > 0) //Sensor Funcionando OK
    {
      result = gravaLote();
      espera = true;
    }
    else //Sensor com Problema
    {
      if (apontamentos.length() > 0 && val_sensor == 1)
      {
        result = gravaLote();
        //Nao tenho certeza se precisa! VERIFICAR COM O ANGELO
        /*aux_apt = apontamentos;
        aux_apt += "|";
        if (id_prxlote > 0)
        {
          if (aux_apt != lotes[id_prxlote - 1])
            result = gravaLote();
        }
        else
        {
          if (id_prxlote == 0)
          {
            if (aux_apt != lotes[limiteVetor])
              result = gravaLote();
          }
          else
          {
            if (id_prxlote == limiteVetor)
              if (aux_apt != lotes[0])
                result = gravaLote();
          }
        }*/
      }
    }
    /*if (result == 0)
    {
      Serial.println("Timer igual 0, fazer um getLog");
    }
    else
    {
      if (result == 1)
      {
        Serial.println("Apontamento nao realizado pois todos os lotes estavam cheios!");
        //Disparar Alerta
      }
    }*/
    if (val_sensor == 1)
    {
      espera = false;
    }
    apontamentos.clear();
    delay(100);
    tLoop = millis() - tLoop;
  }
  catch (...)
  {
    resetModule();
  }
}