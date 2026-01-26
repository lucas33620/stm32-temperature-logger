/**
 * @file cli_uart.c
 * @brief Fournir une interface CLI au dessus de l'UART
 * @copyright
 * © 2025 SYLORIA — MIT License — BAQUEY Lucas (contact@syloria.fr)
 */

#include "cli_uart.h"
#include "stm32f4xx_hal.h"

#define CLI_UART_TX_LEN_BYTES        (1U)
#define CLI_UART_TX_TIMEOUT_MS       (10U)

extern UART_HandleTypeDef huart2; /* Genere par CubeMX */

/* Buffer RX */
static volatile uint8_t  rx_line[CLI_UART_MAX_LINE_LEN];
static volatile uint32_t rx_len = 0U;
static volatile uint8_t  end_line = 0U;
static volatile uint8_t  overflow_flag = 0U;

/* --- helpers internes --- */
static CLICmd_t CliUart_DecodeLine(const uint8_t * buf, uint32_t len)
{
    CLICmd_t cmd = CLI_CMD_INVALID;

    if ((buf != NULL) && (len != 0U))
    {
        if ((len == 3U) && (buf[0] == (uint8_t)'G') && (buf[1] == (uint8_t)'E') && (buf[2] == (uint8_t)'T'))
        {
            cmd = CLI_CMD_GET;
        }
        else if ((len == 4U) && (buf[0] == (uint8_t)'L') && (buf[1] == (uint8_t)'A') && (buf[2] == (uint8_t)'S') && (buf[3] == (uint8_t)'T'))
        {
            cmd = CLI_CMD_LAST;
        }
        else if ((len == 5U) && (buf[0] == (uint8_t)'C') && (buf[1] == (uint8_t)'L') && (buf[2] == (uint8_t)'E') && (buf[3] == (uint8_t)'A') && (buf[4] == (uint8_t)'R'))
        {
            cmd = CLI_CMD_CLEAR;
        }
        else if ((len == 7U) && (buf[0] == (uint8_t)'M') && (buf[1] == (uint8_t)'E') && (buf[2] == (uint8_t)'A') &&
                 (buf[3] == (uint8_t)'S') && (buf[4] == (uint8_t)'U') && (buf[5] == (uint8_t)'R') && (buf[6] == (uint8_t)'E'))
        {
            cmd = CLI_CMD_MEASURE;
        }
        else
        {
            cmd = CLI_CMD_INVALID;
        }
    }

    return cmd;
}

void CliUart_Init(void)
{
    rx_len = 0U;
    end_line = 0U;
    overflow_flag = 0U;
}

/* Note: CR and LF are both treated as end-of-line.
 * A CRLF sequence will trigger end-of-line on first character only.
 */
void CliUart_OnRxChar(uint8_t c)
{
    /* Si une ligne est deja complete, on ignore les nouveaux chars jusqu'au reset par Process() */
    if (end_line != 0U)
    {
        (void)c;
    }
    else if ((c == (uint8_t)'\r') || (c == (uint8_t)'\n'))
    {
        if (rx_len != 0U)
        {
            end_line = 1U;
        }
    }
    else
    {
        if (rx_len < ((uint32_t)CLI_UART_MAX_LINE_LEN - 1U))
        {
            rx_line[rx_len] = c;
            rx_len += 1U;
        }
        else
        {
            overflow_flag = 1U;
            rx_len = 0U;
        }
    }
}

CLIStatus_t CliUart_Process(CLICmd_t * out_cmd)
{
    CLIStatus_t status = CLI_STATUS_OK;

    if (out_cmd == NULL)
    {
        status = CLI_STATUS_ERR_PARAM;
    }
    else
    {
        *out_cmd = CLI_CMD_INVALID;

        if (overflow_flag != 0U)
        {
            overflow_flag = 0U;
            status = CLI_STATUS_ERR_OVERFLOW;
            CliUart_OnOverflow();
        }

        if ((end_line != 0U) && (status == CLI_STATUS_OK))
        {
            uint32_t len_copy = rx_len;
            uint8_t  line_copy[CLI_UART_MAX_LINE_LEN];
            uint32_t i;

            if (len_copy >= (uint32_t)CLI_UART_MAX_LINE_LEN)
            {
                /* Defensive: should never happen */
                rx_len = 0U;
                end_line = 0U;
                status = CLI_STATUS_ERR_OVERFLOW;
            }
            else
            {
                for (i = 0U; i < len_copy; i++)
                {
                    line_copy[i] = rx_line[i];
                }

                rx_len = 0U;
                end_line = 0U;

                *out_cmd = CliUart_DecodeLine(&line_copy[0], len_copy);
            }
        }
    }

    return status;
}

CLIStatus_t CliUart_TxChar(uint8_t c)
{
    HAL_StatusTypeDef hal_status;
    CLIStatus_t status = CLI_STATUS_OK;

    hal_status = HAL_UART_Transmit(&huart2,
                                   &c,
                                   (uint16_t)CLI_UART_TX_LEN_BYTES,
                                   (uint32_t)CLI_UART_TX_TIMEOUT_MS);

    if (hal_status != HAL_OK)
    {
        status = CLI_STATUS_ERR_UART;
    }

    return status;
}

/* -- API -- */
#if defined(__GNUC__)
#define CLI_UART_WEAK __attribute__((weak))
#else
#define CLI_UART_WEAK
#endif

CLI_UART_WEAK void CliUart_OnOverflow(void)
{
    /* Default empty task; may be overridden by application */
}
