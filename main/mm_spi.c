#include "driver/spi_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "mm_spi.h"
#include "driver/gpio.h"
#include "hy3131_sw.h"

static const char *TAG = "HY3131_SPI";

// Global variables
MCUSTATUS   MCUSTATUSbits;
DMMSTATUS   DMMSTATUSbits;
TIME_BASE   TSTATUS;
KEY_STATUS  KSTATUS;
TYPE_flag   flag;

unsigned int TimerCount0 = 0;
unsigned int settling_time = 0;
unsigned int TimerCount = 0;
unsigned int TB_31P25ms = 0;
unsigned int Timer1sCnt = 0;
unsigned int Timer2sCnt = 0;
unsigned int T1s_Cap_Cnt = 0;

unsigned int TimerBCount = 0;
unsigned char dummyread = 0;
//unsigned int rxIntHappened0;

TYPE_ADC_DATA i32_ad1Out = {0};
TYPE_ADC_DATA i32_rmsVal = {0};

long i32_temp = 0;
long i32_temp1 = 0;
long i32_temp2 = 0;
long i32_temp3 = 0;
long i32_temp4 = 0;
long i32_temp5 = 0;
long iAvgAD1 = 0;
long i32_b_Hz = 0;
long i32_b_Volt = 0;
long i32_Last_Hz1 = 0;
long i32_Hzavg = 0;
int iAvgRMS = 0;
int Last_Hz[8] = {0};
int temp_Hz_Cnt = 0;
int Hz_ZERO_Cnt = 0;
int CAP_discharge_Cnt = 0;
int CAP_charge_Cnt = 0;
int temp_flag1 = 0;
uint32_t temp_Cnt = 0;
uint32_t Volt_6VOffset = 0;
uint32_t Volt_AC6VOffset = 0;
uint32_t temp_Cnt0 = 0;
long temp_sum0 = 0;
long temp_sum1 = 0;
int8_t Bar_NumBuff = 0;
int8_t Bar_NumBuff1 = 0;
int8_t MINUS = 0;
int8_t key_temp = 0;
int8_t last_keyTemp = 0;
int8_t i32_key_temp = 0;
int8_t pressKey = 0;
int8_t releaseKey = 0;
int8_t longKey = 0;
uint8_t u8_cnt = 0;
uint8_t u8_keyCnt = 0;
uint32_t getkey_temp = 0;
int8_t RangeCode1 = 0;
int8_t RangeCode = 0;
int8_t FunCode = 0;
int8_t Func_temp = 0;
int8_t NowFunc_temp = 0;
int8_t LastFunc_temp = 0;
uint8_t u8_FuncCnt = 0;
long RMSBUFF05 = 0;
signed long i16_mainVal = 0;
unsigned int AD1Data_000 = 0;
uint8_t SPI_Data[64] = {0};
uint8_t SPI_DataOut[3] = {0};
int32_t ADCData = 0;
long Cal_AD1_temp = 0;
int cal_stepNum = 0;
int32_t AD1DataBuffer = 0;
int32_t AD2DataBuffer = 0;
int32_t AD3DataBuffer = 0;
int32_t LPF2DataBuffer = 0;
int32_t LPF3DataBuffer = 0;
int32_t RMS2DataBufferLo = 0;
int32_t RMS2DataBufferHi = 0;
int32_t RMS3DataBufferLo = 0;
int32_t RMS3DataBufferHi = 0;
int32_t PeakMinDataBuffer = 0;
int32_t PeakMaxDataBuffer = 0;
long RMS2DataBuffer = 0;
double Count_A = 0;
double Count_B = 0;
double Count_C = 0;
int8_t R20Buffer = 0;
int8_t R29Buffer = 0;
int8_t R34Buffer = 0;
int8_t INTFBuffer = 0;
int32_t FreqBuff2 = 0;
double FreqBuffer = 0;
double DutyBuffer = 0;
double PeriodBuffer = 0;
double PeriodBuffer1 = 0;
double PeriodBuffer2 = 0;
double FreqBuff1 = 0;
int32_t DutyBuffer1 = 0;
double TotalTime = 0;
uint8_t BeepSTATUS = 0;
uint8_t A14Buffer = 0;

