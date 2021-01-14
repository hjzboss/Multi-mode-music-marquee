#include <reg51.h>
#include <intrins.h>
#define uint unsigned int
#define uchar unsigned char
#define MAXSPEED 2
#define MINSPEED 0
#define LASTMUSIC 3
#define FIRSTMUSIC 0
#define FIRSTMODE 1
#define LASTMODE 6

uchar led_mode;							 //led模式指针
uchar code speed_h[3] = {0x6a, 0x20, 0}; //led闪烁间隔，定时器计数器高位
uchar code speed_l[3] = {0xff, 0, 0};	 //低位
uchar sp;								 //定时器数组下标
uchar led_index;						 //led数组的下标
uchar music_index;						 //音乐下标
bit music_change_flag;					 //音乐切换标志位
bit speed_up_flag;						 //加速标志位
bit speed_down_flag;					 //减速标志位
uint led_delay_time;					 //led灯额外的延迟时间，用于最慢的速度

sbit key1 = P3 ^ 0; //按钮1，减速
sbit key2 = P3 ^ 1; //加速按钮
sbit key3 = P3 ^ 2; //切换跑马灯按钮
sbit key4 = P3 ^ 3; //切歌按钮

sbit sound = P1 ^ 5; //蜂鸣器端口

uchar code digit_light[16] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f,
									   0x6f, 0x77, 0x7c, 0x39, 0x5e, 0x79, 0x71}; //数码管

