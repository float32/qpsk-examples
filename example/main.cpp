// MIT License
//
// Copyright 2021 Tyler Coy
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "stm32f4xx_hal.h"
#include "stm32f4xx_ll_tim.h"
#include "stm32f4xx_ll_gpio.h"
#include "stm32f4xx_ll_adc.h"
#include "qpsk/decoder.h"

constexpr uint32_t kAppStartAddress = FLASH_BASE + BOOTLOADER_SIZE;

constexpr uint32_t kRedLEDPin    = GPIO_PIN_14;
constexpr uint32_t kGreenLEDPin  = GPIO_PIN_12;
constexpr uint32_t kOrangeLEDPin = GPIO_PIN_13;
constexpr uint32_t kBlueLEDPin   = GPIO_PIN_15;
constexpr uint32_t kProfilingPin = GPIO_PIN_11;
constexpr uint32_t kADCInterruptPin = GPIO_PIN_9;
constexpr uint32_t kSwitchPin    = GPIO_PIN_0;

static_assert(SAMPLE_RATE % SYMBOL_RATE == 0);
constexpr uint32_t kSampleRate = SAMPLE_RATE;
constexpr uint32_t kSymbolRate = SYMBOL_RATE;
constexpr uint32_t kPacketSize = PACKET_SIZE;
constexpr uint32_t kBlockSize = BLOCK_SIZE;
constexpr uint32_t kCRCSeed = CRC_SEED;

qpsk::Decoder<kSampleRate, kSymbolRate, kPacketSize, kBlockSize> decoder;

