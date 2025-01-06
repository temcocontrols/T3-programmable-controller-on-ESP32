#include "multiMeter.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "define.h"
#include "ud_str.h"
#include "i2c_task.h"
#include "user_data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "i2c_task.h"

#define MAX_CHANNELS 8
#define NUM_SAMPLES 10
#define THRESHOLD 	10 // threshold value

uint32_t measurementBuffer[MAX_CHANNELS][NUM_SAMPLES] = {0};
uint8_t sampleIndex[MAX_CHANNELS] = {0};

extern uint8_t i2c_rcv_buf[];
extern uint8_t i2c_send_buf[];

// Initialize the multi-meter
void initMultiMeter(MultiMeter *meter) {
    meter->num_channels = 0;
    for (int i = 0; i < MAX_CHANNELS; i++) {
        meter->channels[i].channel = i;
        meter->channels[i].type = VOLTAGE; // Default type
        meter->channels[i].value = 0.0f;
        meter->channels[i].range = RANGE_2_2000V; // Default range
        snprintf(meter->channels[i].description, sizeof(meter->channels[i].description), "Channel %d", i + 1);
        snprintf(meter->channels[i].label, sizeof(meter->channels[i].label), "CH%d", i + 1);
        memset(meter->channels[i].buffer, 0, sizeof(meter->channels[i].buffer)); // Initialize buffer
        meter->channels[i].bufferIndex = 0; // Initialize buffer index
    }
}

// Simulate reading a measurement from a specific channel
void readMeasurement(MultiMeter *meter, uint8_t channel, MeasurementType type, uint8_t range) {
    if (channel >= MAX_CHANNELS) {
        printf("Invalid channel number\n");
        return;
    }

    meter->channels[channel].type = type;
    meter->channels[channel].range = range;

    // Simulate reading a value based on the type and range
    switch (type) {
        case VOLTAGE:
            meter->channels[channel].value = 220.0f; // Simulated voltage value
            break;
        case AUTO_UA_CURRENT:
            meter->channels[channel].value = 0.005f; // Simulated current value in microamps
            break;
        case AUTO_MA_CURRENT:
            meter->channels[channel].value = 0.5f; // Simulated current value in milliamps
            break;
        case CURRENT_22A:
            meter->channels[channel].value = 10.0f; // Simulated current value in amps
            break;
        case MANUAL_A_CURRENT:
            meter->channels[channel].value = 1.0f; // Simulated current value in amps
            break;
        case RESISTANCE:
            meter->channels[channel].value = 1000.0f; // Simulated resistance value in ohms
            break;
        case CONTINUITY:
            meter->channels[channel].value = 1.0f; // Simulated continuity value (1 for continuity, 0 for no continuity)
            break;
        case DIODE:
            meter->channels[channel].value = 0.7f; // Simulated diode voltage drop
            break;
        case FREQUENCY:
            meter->channels[channel].value = 50.0f; // Simulated frequency value in Hz
            break;
        case CAPACITANCE:
            meter->channels[channel].value = 100.0f; // Simulated capacitance value in microfarads
            break;
        case TEMPERATURE:
            meter->channels[channel].value = 25.0f; // Simulated temperature value in degrees Celsius
            break;
        case ADP:
            meter->channels[channel].value = 3.3f; // Simulated ADP value
            break;
        default:
            printf("Unknown measurement type\n");
            return;
    }

    // Store the value in the circular buffer
    meter->channels[channel].buffer[meter->channels[channel].bufferIndex] = meter->channels[channel].value;
    meter->channels[channel].bufferIndex = (meter->channels[channel].bufferIndex + 1) % BUFFER_SIZE;

    printf("Channel %d: %s = %.2f\n", channel, meter->channels[channel].description, meter->channels[channel].value);
}

// Control function to set the measurement type and range for a specific channel
void setChannelMeasurement(MultiMeter *meter, uint8_t channel, MeasurementType type, uint8_t range) {
    if (channel >= MAX_CHANNELS) {
        printf("Invalid channel number\n");
        return;
    }

    meter->channels[channel].type = type;
    meter->channels[channel].range = range;
    snprintf(meter->channels[channel].description, sizeof(meter->channels[channel].description), "Channel %d - %s", channel + 1, (type == VOLTAGE) ? "Voltage" : "Other");
    printf("Channel %d measurement type set to %d and range set to %d\n", channel, type, range);
}

