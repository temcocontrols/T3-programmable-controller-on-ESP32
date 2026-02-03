//===============================================================================
// t3_snmp_bacnet_mapping.c
//===============================================================================

//=================================== Includes ==================================
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "t3_snmp_bacnet_mapping.h"
#include "esp_log.h"
#include "bacnet.h"
#include "user_data.h"
#include "bacenum.h"
#include "proprietary.h"

//=================================== Defines ===================================

//=================================== Variables =================================

// Log tag
static const char *TAG = "snmp_bacnet";

const char enterprise_oid_str[] = "64991.1";

/* Configuration Type Names */
static const char* cfgtype_names[] =
{
    "Unknown",
    "Binary Input",             // 1
    "Analog Input 0-100",       // 2
    "Analog Input 0-10V",       // 3
    "Analog Input 4-20mA",      // 4
    "Analog Input -10 to +10V", // 5
    "Analog Input 0-5V",        // 6
    "Binary Output",            // 7
    "Analog Output 0-10V",      // 8
    "Analog Output 4-20mA",     // 9
    "Analog Output 0-100%",     // 10
    "Variable Integer",         // 11
    "Variable Float",           // 12
    "Variable String",          // 13
    "Reserved"                  // 14
};

/* Units Names */
static const char* units_names[] = 
{
    "None",         // 0
    "°C",           // 1
    "°F",           // 2
    "K",            // 3
    "%",            // 4
    "Pa",           // 5
    "PSI",          // 6
    "mA",           // 7
    "V",            // 8
    "Hz",           // 9
    "s"             // 10
};

//=================================== Functions ================================

/* Configuration Type Functions */
const char* t3_get_cfgtype_name(uint32_t cfg_type)
{
    if (cfg_type > 0 && cfg_type < sizeof(cfgtype_names) / sizeof(cfgtype_names[0])) {
        return cfgtype_names[cfg_type];
    }
    return cfgtype_names[0]; // "Unknown"
}

bool t3_is_input_type(uint32_t cfg_type)
{
    return (cfg_type >= T3_CFGTYPE_BI && cfg_type <= T3_CFGTYPE_AI_0_5V);
}

bool t3_is_output_type(uint32_t cfg_type)
{
    return (cfg_type >= T3_CFGTYPE_BO && cfg_type <= T3_CFGTYPE_AO_0_100);
}

bool t3_is_variable_type(uint32_t cfg_type)
{
    return (cfg_type >= T3_CFGTYPE_VAR_INT && cfg_type <= T3_CFGTYPE_VAR_STRING);
}

uint8_t t3_cfgtype_to_bacnet_object(uint32_t cfg_type)
{
    switch (cfg_type) {
        case T3_CFGTYPE_BI:
            return T3_BACNET_BI;
        case T3_CFGTYPE_AI_0_100:
        case T3_CFGTYPE_AI_0_10V:
        case T3_CFGTYPE_AI_4_20MA:
        case T3_CFGTYPE_AI_NEG10_10V:
        case T3_CFGTYPE_AI_0_5V:
            return T3_BACNET_AI;
        case T3_CFGTYPE_BO:
            return T3_BACNET_BO;
        case T3_CFGTYPE_AO_0_10V:
        case T3_CFGTYPE_AO_4_20MA:
        case T3_CFGTYPE_AO_0_100:
            return T3_BACNET_AO;
        case T3_CFGTYPE_VAR_INT:
        case T3_CFGTYPE_VAR_FLOAT:
        case T3_CFGTYPE_VAR_STRING:
            return T3_BACNET_AV;
        default:
            return 255; // Invalid
    }
}

/* Validation Functions */
bool t3_is_valid_instance(uint32_t instance)
{
    return instance <= T3_MAX_INSTANCES;
}

bool t3_is_valid_cfgtype(uint32_t cfg_type)
{
    return cfg_type >= T3_CFGTYPE_BI && cfg_type <= T3_CFGTYPE_RESERVED;
}

bool t3_is_valid_input_instance(uint32_t instance)
{
    return t3_is_valid_instance(instance);
}

bool t3_is_valid_output_instance(uint32_t instance)
{
    return t3_is_valid_instance(instance);
}

bool t3_is_valid_variable_instance(uint32_t instance)
{
    return t3_is_valid_instance(instance);
}

