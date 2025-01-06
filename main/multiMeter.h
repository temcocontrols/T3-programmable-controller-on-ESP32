#ifndef MULTIMETER_H
#define MULTIMETER_H

#include <stdint.h>

// Define constants
#define MAX_CHANNELS 8
#define BUFFER_SIZE 10 // Size of the circular buffer

// Function definitions
#define FUNCTION_22A_CURRENT 48   // 0b0110000
#define FUNCTION_DIODE 49          // 0b0110001
#define FUNCTION_FREQUENCY 50     // 0b0110010
#define FUNCTION_RESISTANCE 51    // 0b0110011
#define FUNCTION_TEMPERATURE 52   // 0b0110100
#define FUNCTION_CONTINUITY 53    // 0b0110101
#define FUNCTION_CAPACITANCE 54   // 0b0110110
#define FUNCTION_VOLTAGE 59       // 0b0111011
#define FUNCTION_MANUAL_A_CURRENT 57 // 0b0111001
#define FUNCTION_AUTO_UA_CURRENT 61 // 0b0111101
#define FUNCTION_ADP 62           // 0b0111110
#define FUNCTION_AUTO_MA_CURRENT 63 // 0b0111111


// Range Resistor definitions
#define RANGE_220_00_OHM 48    // 0b0110000, 0x30
#define RANGE_2_2000K_OHM 49   // 0b0110001, 0x31
#define RANGE_22_000K_OHM 50   // 0b0110010, 0x32
#define RANGE_220_00K_OHM 51   // 0b0110011, 0x33
#define RANGE_2_2000M_OHM 52   // 0b0110100, 0x34
#define RANGE_22_000M_OHM 53   // 0b0110101, 0x35
#define RANGE_220_00M_OHM 54   // 0b0110110, 0x36

// Range definitions
#define RANGE_2_2000V 48          // 0b0110000
#define RANGE_22_000V 49          // 0b0110001
#define RANGE_220_00V 50          // 0b0110010
#define RANGE_2200_0V 51          // 0b0110011
#define RANGE_220_00MV 52         // 0b0110100
#define RANGE_2_2000A 48          // 0b0110000
#define RANGE_22_000A 49          // 0b0110001
#define RANGE_220_00A 50          // 0b0110010
#define RANGE_2200_0A 51          // 0b0110011
#define RANGE_22000A 52           // 0b0110100
#define RANGE_LOWER_AUTO_A 48     // 0b0110000
#define RANGE_HIGHER_AUTO_A 49    // 0b0110001

// Range Capacitor definitions
#define RANGE_22_000NF 48         // 0b0110000
#define RANGE_220_00NF 49         // 0b0110001
#define RANGE_2_2000UF 50         // 0b0110010
#define RANGE_22_000UF 51         // 0b0110011
#define RANGE_220_00UF 52         // 0b0110100
#define RANGE_2_2000MF 53         // 0b0110101
#define RANGE_22_000MF 54         // 0b0110110
#define RANGE_220_00MF 55         // 0b0110111

// Range Manual A Current definitions
#define RANGE_2_2000A 48       // 0b0110000
#define RANGE_22_000A 49       // 0b0110001
#define RANGE_220_00A 50       // 0b0110010
#define RANGE_2200_0A 51       // 0b0110011
#define RANGE_22000A 52        // 0b0110100

// Range Frequency definitions
#define RANGE_22_00HZ 48       // 0b0110000
#define RANGE_220_0HZ 49       // 0b0110001
#define RANGE_22_000KHZ 51     // 0b0110011
#define RANGE_220_00KHZ 52     // 0b0110100
#define RANGE_2_2000MHZ 53     // 0b0110101
#define RANGE_22_000MHZ 54     // 0b0110110
#define RANGE_220_00MHZ 55     // 0b0110111

// Mode definitions
#define MODE_DC_VOLTAGE_MEASUREMENT              0
#define MODE_AUTO_DC_CURRENT_MEASUREMENT_UA      1
#define MODE_AUTO_DC_CURRENT_MEASUREMENT_MA      2
#define MODE_322A_DC_CURRENT_MEASUREMENT_A       3
#define MODE_DC_220MV                            4
#define MODE_MANUAL_DC_22A                       5
#define MODE_MANUAL_DC_220A                      6
#define MODE_MANUAL_DC_2200A                     7
#define MODE_MANUAL_DC_22000A                    8
#define MODE_RESISTANCE_MEASUREMENT              9
#define MODE_CONTINUITY_CHECK                    10
#define MODE_DIODE_MEASUREMENT                   11
#define MODE_FREQUENCY_MEASUREMENT               12
#define MODE_CAPACITANCE_MEASUREMENT             13
#define MODE_TEMPERATURE_MEASUREMENT_C           14
#define MODE_RESISTANCE_MEASUREMENT_ALT          15
#define MODE_AC_VOLTAGE_MEASUREMENT              16
#define MODE_AUTO_AC_CURRENT_MEASUREMENT_UA      17
#define MODE_AUTO_AC_CURRENT_MEASUREMENT_MA      18
#define MODE_322A_AC_CURRENT_MEASUREMENT_A       19
#define MODE_AC_220MV                            20
#define MODE_MANUAL_AC_22A                       21
#define MODE_MANUAL_AC_220A                      22
#define MODE_MANUAL_AC_2200A                     23
#define MODE_MANUAL_AC_22000A                    24
#define MODE_ADP0                                25
#define MODE_ADP1                                26
#define MODE_ADP2                                27
#define MODE_ADP3                                28
#define MODE_ADP4                                29
#define MODE_TEMPERATURE_MEASUREMENT_F           30
#define MODE_CAPACITANCE_MEASUREMENT_CLAMP       31

// Define measurement types
typedef enum {
    VOLTAGE,
    AUTO_UA_CURRENT,
    AUTO_MA_CURRENT,
    CURRENT_22A,
    MANUAL_A_CURRENT,
    RESISTANCE,
    CONTINUITY,
    DIODE,
    FREQUENCY,
    CAPACITANCE,
    TEMPERATURE,
    ADP
} MeasurementType;

typedef enum {
    MEASUREMENT_MODE_MANUAL,
    MEASUREMENT_MODE_AUTO
} MeasurementMode;

// Define data structures
typedef struct {
    uint8_t channel;          // Channel number
    MeasurementType type;     // Type of measurement
    float value;              // Measurement value
    uint8_t range;            // Measurement range
    char description[50];     // Description of the channel
    char label[20];           // Label for the channel
    float buffer[BUFFER_SIZE]; // Circular buffer to store recent measurements
    int bufferIndex;          // Index for the circular buffer
} MultiMeterChannel;

typedef struct {
    MultiMeterChannel channels[MAX_CHANNELS];  // Array of channels
    uint8_t num_channels;                      // Number of active channels
    MeasurementMode mode;
} MultiMeter;

// Define data packet structure
typedef struct {
    uint8_t data[10]; // 10-byte data packet
} DataPacket;

// Function prototypes
void initMultiMeter(MultiMeter *meter);
void readMeasurement(MultiMeter *meter, uint8_t channel, MeasurementType type, uint8_t range);
void setChannelMeasurement(MultiMeter *meter, uint8_t channel, MeasurementType type, uint8_t range);
void filterMeasurements(uint32_t *measurements, int size, uint32_t threshold);
void displayReadings(const MultiMeter *meter);
void processReceivedData(MultiMeter *meter, const DataPacket *packet);

// Task function prototype
void multiMeterTask(void *pvParameters);

#endif // MULTIMETER_H
