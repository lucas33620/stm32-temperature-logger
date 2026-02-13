/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Demo MCP9808 (tick 10ms, sample 100ms, UART logs)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN Includes */
#include "scheduler.h"
#include "app_mcp9808.h"
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

/* USER CODE BEGIN PV */
static volatile int16_t  g_last_temp_x10 = 0;  /* watch variable */
static volatile uint32_t g_samples_ok    = 0U;
static volatile uint32_t g_samples_stale = 0U;
static volatile uint32_t g_samples_fault = 0U;
/* USER CODE END PV */

void SystemClock_Config(void);

/* USER CODE BEGIN 0 */

#define LOG_CRLF "\r\n"

/* Optional ANSI colors (PuTTY: depends on config; keep OFF by default) */
#define LOG_USE_ANSI_COLOR (0U)

#if (LOG_USE_ANSI_COLOR == 1U)
#define ANSI_RST "\x1B[0m"
#define ANSI_RED "\x1B[31m"
#define ANSI_GRN "\x1B[32m"
#define ANSI_YEL "\x1B[33m"
#define ANSI_BLU "\x1B[34m"
#else
#define ANSI_RST ""
#define ANSI_RED ""
#define ANSI_GRN ""
#define ANSI_YEL ""
#define ANSI_BLU ""
#endif

static uint16_t AppendUIntToBuf(char *buf, uint16_t pos, uint32_t val)
{
    char tmp[10];
    uint16_t i = 0U;

    if (val == 0UL)
    {
        buf[pos++] = '0';
        return pos;
    }

    while ((val != 0UL) && (i < (uint16_t)sizeof(tmp)))
    {
        tmp[i++] = (char)('0' + (char)(val % 10UL));
        val /= 10UL;
    }

    while (i > 0U)
    {
        i--;
        buf[pos++] = tmp[i];
    }

    return pos;
}


static const char* SensorStatus_ToStr(SensorStatus_t st)
{
    switch (st)
    {
        case SENSOR_OK:         return "OK";
        case SENSOR_ERR_INIT:   return "INIT";
        case SENSOR_ERR_PARAM:  return "PARAM";
        case SENSOR_ERR_I2C:    return "I2C";
        case SENSOR_ERR_STALE:  return "STALE";
        case SENSOR_ERR_TIMEOUT:return "TIMEO";
        default:                return "UNK";
    }
}

static const char* Color_For(SensorStatus_t st)
{
    switch (st)
    {
        case SENSOR_OK:        return ANSI_GRN;
        case SENSOR_ERR_STALE: return ANSI_YEL;
        case SENSOR_ERR_I2C:   return ANSI_RED;
        case SENSOR_ERR_TIMEOUT:return ANSI_RED;
        case SENSOR_ERR_INIT:  return ANSI_BLU;
        default:               return ANSI_BLU;
    }
}

static uint16_t AppendStr(char *buf, uint16_t pos, const char *s)
{
    while (*s != '\0')
    {
        buf[pos++] = *s++;
    }
    return pos;
}

static uint16_t AppendHex8(char *buf, uint16_t pos, uint8_t v)
{
    static const char hx[] = "0123456789ABCDEF";
    buf[pos++] = '0'; buf[pos++] = 'x';
    buf[pos++] = hx[(v >> 4) & 0x0F];
    buf[pos++] = hx[v & 0x0F];
    return pos;
}

/* temp_x10 -> "21.0C" or "NA" */
static uint16_t AppendTempX10(char *buf, uint16_t pos, SensorStatus_t st_temp, int16_t temp_x10)
{
    if (st_temp != SENSOR_OK)
    {
        return AppendStr(buf, pos, "NA");
    }

    if (temp_x10 < 0)
    {
        buf[pos++] = '-';
        temp_x10 = (int16_t)(-temp_x10);
    }

    pos = AppendUIntToBuf(buf, pos, (uint32_t)(temp_x10 / 10));
    buf[pos++] = '.';
    buf[pos++] = (char)('0' + (char)(temp_x10 % 10));
    buf[pos++] = 'C';
    return pos;
}

/* One-line log:
   [ts=012340ms] TEMP | READ  | st=OK    | T=21.0C | age=0000ms | rc=OK    | cnt=0 | note=...
*/
static void LogTempLine(const char *evt,
                        SensorStatus_t st_disp,
                        SensorStatus_t rc,
                        SensorStatus_t st_temp,
                        int16_t temp_x10,
                        uint32_t age_ms,
                        uint16_t cnt,
                        const char *note)
{
    char line[160];
    uint16_t pos = 0U;

    pos = AppendStr(line, pos, Color_For(st_disp));
    pos = AppendStr(line, pos, "[ts=");
    pos = AppendUIntToBuf(line, pos, (uint32_t)HAL_GetTick());
    pos = AppendStr(line, pos, "ms] TEMP | ");

    /* EVT */
    pos = AppendStr(line, pos, evt);
    pos = AppendStr(line, pos, " | st=");
    pos = AppendStr(line, pos, SensorStatus_ToStr(st_disp));

    pos = AppendStr(line, pos, " | T=");
    pos = AppendTempX10(line, pos, st_temp, temp_x10);

    pos = AppendStr(line, pos, " | age=");
    pos = AppendUIntToBuf(line, pos, age_ms);
    pos = AppendStr(line, pos, "ms");

    pos = AppendStr(line, pos, " | rc=");
    pos = AppendStr(line, pos, SensorStatus_ToStr(rc));

    pos = AppendStr(line, pos, " | cnt=");
    pos = AppendUIntToBuf(line, pos, (uint32_t)cnt);

    if ((note != NULL) && (note[0] != '\0'))
    {
        pos = AppendStr(line, pos, " | ");
        pos = AppendStr(line, pos, note);
    }

    pos = AppendStr(line, pos, ANSI_RST);
    pos = AppendStr(line, pos, LOG_CRLF);

    (void)HAL_UART_Transmit(&huart3, (uint8_t*)line, pos, 50U);
}

