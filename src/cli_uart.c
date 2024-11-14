#include <string.h>
#include "../inc/cli_uart.h"
static void cli_rcc_init(void);
static void cli_print(__IO char*);
static void cli_init(void);
static void cli_ctrl_handler(void);
static cli_t cli;



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



static void cli_print(__IO char* data)
{
    cli.print_buf = data;
    cli.status = TRANSMITTING;
    _CLI_SERIAL->CR1 |= USART_CR1_TXEIE;
}

void cli_begin(uint32_t baud_rate, uint8_t freq_mhz)
{
    cli_init();
    cli_rcc_init();
    NVIC_EnableIRQ(_CLI_SERIAL_IRQn);
    //TODO:Добавить сюда приоритет прерываний
    uint32_t uart_div = (uint32_t) freq_mhz * 1000000 / APB1_DEVIDER / baud_rate;
    _CLI_SERIAL->CR1 |= USART_CR1_UE;
    _CLI_SERIAL->BRR = ((uart_div % 16) & 0xF) | ((uart_div / 16) << 4);
    _CLI_SERIAL->CR1 |= USART_CR1_TE;
    while(!(_CLI_SERIAL->SR & USART_SR_TXE));
    _CLI_SERIAL->DR = 0x7F;
    _CLI_SERIAL->CR1 |= USART_CR1_RE;
    _CLI_SERIAL->CR1 |= USART_CR1_RXNEIE;
    cli_print("Hello from Putty v. 0.62.0!\r\n_>");
}


static void cli_ctrl_handler(void)
{

}


static void cli_print_handler()
{

}

void _CLI_SERIAL_IRQHandler(void)
{
    if (USART3->SR & USART_SR_RXNE && cli.status == RX) //Обрабатываем прием символа
    {
        //TODO: добавить проверку на выход за границы длины буффера
        cli.buffer[cli.cursor] = _CLI_SERIAL->DR;
        cli.cursor++;
        cli.current_len++;

        if (_CLI_IS_CTRL(cli.buffer[cli.cursor - 1]))
        {
            cli_ctrl_handler();
        }
        else
        {
            _CLI_SERIAL->DR = cli.buffer[cli.cursor - 1];
        }
    }

    else if (cli.status == TRANSMITTING && USART3->SR & USART_SR_TXE)                    //Обработка отправки символа/строки в терминал
    {
        //TODO: вся обработка отправки строки будет здесь в одной функции cli_print_handler
    }
}
