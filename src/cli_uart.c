#include <string.h>
#include "../inc/cli_uart.h"
#include "inc/cli_controls.h"
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
static void cli_check_esc(char c);
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
    _CLI_TX_ON;
    while(!(_CLI_SERIAL->SR & USART_SR_TXE));
    _CLI_RX_ON;
    while(!(_CLI_SERIAL->SR & USART_SR_RXNE));
}

static void cli_init(void)
{
    //memset(cli.buffer, 0, _BUFFER_LEN); - Нет смысла делать memset, даже если там лежит мусор. Мы все равно закончим строку 0, отправив Ентер
    cli.cursor = 0;
    cli.current_len = 0;
    cli.rdy_for_parse = 0;
    cli.print_buf = NULL;
    cli.print_cnt = 0;
    cli.is_waiting_esc = 0;
    cli.ctrl_counter = 0;
}


static void cli_print(const char* data)
{
    //Отключаем любой прием символов на время отправки в консоль
    _CLI_RX_OFF;
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
    cli_print("Hello from Putty v. 0.62.0!\r\n_> ");
}


void cli_overflow_handler(void)
{   cli.status = OVERFLOW;
    cli.cursor = 0;
    cli.current_len = 0;
    cli.rdy_for_parse = 0;
    cli.print_buf = NULL;
    cli.print_cnt = 0;
    //Отключаем любой прием символов
    _CLI_IRQ_RX_OFF;
    _CLI_RX_OFF;
}


//TODO: в обработчике проверять ентер, если он нажат - готовы к парсингу, rdy_for_parse = 1.
//Также сделать обработку пустой строки на всякий случай (опционально, у нас мозги, чтобы руками не слать пустую строку)
//Сделать тест cli_print_handler
static void cli_print_handler(void)
{
    _CLI_SERIAL_DR = cli.print_buf[cli.print_cnt];
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
            _CLI_RX_ON;
            _CLI_IRQ_RX_ON;
            cli.status = RX;
        }
    }
}


static void cli_movebuf_right(void)
{
    uint8_t move_len = cli.current_len - cli.cursor;
    for (uint8_t i = 0; i < move_len; i++)
    {
        cli.buffer[cli.current_len - i] = cli.buffer[cli.current_len - 1 - i];
    }
}


static uint8_t cli_getstat(void)
{
    return cli.status;
}

static void cli_movebuf_left(void)
{
    for (uint8_t i = cli.cursor; i < cli.current_len; i++)
    {
        cli.buffer[i] = cli.buffer[i + 1];
    }
}

//
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

static void cli_check_esc(char c)
{
    cli.ctrl_buf[cli.ctrl_counter] = c;

    //TODO: дописать поиск ESC последовательности. В первую очередь сделать поиск ARROWS и HOME/END
}

//Стрелочки и home/end отправляются через последовательность с начальным контрольным значением 27. Нужно принимать
//TODO: дописать
static void cli_ctrl_handler(char c)
{
    switch(c)
    {
        case KEY_ETX: /**< ^C End of text */
            //Отключаем любой прием и ждем когда в основном цикле обработается KB_IRQ
            cli.status = KB_IRQ;
            _CLI_RX_OFF;
            _CLI_IRQ_RX_OFF;
            break;

        case KEY_BS: /**< ^H Backspace, works on HP terminals/computers */
            if (cli.cursor == 0) break;
            cli.cursor--;
        cli_movebuf_left();
        cli.current_len--;
        _CLI_SERIAL_DR = c;
        break;
        case KEY_DEL: /**< DEL/BS  */
            if (cli.cursor == 0) break;
            cli.cursor--;
        cli_movebuf_left();
        cli.current_len--;
        _CLI_SERIAL_DR = c;
        break;
        case KEY_ESC: /**< ^[ Escape, next character is not echoed */
            cli.is_waiting_esc = 1;
        default:
            break;
    }
}

void _CLI_SERIAL_IRQHandler(void)
{
    //Прием и обработка символа от пользователя
    if (_CLI_SERIAL->SR & USART_SR_RXNE && cli.status == RX)
    {
        char c = _CLI_SERIAL_DR;
        //Если принятый символ - контрольный
        if (_CLI_IS_CTRL(c) || c == KEY_DEL)
        {
            cli_ctrl_handler(c);
        }
        //Если выше было получено, что сейчас будет ESC последовательность.
        else if (cli.is_waiting_esc)
        {
            cli_check_esc(c);
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

    else
    {
        _CLI_SERIAL_DR;
    }

}

//TODO: в cli_handle() будем обрабатывать overflow/parsing.
