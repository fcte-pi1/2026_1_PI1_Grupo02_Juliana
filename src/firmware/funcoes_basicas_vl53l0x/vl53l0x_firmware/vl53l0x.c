// ====================================================================
//  Driver VL53L0X para Raspberry Pi Pico (Pico SDK)
//  Porte do driver da Pololu (pololu/vl53l0x-arduino), que por sua vez
//  é derivado da API oficial da STMicroelectronics.
//
//  Faz a sequência de inicialização COMPLETA que o sensor exige:
//    - configuração de I/O 2.8V
//    - leitura da stop variable
//    - tuning settings da ST
//    - gerenciamento de SPADs de referência
//    - calibração de referência (VHV + fase)
//    - timing budget e sequence config
//  Sem isso, o sensor devolve valores incorretos/instáveis.
// ====================================================================

#include "vl53l0x.h"
#include "pico/stdlib.h"

// ------------------------------------------------------------------
// Registos
// ------------------------------------------------------------------
#define SYSRANGE_START                              0x00
#define SYSTEM_SEQUENCE_CONFIG                      0x01
#define SYSTEM_INTERRUPT_CONFIG_GPIO                0x0A
#define SYSTEM_INTERRUPT_CLEAR                      0x0B
#define GPIO_HV_MUX_ACTIVE_HIGH                     0x84
#define RESULT_INTERRUPT_STATUS                     0x13
#define RESULT_RANGE_STATUS                         0x14
#define MSRC_CONFIG_CONTROL                         0x60
#define FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT 0x44
#define MSRC_CONFIG_TIMEOUT_MACROP                  0x46
#define PRE_RANGE_CONFIG_VCSEL_PERIOD               0x50
#define PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI          0x51
#define FINAL_RANGE_CONFIG_VCSEL_PERIOD             0x70
#define FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI        0x71
#define VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV           0x89
#define I2C_SLAVE_DEVICE_ADDRESS                    0x8A
#define IDENTIFICATION_MODEL_ID                     0xC0
#define GLOBAL_CONFIG_SPAD_ENABLES_REF_0            0xB0
#define GLOBAL_CONFIG_REF_EN_START_SELECT           0xB6
#define DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD         0x4E
#define DYNAMIC_SPAD_REF_EN_START_OFFSET            0x4F

// ------------------------------------------------------------------
// Macros de timing (da API da ST)
// ------------------------------------------------------------------
#define decodeVcselPeriod(reg_val)      (((reg_val) + 1) << 1)
#define encodeVcselPeriod(period_pclks) (((period_pclks) >> 1) - 1)
#define calcMacroPeriod(vcsel_period_pclks) \
    ((((uint32_t)2304 * (vcsel_period_pclks) * 1655) + 500) / 1000)

typedef struct {
    bool tcc, msrc, dss, pre_range, final_range;
} SequenceStepEnables;

typedef struct {
    uint16_t pre_range_vcsel_period_pclks, final_range_vcsel_period_pclks;
    uint16_t msrc_dss_tcc_mclks, pre_range_mclks, final_range_mclks;
    uint32_t msrc_dss_tcc_us,    pre_range_us,    final_range_us;
} SequenceStepTimeouts;

// ------------------------------------------------------------------
// Primitivas de I2C
// ------------------------------------------------------------------
static void write_reg(VL53L0X *s, uint8_t reg, uint8_t value) {
    uint8_t buf[2] = {reg, value};
    i2c_write_blocking(s->i2c_port, s->address, buf, 2, false);
}

static void write_reg16(VL53L0X *s, uint8_t reg, uint16_t value) {
    uint8_t buf[3] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    i2c_write_blocking(s->i2c_port, s->address, buf, 3, false);
}

static uint8_t read_reg(VL53L0X *s, uint8_t reg) {
    uint8_t value = 0;
    i2c_write_blocking(s->i2c_port, s->address, &reg, 1, true);
    i2c_read_blocking(s->i2c_port, s->address, &value, 1, false);
    return value;
}

static uint16_t read_reg16(VL53L0X *s, uint8_t reg) {
    uint8_t buf[2] = {0, 0};
    i2c_write_blocking(s->i2c_port, s->address, &reg, 1, true);
    i2c_read_blocking(s->i2c_port, s->address, buf, 2, false);
    return (uint16_t)((buf[0] << 8) | buf[1]);
}

// Lê 'count' bytes a partir de 'reg' para 'dst'
static void read_multi(VL53L0X *s, uint8_t reg, uint8_t *dst, size_t count) {
    i2c_write_blocking(s->i2c_port, s->address, &reg, 1, true);
    i2c_read_blocking(s->i2c_port, s->address, dst, count, false);
}

