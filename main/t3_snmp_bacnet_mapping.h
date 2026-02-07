#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define T3_MAX_INSTANCES              64
#define T3_DESCRIPTION_LENGTH         20
#define T3_SHORT_NAME_LENGTH          20
#define T3_MAX_STRING_LENGTH          64

#define DIGITAL_VALUE                  0
#define ANALOG_VALUE                   1

/* Error Codes */
#define T3_SUCCESS                    0
#define T3_ERROR_INVALID_INSTANCE    -1
#define T3_ERROR_INVALID_CFGTYPE     -2
#define T3_ERROR_INVALID_FIELD       -3
#define T3_ERROR_READ_ONLY           -4
#define T3_ERROR_WRITE_ONLY          -5
#define T3_ERROR_TYPE_MISMATCH       -6
#define T3_ERROR_NOT_FOUND           -7
#define T3_ERROR_INVALID_DATA        -8

/* Enterprise OID Structure per T3 Specification */
/* P denotes Private-Enterprises - 1.3.6.1.4.1   */
#define T3_ENTERPRISE_OID            "P.64991.1"
#define T3_INPUTS_OID_BASE           "P.64991.1.1"
#define T3_OUTPUTS_OID_BASE          "P.64991.1.2"
#define T3_VARIABLES_OID_BASE        "P.64991.1.3"

/* Field OID Patterns per specification */
#define T3_INPUT_INDEX_OID           ".0"
#define T3_INPUT_CFGTYPE_OID         ".1"
#define T3_INPUT_ANALOG_OID          ".2"
#define T3_INPUT_BINARY_OID          ".3"
#define T3_INPUT_DESC_OID            ".4"
#define T3_INPUT_UNITS_OID           ".5"

#define T3_OUTPUT_INDEX_OID          ".0"
#define T3_OUTPUT_CFGTYPE_OID        ".1"
#define T3_OUTPUT_ANALOG_OID         ".2"
#define T3_OUTPUT_BINARY_OID         ".3"
#define T3_OUTPUT_DESC_OID           ".4"
#define T3_OUTPUT_UNITS_OID          ".5"

#define T3_VARIABLE_INDEX_OID        ".0"
#define T3_VARIABLE_CFGTYPE_OID      ".1"
#define T3_VARIABLE_INT_OID          ".2"
#define T3_VARIABLE_FLOAT_OID        ".3"
#define T3_VARIABLE_DESC_OID         ".4"
#define T3_VARIABLE_UNITS_OID        ".5"

typedef enum {
    T3_FIELD_INDEX       = 0,
    T3_FIELD_CFGTYPE     = 1,
    T3_FIELD_INTIGER     = 2,
    T3_FIELD_ANALOG      = T3_FIELD_INTIGER,
    T3_FIELD_REAL        = 3,
    T3_FIELD_BINARY      = T3_FIELD_REAL,
    T3_FIELD_DESC        = 4,
    T3_FIELD_UNITS       = 5,
} t3_field_t;

/* Configuration Type Enumeration (cfgType) per specification */
typedef enum {
    T3_CFGTYPE_BI           = 1,   // Binary Input
    T3_CFGTYPE_AI_0_100     = 2,   // Analog Input 0-100
    T3_CFGTYPE_AI_0_10V     = 3,   // Analog Input 0-10V
    T3_CFGTYPE_AI_4_20MA    = 4,   // Analog Input 4-20mA
    T3_CFGTYPE_AI_NEG10_10V = 5,   // Analog Input -10 to +10V
    T3_CFGTYPE_AI_0_5V      = 6,   // Analog Input 0-5V
    T3_CFGTYPE_BO           = 7,   // Binary Output
    T3_CFGTYPE_AO_0_10V     = 8,   // Analog Output 0-10V
    T3_CFGTYPE_AO_4_20MA    = 9,   // Analog Output 4-20mA
    T3_CFGTYPE_AO_0_100     = 10,  // Analog Output 0-100%
    T3_CFGTYPE_VAR_INT      = 11,  // Variable Integer
    T3_CFGTYPE_VAR_FLOAT    = 12,  // Variable Float
    T3_CFGTYPE_VAR_STRING   = 13,  // Variable String
    T3_CFGTYPE_RESERVED     = 14   // Reserved
} t3_cfgtype_t;

