//===============================================================================
// snmp_interface.c
// Implements a simple SNMP agent for ESP32 using uSNMP core functions.
//===============================================================================

//=================================== Includes ==================================
#include <stdio.h>
#include <string.h>
#include "snmp_interface.h"
#include "app_log.h"
#include "esp_log.h"
#include "snmp_agent.h"
#include "t3_snmp_bacnet_mapping.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi.h"

//=================================== Defines ===================================
// SNMP agent configuration.
#define RO_COMMUNITY    "public"				  
#define RW_COMMUNITY    "private"

//=================================== Variables =================================

// Log tag
static const char *TAG = "snmp_agent";

// System MIB entries
char sysDescr[64]     = "TEMCO-ESP32";
char sysContact[32]   = "PLC Group";
char sysName[32]      = "T3 Programmable Controller";
char sysLocation[32]  = "Temco Controls Ltd";
char sysVersion[32]   = "1.0";
char sysIpAddress[16] = "192.168.1.17";

char description[21] = "";

// SNMP MIB entries
unsigned char entOIDBer[MIB_DATA_SIZE];

// SNMP trap destination
char trapDstAddr[16] = TRAP_DST_ADDR;

// MIB tree
MIB *thismib = NULL;

// Temporary variables
uint32_t u32;

static bool snmp_started = false;

//================================= Functions ================================
void initMibTree();
static void init_standard_mibs(void);
static void init_private_mibs(void);

int get_uptime(MIB *thismib);
int get_ipaddress(MIB *thismib);

// T3 SNMP MIB Read Callbacks
int t3_read_output(MIB *thismib);
int t3_read_input(MIB *thismib);
int t3_read_variable(MIB *thismib);

// T3 SNMP MIB Write Callbacks
int t3_write_output(MIB *thismib, void *ptr, int len);
int t3_write_variable(MIB *thismib, void *ptr, int len);

void snmp_agent_init(void);
void update_system_ip_address(void);

//============================= Function Definitions ===========================

//---------------------------------------------------
// SNMP Agent Initialization
//---------------------------------------------------
void snmp_app_init(void)
{
    update_system_ip_address();
    if(snmp_started)
    {
        app_log(TAG, "SNMP agent already running.");
        return;
    }
    app_log(TAG, "Initializing SNMP agent.");
    snmp_agent_init();
}

//---------------------------------------------------
// SNMP Agent Task
//---------------------------------------------------
static void snmp_agent_task(void *pvParameters)
{
    vTaskDelay(30000 / portTICK_PERIOD_MS); // wait for network to be ready

    // Initialize SNMP agent framework
    initSnmpAgent(SNMP_PORT, ENTERPRISE_OID, RO_COMMUNITY, RW_COMMUNITY);

    // Initialize MIB tree
    app_log(TAG, "Initializing MIB tree...");
    initMibTree();
    app_log(TAG, "SNMP agent ready...");

    // Send SNMP cold start trap
    app_log(TAG, "Sending SNMP cold start trap to %s", trapDstAddr);
    send_snmp_trap_cold_start(sysIpAddress, trapDstAddr);

    // Send autonomous trap after startup
    app_log(TAG, "Sending SNMP autonomous notification trap to %s", trapDstAddr);
    send_master_trap_autonomous_notification(sysIpAddress, trapDstAddr, sysLocation, 1, 1, "System Cold Start", 0);

    // Send custom trap
    // snmp_trap_enterprise(sysIpAddress, trapDstAddr, "this is custom trap!!!");

    // Process SNMP requests
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(10));
        if (processSNMP() == COMM_STR_MISMATCH)
        {
            ;
        }
    }
}

//---------------------------------------------------
// SNMP Agent Initialization Function
//---------------------------------------------------
void snmp_agent_init(void)
{
    xTaskCreate(snmp_agent_task, "snmp_agent_task", 4096 + 4096, NULL, 5, NULL);
}

// Initialize MIB tree
void initMibTree()
{
    // Initialize standard MIBs
    init_standard_mibs();

    //check_heap_fragmentation();

    // Initialize private MIBs
    init_private_mibs();
}

