#include <avr/io.h>

// коды ответов ЫШЬ900
#define RESULT_COMMAND_OK '0'
#define RESULT_COMMAND_ERROR '4'

// результаты выполнения комманд
#define COMMAND_COMPLETED 1
#define COMMAND_FAILED 0

#define RING 2

// длина номера телефона
#define LENGTH_NUMBER 12

// конец передачи сообщения
#define END_MESSAGE 0x1A
#define ESCAPE 0x1B
#define END_COMMAND "\r" // CR

#define POWER_PORT PORTD
#define POWER_DDR DDRD
#define POWER_PIN PD4

#define RING_PORT PORTD
#define RING_DDR DDRD
#define RING_PIN PD3

#define LENGTH_IN_SMS mUART_BUFFER_SIZE
#define LENGTH_OUT_SMS 60


// АТ комманды
#define AT_SMS_SEND "AT+CMGS=" // отправка сообщения
#define AT_SMS_READ "AT+CMGR=1,0" // чтение последнего полученного сообщения
#define AT_SMS_DELETE_ALL "AT+CMGD=1,4" // удаление всех сообщений

#define AT_PHONENUMBER_ADD "AT+CPBW=" // добавление номера телефона в справочник AT+CPBW=4,"+71234567890",145,«Test»
#define AT_PHONE_MEMORY_STATE "AT+CPBS?" // состояние памяти в телефонной книге
#define AT_CALL "ATD" // позвонить по номеру. после номера необходима ';'
#define AT_ANSWER_TO_CALL "ATA" // ответить на входящий звонок
#define AT_BREAK_CALL "ATH0" // разорвать все соединения

#define AT_IMEI "AT+GSN" // определение IMEI
#define AT_SIGNAL_LEVEL "AT+CSQ" // проверка уровня сигнала
#define AT_SIM_STATE "AT+CPAS" // состояние модуля
  #define SIM_READY 0 // готов к работе
  #define SIM_UNKNOWN 2 // не определено
  #define SIM_IN_CALL 3 // идет входящий звонок
  #define SIM_HAS_VOICE_CALL 4 // установлено голосовое соединение

#define AT_MODE_EHO "ATE0" // режим эха отключен
#define AT_MODE_REPORT "ATV0" // возвращает только код ответа
#define AT_MODE_ERROR_RETURN "AT+CMEE=1" // при ошибке возвращает ее номер


extern void power_on_off(); 
extern unsigned char send_sms(char *msg, char *tel);
extern void call_on_number(char *number);
extern void read_sms(char* sms,  // текст смс
                     char* tel,  // телефон с которого пришло смс
                     char* contact_name // имя контакта из телефонной книги
                    );
extern int add_telephone(char *tel);
extern void get_power_signal(char* res);