/* SNMP-BACnet Mapping Functions */
bool t3_map_snmp_oid_to_bacnet(const char* snmp_oid, t3_snmp_bacnet_mapping_t *mapping)
{
    if (!snmp_oid || !mapping) {
        return false;
    }
    
    // Initialize mapping
    memset(mapping, 0, sizeof(t3_snmp_bacnet_mapping_t));
    
    // Parse OID to extract group, field, and instance
    // Expected format: 1.3.6.1.4.1.64991.1.<group>.<field>.<instance>
    
    const char* temco_pos = strstr(snmp_oid, enterprise_oid_str);
    if (!temco_pos) {
        ESP_LOGE(TAG, "OID missing %s prefix: %s", enterprise_oid_str, snmp_oid);
        return false;
    }
    
    // Skip "64991.1." to get to group
    const char* group_pos = temco_pos + strlen(enterprise_oid_str);
    if (strlen(group_pos) < 5) { // At least ".<group>"
        ESP_LOGE(TAG, "OID too short after %s.1: %s", enterprise_oid_str, snmp_oid);
        return false;
    }
    
    // Parse group
    uint32_t group = atoi(group_pos + 1);
    
    // Find field and instance
    const char* field_pos = strchr(group_pos + 1, '.');
    if (!field_pos) {
        ESP_LOGE(TAG, "OID missing field: %s", snmp_oid);
        return false;
    }
    
    const char* instance_pos = strchr(field_pos + 1, '.');
    if (!instance_pos) {
        ESP_LOGE(TAG, "OID missing instance: %s", snmp_oid);
        return false;
    }
    
    uint32_t field = atoi(field_pos + 1);
    uint32_t instance = atoi(instance_pos + 1);
    
    // Validate ranges
    if (!t3_is_valid_instance(instance)) {
        ESP_LOGE(TAG, "Invalid instance %u (max %d)", instance, T3_MAX_INSTANCES);
        return false;
    }

    // Set mapping values
    mapping->object_type = group;
    mapping->instance = instance;
    mapping->cfg_type = 0; // Would need to get from actual configuration
    mapping->is_valid = true;
    mapping->field = field;

    // Map to BACnet object type based on group and cfg_type
    switch (group)
    {
        case T3_OBJECT_INPUT:
            mapping->bacnet_type = t3_cfgtype_to_bacnet_object(T3_CFGTYPE_BI); // Default to BI
            break;
        case T3_OBJECT_OUTPUT:
            mapping->bacnet_type = t3_cfgtype_to_bacnet_object(T3_CFGTYPE_BO); // Default to BO
            break;
        case T3_OBJECT_VARIABLE:
            mapping->bacnet_type = t3_cfgtype_to_bacnet_object(T3_CFGTYPE_VAR_INT); // Default to Integer
            break;
        default:
            ESP_LOGE(TAG, "Invalid group %u", group);
            mapping->is_valid = false;
            return false;
    }

    //ESP_LOGI(TAG, "Mapped SNMP OID to BACnet: group=%u, field=%u, instance=%u, bacnet_type=%u", group, field, instance, mapping->bacnet_type);
    
    return true;
}

/* Data Access Functions */
int t3_read_input_value(uint32_t instance, uint32_t field, t3_data_value_t *value)
{
    if (!value || !t3_is_valid_instance(instance)) {
        return T3_ERROR_INVALID_INSTANCE;
    }
    
    memset(value, 0, sizeof(t3_data_value_t));

    Str_points_ptr ptr = put_io_buf(IN, instance);
    if (!ptr.pin) {
        return T3_ERROR_NOT_FOUND;
    }
    switch (field)
    {
        case T3_FIELD_INDEX:
        {
            value->int_value = (int32_t)instance;
            value->is_integer = true;
            //ESP_LOGI(TAG, "Read input %u index: %d", instance, value->int_value);
        }
        break;
       
        case T3_FIELD_CFGTYPE:
        {
            value->int_value = fetch_config_type(T3_OBJECT_INPUT, ptr.pin->digital_analog, ptr.pin->range);
            value->is_integer = true;
            //ESP_LOGI(TAG, "Read input %u cfgType: %d", instance, value->int_value);
        }
        break;
        
        case T3_FIELD_ANALOG:
        {
            float analog_val = (float)ptr.pin->value;
            value->analog_value = analog_val;
            value->float_value = analog_val;
            value->is_analog = true;
            value->is_float = true;
            //ESP_LOGI(TAG, "Read input %u analog value: %.2f", instance, analog_val);
        }
        break;
        
        case T3_FIELD_BINARY:
        {
            int32_t binary_val = ptr.pin->value;
            value->binary_value = binary_val;
            value->is_binary = true;
            //ESP_LOGI(TAG, "Read input %u binary value: %d %d", instance, binary_val, ptr.pin->value);
        }
        break;
        
        case T3_FIELD_DESC:
        {
            memcpy(value->string_value, ptr.pin->label, strlen((const char *)ptr.pin->label));
            value->is_string = true;
            //ESP_LOGI(TAG, "Read input %u string value: %s", instance, value->string_value);
        }
        break;
        
        case T3_FIELD_UNITS:
        {
            value->int_value = fetch_units_type(T3_OBJECT_INPUT, ptr.pin->digital_analog, ptr.pin->range);
            value->is_integer = true;
            //ESP_LOGI(TAG, "Read input %u unit value: %d", instance, value->int_value);
        }
        break;

        default:
        {
            ESP_LOGW(TAG, "Invalid field %u for input instance %u", field, instance);
            return T3_ERROR_INVALID_FIELD;
        }
    }
    return T3_SUCCESS;
}