static void init_standard_mibs(void)
{
    /*---------------- Setup System MIB ---------------*/
    /*-----     B denotes Mgmt-Mib2   - 1.3.6.1.2.1    */
    /*-------------------------------------------------*/

    // sysDescr Entry
    thismib = miblistadd(mibTree, "B.1.1.0", OCTET_STRING, RD_ONLY, sysDescr, strlen(sysDescr));

    // sysObjectID Entry
    thismib = miblistadd(mibTree, "B.1.2.0", OBJECT_IDENTIFIER, RD_ONLY,  entOIDBer, 0); // set length to 0 first
    u32 = str2ber(enterpriseOID, entOIDBer);
    mibsetvalue(thismib, (void *)entOIDBer, (int)u32); // proper length set

    // sysUptime Entry
    u32 = 0;
    thismib = miblistadd(mibTree, "B.1.3.0", TIMETICKS, RD_ONLY, NULL, 0);
    mibsetvalue(thismib, &u32, 0);
    mibsetcallback(thismib, get_uptime, NULL);

    // sysContact Entry
    thismib = miblistadd(mibTree, "B.1.4.0", OCTET_STRING, RD_WR, sysContact, strlen(sysContact));

    // sysIpAddrTable
    thismib = miblistadd(mibTree, "B.1.4.20", IP_ADDRESS, RD_ONLY, sysIpAddress, strlen(sysIpAddress));

    // sysName Entry
    thismib = miblistadd(mibTree, "B.1.5.0", OCTET_STRING, RD_WR, sysName, strlen(sysName));

    // sysLocation Entry
    thismib = miblistadd(mibTree, "B.1.6.0", OCTET_STRING, RD_WR, sysLocation, strlen(sysLocation));

    // sysServices Entry
    thismib = miblistadd(mibTree, "B.1.7.0", INTEGER, RD_ONLY, NULL, 0);
    u32 = 5;
    mibsetvalue(thismib, &u32, 0);
}

