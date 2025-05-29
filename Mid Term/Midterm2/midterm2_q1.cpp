#include "mbed.h"

// Setup I2C - Use correct SDA, SCL pins for your STM32 board
I2C i2c(PB_9, PB_8); // SDA, SCL (check your board's datasheet)
DigitalOut led(LED1); // Onboard LED
InterruptIn intPin(PA_0); // INT pin from MAX30100

const int MAX30100_I2C_ADDRESS = 0xAE >> 1; // MAX30100 7-bit address

// Function to write to MAX30100 register
void writeRegister(uint8_t reg, uint8_t value) {
    char data[2] = {reg, value};
    i2c.write(MAX30100_I2C_ADDRESS << 1, data, 2);
}

// Function to read from MAX30100 register
uint8_t readRegister(uint8_t reg) {
    char data[1] = {reg};
    i2c.write(MAX30100_I2C_ADDRESS << 1, data, 1, true);
    i2c.read(MAX30100_I2C_ADDRESS << 1, data, 1);
    return data[0];
}

// Function to configure the sensor
void configureSensor() {
    // Enable only SPO2_RDY interrupt
    writeRegister(0x01, 0x10); // Enable SpO2 interrupt

    // Set mode to SPO2
    writeRegister(0x06, 0x03);

    // Use default settings in SPO2 Config
    // Bit 6 SPO2_HI_RES_EN = 0 (default), rest default
    writeRegister(0x07, 0x00);
}

// Function to get data from FIFO
void GetData() {
    char fifoData[4];
    char reg = 0x05; // FIFO_DATA register address

    i2c.write(MAX30100_I2C_ADDRESS << 1, &reg, 1, true); // Set read pointer
    i2c.read(MAX30100_I2C_ADDRESS << 1, fifoData, 4);    // Read 4 bytes

    uint16_t ir_data = (fifoData[0] << 8) | fifoData[1];
    uint16_t red_data = (fifoData[2] << 8) | fifoData[3];

    printf("IR LED: %u, RED LED: %u\r\n", ir_data, red_data);
}

// Interrupt Service Routine
void onInterrupt() {
    led = !led; // Toggle onboard LED
    GetData();  // Read and print data
}

int main() {
    printf("Starting MAX30100 SPO2 Monitoring...\r\n");

    i2c.frequency(400000); // Set I2C frequency to 400kHz
    configureSensor();     // Configure the MAX30100

    intPin.fall(&onInterrupt); // Attach ISR to falling edge (active low INT)

    while (1) {
        ThisThread::sleep_for(1s); // Sleep to reduce CPU usage
    }
}