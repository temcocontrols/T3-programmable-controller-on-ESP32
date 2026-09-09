#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "ES51932_Driver.h"
#include "FreeRTOS.h"
#include "task.h"
#include <string.h>

extern uint16_t test[];
extern uint8_t i2c_send[];

volatile DataPacket receivedData;
volatile uint8_t dataBuffer[14];
volatile uint8_t dataIndex = 0;
uint32_t combinedNumber = 0; // global variable
uint8_t resistanceBytes[4]; // Array to store resistance value as bytes
uint8_t receivedSerialData = 0; // Flag to indicate if serial data has been received

void parseStatus(uint8_t status) {
    uint8_t judge = status & 0x01; // Bit 0
    uint8_t sign = (status >> 1) & 0x01; // Bit 1
    uint8_t batt = (status >> 2) & 0x01; // Bit 2
    uint8_t ol = (status >> 3) & 0x01; // Bit 3

    if (judge == 1) {
        // Temperature unit: °C
        // printf("Temperature unit: °C\n");
    } else {
        // Temperature unit: °F
        // printf("Temperature unit: °F\n");
    }

    if (sign == 1) {
        // Negative sign: On
        // printf("Negative sign: On\n");
    } else {
        // Negative sign: Off
        // printf("Negative sign: Off\n");
    }

    if (batt == 1) {
        // Battery low: Yes
        // printf("Battery low: Yes\n");
    } else {
        // Battery low: No
        // printf("Battery low: No\n");
    }

    if (ol == 1) {
        // Input overflow: Yes
        // printf("Input overflow: Yes\n");
    } else {
        // Input overflow: No
        // printf("Input overflow: No\n");
    }
}

void parseOptions(uint8_t option1, uint8_t option2, uint8_t option3, uint8_t option4) {
    // Parse option1
    uint8_t max = option1 & 0x01; // Bit 0
    uint8_t min = (option1 >> 1) & 0x01; // Bit 1
    uint8_t rmr = (option1 >> 2) & 0x01; // Bit 2
    uint8_t rel = (option1 >> 3) & 0x01; // Bit 3

    if (max == 1) {
        // MAX function is active
        // printf("MAX function: Active\n");
    } else {
        // MAX function is not active
        // printf("MAX function: Inactive\n");
    }

    if (min == 1) {
        // MIN function is active
        // printf("MIN function: Active\n");
    } else {
        // MIN function is not active
        // printf("MIN function: Inactive\n");
    }

    if (rmr == 1) {
        // RMR function is active
        // printf("RMR function: Active\n");
    } else {
        // RMR function is not active
        // printf("RMR function: Inactive\n");
    }

    if (rel == 1) {
        // REL function is active
        // printf("REL function: Active\n");
    } else {
        // REL function is not active
        // printf("REL function: Inactive\n");
    }

    // Parse option2
    uint8_t ul = option2 & 0x01; // Bit 0
    uint8_t pmax = (option2 >> 1) & 0x01; // Bit 1
    uint8_t pmin = (option2 >> 2) & 0x01; // Bit 2

    if (ul == 1) {
        // UL condition is met
        // printf("UL condition: Met\n");
    } else {
        // UL condition is not met
        // printf("UL condition: Not met\n");
    }

    if (pmax == 1) {
        // PMAX is active
        // printf("PMAX: Active\n");
    } else {
        // PMAX is not active
        // printf("PMAX: Inactive\n");
    }

    if (pmin == 1) {
        // PMIN is active
        // printf("PMIN: Active\n");
    } else {
        // PMIN is not active
        // printf("PMIN: Inactive\n");
    }

    // Parse option3
    uint8_t dc = option3 & 0x01; // Bit 0
    uint8_t ac = (option3 >> 1) & 0x01; // Bit 1
    uint8_t auto_mode = (option3 >> 2) & 0x01; // Bit 2

    if (dc == 1) {
        // DC measurement mode
        // printf("DC measurement mode: Active\n");
    } else {
        // DC measurement mode is not active
        // printf("DC measurement mode: Inactive\n");
    }

    if (ac == 1) {
        // AC measurement mode
        // printf("AC measurement mode: Active\n");
    } else {
        // AC measurement mode is not active
        // printf("AC measurement mode: Inactive\n");
    }

    if (auto_mode == 1) {
        // Automatic mode
        // printf("Automatic mode: Active\n");
    } else {
        // Manual mode
        // printf("Manual mode: Active\n");
    }

    // Parse option4
    uint8_t vbar = option4 & 0x01; // Bit 0
    uint8_t hold = (option4 >> 1) & 0x01; // Bit 1
    uint8_t lpf0 = (option4 >> 2) & 0x01; // Bit 2
    uint8_t lpf1 = (option4 >> 3) & 0x01; // Bit 3

    if (vbar == 1) {
        // VBAR pin is connected to V-
        // printf("VBAR: Connected to V-\n");
    } else {
        // VBAR pin is not connected to V-
        // printf("VBAR: Not connected to V-\n");
    }

    if (hold == 1) {
        // Hold mode is active
        // printf("Hold mode: Active\n");
    } else {
        // Hold mode is not active
        // printf("Hold mode: Inactive\n");
    }

    if (lpf0 == 1 && lpf1 == 1) {
        // Low-pass filter feature is activated
        // printf("Low-pass filter: Activated\n");
    } else {
        // Low-pass filter feature is not activated
        // printf("Low-pass filter: Not activated\n");
    }
}

