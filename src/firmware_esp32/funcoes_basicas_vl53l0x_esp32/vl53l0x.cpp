#include "vl53l0x.h"

// ---------------------------------------------------------------------------
// Primitivas I2C (TwoWire / Arduino)
// ---------------------------------------------------------------------------

static void write_reg(VL53L0X *s, uint8_t reg, uint8_t val) {
    s->i2c_port->beginTransmission(s->address);
    s->i2c_port->write(reg);
    s->i2c_port->write(val);
    s->i2c_port->endTransmission();
}

static void write_reg16(VL53L0X *s, uint8_t reg, uint16_t val) {
    s->i2c_port->beginTransmission(s->address);
    s->i2c_port->write(reg);
    s->i2c_port->write((val >> 8) & 0xFF);
    s->i2c_port->write(val & 0xFF);
    s->i2c_port->endTransmission();
}

static uint8_t read_reg(VL53L0X *s, uint8_t reg) {
    s->i2c_port->beginTransmission(s->address);
    s->i2c_port->write(reg);
    s->i2c_port->endTransmission(false);
    s->i2c_port->requestFrom((uint8_t)s->address, (uint8_t)1);
    return s->i2c_port->read();
}

static uint16_t read_reg16(VL53L0X *s, uint8_t reg) {
    s->i2c_port->beginTransmission(s->address);
    s->i2c_port->write(reg);
    s->i2c_port->endTransmission(false);
    s->i2c_port->requestFrom((uint8_t)s->address, (uint8_t)2);
    uint16_t val = (uint16_t)s->i2c_port->read() << 8;
    val |= s->i2c_port->read();
    return val;
}

static void read_multi(VL53L0X *s, uint8_t reg, uint8_t *dst, uint8_t len) {
    s->i2c_port->beginTransmission(s->address);
    s->i2c_port->write(reg);
    s->i2c_port->endTransmission(false);
    s->i2c_port->requestFrom((uint8_t)s->address, len);
    for (uint8_t i = 0; i < len; i++) dst[i] = s->i2c_port->read();
}

static void write_multi(VL53L0X *s, uint8_t reg, const uint8_t *src, uint8_t len) {
    s->i2c_port->beginTransmission(s->address);
    s->i2c_port->write(reg);
    for (uint8_t i = 0; i < len; i++) s->i2c_port->write(src[i]);
    s->i2c_port->endTransmission();
}

// ---------------------------------------------------------------------------
// Timeout
// ---------------------------------------------------------------------------

static void start_timeout(VL53L0X *s) {
    s->timeout_start_ms = millis();
}

static bool check_timeout_expired(VL53L0X *s) {
    if (s->io_timeout_ms == 0) return false;
    return (millis() - s->timeout_start_ms) > s->io_timeout_ms;
}

// ---------------------------------------------------------------------------
// Conversões de timing
// ---------------------------------------------------------------------------

static uint32_t decode_timeout(uint16_t val) {
    return ((uint32_t)(val & 0x00FF) << (uint32_t)((val & 0xFF00) >> 8)) + 1;
}

static uint16_t encode_timeout(uint32_t mclks) {
    uint32_t ls_byte = 0;
    uint16_t ms_byte = 0;
    if (mclks > 0) {
        ls_byte = mclks - 1;
        while ((ls_byte & 0xFFFFFF00) > 0) { ls_byte >>= 1; ms_byte++; }
        return (ms_byte << 8) | (ls_byte & 0xFF);
    }
    return 0;
}

static uint32_t timeout_mclks_to_us(uint16_t mclks, uint8_t vcsel_period_pclks) {
    uint32_t macro_period_ns = (uint32_t)(2304 * vcsel_period_pclks + 1655) / 1000;
    return ((uint32_t)mclks * macro_period_ns + 500) / 1000;
}

static uint32_t timeout_us_to_mclks(uint32_t us, uint8_t vcsel_period_pclks) {
    uint32_t macro_period_ns = (uint32_t)(2304 * vcsel_period_pclks + 1655) / 1000;
    return (us * 1000 + macro_period_ns / 2) / macro_period_ns;
}

static uint8_t get_vcsel_pulse_period(VL53L0X *s, vcselPeriodType type) {
    if (type == VcselPeriodPreRange)
        return ((read_reg(s, 0x50) + 1) << 1);
    return ((read_reg(s, 0x70) + 1) << 1);
}

// ---------------------------------------------------------------------------
// Sequência de medição
// ---------------------------------------------------------------------------

typedef struct {
    bool tcc, msrc, dss, pre_range, final_range;
} SequenceStepEnables;

