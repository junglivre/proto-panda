#include <stdbool.h>

#include <stdint.h>

#include <string.h>

#include "LIS3DHTR.h"

#include "nrf_log.h"

#include "nrf_log_ctrl.h"

#include "nrf_log_default_backends.h"

#define TWI_ADDRESSES 127

//Initializing TWI0 instance
#define TWI_INSTANCE_ID 0

static float accRange = 0;

// A flag to indicate the transfer state
static volatile bool m_xfer_done = false;

// Create a Handle for the twi communication
static
const nrf_drv_twi_t m_twi = NRF_DRV_TWI_INSTANCE(TWI_INSTANCE_ID);

void twi_handler(nrf_drv_twi_evt_t
  const * p_event, void * p_context) {
  //Check the event to see what type of event occurred
  switch (p_event -> type) {
    //If data transmission or receiving is finished
  case NRF_DRV_TWI_EVT_DONE:
    m_xfer_done = true; //Set the flag
    break;

  default:
    // do nothing
    break;
  }
}

//Initialize the TWI as Master device
void twi_master_init(void) {
  ret_code_t err_code;

  // Configure the settings for twi communication
  const nrf_drv_twi_config_t twi_config = {
    .scl = TWI_SCL_M, //SCL Pin
    .sda = TWI_SDA_M, //SDA Pin
    .frequency = NRF_DRV_TWI_FREQ_400K, //Communication Speed
    .interrupt_priority = APP_IRQ_PRIORITY_HIGH, //Interrupt Priority(Note: if using Bluetooth then select priority carefully)
    .clear_bus_init = false //automatically clear bus
  };

  //A function to initialize the twi communication
  err_code = nrf_drv_twi_init( & m_twi, & twi_config, twi_handler, NULL);
  APP_ERROR_CHECK(err_code);

  //Enable the TWI Communication
  nrf_drv_twi_enable( & m_twi);

  uint8_t address;
  uint8_t sample_data;
  bool detected_device = false;

  for (address = 1; address <= 126; address++) {
    err_code = nrf_drv_twi_rx( & m_twi, address, & sample_data, sizeof(sample_data));
    if (err_code == NRF_SUCCESS) {
      detected_device = true;
      NRF_LOG_INFO("TWI device detected at address 0x%x.", address);
    }
    NRF_LOG_FLUSH();
  }

}

/*
   A function to write a Single Byte to MPU6050's internal Register
*/
bool LIS3_write_register(uint8_t register_address, uint8_t value) {
  ret_code_t err_code;
  uint8_t tx_buf[LIS3DHTR_ADDRESS_LEN + 1];

  //Write the register address and data into transmit buffer
  tx_buf[0] = register_address;
  tx_buf[1] = value;

  //Set the flag to false to show the transmission is not yet completed
  m_xfer_done = false;

  //Transmit the data over TWI Bus
  err_code = nrf_drv_twi_tx( & m_twi, LIS3DHTR_ADDRESS_UPDATED, tx_buf, LIS3DHTR_ADDRESS_LEN + 1, false);
  int tries = 2000;
  //Wait until the transmission of the data is finished
  while (m_xfer_done == false) {
    tries--;
    if (tries <= 0) {
      err_code = NRF_ERROR_TIMEOUT;
      break;
    }
  }

  // if there is no error then return true else return false
  if (NRF_SUCCESS != err_code) {
    return false;
  }

  return true;
}

ret_code_t anrf_drv_twi_tx(nrf_drv_twi_t
  const * p_instance,
  uint8_t address,
  uint8_t
  const * p_data,
  uint8_t length,
  bool no_stop) {
  ret_code_t result = 0;
  if (NRF_DRV_TWI_USE_TWIM) {
    result = nrfx_twim_tx( & p_instance -> u.twim,
      address, p_data, length, no_stop);
  } else if (NRF_DRV_TWI_USE_TWI) {
    result = nrfx_twi_tx( & p_instance -> u.twi,
      address, p_data, length, no_stop);
  }
  return result;
}

