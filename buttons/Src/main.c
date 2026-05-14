#include <stdint.h>

#define RCC_BASE    0x40023800
#define GPIOA_BASE  0x40020000
#define GPIOB_BASE  0x40020400
#define USART1_BASE 0x40011000

#define RCC_AHB1ENR  (*(volatile uint32_t*)(RCC_BASE + 0x30))
#define RCC_APB2ENR  (*(volatile uint32_t*)(RCC_BASE + 0x44))

#define GPIOA_MODER  (*(volatile uint32_t*)(GPIOA_BASE + 0x00))
#define GPIOA_PUPDR  (*(volatile uint32_t*)(GPIOA_BASE + 0x0C))
#define GPIOA_IDR    (*(volatile uint32_t*)(GPIOA_BASE + 0x10))
#define GPIOA_AFRH   (*(volatile uint32_t*)(GPIOA_BASE + 0x24))

#define GPIOB_MODER  (*(volatile uint32_t*)(GPIOB_BASE + 0x00))
#define GPIOB_ODR    (*(volatile uint32_t*)(GPIOB_BASE + 0x14))

#define USART1_SR    (*(volatile uint32_t*)(USART1_BASE + 0x00))
#define USART1_DR    (*(volatile uint32_t*)(USART1_BASE + 0x04))
#define USART1_BRR   (*(volatile uint32_t*)(USART1_BASE + 0x08))
#define USART1_CR1   (*(volatile uint32_t*)(USART1_BASE + 0x0C))

// COL: PB4-PB7 (выходы)
// ROW: PA0-PA3 (входы)
#define NUMROWS 4
#define NUMCOLS 4

const uint8_t buttons[NUMROWS][NUMCOLS] = {
    { 0,  1,  2,  3},
    { 4,  5,  6,  7},
    { 8,  9, 10, 11},
    {12, 13, 14, 15},
};

uint8_t btn_state[16] = {0};

void uart_send_char(char c)
{
    while (!(USART1_SR & (1 << 7)));
    USART1_DR = c;
}

void uart_send_str(const char *s)
{
    while (*s) uart_send_char(*s++);
}

void uart_send_num(uint8_t n)
{
    if (n >= 10) uart_send_char('0' + n / 10);
    uart_send_char('0' + n % 10);
}

void set_button(uint8_t btn, uint8_t val)
{
    if (btn_state[btn] == val) return;
    btn_state[btn] = val;
    uart_send_str("BTN ");
    uart_send_num(btn);
    uart_send_char(' ');
    uart_send_char(val ? '1' : '0');
    uart_send_str("\r\n");
}

void delay(volatile uint32_t n)
{
    while (n--);
}

void check_buttons(void)
{
    for (int col = 0; col < NUMCOLS; col++)
    {
        // Все COL HIGH, текущий LOW
        GPIOB_ODR |=  (0xF0);
        GPIOB_ODR &= ~(1 << (col + 4));
        delay(200);

        for (int row = 0; row < NUMROWS; row++)
        {
            // ROW читается LOW если кнопка нажата
            uint8_t pressed = !(GPIOA_IDR & (1 << row));
            set_button(buttons[row][col], pressed);
        }
    }
    GPIOB_ODR |= (0xF0); // все COL обратно HIGH
}

int main(void)
{
    RCC_AHB1ENR |= (1 << 0) | (1 << 1);
    RCC_APB2ENR |= (1 << 4);

    // PA0-PA3 — ROW (входы, без подтяжки)
    GPIOA_MODER &= ~(0x000000FF); // input
    GPIOA_PUPDR &= ~(0x000000FF); // no pull

    // PA9, PA10 — USART1 AF7
    GPIOA_MODER &= ~((3 << 18) | (3 << 20));
    GPIOA_MODER |=  ((2 << 18) | (2 << 20));
    GPIOA_AFRH  &= ~((0xF << 4) | (0xF << 8));
    GPIOA_AFRH  |=  ((7   << 4) | (7   << 8));

    // PB4-PB7 — COL (выходы, все HIGH)
    GPIOB_MODER &= ~(0x0000FF00);
    GPIOB_MODER |=  (0x00005500); // output
    GPIOB_ODR   |=  (0xF0);       // HIGH

    // USART1: 9600 бод
    USART1_BRR = 0x683;
    USART1_CR1 = (1 << 3) | (1 << 2) | (1 << 13);

    uart_send_str("Ready\r\n");

    while (1)
    {
        check_buttons();
    }
}