int t3_read_output_value(uint32_t instance, uint32_t field, t3_data_value_t *value)
{
    if (!value || !t3_is_valid_instance(instance))
    {
        return T3_ERROR_INVALID_INSTANCE;
    }
    
    memset(value, 0, sizeof(t3_data_value_t));
    Str_points_ptr ptr = put_io_buf(OUT, instance);
    if (!ptr.pout) {
        return T3_ERROR_NOT_FOUND;
    }

    switch (field)
    {
        case T3_FIELD_INDEX:
        {
            value->int_value = (int32_t)instance;
            value->is_integer = true;
        }
        break;
        
        case T3_FIELD_CFGTYPE:
        {
            value->int_value = fetch_config_type(T3_OBJECT_INPUT, ptr.pin->digital_analog, ptr.pin->range);
            value->is_integer = true;
        }
        break;
        
        case T3_FIELD_ANALOG:
        {
            float analog_val = ptr.pout->value;
            value->analog_value = analog_val;
            value->is_analog = true;
            //ESP_LOGI(TAG, "Read output %u analog value: %.2f", instance, analog_val);
        }
        break;

        case T3_FIELD_BINARY:
        {
            int32_t binary_val = ptr.pout->value;
            value->binary_value = binary_val;
            value->is_binary = true;
            //ESP_LOGI(TAG, "Read output %u binary value: %d", instance, binary_val);
        }
        break;

        case T3_FIELD_DESC:
        {
            memcpy(value->string_value, ptr.pout->label, strlen((const char *)ptr.pout->label));
            value->is_string = true;
            //ESP_LOGI(TAG, "Read output %u string value: %s", instance, value->string_value);
        }
        break;
        
        case T3_FIELD_UNITS:
        {
            value->int_value = fetch_units_type(T3_OBJECT_OUTPUT, ptr.pout->digital_analog, ptr.pout->range);
            value->is_integer = true;
            //ESP_LOGI(TAG, "Read input %u unit value: %d", instance, value->int_value);
        }
        break;

        default:
            return T3_ERROR_INVALID_FIELD;
    }
    
    return T3_SUCCESS;
}

int t3_read_variable_value(uint32_t instance, uint32_t field, t3_data_value_t *value)
{
    if (!value || !t3_is_valid_instance(instance))
    {
        return T3_ERROR_INVALID_INSTANCE;
    }
    
    memset(value, 0, sizeof(t3_data_value_t));
    Str_points_ptr ptr = put_io_buf(VAR, instance);
    if (!ptr.pvar)
    {
        return T3_ERROR_NOT_FOUND;
    }
    
    switch (field)
    {
        case T3_FIELD_INDEX:
        {
            value->int_value = (int32_t)instance;
            value->is_integer = true;
        }
        break;
        
        case T3_FIELD_CFGTYPE:
        {
            value->int_value = fetch_config_type(T3_OBJECT_VARIABLE, ptr.pvar->digital_analog, ptr.pvar->range);
            value->is_integer = true;
        }
        break;
        
        case T3_FIELD_INTIGER:
        {
            value->int_value = (int32_t)TemcoVars_Present_Value(instance);
            value->is_integer = true;
        }
        break;
        
        case T3_FIELD_REAL:
        {
            value->float_value = TemcoVars_Present_Value(instance);
            value->is_float = true;
        }
        break;

        case T3_FIELD_DESC:
        {
            memcpy(value->string_value, ptr.pvar->label, strlen((const char *)ptr.pvar->label));
            value->is_string = true;
            ESP_LOGI(TAG, "Read variable %u string value: %s", instance, value->string_value);
        }
        break;
        
        case T3_FIELD_UNITS:
        {
            value->int_value = fetch_units_type(T3_OBJECT_VARIABLE, ptr.pvar->digital_analog, ptr.pvar->range);
            value->is_integer = true;
            ESP_LOGI(TAG, "Read variable %u unit value: %d", instance, value->int_value);
        }
        break;
        
        default:
        {
            return T3_ERROR_INVALID_FIELD;
        }
    }
    
    return T3_SUCCESS;
}