bool LIS3_read_registerRegion(uint8_t * destination, uint8_t register_address, uint8_t number_of_bytes) {
  ret_code_t err_code;

  //Set the flag to false to show the receiving is not yet completed
  m_xfer_done = false;

  // Send the Register address where we want to write the data

  err_code = anrf_drv_twi_tx( & m_twi, LIS3DHTR_ADDRESS_UPDATED, & register_address, 1, true);

  int tries = 2000;

  //Wait for the transmission to get completed
  while (m_xfer_done == false) {
    tries--;
    if (tries <= 0) {
      err_code = NRF_ERROR_TIMEOUT;
      break;
    }
  }

  // If transmission was not successful, exit the function with false as return value
  if (NRF_SUCCESS != err_code) {
    return false;
  }

  //set the flag again so that we can read data from the MPU6050's internal register
  m_xfer_done = false;

  // Receive the data from the MPU6050
  err_code = nrf_drv_twi_rx( & m_twi, LIS3DHTR_ADDRESS_UPDATED, destination, number_of_bytes);
  tries = 2000;
  //wait until the transmission is completed
  while (m_xfer_done == false) {
    tries--;
    if (tries <= 0) {
      err_code = NRF_ERROR_TIMEOUT;
      break;
    }
  }

  // if data was successfully read, return true else return false
  if (NRF_SUCCESS != err_code) {
    return false;
  }

  return true;
}

/*
  A Function to verify the product id
  (its a basic test to check if we are communicating with the right slave, every type of I2C Device has 
  a special WHO_AM_I register which holds a specific value, we can read it from the MPU6050 or any device
  to confirm we are communicating with the right device)
*/
bool LIS3_verify_product_id(void) {
  uint8_t who_am_i; // create a variable to hold the who am i value
  // Note: All the register addresses including WHO_AM_I are declared in 
  // MPU6050.h file, you can check these addresses and values from the
  // datasheet of your slave device.
  if (LIS3_read_registerRegion( & who_am_i, LIS3DHTR_REG_ACCEL_WHO_AM_I, 1)) {
    if (who_am_i != 51) {
      return false;
    } else {
      return true;
    }
  } else {
    return false;
  }
}

/*
  Function to initialize the LIS3
*/
bool LIS3_init(void) {
  twi_master_init();
  bool transfer_succeeded = true;

  //Check the id to confirm that we are communicating with the right device
  transfer_succeeded &= LIS3_verify_product_id();

  if (LIS3_verify_product_id() == false) {
    return false;
  }

  uint8_t config5 = LIS3DHTR_REG_TEMP_ADC_PD_ENABLED |
    LIS3DHTR_REG_TEMP_TEMP_EN_DISABLED;

  LIS3_write_register(LIS3DHTR_REG_TEMP_CFG, config5);

  uint8_t config1 = LIS3DHTR_REG_ACCEL_CTRL_REG1_LPEN_NORMAL | // Normal Mode
    LIS3DHTR_REG_ACCEL_CTRL_REG1_AZEN_ENABLE | // Acceleration Z-Axis Enabled
    LIS3DHTR_REG_ACCEL_CTRL_REG1_AYEN_ENABLE | // Acceleration Y-Axis Enabled
    LIS3DHTR_REG_ACCEL_CTRL_REG1_AXEN_ENABLE;

  LIS3_write_register(LIS3DHTR_REG_ACCEL_CTRL_REG1, config1);

  nrf_delay_ms(LIS3DHTR_CONVERSIONDELAY);

  uint8_t config4 = LIS3DHTR_REG_ACCEL_CTRL_REG4_BDU_NOTUPDATED | // Continuous Update
    LIS3DHTR_REG_ACCEL_CTRL_REG4_BLE_LSB | // Data LSB @ lower address
    LIS3DHTR_REG_ACCEL_CTRL_REG4_HS_DISABLE | // High Resolution Disable
    LIS3DHTR_REG_ACCEL_CTRL_REG4_ST_NORMAL | // Normal Mode
    LIS3DHTR_REG_ACCEL_CTRL_REG4_SIM_4WIRE; // 4-Wire Interface

  LIS3_write_register(LIS3DHTR_REG_ACCEL_CTRL_REG4, config4);

  nrf_delay_ms(LIS3DHTR_CONVERSIONDELAY);

  LIS3_set_full_scale_range(LIS3DHTR_REG_ACCEL_CTRL_REG4_FS_4G);
  LIS3_set_output_data_rate(LIS3DHTR_DATARATE_400HZ);

  uint8_t bolo = LIS3_read_register(LIS3DHTR_REG_ACCEL_WHO_AM_I);

  return transfer_succeeded;
}