// HY3131 DC voltage measurement configuration table (example, adjust according to datasheet and application)
const unsigned char DCVolt_Setting[1][20] = {
    {
        0x01, 0x00, 0x20, 0x1F, 0x10, 0x02, 0x01, 0x00, 0x00, 0x40,
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
// HY3131 AC voltage measurement configuration table (example)
const unsigned char ACVolt_Setting[1][20] = {
    {
        0x02, 0x00, 0x21, 0x1F, 0x11, 0x03, 0x02, 0x00, 0x00, 0x41,
        0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char DCmVolt_Setting[1][20] = {
    {
        0x03, 0x00, 0x22, 0x1F, 0x12, 0x04, 0x03, 0x00, 0x00, 0x42,
        0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char ACmVolt_Setting[1][20] = {
    {
        0x04, 0x00, 0x23, 0x1F, 0x13, 0x05, 0x04, 0x00, 0x00, 0x43,
        0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char Diode_Setting[1][20] = {
    {
        0x05, 0x00, 0x24, 0x1F, 0x14, 0x06, 0x05, 0x00, 0x00, 0x44,
        0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char Temp_Setting[1][20] = {
    {
        0x06, 0x00, 0x25, 0x1F, 0x15, 0x07, 0x06, 0x00, 0x00, 0x45,
        0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char OHMCV_Setting[1][20] = {
    {
        0x07, 0x00, 0x26, 0x1F, 0x16, 0x08, 0x07, 0x00, 0x00, 0x46,
        0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char OHMCC_Setting[1][20] = {
    {
        0x08, 0x00, 0x27, 0x1F, 0x17, 0x09, 0x08, 0x00, 0x00, 0x47,
        0x08, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char CapCV_Setting[1][20] = {
    {
        0x09, 0x00, 0x28, 0x1F, 0x18, 0x0A, 0x09, 0x00, 0x00, 0x48,
        0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char CapCC_Setting[1][20] = {
    {
        0x0A, 0x00, 0x29, 0x1F, 0x19, 0x0B, 0x0A, 0x00, 0x00, 0x49,
        0x0A, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char Freq_Setting[1][20] = {
    {
        0x0B, 0x00, 0x2A, 0x1F, 0x1A, 0x0C, 0x0B, 0x00, 0x00, 0x4A,
        0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char OHM_Setting[1][20] = {
    {
        0x0C, 0x00, 0x2B, 0x1F, 0x1B, 0x0D, 0x0C, 0x00, 0x00, 0x4B,
        0x0C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char DCmA_Setting[1][20] = {
    {
        0x0D, 0x00, 0x2C, 0x1F, 0x1C, 0x0E, 0x0D, 0x00, 0x00, 0x4C,
        0x0D, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char DCA_Setting[1][20] = {
    {
        0x0E, 0x00, 0x2D, 0x1F, 0x1D, 0x0F, 0x0E, 0x00, 0x00, 0x4D,
        0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char ACmA_Setting[1][20] = {
    {
        0x0F, 0x00, 0x2E, 0x1F, 0x1E, 0x10, 0x0F, 0x00, 0x00, 0x4E,
        0x0F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char Continuity_Setting[1][20] = {
    {
        0x10, 0x00, 0x2F, 0x1F, 0x1F, 0x11, 0x10, 0x00, 0x00, 0x4F,
        0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};
const unsigned char Cap_Setting[1][20] = {
    {
        0x11, 0x00, 0x30, 0x1F, 0x20, 0x12, 0x11, 0x00, 0x00, 0x50,
        0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    }
};

void hy3131_spi_init(void) {
    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 1 * 1000 * 1000, // 1 MHz
        .mode = 0,                         // SPI mode 0
        .spics_io_num = PIN_NUM_CS,        // CS pin
        .queue_size = 7,                   // Transaction queue size
    };

    // Initialize the SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(HSPI_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // Attach the HY3131 to the SPI bus
    spi_device_handle_t hy3131_handle;
    ESP_ERROR_CHECK(spi_bus_add_device(HSPI_HOST, &devcfg, &hy3131_handle));

    ESP_LOGI(TAG, "HY3131 SPI initialized");

    // Create a task to handle HY3131 communication
    xTaskCreate(hy3131_task, "HY3131 Task", 2048, NULL, 5, NULL);
}

void SPI_3131_Read(spi_device_handle_t hy3131_handle) {
    uint8_t SPI_Data[52];
    uint8_t SPI_DataOut[1];
    uint32_t INTFBuffer, R20Buffer, R29Buffer, Buffer;
    uint32_t AD1DataBuffer, AD2DataBuffer, LPF2DataBuffer;
    uint32_t RMS2DataBufferHi, RMS2DataBufferLo;
    uint32_t PeakMaxDataBuffer, PeakMinDataBuffer;
    uint32_t Count_A, Count_B, Count_C;
    uint32_t TotalTime, FreqBuffer, PeriodBuffer, DutyBuffer;

    // Check if Data Ready Output is high
    if (gpio_get_level(PIN_NUM_MISO)) {
        // Read all register data
        spi_transaction_t t = {
            .length = 52 * 8, // 52 bytes
            .rx_buffer = SPI_Data
        };
        ESP_ERROR_CHECK(spi_device_transmit(hy3131_handle, &t));

        // Clear all event flags
        SPI_DataOut[0] = 0x00;
        t.length = 8; // 1 byte
        t.tx_buffer = SPI_DataOut;
        ESP_ERROR_CHECK(spi_device_transmit(hy3131_handle, &t));

        INTFBuffer = SPI_Data[HY3131_INTF] & SPI_Data[HY3131_INTE];
        R20Buffer = SPI_Data[HY3131_R20];
        R29Buffer = SPI_Data[HY3131_R29];
        Buffer = SPI_Data[HY3131_CTSTA];

        if ((INTFBuffer & AD1F) == AD1F) {
            AD1DataBuffer = (SPI_Data[HY3131_AD1_DATA2] << 16) |
                            (SPI_Data[HY3131_AD1_DATA1] << 8) |
                            SPI_Data[HY3131_AD1_DATA0];
            AD1DataBuffer >>= 7;
            ESP_LOGI(TAG, "AD1 Data: %d", AD1DataBuffer);
        }

        if ((INTFBuffer & AD2F) == AD2F) {
            AD2DataBuffer = (SPI_Data[HY3131_AD2_DATA2] << 16) |
                            (SPI_Data[HY3131_AD2_DATA1] << 8) |
                            SPI_Data[HY3131_AD2_DATA0];
            ESP_LOGI(TAG, "AD2 Data: %d", AD2DataBuffer);
        }

        if ((INTFBuffer & LPFF) == LPFF) {
            LPF2DataBuffer = (SPI_Data[HY3131_LPF_DATA2] << 16) |
                             (SPI_Data[HY3131_LPF_DATA1] << 8) |
                             SPI_Data[HY3131_LPF_DATA0];
            ESP_LOGI(TAG, "LPF Data: %d", LPF2DataBuffer);
        }

        if ((INTFBuffer & RMSF) == RMSF) {
            RMS2DataBufferHi = (SPI_Data[HY3131_RMS_DATA4] << 24) |
                               (SPI_Data[HY3131_RMS_DATA3] << 16) |
                               (SPI_Data[HY3131_RMS_DATA2] << 8) |
                               SPI_Data[HY3131_RMS_DATA1];
            RMS2DataBufferLo = SPI_Data[HY3131_RMS_DATA0];
            ESP_LOGI(TAG, "RMS Data Hi: %d, Lo: %d", RMS2DataBufferHi, RMS2DataBufferLo);
        }

        if ((R29Buffer & ENPKH) == ENPKH) {
            PeakMaxDataBuffer = (SPI_Data[HY3131_PKHMAX2] << 16) |
                                (SPI_Data[HY3131_PKHMAX1] << 8) |
                                SPI_Data[HY3131_PKHMAX0];
            PeakMinDataBuffer = (SPI_Data[HY3131_PKHMIN2] << 16) |
                                (SPI_Data[HY3131_PKHMIN1] << 8) |
                                SPI_Data[HY3131_PKHMIN0];
            ESP_LOGI(TAG, "Peak Max: %d, Min: %d", PeakMaxDataBuffer, PeakMinDataBuffer);
        }

        if ((INTFBuffer & CTF) == CTF) {
            Count_A = (SPI_Data[HY3131_CTA2] << 16) |
                      (SPI_Data[HY3131_CTA1] << 8) |
                      SPI_Data[HY3131_CTA0];
            Count_B = (SPI_Data[HY3131_CTB2] << 16) |
                      (SPI_Data[HY3131_CTB1] << 8) |
                      SPI_Data[HY3131_CTB0];
            Count_C = (SPI_Data[HY3131_CTC2] << 16) |
                      (SPI_Data[HY3131_CTC1] << 8) |
                      SPI_Data[HY3131_CTC0];

            TotalTime = Count_A + CTA_Preset;
            FreqBuffer = (Count_B * 491520) / (TotalTime / 1000);
            PeriodBuffer = (TotalTime * 1000000) / (Count_B * 49152);
            DutyBuffer = (Count_C * 10) / (TotalTime / 1000);

            ESP_LOGI(TAG, "Frequency: %d, Period: %d, Duty: %d", FreqBuffer, PeriodBuffer, DutyBuffer);

            SPI_Data[0] = R20Buffer & (~ENCTR);
            t.tx_buffer = SPI_Data;
            t.length = 8;
            ESP_ERROR_CHECK(spi_device_transmit(hy3131_handle, &t));
        }
    }
}

void hy3131_task(void *pvParameters) {
    spi_device_handle_t hy3131_handle = (spi_device_handle_t)pvParameters;
    while (1) {
        SPI_3131_Read(hy3131_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}



// 鐩存祦鐢靛帇ADC鏁版嵁澶勭悊鍑芥暟
void AD1_DCV_PROC(int32_t AD1Data_buff) {
    if ((NowFunc_temp != Func_DCV) || (FunCode != E_DCVolt)) return;
    if (flag.b_unStable_state == 1) return;
    if ((AD1Data_buff < 0) || (AD1Data_buff >= 0x10000)) {
        AD1Data_buff = AD1Data_buff - Volt_6VOffset;
        AD1Data_buff = ~AD1Data_buff;
        AD1Data_buff++;
        AD1Data_buff = AD1Data_buff & 0x0001FFFF;
        MINUS = 1;
        if (AD1Data_buff >= 0x10000) {
            AD1Data_buff = ~AD1Data_buff;
            AD1Data_buff++;
            AD1Data_buff = AD1Data_buff & 0x0000FFFF;
            MINUS = 0;
        }
    } else {
        AD1Data_buff = AD1Data_buff - Volt_6VOffset;
        MINUS = 0;
        if (AD1Data_buff < 0) {
            AD1Data_buff = 0;
        }
    }
    AD1Data_buff = (AD1Data_buff) * 10000 / 9472;
    AD1Data_buff = AD1Data_buff / 10;
    Bar_NumBuff = AD1Data_buff / 200;
    Bar_NumBuff1 = AD1Data_buff % 200;
    i32_temp = AD1Data_buff;
}

// Get the zero offset for 6V DC voltage range
void Get_Volt6V_Zero(spi_device_handle_t hy3131_handle) {
    RangeCode = 5;
    SwitchDMMFun();
    int temp_Cnt = 50;
    while (temp_Cnt) {
        asm volatile ("nop");
        SPI_3131_Read(hy3131_handle);
        vTaskDelay(pdMS_TO_TICKS(10)); // Use FreeRTOS delay instead of busy wait
        temp_Cnt--;
    }
    Volt_6VOffset = AD1DataBuffer;
    flag.b_minus = 0;
    if (Volt_6VOffset >= 0x10000) {
        Volt_6VOffset = ~Volt_6VOffset;
        Volt_6VOffset++;
        Volt_6VOffset = Volt_6VOffset & 0x0000FFFF;
    } else {
        flag.b_minus = 1;
    }
}

// DC voltage measurement initialization
void DCV_initial(spi_device_handle_t hy3131_handle) {
    TimerCount = 0;
    settling_time = C_125ms;
    RangeCode = 0;
    Bar_NumBuff = 0;
    Bar_NumBuff1 = 0;
    FunCode = E_DCVolt;
    flag.b_Auto_Status = 1;
    KSTATUS.b_AutoEn = 1;
    KSTATUS.b_longKey = 0;
    KSTATUS.b_longKeyEn = 0;
    MCUSTATUSbits._byte = 0;
    DMMSTATUSbits._byte = 0;
    flag.b_ad1Ok = 0;
    temp_Cnt0 = 0;
    Get_Volt6V_Zero(hy3131_handle);
    RangeCode = 0;
    SwitchDMMFun();
}

void SwitchDMMFun(void)
{
    switch(FunCode)
    {
        case E_DCVolt:
            HY3131_ADC_Initial(DCVolt_Setting, RangeCode, DCIRQ);
            break;
        case E_ACVolt:
            HY3131_ADC_Initial(ACVolt_Setting, RangeCode, ACIRQ);
            break;
        case E_DCmVolt:
            HY3131_ADC_Initial(DCmVolt_Setting, RangeCode, DCIRQ);
            break;
        case E_ACmVolt:
            HY3131_ADC_Initial(ACmVolt_Setting, RangeCode, ACIRQ);
            break;
        case E_Diode:
            HY3131_ADC_Initial(Diode_Setting, RangeCode, DCIRQ);
            break;
        case E_Temp:
            HY3131_ADC_Initial(Temp_Setting, RangeCode, DCIRQ);
            break;
        case E_OHMCV:
            HY3131_ADC_Initial(OHMCV_Setting, RangeCode, OHMIRQ);
            break;
        case E_OHMCC:
            HY3131_ADC_Initial(OHMCC_Setting, RangeCode, OHMIRQ);
            break;
        case E_CapCV:
            HY3131_ADC_Initial(CapCV_Setting, RangeCode, FreqIRQ);
            break;
        case E_CapCC:
            HY3131_ADC_Initial(CapCC_Setting, RangeCode, FreqIRQ);
            break;
        case E_Freq:
            if (RangeCode == 0) {
                HY3131_ADC_Initial(Freq_Setting, RangeCode, FreqIRQ);
            } else if (RangeCode == 1) {
                HY3131_ADC_Initial(Freq_Setting, RangeCode, (ACIRQ + FreqIRQ));
            } else if (RangeCode == 2) {
                HY3131_ADC_Initial(Freq_Setting, RangeCode, FreqIRQ);
            }
            break;
        case E_OHM:
            HY3131_ADC_Initial(OHM_Setting, RangeCode, OHMIRQ);
            break;
        case E_DCuA:
        case E_DCmA:
            HY3131_ADC_Initial(DCmA_Setting, RangeCode, DCIRQ);
            break;
        case E_DCA:
            HY3131_ADC_Initial(DCA_Setting, RangeCode, DCIRQ);
            break;
        case E_ACuA:
        case E_ACmA:
        case E_ACA:
            HY3131_ADC_Initial(ACmA_Setting, RangeCode, ACIRQ);
            break;
        case E_Continuity:
            HY3131_ADC_Initial(Continuity_Setting, RangeCode, OHMIRQ);
            break;
        case E_CAP:
            if ((RangeCode == 0) || (RangeCode == 1) || (RangeCode == 2)) {
                HY3131_ADC_Initial(Cap_Setting, RangeCode, FreqIRQ);
            } else if ((RangeCode == 3) || (RangeCode == 4)) {
                HY3131_ADC_Initial(Cap_Setting, RangeCode, DCIRQ);
            }
            break;
        default:
            HY3131_ADC_Initial(DCVolt_Setting, RangeCode, DCIRQ);
            break;
    }
}
