#include "unity.h"
#include "define.h"
#include "controls.h"
#include "bacnet.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// Extern definitions for user_data.c
extern int16_t find_local_point(Point *point);
extern int16_t find_remote_point(Point_Net *point);
extern int16_t find_network_point(Point_Net *point);
extern int16_t insert_local_point(Point *point, int16_t index);
extern int16_t insert_remote_point(Point_Net *point, int16_t index);
extern int16_t insert_network_point(Point_Net *point, int16_t index);
extern int16_t get_point_value(Point *point, int32_t *val_ptr);
extern int16_t put_point_value(Point *point, int32_t *val_ptr, int16_t aux, int16_t prog_op);
extern int16_t get_net_point_value(Point_Net *p, int32_t *val_ptr, uint8_t mode, uint8_t flag);
extern int16_t put_net_point_value(Point_Net *p, int32_t *val_ptr, int16_t aux, int16_t prog_op, uint8_t mode);

extern STR_REMOTE_PANEL_DB remote_panel_db[];
extern U8_T remote_panel_num;
extern U8_T panel_number;

void test_local_points_database(void) {
    Point p = {0};
    p.number = 3;
    p.point_type = OUT + 1; // E.g., Output
    
    // Test point insertion and lookup
    int16_t idx = insert_local_point(&p, -1);
    TEST_ASSERT_TRUE(idx >= 0);
    
    int16_t found_idx = find_local_point(&p);
    TEST_ASSERT_EQUAL_INT16(idx, found_idx);

    // Test writing and reading point value
    int32_t write_val = 1500; // E.g., value * 1000
    int16_t put_status = put_point_value(&p, &write_val, 0, 0);
    TEST_ASSERT_TRUE(put_status >= 0);

    int32_t read_val = 0;
    int16_t get_status = get_point_value(&p, &read_val);
    TEST_ASSERT_TRUE(get_status >= 0);
    TEST_ASSERT_EQUAL_INT32(1500, read_val);
}

void test_remote_and_network_points_database(void) {
    // Setup mock remote panel DB entry to resolve network point Get_device_id_by_panel lookup
    panel_number = 0;
    remote_panel_db[0].panel = 12;
    remote_panel_db[0].sub_id = 12;
    remote_panel_db[0].device_id = 999;
    remote_panel_db[0].protocal = BAC_IP_CLIENT;
    remote_panel_num = 1;

    Point_Net pn = {0};
    pn.number = 5;
    pn.point_type = IN + 1; // E.g. Input
    pn.panel = 12; // Panel ID/Device ID
    
    // 1. Remote point lookup and insert
    int16_t rem_idx = insert_remote_point(&pn, -1);
    TEST_ASSERT_TRUE(rem_idx >= 0);

    int16_t found_rem = find_remote_point(&pn);
    TEST_ASSERT_EQUAL_INT16(rem_idx, found_rem);

    // 2. Network point lookup and insert
    int16_t net_idx = insert_network_point(&pn, -1);
    TEST_ASSERT_TRUE(net_idx >= 0);

    int16_t found_net = find_network_point(&pn);
    TEST_ASSERT_EQUAL_INT16(net_idx, found_net);

    // 3. Network point value read/write
    int32_t val = 420;
    int16_t put_status = put_net_point_value(&pn, &val, 0, 0, 0);
    TEST_ASSERT_TRUE(put_status >= 0);

    int32_t read_val = 0;
    int16_t get_status = get_net_point_value(&pn, &read_val, 0, 0);
    TEST_ASSERT_TRUE(get_status >= 0);
    TEST_ASSERT_EQUAL_INT32(420, read_val);
}
