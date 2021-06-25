#include "veml7700.h"

#define MGOS_VEML7700_ADDR 0x10

#define MGOS_VEML7700_REG_CFG 0
#define MGOS_VEML7700_REG_PSM 3
#define MGOS_VEML7700_REG_ALS 4

struct mgos_veml7700 {
  struct mgos_i2c *bus;
  uint16_t cfg, psm;
  float coef;
  int64_t meas_interval_micros;
  int64_t first_sample_micros;
  int64_t next_sample_micros;
};

#define GET_IT(cfg) (cfg & (0xf << 6))
#define SET_IT(cfg, it) ((cfg & ~(0xf << 6)) | it)
#define GET_GAIN(cfg) (cfg & (3 << 11))
#define SET_GAIN(cfg, gain) ((cfg & ~(3 << 11)) | gain)

bool mgos_veml7700_detect(struct mgos_i2c *bus) {
  if (mgos_i2c_read_reg_w(bus, MGOS_VEML7700_ADDR, MGOS_VEML7700_REG_CFG) < 0) {
    return false;
  }
  return true;
}

struct mgos_veml7700 *mgos_veml7700_create(struct mgos_i2c *bus) {
  if (bus == NULL) return NULL;
  struct mgos_veml7700 *ctx = calloc(1, sizeof(struct mgos_veml7700));
  ctx->bus = bus;
  return ctx;
}

int mgos_veml7700_read_reg(struct mgos_veml7700 *ctx, uint8_t reg) {
  if (ctx == NULL) return -1;
  int val = mgos_i2c_read_reg_w(ctx->bus, MGOS_VEML7700_ADDR, reg);
  if (val < 0) return val;
  return ((val >> 8) | ((val & 0xff) << 8));
}

bool mgos_veml7700_write_reg(struct mgos_veml7700 *ctx, uint8_t reg,
                             uint16_t val) {
  if (ctx == NULL) return -1;
  uint16_t swapped_val = (((val & 0xff) << 8) | (val >> 8));
  return mgos_i2c_write_reg_w(ctx->bus, MGOS_VEML7700_ADDR, reg, swapped_val);
}

static int mgos_veml7700_calc_interval_ms(uint16_t cfg, uint16_t psm) {
  int st = 0;
  if ((psm & 1) != 0) {
    st = (500 << ((psm >> 1) & 3));
  }
  int it = 0;
  switch (GET_IT(cfg)) {
    case MGOS_VEML7700_CFG_ALS_IT_25:
      it = 25;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_50:
      it = 50;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_100:
      it = 100;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_200:
      it = 200;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_400:
      it = 400;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_800:
      it = 800;
      break;
  }
  return st + it;
}

bool mgos_veml7700_set_cfg(struct mgos_veml7700 *ctx, uint16_t cfg,
                           uint16_t psm) {
  if (ctx == NULL) return false;
  // App note says shut down before reconfig.
  if (!mgos_veml7700_write_reg(ctx, MGOS_VEML7700_REG_CFG,
                               MGOS_VEML7700_CFG_SD)) {
    return false;
  }
  if ((cfg & MGOS_VEML7700_CFG_SD) != 0) {
    ctx->cfg = cfg;
    return true;
  }
  if (psm != MGOS_VEML7700_PSM_NO_CHANGE) {
    if (!mgos_veml7700_write_reg(ctx, MGOS_VEML7700_REG_PSM, psm)) {
      return false;
    }
    ctx->psm = psm;
  }
  if (!mgos_veml7700_write_reg(ctx, MGOS_VEML7700_REG_CFG, cfg)) {
    return false;
  }
  ctx->cfg = cfg;
  mgos_usleep(3000);
  float coef = 0.0576;  // gain 1, it 100 ms.
  switch (GET_GAIN(cfg)) {
    case MGOS_VEML7700_CFG_ALS_GAIN_1_8:
      coef *= 8;
      break;
    case MGOS_VEML7700_CFG_ALS_GAIN_1_4:
      coef *= 4;
      break;
    case MGOS_VEML7700_CFG_ALS_GAIN_2:
      coef /= 2;
      break;
  }
  switch (GET_IT(cfg)) {
    case MGOS_VEML7700_CFG_ALS_IT_25:
      coef *= 4;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_50:
      coef *= 2;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_200:
      coef /= 2;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_400:
      coef /= 4;
      break;
    case MGOS_VEML7700_CFG_ALS_IT_800:
      coef /= 8;
      break;
  }
  ctx->coef = coef;
  int intvl_ms = mgos_veml7700_calc_interval_ms(ctx->cfg, ctx->psm);
  int meas_time_ms = mgos_veml7700_calc_interval_ms(ctx->cfg, 0);
  ctx->meas_interval_micros = intvl_ms * 1000;
  ctx->first_sample_micros = mgos_uptime_micros() + (meas_time_ms + 50) * 1000;
  ctx->next_sample_micros = ctx->first_sample_micros;
  // LOG(LL_INFO,
  //    ("cfg 0x%04x psm 0x%04x intvl %d ms", ctx->cfg, ctx->psm, intvl_ms));
  return true;
}

int mgos_bh1750_get_meas_interval_ms(struct mgos_veml7700 *ctx) {
  return ctx->meas_interval_micros / 1000;
}

