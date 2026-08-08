#include "tools/fft.hpp"

#include <dsps_fft2r.h>
#include <dsps_wind_hann.h>
#include "tools/logger.hpp"

void FFT::readAndAnalyse() {
    uint32_t bytes_read = 0;
    esp_err_t ret = adc_continuous_read(
        m_adc_handle, m_adcBuf, m_adcBufSize, &bytes_read, 0
    );

    if (ret == ESP_ERR_TIMEOUT) return;

    uint32_t sample_count = bytes_read / SOC_ADC_DIGI_RESULT_BYTES;

    for (int i = 0; i < m_samples; i++) {
        float sample = 0.0f;
        if (ret == ESP_OK && (uint32_t)i < sample_count) {
            adc_digi_output_data_t* p = (adc_digi_output_data_t*)&m_adcBuf[i * SOC_ADC_DIGI_RESULT_BYTES];
            sample = (float)p->type2.data;
        }
        m_fftBuf[i * 2 + 0] = sample * m_window[i];
        m_fftBuf[i * 2 + 1] = 0.0f;
    }

    dsps_fft2r_fc32(m_fftBuf, m_samples);
    dsps_bit_rev_fc32(m_fftBuf, m_samples);
    dsps_cplx2reC_fc32(m_fftBuf, m_samples);

    for (int i = 0; i < m_bandCount; i++) m_bandValues[i] = 0;

    const int MAX_BIN = m_samples / 2;
    for (int i = 2; i < MAX_BIN; i++) {
        if (m_binToBand[i] < 0) continue;
        float re  = m_fftBuf[i * 2 + 0];
        float im  = m_fftBuf[i * 2 + 1];
        float mag = sqrtf(re * re + im * im);
        if (mag <= (float)m_noiseThreshold) continue;
        m_bandValues[m_binToBand[i]] += (int)mag;
    }
}

void FFT::freeBuffers() {
    free(m_adcBuf);     m_adcBuf     = nullptr;
    free(m_fftBuf);     m_fftBuf     = nullptr;
    free(m_window);     m_window     = nullptr;
    free(m_binToBand);  m_binToBand  = nullptr;
    free(m_bandValues); m_bandValues = nullptr;
    dsps_fft2r_deinit_fc32();
}