void USART_Configuration(void) {
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable USART3 and GPIO clocks
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // Configure USART3 Tx (PB10) as alternate function push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure USART3 Rx (PB11) as input floating
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure USART3
    USART_InitStructure.USART_BaudRate = ES51932_BAUDRATE;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_Odd;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
    USART_Init(USART3, &USART_InitStructure);
		
    // Enable USART3 Receive interrupt
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    // Configure and enable USART3 interrupt in NVIC
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // Enable USART3
    USART_Cmd(USART3, ENABLE);
}

void GPIO_Configuration(void) {
    GPIO_InitTypeDef GPIO_InitStructure;

    // Enable GPIO clocks
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);

    // Configure PB3, PB4, PB5, PB8, PB9, PB10, PB13, PB15 as output push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_13 | GPIO_Pin_15;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    // Configure PC0, PC2, PC6, PC7, PC8, PC9, PC10, PC11 as output push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    // Configure PA5, PA6, PA7, PA13 as output push-pull
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_13;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // Set initial states
    GPIO_ResetBits(GPIOB, GPIO_Pin_3); // PB3 low
    GPIO_SetBits(GPIOB, GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10 | GPIO_Pin_15); // Other PB pins high
    GPIO_SetBits(GPIOB, GPIO_Pin_13); // PB13 high
    GPIO_SetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_6 | GPIO_Pin_7 | GPIO_Pin_8 | GPIO_Pin_9); // PC0, PC2, PC6, PC7, PC8, PC9 high
    GPIO_ResetBits(GPIOC, GPIO_Pin_10 | GPIO_Pin_11); // PC10, PC11 low

    // Set initial states for PA pins
    GPIO_SetBits(GPIOA, GPIO_Pin_5); // PA5 (FC5) high
    GPIO_ResetBits(GPIOA, GPIO_Pin_13); // PA13 (RANGE) low
    //GPIO_SetBits(GPIOA, GPIO_Pin_12 | GPIO_Pin_11 | GPIO_Pin_7 | GPIO_Pin_6); // PA12 (FC1), PA11 (FC2), PA7 (FC3), PA6 (FC4) high
		
}