// Function to filter out measurements with large deviations
/*void filterMeasurements(float *measurements, int size, float threshold) {
    float sum = 0.0f;
    int count = 0;

    // Calculate the average value
    for (int i = 0; i < size; i++) {
        sum += measurements[i];
        count++;
    }
    float average = sum / count;

    // Filter out measurements with large deviations
    for (int i = 0; i < size; i++) {
        if (fabs(measurements[i] - average) > threshold) {
            printf("Measurement %.2f at index %d is an outlier and will be removed.\n", measurements[i], i);
            measurements[i] = average; // Replace outlier with average value
        }
    }
}*/
void filterMeasurements(uint32_t *measurements, int size, uint32_t threshold) {
    uint32_t sum = 0;
    int count = 0;

    // Calculate the average value
    for (int i = 0; i < size; i++) {
        sum += measurements[i];
        count++;
    }
    uint32_t average = sum / count;

    // Filter out measurements with large deviations
    for (int i = 0; i < size; i++) {
        if (abs((int32_t)(measurements[i] - average)) > threshold) {
            //printf("Measurement %u at index %d is an outlier and will be removed.\n", measurements[i], i);
            measurements[i] = average; // Replace outlier with average value
        }
    }
}

// Function to process received data packet
void processReceivedData(MultiMeter *meter, const DataPacket *packet) {
    // Extract the value, range, and channel from the data packet
    uint32_t value = (packet->data[0] << 24) | (packet->data[1] << 16) | (packet->data[2] << 8) | packet->data[3];
    uint8_t range = packet->data[4];
    uint8_t channel = packet->data[5];

    if (channel >= MAX_CHANNELS) {
        printf("Invalid channel number in received data\n");
        return;
    }

    // Update the channel with the received data
    meter->channels[channel].value = (float)value;
    meter->channels[channel].range = range;

    // Store the value in the circular buffer
    meter->channels[channel].buffer[meter->channels[channel].bufferIndex] = meter->channels[channel].value;
    meter->channels[channel].bufferIndex = (meter->channels[channel].bufferIndex + 1) % BUFFER_SIZE;

    printf("Received data for Channel %d: Value = %.2f, Range = %d\n", channel, meter->channels[channel].value, range);
}

// Display all readings
void displayReadings(const MultiMeter *meter) {
    for (int i = 0; i < meter->num_channels; i++) {
        //printf("Channel %d (%s): %.2f\n", meter->channels[i].channel, meter->channels[i].description, meter->channels[i].value);
    }
}

// Function to initialize channels
void initializeChannels(void) {
    Str_points_ptr ptr;

    ptr = put_io_buf(IN, 0);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH1 VALUE", strlen("CH1 VALUE") + 1);
    memcpy(ptr.pin->label, "CH1VAL", strlen("CH1VAL") + 1);

    ptr = put_io_buf(IN, 1);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH2 VALUE", strlen("CH2 VALUE") + 1);
    memcpy(ptr.pin->label, "CH2VAL", strlen("CH2VAL") + 1);

    ptr = put_io_buf(IN, 2);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH3 VALUE", strlen("CH3 VALUE") + 1);
    memcpy(ptr.pin->label, "CH3VAL", strlen("CH3VAL") + 1);

    ptr = put_io_buf(IN, 3);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH4 VALUE", strlen("CH4 VALUE") + 1);
    memcpy(ptr.pin->label, "CH4VAL", strlen("CH4VAL") + 1);

    ptr = put_io_buf(IN, 4);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH5 VALUE", strlen("CH5 VALUE") + 1);
    memcpy(ptr.pin->label, "CH5VAL", strlen("CH5VAL") + 1);

    ptr = put_io_buf(IN, 5);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH6 VALUE", strlen("CH6 VALUE") + 1);
    memcpy(ptr.pin->label, "CH6VAL", strlen("CH6VAL") + 1);

    ptr = put_io_buf(IN, 6);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH7 VALUE", strlen("CH7 VALUE") + 1);
    memcpy(ptr.pin->label, "CH7VAL", strlen("CH7VAL") + 1);

    ptr = put_io_buf(IN, 7);
    ptr.pin->range = AC_PWM + 1;
    memcpy(ptr.pin->description, "CH8 VALUE", strlen("CH8 VALUE") + 1);
    memcpy(ptr.pin->label, "CH8VAL", strlen("CH8VAL") + 1);
}

