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
    SensorStatus_t st_temp = SENSOR_OK;
    int16_t        t_x10   = 0;
    uint16_t       age     = 0U;

    (void)Sensor_Tick();

    (void)Sensor_GetLatchedStatus(&st_lat);
    (void)Sensor_GetAgeTicks(&age);

    /* --- FAULT latched : priorité absolue --- */
    static uint8_t fault_reported = 0U;
    if (st_lat != SENSOR_OK)
    {
        g_samples_fault++;

        if (fault_reported == 0U)
        {
            fault_reported = 1U;
            Uart_TxString("FAULT latched\r\n");
            Uart_TxStatusLine("latched", (uint32_t)st_lat);
        }
        return;
    }
    fault_reported = 0U;

    /* --- STALE / INVALID / OK : basé sur la donnée --- */
    st_temp = Sensor_GetLastTemperature_x10(&t_x10);

    static uint8_t stale_reported = 0U;

    if (st_temp == SENSOR_OK)
    {
        stale_reported = 0U;

        /* Nominal : n’afficher que si nouvelle mesure (age == 0) */
        if (age == 0U)
        {
            g_last_temp_x10 = t_x10;
            g_samples_ok++;
            Uart_TxTempX10(t_x10);
        }
        return;
    }

    /* Donnée STALE */
    if (st_temp == SENSOR_ERR_STALE)
    {
        g_samples_stale++;

        if (stale_reported == 0U)
        {
            stale_reported = 1U;
            Uart_TxString("STALE\r\n");
            Uart_TxStatusLine("age_ticks", (uint32_t)age);
        }
        return;
    }

    /* Autres cas (pas encore de donnée valide, init, etc.) */
    if (stale_reported == 0U)
    {
        stale_reported = 1U;
        Uart_TxString("TEMP_NOT_AVAILABLE\r\n");
        Uart_TxStatusLine("st_temp", (uint32_t)st_temp);
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
