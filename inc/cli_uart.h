#ifndef _USART_H_
#define _USART_H_
#include "stm32f407xx.h"

/* Depends on HARDWARE start */
#define _CLI_SERIAL               USART3
#define _CLI_SERIAL_IRQn          USART3_IRQn
#define _CLI_SERIAL_IRQHandler    USART3_IRQHandler
#define _CLI_RX_ON                (_CLI_SERIAL->CR1 |= USART_CR1_RE)
#define _CLI_RX_OFF               (_CLI_SERIAL->CR1 &= ~USART_CR1_RE)
#define _CLI_TX_ON                (_CLI_SERIAL->CR1 |= USART_CR1_TE)
#define _CLI_TX_OFF               (_CLI_SERIAL->CR1 &= ~USART_CR1_TE)
#define _CLI_IRQ_RX_ON            (_CLI_SERIAL->CR1 |= USART_CR1_RXNEIE)
#define _CLI_IRQ_RX_OFF           (_CLI_SERIAL->CR1 &= ~USART_CR1_RXNEIE)
#define _CLI_IRQ_TX_ON            (_CLI_SERIAL->CR1 |= ~USART_CR1_TXEIE)
#define _CLI_IRQ_TX_OFF           (_CLI_SERIAL->CR1 &= ~USART_CR1_TXEIE)
#define _CLI_SERIAL_DR            (_CLI_SERIAL->DR)
/* Depends on HARDWARE end */

#define _CLI_IS_CTRL(c)           ((c) <= 31)
#define _BUFFER_LEN               255

typedef enum
{   KB_IRQ,
    OVERFLOW,
    RX,
    TRANSMITTING,
    PARSING
} cli_status_t;


typedef struct
{
    //IRQn_Type;
    char buffer[_BUFFER_LEN];
    __IO uint16_t cursor;
    __IO uint16_t current_len;
    const char* print_buf;
    __IO uint16_t print_cnt;
    __IO cli_status_t status;
    __IO uint8_t rdy_for_parse : 1;
    __IO uint8_t is_waiting_esc : 1;
    __IO uint8_t ctrl_counter : 6;
} cli_t;


void cli_begin(uint32_t baud_rate);
void _CLI_SERIAL_IRQHandler(void);
#endif