void processMeasurementFunction(MultiMeter *meter, uint8_t *tempBuffer) {
	Str_points_ptr ptr;
	uint32_t measuredValue;

    ptr = put_io_buf(VAR, 3);
    measuredValue = ptr.pvar->value =
        ((uint32_t)tempBuffer[0] << 24) | ((uint32_t)tempBuffer[1] << 16) | ((uint32_t)tempBuffer[2] << 8) | (uint32_t)tempBuffer[3];

    // Save the measured value to the buffer for the current channel
    uint8_t channel = tempBuffer[5] - 1;
    measurementBuffer[channel][sampleIndex[channel]] = measuredValue;
    sampleIndex[channel] = (sampleIndex[channel] + 1) % NUM_SAMPLES;

    // Filter the measurements
    filterMeasurements(measurementBuffer[channel], NUM_SAMPLES, THRESHOLD);

    // Calculate the average value after filtering
    uint32_t sum = 0;
    for (int i = 0; i < NUM_SAMPLES; i++) {
        sum += measurementBuffer[channel][i];
    }
    uint32_t filteredValue = sum / NUM_SAMPLES;

    // Update the measured value with the filtered value
    measuredValue = filteredValue;

    if (tempBuffer[6] == FUNCTION_RESISTANCE) {

        if (ptr.pvar->value > 22100)
            ptr.pvar->value = 0;

        if (measuredValue > 22100)
            measuredValue = 0;

        switch (tempBuffer[4]) {
            case RANGE_220_00_OHM:
                ptr.pvar->value /= 100; // 220.00 Ohm range
                measuredValue /= 100;
                break;
            case RANGE_2_2000K_OHM:
                ptr.pvar->value /= 10; // 2.2000K Ohm range
                measuredValue /= 10;
                break;
            case RANGE_22_000K_OHM:
                ptr.pvar->value /= 1; // 22.0000K Ohm range
                measuredValue /= 1;
                break;
            case RANGE_220_00K_OHM:
                ptr.pvar->value *= 10; // 220.00K Ohm range
                measuredValue *= 10;
                break;
            case RANGE_2_2000M_OHM:
                ptr.pvar->value *= 100; // 2.2000M Ohm range
                measuredValue *= 100;
                break;
            case RANGE_22_000M_OHM:
                ptr.pvar->value *= 1000; // 22.000M Ohm range
                measuredValue *= 1000;
                break;
            case RANGE_220_00M_OHM:
                ptr.pvar->value *= 10000; // 220.00M Ohm range
                measuredValue *= 10000;
                break;
            default:
                // Handle unknown range
                break;
        }

    }else if (tempBuffer[6] == FUNCTION_VOLTAGE) {

        switch (tempBuffer[4]) {
            case RANGE_220_00V:
                ptr.pvar->value *= 10; // 220.00V range
                measuredValue *= 10;
                break;
            case RANGE_2_2000V:
                ptr.pvar->value *= 100; // 2.2000V range
                measuredValue *= 100;
                break;
            case RANGE_22_000V:
                ptr.pvar->value *= 1000; // 22.0000V range
                measuredValue *= 1000;
                break;
            default:
                // Handle unknown range
                break;
        }

    }else if (tempBuffer[6] == FUNCTION_CONTINUITY) {

    } else if (tempBuffer[6] == FUNCTION_DIODE) {
        // Add diode measurement logic here

    } else if (tempBuffer[6] == FUNCTION_CAPACITANCE) {
        if (tempBuffer[4] == RANGE_22_000NF) {
            ptr.pvar->value *= 10; // 22.000nF range
            measuredValue *= 10;
        } else if (tempBuffer[4] == RANGE_220_00NF) {
            ptr.pvar->value *= 100; // 220.00nF range
            measuredValue *= 100;
        } else if (tempBuffer[4] == RANGE_2_2000UF) {
            ptr.pvar->value *= 1000; // 2.2000uF range
            measuredValue *= 1000;
        } else if (tempBuffer[4] == RANGE_22_000UF) {
            ptr.pvar->value *= 10000; // 22.000uF range
            measuredValue *= 10000;
        } else if (tempBuffer[4] == RANGE_220_00UF) {
            ptr.pvar->value *= 100000; // 220.00uF range
            measuredValue *= 100000;
        } else if (tempBuffer[4] == RANGE_2_2000MF) {
            ptr.pvar->value *= 1000000; // 2.2000mF range
            measuredValue *= 1000000;
        } else if (tempBuffer[4] == RANGE_22_000MF) {
            ptr.pvar->value *= 10000000; // 22.000mF range
            measuredValue *= 10000000;
        } else if (tempBuffer[4] == RANGE_220_00MF) {
            ptr.pvar->value *= 100000000; // 220.00mF range
            measuredValue *= 100000000;
        }
    } else if (tempBuffer[6] == FUNCTION_FREQUENCY) {
        if (tempBuffer[4] == RANGE_22_00HZ) {
            ptr.pvar->value *= 10; // 22.00Hz range
            measuredValue *= 10;
        } else if (tempBuffer[4] == RANGE_220_0HZ) {
            ptr.pvar->value *= 100; // 220.0Hz range
            measuredValue *= 100;
        } else if (tempBuffer[4] == RANGE_22_000KHZ) {
            ptr.pvar->value *= 1000; // 22.000kHz range
            measuredValue *= 1000;
        } else if (tempBuffer[4] == RANGE_220_00KHZ) {
            ptr.pvar->value *= 10000; // 220.00kHz range
            measuredValue *= 10000;
        } else if (tempBuffer[4] == RANGE_2_2000MHZ) {
            ptr.pvar->value *= 100000; // 2.2000MHz range
            measuredValue *= 100000;
        } else if (tempBuffer[4] == RANGE_22_000MHZ) {
            ptr.pvar->value *= 1000000; // 22.000MHz range
            measuredValue *= 1000000;
        } else if (tempBuffer[4] == RANGE_220_00MHZ) {
            ptr.pvar->value *= 10000000; // 220.00MHz range
            measuredValue *= 10000000;
        }
    } else if (tempBuffer[6] == FUNCTION_MANUAL_A_CURRENT) {
        if (tempBuffer[4] == RANGE_2_2000A) {
            ptr.pvar->value *= 10; // 2.2000A range
            measuredValue *= 10;
        } else if (tempBuffer[4] == RANGE_22_000A) {
            ptr.pvar->value *= 100; // 22.000A range
            measuredValue *= 100;
        } else if (tempBuffer[4] == RANGE_220_00A) {
            ptr.pvar->value *= 1000; // 220.00A range
            measuredValue *= 1000;
        } else if (tempBuffer[4] == RANGE_2200_0A) {
            ptr.pvar->value *= 10000; // 2200.0A range
            measuredValue *= 10000;
        } else if (tempBuffer[4] == RANGE_22000A) {
            ptr.pvar->value *= 100000; // 22000A range
            measuredValue *= 100000;
        }
    }
    meter->num_channels = tempBuffer[5];
    if (meter->num_channels >= 1 && meter->num_channels <= 8) {
        ptr = put_io_buf(IN, meter->num_channels - 1);
        ptr.pin->value = measuredValue; // Use the received resistance value
        meter->channels[meter->num_channels].value = measuredValue;
    }
}

