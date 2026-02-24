#include <Arduino.h>

// ================= 1. 引脚定义 (完全匹配原理图) =================
// 参考原理图 P1 页面 ESP32-S 模块连线
const int PIN_SPI_SCK  = 13; // SCLK -> IO13 
const int PIN_SPI_DIN  = 14; // SDA  -> IO14 
const int PIN_SPI_CS   = 15; // CS   -> IO15 
const int PIN_SPI_BUSY = 25; // BUSY -> IO25 
const int PIN_SPI_RST  = 26; // RES  -> IO26 
const int PIN_SPI_DC   = 27; // DC   -> IO27 

// ================= 2. 底层驱动函数 =================

// 模拟 SPI 传输 (Bit-bang)，确保在不同频率的 ESP32 上都能稳定工作
void EPD_SpiTransfer(uint8_t data) {
    digitalWrite(PIN_SPI_CS, LOW);
    for (int i = 0; i < 8; i++) {
        digitalWrite(PIN_SPI_DIN, (data & 0x80) ? HIGH : LOW);
        data <<= 1;
        digitalWrite(PIN_SPI_SCK, HIGH); 
        delayMicroseconds(1); // 保证时钟信号稳定
        digitalWrite(PIN_SPI_SCK, LOW);
        delayMicroseconds(1);
    }
    digitalWrite(PIN_SPI_CS, HIGH);
}

void EPD_SendCommand(uint8_t command) {
    digitalWrite(PIN_SPI_DC, LOW); // DC拉低发送命令
    EPD_SpiTransfer(command);
}

void EPD_SendData(uint8_t data) {
    digitalWrite(PIN_SPI_DC, HIGH); // DC拉高发送数据
    EPD_SpiTransfer(data);
}

void EPD_WaitUntilIdle() {
    Serial.println("Waiting for EPD BUSY to go Low...");
    uint32_t timeout = millis();
    // 4.2寸 V2 屏 (SSD1683) 逻辑：1 为忙，0 为空闲
    while(digitalRead(PIN_SPI_BUSY) == HIGH) {
        delay(1);
        if(millis() - timeout > 10000) {
            Serial.println("Error: BUSY Timeout! Check connection on IO25.");
            break;
        }
    }
}

void EPD_Reset() {
    digitalWrite(PIN_SPI_RST, HIGH);
    delay(100);
    digitalWrite(PIN_SPI_RST, LOW);
    delay(10);
    digitalWrite(PIN_SPI_RST, HIGH);
    delay(100);
}

// ================= 3. 屏幕显示控制 =================

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- ESP32-WROOM 4.2inch E-Ink Test ---");

    // 初始化所有引脚
    pinMode(PIN_SPI_BUSY, INPUT);
    pinMode(PIN_SPI_RST, OUTPUT);
    pinMode(PIN_SPI_DC, OUTPUT);
    pinMode(PIN_SPI_SCK, OUTPUT);
    pinMode(PIN_SPI_DIN, OUTPUT);
    pinMode(PIN_SPI_CS, OUTPUT);

    // 1. 复位与软复位
    EPD_Reset();
    EPD_WaitUntilIdle();
    EPD_SendCommand(0x12); // Software Reset
    EPD_WaitUntilIdle();

    // 2. 屏幕驱动设置 (400x300 分辨率)
    EPD_SendCommand(0x11); EPD_SendData(0x03); // Data entry mode (X increment, Y increment)
    EPD_SendCommand(0x44); EPD_SendData(0x00); EPD_SendData(0x31); // Set Ram-X address start/end (0-399)
    EPD_SendCommand(0x45); EPD_SendData(0x00); EPD_SendData(0x00); EPD_SendData(0x2B); EPD_SendData(0x01); // Set Ram-Y address start/end (0-299)

    // 3. 写入黑白数据层 (0x24) -> 全部填充白色
    Serial.println("Clearing screen (White)...");
    EPD_SendCommand(0x24);
    for (int i = 0; i < 15000; i++) EPD_SendData(0xFF); // 400*300/8 = 15000 bytes

    // 4. 写入红色数据层 (0x26) -> 创建红色测试条纹
    Serial.println("Drawing Red patterns...");
    EPD_SendCommand(0x26);
    for (int i = 0; i < 15000; i++) {
        // 每 40 行切换颜色，产生横向条纹
        if ((i / 50) % 40 < 20) EPD_SendData(0xFF); // 无色
        else EPD_SendData(0x00);                    // 红色 (0表示显色)
    }

    // 5. 触发刷新
    Serial.println("Refreshing Display...");
    EPD_SendCommand(0x22);
    EPD_SendData(0xF7); // Full display refresh
    EPD_SendCommand(0x20); // Master Activation
    EPD_WaitUntilIdle();

    Serial.println("Done!");
}

void loop() {
    // 静态显示
}