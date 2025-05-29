#include "mbed.h"

SPI spi(PB_15, PB_14, PB_13); // MISO, MOSI, SCLK
DigitalOut cs(PB_12, 1);      // Active-low CS
InterruptIn intn(PA_1);       // INT̅ from RR200

constexpr uint8_t RR200_ADDR_READ  = 0x01; // bit0 = 1
constexpr uint8_t RR200_ADDR_WRITE = 0x00; // bit0 = 0

uint8_t spiRead(uint8_t reg)
{
    cs = 0;
    spi.write((reg << 1) | RR200_ADDR_READ); // send address + R
    uint8_t val = spi.write(0x00);           // dummy -> get data
    cs = 1;
    return val;
}

void spiWrite(uint8_t reg, uint8_t val)
{
    cs = 0;
    spi.write((reg << 1) | RR200_ADDR_WRITE); // address + W
    spi.write(val);
    cs = 1;
}

void getRRSample()
{
    // 3-byte sample (RR_H, RR_L, QUAL)
    cs = 0;
    spi.write((0x05 << 1) | RR200_ADDR_READ);
    uint16_t rr = (spi.write(0) << 8) | spi.write(0);
    uint8_t  qual = spi.write(0);
    cs = 1;

    float rate_hz = rr / 256.0f;        // 8.8-format stored
    printf("Respiratory-Rate %.2f Hz  (Quality %u)\r\n", rate_hz, qual);
}

void isr()
{
    uint8_t status = spiRead(0x00);
    if (status & 0x20)   getRRSample();      // RR_RDY
    if (status & 0x40)   printf("Temp ready = %dC\r\n", (int8_t)spiRead(0x16));
    if (status & 0x04)   printf("Fault!\r\n");
}

int main()
{
    spi.frequency(8000000);
    spi.format(8, 0);            // 8-bit, SPI mode 0

    // Enable RR_RDY + TEMP_RDY interrupts
    spiWrite(0x01, 0x60);
    // Configure 100 sps, 400 µs pulse, MODE = RR_ONLY
    spiWrite(0x07, 0b0'001'01);
    spiWrite(0x06, 0x02);

    intn.fall(&isr);
    while (true) ThisThread::sleep_for(1s);
}
