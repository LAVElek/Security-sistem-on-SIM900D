#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include "mUART.h"

static char mUART_RxBuf[mUART_BUFFER_SIZE + 1]; // буфер для принятых данных
static volatile unsigned char mUART_RxBuf_size; // длина принятых данных
static volatile unsigned char HAS_RX_DATA; // флаг наличия принятый данных

void mUART_init(unsigned int baudrate)
{
  /*UBRRH = 0;
  UBRRL = 25; // скорость 19200*/
  UBRRH = (unsigned char)(baudrate>>8);
  UBRRL = (unsigned char) baudrate;
  UCSRC = ( 1 << URSEL ) // для доступа к регистру UCSRC выставляет бит URSEL.
          | ( 0 << UCSZ2 ) | ( 1 << UCSZ1 ) | ( 1 << UCSZ0 ) // количество бит данных равно 8
          | ( 0 << UPM1 ) | ( 0 << UPM0 ) // запрещаем контроль четности
          | ( 0 << USBS ); // 1 стоп бит
  UCSRB = (1 << RXCIE)    // разрешаем прерывание по приему
          | (1 << RXEN)   // разрешаем прием
          | (1 << TXEN);  // разрешаем передачу

  mUART_RxBuf_size = 0;
  HAS_RX_DATA = 0;
}

void mUART_putc(char data)
{
  while (!(UCSRA & (1 << UDRE)) ); //Ожидание опустошения буфера приема
  UDR = data; //Начало передачи данных	
}

void mUART_puts(char *string)
{
  /*
  конец строки сигнаизирует \0
  все что не \0 то true
  */
  while(*string)
  {
    mUART_putc(*string);
    string++;
  }
}

// выводит в mUART int
void mUART_puti(const int value)
{
  char buf[10];

  mUART_puts(itoa(value, buf, 10));
}

// выводит в UART unsigned int
void mUART_putu(const unsigned int value)
{
  char buf[10];

  mUART_puts(utoa(value, buf, 10));
}

// выводит в mUART long int
void mUART_putl(const long int value)
{
  char buf[15];

  mUART_puts(ltoa(value, buf, 10));
}

unsigned char isHAS_RX_DATA()
{
  return HAS_RX_DATA;
}

// возвращает текущую длину сообщения
unsigned char mUART_lengthData()
{
  return mUART_RxBuf_size;
}

char * mUART_gets()
{
  if ((HAS_RX_DATA == 0) || (mUART_RxBuf_size ==0))
    return '\0';
  else
  {    
    // обнуляем информацию о принятых данных
    mUART_RxBuf_size = 0;
    HAS_RX_DATA = 0;

    return mUART_RxBuf;
  }
}

unsigned char mUART_getc()
{
  if ((HAS_RX_DATA == 0) || (mUART_RxBuf_size ==0))
    return '\0';
  else
  {
    // обнуляем информацию о принятых данных
    mUART_RxBuf_size = 0;
    HAS_RX_DATA = 0;

    return *mUART_RxBuf;
  }
}

// обработка прерывания по приему байта
ISR(USART_RXC_vect)
{
  signed char data;

  data = (unsigned char)UDR; // вытаскиваем принятый байт

  /*if (HAS_RX_DATA)
  {
    HAS_RX_DATA = 0;
    mUART_RxBuf_size = 0;
    reti();
  }*/
  if (mUART_RxBuf_size < mUART_BUFFER_SIZE - 1)
  {
    // отсекаем пустые строки
    if (!(((data == '\n') || 
           (data == '\r') || 
           (data == '\0')) && (mUART_RxBuf_size == 0)))
    {
      mUART_RxBuf[mUART_RxBuf_size] = data;
      //mUART_RxBuf_size++;
      if ((data == '\n') || (data == '\r') || (data == '\0'))
      {
        mUART_RxBuf[mUART_RxBuf_size] = '\0';
        HAS_RX_DATA = 1;
      }
      mUART_RxBuf_size++;
    }
  }
  else
  {
      mUART_RxBuf[mUART_RxBuf_size++] = data;
      mUART_RxBuf[mUART_RxBuf_size] = '\0';
      HAS_RX_DATA = 1;
  }
}