void ControlChannels(uint16_t value) {
    // Open all channels by setting the corresponding pins low
    if (value != 1) GPIO_ResetBits(GPIOB, GPIO_Pin_3); // Open CH1
    if (value != 2) GPIO_ResetBits(GPIOB, GPIO_Pin_4); // Open CH2
    if (value != 3) GPIO_ResetBits(GPIOB, GPIO_Pin_5); // Open CH3
    if (value != 4) GPIO_ResetBits(GPIOC, GPIO_Pin_6); // Open CH4
    if (value != 5) GPIO_ResetBits(GPIOC, GPIO_Pin_7); // Open CH5
    if (value != 6) GPIO_ResetBits(GPIOB, GPIO_Pin_8); // Open CH6
    if (value != 7) GPIO_ResetBits(GPIOB, GPIO_Pin_9); // Open CH7
    if (value != 8) GPIO_ResetBits(GPIOB, GPIO_Pin_10); // Open CH8

    i2c_send[8]++;
    // Close the specified channel by setting the corresponding pin high
    switch (value) {
        case 1:
            GPIO_SetBits(GPIOB, GPIO_Pin_3); // Close CH1
            break;
        case 2:
            GPIO_SetBits(GPIOB, GPIO_Pin_4); // Close CH2
            break;
        case 3:
            GPIO_SetBits(GPIOB, GPIO_Pin_5); // Close CH3
            break;
        case 4:
            GPIO_SetBits(GPIOC, GPIO_Pin_6); // Close CH4
            break;
        case 5:
            GPIO_SetBits(GPIOC, GPIO_Pin_7); // Close CH5
            break;
        case 6:
            GPIO_SetBits(GPIOB, GPIO_Pin_8); // Close CH6
            break;
        case 7:
            GPIO_SetBits(GPIOB, GPIO_Pin_9); // Close CH7
            break;
        case 8:
            GPIO_SetBits(GPIOB, GPIO_Pin_10); // Close CH8
            break;
        default:
            GPIO_SetBits(GPIOB, GPIO_Pin_3); // Default to Close CH1
            break;
    }
}

void SingleControlChannels(uint16_t ch1, uint16_t ch2, uint16_t ch3, uint16_t ch4, uint16_t ch5, uint16_t ch6, uint16_t ch7, uint16_t ch8) {
    // Control CH1
    if (ch1 == 1) {
        GPIO_SetBits(GPIOB, GPIO_Pin_3); // Close CH1
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_3); // Open CH1
    }

    // Control CH2
    if (ch2 == 1) {
        GPIO_SetBits(GPIOB, GPIO_Pin_4); // Close CH2
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_4); // Open CH2
    }

    // Control CH3
    if (ch3 == 1) {
        GPIO_SetBits(GPIOB, GPIO_Pin_5); // Close CH3
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_5); // Open CH3
    }

    // Control CH4
    if (ch4 == 1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_6); // Close CH4
    } else {
        GPIO_ResetBits(GPIOC, GPIO_Pin_6); // Open CH4
    }

    // Control CH5
    if (ch5 == 1) {
        GPIO_SetBits(GPIOC, GPIO_Pin_7); // Close CH5
    } else {
        GPIO_ResetBits(GPIOC, GPIO_Pin_7); // Open CH5
    }

    // Control CH6
    if (ch6 == 1) {
        GPIO_SetBits(GPIOB, GPIO_Pin_8); // Close CH6
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_8); // Open CH6
    }

    // Control CH7
    if (ch7 == 1) {
        GPIO_SetBits(GPIOB, GPIO_Pin_9); // Close CH7
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_9); // Open CH7
    }

    // Control CH8
    if (ch8 == 1) {
        GPIO_SetBits(GPIOB, GPIO_Pin_10); // Close CH8
    } else {
        GPIO_ResetBits(GPIOB, GPIO_Pin_10); // Open CH8
    }
}

uint8_t decodeDigit(uint8_t digit) {
    switch (digit) {
				case DIGIT_0: 
					return 0;
        case DIGIT_1: 
					return 1;
        case DIGIT_2: 
					return 2;
        case DIGIT_3: 
					return 3;
        case DIGIT_4: 
					return 4;
        case DIGIT_5: 
					return 5;
        case DIGIT_6: 
					return 6;
        case DIGIT_7: 
					return 7;
        case DIGIT_8: 
					return 8;
        case DIGIT_9: 
					return 9;
        default: 
					return 0xFF; // Invalid digit
    }
}

