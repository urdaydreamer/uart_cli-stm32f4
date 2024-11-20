#include <string.h>
#include "../inc/cli_uart.h"
static uint32_t cli_getfreq(void);
static uint8_t cli_getAPB1_div(void);
static void cli_rcc_init(void);
static void cli_uart_init(uint32_t baud_rate);
static void cli_print(const char*);
static void cli_init(void);
static void cli_ctrl_handler(char c);
static void cli_movebuf_right(void);
static void cli_movebuf_left(void);
static void cli_putchar(char c);
static void cli_overflow_handler(void);
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
    _CLI_IRQ_RX_ON;
    while(!(_CLI_SERIAL->SR & USART_SR_TXE));
    _CLI_SERIAL_DR = 0x7F;
    _CLI_SERIAL->CR1 |= USART_CR1_RE;
}

static void cli_init(void)
{
    memset(cli.buffer, 0, _BUFFER_LEN);
    cli.cursor = 0;
    cli.current_len = 0;
    cli.rdy_for_parse = 0;
    cli.print_buf = NULL;
    cli.print_cnt = 0;
}


static void cli_print(const char* data)
{
    _CLI_IRQ_RX_OFF;
    cli.print_buf = data;
    cli.print_cnt = 0;
    cli.status = TRANSMITTING;
    _CLI_IRQ_TX_ON;
}


void cli_begin(uint32_t baud_rate)
{
    cli_init();
    cli_rcc_init();
    cli_uart_init(baud_rate);
    cli_print("Hello from Putty v. 0.62.0!\r\n_>");
}


void cli_overflow_handler(void)
{   cli.status = OVERFLOW;
    cli.cursor = 0;
    cli.current_len = 0;
    cli.rdy_for_parse = 0;
    cli.print_buf = NULL;
    cli.print_cnt = 0;
    _CLI_IRQ_RX_OFF;
}

static void cli_ctrl_handler(char c)
{

}

//TODO: в обработчике проверять ентер, если он нажат - готовы к парсингу, rdy_for_parse = 1.
//Также сделать обработку пустой строки на всякий случай (опционально, у нас мозги, чтобы руками не слать пустую строку)
//Сделать тест cli_print_handler
static void cli_print_handler(void)
{
    _CLI_SERIAL_DR = cli.print_buf[cli.print_cnt];
    //В строке лишь один символ для отправки
    if (cli.print_buf[cli.print_cnt] && !cli.print_buf[cli.print_cnt + 1])
    {
        _CLI_IRQ_TX_OFF;
        if (cli.rdy_for_parse)
        {
            cli.status = PARSING;
            //Нам нет смысла очищать буффер, потому что команда в любом случае будет оканчиваться '0'
            cli.cursor = 0;
            cli.current_len = 0;
            cli.status = RX;
            cli.rdy_for_parse = 0;
            cli.print_buf = NULL;
            cli.print_cnt = 0;
        }
        else
        {
            _CLI_IRQ_RX_ON;
            cli.status = RX;
        }
    }
}


//TODO: TEST cli_movebuf_right
static void cli_movebuf_right(void)
{
    uint8_t move_len = cli.current_len - cli.cursor;
    for (uint8_t i = 0; i < move_len; i++)
    {
        cli.buffer[cli.current_len - i] = cli.buffer[cli.current_len - 1 - i];
    }
}
// abrikos len = 7 cursor = 2


static uint8_t cli_getstat(void)
{
    return cli.status;
}

static void cli_movebuf_left(void)
{

}


static void cli_putchar(char c)
{
    if (cli.current_len + 1 > _BUFFER_LEN)
    {
        cli_overflow_handler();
        return;
    }
    if (cli.cursor != cli.current_len) cli_movebuf_right();
    cli.buffer[cli.cursor] = c;
    cli.current_len++;
    cli.cursor++;
    _CLI_SERIAL_DR = c;
}

void _CLI_SERIAL_IRQHandler(void)
{
    //Прием и обработка символа от пользователя
    if (_CLI_SERIAL->SR & USART_SR_RXNE && cli.status == RX)
    {
        char c = _CLI_SERIAL_DR;
        //Если принятый символ - контрольный
        if (_CLI_IS_CTRL(c))
        {
            cli_ctrl_handler(c);
        }
        //Если обычный - просто записываем его в буффер и отправляем в терминал по курсору
        else
        {
            cli_putchar(c);
        }
    }

    //Отправка строки в CLI
    else if (cli.status == TRANSMITTING && _CLI_SERIAL->SR & USART_SR_TXE)
    {
        cli_print_handler();
    }

}

//TODO: в cli_handle() будем обрабатывать overflow/parsing.