// Escreve 'count' bytes de 'src' a partir de 'reg'
static void write_multi(VL53L0X *s, uint8_t reg, const uint8_t *src, size_t count) {
    uint8_t buf[1 + 6]; // SPAD map tem 6 bytes; suficiente aqui
    buf[0] = reg;
    for (size_t i = 0; i < count; i++) buf[i + 1] = src[i];
    i2c_write_blocking(s->i2c_port, s->address, buf, count + 1, false);
}

// ------------------------------------------------------------------
// Timeout
// ------------------------------------------------------------------
static void start_timeout(VL53L0X *s) {
    s->timeout_start_ms = (uint16_t)to_ms_since_boot(get_absolute_time());
}
static bool check_timeout_expired(VL53L0X *s) {
    uint16_t now = (uint16_t)to_ms_since_boot(get_absolute_time());
    return (s->io_timeout_ms > 0 &&
            (uint16_t)(now - s->timeout_start_ms) > s->io_timeout_ms);
}

// ------------------------------------------------------------------
// Conversões de timeout (da API da ST)
// ------------------------------------------------------------------
static uint16_t decode_timeout(uint16_t reg_val) {
    // formato: "(LSByte * 2^MSByte) + 1"
    return (uint16_t)((reg_val & 0x00FF) <<
                      (uint16_t)((reg_val & 0xFF00) >> 8)) + 1;
}

static uint16_t encode_timeout(uint32_t timeout_mclks) {
    uint32_t ls_byte = 0;
    uint16_t ms_byte = 0;
    if (timeout_mclks > 0) {
        ls_byte = timeout_mclks - 1;
        while ((ls_byte & 0xFFFFFF00) > 0) {
            ls_byte >>= 1;
            ms_byte++;
        }
        return (uint16_t)((ms_byte << 8) | (ls_byte & 0xFF));
    }
    return 0;
}

static uint32_t timeout_mclks_to_us(uint16_t timeout_period_mclks, uint8_t vcsel_period_pclks) {
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return ((timeout_period_mclks * macro_period_ns) + 500) / 1000;
}

static uint32_t timeout_us_to_mclks(uint32_t timeout_period_us, uint8_t vcsel_period_pclks) {
    uint32_t macro_period_ns = calcMacroPeriod(vcsel_period_pclks);
    return (((timeout_period_us * 1000) + (macro_period_ns / 2)) / macro_period_ns);
}

static uint8_t get_vcsel_pulse_period(VL53L0X *s, vcselPeriodType type) {
    if (type == VcselPeriodPreRange)
        return decodeVcselPeriod(read_reg(s, PRE_RANGE_CONFIG_VCSEL_PERIOD));
    else
        return decodeVcselPeriod(read_reg(s, FINAL_RANGE_CONFIG_VCSEL_PERIOD));
}

static void get_sequence_step_enables(VL53L0X *s, SequenceStepEnables *e) {
    uint8_t cfg = read_reg(s, SYSTEM_SEQUENCE_CONFIG);
    e->tcc         = (cfg >> 4) & 0x1;
    e->dss         = (cfg >> 3) & 0x1;
    e->msrc        = (cfg >> 2) & 0x1;
    e->pre_range   = (cfg >> 6) & 0x1;
    e->final_range = (cfg >> 7) & 0x1;
}

static void get_sequence_step_timeouts(VL53L0X *s, const SequenceStepEnables *e,
                                       SequenceStepTimeouts *t) {
    t->pre_range_vcsel_period_pclks = get_vcsel_pulse_period(s, VcselPeriodPreRange);

    t->msrc_dss_tcc_mclks = read_reg(s, MSRC_CONFIG_TIMEOUT_MACROP) + 1;
    t->msrc_dss_tcc_us =
        timeout_mclks_to_us(t->msrc_dss_tcc_mclks, t->pre_range_vcsel_period_pclks);

    t->pre_range_mclks = decode_timeout(read_reg16(s, PRE_RANGE_CONFIG_TIMEOUT_MACROP_HI));
    t->pre_range_us =
        timeout_mclks_to_us(t->pre_range_mclks, t->pre_range_vcsel_period_pclks);

    t->final_range_vcsel_period_pclks = get_vcsel_pulse_period(s, VcselPeriodFinalRange);

    t->final_range_mclks = decode_timeout(read_reg16(s, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI));
    if (e->pre_range)
        t->final_range_mclks -= t->pre_range_mclks;

    t->final_range_us =
        timeout_mclks_to_us(t->final_range_mclks, t->final_range_vcsel_period_pclks);
}