void ControlSwitches(uint16_t sw2, uint16_t sw3, uint16_t sw4, uint16_t sw5) {
    // Control SW2
    if (sw2 == 1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_9); // Turn on SW2 (low)
    } else {
        GPIO_SetBits(GPIOC, GPIO_Pin_9); // Turn off SW2 (high)
    }

    // Control SW3
    if (sw3 == 1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_10); // Turn on SW3 (low)
    } else {
        GPIO_SetBits(GPIOC, GPIO_Pin_10); // Turn off SW3 (high)
    }

    // Control SW4
    if (sw4 == 1) {
        GPIO_ResetBits(GPIOC, GPIO_Pin_11); // Turn on SW4 (low)
    } else {
        GPIO_SetBits(GPIOC, GPIO_Pin_11); // Turn off SW4 (high)
    }

    // Control SW5
    if (sw5 == 1) {
        GPIO_ResetBits(GPIOB, GPIO_Pin_15); // Turn on SW5 (low)
    } else {
        GPIO_SetBits(GPIOB, GPIO_Pin_15); // Turn off SW5 (high)
    }
}

extern uint8_t multiChannel;
static uint8_t sequenceNumber = 0;
void ProcessAndConvertResistance(uint8_t *buffer) {
//    uint32_t resistanceValue = 0;

//    if (receivedData.function == FUNCTION_RESISTANCE) {
//        switch (receivedData.range) {
//            case RANGE_220_00_OHM:
//                resistanceValue = combinedNumber * 100; // 220.00 Ohm range
//                break;
//            case RANGE_2_2000K_OHM:
//                resistanceValue = combinedNumber * 1000; // 2.2000K Ohm range
//                break;
//            case RANGE_22_000K_OHM:
//                resistanceValue = combinedNumber * 1000; // 22.0000K Ohm range
//                break;
//            case RANGE_220_00K_OHM:
//                resistanceValue = combinedNumber * 1000; // 220.00K Ohm range
//                break;
//            case RANGE_2_2000M_OHM:
//                resistanceValue = combinedNumber * 1000000; // 2.2000M Ohm range
//                break;
//            case RANGE_22_000M_OHM:
//                resistanceValue = combinedNumber * 1000000; // 22.000M Ohm range
//                break;
//            case RANGE_220_00M_OHM:
//                resistanceValue = combinedNumber * 1000000; // 220.00M Ohm range
//                break;
//            default:
//                // Handle unknown range
//                break;
//        }
//    }

//    // Convert resistance value to bytes and store in buffer
//    buffer[0] = (uint8_t)(resistanceValue >> 24);
//    buffer[1] = (uint8_t)(resistanceValue >> 16);
//    buffer[2] = (uint8_t)(resistanceValue >> 8);
//    buffer[3] = (uint8_t)resistanceValue;

//		// Store the resistance value in test array at index 216 and 217
//    test[216] = (uint16_t)(resistanceValue >> 16);
//    test[217] = (uint16_t)resistanceValue;
    // Print or use the resistance value in ohms as needed
    //printf("Resistance: %u ohms\n", resistanceValue);
    uint32_t resistanceValue = combinedNumber;
		
		//vTaskDelay(400);

    // Convert resistance value to bytes and store in buffer
    buffer[0] = (uint8_t)(resistanceValue >> 24);
    buffer[1] = (uint8_t)(resistanceValue >> 16);
    buffer[2] = (uint8_t)(resistanceValue >> 8);
    buffer[3] = (uint8_t)resistanceValue;

    // Store the range in the fifth byte
    buffer[4] = receivedData.range;

    // Store the channel in the sixth byte
    buffer[5] = multiChannel;

    // Store the sequence number in the seventh byte
    buffer[6] = sequenceNumber;

    // Increment the sequence number for the next message
    sequenceNumber++;


    // Store the resistance value in test array at index 216 and 217
    test[216] = (uint16_t)(resistanceValue >> 16);
    test[217] = (uint16_t)resistanceValue;		
}

void ControlChannelsTask(void *pvParameters) {
    while (1) {
        //ControlChannels(test[220]);
        //SingleControlChannels(test[220], test[221], test[222], test[223], test[224], test[225], test[226], test[227]);
        //ControlSwitches(test[230], test[231], test[232], test[233]);
        //ControlSwitches(0, 1, 1, 0);
        //ProcessAndConvertResistance(i2c_send);
        vTaskDelay(100); // Delay for 100 milliseconds
    }
}

void ES51932_Init(void) {
    // Initialize USART3 and GPIO
    USART_Configuration();
    GPIO_Configuration();

    // Create a task to call ControlChannels every 100 milliseconds
    xTaskCreate(ControlChannelsTask, ( signed portCHAR * )"ControlChannelsTask", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY+5, NULL);
	
}