/* Write Functions */
int t3_write_output_value(uint32_t instance, uint32_t field, const t3_data_value_t *value)
{
    if (!value || !t3_is_valid_instance(instance))
    {
        return T3_ERROR_INVALID_INSTANCE;
    }

    uint8_t priority = 1; // Default priority
    Str_points_ptr ptr = put_io_buf(OUT, instance);
    if (!ptr.pout)
    {
        return T3_ERROR_NOT_FOUND;
    }

    switch (field)
    {
        case T3_FIELD_ANALOG:
        {
            if (value->is_analog)
            {
                ESP_LOGI(TAG, "Write output %u analog value: %.2f", instance, value->analog_value);
                ptr.pout->value = value->analog_value;
                return T3_SUCCESS;
            }
        }
        break;

        case T3_FIELD_BINARY:
        {
            if (value->is_binary)
            {
                ESP_LOGI(TAG, "Write output %u binary value: %d", instance, value->binary_value);
                ptr.pout->value = value->binary_value;
                return T3_SUCCESS;
            }
        }
        break;
        
        case T3_FIELD_DESC:
        {
            if (value->is_string)
            {
                ESP_LOGI(TAG, "Write output %u description: %s", instance, value->string_value);
                memset(ptr.pout->label, 0, sizeof(ptr.pout->label));
                memcpy(ptr.pout->label, value->string_value, strlen((const char *)value->string_value));
                return T3_SUCCESS;
            }
        }
        break;
        
        default:
        {
            return T3_ERROR_INVALID_FIELD;
        }
    }
    return T3_ERROR_TYPE_MISMATCH;
}

int t3_write_variable_value(uint32_t instance, uint32_t field, const t3_data_value_t *value)
{
    if (!value || !t3_is_valid_instance(instance))
    {
        return T3_ERROR_INVALID_INSTANCE;
    }
    
    uint8_t priority = 1; // Default priority
    Str_points_ptr ptr = put_io_buf(VAR, instance);
    if (!ptr.pvar)
    {
        return T3_ERROR_NOT_FOUND;
    }

    switch (field)
    {
        case T3_FIELD_INTIGER:
        {
            if (value->is_integer) {
                ESP_LOGI(TAG, "Write variable %u int value: %d", instance, value->int_value);
                if (!TemcoVars_Present_Value_Set(instance, (float)value->int_value, priority))
                {
                    return T3_ERROR_WRITE_ONLY;
                }
                return T3_SUCCESS;
            }
        }
        break;
        
        case T3_FIELD_REAL:
        {
            if (value->is_float)
            {
                ESP_LOGI(TAG, "Write variable %u float value: %.2f", instance, value->float_value);
                if (!TemcoVars_Present_Value_Set(instance, value->float_value, priority))
                {
                    return T3_ERROR_WRITE_ONLY;
                }
                return T3_SUCCESS;
            }
        }
        break;
        
        case T3_FIELD_DESC:
        {
            if (value->is_string)
            {
                ESP_LOGI(TAG, "Write variable %u description: %s", instance, value->string_value);
                memset(ptr.pvar->label, 0, sizeof(ptr.pvar->label));
                memcpy(ptr.pvar->label, value->string_value, strlen((const char *)value->string_value));
                return T3_SUCCESS;
            }
        }
        break;
        
        default:
        {
            return T3_ERROR_INVALID_FIELD;
        }
    }
    return T3_ERROR_TYPE_MISMATCH;
}

