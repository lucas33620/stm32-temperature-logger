/**
 * @file cli_uart.h
 * @brief Fournir une interface CLI au dessus de l'UART
 * @copyright
 * © 2025 Syloria — MIT
 */

#ifndef CLIUART_H
#define CLIUART_H

#include <stdint.h>

#define CLI_UART_MAX_LINE_LEN (64U)

/* Statuts */
typedef enum
{
    CLI_STATUS_OK = 0,
    CLI_STATUS_ERR_PARAM,
    CLI_STATUS_ERR_OVERFLOW
} CLIStatus_t;

/*Commandes normées*/
typedef enum
{
    CLI_CMD_INVALID = 0,
    CLI_CMD_GET,
    CLI_CMD_LAST,
    CLI_CMD_CLEAR,
    CLI_CMD_MEASURE,
    CLI_CMD_COUNT
} CLICmd_t;

/*Initialisation le module CLI*/
CLIStatus_t CliUart_Init(void);

/*Fournit au CLI un caractere recu sur l'UART.*/
void CliUart_OnRxChar(uint8_t c);

/*Traite les caracteres bufferises et decode une commande si une ligne est complete.*/
CLIStatus_t CLIUart_Process(CLICmd_t * out_cmd);

#endif /* CLIUART */