typedef struct {
    uint16_t pre_range_vcsel_period_pclks, final_range_vcsel_period_pclks;
    uint16_t msrc_dss_tcc_mclks, pre_range_mclks, final_range_mclks;
    uint32_t msrc_dss_tcc_us,    pre_range_us,    final_range_us;
} SequenceStepTimeouts;

static void get_sequence_step_enables(VL53L0X *s, SequenceStepEnables *e) {
    uint8_t seq = read_reg(s, 0x01);
    e->tcc        = (seq >> 4) & 0x1;
    e->dss        = (seq >> 3) & 0x1;
    e->msrc       = (seq >> 2) & 0x1;
    e->pre_range  = (seq >> 6) & 0x1;
    e->final_range = (seq >> 7) & 0x1;
}

static void get_sequence_step_timeouts(VL53L0X *s,
                                       const SequenceStepEnables *e,
                                       SequenceStepTimeouts *t) {
    t->pre_range_vcsel_period_pclks  = get_vcsel_pulse_period(s, VcselPeriodPreRange);
    t->final_range_vcsel_period_pclks = get_vcsel_pulse_period(s, VcselPeriodFinalRange);

    t->msrc_dss_tcc_mclks = read_reg(s, 0x46) + 1;
    t->msrc_dss_tcc_us    = timeout_mclks_to_us(t->msrc_dss_tcc_mclks,
                                                  t->pre_range_vcsel_period_pclks);

    t->pre_range_mclks = decode_timeout(read_reg16(s, 0x51));
    t->pre_range_us    = timeout_mclks_to_us(t->pre_range_mclks,
                                              t->pre_range_vcsel_period_pclks);

    uint16_t fr_mclks = decode_timeout(read_reg16(s, 0x71));
    if (e->pre_range) fr_mclks -= t->pre_range_mclks;
    t->final_range_mclks = fr_mclks;
    t->final_range_us    = timeout_mclks_to_us(t->final_range_mclks,
                                                t->final_range_vcsel_period_pclks);
}

// ---------------------------------------------------------------------------
// Timing budget
// ---------------------------------------------------------------------------

uint32_t vl53l0x_get_measurement_timing_budget(VL53L0X *s) {
    SequenceStepEnables  enables;
    SequenceStepTimeouts timeouts;
    const uint16_t StartOverhead     = 1910;
    const uint16_t EndOverhead       = 960;
    const uint16_t MsrcOverhead      = 660;
    const uint16_t TccOverhead       = 590;
    const uint16_t DssOverhead       = 690;
    const uint16_t PreRangeOverhead  = 660;
    const uint16_t FinalRangeOverhead = 550;

    uint32_t budget = StartOverhead + EndOverhead;
    get_sequence_step_enables(s, &enables);
    get_sequence_step_timeouts(s, &enables, &timeouts);

    if (enables.tcc)        budget += timeouts.msrc_dss_tcc_us + TccOverhead;
    if (enables.dss)        budget += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    else if (enables.msrc)  budget += timeouts.msrc_dss_tcc_us + MsrcOverhead;
    if (enables.pre_range)  budget += timeouts.pre_range_us + PreRangeOverhead;
    if (enables.final_range) budget += timeouts.final_range_us + FinalRangeOverhead;

    s->measurement_timing_budget_us = budget;
    return budget;
}

bool vl53l0x_set_measurement_timing_budget(VL53L0X *s, uint32_t budget_us) {
    SequenceStepEnables  enables;
    SequenceStepTimeouts timeouts;
    const uint16_t StartOverhead      = 1320;
    const uint16_t EndOverhead        = 960;
    const uint16_t MsrcOverhead       = 660;
    const uint16_t TccOverhead        = 590;
    const uint16_t DssOverhead        = 690;
    const uint16_t PreRangeOverhead   = 660;
    const uint16_t FinalRangeOverhead = 550;
    const uint32_t MinTimingBudget    = 20000;

    if (budget_us < MinTimingBudget) return false;

    uint32_t used = StartOverhead + EndOverhead;
    get_sequence_step_enables(s, &enables);
    get_sequence_step_timeouts(s, &enables, &timeouts);

    if (enables.tcc)        used += timeouts.msrc_dss_tcc_us + TccOverhead;
    if (enables.dss)        used += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    else if (enables.msrc)  used += timeouts.msrc_dss_tcc_us + MsrcOverhead;
    if (enables.pre_range)  used += timeouts.pre_range_us + PreRangeOverhead;

    if (enables.final_range) {
        if (budget_us <= used + FinalRangeOverhead) return false;
        uint32_t fr_us = budget_us - used - FinalRangeOverhead;
        uint32_t fr_mclks = timeout_us_to_mclks(fr_us,
                                timeouts.final_range_vcsel_period_pclks);
        if (enables.pre_range) fr_mclks += timeouts.pre_range_mclks;
        write_reg16(s, 0x71, encode_timeout(fr_mclks));
    }

    s->measurement_timing_budget_us = budget_us;
    return true;
}

