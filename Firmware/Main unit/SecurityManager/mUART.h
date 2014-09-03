/*
Настройка параметров mUART и функции для работы с ним
*/

#define mUART_UBRR(speed, xtalCpu) ((xtalCpu/((speed) * 16l)) - 1)

#define mUART_BUFFER_SIZE 100
#define DELAY_FOR_WAIT_DATA 50

extern void mUART_init(unsigned int baudrate);
extern void mUART_putc(char data);
extern void mUART_puts(char* string);
extern unsigned char isHAS_RX_DATA();
void mUART_gets(char* res);
extern unsigned char mUART_lengthData();
extern void mUART_puti(const int value);
extern void mUART_putl(const long int value);
unsigned char mUART_getc();
char mUART_wait_and_return_string(unsigned char count_iter, // кол-во итераций
                                  char* res) ;