bool FFT::begin(int gpio, int samples, int samplingFreq, int noiseThreshold, int bandCount) {
    m_started = false;
    if (samples < 64 || samples > 4096 || (samples & (samples - 1)) != 0) {
        Logger::Info("[FFT] begin() failed: samples must be a power of 2 between 64 and 4096");
        return false;
    }
    if (samplingFreq < 20000 || samplingFreq > 83333) {
        Logger::Info("[FFT] begin() failed: samplingFreq must be between 20000 and 83333 hz");
        return false;
    }
    if (noiseThreshold < 0) {
        Logger::Info("[FFT] begin() failed: noiseThreshold must be >= 0");
        return false;
    }
    if (bandCount < 1 || bandCount > samples / 2) {
        Logger::Info("[FFT] begin() failed: bandCount must be between 1 and %d (samples/2)\n", samples / 2);
        return false;
    }

    adc_unit_t u; 
    adc_channel_t c;
    if (adc_continuous_io_to_channel(gpio, &u, &c) != ESP_OK || u != ADC_UNIT_1) {
        Logger::Info("[FFT] begin() failed: GPIO %d is not allowed or is not part of the ADC1  [u=%d c=%d]", u, c);
        return false;
    }


    m_samples        = samples;
    m_samplingFreq   = samplingFreq;
    m_noiseThreshold = noiseThreshold;
    m_bandCount      = bandCount;
    m_adcBufSize     = samples * SOC_ADC_DIGI_RESULT_BYTES;

    m_fftBuf    = (float*)heap_caps_aligned_alloc(16, sizeof(float) * m_samples * 2, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
    m_window    = (float*)ps_malloc(sizeof(float) * m_samples);
    m_binToBand = (int*)ps_malloc(sizeof(int) * (m_samples / 2));
    m_bandValues = (int*)ps_malloc(sizeof(int) * m_bandCount);
    m_adcBuf    = (uint8_t*)heap_caps_malloc(m_adcBufSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA);

    if (!m_fftBuf || !m_window || !m_binToBand || !m_bandValues || !m_adcBuf) {
        Logger::Info("[FFT] begin() failed: out of memory");
        freeBuffers();
        return false;
    }

    memset(m_bandValues, 0, sizeof(int) * m_bandCount);

    esp_err_t err = dsps_fft2r_init_fc32(NULL, m_samples);
    if (err != ESP_OK) {
        Logger::Info("[FFT] dsps_fft2r_init_fc32 failed: %d\n", err);
        freeBuffers();
        return false;
    }

    dsps_wind_hann_f32(m_window, m_samples);


    const int MAX_BIN = m_samples / 2;
    for (int i = 0; i < MAX_BIN; i++) m_binToBand[i] = -1;

    const float logMin   = log(2.0f);
    const float logRange = log((float)MAX_BIN) - logMin;

    int lastHi = 2; // bins 0 and 1 are always excluded (DC / near-DC)
    for (int b = 0; b < m_bandCount; b++) {
        float t1 = (float)(b + 1) / (float)m_bandCount;
        int hi = (int)roundf(exp(t1 * logRange + logMin));
        hi = constrain(hi, lastHi + 1, MAX_BIN);

        for (int i = lastHi; i < hi && i < MAX_BIN; i++) {
            m_binToBand[i] = b;
        }
        lastHi = hi;
    }

    adc_continuous_handle_cfg_t adc_config = {
        .max_store_buf_size = (uint32_t)(m_adcBufSize * 3),
        .conv_frame_size    = (uint32_t)m_adcBufSize,
    };
    err = adc_continuous_new_handle(&adc_config, &m_adc_handle);
    if (err != ESP_OK) {
        Logger::Info("[FFT] failed to create ADC handle: %d\n", err);
        freeBuffers();
        return false;
    }

    adc_digi_pattern_config_t adc_pattern = {
        .atten     = ADC_ATTEN_DB_12,
        .channel   = (adc_channel_t)(gpio-1),
        .unit      = ADC_UNIT_1,
        .bit_width = ADC_BITWIDTH_12,
    };
    adc_continuous_config_t dig_cfg = {
        .pattern_num    = 1,
        .adc_pattern    = &adc_pattern,
        .sample_freq_hz = (uint32_t)m_samplingFreq,
        .conv_mode      = ADC_CONV_SINGLE_UNIT_1,
        .format         = ADC_DIGI_OUTPUT_FORMAT_TYPE2,
    };
    err = adc_continuous_config(m_adc_handle, &dig_cfg);
    if (err != ESP_OK) {
        Logger::Info("[FFT] failed to configure ADC: %d\n", err);
        freeBuffers();
        return false;
    }
    m_started = true;

    Logger::Info("[FFT] begin() started with channel %d and pin %d", gpio-1, gpio);
    return true;
}

bool FFT::start() {
    if (!m_started){
        Logger::Info("[FFT] start() called before begin()");
        return false;
    }
    if (!m_adc_handle) {
        Logger::Info("[FFT] start() called before begin()");
        return false;
    }
    esp_err_t err = adc_continuous_start(m_adc_handle);
    if (err != ESP_OK) {
        Logger::Info("[FFT] start() failed: %d\n", err);
        return false;
    }
    m_running = true;
    Logger::Info("[FFT] started");
    return true;
}

void FFT::stop() {
    if (m_adc_handle && m_running) {
        adc_continuous_stop(m_adc_handle);
        m_running = false;
        Logger::Info("[FFT] stopped");
    }
}

void FFT::deinit() {
    stop();
    if (m_adc_handle) {
        adc_continuous_deinit(m_adc_handle);
        m_adc_handle = nullptr;
    }
    freeBuffers();
}

int  FFT::getBandCount(){ 
    return m_bandCount; 
}
bool FFT::isRunning(){ 
    return m_running; 
}

int FFT::getBandValue(int i){
    if (!m_running)               return 0;
    if (i < 0 || i >= m_bandCount) return 0;
    return m_bandValues[i];
}

void FFT::update() {
    if (!m_running) return;
    readAndAnalyse();
}