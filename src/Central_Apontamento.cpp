#include "Apontamento.h"
extern "C" // MEDE TEMPERATURA INTERNA DO ESP32
{
  uint8_t temprature_sens_read();
}
#include "Conexao.h"
#include <pthread.h>
//pthread_t thread_esteira;
//int concluido;
bool espera;
int result;
bool sen_atv;
String aux_apt;
#define esteira 32

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
    pinMode(esteira, OUTPUT);
    Serial.begin(115200);
    pinMode(SDA, INPUT);
    pinMode(SCL, INPUT);
    Wire.begin(SDA, SCL, 400000);
    espera = false;
    sen_atv = false;
    //concluido = 0;
    //Thread do tipo xTask
    xTaskCreatePinnedToCore(setupcoreZero, "setupcoreZero", 8192, NULL, 0, NULL, 0);
    // configura WatchDog
    setupWatchDog();
  }
  catch (...)
  {
    resetModule();
  }
}

bool procura_iguais(String atual)
{
  String aux = lotes[id_prxlote], comparar;
  int pos = 0;
  for(int i = 0; i < aux.length(); i++)
  {
    comparar[pos++] = aux[i];
    if(aux[i] == '|')
    {
      if(comparar == atual)
        return true;
      else
      {
        pos = 0;
        comparar.clear();
      }
      
    }
  }
  return false;
}

/*
void *tempo_Leitura(void *)
{
  concluido = 1;
  int i = 0, tempo = 30;
  while(i < tempo && concluido == 1){
    delay(1000);
    i++;
    Serial.print("Tempo: ");
    Serial.print(tempo - i);
    Serial.println();
  }
  if(i == 30 || concluido == 3)
  {
    concluido = 2;
    Serial.println("Concluido");
    pthread_exit(NULL);
  }
}*/

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

    for(int i = 0; i < tam_slave; i++)
    {
      envia_Sensor(val_sensor, slave[i]);
      delay(50);
    }
    for(int i = 0; i < tam_slave && !espera; i++)
    {
      escravo(slave[i]);
      delay(100);
    }

    if(val_sensor == 0 && !sen_atv)
    {
      sensor_ativo(val_sensor);
      sen_atv = true;
      //if(concluido == 0) //Para nao criar varias threads
      //  pthread_create(&thread_esteira, NULL, tempo_Leitura, NULL);//cria uma thread para controlar o tempo de esteira parado
      digitalWrite(esteira, HIGH);
    }
    else{
      if(val_sensor == 1 && sen_atv)
      {
        sen_atv = false;
      }
    }

    /*if(concluido == 2) //Libera a esteira
    {
      digitalWrite(esteira, LOW);
      concluido = 0;
    }*/
    
    if (val_sensor == 0 && apontamentos.length() > 0) //Sensor Funcionando OK
    {
      result = gravaLote();
      digitalWrite(esteira, LOW);
      espera = true;
      //concluido = 3; //Libera a esteira antes dos 30 segundos 
    }
    else //Sensor com Problema
    {
      if (apontamentos.length() > 0 && val_sensor == 1)
      {
        //result = gravaLote();
        //Nao tenho certeza se precisa! VERIFICAR COM O ANGELO
        aux_apt = apontamentos;
        aux_apt += "|";
        if(lotes[id_prxlote] == ".")
        {
          result = gravaLote();
          //concluido = 3; //Libera a esteira antes dos 30 segundos 
        }
        else
        {
          if(!procura_iguais(aux_apt))
          {
            result = gravaLote();
            //concluido = 3; //Libera a esteira antes dos 30 segundos 
          }
        }
        /*if (id_prxlote > 0)
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
    //delay(100);
    tLoop = millis() - tLoop;
  }
  catch (...)
  {
    resetModule();
  }
}