void LIS3_set_output_data_rate(odr_type_t odr) {
  uint8_t data = 0;
  data = LIS3_read_register(LIS3DHTR_REG_ACCEL_CTRL_REG1);
  data &= ~LIS3DHTR_REG_ACCEL_CTRL_REG1_AODR_MASK;
  data |= odr;
  LIS3_write_register(LIS3DHTR_REG_ACCEL_CTRL_REG1, data);
  nrf_delay_ms(LIS3DHTR_CONVERSIONDELAY);
}

uint8_t LIS3_read_register(uint8_t reg) {
  uint8_t data;
  LIS3_read_registerRegion( & data, reg, 1);
  return data;
}
uint16_t LIS3_read_register_int16(uint8_t reg) {

  uint8_t myBuffer[2];
  LIS3_read_registerRegion(myBuffer, reg, 2);
  uint16_t output = myBuffer[0] | ((uint16_t)(myBuffer[1] << 8));
  return output;
}

int16_t LIS3_get_acceleration_X(void) {
  // Read the Accelerometer
  uint8_t xAccelLo, xAccelHi;
  int16_t x;
  xAccelLo = LIS3_read_register(LIS3DHTR_REG_ACCEL_OUT_X_L);
  xAccelHi = LIS3_read_register(LIS3DHTR_REG_ACCEL_OUT_X_H);
  x = (int16_t)((xAccelHi << 8) | xAccelLo);

  return x;
}

int16_t LIS3_get_acceleration_Y(void) {
  // Read the Accelerometer
  uint8_t xAccelLo, xAccelHi;
  int16_t x;
  xAccelLo = LIS3_read_register(LIS3DHTR_REG_ACCEL_OUT_Y_L);
  xAccelHi = LIS3_read_register(LIS3DHTR_REG_ACCEL_OUT_Y_H);
  x = (int16_t)((xAccelHi << 8) | xAccelLo);

  return x;
}

int16_t LIS3_get_acceleration_Z(void) {
  // Read the Accelerometer
  uint8_t xAccelLo, xAccelHi;
  int16_t x;
  xAccelLo = LIS3_read_register(LIS3DHTR_REG_ACCEL_OUT_Z_L);
  xAccelHi = LIS3_read_register(LIS3DHTR_REG_ACCEL_OUT_Z_H);
  x = (int16_t)((xAccelHi << 8) | xAccelLo);

  return x;
}

void LIS3_set_full_scale_range(scale_type_t range) {
  uint8_t data = 0;

  data = LIS3_read_register(LIS3DHTR_REG_ACCEL_CTRL_REG4);

  data &= ~LIS3DHTR_REG_ACCEL_CTRL_REG4_FS_MASK;
  data |= range;

  LIS3_write_register(LIS3DHTR_REG_ACCEL_CTRL_REG4, data);
  nrf_delay_ms(LIS3DHTR_CONVERSIONDELAY);

  switch (range) {
  case LIS3DHTR_REG_ACCEL_CTRL_REG4_FS_16G:
    accRange = 1280;
    break;
  case LIS3DHTR_REG_ACCEL_CTRL_REG4_FS_8G:
    accRange = 3968;
    break;
  case LIS3DHTR_REG_ACCEL_CTRL_REG4_FS_4G:
    accRange = 7282;
    break;
  case LIS3DHTR_REG_ACCEL_CTRL_REG4_FS_2G:
    accRange = 16000;
    break;
  default:
    break;
  }

}

float getPica() {
  return accRange;
}

void LIS3_set_power_mode(power_type_t mode) {
  uint8_t data = 0;

  data = LIS3_read_register(LIS3DHTR_REG_ACCEL_CTRL_REG1);

  data &= ~LIS3DHTR_REG_ACCEL_CTRL_REG1_LPEN_MASK;
  data |= mode;

  LIS3_write_register(LIS3DHTR_REG_ACCEL_CTRL_REG1, data);
  //nrf_delay_ms(LIS3DHTR_CONVERSIONDELAY);
}

