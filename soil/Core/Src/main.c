/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "i2c.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp.h"
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
extern unsigned char timCount;

uint32_t last_report_time = 0;

extern unsigned char esp8266_buf[512];
extern volatile uint8 esp8266_rev_flag;
extern uint16 esp8266_rev_len;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t SenWaitForAck = 0;

extern volatile uint8  dxct511_rev_flag;
extern uint16          dxct511_rev_len;
extern unsigned char   dxct511_buf[1024];

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	if(huart == &huart2)
	{
		esp8266_rev_len = Size;
		esp8266_rev_flag = 1;
		HAL_UARTEx_ReceiveToIdle_DMA(&huart2, esp8266_buf, sizeof(esp8266_buf));
	}
	else if(huart == &huart3)
	{
		dxct511_rev_len = Size;
		dxct511_rev_flag = 1;
		HAL_UARTEx_ReceiveToIdle_DMA(&huart3, dxct511_buf, sizeof(dxct511_buf));
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	unsigned char *dataPtr = NULL;
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
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART3_UART_Init();
  /* USER CODE BEGIN 2 */
  scheduler_init();
  OLED_Init();
  OLED_Clear();
  Bun_init();
  soil_sensor_init();

  HAL_TIM_Base_Start_IT(&htim2);

  /*---- DX-CT511 4G模块初始化 ----*/
  OLED_Clear();
  my_oledprintf(0, "DX-CT511 Init...");
  DXCT511_Init();

  /*---- 首次 MQTT 连接 ----*/
connect:
  OLED_Clear();
  my_oledprintf(0, "Conn MQTT...");
  while(!DXCT511_MqttConnect())
  {
    HAL_Delay(2000);
  }
  DXCT511_MqttSubscribe("$sys/5S6Nv8BF3z/d1/thing/property/post/reply", 0);
  DXCT511_MqttSubscribe("$sys/5S6Nv8BF3z/d1/thing/property/set", 0);
  OLED_Clear();
  last_report_time = 0;
  SenWaitForAck    = 0;

  while (1)
  {
    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
      scheduler_run();

      /*-- MQTT数据接收（长超时确保收到ACK） --*/
      dataPtr = DXCT511_GetData(30);
      if(dataPtr != NULL)
        DXCT511_RevPro(dataPtr);

      /*-- 传感器数据上报 --*/
      if(timCount == 1)
      {
        uint32_t current_time = HAL_GetTick();
        uint8  force_upload = (Bun_Flag != 0);
        uint32 interval = force_upload ? 200 : 2000;

        if(current_time - last_report_time >= interval)
        {
          DXCT511_SendSensorData();

          if(force_upload) Bun_Flag = 0;

          if(SenWaitForAck >= 5)  // 连续5次上发无ACK → 断线重连
          {
            SenWaitForAck = 0;
            break;
          }

          last_report_time = current_time;
        }
        timCount = 0;
      }
    }

    /*-- 重连 --*/
    DXCT511_MqttClose();
    HAL_Delay(500);
    goto connect;
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	if(htim->Instance == TIM2)
	{
		timCount = 1;
	}
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