#ifdef USE_FULL_ASSERT
extern "C"
void assert_failed(
    [[maybe_unused]] uint8_t* file,
    [[maybe_unused]] uint32_t line)
{
    for (;;)
    {
        LL_GPIO_TogglePin(GPIOD, kRedLEDPin);
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
    LL_GPIO_SetOutputPin(GPIOD, kADCInterruptPin);

    // Get the sample from the ADC, convert to float, and pass it to the
    // decoder.
    int16_t data = LL_ADC_REG_ReadConversionData12(ADC1);
    float sample = (data - 0x800) / 2048.f;
    decoder.Push(sample);

    LL_GPIO_ResetOutputPin(GPIOD, kADCInterruptPin);
}

void InitOutputPins(void)
{
    __HAL_RCC_GPIOD_CLK_ENABLE();

    auto pins = kRedLEDPin | kGreenLEDPin | kOrangeLEDPin | kBlueLEDPin |
        kProfilingPin | kADCInterruptPin;

    GPIO_InitTypeDef gpio_init =
    {
        .Pin       = pins,
        .Mode      = GPIO_MODE_OUTPUT_PP,
        .Pull      = GPIO_NOPULL,
        .Speed     = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };

    HAL_GPIO_Init(GPIOD, &gpio_init);
}

void InitSwitch(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init =
    {
        .Pin       = kSwitchPin,
        .Mode      = GPIO_MODE_INPUT,
        .Pull      = GPIO_PULLDOWN,
        .Speed     = GPIO_SPEED_FREQ_LOW,
        .Alternate = 0,
    };

    HAL_GPIO_Init(GPIOA, &gpio_init);
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

        // Assuming HSE is 8MHz,
        // VCO clock is HSE * N/M = 8 MHz * 96/4 = 192 MHz
        // System clock is VCO / P = 192 MHz / 6 = 32 MHz
        // USB clock is VCO / Q = 192 MHz / 4 = 48 MHz
        .PLL =
        {
            .PLLState  = RCC_PLL_ON,
            .PLLSource = RCC_PLLSOURCE_HSE,
            .PLLM      = 4,
            .PLLN      = 96,
            .PLLP      = RCC_PLLP_DIV6,
            .PLLQ      = 4,
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
        .Autoreload        = HAL_RCC_GetPCLK1Freq() * 2 / kSampleRate - 1,
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

struct SectorInfo
{
    uint32_t sector_num;
    uint32_t address;
    uint32_t erase_time_ms;
};

const SectorInfo kSectors[] =
{
    { FLASH_SECTOR_0,  0x08000000,  500 },
    { FLASH_SECTOR_1,  0x08004000,  500 },
    { FLASH_SECTOR_2,  0x08008000,  500 },
    { FLASH_SECTOR_3,  0x0800C000,  500 },
    { FLASH_SECTOR_4,  0x08010000, 1100 },
    { FLASH_SECTOR_5,  0x08020000, 2000 },
    { FLASH_SECTOR_6,  0x08040000, 2000 },
    { FLASH_SECTOR_7,  0x08060000, 2000 },
    { FLASH_SECTOR_8,  0x08080000, 2000 },
    { FLASH_SECTOR_9,  0x080A0000, 2000 },
    { FLASH_SECTOR_10, 0x080C0000, 2000 },
    { FLASH_SECTOR_11, 0x080E0000, 2000 },
};

bool WriteBlock(uint32_t address, const uint32_t* data, bool dry_run)
{
    SectorInfo sector_info;
    bool do_erase = false;

    for (auto& sector : kSectors)
    {
        if (address == sector.address)
        {
            sector_info = sector;
            do_erase = true;
        }
    }

    if (dry_run)
    {
        if (do_erase)
        {
            HAL_Delay(sector_info.erase_time_ms);
        }

        HAL_Delay(410);
    }
    else
    {
        if (HAL_OK != HAL_FLASH_Unlock())
        {
            return false;
        }

        if (do_erase)
        {
            FLASH_Erase_Sector(sector_info.sector_num,
                FLASH_VOLTAGE_RANGE_3);
            FLASH_WaitForLastOperation(HAL_MAX_DELAY);
        }

        for (uint32_t i = 0; i < kBlockSize; i += 4)
        {
            if (HAL_OK != HAL_FLASH_Program(
                FLASH_TYPEPROGRAM_WORD, address + i, *data++))
            {
                return false;
            }
        }

        if (HAL_OK != HAL_FLASH_Lock())
        {
            return false;
        }
    }

    return true;
}

int main(void)
{
    assert_param(HAL_OK == HAL_Init());
    InitOutputPins();
    InitSwitch();
    InitPowerAndClock();
    InitTimer();
    InitADC();
    decoder.Init(kCRCSeed);
    __enable_irq();

    // Write to flash only if the button is held at power on. Otherwise just
    // do a dry run, simulating the flash writes with delays.
    bool dry_run = !HAL_GPIO_ReadPin(GPIOA, kSwitchPin);

    uint32_t block_address = kAppStartAddress;

    constexpr auto kPacketLED = kBlueLEDPin;
    constexpr auto kWriteLED = kOrangeLEDPin;
    constexpr auto kErrorLED = kRedLEDPin;
    constexpr auto kSuccessLED = kGreenLEDPin;

    for (;;)
    {
        // We actually don't need to wait here for samples to be available.
        // We only do so to make the profiling signal more informative.
        while (!decoder.samples_available());

        LL_GPIO_SetOutputPin(GPIOD, kProfilingPin);
        auto result = decoder.Process();
        LL_GPIO_ResetOutputPin(GPIOD, kProfilingPin);

        if (result == qpsk::RESULT_PACKET_COMPLETE)
        {
            LL_GPIO_TogglePin(GPIOD, kPacketLED);
        }
        else if (result == qpsk::RESULT_BLOCK_COMPLETE)
        {
            LL_GPIO_ResetOutputPin(GPIOD, kPacketLED);
            LL_GPIO_SetOutputPin(GPIOD, kWriteLED);

            if (!WriteBlock(block_address, decoder.block_data(), dry_run))
            {
                decoder.Abort();
            }

            block_address += kBlockSize;
            LL_GPIO_ResetOutputPin(GPIOD, kWriteLED);
        }
        else if (result == qpsk::RESULT_END)
        {
            for (;;)
            {
                LL_GPIO_TogglePin(GPIOD, kSuccessLED);
                HAL_Delay(100);
            }
        }
        else if (result == qpsk::RESULT_ERROR)
        {
            LL_GPIO_ResetOutputPin(GPIOD, kPacketLED);

            switch (decoder.error())
            {
                case qpsk::ERROR_SYNC:
                    LL_GPIO_SetOutputPin(GPIOD, kWriteLED);
                    break;
                case qpsk::ERROR_CRC:
                    LL_GPIO_SetOutputPin(GPIOD, kPacketLED);
                    break;
                case qpsk::ERROR_OVERFLOW:
                    LL_GPIO_SetOutputPin(GPIOD, kWriteLED);
                    LL_GPIO_SetOutputPin(GPIOD, kPacketLED);
                    break;
                default:
                    break;
            }

            while (!HAL_GPIO_ReadPin(GPIOA, kSwitchPin))
            {
                LL_GPIO_TogglePin(GPIOD, kErrorLED);
                HAL_Delay(100);
            }

            LL_GPIO_SetOutputPin(GPIOD, kErrorLED);
            while (HAL_GPIO_ReadPin(GPIOA, kSwitchPin));
            LL_GPIO_ResetOutputPin(GPIOD, kErrorLED);
            LL_GPIO_ResetOutputPin(GPIOD, kWriteLED);
            LL_GPIO_ResetOutputPin(GPIOD, kPacketLED);

            block_address = kAppStartAddress;
            decoder.Reset();
        }
    }

    return 0;
}