static void init_private_mibs(void)
{
    char oid_buffer[64] = {0};

    /*---------------- Setup Private MIB --------------*/
    /* P denotes Private-Enterprises - 1.3.6.1.4.1     */
    /* The T3 controller exposes three primary object  */
    /* categories through SNMP                         */
    /*-------------------------------------------------*/

    app_log(TAG, "Initializing T3 SNMP MIB tree (instances=%d)", T3_MAX_INSTANCES);

    /* --- T3 Input Objects (Read-Only) --- */
    for (int idx = 0; idx < T3_MAX_INSTANCES; ++idx)
    {
        /* index (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_INPUTS_OID_BASE, T3_INPUT_INDEX_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = idx;
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_input, NULL);
            //app_log(TAG, "Added MIB for Input index %d: OID=%s", idx, oid_buffer);
        }

        /* cfgType (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_INPUTS_OID_BASE, T3_INPUT_CFGTYPE_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = T3_CFGTYPE_BI; // Default to binary input
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_input, NULL);
            //app_log(TAG, "Added MIB for Input cfgType %d: OID=%s", idx, oid_buffer);
        }

        /* analogVal (REAL, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_INPUTS_OID_BASE, T3_INPUT_ANALOG_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            mibsetcallback(thismib, t3_read_input, NULL);
            //app_log(TAG, "Added MIB for Input analogVal %d: OID=%s", idx, oid_buffer);
        }

        /* binaryVal (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_INPUTS_OID_BASE, T3_INPUT_BINARY_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            mibsetcallback(thismib, t3_read_input, NULL);
            //app_log(TAG, "Added MIB for Input binaryVal %d: OID=%s", idx, oid_buffer);
        }

        /* desc (OCTET_STRING, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_INPUTS_OID_BASE, T3_INPUT_DESC_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, OCTET_STRING, RD_ONLY, description, strlen(description));
        if (thismib)
        {
            mibsetvalue(thismib, "", 0);
            mibsetcallback(thismib, t3_read_input, NULL);
            //app_log(TAG, "Added MIB for Input desc %d: OID=%s", idx, oid_buffer);
        }

        /* units (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_INPUTS_OID_BASE, T3_INPUT_UNITS_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = T3_UNITS_NONE;
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_input, NULL);
            //app_log(TAG, "Added MIB for Input units %d: OID=%s", idx, oid_buffer);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // small delay to avoid watchdog reset
    }
    //app_log(TAG, "Input Init done. heap: ");
    //check_heap_fragmentation();
    vTaskDelay(pdMS_TO_TICKS(1000));

    /* --- T3 Output Objects (Read-Write) --- */
    for (int idx = 0; idx < T3_MAX_INSTANCES; ++idx)
    {
        /* index (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_OUTPUTS_OID_BASE, T3_OUTPUT_INDEX_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = idx;
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_output, NULL);
        }

        /* cfgType (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_OUTPUTS_OID_BASE, T3_OUTPUT_CFGTYPE_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = T3_CFGTYPE_BO; // Default to binary output
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_output, NULL);
        }

        /* analogVal (INTEGER, RD_WR) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_OUTPUTS_OID_BASE, T3_OUTPUT_ANALOG_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_WR, NULL, 0);
        if (thismib)
        {
            mibsetcallback(thismib, t3_read_output, t3_write_output);
        }

        /* binaryVal (INTEGER, RD_WR) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_OUTPUTS_OID_BASE, T3_OUTPUT_BINARY_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_WR, NULL, 0);
        if (thismib)
        {
            mibsetcallback(thismib, t3_read_output, t3_write_output);
        }

        /* desc (OCTET_STRING, RD_WR) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_OUTPUTS_OID_BASE, T3_OUTPUT_DESC_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, OCTET_STRING, RD_WR, description, sizeof(description));
        if (thismib)
        {
            mibsetvalue(thismib, "", 0);
            mibsetcallback(thismib, t3_read_output, t3_write_output);
        }

        /* units (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_OUTPUTS_OID_BASE, T3_OUTPUT_UNITS_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = T3_UNITS_NONE;
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_output, NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // small delay to avoid watchdog reset
    }

    /* --- T3 Variable Objects (Read-Write) --- */
    for (int idx = 0; idx < T3_MAX_INSTANCES; ++idx)
    {
        /* index (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_VARIABLES_OID_BASE, T3_VARIABLE_INDEX_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = idx;
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_variable, NULL);
        }

        /* cfgType (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_VARIABLES_OID_BASE, T3_VARIABLE_CFGTYPE_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = T3_CFGTYPE_VAR_FLOAT; // Default to float variable
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_variable, NULL);
        }

        /* intVal (INTEGER, RD_WR) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_VARIABLES_OID_BASE, T3_VARIABLE_INT_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_WR, NULL, 0);
        if (thismib)
        {
            mibsetcallback(thismib, t3_read_variable, t3_write_variable);
        }

        /* floatVal (REAL, RD_WR) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_VARIABLES_OID_BASE, T3_VARIABLE_FLOAT_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_WR, NULL, 0);
        if (thismib)
        {
            mibsetcallback(thismib, t3_read_variable, t3_write_variable);
        }

        /* desc (OCTET_STRING, RD_WR) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_VARIABLES_OID_BASE, T3_VARIABLE_DESC_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, OCTET_STRING, RD_WR, description, sizeof(description));
        if (thismib)
        {
            mibsetvalue(thismib, "", 0);
            mibsetcallback(thismib, t3_read_variable, t3_write_variable);
        }

        /* units (INTEGER, RD_ONLY) */
        snprintf(oid_buffer, sizeof(oid_buffer), "%s%s.%u", T3_VARIABLES_OID_BASE, T3_VARIABLE_UNITS_OID, idx);
        thismib = miblistadd(mibTree, oid_buffer, INTEGER, RD_ONLY, NULL, 0);
        if (thismib)
        {
            u32 = T3_UNITS_NONE;
            mibsetvalue(thismib, &u32, 0);
            mibsetcallback(thismib, t3_read_variable, NULL);
        }
        vTaskDelay(pdMS_TO_TICKS(10)); // small delay to avoid watchdog reset
    }
	app_log(TAG, "init private Mib tree complete.");
}

char* miboid_to_string(OID *oid, char *buffer, size_t buflen)
{
	if (!oid || !buffer || buflen == 0)
    {
		return NULL;
	}

#if 0
	//print OID as string
	app_log(TAG, "OID Length: %u", oid->len);
	for (unsigned char i = 0; i < oid->len; ++i)
	{
		app_log(TAG, "  OID[%u]: %u", i, oid->array[i]);
	}
#endif

	size_t offset = 0;
	for (unsigned char i = 0; i < oid->len; ++i)
	{
		int written = snprintf(buffer + offset, buflen - offset, "%s%u", (i == 0) ? "" : ".", oid->array[i]);
		if (written < 0 || (size_t)written >= buflen - offset)
        {
			// Truncated or error
			break;
		}
		offset += (size_t)written;
	}
	return buffer;
}

//---------------------------------------------------
// T3 SNMP Read/Write Callbacks
//---------------------------------------------------

/* Read callback for T3 Input Objects */
int t3_read_input(MIB *thismib)
{
    if(thismib == NULL)
    {
        return NO_SUCH_NAME;
    }
	char oid_buffer[128] = {0};
	t3_snmp_bacnet_mapping_t mapping;
    char* snmp_oid = miboid_to_string(&thismib->oid, oid_buffer, sizeof(oid_buffer));
	//app_log(TAG, "Read input for OID: %s", snmp_oid);

    // Get mapping
    if (!t3_map_snmp_oid_to_bacnet(snmp_oid, &mapping))
    {
		ESP_LOGE(TAG, "No mapping for OID: %s", snmp_oid);
        return NO_SUCH_NAME;
    }
    //app_log(TAG, "Read Input : instance [%d], field [%d], object_type [%d], bacnet_type [%d]",
    //    mapping.instance, mapping.field, mapping.object_type, mapping.bacnet_type);

    // Read value
    t3_data_value_t value;
    uint32_t instance = mapping.instance;
    uint32_t field = mapping.field;
    int result = t3_read_input_value(instance, field, &value);
    if (result == T3_SUCCESS)
    {
        if (field == T3_FIELD_INDEX)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_CFGTYPE  && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_ANALOG && value.is_analog)
        {
            thismib->u.intval = value.analog_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_BINARY && value.is_binary)
        {
            thismib->u.intval = value.binary_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_DESC && value.is_string)
        {
            mibsetvalue(thismib, value.string_value, strlen(value.string_value));
            return SUCCESS;
        }
        else if (field == T3_FIELD_UNITS && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
    }
    return NO_SUCH_NAME;
}

/* Read callback for T3 Output Objects */
int t3_read_output(MIB *thismib)
{
    char oid_buffer[128] = {0};
    char* snmp_oid = miboid_to_string(&thismib->oid, oid_buffer, sizeof(oid_buffer));
    //app_log(TAG, "Read output for OID: %s", snmp_oid);

    // Get mapping
    t3_snmp_bacnet_mapping_t mapping;
    if (!t3_map_snmp_oid_to_bacnet(snmp_oid, &mapping))
    {
        return NO_SUCH_NAME;
    }
    
    // Read value
    t3_data_value_t value;
    uint32_t instance = mapping.instance;
    uint32_t field = mapping.field;
    int result = t3_read_output_value(instance, field, &value);

    if (result == T3_SUCCESS)
    {
        if (field == T3_FIELD_INDEX && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_CFGTYPE && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }        
        else if (field == T3_FIELD_ANALOG && value.is_analog)
        {
            thismib->u.intval = value.analog_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_BINARY && value.is_binary)
        {
            thismib->u.intval = value.binary_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_DESC && value.is_string)
        {
            mibsetvalue(thismib, value.string_value, strlen(value.string_value));
            return SUCCESS;
        }
        else if (field == T3_FIELD_UNITS && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
    }
    return NO_SUCH_NAME;
}

/* Read callback for T3 Variable Objects */
int t3_read_variable(MIB *thismib)
{
    char oid_buffer[128] = {0};
    char* snmp_oid = miboid_to_string(&thismib->oid, oid_buffer, sizeof(oid_buffer));
    //app_log(TAG, "Read variable for OID: %s", snmp_oid);

    // Get mapping
    t3_snmp_bacnet_mapping_t mapping;
    if (!t3_map_snmp_oid_to_bacnet(snmp_oid, &mapping))
    {
        return NO_SUCH_NAME;
    }
    
    // Read value
    t3_data_value_t value;
    uint32_t instance = mapping.instance;
    uint32_t field = mapping.field;
    int result = t3_read_variable_value(instance, field, &value);
    if (result == T3_SUCCESS)
    {
        if (field == T3_FIELD_INDEX && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_CFGTYPE && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }        
        else if (field == T3_FIELD_INTIGER && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_REAL && value.is_float)
        {
            thismib->u.intval = value.float_value;
            return SUCCESS;
        }
        else if (field == T3_FIELD_DESC && value.is_string)
        {
            mibsetvalue(thismib, value.string_value, strlen(value.string_value));
            return SUCCESS;
        }
        else if (field == T3_FIELD_UNITS && value.is_integer)
        {
            thismib->u.intval = value.int_value;
            return SUCCESS;
        }
    }
    return NO_SUCH_NAME;
}

/* Write callback for T3 Output Objects */
int t3_write_output(MIB *thismib, void *ptr, int len)
{
    if(thismib == NULL || ptr == NULL)
    {
        return BAD_VALUE;
    }
    char oid_buffer[128] = {0};
    char* snmp_oid = miboid_to_string(&thismib->oid, oid_buffer, sizeof(oid_buffer));
    //app_log(TAG, "Write output for OID: %s value %d", snmp_oid, thismib->u.intval);

    // Get mapping
    t3_snmp_bacnet_mapping_t mapping;
    if (!t3_map_snmp_oid_to_bacnet(snmp_oid, &mapping))
    {
        return BAD_VALUE;
    }
    
    // Write value
    t3_data_value_t value;
    uint32_t instance = mapping.instance;
    uint32_t field = mapping.field;
    memset(&value, 0, sizeof(t3_data_value_t));
    //app_log(TAG, "Write Output : instance [%d], field [%d]", instance, field);
    // Prepare value based on data type
    if (field == T3_FIELD_ANALOG && thismib->dataType == INTEGER)
    {
        int32_t intval = *(int32_t *)ptr;
        //app_log(TAG, "Write output %u analog value: %d", instance, intval);
        value.analog_value = intval;
        value.float_value = intval;
        value.is_analog = true;
        value.is_float = true;
    }
    else if (field == T3_FIELD_BINARY  && thismib->dataType == INTEGER)
    {
        uint32_t intval = *(uint32_t *)ptr;
        //app_log(TAG, "Write output %u binary value: %d", instance, intval);
        value.binary_value = intval;
        value.is_binary = true;
    }
    else if (field == T3_FIELD_DESC && thismib->dataType == OCTET_STRING)
    {
        //app_log(TAG, "Write output %u string value: %s", instance, thismib->u.octetstring);
        memset(value.string_value, 0, sizeof(value.string_value));
        memcpy(value.string_value, ptr, len);
        value.is_string = true;
    }
    else
    {
        return BAD_VALUE;
    }
    
    int result = t3_write_output_value(instance, field, &value);
    return (result == T3_SUCCESS) ? SUCCESS : BAD_VALUE;
}

/* Write callback for T3 Variable Objects */
int t3_write_variable(MIB *thismib, void *ptr, int len)
{
    if(thismib == NULL || ptr == NULL)
    {
        return BAD_VALUE;
    }
    char oid_buffer[128] = {0};
    char* snmp_oid = miboid_to_string(&thismib->oid, oid_buffer, sizeof(oid_buffer));
    //app_log(TAG, "Write variable for OID: %s", snmp_oid);

    t3_snmp_bacnet_mapping_t mapping;
    if (!t3_map_snmp_oid_to_bacnet(snmp_oid, &mapping))
    {
        return BAD_VALUE;
    }
    
    t3_data_value_t value;
    uint32_t instance = mapping.instance;
    uint32_t field = mapping.field;
    memset(&value, 0, sizeof(t3_data_value_t));
    
    // Prepare value based on data type
    if (field == T3_FIELD_INTIGER && thismib->dataType == INTEGER)
    {
        int32_t intval = *(int32_t *)ptr;
        value.int_value = intval;
        value.is_integer = true;
    }
    else if (field == T3_FIELD_REAL && thismib->dataType == INTEGER)
    {
        int32_t intval = *(int32_t *)ptr;
        value.float_value = intval;
        value.is_float = true;
    }
    else if (field == T3_FIELD_DESC && thismib->dataType == OCTET_STRING)
    {
        memset(value.string_value, 0, sizeof(value.string_value));
        memcpy(value.string_value, ptr, len);
        value.is_string = true;
    }
    else
    {
        return BAD_VALUE;
    }
    int result = t3_write_variable_value(instance, field, &value);
    return (result == T3_SUCCESS) ? SUCCESS : BAD_VALUE;
}

/* Description read callback */
int get_uptime(MIB *thismib)
{
    thismib->u.intval = sysUpTime();
    return SUCCESS;
}

int get_ipaddress(MIB *thismib)
{
    thismib->u.octetstring = (unsigned char *)sysIpAddress;
    thismib->dataLen = strlen(sysIpAddress);
    return SUCCESS;
}

void update_system_ip_address(void)
{
    memset(sysIpAddress, 0, sizeof(sysIpAddress));
    snprintf(sysIpAddress, sizeof(sysIpAddress),  "%d.%d.%d.%d", SSID_Info.ip_addr[0],SSID_Info.ip_addr[1],SSID_Info.ip_addr[2],SSID_Info.ip_addr[3]);
    sysIpAddress[sizeof(sysIpAddress) - 1] = '\0'; // Ensure null-termination
    app_log(TAG, "Updated SNMP system IP address: %s", sysIpAddress);
}

void update_system_location(const char* location)
{
    if (location)
    {
        strncpy(sysLocation, location, sizeof(sysLocation) - 1);
        sysLocation[sizeof(sysLocation) - 1] = '\0'; // Ensure null-termination
        app_log(TAG, "Updated SNMP system location: %s", sysLocation);
    }
}

void update_system_name(const char* name)
{
    if (name)
    {
        strncpy(sysName, name, sizeof(sysName) - 1);
        sysName[sizeof(sysName) - 1] = '\0'; // Ensure null-termination
        app_log(TAG, "Updated SNMP system name: %s", sysName);
    }
}

void update_system_description(const char* description)
{
    if (description)
    {
        strncpy(sysDescr, description, sizeof(description) - 1);
        sysDescr[sizeof(description) - 1] = '\0'; // Ensure null-termination
        app_log(TAG, "Updated SNMP system description: %s", sysDescr);
    }
}

void update_system_contact(const char* contact)
{
    if (contact)
    {
        strncpy(sysContact, contact, sizeof(sysContact) - 1);
        sysContact[sizeof(sysContact) - 1] = '\0'; // Ensure null-termination
        app_log(TAG, "Updated SNMP system contact: %s", sysContact);
    }
}

void update_system_version(const char* version)
{
    if (version)
    {
        strncpy(sysVersion, version, sizeof(sysVersion) - 1);
        sysVersion[sizeof(sysVersion) - 1] = '\0'; // Ensure null-termination
        app_log(TAG, "Updated SNMP system version: %s", sysVersion);
    }
}

void update_trap_destination(const char* dest_ip)
{
    if (dest_ip)
    {
        strncpy(trapDstAddr, dest_ip, sizeof(trapDstAddr) - 1);
        trapDstAddr[sizeof(trapDstAddr) - 1] = '\0'; // Ensure null-termination
        app_log(TAG, "Updated SNMP trap destination: %s", trapDstAddr);
    }
}

// --- Master Trap Implementation ---
void send_master_trap_autonomous_notification(const char* local_ip, const char* dst_ip,
                                              const char* site_location,
                                              uint32_t event_id,
                                              uint8_t event_set,
                                              const char* alarm_type,
                                              int32_t alarm_value)
{
    // Master Trap OID: 1.3.6.1.4.1.<OFFICIAL_IANA_PAN>.1.0.1 (t3AlarmNotification)
    uint32_t trap_oid[] = {1, 3, 6, 1, 4, 1, OFFICIAL_IANA_PAN, 1, 0, 1};
    
    // Create persistent storage for integer values (must survive function call)
    int32_t int_values[3];
    int_values[0] = (int32_t)event_id;      // eventID
    int_values[1] = (int32_t)event_set;     // eventSet
    int_values[2] = (int32_t)alarm_value;   // alarmValue
    
    // Prepare varbinds for the trap payload
    FlexibleVarbind varbinds[5];
    int varbind_count = 0;
    
    // 1. sysLocation (1.3.6.1.2.1.1.6.0)
    static uint32_t oid_sysLocation[] = {1, 3, 6, 1, 2, 1, 1, 6, 0};
    varbinds[varbind_count].oid = oid_sysLocation;
    varbinds[varbind_count].oid_len = 9;
    varbinds[varbind_count].data_type = VARBIND_TYPE_OCTET_STRING;
    varbinds[varbind_count].value = (void*)site_location;
    varbinds[varbind_count].value_len = strlen(site_location);
    varbind_count++;
    
    // 2. eventID (1.3.6.1.4.1.<OFFICIAL_IANA_PAN>.1.4.1)
    static uint32_t oid_eventID[] = {1, 3, 6, 1, 4, 1, OFFICIAL_IANA_PAN, 1, 4, 1};
    varbinds[varbind_count].oid = oid_eventID;
    varbinds[varbind_count].oid_len = 10;
    varbinds[varbind_count].data_type = VARBIND_TYPE_INTEGER;
    varbinds[varbind_count].value = (void*)&int_values[0];
    varbinds[varbind_count].value_len = 0;
    varbind_count++;
    
    // 3. eventSet (1.3.6.1.4.1.<OFFICIAL_IANA_PAN>.1.4.2): 1 = Active, 0 = Cleared
    static uint32_t oid_eventSet[] = {1, 3, 6, 1, 4, 1, OFFICIAL_IANA_PAN, 1, 4, 2};
    varbinds[varbind_count].oid = oid_eventSet;
    varbinds[varbind_count].oid_len = 10;
    varbinds[varbind_count].data_type = VARBIND_TYPE_INTEGER;
    varbinds[varbind_count].value = (void*)&int_values[1];
    varbinds[varbind_count].value_len = 0;
    varbind_count++;
    
    // 4. alarmType (1.3.6.1.4.1.<OFFICIAL_IANA_PAN>.1.4.3): Contextual string
    static uint32_t oid_alarmType[] = {1, 3, 6, 1, 4, 1, OFFICIAL_IANA_PAN, 1, 4, 3};
    varbinds[varbind_count].oid = oid_alarmType;
    varbinds[varbind_count].oid_len = 10;
    varbinds[varbind_count].data_type = VARBIND_TYPE_OCTET_STRING;
    varbinds[varbind_count].value = (void*)alarm_type;
    varbinds[varbind_count].value_len = strlen(alarm_type);
    varbind_count++;
    
    // 5. alarmValue (1.3.6.1.4.1.<OFFICIAL_IANA_PAN>.1.4.4): Numeric value
    static uint32_t oid_alarmValue[] = {1, 3, 6, 1, 4, 1, OFFICIAL_IANA_PAN, 1, 4, 4};
    varbinds[varbind_count].oid = oid_alarmValue;
    varbinds[varbind_count].oid_len = 10;
    varbinds[varbind_count].data_type = VARBIND_TYPE_INTEGER;
    varbinds[varbind_count].value = (void*)&int_values[2];
    varbinds[varbind_count].value_len = 0;
    varbind_count++;
    
    // Send the master trap
    // Note: sysUpTime and sysTime are automatically included by snmp_send_v2_flexible
    snmp_send_v2_flexible(local_ip, dst_ip, trap_oid, 10, varbinds, varbind_count, 0);
}