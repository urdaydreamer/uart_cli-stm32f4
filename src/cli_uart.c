#include <string.h>
#include "../inc/cli_uart.h"
static uint32_t cli_getfreq(void);
static uint8_t cli_getAPB1_div(void);
static void cli_rcc_init(void);
static void cli_uart_init(uint32_t baud_rate);
static void cli_print(const char*);
static void cli_init(void);
static void cli_ctrl_handler(void);
static void cli_movebuf_right(void);
static void cli_movebuf_left(void);
static __IO cli_t cli;



static void cli_rcc_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    GPIOD->MODER |= GPIO_MODER_MODER8_1;
    GPIOD->OSPEEDR |= GPIO_OSPEEDER_OSPEEDR8; //TX - PD8
    GPIOD->AFR[1] |= 7;
    GPIOD->MODER |= GPIO_MODER_MODER9_1;       //RX - PD9
    GPIOD->AFR[1] |= (7 << 4);
    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
}


static uint32_t cli_getfreq(void)
{

}

static uint8_t cli_getAPB1_div(void)
{

}


static void cli_uart_init(uint32_t baud_rate)
{
    uint32_t freq = cli_getfreq();
    uint8_t APB1_div = cli_getAPB1_div();
    NVIC_EnableIRQ(_CLI_SERIAL_IRQn);
    //TODO:Добавить сюда приоритет прерываний
    uint32_t uart_div =  freq / (uint32_t) APB1_div / baud_rate;
    _CLI_SERIAL->CR1 |= USART_CR1_UE;
    _CLI_SERIAL->BRR = ((uart_div % 16) & 0xF) | ((uart_div / 16) << 4);
    _CLI_SERIAL->CR1 |= USART_CR1_TE;
    while(!(_CLI_SERIAL->SR & USART_SR_TXE));
    _CLI_SERIAL->DR = 0x7F;
    _CLI_SERIAL->CR1 |= USART_CR1_RE;
    _CLI_SERIAL->CR1 |= USART_CR1_RXNEIE;
}

static void cli_init(void)
{
    memset(cli.buffer, 0, _BUFFER_LEN);
    cli.cursor = 0;
    cli.current_len = 0;
    cli.status = RX;
    cli.rdy_for_parse = 0;
    cli.print_buf = NULL;
    cli.print_cnt = 0;
}



static void cli_print(const char* data)
{
    cli.print_buf = data;
    cli.status = TRANSMITTING;
    _CLI_SERIAL->CR1 |= USART_CR1_TXEIE;
}



void cli_begin(uint32_t baud_rate)
{

    cli_init();
    cli_rcc_init();
    cli_uart_init(baud_rate);
    cli_print("Hello from Putty v. 0.62.0!\r\n_>");
}


static void cli_ctrl_handler(void)
{

}


static void cli_print_handler(void)
{

}

static void cli_movebuf_left(void)
{
    uint8_t move_len = cli.current_len - cli.cursor;
    // for (uint8_t i = 0; )
}

static void cli_movebuf_right(void)
{

}

static void cli_putchar(char c)
{
    //TODO: Сделать проверку на выход за границы буфера.
    if (cli.cursor == cli.current_len)
    {
        cli.buffer[cli.cursor] = c;
        cli.current_len++;
        cli.cursor++;
        _CLI_SERIAL->DR = c;
    }
    else
    {
        cli_movebuf_right();
    }
}

void _CLI_SERIAL_IRQHandler(void)
{
    if (USART3->SR & USART_SR_RXNE && cli.status == RX) //Обрабатываем прием символа
    {
        char c = _CLI_SERIAL->DR;
        if (_CLI_IS_CTRL(c))
        {
            cli_ctrl_handler();
        }
        else
        {
            cli_putchar(c);
        }
    }

    else if (cli.status == TRANSMITTING && USART3->SR & USART_SR_TXE)                    //Обработка отправки символа/строки в терминал
    {
        cli_print_handler();
    }
}

