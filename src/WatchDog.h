/*Variaveis*/
hw_timer_t *timer = NULL;

void IRAM_ATTR resetModule()//funcao que o temporizador ira chamar, para reiniciar o ESP32
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

