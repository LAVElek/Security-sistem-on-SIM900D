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

// ���������/���������� �������
void power_on_off()
{
  char buf[30] = "\0";
  unsigned char i;
  
  // �������� ������
  POWER_PORT |= (1 << POWER_PIN);
  _delay_ms(1100);
  POWER_PORT ^= (1 << POWER_PIN);
  
  // ��������� ������
  // ���� ��������� � ����������
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
  while (1); // ���� ���� ������������ ������ �� ����� ���������� �� "+CPAS: 0"
  // ������ �����
  // ������ �������������� ��������� ������
  init_sim900();
}

// ��������� SIM900
void init_sim900()
{
  _delay_ms(1000);
  // ��������� ���
  mUART_clear_buffer();
  mUART_puts(AT_MODE_EHO);
  mUART_puts(END_COMMAND);
  while (!isHAS_RX_DATA())
  {
    _delay_ms(10);
  }

  // ����� ������ �����
  mUART_clear_buffer();
  mUART_puts(AT_MODE_REPORT);
  mUART_puts(END_COMMAND);
  while (!isHAS_RX_DATA())
  {
    _delay_ms(10);
  }

  // ��� ������ ��������� �� �����
  mUART_clear_buffer();
  mUART_puts(AT_MODE_ERROR_RETURN);
  mUART_puts(END_COMMAND);
  while (!isHAS_RX_DATA())
  {
    _delay_ms(10);
  }

  // ������� ������ ���
  clear_sms_memory();
}

// �������� ���������
unsigned char send_sms(char *msg, char *tel)
{
  char i = 0;

  _delay_ms(DELAY_BEFORE_RUN_COMMAND); // ��������, �.�. SIM900 ����� ���� ��� �� ����� � ��������, ����� ���������� ���������� ������
  mUART_clear_buffer();

  mUART_puts(AT_SMS_SEND);
  mUART_puts("\"");
  mUART_puts(tel);
  mUART_puts("\"");
  mUART_puts(END_COMMAND);

  i = 0;
  // ����� ��������� ������� �� �������� ��� ������ ������ ������
  // ����������� �� ���� ���(������� ">") ��� ������� ����� ������,
  // ������� ��������� �� ���-�� �������� ��������
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
      // ���� �����
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

// ���������� �������� � ���������� �����
int add_telephone(char* tel)
{
  unsigned char count_number;
  char buf[25];
  char str_tek_count[3];

  _delay_ms(DELAY_BEFORE_RUN_COMMAND); // ��������, �.�. SIM900 ����� ���� ��� �� ����� � ��������, ����� ���������� ���������� ������
  mUART_clear_buffer();

  mUART_puts(AT_PHONE_MEMORY_STATE);
  mUART_puts(END_COMMAND);

  // ����� ��������� ������� �� �������� ������ ������ ������
  // +CPBS: �SM�,3,250
  // ��� SM - ��� ������ ��� ��������� ������. � ������ ������ ���-�����
  //     3 - ���������� ���������� �������
  //     250 - ������������ ���������� �������
  mUART_clear_buffer();
  if (mUART_wait_and_return_string(10, buf))
  {
    if (strcasestr_P(buf, CPBS_STR))
    {
      unsigned char i = 0;
      char* position;

      position = strcasestr(buf, ","); // ������� ������ ��������� �������
      position++; // ��������� � ������� ������� ����� �������
      while ((position[i] != ',') && (i <=3))
      {
        str_tek_count[i] = position[i];
        i++;
      }
      if (i > 0)
      {
        count_number = atoi(str_tek_count);
        count_number++;
      
        // �������� ������� �� ���������� ������
        mUART_puts(AT_PHONENUMBER_ADD);
        mUART_puti((int)count_number);
        mUART_puts(",\"");
        mUART_puts(tel);
        mUART_puts("\",145,\"");
        mUART_puts(tel);
        mUART_puts("\"");
        mUART_puts(END_COMMAND);

        // ���� ������
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

// ���������� �������� ������� ����
void get_power_signal(char* res)
{
  /*char bufer[15];
  
  res[0] = '\0';
  mUART_clear_buffer();

  // ������ �������� �������
  mUART_puts(AT_SIGNAL_LEVEL);
  mUART_puts(END_COMMAND);

  mUART_wait_and_return_string(10, bufer);
  if ((bufer[0] == '\0') || (strcasestr_P(bufer, CSQ_STR) == 0))
    return;
  
  // ������ � ����������� ����� ���: +CSQ: ??,?
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

void read_sms(char* sms,  // ����� ���
              char* tel,  // ������� � �������� ������ ���
              char* contact_name // ��� �������� �� ���������� �����
             )
{
  char buf_info[LENGTH_IN_SMS];
  char buf_sms[LENGTH_IN_SMS];
  char* position;
  unsigned char i;

  buf_info[0] = '\0';
  buf_sms[0] = '\0';
  mUART_clear_buffer();

  // ����������� ���
  mUART_puts(AT_SMS_READ);
  mUART_puts(END_COMMAND);

  mUART_wait_and_return_string(10, buf_info);
  mUART_wait_and_return_string(10, buf_sms);
  
  sms[0] = '\0';
  tel[0] = '\0';
  contact_name[0] = '\0';

  if (   (strlen(buf_info) < 20)  // ���� ������� �������� ���������� � ���������
      || (strlen(buf_sms) == 0) // ���� ������ ����� ���
      || (!strcasestr(buf_info, "\",\"+")) // ���� ��� ������ � ���������� � ���(��������� �� ���������), ������ ���� ��������� ","+
      || (strlen(buf_sms) > 5)) // ���� ������� ������� ����� ���, �.�. � ��� ����� ���� ������ 3 �������
  {
    // ������� ������ ���
    clear_sms_memory();
    return;
  }

  // ���
  strcat(sms, buf_sms);

  // ����� ��������
  // ��������� �� ������ ������
  position = strcasestr(buf_info, "\",\"");
  i = 0;
  while ((position[i + 3] != '\"') && (i < 15))
  {
    tel[i] = position[i + 3];
    i++;
  }
  tel[i] = '\0';

  // ��� ��������
  position = position + 3 + i; // ��������� � ����� ��������
  i = 0;
  while ((position[i + 3] != '\"') && (i < 15))
  {
    contact_name[i] = position[i + 3];
    i++;
  }
  contact_name[i] = '\0';  

  // ������� ������ ���
  clear_sms_memory();
}
