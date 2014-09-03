#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

#include "SIM900.h"
#include "mUART.h"

#define MAX_WAIT_COUNT 5
#define WAIT_DELAY 300
#define DELAY_BEFORE_RUN_COMMAND 300

////////////////////////////
char CPAS_0_STR[] PROGMEM = "+CPAS: 0";
char CMGS_STR[] PROGMEM = "+CMGS:";
char CPBS_STR[] PROGMEM = "+CPBS:";
char CSQ_STR[] PROGMEM = "+CSQ:";
////////////////////////////

void clear_sms_memory()
{
  _delay_ms(300);
  mUART_puts(AT_SMS_DELETE_ALL);
  mUART_puts(END_COMMAND);
}

// включение/отключение модулюя
void power_on_off()
{
  char buf[30] = "\0";
  unsigned char i;
  
  // включаем модуль
  POWER_PORT |= (1 << POWER_PIN);
  _delay_ms(1100);
  POWER_PORT ^= (1 << POWER_PIN);
  
  // настройка модуля
  // ждем сообщения о готовности
  i = 0;
  do
  {
    mUART_clear_buffer();
    mUART_puts(AT_SIM_STATE);
    mUART_puts(END_COMMAND);
    if (mUART_wait_and_return_string(15, buf))
    {
      if (strcasestr_P(buf, CPAS_0_STR) == 0)
      {
        if (isHAS_RX_DATA())
        {
          mUART_gets(buf);
          if (strcasestr_P(buf, CPAS_0_STR) != 0)
            break;
        }
      }
      else
        break;
    }
  }
  while (1); // ждем пока возвращенная строка не будет начинаться на "+CPAS: 0"
  // модуль готов
  // делаем соответсвующие настройки модуля
  init_sim900();
}

// настройка SIM900
void init_sim900()
{
  _delay_ms(1000);
  // отключаем эхо
  mUART_clear_buffer();
  mUART_puts(AT_MODE_EHO);
  mUART_puts(END_COMMAND);
  while (!isHAS_RX_DATA())
  {
    _delay_ms(10);
  }

  // ответ только кодом
  mUART_clear_buffer();
  mUART_puts(AT_MODE_REPORT);
  mUART_puts(END_COMMAND);
  while (!isHAS_RX_DATA())
  {
    _delay_ms(10);
  }

  // при ошибке возращает ее номер
  mUART_clear_buffer();
  mUART_puts(AT_MODE_ERROR_RETURN);
  mUART_puts(END_COMMAND);
  while (!isHAS_RX_DATA())
  {
    _delay_ms(10);
  }

  // очищаем память смс
  clear_sms_memory();
}

// отправка сообщения
unsigned char send_sms(char *msg, char *tel)
{
  char i = 0;

  _delay_ms(DELAY_BEFORE_RUN_COMMAND); // задержка, т.к. SIM900 может быть еще не готов к отправке, после выполнения предыдущих команд
  mUART_clear_buffer();

  mUART_puts(AT_SMS_SEND);
  mUART_puts("\"");
  mUART_puts(tel);
  mUART_puts("\"");
  mUART_puts(END_COMMAND);

  i = 0;
  // после получения команды на отправку смс модуль должен выдать
  // приглашение на ввод смс(вернуть ">") без символа конца строки,
  // поэтому проверяем на кол-во принятых символов
  while((mUART_lengthData() == 0) && (i < MAX_WAIT_COUNT))
  {
    i++;
    _delay_ms(WAIT_DELAY);
  }

  if (i == MAX_WAIT_COUNT)
    mUART_putc(ESCAPE);
  else
  {
    char buf[15];
    mUART_gets(buf);
    if (buf[0] == '>')
    {
      mUART_puts(msg);
      mUART_clear_buffer();
      mUART_putc(END_MESSAGE);
      // ждем ответ
      char res[60];
      if (mUART_wait_and_return_string(100, res))
      {
        return (strcasestr_P(res, CMGS_STR) != 0) ? COMMAND_COMPLETED : COMMAND_FAILED;
      }
    }
    else
      mUART_putc(ESCAPE);
  }
  return COMMAND_FAILED;
}

