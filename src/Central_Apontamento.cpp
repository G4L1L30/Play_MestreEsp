#include "Apontamento.h"
#include "Conexao.h"
// MEDE TEMPERATURA INTERNA DO ESP32
extern "C"
{
  uint8_t temprature_sens_read();
}

void setupWatchDog()
{
  try
  {

    timer = timerBegin(0, 80, true);
    timerAttachInterrupt(timer, &resetModule, true);
    //timer,tempo(us),repeticao
    timerAlarmWrite(timer, 5000000, true);
    timerAlarmEnable(timer); //habilita a interrupcao
  }
  catch (...)
  {
    resetModule();
  }
}

void loopWatchDog()
{
  try
  {
    timerWrite(timer, 0); //reseta o temporizador (alimenta o watchdog)
  }
  catch (...)
  {
    resetModule();
  }
}

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
    pinMode(sd_sensor, OUTPUT);

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
    loopWatchDog();
    for(int i = 0; i < tam_slave; i++)
    {
      ativo = digitalRead(ent_sensor);
      digitalWrite(sd_sensor, ativo);
      escravo(slave[i]);
      delay(200);
    }
    delay(2);
  }
  catch (...)
  {
    resetModule();
  }
}