static void mgos_veml7700_adjust_up(uint16_t *gain, uint16_t *it) {
  switch (*gain) {
    case MGOS_VEML7700_CFG_ALS_GAIN_1_8:
      *gain = MGOS_VEML7700_CFG_ALS_GAIN_1_4;
      *it = MGOS_VEML7700_CFG_ALS_IT_100;
      break;
    case MGOS_VEML7700_CFG_ALS_GAIN_1_4:
      *gain = MGOS_VEML7700_CFG_ALS_GAIN_1;
      *it = MGOS_VEML7700_CFG_ALS_IT_100;
      break;
    case MGOS_VEML7700_CFG_ALS_GAIN_1:
      *gain = MGOS_VEML7700_CFG_ALS_GAIN_2;
      *it = MGOS_VEML7700_CFG_ALS_IT_100;
      break;
    default: {
      switch (*it) {
        case MGOS_VEML7700_CFG_ALS_IT_25:
          *it = MGOS_VEML7700_CFG_ALS_IT_50;
          break;
        case MGOS_VEML7700_CFG_ALS_IT_50:
          *it = MGOS_VEML7700_CFG_ALS_IT_100;
          break;
        case MGOS_VEML7700_CFG_ALS_IT_100:
          *it = MGOS_VEML7700_CFG_ALS_IT_200;
          break;
        case MGOS_VEML7700_CFG_ALS_IT_200:
          *it = MGOS_VEML7700_CFG_ALS_IT_400;
          break;
        default:
          *it = MGOS_VEML7700_CFG_ALS_IT_800;
          break;
      }
      break;
    }
  }
}

static void mgos_veml7700_adjust_down(uint16_t *gain, uint16_t *it) {
  switch (*gain) {
    case MGOS_VEML7700_CFG_ALS_GAIN_2:
      *gain = MGOS_VEML7700_CFG_ALS_GAIN_1;
      *it = MGOS_VEML7700_CFG_ALS_IT_100;
      break;
    case MGOS_VEML7700_CFG_ALS_GAIN_1:
      *gain = MGOS_VEML7700_CFG_ALS_GAIN_1_4;
      *it = MGOS_VEML7700_CFG_ALS_IT_100;
      break;
    case MGOS_VEML7700_CFG_ALS_GAIN_1_4:
      *gain = MGOS_VEML7700_CFG_ALS_GAIN_1_8;
      *it = MGOS_VEML7700_CFG_ALS_IT_100;
      break;
    default: {
      switch (*it) {
        case MGOS_VEML7700_CFG_ALS_IT_800:
          *it = MGOS_VEML7700_CFG_ALS_IT_400;
          break;
        case MGOS_VEML7700_CFG_ALS_IT_400:
          *it = MGOS_VEML7700_CFG_ALS_IT_200;
          break;
        case MGOS_VEML7700_CFG_ALS_IT_200:
          *it = MGOS_VEML7700_CFG_ALS_IT_100;
          break;
        case MGOS_VEML7700_CFG_ALS_IT_100:
          *it = MGOS_VEML7700_CFG_ALS_IT_50;
          break;
        default:
          *it = MGOS_VEML7700_CFG_ALS_IT_25;
          break;
      }
      break;
    }
  }
}

static bool mgos_veml7700_adjust_als(struct mgos_veml7700 *ctx, int als) {
  if (als >= MGOS_VEML7700_ADJUST_UP_THRESH &&
      als <= MGOS_VEML7700_ADJUST_DOWN_THRESH) {
    return true;
  }
  if (als < 0) return false;
  if ((ctx->cfg & MGOS_VEML7700_CFG_SD) != 0) return false;
  if (mgos_uptime_micros() < ctx->next_sample_micros) return false;
  uint16_t cfg = ctx->cfg;
  uint16_t gain = GET_GAIN(cfg), it = GET_IT(cfg);
  uint16_t new_gain = gain, new_it = it;
  if (als < MGOS_VEML7700_ADJUST_UP_THRESH) {
    mgos_veml7700_adjust_up(&new_gain, &new_it);
  } else {
    mgos_veml7700_adjust_down(&new_gain, &new_it);
  }
  if (new_gain == gain && new_it == it) {
    return true;
  }
  uint16_t new_cfg = SET_GAIN(cfg, new_gain);
  new_cfg = SET_IT(new_cfg, new_it);
  return mgos_veml7700_set_cfg(ctx, new_cfg, MGOS_VEML7700_PSM_NO_CHANGE);
}

bool mgos_veml7700_adjust(struct mgos_veml7700 *ctx) {
  return mgos_veml7700_adjust_als(
      ctx, mgos_veml7700_read_reg(ctx, MGOS_VEML7700_REG_ALS));
}

float mgos_veml7700_read_lux(struct mgos_veml7700 *ctx, bool adjust) {
  if (ctx == NULL) return -1;
  int val = mgos_veml7700_read_reg(ctx, MGOS_VEML7700_REG_ALS);
  float res = val * ctx->coef;
  // LOG(LL_INFO, ("%d %.4f %.4f %lld %lld", val, ctx->coef, res,
  // mgos_uptime_micros(), ctx->first_sample_micros));
  if (val < 0) return -2;
  if (val == 0 && mgos_uptime_micros() < ctx->first_sample_micros) {
    return -3;
  }
  if (adjust) mgos_veml7700_adjust_als(ctx, val);
  return res;
}

bool mgos_veml7700_init(void) {
  return true;
}