// ------------------------------------------------------------------
// Timing budget
// ------------------------------------------------------------------
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

    uint32_t budget_us = StartOverhead + EndOverhead;

    get_sequence_step_enables(s, &enables);
    get_sequence_step_timeouts(s, &enables, &timeouts);

    if (enables.tcc)  budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
    if (enables.dss)  budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    else if (enables.msrc) budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
    if (enables.pre_range)   budget_us += (timeouts.pre_range_us + PreRangeOverhead);
    if (enables.final_range) budget_us += (timeouts.final_range_us + FinalRangeOverhead);

    s->measurement_timing_budget_us = budget_us;
    return budget_us;
}

bool vl53l0x_set_measurement_timing_budget(VL53L0X *s, uint32_t budget_us) {
    SequenceStepEnables  enables;
    SequenceStepTimeouts timeouts;

    const uint16_t StartOverhead      = 1320; // diferente do get!
    const uint16_t EndOverhead        = 960;
    const uint16_t MsrcOverhead       = 660;
    const uint16_t TccOverhead        = 590;
    const uint16_t DssOverhead        = 690;
    const uint16_t PreRangeOverhead   = 660;
    const uint16_t FinalRangeOverhead = 550;
    const uint32_t MinTimingBudget    = 20000;

    if (budget_us < MinTimingBudget) return false;

    uint32_t used_budget_us = StartOverhead + EndOverhead;

    get_sequence_step_enables(s, &enables);
    get_sequence_step_timeouts(s, &enables, &timeouts);

    if (enables.tcc)  used_budget_us += (timeouts.msrc_dss_tcc_us + TccOverhead);
    if (enables.dss)  used_budget_us += 2 * (timeouts.msrc_dss_tcc_us + DssOverhead);
    else if (enables.msrc) used_budget_us += (timeouts.msrc_dss_tcc_us + MsrcOverhead);
    if (enables.pre_range) used_budget_us += (timeouts.pre_range_us + PreRangeOverhead);

    if (enables.final_range) {
        used_budget_us += FinalRangeOverhead;
        if (used_budget_us > budget_us) return false; // timeout pedido grande demais

        uint32_t final_range_timeout_us = budget_us - used_budget_us;
        uint32_t final_range_timeout_mclks =
            timeout_us_to_mclks(final_range_timeout_us, timeouts.final_range_vcsel_period_pclks);

        if (enables.pre_range)
            final_range_timeout_mclks += timeouts.pre_range_mclks;

        write_reg16(s, FINAL_RANGE_CONFIG_TIMEOUT_MACROP_HI,
                    encode_timeout(final_range_timeout_mclks));

        s->measurement_timing_budget_us = budget_us;
    }
    return true;
}

// ------------------------------------------------------------------
// Informação de SPADs
// ------------------------------------------------------------------
static bool get_spad_info(VL53L0X *s, uint8_t *count, bool *type_is_aperture) {
    uint8_t tmp;

    write_reg(s, 0x80, 0x01);
    write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x00, 0x00);

    write_reg(s, 0xFF, 0x06);
    write_reg(s, 0x83, read_reg(s, 0x83) | 0x04);
    write_reg(s, 0xFF, 0x07);
    write_reg(s, 0x81, 0x01);

    write_reg(s, 0x80, 0x01);

    write_reg(s, 0x94, 0x6b);
    write_reg(s, 0x83, 0x00);

    start_timeout(s);
    while (read_reg(s, 0x83) == 0x00) {
        if (check_timeout_expired(s)) return false;
    }
    write_reg(s, 0x83, 0x01);
    tmp = read_reg(s, 0x92);

    *count = tmp & 0x7f;
    *type_is_aperture = (tmp >> 7) & 0x01;

    write_reg(s, 0x81, 0x00);
    write_reg(s, 0xFF, 0x06);
    write_reg(s, 0x83, read_reg(s, 0x83) & ~0x04);
    write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x00, 0x01);

    write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x80, 0x00);

    return true;
}

