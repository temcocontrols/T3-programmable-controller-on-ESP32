#include "snmp_interface.h"
#include <string.h>
#include "esp_log.h"
#include "SnmpAgent.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "snmp_agent";

// SNMP agent configuration.
#define ENTERPRISE_OID  "P.38644.30"  // used as sysObjectID and in trap
#define RO_COMMUNITY    "public"				  
#define RW_COMMUNITY    "private"
#define TRAP_DST_ADDR   "192.168.31.118"  // Destination address for SNMP traps

char sysDescr[]    = "ESP32";
char sysContact[]  = "sysAdmin";
char sysName[]     = "hostName";
char sysLocation[] = "placeName";
unsigned char entOIDBer[MIB_DATA_SIZE];

uint32_t i, j;
char dInIndex[] = "P.38644.30.1.1.2.10";
unsigned char c, lastDIN;

static uint32_t data[32] = {0};
char trapDstAddr[] = TRAP_DST_ADDR;

void initMibTree();
int get_uptime(MIB *thismib);
int get_ain(MIB *thismib);
int set_dio(MIB *thismib, void *ptr, int len);
int get_dio(MIB *thismib);

/**
 * Initialize the SNMP agent subsystem.
 */
extern void snmp_agent_init(void);

static void snmp_agent_task(void *pvParameters)
{
	vTaskDelay(5000 / portTICK_PERIOD_MS); // wait for network to be ready
	initSnmpAgent(SNMP_PORT, ENTERPRISE_OID, RO_COMMUNITY, RW_COMMUNITY);
	initMibTree();

	static const uint32_t my_trap_oid[] = {1,3,6,1,4,1,9999,1,0};
    snmp_send_v2c_trap(TRAP_DST_ADDR, "public", my_trap_oid, 9, esp_log_timestamp());

	for (;;)
	{
		if (processSNMP() == COMM_STR_MISMATCH) {
			;
		}
	}
}

void snmp_agent_init(void)
{
    ESP_LOGI(TAG, "SNMP agent initialized");
	xTaskCreate(snmp_agent_task, "snmp_agent_task", 4096, NULL, 5, NULL);
}

void initMibTree()
{
	MIB *thismib;

	/* System MIB */

	// sysDescr Entry
	thismib = miblistadd(mibTree, "B.1.1.0", OCTET_STRING, RD_ONLY, sysDescr, strlen(sysDescr));

	// sysObjectID Entry
	thismib = miblistadd(mibTree, "B.1.2.0", OBJECT_IDENTIFIER, RD_ONLY,  entOIDBer, 0); // set length to 0 first
	i = str2ber(enterpriseOID, entOIDBer);
	mibsetvalue(thismib, (void *)entOIDBer, (int)i); // proper length set

	// sysUptime Entry
	thismib = miblistadd(mibTree, "B.1.3.0", TIMETICKS, RD_ONLY, NULL, 0);
	i = 0;
	mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_uptime, NULL);

	// sysContact Entry
	thismib = miblistadd(mibTree, "B.1.4.0", OCTET_STRING, RD_WR, sysContact, strlen(sysContact));

	// sysName Entry
	thismib = miblistadd(mibTree, "B.1.5.0", OCTET_STRING, RD_WR, sysName, strlen(sysName));

	// sysLocation Entry
	thismib = miblistadd(mibTree, "B.1.6.0", OCTET_STRING, RD_WR, sysLocation, strlen(sysLocation));

	// sysServices Entry
	thismib = miblistadd(mibTree, "B.1.7.0", INTEGER, RD_ONLY, NULL, 0);
	i = 5;
	mibsetvalue(thismib, &i, 0);

	/* inputs */

	// Digital input #16 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.16", INTEGER, RD_ONLY, NULL, 0);
	i = 16;
	mibsetvalue(thismib, &i, 0);

	// The value of Digital #16
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.16", INTEGER, RD_ONLY, NULL, 0);
	i = 0;
	mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #17 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.17", INTEGER, RD_ONLY, NULL, 0);
	i = 17;
	mibsetvalue(thismib, &i, 0);
	// The value of Digital #17
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.17", INTEGER, RD_ONLY, NULL, 0);
	i = 0;
	mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #18 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.18", INTEGER, RD_ONLY, NULL, 0);
	i = 18;
	mibsetvalue(thismib, &i, 0);
	// The value of Digital #18
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.18", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	// Digital input #19 index
	thismib = miblistadd(mibTree, "P.38644.30.1.1.1.19", INTEGER, RD_ONLY, NULL, 0);
	i = 19;
	mibsetvalue(thismib, &i, 0);
	// The value of Digital #19
	thismib = miblistadd(mibTree, "P.38644.30.1.1.2.19", INTEGER, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, NULL);

	/* GPIO21-23 are designated for digital outputs. */

	// Digital output #21 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.21", INTEGER, RD_ONLY, NULL, 0);
	i = 21;
	mibsetvalue(thismib, &i, 0);
	// The value of Digital #21
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.21", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	// Digital output #22 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.22", INTEGER, RD_ONLY, NULL, 0);
	i = 22;
	mibsetvalue(thismib, &i, 0);
	// The value of Digital #22
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.22", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	// Digital output #23 index
	thismib = miblistadd(mibTree, "P.38644.30.2.1.1.23", INTEGER, RD_ONLY, NULL, 0);
	i = 23;
	mibsetvalue(thismib, &i, 0);
	// The value of Digital #23
	thismib = miblistadd(mibTree, "P.38644.30.2.1.2.23", INTEGER, RD_WR, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_dio, set_dio);

	/* GPIO33-35 are designated for analog inputs. */

	// Analog input #33 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.33", INTEGER, RD_ONLY, NULL, 0);
	i = 33;
	mibsetvalue(thismib, &i, 0);
	
	// The value of Analog #25
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.33", GAUGE, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_ain, NULL);

	// Analog input #34 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.34", INTEGER, RD_ONLY, NULL, 0);
	i = 34;
	mibsetvalue(thismib, &i, 0);
	
	// The value of Analog #26
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.34", GAUGE, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_ain, NULL);

	// Analog input #35 index
	thismib = miblistadd(mibTree, "P.38644.30.3.1.1.35", INTEGER, RD_ONLY, NULL, 0);
	i = 35;
	mibsetvalue(thismib, &i, 0);
	
	// The value of Analog #27
	thismib = miblistadd(mibTree, "P.38644.30.3.1.2.35", GAUGE, RD_ONLY, NULL, 0);
	i = 0; mibsetvalue(thismib, &i, 0);
	mibsetcallback(thismib, get_ain, NULL);
}

int get_uptime(MIB *thismib)
{
	thismib->u.intval = sysUpTime();
	return SUCCESS;
}

int get_dio(MIB *thismib)
{
	c = thismib->oid.array[thismib->oid.len-1];
	j = (uint32_t) data[c];
	thismib->u.intval = j;
	return SUCCESS;
}

int set_dio(MIB *thismib, void *ptr, int len)
{
	c = thismib->oid.array[thismib->oid.len-1];
	j = *(uint32_t *)ptr;
	// if ( j!=0 && j!=1 )
	// 	return ILLEGAL_DATA;
	// else
	data[c] = j;
	thismib->u.intval = j;
	return SUCCESS;
}

int get_ain(MIB *thismib)
{
	c = thismib->oid.array[thismib->oid.len-1];
	thismib->u.intval = data[c];
	return SUCCESS;
}
