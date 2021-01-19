#include "Apontamento.h"
extern "C" // MEDE TEMPERATURA INTERNA DO ESP32
{
  uint8_t temprature_sens_read();
}
#include "Conexao.h"
bool espera;

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
    pinMode(ent_sensor, INPUT);
    espera = false;

    Serial.begin(115200);
    Wire.begin(SDA, SCL);
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
    for (int i = 0; i < tam_slave && !espera; i++)
    {
      escravo(slave[i]);
      delay(200);
    }
    val_sensor = digitalRead(ent_sensor);
    if(val_sensor == 0 && apontamentos.length() > 0)
    {
      gravaLote();
      espera = true;
    }
    if(val_sensor == 1)
      espera = false;

    apontamentos.clear();
      
    delay(2);
    tLoop = millis() - tLoop;
  }
  catch (...)
  {
    resetModule();
  }
}