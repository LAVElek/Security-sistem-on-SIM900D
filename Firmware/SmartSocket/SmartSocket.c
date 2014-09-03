#include <avr/io.h>
#include <avr/interrupt.h>
#include <string.h>
#include <util/delay.h>
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#include "lib/mUART.h"
#include "lib/onewire.h"
#include "lib/ds18x20.h"
#include "lib/nrf24l01/nrf24l01.h"
#include "command.h"


#define STATE_RELAY_PIN PD6
#define STATE_RELAY_PORT PORTD

#define RALAY_PIN PC5
#define RALAY_PORT PORTC

#define UART_SPEED 19200
#define ADC_CAPACITY 25 // mV * 10^-1
#define ADC_KOEF 196 // *10^-2

#define ERROR_TEMPERATURE 2550
#define TO_CORRECT_TEMP -30 // ��������� ������� ��������� �� 3 �������

#define WORK_PIPE 0

const unsigned char EE_STATE_RELAY EEMEM = 0; // ����� � ������ ��� ��������� ����

const unsigned char ADDRESS[5] = {'S','O' ,'C', 'E','T'};

// ��������� �/�� �����������(�)
char message[NRF24L01_PAYLOAD];

//////////////////////////////////
char SOCKET_ON_STR[] PROGMEM = "Socket: on";
char SOCKET_OFF_STR[] PROGMEM = "Socket: off";
char TEMPERATURE_STR[] PROGMEM = "Temp.: ";
char TEMPERATURE_UNITS_STR[] PROGMEM = "C";
char POWER_STR[] PROGMEM = "Power: ";
char POWER_UNITS_STR[] PROGMEM = "mV";
char SHORT_TEMPERATURE_STR[] PROGMEM = "T: ";
char SHORT_POWER_STR[] PROGMEM = "P: ";
char SHORT_SOCKET_ON_STR[] PROGMEM = "S: on";
char SHORT_SOCKET_OFF_STR[] PROGMEM = "S: off";
char UNKNOWN_COMMAND_STR[] PROGMEM = "Unknown command";
//////////////////////////////////

// �������� ����
void relay_on()
{
  STATE_RELAY_PORT |= (1 << STATE_RELAY_PIN);
  RALAY_PORT |= (1 << RALAY_PIN);
  eeprom_write_byte(&EE_STATE_RELAY, 1);
}

// ��������� ����
void relay_off()
{
  STATE_RELAY_PORT &= ~(1 << STATE_RELAY_PIN);
  RALAY_PORT &= ~(1 << RALAY_PIN);
  eeprom_write_byte(&EE_STATE_RELAY, 0);
}

// ��������� ���
void init_ADC()
{
  ADMUX =   (1 << REFS1) | (1 << REFS0) //���������� ���������� ������� ����������
          | (0 << ADLAR) // 2 ������� ���� � ADCH
          | (0 << MUX3) | (1 << MUX2) | (0 << MUX1) | (0 << MUX0); // ����� ADC4
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

  return  (long int)ADC_KOEF * ADC_CAPACITY * ADCW / 1000; // * 10^-2 * 10^-1
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
}

// �������� ��������� �������� ������
void send_message(char* data)
{
  const unsigned char delay = 100;
  unsigned char delay_count;
  
  delay_count = 0;
  while ((nrf24l01_write(data) != 1) && (delay_count < 20)) 
  {
    _delay_ms(delay);
    delay_count++;
  };
}

