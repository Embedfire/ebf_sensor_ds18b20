/**
  ******************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2019-xx-xx
  * @brief   LTDC—液晶显示采集的温度
  ******************************************************************
  * @attention
  *
  * 实验平台:野火 STM32F767 开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :http://firestm32.taobao.com
  *
  ******************************************************************
  */  
  
#include "stm32f7xx.h"
#include "./led/bsp_led.h"
#include "./usart/bsp_debug_usart.h"
#include "./lcd/bsp_lcd.h"
#include <stdlib.h>
#include "main.h"
#include  "./ds18b20/bsp_ds18b20.h"


/* 显示缓冲区 */
uint8_t dis_buf[1024];
float temperature;

__IO uint8_t* qspi_addr = (__IO uint8_t*)(0x90000000);

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{   
	uint8_t uc,DS18B20Id[8];
	uint8_t DS18B20Id_str[20];
	
	/* 系统时钟初始化成236MHz */
	SystemClock_Config();
	
	/* 开启I-Cache */
	SCB_EnableICache();
	
	/* 开启D-Cache */
	SCB_EnableDCache();
	
	/* LED 端口初始化 */
	LED_GPIO_Config();
	
	/* 配置串口1为：115200 8-N-1 */
	DEBUG_USART_Config();
	
	/* LCD 端口初始化 */ 
	LCD_Init();
	
	/* LCD 第一层初始化 */ 
	LCD_LayerInit(0, LCD_FB_START_ADDRESS,RGB888);

	/* 使能LCD，包括开背光 */ 
	LCD_DisplayOn(); 

	/* 选择LCD第一层 */
	LCD_SelectLayer(0);

	/* 第一层清屏，显示全黑 */ 
	LCD_Clear(LCD_COLOR_BLACK);  

	/* 设置字体的颜色以及字体背景颜色(此处的背景不是指LCD的背景层,注意区分) */
	LCD_SetColors(LCD_COLOR_WHITE,LCD_COLOR_BLACK);
	
	/* 选择字体 */
	LCD_SetFont(&LCD_DEFAULT_FONT);
	
	LCD_DisplayStringLine_EN_CH(1,(uint8_t* )"DS18B20 temperature detect demo");
	/* 温度传感器初始化 */
		if(DS18B20_Init()==0)
	{
		printf("DS18B20初始化成功\n");
	}
	else
	{
		printf("DS18B20初始化失败\n");
		printf("请将传感器正确插入到插槽内\n");
		LCD_SetTextColor(LCD_COLOR_RED);
    LCD_DisplayStringLine_EN_CH(2,(uint8_t* )"DS18B20 initialization failed!");
    LCD_DisplayStringLine_EN_CH(3,(uint8_t* )"Please check the connection!");
		/* 停机 */
		while(1)
		{}			
	}		
  DS18B20_ReadId ( DS18B20Id  );           // 读取 DS18B20 的序列号	 
	for ( uc = 0; uc < 8; uc++ )             // 打印 DS18B20 的序列号
  {    
    sprintf((char *)&DS18B20Id_str[2*uc], "%.2x",DS18B20Id[uc]);
    if(uc == 7)
      DS18B20Id_str[17] = '\0';         
  }
  printf("\r\nDS18B20的序列号是： 0x%s\r\n",DS18B20Id_str);

  sprintf((char*)dis_buf,"DS18B20 serial num:0x%s",DS18B20Id_str);  
  LCD_DisplayStringLine_EN_CH(4,dis_buf);
  
	while(1)
	{		
		temperature=DS18B20_Get_Temp();
		printf("DS18B20读取到的温度为：%0.3f\n",temperature);   
    sprintf((char*)dis_buf,"Temperature:   %0.3f   degree Celsius",temperature);
    LCD_DisplayStringLine_EN_CH(7,(uint8_t* )dis_buf);
    HAL_Delay(1000); 
	}
}

/**
  * @brief  System Clock 配置
  *         system Clock 配置如下 : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 216000000
  *            HCLK(Hz)                       = 216000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 432
  *            PLL_P                          = 2
  *            PLL_Q                          = 9
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 7
  * @param  无
  * @retval 无
  */
void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	HAL_StatusTypeDef ret = HAL_OK;

	/* 使能HSE，配置HSE为PLL的时钟源，配置PLL的各种分频因子M N P Q 
	 * PLLCLK = HSE/M*N/P = 25M / 25 *432 / 2 = 216M
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_ON;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9;

	ret = HAL_RCC_OscConfig(&RCC_OscInitStruct);
	if(ret != HAL_OK)
	{
		while(1) { ; }
	}

	/* 激活 OverDrive 模式以达到216M频率  */  
	ret = HAL_PWREx_EnableOverDrive();
	if(ret != HAL_OK)
	{
		while(1) { ; }
	}

	/* 选择PLLCLK作为SYSCLK，并配置 HCLK, PCLK1 and PCLK2 的时钟分频因子 
	 * SYSCLK = PLLCLK     = 216M
	 * HCLK   = SYSCLK / 1 = 216M
	 * PCLK2  = SYSCLK / 2 = 108M
	 * PCLK1  = SYSCLK / 4 = 54M
	 */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2; 

	ret = HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7);
	if(ret != HAL_OK)
	{
		while(1) { ; }
	}  
}

/*********************************************END OF FILE**********************/

