/*
 * Project: Smart Room Controller
 * MCU: STM32F103C6 or C8
 * Clock: 8.00 MHz (Ensure Proteus Component is also set to 8MHz)
 *
 * Connections (Proteus):
 * UART1:
 * - MCU PA9 (TX)  ---> Virtual Terminal RXD
 * - MCU PA10 (RX) <--- Virtual Terminal TXD
 * LEDs:
 * - PB0: Light
 * - PB1: Fan
 * - PB2: AC
 * LCD (16x2):
 * - RS -> PA0
 * - EN -> PA1
 * - D4 -> PA4
 * - D5 -> PA5
 * - D6 -> PA6
 * - D7 -> PA7
 */

// -------- LCD pin mapping for MikroC Library --------
sbit LCD_RS at GPIOA_ODR.B0;
sbit LCD_EN at GPIOA_ODR.B1;
sbit LCD_D4 at GPIOA_ODR.B4;
sbit LCD_D5 at GPIOA_ODR.B5;
sbit LCD_D6 at GPIOA_ODR.B6;
sbit LCD_D7 at GPIOA_ODR.B7;

// -------- Globals --------
unsigned short light1 = 0;
unsigned short fan    = 0;
unsigned short ac     = 0;

char cmd[20];

// Function Prototypes
void UpdateOutputsAndLCD(void);
void ParseCommand(void);
void Config_GPIO_System(void);

void main() {
    int i;

    // 1. Configure System Clocks and GPIO Pins
    Config_GPIO_System();

    // 2. Initialize Peripherals
    // UART1 Init: 9600 bps, 8 data bits, No parity, 1 stop bit
    UART1_Init(9600);
    Delay_ms(100);

    // LCD Init
    Lcd_Init();
    Lcd_Cmd(_LCD_CLEAR);
    Lcd_Cmd(_LCD_CURSOR_OFF);

    // Initial Screen
    Lcd_Out(1,1,"Smart Room Ctrl");
    Lcd_Out(2,1,"System Ready...");
    Delay_ms(1000);

    UpdateOutputsAndLCD();

    while(1) {
        // Read text line from UART
        // Logic: Wait for data, read until CR (0x0D) or LF (0x0A) or buffer full
        i = 0;
        memset(cmd, 0, 20); // Clear buffer

        while(i < 19) {
            // Blocking wait for character
            while(!UART1_Data_Ready()) ;

            char tmp = UART1_Read();

            // Handle Echo (Optional: allows you to see what you type in Terminal)
            UART1_Write(tmp);

            // Check for Enter key (Carriage Return or Line Feed)
            if(tmp == '\r' || tmp == '\n') {
                break;
            }

            cmd[i] = tmp;
            i++;
        }
        cmd[i] = 0; // Null-terminate string

        // Avoid parsing empty strings (e.g. if terminal sends both CR and LF)
        if (i > 0) {
            ParseCommand();
            UpdateOutputsAndLCD();
        }
    }
}

void Config_GPIO_System() {
    // Enable Clocks for Port A, Port B, and Alternate Functions
    RCC_APB2ENRbits.IOPAEN = 1;
    RCC_APB2ENRbits.IOPBEN = 1;
    RCC_APB2ENRbits.AFIOEN = 1;

    // --- PORT B Setup (LEDs) ---
    // PB0, PB1, PB2 as Output Push-Pull 2MHz
    // RESET value of GPIOB_CRL is 0x44444444 (Input Floating)
    // We want 0x...4444222
    // MODE=2 (Output 2MHz), CNF=0 (Push-Pull)
    GPIOB_CRLbits.MODE0 = 2; GPIOB_CRLbits.CNF0 = 0;
    GPIOB_CRLbits.MODE1 = 2; GPIOB_CRLbits.CNF1 = 0;
    GPIOB_CRLbits.MODE2 = 2; GPIOB_CRLbits.CNF2 = 0;

    // Default LEDs to OFF
    GPIOB_ODR.B0 = 0;
    GPIOB_ODR.B1 = 0;
    GPIOB_ODR.B2 = 0;

    // --- PORT A Setup (LCD + UART) ---
    // RESET value of GPIOA_CRL is 0x44444444

    // 1. LCD Pins (PA0, PA1, PA4..PA7) -> Output Push-Pull 2MHz
    GPIOA_CRLbits.MODE0 = 2; GPIOA_CRLbits.CNF0 = 0; // RS
    GPIOA_CRLbits.MODE1 = 2; GPIOA_CRLbits.CNF1 = 0; // EN
    // PA2, PA3 skipped (leave default)
    GPIOA_CRLbits.MODE4 = 2; GPIOA_CRLbits.CNF4 = 0; // D4
    GPIOA_CRLbits.MODE5 = 2; GPIOA_CRLbits.CNF5 = 0; // D5
    GPIOA_CRLbits.MODE6 = 2; GPIOA_CRLbits.CNF6 = 0; // D6
    GPIOA_CRLbits.MODE7 = 2; GPIOA_CRLbits.CNF7 = 0; // D7

    // 2. UART Pins (PA9, PA10) are in CRH register
    // PA9 (TX): Must be Alternate Function Push-Pull (CNF=10, MODE=10 or 11)
    // Let's use Output 10MHz (MODE=1) and AF Push-Pull (CNF=2) -> Binary 1001 = 0x9 or 1010 = 0xA
    GPIOA_CRHbits.MODE9 = 2; // Output 2MHz
    GPIOA_CRHbits.CNF9  = 2; // Alternate Function Push-Pull

    // PA10 (RX): Must be Input Floating (CNF=01, MODE=00) or Pull-Up
    // Default is Floating (0x4), but let's be explicit
    GPIOA_CRHbits.MODE10 = 0; // Input
    GPIOA_CRHbits.CNF10  = 1; // Floating Input
}

void ParseCommand(void) {
    // Check commands. Note: strstr finds substring.
    // "LIGHT1 ON"
    if      (strstr(cmd, "LIGHT1 ON")  != 0) light1 = 1;
    else if (strstr(cmd, "LIGHT1 OFF") != 0) light1 = 0;
    else if (strstr(cmd, "FAN ON")     != 0) fan    = 1;
    else if (strstr(cmd, "FAN OFF")    != 0) fan    = 0;
    else if (strstr(cmd, "AC ON")      != 0) ac     = 1;
    else if (strstr(cmd, "AC OFF")     != 0) ac     = 0;
    else {
        Lcd_Cmd(_LCD_CLEAR);
        Lcd_Out(1,1,"Unknown Cmd:");
        Lcd_Out(2,1, cmd); // Print the received text to debug
        Delay_ms(1500);
    }
}

void UpdateOutputsAndLCD(void) {
    // Update LEDs
    GPIOB_ODR.B0 = light1;
    GPIOB_ODR.B1 = fan;
    GPIOB_ODR.B2 = ac;

    // Update LCD
    Lcd_Cmd(_LCD_CLEAR);

    // Row 1: L1 and Fan
    Lcd_Out(1,1,"L1:");
    if(light1) Lcd_Out(1,4,"ON ");
    else       Lcd_Out(1,4,"OFF");

    Lcd_Out(1,9,"F:");
    if(fan)    Lcd_Out(1,11,"ON ");
    else       Lcd_Out(1,11,"OFF");

    // Row 2: AC
    Lcd_Out(2,1,"AC:");
    if(ac)     Lcd_Out(2,4,"ON ");
    else       Lcd_Out(2,4,"OFF");
}