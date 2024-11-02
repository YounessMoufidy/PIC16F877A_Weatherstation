#include <xc.h>
#include "config.h"
#define _XTAL_FREQ 4000000 // Fosc
#define I2C_BaudRate 100000 // I2C Baud Rate = 100 Kbps

#define SCL_D                 TRISC3
#define SDA_D                 TRISC4
 
#define LCD_BACKLIGHT         0x08
#define LCD_NOBACKLIGHT       0x00
#define LCD_FIRST_ROW         0x80
#define LCD_SECOND_ROW        0xC0
#define LCD_THIRD_ROW         0x94
#define LCD_FOURTH_ROW        0xD4
#define LCD_CLEAR             0x01
#define LCD_RETURN_HOME       0x02
#define LCD_ENTRY_MODE_SET    0x04
#define LCD_CURSOR_OFF        0x0C
#define LCD_UNDERLINE_ON      0x0E
#define LCD_BLINK_CURSOR_ON   0x0F
#define LCD_MOVE_CURSOR_LEFT  0x10
#define LCD_MOVE_CURSOR_RIGHT 0x14
#define LCD_TURN_ON           0x0C
#define LCD_TURN_OFF          0x08
#define LCD_SHIFT_LEFT        0x18
#define LCD_SHIFT_RIGHT       0x1E
#define LCD_TYPE              2 // 0 -> 5x7 | 1 -> 5x10 | 2 -> 2 lines
#include "string.h"
#include <stdio.h> 
#include <stdlib.h>
  unsigned char RS, i2c_add, BackLight_State = LCD_BACKLIGHT;
//----------------------------------------------------------
//-----------------[ Functions' Prototypes ]----------------
void I2C_Master_Init(void);
void I2C_Wait(void);
void I2C_Start(void);

void I2C_Stop(void);
void I2C_Restart(void);
void I2C_ACK(void);
void I2C_NACK(void);
unsigned char I2C_Write(unsigned char Data);
unsigned char I2C_Read(void);
//----------------------------------------------------------
//EEPROM_functions
void EEPROM_write(char ,char ,unsigned char );
uint8_t  EEPROM_Read(char ,char );
//---[ LCD Routines ]---
 
void LCD_Init(unsigned char );
void IO_Expander_Write(unsigned char );
void LCD_Write_4Bit(unsigned char );
void LCD_CMD(unsigned char );
void LCD_Set_Cursor(unsigned char , unsigned char );
void LCD_Write_Char(char);
void LCD_Write_String(char*);
void Backlight();
void noBacklight();
void LCD_SR();
void LCD_SL();
void LCD_Clear();

uint8_t Temp_value;
#define ADDRESSH 0x31
#define ADDRESSL 0x38

void tostring(char str[], int num)
{
    int i, rem, len = 0, n;
 
    n = num;
    while (n != 0)
    {
        len++;
        n /= 10;
    }
    for (i = 0; i < len; i++)
    {
        rem = num % 10;
        num = num / 10;
        str[len - (i + 1)] = rem + '0';
    }
    str[len] = '\0';
}

void main()
{
  TRISD=0;
  I2C_Master_Init();
  LCD_Init(0x4E); // Initialize LCD module with I2C address = 0x4E
  LCD_Set_Cursor(1, 1);
  LCD_Write_String("Temperature");
  char str[30];
  while(1)
  {
    I2C_Start(); // I2C Start Sequence
    I2C_Write(0x41); // I2C Slave Device Address + Write
    Temp_value=I2C_Read(); // The Data To Be Sent
    I2C_Stop(); // I2C Stop Sequence
    PORTDbits.RD0=1;
    __delay_ms(500);
    //itoa(Temp_value,str,10);
    tostring(str,Temp_value);
    EEPROM_write(ADDRESSH,ADDRESSL,Temp_value);
    LCD_Set_Cursor(2, 1);
    LCD_Write_String(str);
    __delay_ms(2500);
  }
}
//------------[ END OF MAIN ]--------------
//-----------------------------------------
void I2C_Master_Init()
{
  SSPCON = 0x28;
  SSPCON2 = 0x00;
  SSPSTAT = 0x00;
  SSPADD = ((_XTAL_FREQ/4)/I2C_BaudRate) - 1;
  TRISC3 = 1;
  TRISC4 = 1;
}
void I2C_Wait()
{
  while ((SSPSTAT & 0x04) || (SSPCON2 & 0x1F));
}
void I2C_Start()
{
  //---[ Initiate I2C Start Condition Sequence ]---
  I2C_Wait();
  SEN = 1;
}
void I2C_Stop()
{
  //---[ Initiate I2C Stop Condition Sequence ]---
  I2C_Wait();
  PEN = 1;
}
void I2C_Restart()
{
  //---[ Initiate I2C Restart Condition Sequence ]---
  I2C_Wait();
  RSEN = 1;
}
void I2C_ACK(void)
{
  //---[ Send ACK - For Master Receiver Mode ]---
  I2C_Wait();
  ACKDT = 0; // 0 -> ACK, 1 -> NACK
  ACKEN = 1; // Send ACK Signal!
}
void I2C_NACK(void)
{
  //---[ Send NACK - For Master Receiver Mode ]---
  I2C_Wait();
  ACKDT = 1; // 1 -> NACK, 0 -> ACK
  ACKEN = 1; // Send NACK Signal!
}
unsigned char I2C_Write(unsigned char Data)
{
  //---[ Send Byte, Return The ACK/NACK ]---
  I2C_Wait();
  SSPBUF = Data;
  I2C_Wait();
  return ACKSTAT;
}

