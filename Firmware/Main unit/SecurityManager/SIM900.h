#include <avr/io.h>

// ���� ������� ���900
#define RESULT_COMMAND_OK '0'
#define RESULT_COMMAND_ERROR '4'

// ���������� ���������� �������
#define COMMAND_COMPLETED 1
#define COMMAND_FAILED 0

#define RING 2

// ����� ������ ��������
#define LENGTH_NUMBER 12

// ����� �������� ���������
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


// �� ��������
#define AT_SMS_SEND "AT+CMGS=" // �������� ���������
#define AT_SMS_READ "AT+CMGR=1,0" // ������ ���������� ����������� ���������
#define AT_SMS_DELETE_ALL "AT+CMGD=1,4" // �������� ���� ���������

#define AT_PHONENUMBER_ADD "AT+CPBW=" // ���������� ������ �������� � ���������� AT+CPBW=4,"+71234567890",145,�Test�
#define AT_PHONE_MEMORY_STATE "AT+CPBS?" // ��������� ������ � ���������� �����
#define AT_CALL "ATD" // ��������� �� ������. ����� ������ ���������� ';'
#define AT_ANSWER_TO_CALL "ATA" // �������� �� �������� ������
#define AT_BREAK_CALL "ATH0" // ��������� ��� ����������

#define AT_IMEI "AT+GSN" // ����������� IMEI
#define AT_SIGNAL_LEVEL "AT+CSQ" // �������� ������ �������
#define AT_SIM_STATE "AT+CPAS" // ��������� ������
  #define SIM_READY 0 // ����� � ������
  #define SIM_UNKNOWN 2 // �� ����������
  #define SIM_IN_CALL 3 // ���� �������� ������
  #define SIM_HAS_VOICE_CALL 4 // ����������� ��������� ����������

#define AT_MODE_EHO "ATE0" // ����� ��� ��������
#define AT_MODE_REPORT "ATV0" // ���������� ������ ��� ������
#define AT_MODE_ERROR_RETURN "AT+CMEE=1" // ��� ������ ���������� �� �����


extern void power_on_off(); 
extern unsigned char send_sms(char *msg, char *tel);
extern void call_on_number(char *number);
extern void read_sms(char* sms,  // ����� ���
                     char* tel,  // ������� � �������� ������ ���
                     char* contact_name // ��� �������� �� ���������� �����
                    );
extern int add_telephone(char *tel);
extern void get_power_signal(char* res);
