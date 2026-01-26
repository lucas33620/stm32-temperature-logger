/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Temperature test (LM75 -> UART)
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "scheduler.h"
#include "sensor_temp.h"
#include "stm32f4xx_hal.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static volatile int16_t g_last_temp_x10 = 0; /* watch variable (temp *10) */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void Uart_TxBytes(const uint8_t *buf, uint16_t len)
{
    (void)HAL_UART_Transmit(&huart3, (uint8_t*)buf, len, 50U);
}

static void Uart_TxString(const char *s)
{
    uint16_t len = 0U;
    while (s[len] != '\0')
    {
        len++;
    }
    Uart_TxBytes((const uint8_t*)s, len);
}

static uint16_t AppendUIntToBuf(char *buf, uint16_t pos, uint32_t val)
{
    char tmp[10];
    uint16_t i = 0U;
    uint16_t j;

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

    j = i;
    while (j > 0U)
    {
        j--;
        buf[pos++] = tmp[j];
    }

    return pos;
}

static void Uart_TxTempX10(int16_t temp_x10)
{
    char line[32];
    uint16_t pos = 0U;
    uint32_t abs_x10;
    uint32_t int_part;
    uint32_t frac_part;

    /* Prefix */
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

/* Override weak callback from scheduler.c */
void Scheduler_Task(void)
{
    float temp_c = 0.0F;
    SensorStatus_t s = Sensor_ReadTemperature(&temp_c);

    if (s == SENSOR_OK)
    {
        /* Convert float -> int16 temp_x10 with rounding */
        int32_t x10;
        if (temp_c >= 0.0F)
        {
            x10 = (int32_t)((temp_c * 10.0F) + 0.5F);
        }
        else
        {
            x10 = (int32_t)((temp_c * 10.0F) - 0.5F);
        }

        g_last_temp_x10 = (int16_t)x10; /* watch variable */
        Uart_TxTempX10(g_last_temp_x10);
    }
    else
    {
        Uart_TxString("SENSOR_ERR\r\n");
    }
}

/* Use SysTick (1ms) as scheduler tick source */
void HAL_SYSTICK_Callback(void)
{
    Scheduler_OnTick();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_I2C1_Init();
  MX_SPI1_Init();
  MX_TIM2_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  Scheduler_Init(200U);

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
      /* optionnel: petite pause pour éviter 100% CPU */
      HAL_Delay(1U);
  }

  /* USER CODE END 3 */
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
