#include <EEPROM.h>
//LED: PB4/PB5/PC2/PC3
#define Set_Bit(val, bitn) (val |= (1 << (bitn)))
#define Clr_Bit(val, bitn) (val &= ~(1 << (bitn)))
#define Get_Bit(val, bitn) (val & (1 << (bitn)))
//MS PD2/3/4
//SETP PD6,DIR PD5
//EN PB0
//LIMIT PB1
#define DEV_ID 0x01
//led sck mi c2 c3
#define LedRed1        \
    Set_Bit(PORTB, 4); \
    Set_Bit(PORTC, 3)
#define LedRed0        \
    Clr_Bit(PORTB, 4); \
    Clr_Bit(PORTC, 3)
#define LedBlue1       \
    Set_Bit(PORTB, 5); \
    Set_Bit(PORTC, 2)
#define LedBlue0       \
    Clr_Bit(PORTB, 5); \
    Clr_Bit(PORTC, 2)

#define Limit        (PINB & 0x02) != 0x02
#define DisableMotor Set_Bit(PORTB, 0)
#define EnableMotor  Clr_Bit(PORTB, 0)
#define P1           Set_Bit(PORTD, 6)
#define P0           Clr_Bit(PORTD, 6)
#define D1           Set_Bit(PORTD, 5)
#define D0           Clr_Bit(PORTD, 5)
#define MS1_1        Set_Bit(PORTD, 2)
#define MS1_0        Clr_Bit(PORTD, 2)
#define MS2_1        Set_Bit(PORTD, 3)
#define MS2_0        Clr_Bit(PORTD, 3)
#define MS3_1        Set_Bit(PORTD, 4)
#define MS3_0        Clr_Bit(PORTD, 4)
//EEPROM MAP
#define eeprom_ID        0x00
#define eeprom_LENGTH    0x01
#define eeprom_N         0x02
#define eeprom_MS        0x03
#define eeprom_MAX_SPEED 0x04
#define eeprom_NOW       0x05

int CommandStatus = 0;
int DeviceID      = 0x01;
byte CommandType  = 0;
byte DataL = 0, DataH = 0;
int Target, Speed, Acc, Now, Err;
int AccStatus =
    0;  // 用来描述加速的那个阶段，静止，0->提速，1->爬升，2->缓冲，3->到达最高速度，4->减速，5->骤降，6->缓冲
int SpeedPoint[7] = {0, 300, 200, 100, 50, 100, 200, 300};

void setup() {
    DDRC  = 0xff;
    DDRB  = 0xff;
    DDRD  = 0xff;
    PORTB = 0xff;
    DisableMotor;
    LedBlue0;
    LedRed0;
    MS1_1;
    MS2_1;
    MS3_1;
    P0;
    D0;
    DeviceID = EEPROM.read(eeprom_ID);
    if (DeviceID == 0xff) {
        DeviceID = 0x01;
        EEPROM.write(eeprom_ID, 0x01);
    }

    Serial.begin(9600);
    for (int i = 0; i < 5; i++) {
        LedBlue1;
        delay(20);
        LedRed0;
        delay(20);
        LedBlue0;
        delay(20);
        LedRed1;
        delay(20);
    }
    LedRed0;
    LedBlue0;
}

void SerailEvent() {
    while (Serial.available()) {
        LedBlue1;
        byte rd = Serial.read();
        switch (CommandStatus) {
            case 0x00:
                if (rd == 0xAA)
                    CommandStatus = 0x01;
                else
                    CommandStatus = 0;
                break;  //查询到包头
            case 0x01:
                if (rd == DeviceID)
                    CommandStatus = 0x02;
                else
                    CommandStatus = 0;
                break;  //核对ID
            case 0x02:  //不同的命令类型
                switch (rd) {
                    case 0xff:
                        CommandStatus = 0xf0;
                        break;  //询问是否在线 AA ID FF 55 -> 'Y'
                    case 0x10:
                        CommandStatus = 0x10;
                        break;  //直接移动  AA ID 10 High Low 55 ->'Y'
                    case 0x20:  //缓慢移动
                    case 0x30:  //设置行程
                    case 0x40:  //设置最高速度
                        break;
                }
                break;

            case 0x10:  //设置运动终点的高位，单位为mm
                DataH         = rd;
                CommandStatus = 0x11;
                break;
            case 0x11:  //设置运动终点的高位，
                DataL         = rd;
                CommandStatus = 0x12;
                break;
            case 0x12:  //
                if (rd == 0x55) {
                    Target = DataH * 256 + DataL;
                    Serial.write(Target / 256);
                    Serial.write(Target % 256);
                }
                CommandStatus = 0;
                break;

            case 0xf0:  //询问是否在线
                if (rd == 0x55) {
                    LedRed1;
                    Serial.write(DeviceID);
                    LedRed0;
                }
                CommandStatus = 0;
                break;
        }
    }
    LedBlue0;
}
unsigned int TimeMap[141] = {
    0,   376, 356, 337, 319, 303, 287, 272, 257, 244, 231, 219, 208, 197, 187, 177,
    168, 159, 151, 144, 136, 129, 123, 117, 111, 105, 100, 95,  91,  86,  82,  78,
    74,  71,  68,  65,  62,  59,  56,  54,  51,  49,  47,  45,  43,  41,  39,  38,
    36,  35,  34,  32,  31,  30,  29,  28,  27,  26,  25,  24,  23,  23,  22,  21,
    21,  20,  20,  19,  19,  18,  18,  17,  17,  16,  16,  16,  15,  15,  15,  15,
    14,  14,  14,  14,  14,  13,  13,  13,  13,  13,  13,  12,  12,  12,  12,  12,
    12,  12,  12,  12,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,  11,
    11,  11,  11,  11,  11,  11,  11,  10,  10,  10,  10,  10,  10,  9,   9,   9,
    9,   8,   8,   8,   8,   8,   7,   7,   7,   7,   7,   6,   6};

int AreaA, AreaB, AreaC = 0;
#define Stop     0
#define StatusA  1
#define StatusB  2
#define StatusC  3
#define Status_A -1
#define Status_B -2
#define Status_C -3
void loop() {
    // put your main code here, to run repeatedly:

    SerailEvent();
    switch (Status) {
        case Stop:
            Err = Target - Now;
            if (Err == 0) {
                LedRed0;
                DisableMotor;
            } else {
                if (Err > 0)  //从零起步的正向走
                {
                    Status = StatusA;
                    if (Err >= 280)  //先判断是否有完整加减速过程
                    {
                        AreaA = 140;
                        AreaC = 140;
                        Area  = Err - AreaA - AreaC;
                        Speed = TimeMap[0];
                        D0;
                        P0;
                        P1;
                        P0;
                    } else {
                        AreaA = Err / 2;
                        AreaB = 0;
                        AreaC = Err - AreaA;
                        Speed = TimeMap[0];
                        D0;
                        P0;
                        P1;
                        P0;
                    }
                }
            }
    }
    if (Target == Now) {
        LedRed0;
        DisableMotor;
    } else {
        LedRed1;
        EnableMotor;
        if (Target > Now) Err = Target - Now;
    }
}