void USART3_IRQHandler(void) {
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET) {
        // Read one byte from the receive data register
        uint8_t data = USART_ReceiveData(USART3);

        // Clear the highest bit (set the first bit to 0)
        data &= 0x7F;
			
        // Store received data in buffer
        dataBuffer[dataIndex++] = data;

        // Check if we have received a full packet
        if (dataIndex >= 14) {
            // Copy buffer to receivedData structure
            receivedData.range = dataBuffer[0];
            receivedData.digit4 = dataBuffer[1];
            receivedData.digit3 = dataBuffer[2];
            receivedData.digit2 = dataBuffer[3];
            receivedData.digit1 = dataBuffer[4];
            receivedData.digit0 = dataBuffer[5];
            receivedData.function = dataBuffer[6];
            receivedData.status = dataBuffer[7];
            receivedData.option1 = dataBuffer[8];
            receivedData.option2 = dataBuffer[9];
            receivedData.option3 = dataBuffer[10];
            receivedData.option4 = dataBuffer[11];
            receivedData.CR = dataBuffer[12];
            receivedData.LF = dataBuffer[13];

            // Reset buffer index
            //dataIndex = 0;
            // Check for end of message (carriage return and line feed)
            //if (receivedData.CR  == 0x0D && receivedData.LF == 0x0A) {
							// Process received data
							// Decode digits and combine into a single number
							combinedNumber = 0;
							combinedNumber += decodeDigit(receivedData.digit4) * 10000;
							combinedNumber += decodeDigit(receivedData.digit3) * 1000;
							combinedNumber += decodeDigit(receivedData.digit2) * 100;
							combinedNumber += decodeDigit(receivedData.digit1) * 10;
							combinedNumber += decodeDigit(receivedData.digit0);
							
							// Copy receivedData elements to test array starting from index 200
							test[200] = receivedData.range;
							test[201] = receivedData.digit4;
							test[202] = receivedData.digit3;
							test[203] = receivedData.digit2;
							test[204] = receivedData.digit1;
							test[205] = receivedData.digit0;
							test[206] = receivedData.function;
							test[207] = receivedData.status;
							test[208] = receivedData.option1;
							test[209] = receivedData.option2;
							test[210] = receivedData.option3;
							test[211] = receivedData.option4;
							test[212] = receivedData.CR;
							test[213] = receivedData.LF;
							
							// Store the combined number in the test array as 16-bit values
							test[214] = (combinedNumber >> 16) & 0xFFFF;
							test[215] = combinedNumber & 0xFFFF;

							// Convert resistance value to bytes and store in i2c_send_buf
							i2c_send[0] = (uint8_t)(combinedNumber >> 24);
							i2c_send[1] = (uint8_t)(combinedNumber >> 16);
							i2c_send[2] = (uint8_t)(combinedNumber >> 8);
							i2c_send[3] = (uint8_t)combinedNumber;

							// Store the range in the fifth byte
							i2c_send[4] = receivedData.range;

							// Store the channel in the sixth byte
							i2c_send[5] = multiChannel;
							
	//						// Store the sequence number in the seventh byte
	//						i2c_send[6] = sequenceNumber;
	
							//
							i2c_send[6] = receivedData.function;

	//						// Increment the sequence number for the next message
	//						sequenceNumber++;

							// Set the flag to indicate that serial data has been received
							receivedSerialData = 1;
//						}else {
//                // If the last two bytes are not CR and LF, discard the packet
//                dataIndex = 0;
//            }
        }
				
        // Set the flag to indicate that serial data has been received
        receivedSerialData = 1;	

        // Prevent buffer overflow
        if (dataIndex >= sizeof(dataBuffer)) {
            dataIndex = 0;
        }				

        // Clear the USART3 receive interrupt
        USART_ClearITPendingBit(USART3, USART_IT_RXNE);
    }
}

