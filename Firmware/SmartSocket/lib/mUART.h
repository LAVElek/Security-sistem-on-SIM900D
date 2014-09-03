/*
Настройка параметров mUART и функции для работы с ним
*/

#define mUART_UBRR(speed, xtalCpu) ((xtalCpu/((speed) * 16l)) - 1)

#define mUART_BUFFER_SIZE 100

extern void mUART_init(unsigned int baudrate);
extern void mUART_putc(char data);
extern void mUART_puts(char *string);
extern unsigned char isHAS_RX_DATA();
extern char * mUART_gets();
extern unsigned char mUART_lengthData();
extern void mUART_puti(const int value);
void mUART_putu(const unsigned int value);
extern void mUART_putl(const long int value);
unsigned char mUART_getc();
