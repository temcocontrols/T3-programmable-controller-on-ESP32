#ifndef __SCD40_H
#define __SCD40_H

extern int16_t scd40_start_periodic_measurement(uint16_t ambient_pressure_mbar);
extern int16_t scd40_stop_periodic_measurement();
extern int16_t scd40_read_measurement(float* co2_ppm, float* temperature,
                               float* humidity);
extern int16_t scd40_set_measurement_interval(uint16_t interval_sec);
#endif