void SetMode(uint8_t mode) {
    // Ensure PB13 is high and PA5 is high at the start
    //GPIO_SetBits(GPIOB, GPIO_Pin_13); // PB13 high
    GPIO_SetBits(GPIOA, GPIO_Pin_5);  // PA5 high  // FC5 is low, negative voltage means FC5 is low

    i2c_send[7]++;

    switch (mode) {
        case 0:
            // DC Voltage Measurement
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for voltage measurement
            break;
        case 1:
            // Auto DC Current Measurement(µA)
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 2:
            // Auto DC Current Measurement(mA)
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 3:
            // 322A DC Current Measurement(A)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 4:
            // DC 220.00mV
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for voltage measurement
            break;
        case 5:
            // Manual DC 22.000A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 6:
            // Manual DC 220.00A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 7:
            // Manual DC 2200.0A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 8:
            // Manual DC 22000A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 9:
            // Resistance Measurement
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 1, 1, 0);          // Set switches for resistance measurement
            break;
        case 10:
            // Continuity Check
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 1, 1, 0);          // Set switches for continuity check
            break;
        case 11:
            // Diode Measurement
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 1, 1, 0);          // Set switches for diode measurement
            break;
        case 12:
            // Frequency Measurement
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 1);          // Set switches for frequency measurement
            break;
        case 13:
            // Capacitance Measurement
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 1, 1, 0);          // Set switches for capacitance measurement
            break;
        case 14:
            // Temperature Measurement (oC)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for temperature measurement
            break;
        case 15:
            // Resistance Measurement (Alternative)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 1, 1, 0);          // Set switches for resistance measurement
            break;
        case 16:
            // AC Voltage Measurement
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for voltage measurement
            break;
        case 17:
            // Auto AC Current Measurement(µA)
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 18:
            // Auto AC Current Measurement(mA)
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 19:
            // 322A AC Current Measurement(A)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 20:
            // AC 220.00mV
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for voltage measurement
            break;
        case 21:
            // Manual AC 22.000A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 22:
            // Manual AC 220.00A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 23:
            // Manual AC 2200.0A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 24:
            // Manual AC 22000A
            GPIO_ResetBits(GPIOC, GPIO_Pin_0);    // FC1 low
            GPIO_SetBits(GPIOC, GPIO_Pin_2);      // FC2 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(0, 0, 0, 0);          // Set switches for current measurement
            break;
        case 25:
            // ADP0(22000)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 low
            ControlSwitches(1, 0, 0, 0);          // Set switches for ADP measurement
            break;
        case 26:
            // ADP1(2200.0)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(1, 0, 0, 0);          // Set switches for ADP measurement
            break;
        case 27:
            // ADP2(220.00)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(1, 0, 0, 0);          // Set switches for ADP measurement
            break;
        case 28:
            // ADP3(22.000)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(1, 0, 0, 0);          // Set switches for ADP measurement
            break;
        case 29:
            // ADP4(2.2000)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_ResetBits(GPIOA, GPIO_Pin_7);    // FC3 low
            GPIO_SetBits(GPIOA, GPIO_Pin_6);      // FC4 high
            ControlSwitches(1, 0, 0, 0);          // Set switches for ADP measurement
            break;
        case 30:
            // Temperature Measurement (oF)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 high
            ControlSwitches(0, 0, 0, 0);          // Set switches for temperature measurement
            break;
        case 31:
            // Capacitance Measurement (Clamp)
            GPIO_SetBits(GPIOC, GPIO_Pin_0);      // FC1 high
            GPIO_ResetBits(GPIOC, GPIO_Pin_2);    // FC2 low
            GPIO_SetBits(GPIOA, GPIO_Pin_7);      // FC3 high
            GPIO_ResetBits(GPIOA, GPIO_Pin_6);    // FC4 high
            ControlSwitches(0, 1, 1, 0);          // Set switches for capacitance measurement
            break;
        default:
            // Invalid mode, set all FC pins to high (1)
            GPIO_ResetBits(GPIOC, GPIO_Pin_0 | GPIO_Pin_2 | GPIO_Pin_7 | GPIO_Pin_6);
            break;
    }
}

void SetSLACDC(uint8_t mode) {
    if (mode == SELECT_DC) {
        // Set PB13 high for SELECT_DC
        GPIO_SetBits(GPIOB, GPIO_Pin_13);
    } else if (mode == SELECT_AC) {
        // Set PB13 low for SELECT_AC
        GPIO_ResetBits(GPIOB, GPIO_Pin_13);
    }
}
