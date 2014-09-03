#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include "mUART.h"

char mUART_RxBuf[mUART_BUFFER_SIZE + 1]; // ����� ��� �������� ������ ��������
unsigned char mUART_RxBuf_size; // ����� �������� ������
unsigned char HAS_RX_DATA; // ���� ������� �������� ������


char mUART_RxBuf2[mUART_BUFFER_SIZE + 1]; // ����� ��� �������� ������ ��������������
unsigned char mUART_RxBuf_size2; // ����� �������� ������ � �������������� ������
unsigned char HAS_RX_DATA2; // ���� ������� �������� ������ ���������������� ������

void mUART_init(unsigned int baudrate)
{
  /*UBRRH = 0;
  UBRRL = 25; // �������� 19200*/
  UBRRH = (unsigned char)(baudrate>>8);
  UBRRL = (unsigned char) baudrate;
  UCSRC = ( 1 << URSEL ) // ��� ������� � �������� UCSRC ���������� ��� URSEL.
          | ( 0 << UCSZ2 ) | ( 1 << UCSZ1 ) | ( 1 << UCSZ0 ) // ���������� ��� ������ ����� 8
          | ( 0 << UPM1 ) | ( 0 << UPM0 ) // ��������� �������� ��������
          | ( 0 << USBS ); // 1 ���� ���
  UCSRB = (1 << RXCIE)    // ��������� ���������� �� ������
          | (1 << RXEN)   // ��������� �����
          | (1 << TXEN);  // ��������� ��������

  mUART_RxBuf_size = 0;
  HAS_RX_DATA = 0;

  mUART_RxBuf_size2 = 0;
  HAS_RX_DATA2 = 0;
}

void mUART_putc(char data)
{
  while (!(UCSRA & (1 << UDRE)) ); //�������� ����������� ������ ������
  UDR = data; //������ �������� ������	
}

void mUART_puts(char *string)
{
  /*
  ����� ������ ������������ \0
  ��� ��� �� \0 �� true
  */
  while(*string)
  {
    mUART_putc(*string);
    string++;
  }
}

// ������� � mUART int
void mUART_puti(const int value)
{
  char buf[10];

  mUART_puts(itoa(value, buf, 10));
}

// ������� � UART unsigned int
void mUART_putu(const unsigned int value)
{
  char buf[10];

  mUART_puts(utoa(value, buf, 10));
}

// ������� � mUART long int
void mUART_putl(const long int value)
{
  char buf[15];

  mUART_puts(ltoa(value, buf, 10));
}

unsigned char isHAS_RX_DATA()
{
  return HAS_RX_DATA;
}

// ���������� ������� ����� ���������
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

  // �������� ���������� � �������� ������
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
    // �������� ���������� � �������� ������
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

char mUART_wait_and_return_string(unsigned char count_iter, // ���-�� ��������
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

// ��������� ������ �� ���������������� ������ � ��������
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

// ��������� ���������� �� ������ �����
ISR(USART_RXC_vect)
{
  char data;
  
  data = UDR; // ����������� �������� ����

  if (HAS_RX_DATA == 0) 
  {
    copy_to_main_buffer();
    if (mUART_RxBuf_size < mUART_BUFFER_SIZE - 1)
    {
      // �������� ������ ������
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
      // �������� ������ ������
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
