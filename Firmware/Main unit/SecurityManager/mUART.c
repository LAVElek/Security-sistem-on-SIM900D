#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "mUART.h"

char mUART_RxBuf[mUART_BUFFER_SIZE + 1]; // буфер для принятых данных основной
unsigned char mUART_RxBuf_size; // длина принятых данных
unsigned char HAS_RX_DATA; // флаг наличия принятый данных


char mUART_RxBuf2[mUART_BUFFER_SIZE + 1]; // буфер для принятых данных дополнительный
unsigned char mUART_RxBuf_size2; // длина принятых данных в дополнительном буфере
unsigned char HAS_RX_DATA2; // флаг наличия принятый данных вспомогательного буфера

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

  mUART_RxBuf_size2 = 0;
  HAS_RX_DATA2 = 0;
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

void mUART_gets(char *res)
{
  res[0] = '\0';
  if (mUART_RxBuf_size > 0)
  {
    strcat(res, mUART_RxBuf);
  }

  // обнуляем информацию о принятых данных
  mUART_RxBuf[0] = '\0';
  mUART_RxBuf_size = 0;
  HAS_RX_DATA = 0;

  copy_to_main_buffer();
}

unsigned char mUART_getc()
{
  unsigned char res;

  if ((HAS_RX_DATA == 0) || (mUART_RxBuf_size ==0))
    res = '\0';
  else
  {
    // обнуляем информацию о принятых данных
    mUART_RxBuf_size = 0;
    HAS_RX_DATA = 0;

    res = *mUART_RxBuf;

    copy_to_main_buffer();
  }

  return res;
}

void mUART_clear_buffer()
{
  HAS_RX_DATA = 0;
  mUART_RxBuf_size = 0;
  mUART_RxBuf[0] = '\0';

  HAS_RX_DATA2 = 0;
  mUART_RxBuf_size2 = 0;
  mUART_RxBuf2[0] = '\0';
}

char mUART_wait_and_return_string(unsigned char count_iter, // кол-во итераций
                                  char* res) 
{
  unsigned char i = 0;

  //mUART_clear_buffer();
  res[0] = '\0';

  while ((!HAS_RX_DATA) && (i++ < count_iter))
  {
    _delay_ms(DELAY_FOR_WAIT_DATA);
  }

  if (i != count_iter)  
  {
    strcat(res, mUART_RxBuf);
    copy_to_main_buffer();
    return 1;
  }
  else
    return 0;
}

// переносит данные из вспомогательного буфера в основной
void copy_to_main_buffer()
{
  if (mUART_RxBuf_size2 > 0)
  {
    mUART_RxBuf[0] = '\0';

    strcat(mUART_RxBuf, mUART_RxBuf2);
    mUART_RxBuf_size = mUART_RxBuf_size2;
    HAS_RX_DATA = HAS_RX_DATA2;

    mUART_RxBuf_size2 = 0;
    HAS_RX_DATA2 = 0;
    mUART_RxBuf2[0] = '\0';
  }
}

// обработка прерывания по приему байта
ISR(USART_RXC_vect)
{
  char data;
  
  data = UDR; // вытаскиваем принятый байт

  if (HAS_RX_DATA == 0) 
  {
    copy_to_main_buffer();
    if (mUART_RxBuf_size < mUART_BUFFER_SIZE - 1)
    {
      // отсекаем пустые строки
      if (!(((data == '\n') || 
             (data == '\r') || 
             (data == '\0')) && (mUART_RxBuf_size == 0)))
      {
        if ((data == '\n') || (data == '\r') || (data == '\0'))
        {
          mUART_RxBuf[mUART_RxBuf_size] = '\0';
          HAS_RX_DATA = 1;
        }
        else
        {
          mUART_RxBuf[mUART_RxBuf_size] = data;
        }
        mUART_RxBuf_size++;
      }
    }
    else
    {
        //mUART_RxBuf[mUART_RxBuf_size++] = data;
        mUART_RxBuf[mUART_RxBuf_size] = '\0';
        HAS_RX_DATA = 1;
    }
  }
  else if (HAS_RX_DATA2 == 0) 
  { 
    if (mUART_RxBuf_size2 < mUART_BUFFER_SIZE - 1)
    {
      // отсекаем пустые строки
      if (!(((data == '\n') || 
             (data == '\r') || 
             (data == '\0')) && (mUART_RxBuf_size2 == 0)))
      {
        if ((data == '\n') || (data == '\r') || (data == '\0'))
        {
          mUART_RxBuf2[mUART_RxBuf_size2] = '\0';
          HAS_RX_DATA2 = 1;
        }
        else
        {
          mUART_RxBuf2[mUART_RxBuf_size2] = data;
        }
        mUART_RxBuf_size2++;
      }
    }
    else
    {
        //mUART_RxBuf2[mUART_RxBuf_size2++] = data;
        mUART_RxBuf2[mUART_RxBuf_size2] = '\0';
        HAS_RX_DATA2 = 1;
    }
  }
}
