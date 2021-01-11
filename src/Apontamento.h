#include <Wire.h>
#include <Arduino.h>
#define SDA 13
#define SCL 16
#define rst_s0 17
#define rst_s1 19
#define ent_sensor 39
#define sd_sensor 5

int slave[2] = {0x32, 0x42};
bool estado[2] = {false, false};
int pin_reset[2] = {rst_s0, rst_s1};
int log_reset[2] = {0, 0};

char dado[50];
String apontamentos;
int conta = 0, ativo;

int tam_slave = sizeof(slave) / sizeof(int);
int tam_pin = sizeof(pin_reset) / sizeof(int);
int tam_log = sizeof(log_reset) / sizeof(int);

void inicia_PinMode()
{
  for (int i = 0; i < tam_pin; i++)
  {
    pinMode(pin_reset[i], OUTPUT);
    digitalWrite(pin_reset[i], HIGH);
  }
}

int procura_PosSlave(int quem)
{
  int i = 0;
  while (i < tam_slave && slave[i] != quem)
    i++;
  return i;
}

void exibe_logReset()
{
  Serial.print("[ ");
  for (int i = 0; i < tam_log; i++)
  {
    Serial.print(log_reset[i]);
    Serial.print(" |");
  }
  Serial.println("]");
  //delay(3000);
}

void reset(int qual)
{
  int pos = procura_PosSlave(qual);
  if (slave[pos] == qual && estado[pos] == false)
  {
    estado[pos] = true;
    digitalWrite(pin_reset[pos], LOW);
    delay(1000);
    digitalWrite(pin_reset[pos], HIGH);
    log_reset[pos] += 1;
    exibe_logReset();
  }
}

void normaliza(int qual)
{
  int pos = procura_PosSlave(qual);
  if (slave[pos] == qual && estado[pos] == true) //duvidas aqui
    estado[pos] = false;
}

/*void envia(String info, String status)
{
  apontamentos[apontamentos->length()] = info;
  status_apt[status_apt->length()] = status;
  /*STATUS
  true = apontada e confirmada
  aguarda = apontada e nao confirmada
  erro = nao apontada
  
}
*/

//FUNCAO PARA LER E ENVIAR DADOS AO(S) ESCRAVO(S)
void escravo(int slave)
{
  int tam_resp = 0; //pega o tamanho da resposta vindo do maix bit
  int leitura, i;
  char letra;
  char check[10], informacao[50];
  int cont_info = 0, val_check, check_info = 0;

  //VAI PERGUNTAR SE TEM DADO
  Wire.beginTransmission(slave);   //abre a transmissao
  Wire.write(0);                   // '0' pergunta se tem dado
  if (Wire.endTransmission() == 0) //fecha a transmissao e confirma que o maix bit esta vivo
  {
    normaliza(slave);
    Serial.print(conta++);
    Serial.print(" 0x");
    Serial.print(slave, HEX);
    Serial.println(": Pergunta recebida!");

    Wire.requestFrom(slave, 1); //PEGANDO RESPOSTA
    if (Wire.available())
    {
      tam_resp = Wire.read(); //"Tamanho da resposta"
    }

    if (tam_resp > 0) //Maix Bit tem Dados
    {
      //Requisitando dados
      Wire.beginTransmission(slave);   //abre a transmissao
      Wire.write(1);                   // '1' Solicita a informacao
      if (Wire.endTransmission() == 0) //fecha a transmissao e confirma que o maix bit esta vivo
      {
        Serial.print("0x");
        Serial.print(slave, HEX);
        Serial.println(": Enviando dados!");

        Wire.requestFrom(slave, tam_resp); //PEGANDO RESPOSTA
        while (Wire.available() && cont_info < tam_resp)
        {
          leitura = Wire.read();           //pega inteiro por inteiro do canal I2C
          letra = (char)leitura;           //converte o inteiro para char
          informacao[cont_info++] = letra; //guarda na informacao
        }
        if (cont_info == tam_resp) //chegou todas as informações
        {
          informacao[cont_info] = '\0'; //agora vira uma string que pode ser lida

          String split = strtok(informacao, ","); //primeiro split, para pegar a informacao
          String check_sum = strtok(NULL, ",");   //segundo split, para pegar o check sum da mensagem original

          //converte o split para o dado
          for (i = 0; i < split.length(); i++)
          {
            dado[i] = informacao[i];          // aqui o dado é global esse sera enviado a requisicao WebAPI
            check_info += (int)informacao[i]; //aqui faz o check sum da mensagem recebida, ou seja, soma os inteiro da mensagem que chegou
          }
          dado[i] = '\0'; //Aqui esta a informação pode ser lida

          //pega o check sum da mensagem original, ou seja, do segundo split e convert para uma string char para pode usar o atoi
          for (i = 0; i < check_sum.length(); i++)
            check[i] = check_sum[i];
          check[i] = '\0';
          val_check = atoi(check); //Aqui pega o check original que vem da mensagem, converte para inteiro

          if (val_check == check_info) //se o check da mensagem origial for igual ao check que fez decodificando a mensagem entao exibe, ou seja, o dado é confiavel
          {
            Serial.print("0x");
            Serial.print(slave, HEX);
            Serial.print(" - Dado: ");
            Serial.println(dado);
            /*if(info.length() <= 0)
              info = dado;*/
            dado[0] = '\0';
          }
        }
        else //nao chegou toda as informações
        {
          Serial.print("0x");
          Serial.print(slave, HEX);
          Serial.println(": Nao chegou todas as informações");
          reset(slave);
        }
      }
      else //Nao esta vivo
      {
        Serial.print("0x");
        Serial.print(slave, HEX);
        Serial.println(": Morreu na pergunta 1");
        reset(slave);
      }
    }
    else
    {
      Serial.print("0x");
      Serial.print(slave, HEX);
      Serial.println(": Sem dados");
    }
  }
  else //Nao esta vivo
  {
    Serial.print("0x");
    Serial.print(slave, HEX);
    Serial.println(": Morreu na pergunta 0");
    reset(slave);
  }
}