static void Uart_TxBytes(const uint8_t *buf, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart3, (uint8_t*)buf, len, 50U);
}

static void Uart_TxString(const char *s)
{
    uint16_t len = 0U;
    while (s[len] != '\0') { len++; }
    Uart_TxBytes((const uint8_t*)s, len);
}




static void Uart_TxTempX10(int16_t temp_x10)
{
    char line[40];
    uint16_t pos = 0U;
    uint32_t abs_x10;
    uint32_t int_part;
    uint32_t frac_part;

    line[pos++] = 'T';
    line[pos++] = '=';

    if (temp_x10 < 0)
    {
        line[pos++] = '-';
        abs_x10 = (uint32_t)(-(int32_t)temp_x10);
    }
    else
    {
        abs_x10 = (uint32_t)temp_x10;
    }

    int_part  = abs_x10 / 10UL;
    frac_part = abs_x10 % 10UL;

    pos = AppendUIntToBuf(line, pos, int_part);
    line[pos++] = '.';
    line[pos++] = (char)('0' + (char)frac_part);
    line[pos++] = 'C';
    line[pos++] = '\r';
    line[pos++] = '\n';
    line[pos++] = '\0';

    Uart_TxString(line);
}

static void Uart_TxStatusLine(const char *tag, uint32_t val)
{
    char line[64];
    uint16_t pos = 0U;

    while (*tag != '\0') { line[pos++] = *tag++; }
    line[pos++] = '=';
    pos = AppendUIntToBuf(line, pos, val);
    line[pos++] = '\r';
    line[pos++] = '\n';
    line[pos++] = '\0';

    Uart_TxString(line);
}

void Scheduler_Task(void)
{
    SensorStatus_t st_lat  = SENSOR_OK;
    SensorStatus_t st_last = SENSOR_OK;
    SensorStatus_t st_temp = SENSOR_OK;

    int16_t  t_x10 = 0;
    uint16_t age_ticks = 0U;
    uint16_t cnt = 0U;

    (void)Sensor_Tick();

    (void)Sensor_GetLatchedStatus(&st_lat);
    (void)Sensor_GetLastStatus(&st_last);
    (void)Sensor_GetAgeTicks(&age_ticks);
    (void)Sensor_GetFaultCount(&cnt);

    const uint32_t age_ms = (uint32_t)age_ticks * 10U;

    static uint8_t was_fault = 0U;
    static uint8_t was_stale = 0U;
    static uint8_t init_logged = 0U;

    /* NEW: remember last logged ERR to avoid spam */
    static SensorStatus_t last_logged_err = SENSOR_OK;

    if (init_logged == 0U)
    {
        init_logged = 1U;
        LogTempLine("INIT ", SENSOR_ERR_INIT, SENSOR_OK, SENSOR_ERR_INIT, 0, age_ms, cnt, "addr=0x18");
    }

    if (st_lat != SENSOR_OK)
    {
        if (was_fault == 0U)
        {
            was_fault = 1U;
            LogTempLine("FAULT", SENSOR_ERR_I2C, st_lat, SENSOR_ERR_INIT, 0, age_ms, cnt, "latched=1");
        }

        /* Optional: reset ERR latch when entering FAULT */
        last_logged_err = SENSOR_OK;
        return;
    }
    was_fault = 0U;

    st_temp = Sensor_GetLastTemperature_x10(&t_x10);

    if (st_temp == SENSOR_OK)
    {
        /* returning to OK -> reset err latch */
        last_logged_err = SENSOR_OK;

        if (age_ticks == 0U)
        {
            g_last_temp_x10 = t_x10;
            LogTempLine("READ ", SENSOR_OK, SENSOR_OK, SENSOR_OK, t_x10, 0U, cnt, "");
        }
        was_stale = 0U;
        return;
    }

    if (st_temp == SENSOR_ERR_STALE)
    {
        /* returning to STALE -> reset err latch */
        last_logged_err = SENSOR_OK;

        if (was_stale == 0U)
        {
            was_stale = 1U;
            LogTempLine("STALE", SENSOR_ERR_STALE, SENSOR_OK, SENSOR_OK, t_x10, age_ms, cnt, "");
        }
        return;
    }

    if (st_temp == SENSOR_ERR_INIT)
    {
        /* returning to INIT -> reset err latch */
        last_logged_err = SENSOR_OK;
        return;
    }

    /* Autres erreurs non latched : log uniquement si changement */
    if (st_temp != last_logged_err)
    {
        last_logged_err = st_temp;
        LogTempLine("ERR  ", st_temp, st_temp, st_temp, 0, age_ms, cnt, "");
    }
}




/* Use SysTick (1ms) as scheduler tick source */
void HAL_SYSTICK_Callback(void)
{
    Scheduler_OnTick();
}

/* USER CODE END 0 */

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART3_UART_Init();

  /* USER CODE BEGIN 2 */
  (void)Scheduler_Init(10U); /* 10ms task period (SysTick=1ms) */

  if (Sensor_Init(0x18U) != SENSOR_OK)
  {
      Uart_TxString("Sensor init FAIL\r\n");
  }
  else
  {
      Uart_TxString("Sensor init OK\r\n");
  }

  while (1)
  {
      Scheduler_Process();

      /* démo: petite respiration CPU */
      HAL_Delay(1U);
  }
  /* USER CODE END 2 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