// ---------------------------------------------------------------------------
// SPADs e calibração
// ---------------------------------------------------------------------------

static bool get_spad_info(VL53L0X *s, uint8_t *count, bool *type_is_aperture) {
    write_reg(s, 0x80, 0x01); write_reg(s, 0xFF, 0x01); write_reg(s, 0x00, 0x00);
    write_reg(s, 0xFF, 0x06);
    write_reg(s, 0x83, read_reg(s, 0x83) | 0x04);
    write_reg(s, 0xFF, 0x07); write_reg(s, 0x81, 0x01);
    write_reg(s, 0x80, 0x01); write_reg(s, 0x94, 0x6b); write_reg(s, 0x83, 0x00);

    start_timeout(s);
    while (read_reg(s, 0x83) == 0x00) {
        if (check_timeout_expired(s)) return false;
    }

    write_reg(s, 0x83, 0x01);
    uint8_t tmp = read_reg(s, 0x92);
    *count = tmp & 0x7F;
    *type_is_aperture = (tmp >> 7) & 0x01;

    write_reg(s, 0x81, 0x00); write_reg(s, 0xFF, 0x06);
    write_reg(s, 0x83, read_reg(s, 0x83) & ~0x04);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x00, 0x01);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x80, 0x00);
    return true;
}

static bool perform_single_ref_calibration(VL53L0X *s, uint8_t vhv_init_byte) {
    write_reg(s, 0x00, 0x01 | vhv_init_byte);
    start_timeout(s);
    while ((read_reg(s, 0x13) & 0x07) == 0) {
        if (check_timeout_expired(s)) return false;
    }
    write_reg(s, 0x0B, 0x01);
    write_reg(s, 0x00, 0x00);
    return true;
}

// ---------------------------------------------------------------------------
// API pública
// ---------------------------------------------------------------------------