// ���������� ���������� �������
void processing_command(unsigned char command)
{
  char str[11];
  int temp;
  unsigned int adc;
  unsigned char delay_count;
  
  // ����� ����������� ������� �������� ����� ���������� ����� �������������
  message[0] = '\0';
  switch (command)
  {
    case SOCKET_ON:
      relay_on();
      strcat_P(message, SOCKET_ON_STR);
      break;
    case SOCKET_OFF:
      relay_off();
      strcat_P(message, SOCKET_OFF_STR);
      break;
    case GET_TEMP:
      strcat_P(message, TEMPERATURE_STR);
      temp = getTemperature();
      DS18X20_format_from_decicelsius(temp, str, 10);
      strcat(message, str);
      strcat_P(message, TEMPERATURE_UNITS_STR);
      break;
    case GET_SOCKET_STATUS:
      if ((STATE_RELAY_PORT & (1 << STATE_RELAY_PIN)) > 0)
        strcat_P(message, SOCKET_ON_STR);
      else
        strcat_P(message, SOCKET_OFF_STR);
      break;
    case GET_POWER:
      strcat_P(message, POWER_STR);
      adc = get_ADC();
      utoa(adc, str, 10);
      strcat(message, str);
      strcat_P(message, POWER_UNITS_STR);
      break;
    case GET_FULL_INFO:
      // �����������
      strcat_P(message, SHORT_TEMPERATURE_STR);
      temp = getTemperature();
      DS18X20_format_from_decicelsius(temp, str, 10);
      strcat(message, str);
      strcat_P(message, TEMPERATURE_UNITS_STR);
      strcat(message, "\n");
      // ����������
      strcat_P(message, SHORT_POWER_STR);
      adc = get_ADC();
      utoa(adc, str, 10);
      strcat(message, str);
      strcat_P(message, POWER_UNITS_STR);
      strcat(message, "\n");
      // ��������� �������
      if ((STATE_RELAY_PORT & (1 << STATE_RELAY_PIN)) > 0)
        strcat_P(message, SHORT_SOCKET_ON_STR);
      else
        strcat_P(message, SHORT_SOCKET_OFF_STR);
      break;
    default:
      strcat_P(message, UNKNOWN_COMMAND_STR); 
  }
  
  // �������� ��������� � ������� 5 ������
  delay_count = 0;
  while ((nrf24l01_write(message) != 1) && (delay_count < 50)) 
  {
    _delay_ms(100);
    delay_count++;
  };
  
  return;
}

// ��������� ������ �� �����������
void recive_data_from_nrf()
{
  if (nrf24l01_readready(WORK_PIPE))
  {
    //mUART_puts("���. ������: ");
    nrf24l01_read(message);
    //mUART_puts(message);
    //mUART_puts("\n");
    processing_command((unsigned char)atoi(message));
  }
  return;
}

// ���������� ���������� ��������� ����
void return_state_socket()
{
  unsigned char prev_state;

  prev_state = eeprom_read_byte(&EE_STATE_RELAY);
  if (prev_state == 0)
    relay_off();
  else
    relay_on();
}

int main()
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
  //        |||- ���������/���������� ���� 
  //        ||||- ��� - ������ ���������� �������
  //        |||||- 
  //        ||||||- 
  //        |||||||- 
  //        ||||||||- 
  PORTC = 0b00000000;
  DDRC =  0b00100000;
  
  //        76543210
  //        |-
  //        ||- ��������� �������(1 - ��������, 0 - ���������)
  //        |||-
  //        ||||-
  //        |||||-
  //        ||||||- ������ �����������  
  //        |||||||- TX
  //        ||||||||- RX
  PORTD = 0b00000000;
  DDRD =  0b01000000;
  
  // ����������� ���
  init_ADC();

  //�������������� UART
  mUART_init(mUART_UBRR(UART_SPEED,F_CPU));

  // ����������� 1-Wire ����
  ow_set_bus(&PIND,&PORTD,&DDRD,PD3);

  // ����������� ������ �����������
  setupDS18B20();

  // ��������� �����������
  nrf24l01_init();

  // ��������� ����������
  sei();

  // ������������� ����� ������/��������
  nrf24l01_setrxaddr(WORK_PIPE, ADDRESS);
  nrf24l01_settxaddr(ADDRESS);

  // ��������� ���������� ��������� ����
  return_state_socket();

  while(1)
  {
    recive_data_from_nrf();
  }

  return 0;
}
