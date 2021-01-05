/*Variaveis*/
int limiteVetor = 500;
String lotes[500] = {"", "", ""};
int id_prxlote = 0;
int priFila = 0;

/*WatchDog*/

//funcao que o temporizador ira chamar, para reiniciar o ESP32
void IRAM_ATTR resetModule()
{
  ESP.restart();
}

char *itoa(int value, char *str)
{
  try
  {
    char temp;
    memset(&str, 0, sizeof(str));
    int i = 0;
    while (value > 0)
    {
      int digito = value % 10;
      str[i] = digito + '0';
      value /= 10;
      i++;
    }
    i = 0;
    int j = strlen(str) - 1;
    while (i < j)
    {
      temp = str[i];
      str[i] = str[j];
      str[j] = temp;
      i++;
      j--;
    }
    return str;
  }
  catch (...)
  {
    resetModule();
  }
}

String CtoS(const char *c)
{
  try
  {
    String s = "";
    int i;
    for (i = 0; i < 6; i++)
    {
      s += c[i];
    }
    return s;
  }
  catch (...)
  {
    resetModule();
  }
}

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