// ------------------------------------------------------------------
// Calibração de referência
// ------------------------------------------------------------------
static bool perform_single_ref_calibration(VL53L0X *s, uint8_t vhv_init_byte) {
    write_reg(s, SYSRANGE_START, 0x01 | vhv_init_byte);

    start_timeout(s);
    while ((read_reg(s, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if (check_timeout_expired(s)) return false;
    }

    write_reg(s, SYSTEM_INTERRUPT_CLEAR, 0x01);
    write_reg(s, SYSRANGE_START, 0x00);
    return true;
}

// ------------------------------------------------------------------
// Inicialização completa
// ------------------------------------------------------------------
bool vl53l0x_init(VL53L0X *s, i2c_inst_t *i2c_port, bool io_2v8) {
    s->i2c_port = i2c_port;
    s->address  = VL53L0X_DEFAULT_ADDRESS;
    s->io_timeout_ms = 500;
    s->did_timeout = false;

    // Verifica se há mesmo um VL53L0X respondendo
    if (read_reg(s, IDENTIFICATION_MODEL_ID) != 0xEE) return false;

    // --- VL53L0X_DataInit() ---
    if (io_2v8) {
        write_reg(s, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV,
                  read_reg(s, VHV_CONFIG_PAD_SCL_SDA__EXTSUP_HV) | 0x01); // 2.8V
    }

    // Modo I2C "standard"
    write_reg(s, 0x88, 0x00);

    // Lê a stop variable única deste sensor
    write_reg(s, 0x80, 0x01);
    write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x00, 0x00);
    s->stop_variable = read_reg(s, 0x91);
    write_reg(s, 0x00, 0x01);
    write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x80, 0x00);

    // Desabilita os checks de SIGNAL_RATE_MSRC e SIGNAL_RATE_PRE_RANGE
    write_reg(s, MSRC_CONFIG_CONTROL, read_reg(s, MSRC_CONFIG_CONTROL) | 0x12);

    // Limite de taxa de sinal = 0.25 MCPS
    write_reg16(s, FINAL_RANGE_CONFIG_MIN_COUNT_RATE_RTN_LIMIT, (uint16_t)(0.25f * (1 << 7)));

    write_reg(s, SYSTEM_SEQUENCE_CONFIG, 0xFF);

    // --- VL53L0X_StaticInit() ---
    uint8_t spad_count;
    bool    spad_type_is_aperture;
    if (!get_spad_info(s, &spad_count, &spad_type_is_aperture)) return false;

    uint8_t ref_spad_map[6];
    read_multi(s, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

    write_reg(s, 0xFF, 0x01);
    write_reg(s, DYNAMIC_SPAD_REF_EN_START_OFFSET, 0x00);
    write_reg(s, DYNAMIC_SPAD_NUM_REQUESTED_REF_SPAD, 0x2C);
    write_reg(s, 0xFF, 0x00);
    write_reg(s, GLOBAL_CONFIG_REF_EN_START_SELECT, 0xB4);

    uint8_t first_spad_to_enable = spad_type_is_aperture ? 12 : 0;
    uint8_t spads_enabled = 0;

    for (uint8_t i = 0; i < 48; i++) {
        if (i < first_spad_to_enable || spads_enabled == spad_count) {
            ref_spad_map[i / 8] &= ~(1 << (i % 8));
        } else if ((ref_spad_map[i / 8] >> (i % 8)) & 0x1) {
            spads_enabled++;
        }
    }
    write_multi(s, GLOBAL_CONFIG_SPAD_ENABLES_REF_0, ref_spad_map, 6);

    // --- Tuning settings "mágicos" da ST ---
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x00, 0x00);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x09, 0x00);
    write_reg(s, 0x10, 0x00); write_reg(s, 0x11, 0x00);
    write_reg(s, 0x24, 0x01); write_reg(s, 0x25, 0xFF);
    write_reg(s, 0x75, 0x00); write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x4E, 0x2C); write_reg(s, 0x48, 0x00);
    write_reg(s, 0x30, 0x20); write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x30, 0x09); write_reg(s, 0x54, 0x00);
    write_reg(s, 0x31, 0x04); write_reg(s, 0x32, 0x03);
    write_reg(s, 0x40, 0x83); write_reg(s, 0x46, 0x25);
    write_reg(s, 0x60, 0x00); write_reg(s, 0x27, 0x00);
    write_reg(s, 0x50, 0x06); write_reg(s, 0x51, 0x00);
    write_reg(s, 0x52, 0x96); write_reg(s, 0x56, 0x08);
    write_reg(s, 0x57, 0x30); write_reg(s, 0x61, 0x00);
    write_reg(s, 0x62, 0x00); write_reg(s, 0x64, 0x00);
    write_reg(s, 0x65, 0x00); write_reg(s, 0x66, 0xA0);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x22, 0x32);
    write_reg(s, 0x47, 0x14); write_reg(s, 0x49, 0xFF);
    write_reg(s, 0x4A, 0x00); write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x7A, 0x0A); write_reg(s, 0x7B, 0x00);
    write_reg(s, 0x78, 0x21); write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x23, 0x34); write_reg(s, 0x42, 0x00);
    write_reg(s, 0x44, 0xFF); write_reg(s, 0x45, 0x26);
    write_reg(s, 0x46, 0x05); write_reg(s, 0x40, 0x40);
    write_reg(s, 0x0E, 0x06); write_reg(s, 0x20, 0x1A);
    write_reg(s, 0x43, 0x40); write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x34, 0x03); write_reg(s, 0x35, 0x44);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x31, 0x04);
    write_reg(s, 0x4B, 0x09); write_reg(s, 0x4C, 0x05);
    write_reg(s, 0x4D, 0x04); write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x44, 0x00); write_reg(s, 0x45, 0x20);
    write_reg(s, 0x47, 0x08); write_reg(s, 0x48, 0x28);
    write_reg(s, 0x67, 0x00); write_reg(s, 0x70, 0x04);
    write_reg(s, 0x71, 0x01); write_reg(s, 0x72, 0xFE);
    write_reg(s, 0x76, 0x00); write_reg(s, 0x77, 0x00);
    write_reg(s, 0xFF, 0x01); write_reg(s, 0x0D, 0x01);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x80, 0x01);
    write_reg(s, 0x01, 0xF8); write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x8E, 0x01); write_reg(s, 0x00, 0x01);
    write_reg(s, 0xFF, 0x00); write_reg(s, 0x80, 0x00);

    // --- Configuração de interrupção (data ready, ativo baixo) ---
    write_reg(s, SYSTEM_INTERRUPT_CONFIG_GPIO, 0x04);
    write_reg(s, GPIO_HV_MUX_ACTIVE_HIGH, read_reg(s, GPIO_HV_MUX_ACTIVE_HIGH) & ~0x10);
    write_reg(s, SYSTEM_INTERRUPT_CLEAR, 0x01);

    // Guarda o timing budget atual e reaplica
    s->measurement_timing_budget_us = vl53l0x_get_measurement_timing_budget(s);

    write_reg(s, SYSTEM_SEQUENCE_CONFIG, 0xE8);
    vl53l0x_set_measurement_timing_budget(s, s->measurement_timing_budget_us);

    // --- VL53L0X_PerformRefCalibration() ---
    write_reg(s, SYSTEM_SEQUENCE_CONFIG, 0x01);
    if (!perform_single_ref_calibration(s, 0x40)) return false; // VHV

    write_reg(s, SYSTEM_SEQUENCE_CONFIG, 0x02);
    if (!perform_single_ref_calibration(s, 0x00)) return false; // fase

    // Restaura a sequência
    write_reg(s, SYSTEM_SEQUENCE_CONFIG, 0xE8);

    return true;
}

