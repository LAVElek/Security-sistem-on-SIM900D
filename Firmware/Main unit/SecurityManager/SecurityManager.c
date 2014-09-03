//#define F_CPU 8000000

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>
#include <avr/pgmspace.h>

#include "onewire.h"
#include "ds18x20.h"
#include "mUART.h"
#include "SIM900.h"
#include "nrf24l01/nrf24l01.h"
#include "SecurityManagerCommand.h"


#define UART_SPEED 19200

#define ADC_CAPACITY 25 //mV * 10^-1, 
#define ADC_KOEF 196 // *10^-2

#define WORK_PIPE 0

#define ERROR_TEMPERATURE 255
#define TO_CORRECT_TEMP -15 // ��������� ������� ��������� �� 1.5 �������

// ��������� �/�� �����������(�)
char message[NRF24L01_PAYLOAD];

const unsigned char ADDRESS[5] = {'S','O' ,'C', 'E','T'};

//////////////////////////
char TEMPERATURE_STR[] PROGMEM = "Temp.: ";
char TEMPERATURE_UNITS_STR[] PROGMEM = "C";
char POWER_STR[] PROGMEM = "Power: ";
char POWER_UNITS_STR[] PROGMEM = "mV";
char NO_INFO_STR[] PROGMEM = "Information is not received";
char ACCESS_DENIED_STR[] PROGMEM = "Access denied";
char ERROR_COMMAND_STR[] PROGMEM = "Error command";
char OK_STR[] PROGMEM = "OK";
char FAIL_STR[] PROGMEM = "FAIL";
char UNKNOWN_COMMAND_STR[] PROGMEM = "Unknown command";
char CMTI_STR[] PROGMEM = "+CMTI: \"SM\"";
//////////////////////////

// ��������� ���
// �� ����� ������ 1.0 �������� �� ���������. ����� �������� ��������� ����� Vref
// ������ ����� ����������� ������������ � �����
// �� ����� ������ 1.0 ������� ����������� �������� VCC, �.�. Vref ���������� Vcc
void init_ADC()
{
  ADMUX =   (1 << REFS1) | (1 << REFS0) //���������� ���������� ������� ����������
          | (0 << ADLAR) // 2 ������� ���� � ADCH
          | (0 << MUX3) | (1 << MUX2) | (1 << MUX1) | (1 << MUX0); // ����� ADC7
  ADCSRA = (1 << ADEN) // ��� ��������
           | (0 << ADFR) // ����� ������������ ��������������
           | (0 << ADIE) // ���������� �� ��� ���������
           | (1 << ADSC) // ������ ����� ��������������, �.�. ��� ������ ������� ����� 0
           | (1 << ADPS2) | (1 << ADPS1) | (0 << ADPS0); // ������� ��/64 => 8000000/64 = 125���
}

// ���������� ������� �������� ��� � ��
unsigned int get_ADC()
{
  ADCSRA |= (1 << ADSC); // ��������� ��������������

  while ((ADCSRA & (1 << ADIF)) == 0); // ���� ���� ��� �� �������� ��������������

  return (long int)ADC_KOEF * ADC_CAPACITY * ADCW / 1000; // * 10^-2 * 10^-1
}

// ������ �����������
// �.�. � ��� ������ 1 ������ �� �������� � �������� ������ null
int getTemperature()
{
  int temperature;

  temperature = ERROR_TEMPERATURE;

  DS18X20_start_meas(DS18X20_POWER_EXTERN, 0);
  _delay_ms(DS18B20_TCONV_12BIT);
  DS18X20_read_decicelsius_single(DS18B20_FAMILY_CODE, &temperature);

  return temperature + TO_CORRECT_TEMP;
}

// ��������� ������� �����������
// ����������� ������ �������� � ��������� 12 ���
// ��������� ��������� �� ������
void setupDS18B20()
{
  unsigned char sp[DS18X20_SP_SIZE];

  // ��������� ������� ���������
  DS18X20_read_scratchpad(0, sp, DS18X20_SP_SIZE );
  // ���� ����� �� 12 ���, �� ���������� ���
  if ((sp[DS18B20_CONF_REG] & DS18B20_RES_MASK) != DS18B20_12_BIT)
  {
    unsigned char th, tl;

    th = sp[DS18X20_TH_REG];
	  tl = sp[DS18X20_TL_REG];
    
    DS18X20_write_scratchpad(0, th, tl, DS18B20_12_BIT);
  } 
  return;
}

// ��������� ����������������� �����������
void getFormattedTemperature(char* ftemp)
{
  char str_value[10];
  int value;

  value = getTemperature();
  DS18X20_format_from_decicelsius(value, str_value, 10);
  ftemp[0] = '\0';
  strcat_P(ftemp, TEMPERATURE_STR);
  strcat(ftemp, str_value);
  strcat_P(ftemp, TEMPERATURE_UNITS_STR);
  return;
}

// ���������� ����������������� �������� ����������
void getFormattedADC(char* fadc)
{
  char str_adc[10];
  unsigned int adc;

  adc = get_ADC();
  utoa(adc, str_adc, 10);
  fadc[0] = '\0';
  strcat_P(fadc, POWER_STR);
  strcat(fadc, str_adc);
  strcat_P(fadc, POWER_UNITS_STR);
  return;
}

