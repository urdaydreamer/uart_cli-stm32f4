#ifndef _USART_H_
#define _USART_H_
#include "stm32f407xx.h"
#define _BUFFER_LEN 255
#define _CLI_SERIAL USART3
#define _CLI_SERIAL_IRQn USART3_IRQn
#define _CLI_SERIAL_IRQHandler USART3_IRQHandler
#define _CLI_IS_CTRL(c) ((c) <= 31)

typedef enum
{
    RX,
    TRANSMITTING,
    ENTER,
    PARSING
} cli_status_t;


typedef struct
{
    //IRQn_Type;
    uint8_t buffer[_BUFFER_LEN];
    uint8_t cursor;
    uint8_t current_len;
    const char* print_buf;
    uint16_t print_cnt;
    cli_status_t status;
    uint8_t rdy_for_parse;
} cli_t;


void cli_begin(uint32_t baud_rate);
#endif