unsigned char I2C_Read()
{
  unsigned char Data;
  I2C_Wait();
  SSPCON2bits.RCEN = 1;
  I2C_Wait();
  Data = SSPBUF;
  I2C_NACK();
  return Data;
}


//---------------[ LCD Routines ]----------------
//-----------------------------------------------
 
void LCD_Init(unsigned char I2C_Add)
{
  i2c_add = I2C_Add;
  IO_Expander_Write(0x00);
  __delay_ms(30);
  LCD_CMD(0x03);
  __delay_ms(5);
  LCD_CMD(0x03);
  __delay_ms(5);
  LCD_CMD(0x03);
  __delay_ms(5);
  LCD_CMD(LCD_RETURN_HOME);
  __delay_ms(5);
  LCD_CMD(0x20 | (LCD_TYPE << 2));
  __delay_ms(50);
  LCD_CMD(LCD_TURN_ON);
  __delay_ms(50);
  LCD_CMD(LCD_CLEAR);
  __delay_ms(50);
  LCD_CMD(LCD_ENTRY_MODE_SET | LCD_RETURN_HOME);
  __delay_ms(50);
}
 
void IO_Expander_Write(unsigned char Data)
{
  I2C_Start();
  I2C_Write(i2c_add);
  I2C_Write(Data | BackLight_State);
  I2C_Stop();
}
 
void LCD_Write_4Bit(unsigned char Nibble)
{
  // Get The RS Value To LSB OF Data
  Nibble |= RS;
  IO_Expander_Write(Nibble | 0x04);
  IO_Expander_Write(Nibble & 0xFB);
  __delay_us(50);
}
 
void LCD_CMD(unsigned char CMD)
{
  RS = 0; // Command Register Select
  LCD_Write_4Bit(CMD & 0xF0);
  LCD_Write_4Bit((CMD << 4) & 0xF0);
}
 
void LCD_Write_Char(char Data)
{
  RS = 1; // Data Register Select
  LCD_Write_4Bit(Data & 0xF0);
  LCD_Write_4Bit((Data << 4) & 0xF0);
}
 
void LCD_Write_String(char* Str)
{
  for(int i=0; Str[i]!='\0'; i++)
    LCD_Write_Char(Str[i]);
}
 
void LCD_Set_Cursor(unsigned char ROW, unsigned char COL)
{
  switch(ROW)
  {
    case 2:
      LCD_CMD(0xC0 + COL-1);
      break;
    case 3:
      LCD_CMD(0x94 + COL-1);
      break;
    case 4:
      LCD_CMD(0xD4 + COL-1);
      break;
    // Case 1
    default:
      LCD_CMD(0x80 + COL-1);
  }
}
 
void Backlight()
{
  BackLight_State = LCD_BACKLIGHT;
  IO_Expander_Write(0);
}
 
void noBacklight()
{
  BackLight_State = LCD_NOBACKLIGHT;
  IO_Expander_Write(0);
}
 
void LCD_SL()
{
  LCD_CMD(0x18);
  __delay_us(40);
}
 
void LCD_SR()
{
  LCD_CMD(0x1C);
  __delay_us(40);
}
 
void LCD_Clear()
{
  LCD_CMD(0x01);
  __delay_us(40);
}


void EEPROM_write(char Address_High,char Address_LOW,unsigned char data)
{
    I2C_Start();
    I2C_Write(0xA0);//The address +R/W
    I2C_Write(Address_High);//the adreess of the place to write
    I2C_Write(Address_LOW);
    I2C_Write(data);
    I2C_Stop();

    
}

uint8_t  EEPROM_Read(char Address_High,char Address_LOW)
{
    uint8_t returned_data=0;
    I2C_Start();
    I2C_Write(0xA0);//The address +R/W
    I2C_Write(Address_High);//the adreess of the place to write
    I2C_Write(Address_LOW);
    I2C_Restart();
    I2C_Write(0xA1);
    returned_data=I2C_Read();
    I2C_NACK();//pour terminer la transmission
    I2C_Stop();
    return returned_data;

}