// ------------------------------------------------------------------
// Endereço (opcional)
// ------------------------------------------------------------------
void vl53l0x_set_address(VL53L0X *s, uint8_t new_address) {
    write_reg(s, I2C_SLAVE_DEVICE_ADDRESS, new_address & 0x7F);
    s->address = new_address;
}

uint8_t vl53l0x_read_model_id(VL53L0X *s) {
    return read_reg(s, IDENTIFICATION_MODEL_ID);
}

// ------------------------------------------------------------------
// Leitura single-shot
// ------------------------------------------------------------------
uint16_t vl53l0x_read_range_single_millimeters(VL53L0X *s) {
    // Reinjeta a stop variable
    write_reg(s, 0x80, 0x01);
    write_reg(s, 0xFF, 0x01);
    write_reg(s, 0x00, 0x00);
    write_reg(s, 0x91, s->stop_variable);
    write_reg(s, 0x00, 0x01);
    write_reg(s, 0xFF, 0x00);
    write_reg(s, 0x80, 0x00);

    // Dispara
    write_reg(s, SYSRANGE_START, 0x01);

    // Espera o bit de start limpar
    start_timeout(s);
    while (read_reg(s, SYSRANGE_START) & 0x01) {
        if (check_timeout_expired(s)) { s->did_timeout = true; return 65535; }
    }

    // Espera os dados ficarem prontos
    start_timeout(s);
    while ((read_reg(s, RESULT_INTERRUPT_STATUS) & 0x07) == 0) {
        if (check_timeout_expired(s)) { s->did_timeout = true; return 65535; }
    }

    // Distância em mm está em RESULT_RANGE_STATUS + 10 (= 0x1E)
    uint16_t range = read_reg16(s, RESULT_RANGE_STATUS + 10);

    write_reg(s, SYSTEM_INTERRUPT_CLEAR, 0x01);

    s->did_timeout = false;
    return range;
}