int fetch_config_type(t3_object_type_t type, int digital_analog, int range)
{
    if(type == T3_OBJECT_INPUT)
    {
       if(digital_analog == DIGITAL_VALUE)
        {
            return T3_CFGTYPE_BI;
        }
        else
        {
            if((range == V0_5) || (range == P0_100_0_5V))
            {
                return T3_CFGTYPE_AI_0_5V;
            }
            else if((range == V0_10_IN) || (range == P0_100_0_10V))
            {
                return T3_CFGTYPE_AI_0_10V;
            }
            else if((range == I0_20ma) || (range == P0_100_4_20ma))
            {
                return T3_CFGTYPE_AI_4_20MA;
            }
            else if(range == I0_100Amps)
            {
                return T3_CFGTYPE_AI_0_100;
            }
            else
            {
                return T3_CFGTYPE_AI_NEG10_10V;
            }
        }
    }
    else if(type == T3_OBJECT_OUTPUT)
    {
       if(digital_analog == DIGITAL_VALUE)
        {
            return T3_CFGTYPE_BO;
        }
        else
        {
            if(range == V0_10)
            {
                return T3_CFGTYPE_AO_0_10V;
            }
            else if(range == I_0_20ma)
            {
                return T3_CFGTYPE_AO_4_20MA;
            }
            else if(range == P0_20psi)
            {
                return T3_CFGTYPE_AO_4_20MA; //TBD: is it correct ? specs 4.1 says nothing
            }
            else
            {
                return T3_CFGTYPE_AO_0_100;
            }
        }
    }
    else if(type == T3_OBJECT_VARIABLE)
    {
        if(range == Sec || range == Hours || range == Days || range == Min)
		{
            return T3_CFGTYPE_VAR_INT;
        }
        else
        {
            return T3_CFGTYPE_VAR_FLOAT;
            //return T3_CFGTYPE_VAR_STRING;  //TBD: do we have any examples for string var ?
        }
    }
    return 0;
}

int fetch_units_type(t3_object_type_t type, int digital_analog, int range)
{
    if(digital_analog == DIGITAL_VALUE)
    {
        return T3_UNITS_NONE;
    }
    else if(type == T3_OBJECT_INPUT)
    {
        switch(range)
        {
            case Y3K_40_150DegC:
            case R10K_40_120DegC:
            case R3K_40_150DegC:
            case KM10K_40_120DegC:
            case PT1000_200_300DegC:
                return T3_UNITS_CELSIUS;
            case Y3K_40_300DegF:
            case R10K_40_250DegF:
            case R3K_40_300DegF:
            case KM10K_40_250DegF:
            case PT1000_200_570DegF:
                return T3_UNITS_FAHRENHEIT;
            case V0_5:
            case V0_10_IN:
            case P0_100_0_10V:
            case P0_100_0_5V:
                return T3_UNITS_VOLTS;
            case I0_20ma:
            case I0_100Amps:
            case P0_100_4_20ma:
                return T3_UNITS_MILLIAMPS;
            case Frequence:
                return T3_UNITS_HERTZ;
            default:
                return T3_UNITS_NONE;
        }
    }
    else if(type == T3_OBJECT_OUTPUT)
    {
        switch(range)
        {
            case V0_10:
            case P0_100_0_10V:
                return T3_UNITS_VOLTS;
            case I_0_20ma:
            case P0_100_4_20ma:
            case P0_20psi:
                return T3_UNITS_MILLIAMPS;
            case P0_100:
            case P0_100_Close:
            case P0_100_Open:
            case P0_100_PWM:
                return T3_UNITS_PERCENT;
            default:
                return T3_UNITS_NONE;
        }
    }
    else if(type == T3_OBJECT_VARIABLE)
    {
        switch(range)
        {
            case Sec:
            case Min:
            case Hours:
            case Days:
            case time_unit:
                return T3_UNITS_SECONDS;
            case degC:
                return T3_UNITS_CELSIUS;
            case degF:
                return T3_UNITS_FAHRENHEIT;
            case Volts:
            case KV:
                return T3_UNITS_VOLTS;
            case ma:
            case Amps:
                return T3_UNITS_MILLIAMPS;
            case Pa:
            case KPa:
                return T3_UNITS_PASCAL;
            case psi:
                return T3_UNITS_PSI;
            case procent:
                return T3_UNITS_PERCENT;
            default:
                return T3_UNITS_NONE;
        }
    }
    return T3_UNITS_NONE;
}