// Define task function
void multiMeterTask(void *pvParameters) {
    MultiMeter meter;
    initMultiMeter(&meter);

#if 0
    // Simulate reading measurements
    readMeasurement(&meter, 0, VOLTAGE, RANGE_220_00V);
    readMeasurement(&meter, 1, AUTO_UA_CURRENT, RANGE_LOWER_AUTO_A);
    readMeasurement(&meter, 2, RESISTANCE, RANGE_220_00_OHM);

    // Set channel measurement type and range
    setChannelMeasurement(&meter, 0, CURRENT_22A, RANGE_22_000A);

    // Simulate receiving a data packet
    DataPacket packet = {{0x00, 0x00, 0x03, 0xE8, RANGE_220_00V, 0x00}}; // Example packet
    processReceivedData(&meter, &packet);

    // Simulate a series of measurements
    float measurements[] = {220.0f, 221.0f, 219.5f, 300.0f, 220.5f, 218.0f};
    int size = sizeof(measurements) / sizeof(measurements[0]);

    // Filter out measurements with large deviations
    filterMeasurements(measurements, size, 10.0f);

    // Display all readings
    displayReadings(&meter);
#endif

    Str_points_ptr ptr;
    uint8_t index = 0;
    uint32_t multiMeterChannelvalue;
    //uint32_t measuredValue;
    uint8_t tempBuffer[20]; // Temporary buffer to store received data

    meter.mode = MEASUREMENT_MODE_MANUAL;

    i2c_master_init();

    ptr = put_io_buf(VAR, 0);
    ptr.pvar->range = custom1;
	memcpy(ptr.pvar->description,"MEASURE FUNCTION", strlen("MEASURE FUNCTION") + 1);
	memcpy(ptr.pvar->label,"FUNCTION",strlen("FUNCTION") + 1);
	if(ptr.pvar->value  < (MODE_DC_VOLTAGE_MEASUREMENT*1000) || ptr.pvar->value > (MODE_CAPACITANCE_MEASUREMENT_CLAMP*1000)) {
		ptr.pvar->value = 1000*MODE_RESISTANCE_MEASUREMENT;
		multiMeterChannelvalue = ptr.pvar->value;
	}

	ptr = put_io_buf(VAR, 1);
	//if(ptr.pvar->range != 0)
	{
		ptr.pvar->range = custom1;
		memcpy(ptr.pvar->description,"MEASURE MODE", strlen("MEASURE MODE") + 1);
		memcpy(ptr.pvar->label,"MODE",strlen("MODE") + 1);
		if(ptr.pvar->value  < 0 || ptr.pvar->value > 1000) {
			ptr.pvar->value = 0;
			meter.mode = ptr.pvar->value/1000;
		}
	}

    ptr = put_io_buf(VAR, 2);
    //if(ptr.pvar->range != 0)
    {
        ptr.pvar->range = custom1;
        memcpy(ptr.pvar->description,"MEASURE CHANNEL", strlen("MEASURE CHANNEL") + 1);
        memcpy(ptr.pvar->label,"CHANNEL",strlen("CHANNEL") + 1);
        if(ptr.pvar->value  < 1000 || ptr.pvar->value > 8000) {
            ptr.pvar->value = 1000;
            multiMeterChannelvalue = ptr.pvar->value;
        }
    }
    ptr = put_io_buf(VAR, 3);
    //if(ptr.pvar->range != 0)
    {
        ptr.pvar->range = ohms;
        memcpy(ptr.pvar->description,"RESISTANCE VALUE", strlen("RESISTANCE VALUE"));
        memcpy(ptr.pvar->label,"VALUE",strlen("VALUE"));
    }

     // Simplified initialization of channels
    /*for (int i = 5; i <= 7; i++) {
        ptr = put_io_buf(IN, i);
        ptr.pin->range = AC_PWM + 1;
        snprintf((char *)ptr.pin->description, 21, "CH%d RESISTANCE VALUE", i + 1);
        snprintf((char *)ptr.pin->label, 9, "CH%dVAL", i + 1);
    }*/
    initializeChannels();

    // Task loop
    while (1) {
    	//Measure mode
    	ptr = put_io_buf(VAR, 1);
    	if(ptr.pvar->value >= 0 && ptr.pvar->value <2000)
    		meter.mode =ptr.pvar->value/1000;
    	else
    		meter.mode = MEASUREMENT_MODE_MANUAL;

    	// Measure channel
        ptr = put_io_buf(VAR, 2);
        if (meter.mode == MEASUREMENT_MODE_AUTO) {
			ptr.pvar->value += 1000;
			if (ptr.pvar->value > 8000) {
				ptr.pvar->value = 1000;
			}
        }else {
        	// MEASUREMENT_MODE_MANUAL
        }
        // Prepare the data to send
        i2c_send_buf[0] = (uint8_t)(ptr.pvar->value / 1000);
        // Send the data via I2C
        stm_i2c_write(10, i2c_send_buf, 1);

        vTaskDelay(pdMS_TO_TICKS(10));

        // Measure function, prepare to send to slave
        ptr = put_io_buf(VAR, 0);
        i2c_send_buf[0] = (uint8_t)(ptr.pvar->value / 1000);
        stm_i2c_write(11, i2c_send_buf, 1);

        if (meter.mode == MEASUREMENT_MODE_AUTO) {
        	vTaskDelay(pdMS_TO_TICKS(5000)); // Delay for 5 seconds
        }else {
        	vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 1 second
        }

        // Receive data via I2C
        stm_i2c_read(0, i2c_rcv_buf, 20); // Store received data in i2c_rcv_buf
        memcpy(tempBuffer, i2c_rcv_buf, 20); // Copy data to tempBuffer
        //memcpy(&Test[30], tempBuffer, 20);

        processMeasurementFunction(&meter, tempBuffer);

        if(meter.mode == MEASUREMENT_MODE_AUTO){

        }else {  // MEASUREMENT_MODE_MANUAL

        }

        if (meter.mode == MEASUREMENT_MODE_AUTO) {
        	vTaskDelay(pdMS_TO_TICKS(3000)); // Delay for 1 second
        } else {
        	vTaskDelay(pdMS_TO_TICKS(2000)); // Delay for 1 second
        }
    }
}
