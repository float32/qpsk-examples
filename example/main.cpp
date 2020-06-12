#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_adc.h"

constexpr uint32_t kRedLedPin    = GPIO_PIN_14;
constexpr uint32_t kGreenLedPin  = GPIO_PIN_12;
constexpr uint32_t kOrangeLedPin = GPIO_PIN_13;
constexpr uint32_t kBlueLedPin   = GPIO_PIN_15;

#ifdef USE_FULL_ASSERT
extern "C"
void assert_failed(
    [[maybe_unused]] uint8_t* file,
    [[maybe_unused]] uint32_t line)
{
    for (;;)
    {
        LL_GPIO_TogglePin(GPIOD, kRedLedPin);
        HAL_Delay(100);
    }
}
#endif

void SysTick_Handler(void)
{
    HAL_IncTick();
}

void ADC_IRQHandler(void)
{
    LL_ADC_ClearFlag_EOCS(ADC1);
    LL_GPIO_TogglePin(GPIOD, kBlueLedPin);
}

void InitLEDs(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init =
    {
        .Pin       = kRedLedPin | kGreenLedPin | kOrangeLedPin | kBlueLedPin,
        .Mode      = GPIO_MODE_OUTPUT_PP,
        .Pull      = GPIO_NOPULL,
        .Speed     = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };

    HAL_GPIO_Init(GPIOD, &gpio_init);
}

void InitPowerAndClock(void)
{
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    __HAL_RCC_PWR_CLK_ENABLE();

    RCC_OscInitTypeDef osc_init =
    {
        .OscillatorType      = RCC_OSCILLATORTYPE_HSE,
        .HSEState            = RCC_HSE_ON,
        .LSEState            = RCC_LSE_OFF,
        .HSIState            = RCC_HSI_OFF,
        .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
        .LSIState            = RCC_LSI_OFF,
        .PLL =
        {
            .PLLState  = RCC_PLL_ON,
            .PLLSource = RCC_PLLSOURCE_HSE,
            .PLLM      = 4,
            .PLLN      = 168,
            .PLLP      = RCC_PLLP_DIV2,
            .PLLQ      = 7,
        },
    };

    assert_param(HAL_OK == HAL_RCC_OscConfig(&osc_init));

    RCC_ClkInitTypeDef clk_init =
    {
        .ClockType      = RCC_CLOCKTYPE_SYSCLK |
                          RCC_CLOCKTYPE_HCLK |
                          RCC_CLOCKTYPE_PCLK1 |
                          RCC_CLOCKTYPE_PCLK2,
        .SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK,
        .AHBCLKDivider  = RCC_SYSCLK_DIV1,
        .APB1CLKDivider = RCC_HCLK_DIV4,
        .APB2CLKDivider = RCC_HCLK_DIV2,
    };

    assert_param(HAL_OK == HAL_RCC_ClockConfig(&clk_init, FLASH_LATENCY_5));
}

void InitTimer(void)
{
    __HAL_RCC_TIM2_CLK_ENABLE();

    LL_TIM_InitTypeDef timer_init =
    {
        .Prescaler         = 0,
        .CounterMode       = LL_TIM_COUNTERMODE_DOWN,
        .Autoreload        = HAL_RCC_GetPCLK1Freq() * 2 / 48000 - 1,
        .ClockDivision     = LL_TIM_CLOCKDIVISION_DIV1,
        .RepetitionCounter = 0,
    };

    LL_TIM_Init(TIM2, &timer_init);
    LL_TIM_SetTriggerOutput(TIM2, LL_TIM_TRGO_UPDATE);
    LL_TIM_EnableCounter(TIM2);
}

void InitADC(void)
{
    // PC4 = ADC12_IN14
    __HAL_RCC_GPIOC_CLK_ENABLE();
    GPIO_InitTypeDef gpio_init =
    {
        .Pin       = GPIO_PIN_4,
        .Mode      = GPIO_MODE_ANALOG,
        .Pull      = GPIO_NOPULL,
        .Speed     = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };
    HAL_GPIO_Init(GPIOC, &gpio_init);

    __HAL_RCC_ADC1_CLK_ENABLE();

    LL_ADC_CommonInitTypeDef adc_common_init;
    LL_ADC_CommonStructInit(&adc_common_init);
    assert_param(SUCCESS == LL_ADC_CommonInit(ADC123_COMMON, &adc_common_init));

    LL_ADC_InitTypeDef adc_init =
    {
        .Resolution         = LL_ADC_RESOLUTION_12B,
        .DataAlignment      = LL_ADC_DATA_ALIGN_RIGHT,
        .SequencersScanMode = LL_ADC_SEQ_SCAN_DISABLE,
    };
    assert_param(SUCCESS == LL_ADC_Init(ADC1, &adc_init));

    LL_ADC_REG_InitTypeDef adc_reg_init =
    {
        .TriggerSource    = LL_ADC_REG_TRIG_EXT_TIM2_TRGO,
        // .TriggerSource    = LL_ADC_REG_TRIG_SOFTWARE,
        .SequencerLength  = LL_ADC_REG_SEQ_SCAN_DISABLE,
        .SequencerDiscont = LL_ADC_REG_SEQ_DISCONT_DISABLE,
        .ContinuousMode   = LL_ADC_REG_CONV_SINGLE,
        .DMATransfer      = LL_ADC_REG_DMA_TRANSFER_NONE,
    };
    assert_param(SUCCESS == LL_ADC_REG_Init(ADC1, &adc_reg_init));
    LL_ADC_REG_SetSequencerRanks(ADC1, LL_ADC_REG_RANK_1, LL_ADC_CHANNEL_14);
    LL_ADC_SetChannelSamplingTime(ADC1, LL_ADC_CHANNEL_14,
        LL_ADC_SAMPLINGTIME_144CYCLES);
    LL_ADC_REG_StartConversionExtTrig(ADC1, LL_ADC_REG_TRIG_EXT_RISING);
    LL_ADC_Enable(ADC1);
    LL_ADC_EnableIT_EOCS(ADC1);

    HAL_NVIC_SetPriority(ADC_IRQn, 3, 0);
    HAL_NVIC_EnableIRQ(ADC_IRQn);
}

int main(void)
{
    assert_param(HAL_OK == HAL_Init());
    InitLEDs();
    InitPowerAndClock();
    InitTimer();
    InitADC();
    __enable_irq();

    uint32_t then = HAL_GetTick();

    for (;;)
    {
        uint32_t now = HAL_GetTick();

        if (now - then >= 125)
        {
            LL_GPIO_TogglePin(GPIOD, kGreenLedPin);
            then = now;
        }
    }

    return 0;
}