void LIS3_set_high_solution(bool enable) {
  uint8_t data = 0;
  data = LIS3_read_register(LIS3DHTR_REG_ACCEL_CTRL_REG4);

  data = enable ? data | LIS3DHTR_REG_ACCEL_CTRL_REG4_HS_ENABLE : data & ~LIS3DHTR_REG_ACCEL_CTRL_REG4_HS_ENABLE;

  LIS3_write_register(LIS3DHTR_REG_ACCEL_CTRL_REG4, data);
  return;
}

bool LSM6_init(void) {

  if (LSM6_verify_product_id() == false) {
    return false;
  }

  nrf_delay_ms(LIS3DHTR_CONVERSIONDELAY);

  return true;
}

bool LSM6_verify_product_id(void) {
  uint8_t who_am_i; // create a variable to hold the who am i value
  // Note: All the register addresses including WHO_AM_I are declared in 
  // MPU6050.h file, you can check these addresses and values from the
  // datasheet of your slave device.
  if (LIS3_read_registerRegion( & who_am_i, LSM6DS3_WHO_AM_I_REG, 1)) {
    NRF_LOG_INFO("product id is: %d\n", who_am_i);
    if (who_am_i == 0x6A || who_am_i == 0x69) {
      return true;
    } else {
      return false;
    }
  } else {
    return false;
  }
}

void LSM6_configure() {
  LSM6_set_power_mode(true);
  //set the gyroscope control register to work at 104 Hz, 2000 dps and in bypass mode
  LIS3_write_register(LSM6DS3_CTRL2_G, 0x4C);
  // Set the Accelerometer control register to work at 104 Hz, 4 g,and in bypass mode and enable ODR/4
  // low pass filter (check figure9 of LSM6DS3's datasheet)
  LIS3_write_register(LSM6DS3_CTRL1_XL, 0x4A);
  // set gyroscope power mode to high performance and bandwidth to 16 MHz
  LIS3_write_register(LSM6DS3_CTRL7_G, 0x00);
  // Set the ODR config register to ODR/4
  LIS3_write_register(LSM6DS3_CTRL8_XL, 0x09);
}

bool LSM6_readAcceleration(int16_t * x, int16_t * y, int16_t * z) {
  int16_t data[3];
  if (!LIS3_read_registerRegion((uint8_t * ) data, LSM6DS3_OUTX_L_XL, sizeof(data))) {
    return false;
  }
  ( * x) = data[0];
  ( * y) = data[1];
  ( * z) = data[2];
  return true;
}

bool LSM6_readGyro(int16_t * x, int16_t * y, int16_t * z) {
  int16_t data[3];
  if (!LIS3_read_registerRegion((uint8_t * ) data, LSM6DS3_OUTX_L_G, sizeof(data))) {
    return false;
  }
  ( * x) = data[0];
  ( * y) = data[1];
  ( * z) = data[2];
  return true;
}

bool LSM6_readTemperature(int16_t * tm) {
  int16_t data[1];
  if (!LIS3_read_registerRegion((uint8_t * ) data, LSM6DS3_OUT_TEMP_L, sizeof(data))) {
    return false;
  }
  ( * tm) = data[0];
  return true;
}

void LSM6_set_power_mode(bool on) {
  if (on) {
    LIS3_write_register(LSM6DS3_CTRL2_G, 0x4C);
    LIS3_write_register(LSM6DS3_CTRL1_XL, 0x4A);
    LIS3_write_register(LSM6DS3_CTRL7_G, 0x00);
    LIS3_write_register(LSM6DS3_CTRL8_XL, 0x09);

    //LIS3_write_register(LSM6DS3_WAKE_UP_DUR, 0x00);
    //LIS3_write_register(LSM6DS3_WAKE_UP_THS, 0x02);
    //LIS3_write_register(LSM6DS3_TAP_CFG, 0b00010010);
    //LIS3_write_register(LSM6DS3_CTRL1_XL, 0x70);
    //nrf_delay_ms(4);
    //LIS3_write_register(LSM6DS3_CTRL1_XL, 0x10);
    //LIS3_write_register(LSM6DS3_MD1_CFG, 0x20);
  } else {
    LIS3_write_register(LSM6DS3_CTRL2_G, 0x00);
    LIS3_write_register(LSM6DS3_CTRL1_XL, 0x00);
  }
}