// ���������� ������ ���� � ����������������� ����
void getFormattedFullInfo(char* ffull)
{
  char buf[20];

  ffull[0] = '\0';

  getFormattedTemperature(buf);
  strcat(ffull, buf);
  strcat(ffull, "\n");
  getFormattedADC(buf);
  strcat(ffull, buf);
  return;
}

// �������� ������ �� �����������
void send_data_on_radio(char* result)
{
  const unsigned char max_delay = 100;
  const unsigned char delay = 100;
  unsigned char delay_count;
  
  delay_count = 0;
  // �������� ��������� � ������� 10 ������
  while ((nrf24l01_write(message) != 1) && (delay_count < max_delay)) 
  {
    _delay_ms(delay);
    delay_count++;
  };
  // ���� ������. 10 ������
  for(delay_count = 0; delay_count < max_delay; delay_count++)
  {
    if (nrf24l01_readready(WORK_PIPE))
    {
      nrf24l01_read(message);
      break;
    }
    _delay_ms(delay);
  }

  if (delay_count == max_delay)
  {
    message[0] = '\0';
    strcat_P(message, NO_INFO_STR);
  }

  result[0] = '\0';
  strcat(result, message);
  
  return;
}

void complete_command()
{
  char sms[LENGTH_IN_SMS];
  char contact_name[20];
  char telefone[20];
  char tekst_sms[LENGTH_OUT_SMS];

  // ����������� ������ �� ���
  read_sms(sms, telefone, contact_name);

  // ������ ��� - ������ ������
  if (sms[0] == '\0')
    return;

  tekst_sms[0] = '\0';
  // ��������� ������� ����� ������ ������������������ ������������(������� ��������� � �������� �����)
  // ���� ������������ �� ��������������� ���� �� ������ ������� �� ��� ���������� � ������ �� �������� �� ������
  if ((contact_name[0] == '\0') && (sms[0] != SM_ADD_USER))
  {
    strcat_P(tekst_sms, ACCESS_DENIED_STR);
  }
  else
  {
    // ��������� ��������������� �������
    switch (*sms)
    {
      case SM_SEND_COMMAND_TO_CHILD:
        if (strlen(sms) >= 3)
        {
          //char send_data[5];
          unsigned char i = 0;
          do
          {
            message[i] = sms[i + 2];
            i++;
          }
          while ((i < 5) && (sms[i + 2] > 32));
          message[i] = '\0';
          send_data_on_radio(tekst_sms);
        }
        else
          strcat_P(tekst_sms, ERROR_COMMAND_STR);
        break;
      case SM_GET_TEMP:
        getFormattedTemperature(tekst_sms);
        break;
      case SM_GET_POWER:
        getFormattedADC(tekst_sms);
        break;
      case SM_GET_FULL_INFO:
        getFormattedFullInfo(tekst_sms);
        break;
      case SM_ADD_USER:
        if (add_telephone(telefone) == COMMAND_COMPLETED)
          strcat_P(tekst_sms, OK_STR);
        else
          strcat_P(tekst_sms, FAIL_STR);
        break;
      /*case SM_GET_POWER_SIGNAL:
        get_power_signal(tekst_sms);
        break;*/
      default:
        strcat_P(tekst_sms, UNKNOWN_COMMAND_STR);
        break;
    }
  }
  if (telefone[0] != '\0')
    send_sms(tekst_sms, telefone);
  return;
}

int main(void)
{
  // ��������� ��������� ������
  //        76543210
  //        |-
  //        ||-
  //        |||- 
  //        ||||- 
  //        |||||- 
  //        ||||||- 
  //        |||||||- 
  //        ||||||||- 
  PORTB = 0b00000000;
  DDRB =  0b00000000;

  //        76543210
  //        |-
  //        ||-
  //        |||- 
  //        ||||- 
  //        |||||- 
  //        ||||||- 
  //        |||||||- ���������-��������� 
  //        ||||||||- ������ �����������
  PORTC = 0b00000000;
  DDRC =  0b00000011;
  
  //        76543210
  //        |-
  //        ||-
  //        |||- 
  //        ||||- PWD - ���������/���������� SIM900
  //        |||||- INT1 - ������� ���������� �� SIM900(������ ��� ���)
  //        ||||||- 
  //        |||||||- TX
  //        ||||||||- RX
  PORTD = 0b00000000;
  DDRD =  0b00010000;

  //�������������� UART
  mUART_init(mUART_UBRR(UART_SPEED,F_CPU));
  
  // ����������� 1-Wire ����
  ow_set_bus(&PINC,&PORTC,&DDRC,PC0);

  // ����������� ������ �����������
  setupDS18B20();
  
  // ����������� ���
  init_ADC();

  // ��������� �����������
  nrf24l01_init();

  // ��������� ����������
  sei();

  // ������������� ����� ������/��������
  nrf24l01_setrxaddr(WORK_PIPE, ADDRESS);
  nrf24l01_settxaddr(ADDRESS);
  
  // �������� � ����������� SIM900
  power_on_off();
  PORTC ^= (1 << 1);

  char sim_message[LENGTH_IN_SMS];

  while(1)
  {
    if (isHAS_RX_DATA())
    {
      mUART_gets(sim_message); 
           
      if (sim_message[0] != '\0')
      {
        if (sim_message[0] == '2') // ���� ������
        {
          break_call();
        }
        else if (strcasestr_P(sim_message, CMTI_STR) != 0)
        {
          complete_command();
        }   
      } 
    }
  }
  return 0;
}
