#ifndef _USART_H_
#define _USART_H_
#include "stm32f407xx.h"
#define _BUFFER_LEN 255
#define _CLI_SERIAL USART3
#define _CLI_SERIAL_IRQn USART3_IRQn
#define _CLI_SERIAL_IRQHandler USART3_IRQHandler
#define _CLI_IS_CTRL(c) ((c) <= 31)
#define APB1_DEVIDER 1

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
    __IO uint8_t buffer[_BUFFER_LEN];
    __IO uint8_t cursor;
    __IO uint8_t current_len;
    __IO char* print_buf;
    __IO uint16_t print_cnt;
    cli_status_t status;
    __IO uint8_t rdy_for_parse;
} cli_t;


void cli_begin(uint32_t baud_rate, uint8_t freq_mhz);
#endif