//led灯闪烁顺序，共有5种模式，每种模式对应一个数组，下标指针为led_index为通用，按照数组循环闪烁，数组中每个元素对应一种跑马灯的亮和灭
uchar code mode_one[9] = {0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff}; 
uchar code mode_two[2] = {0x00, 0xff};											 
uchar code mode_three[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
uchar code mode_four[6] = {0xf0, 0x0f, 0x00, 0xff, 0x00, 0xff};
uchar code mode_five[6] = {0xff, 0x7e, 0xbd, 0xdb, 0xe7, 0xff};

uchar code music_tone[][50] = {{21, 21, 19, 21, 15, 16, 21, 21, 19, 21, 14, 15, 21, 21, 10, 12, 15, 16, 19, 11, 11, 12, 15, 14, 15, 0},
							   {21, 21, 25, 25, 26, 26, 25, 24, 24, 23, 23, 22, 22, 21, 25, 25, 24, 24, 23, 23, 22, 25, 25, 24, 24, 23, 23, 22, 0},
							   {0x18, 0x1c, 0x20, 0x1c, 0x18, 0x20, 0x1c, 0x18, 0x1c, 0x20, 0x1c, 0x18, 0x20, 0}}; //音高
uchar code music_long[][50] = {{18, 6, 24, 24, 24, 48, 18, 6, 24, 24, 24, 48, 18, 6, 24, 24, 24, 24, 24, 18, 6, 24, 24, 24, 48, 0},
							   {20, 20, 20, 20, 20, 20, 10, 20, 20, 20, 20, 20, 10, 20, 20, 20, 20, 20, 20, 10, 20, 20, 20, 20, 20, 20, 10, 0},
							   {0x30, 0x10, 0x40, 0x10, 0x10, 0x10, 0x10, 0x40, 0x20, 0x20, 0x20, 0x20, 0x80, 0}}; //音长

/*
*	通用延时time秒函数
*/
void delay(uint time)
{
	uint b, c;
	for (b = time; b > 0; b--)
	{
		for (c = 120; c > 0; c--)
			;
	}
}

/*
*	led灯延时函数，用于最慢的速度
*/
void delay_s()
{
	while (led_delay_time--)
		;
}

/*
*	led模式1
*/
void led_one()
{
	P0 = digit_light[1];
	if (led_index >= 9)
		led_index = 0;
	P2 = mode_one[led_index];
	led_index++;
	delay_s();
}

/*
*	led模式2
*/
void led_two()
{
	P0 = digit_light[2];
	P2 = ~P2;
	delay_s();
}

/*
*	led模式3
*/
void led_three()
{
	P0 = digit_light[3];
	if (led_index >= 8)
		led_index = 0;
	P2 = mode_three[led_index];
	led_index++;
	delay_s();
}

/*
*	led模式4
*/
void led_four()
{
	P0 = digit_light[4];
	if (led_index >= 6)
		led_index = 0;
	P2 = mode_four[led_index];
	led_index++;
	delay_s();
}

/*
*	led模式5
*/
void led_five()
{
	P0 = digit_light[5];
	if (led_index >= 6)
		led_index = 0;
	P2 = mode_five[led_index];
	led_index++;
	delay_s();
}

/*
*	闪烁一次led灯
*/
void led_show()
{
	switch (led_mode)
	{
	case 1:
	{
		led_one();
		break;
	}
	case 2:
	{
		led_two();
		break;
	}
	case 3:
	{
		led_three();
		break;
	}
	case 4:
	{
		led_four();
		break;
	}
	case 5:
	{
		led_five();
		break;
	}
	default:
		break;
	}
}

/*
*	初始化函数
*/
void initial()
{
	TMOD = 0x11;//定时器工作方式3
	TH0 = 0XEE;//计数器高位
	TL0 = 0X00;//计数器低位
	ET0 = 1;//定时器中断允许
	TR0 = 1;//打开t0中断，用于跑马灯
	EX0 = 1;//允许中断0
	IT0 = 0;//低电平有效
	EX1 = 1;//允许中断1
	IT1 = 0;//低电平有效
	EA = 1;//全局中断打开
	sp = MINSPEED;//速度指针指向最慢
	led_mode = FIRSTMODE;//设置led灯模式为1
	music_index = FIRSTMUSIC;//第一首歌
	led_index = 0;//led灯数组下标为0
	led_delay_time = 0;
	led_show();
}

/*
*	播放音乐，在此函数中还会检测加减速按钮是否按下
*/
void play_music(uchar num)
{
	if (music_change_flag == 1)
	{
		music_change_flag = 0;
		return;
	} //如果音乐标志位为1，则退出循环
	else
	{
		uint i = 0, j, k;
		while (music_long[num][i] != 0 || music_tone[num][i] != 0)
		{
			if (!key1)
			{
				speed_up_flag = 1;
				return;
			} //将加速标志置1，当从函数返回时在main函数中调整速度
			if (!key2)
			{
				speed_down_flag = 1;
				return;
			}//同理
			for (j = 0; j < music_long[num][i] * 20; j++)
			{
				if (music_change_flag == 1)
				{
					music_change_flag = 0;
					return;
				}
				if (!key1)
				{
					speed_up_flag = 1;
					return;
				}
				if (!key2)
				{
					speed_down_flag = 1;
					return;
				}
				sound = ~sound;
				for (k = 0; k < music_tone[num][i]; k++)
					;
			}
			delay(10);
			i++;
		}
	}
}

void main()
{
	initial();
	while (1)
	{
		play_music(music_index);
		//检测加减速标志位，调整速度指针
		if (speed_up_flag == 1)
		{
			if (sp == MAXSPEED)
			{
				//什么也不做，已到达最大值
			}
			else if (sp < MAXSPEED)
			{
				sp++;
			}
			//标志位清零
			speed_up_flag = 0;
		}
		if (speed_down_flag == 1)
		{
			if (sp == MINSPEED)
			{
			}
			else if (sp > MINSPEED)
				sp--;
			speed_down_flag = 0;
		}
	}
}

/*
*	中断0用于切换跑马灯模式，检测按钮3
*/
void change_mode() interrupt 0
{
	delay(500); //消抖
	if (!key3)
	{
		led_mode++;
		if (led_mode == LASTMODE)
			led_mode = FIRSTMODE;
		delay(10);
	}
}

/*
*	计数器中断用于播放跑马灯，每次计数器为0时产生一次中断，按照模式数组闪烁一次跑马灯，再重装计数器初值
*/
void led_control() interrupt 1
{
	led_delay_time = 0;
	TH0 = speed_h[sp];//重装初值
	TL0 = speed_l[sp];
	if (sp == MINSPEED)
		led_delay_time = 1000; //速度为最慢时，增加额外延时
	led_show();
}

/*
*	用于按钮4，切歌
*/
void change_music() interrupt 2
{
	delay(500);
	if (!key4)
	{
		music_change_flag = 1;//切歌按钮标志为1，播放音乐时会进行检测，切歌
		music_index++;
		if (music_index == LASTMUSIC)
			music_index = FIRSTMUSIC;
	}
}