// добавление телефона в телефонную книгу
int add_telephone(char* tel)
{
  unsigned char count_number;
  char buf[25];
  char str_tek_count[3];

  _delay_ms(DELAY_BEFORE_RUN_COMMAND); // задержка, т.к. SIM900 может быть еще не готов к отправке, после выполнения предыдущих команд
  mUART_clear_buffer();

  mUART_puts(AT_PHONE_MEMORY_STATE);
  mUART_puts(END_COMMAND);

  // после получения команды на отправку модуль должен выдать
  // +CPBS: «SM»,3,250
  // где SM - тип памяти где храняться номера. в данном случае сим-карта
  //     3 - количество записанных номеров
  //     250 - максимальное количество записей
  mUART_clear_buffer();
  if (mUART_wait_and_return_string(10, buf))
  {
    if (strcasestr_P(buf, CPBS_STR))
    {
      unsigned char i = 0;
      char* position;

      position = strcasestr(buf, ","); // находим первое вхождение запятой
      position++; // переходим к первому символу после запятой
      while ((position[i] != ',') && (i <=3))
      {
        str_tek_count[i] = position[i];
        i++;
      }
      if (i > 0)
      {
        count_number = atoi(str_tek_count);
        count_number++;
      
        // посылаем команду на добавление номера
        mUART_puts(AT_PHONENUMBER_ADD);
        mUART_puti((int)count_number);
        mUART_puts(",\"");
        mUART_puts(tel);
        mUART_puts("\",145,\"");
        mUART_puts(tel);
        mUART_puts("\"");
        mUART_puts(END_COMMAND);

        // ждем ответа
        mUART_clear_buffer();
        if (mUART_wait_and_return_string(10, buf))
        {
          return (buf[0] == RESULT_COMMAND_OK ? COMMAND_COMPLETED : COMMAND_FAILED);
        }
      }
    }
  }
  
  return COMMAND_FAILED;
}

void call_on_number(char *number)
{
  mUART_puts(AT_CALL);
  mUART_puts(number);
  mUART_puts(";");
  mUART_puts(END_COMMAND);
}

void break_call()
{
  mUART_puts(AT_BREAK_CALL);
  mUART_puts(END_COMMAND);
}

void answer_to_call()
{
  mUART_puts(AT_ANSWER_TO_CALL);
  mUART_puts(END_COMMAND);
}

// возвращает качество сигнала сети
void get_power_signal(char* res)
{
  /*char bufer[15];
  
  res[0] = '\0';
  mUART_clear_buffer();

  // запрос качества сигнала
  mUART_puts(AT_SIGNAL_LEVEL);
  mUART_puts(END_COMMAND);

  mUART_wait_and_return_string(10, bufer);
  if ((bufer[0] == '\0') || (strcasestr_P(bufer, CSQ_STR) == 0))
    return;
  
  // строка с результатом имеет вид: +CSQ: ??,?
  res[0] = bufer[6];
  if (bufer[7] == ',')
    res[1] = '\0';
  else
  {
    res[1] = bufer[7];
    res[2] = '\0';
  }*/
  return;
}

void read_sms(char* sms,  // текст смс
              char* tel,  // телефон с которого пришло смс
              char* contact_name // имя контакта из телефонной книги
             )
{
  char buf_info[LENGTH_IN_SMS];
  char buf_sms[LENGTH_IN_SMS];
  char* position;
  unsigned char i;

  buf_info[0] = '\0';
  buf_sms[0] = '\0';
  mUART_clear_buffer();

  // запрашиваем смс
  mUART_puts(AT_SMS_READ);
  mUART_puts(END_COMMAND);

  mUART_wait_and_return_string(10, buf_info);
  mUART_wait_and_return_string(10, buf_sms);
  
  sms[0] = '\0';
  tel[0] = '\0';
  contact_name[0] = '\0';

  if (   (strlen(buf_info) < 20)  // если слишком короткая информация о сообщении
      || (strlen(buf_sms) == 0) // если пустой текст смс
      || (!strcasestr(buf_info, "\",\"+")) // если нет номера в информации о смс(сообщение от оператора), должна быть вхождение ","+
      || (strlen(buf_sms) > 5)) // если слишком длинный текст смс, т.к. у нас может быть только 3 символа
  {
    // очищаем память смс
    clear_sms_memory();
    return;
  }

  // смс
  strcat(sms, buf_sms);

  // номер телефона
  // переходим на начало номера
  position = strcasestr(buf_info, "\",\"");
  i = 0;
  while ((position[i + 3] != '\"') && (i < 15))
  {
    tel[i] = position[i + 3];
    i++;
  }
  tel[i] = '\0';

  // имя контакта
  position = position + 3 + i; // переходим к имени контакта
  i = 0;
  while ((position[i + 3] != '\"') && (i < 15))
  {
    contact_name[i] = position[i + 3];
    i++;
  }
  contact_name[i] = '\0';  

  // очищаем память смс
  clear_sms_memory();
}
