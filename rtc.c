#include "includes.h"
/*
此头文件包含GPIO寄存器定义
*/

#define IN 	1
#define OUT 0
#define ACK 0x00
#define NACK 0x01
#define SDA_O 		PC_ODR_5
#define SDA_I		PC_IDR_5
#define SCL 		PC_ODR_4

//Device Address & opreation
#define PCF8563		0xA2
#define WRITE		0x00
#define READ		0x01
//RTC RAM map
#define RTC_CR1		0x00
#define RTC_CR2		0x01
#define RTC_SEC		0x02
#define RTC_MIN		0x03
#define RTC_HOU		0x04
#define RTC_DAY		0x05
#define RTC_WEK		0x06
#define RTC_MON		0x07

uchar RTC_RAM[16];
/*BCD 2 DEC*/
uchar BCD2DEC(uchar BCD)
{
	return (10*((BCD&0xf0)>>4) + (BCD&0x0f));
}
/*DEC 2 BCD*/
uchar DEC2BCD(uchar DEC)
{
	return (((DEC/10)<<4)|(DEC%10));
}
/*SDA Function alternate*/
void SET_SDA(uchar s)
{
	if(s == IN)
	{
		PC_DDR &= 0B11011111;
		PC_CR1 &= 0B11011111;//SDA
	}
	else
	{
		PC_DDR |= 0B00100000;
		PC_CR1 |= 0B00100000;//SDA
	}
}
/*delay t us*/
//时间不需要很严格
void delay(uchar t)
{
	char i = -10;
	for(;i < t;i++); 
}
/*IIC IO Initialize*/
void IIC_Init(void)
{
	SET_SDA(OUT);
	SCL = 1;
	SDA_O = 1;
}
/*I2C start signal*/
void startIIC(void)
{
	SDA_O = 1;
	SCL   = 1;
	delay(1);
	SDA_O = 0;
	delay(1);
	SCL   = 0;
}
/*I2C stop signal*/
void stopIIC(void)
{
	SCL   = 0;
	SDA_O = 0;
	delay(1);
	SCL   = 1;
	delay(1);
	SDA_O = 1;
}
/*Send One Byte data to the I2C Bus*/
static void write(uchar data)
{
	uchar i;
	SCL 	= 0;
	SDA_O = 0;
	for(i = 0;i < 8;i++)
	{
		if(data&0x80)
		{
			SDA_O = 1;
		}
		else
		{
			SDA_O = 0;
		}
		data <<= 1;
		delay(2);
		SCL = 1;
		delay(2);
		SCL = 0;
	}
	SDA_O = 0;
}
/*Receive ACK*/
uchar receiveACK(void)
{
	uchar ack;
	SDA_O = 1;
	SET_SDA(IN);
	delay(2);
	SCL = 1;
	delay(2);
	ack = SDA_I;
	SCL = 0;
	SDA_O = 0;
	SET_SDA(OUT);
	return ack;
}
/*Read One byte From I2C Bus*/
static uchar read(void)
{
	uchar byte,i;
	
	SDA_O = 1;
	SET_SDA(IN);
	SCL 	= 0;
	for(i = 0;i < 8;i++)
	{
		byte <<= 1;
		if(SDA_I)
		{
			byte++;
		}
		delay(2);
		SCL = 1;
		delay(2);
		SCL = 0;
	}
	SDA_O = 0;
	SET_SDA(OUT);
	return byte;
}
/*ACK or NACK send function*/
void sendACK(uchar ack)
{
	SCL = 0;
	if(ack == ACK)
	{
		SDA_O = 0;
	}
	else
	{
		SDA_O = 1;
	}
	delay(2);
	SCL = 1;
	delay(2);
	SCL = 0;
}
/*Write One Byte*/
static void writeByte(uchar addr,uchar data)
{
	uchar i;
	startIIC();
	write(PCF8563|WRITE);
	i = receiveACK();
	write(addr);
	i = receiveACK();
	write(data);
	i = receiveACK();
	stopIIC();
}
/*Read One Byte*/
static uchar readByte(uchar addr)
{
	uchar byte;

	startIIC();
	write(PCF8563|WRITE);
	byte = receiveACK();
	write(addr);
	byte = receiveACK();
	startIIC();
	write(PCF8563|READ);
	byte = receiveACK();
	byte = read();
	sendACK(NACK);
	stopIIC();
	return byte;
}
/*Write RTC*/
void updateRTC(void)
{
	uchar i;
	startIIC();
	write(PCF8563|WRITE);
	i = receiveACK();
	write(0x00);
	i = receiveACK();
	for(i = 0;i < 16;i++)
	{
		write(RTC_RAM[i]);
		i = receiveACK();
	}
	stopIIC();
}
/*Read RTC*/
void readRTC(void)
{
	uchar i;

	startIIC();
	write(PCF8563|WRITE);
	i = receiveACK();
	write(0x02);
	i = receiveACK();
	startIIC();
	write(PCF8563|READ);
	i = receiveACK();
	for(i = 2;i < 5;i++)
	{
		RTC_RAM[i] = read();
		if(i == 4)sendACK(NACK);
		else sendACK(ACK);
	}
	stopIIC();
}
/*RTC Initialize*/
void _RTC_Init(void)
{
	IIC_Init();
	//writeByte(RTC_SEC,0x05);
 	//writeByte(RTC_MIN,0x49);
	//writeByte(RTC_HOU,0x10);
//	readRTC();
	writeByte(RTC_CR1,0x00);//Run RTC
}
/*Get Time*/
/*
需引入外部结构体:
struct TimeType_Def{
uchar hour;
uchar minute;
uchar second;
}
*/
void _getTime(void)
{
//	if((MachineStatus == SETTING)&&(SetMode == TIME_SET))return;
	readRTC();
	Now.Second = BCD2DEC(RTC_RAM[2]&0x7f);
	Now.Minute = BCD2DEC(RTC_RAM[3]&0x7f);
	Now.Hour = BCD2DEC(RTC_RAM[4]&0x3f);
}
/*Set Time*/
void _setTime(void)
{
	writeByte(RTC_HOU,DEC2BCD(Now.Hour));
	writeByte(RTC_MIN,DEC2BCD(Now.Minute));
	writeByte(RTC_SEC,DEC2BCD(0));
	writeByte(RTC_CR1,0x00);//Run RTC
}