bool vl53l0x_init(VL53L0X *s, TwoWire *i2c_port, bool io_2v8) {
    s->i2c_port    = i2c_port;
    s->address     = VL53L0X_DEFAULT_ADDRESS;
    s->io_timeout_ms = 500;
    s->did_timeout = false;

    // Verifica ID do modelo
    if (read_reg(s, 0xC0) != 0xEE) return false;

    // Tensão de I/O 2.8V
    if (io_2v8) {
        write_reg(s, 0x89, read_reg(s, 0x89) | 0x10);
    }

    // Lê stop variable
    write_reg(s, 0x80, 0x01); write_reg(s, 0xFF, 0x01); write_reg(s, 0x00, 0x00);
    s->stop_variable = read_reg(s, 0x91);
    write_reg(s, 0x00, 0x01); write_reg(s, 0xFF, 0x00); write_reg(s, 0x80, 0x00);

    // Desativa SIGNAL_RATE_MSRC e SIGNAL_RATE_PRE_RANGE
    write_reg(s, 0x60, read_reg(s, 0x60) | 0x12);

    // Limite de taxa de sinal: 0.25 MCPS
    write_reg16(s, 0x44, 0x0020);

    // Configura SPADs de referência
    uint8_t spad_count;
    bool    spad_type_is_aperture;
    if (!get_spad_info(s, &spad_count, &spad_type_is_aperture)) return false;

    uint8_t ref_spad_map[6];
    read_multi(s, 0xB0, ref_spad_map, 6);

    write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x4F, 0x00);
    write_reg(s, 0x4E, 0x2C);
    write_reg(s, 0xFF, 0x00);
    write_reg(s, 0xB6, 0xB4);

    uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0;
    uint8_t spads_enabled = 0;
    for (uint8_t i = 0; i < 48; i++) {
        if (i < first_spad_to_enable || spads_enabled == spad_count) {
            ref_spad_map[i / 8] &= ~(1 << (i % 8));
        } else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1) {
            spads_enabled++;
        }
    }
    write_multi(s, 0xB0, ref_spad_map, 6);

    // Tuning settings da ST
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x00, 0x00);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x09, 0x00);
    write_reg(s, 0x10, 0x00); write_reg(s, 0x11, 0x00);
    write_reg(s, 0x24, 0x01); write_reg(s, 0x25, 0xFF);
    write_reg(s, 0x75, 0x00);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x4E, 0x2C);
    write_reg(s, 0x48, 0x00); write_reg(s, 0x30, 0x20);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x30, 0x09);
    write_reg(s, 0x54, 0x00); write_reg(s, 0x31, 0x04);
    write_reg(s, 0x32, 0x03); write_reg(s, 0x40, 0x83);
    write_reg(s, 0x46, 0x25); write_reg(s, 0x60, 0x00);
    write_reg(s, 0x27, 0x00); write_reg(s, 0x50, 0x06);
    write_reg(s, 0x51, 0x00); write_reg(s, 0x52, 0x96);
    write_reg(s, 0x56, 0x08); write_reg(s, 0x57, 0x30);
    write_reg(s, 0x61, 0x00); write_reg(s, 0x62, 0x00);
    write_reg(s, 0x64, 0x00); write_reg(s, 0x65, 0x00);
    write_reg(s, 0x66, 0xA0);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x22, 0x32);
    write_reg(s, 0x47, 0x14); write_reg(s, 0x49, 0xFF);
    write_reg(s, 0x4A, 0x00);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x7A, 0x0A);
    write_reg(s, 0x7B, 0x00); write_reg(s, 0x78, 0x21);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x23, 0x34);
    write_reg(s, 0x42, 0x00); write_reg(s, 0x44, 0xFF);
    write_reg(s, 0x45, 0x26); write_reg(s, 0x46, 0x05);
    write_reg(s, 0x40, 0x40); write_reg(s, 0x0E, 0x06);
    write_reg(s, 0x20, 0x1A); write_reg(s, 0x43, 0x40);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x34, 0x03);
    write_reg(s, 0x35, 0x44); write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x31, 0x04); write_reg(s, 0x4B, 0x09);
    write_reg(s, 0x4C, 0x05); write_reg(s, 0x4D, 0x04);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x44, 0x00);
    write_reg(s, 0x45, 0x20); write_reg(s, 0x47, 0x08);
    write_reg(s, 0x48, 0x28); write_reg(s, 0x67, 0x00);
    write_reg(s, 0x70, 0x04); write_reg(s, 0x71, 0x01);
    write_reg(s, 0x72, 0xFE); write_reg(s, 0x76, 0x00);
    write_reg(s, 0x77, 0x00);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x0D, 0x01);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x80, 0x01);
    write_reg(s, 0x01, 0xF8);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x8E, 0x01);
    write_reg(s, 0x00, 0x01); write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x80, 0x00);

    // Interrupção em dados prontos
    write_reg(s, 0x0A, 0x04);
    write_reg(s, 0x84, read_reg(s, 0x84) & ~0x10);
    write_reg(s, 0x0B, 0x00);

    // Salva e reaplica timing budget
    vl53l0x_get_measurement_timing_budget(s);
    write_reg(s, 0x01, 0xE8); // sequência padrão
    vl53l0x_set_measurement_timing_budget(s, s->measurement_timing_budget_us);

    // Calibração VHV e de fase
    write_reg(s, 0x01, 0x01);
    if (!perform_single_ref_calibration(s, 0x40)) return false;
    write_reg(s, 0x01, 0x02);
    if (!perform_single_ref_calibration(s, 0x00)) return false;
    write_reg(s, 0x01, 0xE8);

    return true;
}

void vl53l0x_set_address(VL53L0X *s, uint8_t new_address) {
    write_reg(s, 0x8A, new_address & 0x7F);
    s->address = new_address;
}

uint8_t vl53l0x_read_model_id(VL53L0X *s) {
    return read_reg(s, 0xC0);
}

uint16_t vl53l0x_read_range_single_millimeters(VL53L0X *s) {
    write_reg(s, 0x80, 0x01); write_reg(s, 0xFF, 0x01); write_reg(s, 0x00, 0x00);
    write_reg(s, 0x91, s->stop_variable);
    write_reg(s, 0x00, 0x01); write_reg(s, 0xFF, 0x00); write_reg(s, 0x80, 0x00);

    write_reg(s, 0x00, 0x01);
    start_timeout(s);
    while (read_reg(s, 0x00) & 0x01) {
        if (check_timeout_expired(s)) { s->did_timeout = true; return 65535; }
    }

    start_timeout(s);
    while ((read_reg(s, 0x13) & 0x07) == 0) {
        if (check_timeout_expired(s)) { s->did_timeout = true; return 65535; }
    }

    uint16_t range = read_reg16(s, 0x1E);
    write_reg(s, 0x0B, 0x01);
    return range;
}