/* Engineering Units Enumeration per specification */
typedef enum {
    T3_UNITS_NONE       = 0,
    T3_UNITS_CELSIUS    = 1,
    T3_UNITS_FAHRENHEIT = 2,
    T3_UNITS_KELVIN     = 3,
    T3_UNITS_PERCENT    = 4,
    T3_UNITS_PASCAL     = 5,
    T3_UNITS_PSI        = 6,
    T3_UNITS_MILLIAMPS  = 7,
    T3_UNITS_VOLTS      = 8,
    T3_UNITS_HERTZ      = 9,
    T3_UNITS_SECONDS    = 10
} t3_units_t;

/* Object Types */
typedef enum {
    T3_OBJECT_INPUT    = 1,
    T3_OBJECT_OUTPUT   = 2,
    T3_OBJECT_VARIABLE = 3
} t3_object_type_t;

/* BACnet Object Type Mapping */
typedef enum {
    T3_BACNET_AI = 0,     // Analog Input
    T3_BACNET_BI = 1,     // Binary Input
    T3_BACNET_AO = 2,     // Analog Output
    T3_BACNET_BO = 3,     // Binary Output
    T3_BACNET_AV = 4      // Analog Variable
} t3_bacnet_object_t;

/* SNMP to BACnet Property Mapping */
typedef enum {
    T3_PROPERTY_PRESENT_VALUE  = 0,
    T3_PROPERTY_DESCRIPTION    = 1,
    T3_PROPERTY_UNITS          = 2,
    T3_PROPERTY_STATUS_FLAGS   = 3,
    T3_PROPERTY_EVENT_STATE    = 4,
    T3_PROPERTY_OUT_OF_SERVICE = 5
} t3_bacnet_property_t;

/* Point Mapping Structure */
typedef struct {
    uint32_t object_type;       // T3_OBJECT_INPUT/OUTPUT/VARIABLE
    uint32_t instance;          // 0-63
    uint32_t cfg_type;          // T3_CFGTYPE_*
    uint32_t field;             // 0-5
    t3_bacnet_object_t bacnet_type;
    bool is_valid;
} t3_snmp_bacnet_mapping_t;

/* Data Access Structure */
typedef struct {
    float analog_value;
    int32_t binary_value;
    int32_t int_value;
    float float_value;
    char string_value[21];
    bool is_analog;
    bool is_binary;
    bool is_integer;
    bool is_float;
    bool is_string;
} t3_data_value_t;

/* Configuration Type Functions */
const char* t3_get_cfgtype_name(uint32_t cfg_type);
bool t3_is_input_type(uint32_t cfg_type);
bool t3_is_output_type(uint32_t cfg_type);
bool t3_is_variable_type(uint32_t cfg_type);
uint8_t t3_cfgtype_to_bacnet_object(uint32_t cfg_type);

bool t3_map_snmp_oid_to_bacnet(const char* snmp_oid, t3_snmp_bacnet_mapping_t *mapping);
int t3_read_input_value(uint32_t instance, uint32_t field, t3_data_value_t *value);
int t3_read_variable_value(uint32_t instance, uint32_t field, t3_data_value_t *value);
int t3_read_output_value(uint32_t instance, uint32_t field, t3_data_value_t *value);

int t3_write_output_value(uint32_t instance, uint32_t field, const t3_data_value_t *value);
int t3_write_variable_value(uint32_t instance, uint32_t field, const t3_data_value_t *value);

int fetch_config_type(t3_object_type_t type, int digital_analog, int range);
int fetch_units_type(t3_object_type_t type, int digital_analog, int range);

#ifdef __cplusplus
}
#endif