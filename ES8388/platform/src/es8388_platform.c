#include "main.h"
#include "es8388_platform.h"
#include "tim.h"
#include "spi.h"
#include "i2c.h"
#include "string.h"
#include "stdio.h"
#include "mode.h"

#include "minimp3_platform.h"


#define AUDIO_LOG_ENABLED	0

bool b_AUDIO_Init();

int i_AUDIO_DAC_Digital_Volume(int* pi_init, int16_t i16_vol, uint16_t u16_dot);
int i_AUDIO_DAC_Analog_Volume(int* pi_init, e_AUDIO_OUT_t e_out, int16_t i16_vol);
int i_AUDIO_ADC_Volume(int* pi_init, int16_t i16_vol, uint16_t u16_dot);

int i_AUDIO_Reset(int* pi_init);
bool b_AUDIO_CODEC();
int i_AUDIO_Playback(int* pi_init);
int i_AUDIO_PWR_Down(int* pi_init);
int i_AUDIO_PWR_Up(int* pi_init);
int i_AUDIO_Mute(int* pi_init);
int i_AUDIO_UnMute(int* pi_init);



static int i_toutAct;
static uint32_t u32_toutRef;

static e_COMM_STAT_t e_codec_config;
static e_COMM_STAT_t e_codec_evt;

uint8_t u8_Codec_rdBuf[16];


void v_Codec_WrDone(){
	e_codec_evt = COMM_STAT_DONE;
	i_toutAct = 0;
}

void v_Codec_RdDone(uint8_t* pu8_arr, uint16_t u16_len){
	e_codec_evt = COMM_STAT_DONE;
	i_toutAct = 0;
	memcpy(u8_Codec_rdBuf, pu8_arr, u16_len);
}


int i_Codec_Write(uint16_t u16_memAddr, uint8_t* pu8, uint16_t u16_cnt){
	i_toutAct = 1;
	u32_toutRef = u32_Tim_1msGet();
	e_codec_evt = COMM_STAT_BUSY;
	return i_I2C5_Write(ADDR_CODEC, u16_memAddr, pu8, u16_cnt);
}

int i_Codec_Read(uint16_t u16_memAdddr, uint16_t u16_cnt){
	i_toutAct = 1;
	u32_toutRef = u32_Tim_1msGet();
	e_codec_evt = COMM_STAT_BUSY;
	return i_I2C5_Read(ADDR_CODEC, u16_memAdddr, u16_cnt);
}

void v_Codec_Tout_Handler(){
	if(i_toutAct && _b_Tim_Is_OVR(u32_Tim_1msGet(), u32_toutRef, 2000)){
		//timeout
		i_toutAct = 0;
		e_codec_config = COMM_STAT_ERR;
		v_Mode_Set_Error(modeERR_AUDIO);
		v_Mode_SetNext(modeERROR);
	}
}


void v_Codec_Deinit(){
	e_codec_config = COMM_STAT_READY;
	e_codec_evt = COMM_STAT_READY;
}

e_COMM_STAT_t e_Codec_Ready(){
	static uint16_t order;
	static int init = 1;
	if(e_codec_config == COMM_STAT_READY && e_codec_evt != COMM_STAT_BUSY){
		switch(order){
		case 0:
			if(i_AUDIO_Reset(&init) != 1){break;}
			order++;
			init = 1;
		case 1:
			if(i_AUDIO_Playback(&init) != 1){break;}
			order++;
			init = 1;
		case 2:
			if(i_AUDIO_DAC_Digital_Volume(&init, 0, 0) != 1){break;}
			order++;
			init = 1;
		case 3:
			if(i_AUDIO_DAC_Analog_Volume(&init, AUDIO_OUT_OUT1, MODE_SOUND_VOLUME_INIT * 10) != 1){break;}
			order++;
			init = 1;
		case 4:
			order = 0;
			init = 1;
			return e_codec_config = COMM_STAT_DONE;
		}
	}
	return e_codec_config;
}




int i_Codec_Volume_Ctrl(int16_t i16_vol){
	//volume check
	static int init;
	static int16_t prev_vol = -1;
	if(e_codec_config != COMM_STAT_DONE){return 0;}
	if(e_codec_evt == COMM_STAT_BUSY){return 0;}
	if(prev_vol != i16_vol){
		prev_vol = i16_vol;
		init = true;
	}
	return i_AUDIO_DAC_Analog_Volume(&init, AUDIO_OUT_OUT1, i16_vol);
#if 0
	int16_t vol;
	if(i16_vol == 0){
		vol = -96;
	}
	else{
		vol = -(100 - i16_vol);
	}

	return i_AUDIO_DAC_Digital_Volume(&init, vol, 0);
#endif
}



void v_AUDIO_Test(){
	static uint32_t timRef, timItv;
	static bool config;
	static volatile uint16_t order;
	static int init;
	//interval : 10s
	if(_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){
		timRef = u32_Tim_1msGet();

		if(config == false && e_codec_evt != COMM_STAT_BUSY){
			timItv = 100;

			switch(order){
			case 0:
				if(i_AUDIO_Reset(&init) != 1){break;}
				init = 1;
				order++;
			case 1:
				if(i_AUDIO_Playback(&init) != 1){break;}
				init = 1;
				order++;
			case 2:
				if(i_AUDIO_DAC_Digital_Volume(&init, 0, 0) != 1){break;}
				init = 1;
				order++;
			case 3:
				if(i_AUDIO_DAC_Analog_Volume(&init, AUDIO_OUT_LOUT1, 30) != -1){break;}
				init = 1;
				order++;
			case 4:
				if(i_AUDIO_DAC_Analog_Volume(&init, AUDIO_OUT_OUT2, 30) != 1){break;}
				init = 1;
				order++;
				break;
			default:
				order = 0;
				config = true;
				printf("config - complete\n");
				break;
			}
		}
		else{
			if(order == 0){
				printf("speaker on\n");
				HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_SET);
				HAL_Delay(100);
				order++;
			}

			if(order == 1){
				v_MP3_Play_T();
				order++;
			}

			if(order == 2){
				HAL_GPIO_WritePin(DO_AUDIO_SHDN_GPIO_Port, DO_AUDIO_SHDN_Pin, GPIO_PIN_RESET);
				printf("speaker off\n");
				timRef = u32_Tim_1msGet();
				timItv = 5000;
				order = 0;
			}
		}
	}
}



int i_AUDIO_ADC_Volume(int* pi_init, int16_t i16_vol, uint16_t u16_dot){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	uint8_t wr[4];
	uint8_t vol=0;
	if(i16_vol < -96 || i16_vol > 0){
		if(i16_vol < -96)	{vol = -96;}
		else				{vol = 0;}
	}
	uint16_t dot = u16_dot >= 5 ? 1 : 0;
	vol = (-vol << 1) + dot;

	wr[0] = vol;

	switch(order){
	case 0:
		if(i_Codec_Write(ES8388_REG_ADC_CTRL8, wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 1:
		if(i_Codec_Write(ES8388_REG_ADC_CTRL9, wr, 1) != COMM_STAT_OK){return -1;}
		break;
	default:
		return 1;
	}
	order++;
	return 0;
}



int i_AUDIO_DAC_Digital_Volume(int* pi_init, int16_t i16_vol, uint16_t u16_dot){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	uint8_t wr[4];
	uint8_t vol;
	if(i16_vol < -96 || i16_vol > 0){
		if(i16_vol < -96)	{i16_vol = -96;}
		else				{i16_vol = 0;}
	}
	uint16_t dot = u16_dot >= 5 ? 1 : 0;
	vol = (-i16_vol << 1) + dot;
	wr[0] = vol;

	switch(order){
	case 0:
		if(i_Codec_Write(ES8388_REG_DAC_CTRL4, wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 1:
		if(i_Codec_Write(ES8388_REG_DAC_CTRL5, wr, 1) != COMM_STAT_OK){return -1;}
		break;
	default:
		return 1;
	}
	order++;
	return 0;
}


/*
 * brief	: LOUTn, ROUTn output volume control
 * date
 * - create	: 25.06.11
 * note
 * - volume range : 0 ~ 100
 *
 */
int i_AUDIO_DAC_Analog_Volume(int* pi_init, e_AUDIO_OUT_t e_out, int16_t i16_vol){
	static uint16_t order;
	static uint32_t timRef, timItv;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
		timItv = 0;
	}
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){return 0;}
	uint8_t wr[4];
	if(i16_vol < 0)			{i16_vol = 0;}
	else if(i16_vol > 100)	{i16_vol = 100;}
	uint8_t vol = i16_vol / 3;
	wr[0] = vol;

	if(e_out < AUDIO_OUT_OUT1){
		uint8_t reg = ES8388_REG_DAC_CTRL24 + e_out;
		switch(order){
		case 0:
			if(i_Codec_Write(reg, wr, 1) != COMM_STAT_OK){return -1;}
			break;
		default:
			return 1;
		}
	}
	else{
		if(e_out == AUDIO_OUT_OUT1){
			//LOUT1, ROUT1
			switch(order){
			case 0:
				if(i_Codec_Read(ES8388_REG_DAC_CTRL3, 1) != COMM_STAT_OK){return -1;}
				break;
			case 1:
				wr[0] = u8_Codec_rdBuf[0];
				if(vol == 0){wr[0] |= 0x04;}	//mute
				else		{wr[0] &= ~0x04;}	//normal
				if(i_Codec_Write(ES8388_REG_DAC_CTRL3, wr, 1) != COMM_STAT_OK){return -1;}
				break;
			case 2:
				if(i_Codec_Write(ES8388_REG_DAC_CTRL24, wr, 1) != COMM_STAT_OK){return -1;}
				break;
			case 3:
				if(i_Codec_Write(ES8388_REG_DAC_CTRL25, wr, 1) != COMM_STAT_OK){return -1;}
				break;
			default:
				return 1;
			}
		}
		else{
			//LOUT2, ROUT2
			switch(order){
			case 0:
				if(i_Codec_Write(ES8388_REG_DAC_CTRL26, wr, 1) != COMM_STAT_OK){return -1;}
				break;
			case 1:
				if(i_Codec_Write(ES8388_REG_DAC_CTRL27, wr, 1) != COMM_STAT_OK){return -1;}
				break;
			default:
				return 1;
			}
		}
	}
	order++;
	return 0;
}

/*
 * brief	:
 */
bool b_AUDIO_Start(){
	return true;
}


/*
 * brief	:
 */
bool b_AUDIO_Stop(){
	return true;
}


int i_AUDIO_Reset(int* pi_init){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	static uint32_t timRef, timItv;
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, timItv)){return false;}
	uint8_t wr;
	switch(order){
	case 0:
		//CHIP_CTRL1	: 0x80
		wr = 0x80;
		if(i_Codec_Write(ES8388_REG_CHIP_CTRL1, &wr, 1) != COMM_STAT_OK){return -1;}
		timRef = u32_Tim_1msGet();
		timItv = 100;
		break;
	case 1:
		//CHIP_CTRL1	: 0x00
		wr = 0x00;
		if(i_Codec_Write(ES8388_REG_CHIP_CTRL1, &wr, 1) != COMM_STAT_OK){return -1;}
		timRef = u32_Tim_1msGet();
		timItv = 100;
		break;
	default:
		return 1;
	}
	order++;
	return 0;
}


int i_AUDIO_Playback(int* pi_init){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	static uint32_t timRef;
	static uint32_t delay;
	uint8_t wr;
	if(!_b_Tim_Is_OVR(u32_Tim_1msGet(), timRef, delay)){return false;}
	switch(order){
	case 0:
		//power down whole chip analog	: 0x58
		/*
		 * RSVD			: 	01
		 * LPVcmMod		: 	0	: normal	(default)
		 * LPVrefBuf	:	1	: low power	(default)
		 * PdnAna		:	1	: normal	(default)
		 * PdnIbiasgen	:	0	: normal
		 * VrefLo		:	0	: normal	(default)
		 * PdnVrefbuf	:	0	: normal	(default)
		 */
		wr = ES8388_RSVD_MASK_CHIP_CTRL2(0x58);
		if(i_Codec_Write(ES8388_REG_CHIP_CTRL2, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 1:
		//power up whole chip analog	: 0x50
		/*
		 * RSVD			: 01
		 * LPVcmMod		: 0	: normal	(default)
		 * LPVrefBuf	: 1	: low power (default)
		 * PdnAna		: 0	: normal
		 * PdnIbiasgen	: 0	: normal
		 * VrefLo		: 0	: normal	(default)
		 * PdnVrefbuf	: 0	: normal	(default)
		 */
		wr = ES8388_RSVD_MASK_CHIP_CTRL2(0x50);
		if(i_Codec_Write(ES8388_REG_CHIP_CTRL2, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 2:
		//stop STM and DLL, power down DAC & ADC vref	: 0xF3
		/*
		 * adc_DigPDN	: 1	: reset ADC DEM, filter and serial data port (default)
		 * dac_DigPDN	: 1	: reset DAC DSM, DEM, filter and serial data port (default)
		 * adc_stm_rst	: 1	: reset ADC state machine to power down state
		 * data_stm_rst	: 1	: reset DAC state machine to power down state
		 * ADCDLL_PDN	: 0	: normal (default)
		 * DACDLL_PDN	: 0 : normal (default)
		 * adcVref_PDN	: 1	: ADC analog reference power down (default)
		 * dacVref_PDN	: 1 : DAC analog reference power down (default)
		 */
		wr = 0xF3;
		if(i_Codec_Write(ES8388_REG_CHIP_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 3:
		//power up DAC & ADC vref : 0xF0
		/*
		 * adc_DigPDN	: 1	: reset ADC DEM, filter and serial data port (default)
		 * dac_DigPDN	: 1	: reset DAC DSM, DEM, filter and serial data port (default)
		 * adc_stm_rst	: 1	: reset ADC state machine to power down state
		 * data_stm_rst	: 1	: reset DAC state machine to power down state
		 * ADCDLL_PDN	: 0	: normal (default)
		 * DACDLL_PDN	: 0 : normal (default)
		 * adcVref_PDN	: 0	: ADC analog reference power up
		 * dacVref_PDN	: 0 : DAC analog reference power up
		 */
		wr = 0xF0;
		if(i_Codec_Write(ES8388_REG_CHIP_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 4:
		//set internal ADC and DAC use the same LRCK clock, dac LRCK as internal LRCK : 0x80
		/*
		 * slrck		: 1	: DACLRC and ADCLRC same
		 * lrck_sel		: 0	: use DAC LRCK (default)
		 * offset_dis	: 0	: disable offset (default)
		 * mclk_dis		: 0	: normal (default)
		 * adc_dll_pwd	: 0	: normal (default)
		 * dac_dll_pwd	: 0	: normal (default)
		 * RSVD			: 00
		 */
		wr = ES8388_RSVD_MASK_DAC_CTRL21(0x80);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL21, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 5:
		//ADC clock is same as DAC, use DAC MCLK as internal LRCK	: 0x36
		/*
		 * SCPReset	: 0		: normal (default)
		 * LRCM		: 0		: ALRCK disabled when both ADC disabled; DLRCK disabled when both DAC disabled (default)
		 * DACMCLK	: 1		: when SameFs=1, DACMCLK is the chip master clock source
		 * SameFs	: 1		: ADC Fs is the same as DAC Fs
		 * SeqEn	: 0		: internal power up/down sequence disable (default)
		 * EnRef	: 1		: enable reference (default)
		 * VMIDSEL	: 10	: 500 kΩ divider enabled (default)
		 */
		wr = 0x36;
		if(i_Codec_Write(ES8388_REG_CHIP_CTRL1, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 6:
		//CODEC in I2S master / slave mode	: slave	- 0x00	(-> 0x1B X)
		/*
		 * MSC		: 0		: slave serial port mode
		 * MCLKDIV2	: 0		: MCLK not divide (default)
		 * BCLK_INV	: 0		: normal (default)
		 * BCLKDIV	: 00000	: master mode BCLK generated automatically based on the clock table (default)
		 */
		wr = 0x00;	// -> 32 (16 * 2)	: 11011	: 1B
		wr = 0x1B;
		if(i_Codec_Write(ES8388_REG_MASTER_CTRL, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 7:
		//power up DAC	: 0x00
		/*
		 * PdnDACL	: 0	: left DAC power up
		 */
		wr = ES8388_RSVD_MASK_DAC_PWR(0x00);
		if(i_Codec_Write(ES8388_REG_DAC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 8:
		//low power setting	: 0x00
		wr = ES8388_RSVD_MASK_CHIP_LPWR1(0x00);
		if(i_Codec_Write(ES8388_REG_CHIP_LPWR1, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 9:
		//low power setting : 0xC3
		wr = ES8388_RSVD_MASK_CHIP_LPWR2(0xC3);
		if(i_Codec_Write(ES8388_REG_CHIP_LPWR2, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 10:
#if AUDIO_MONO
		//DAC I2S - 16bit	: 0x18 (change?)	(-> 0x5E -> 0x1E)
		wr = ES8388_RSVD_MASK_DAC_CTRL1(0x18);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL1, &wr, 1) != COMM_STAT_OK){return -1;}
#else
		//DAC I2S - 16bit	: 0x18
		wr = ES8388_RSVD_MASK_DAC_CTRL1(0x18);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL1, &wr, 1) != COMM_STAT_OK){return -1;}
#endif
		break;
	case 11:
		//DACLRCK = MCLK / 256	: 0x02 (256)
		wr = ES8388_RSVD_MASK_DAC_CTRL2(0x02);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL2, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 12:
		//DAC volume 0 dB	: 0x00 (left)
		wr = 0x00;
		if(i_Codec_Write(ES8388_REG_DAC_CTRL4, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 13:
		//DAC volume 0 dB	: 0x00 (right)
		wr = 0x00;
		if(i_Codec_Write(ES8388_REG_DAC_CTRL5, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 14:
#if AUDIO_MONO
		//DAC_CTRL7	:  mono	: mono	(skip - 0x00)
		wr = 0x00;
		if(i_Codec_Write(ES8388_REG_DAC_CTRL7, &wr, 1) != COMM_STAT_OK){return -1;}
#else
		//DAC_CTRL7	:  mono	: mono	(skip - 0x00)
		wr = 0x00;
		if(i_Codec_Write(ES8388_REG_DAC_CTRL7, &wr, 1) != COMM_STAT_OK){return -1;}
#endif
		break;
	case 15:
		//Mixer setting for LDAC to LOUT	: 0xB8	(-> 0x38 X)
		wr = ES8388_RSVD_MASK_DAC_CTRL17(0xB8);
		//wr = ES8388_RSVD_MASK_DAC_CTRL17(0x38);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL17, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 16:
		//Mixer setting for RDAC to ROUT	: 0xB8	(-> 0x38 X)
		wr = ES8388_RSVD_MASK_DAC_CTRL20(0xB8);
		//wr = ES8388_RSVD_MASK_DAC_CTRL20(0x38);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL20, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 17:
		//startup FSM and DLL	: 0xAA
		wr = 0xAA;
		if(i_Codec_Write(ES8388_REG_CHIP_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 18:
		//Dealy	: 500 ms
		timRef = u32_Tim_1msGet();
		delay = 500;
		break;
	case 19:
		//LOUT1 0 dB	: 0x1C
		wr= ES8388_RSVD_MASK_DAC_CTRL24(0x1C);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL24, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 20:
		//ROUT1 0 dB	: 0x1C
		wr = ES8388_RSVD_MASK_DAC_CTRL25(0x1C);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL25, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 21:
		//LOUT2 0 dB	: 0x1C
		wr = ES8388_RSVD_MASK_DAC_CTRL26(0x1C);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL26, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 22:
		//ROUT2 0 dB	: 0x1C
		wr = ES8388_RSVD_MASK_DAC_CTRL27(0x1C);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL27, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 23:
		//enable LOUT1 & ROUT1, enable LOUT2 & ROUT2	: 0x3C
		wr = ES8388_RSVD_MASK_DAC_PWR(0x30);
		if(i_Codec_Write(ES8388_REG_DAC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	case 24:
		//power down ADC, disable LIN & RIN, power down MICBIAS,m set int1lp to low power mode	: 0xFF
		wr = 0xFF;
		if(i_Codec_Write(ES8388_REG_ADC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		break;
	default:
		return 1;
	}
	order++;
	return 0;
}


int i_AUDIO_PWR_Down(int* pi_init){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	static uint8_t wr;
	switch(order){
	case 0:
		//DAC_CTRL3	: 0xE6
		wr = ES8388_RSVD_MASK_DAC_CTRL3(0xE6);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL3, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 1:
		//DAC_PWR	: 0xFC
		wr = ES8388_RSVD_MASK_DAC_PWR(0xFC);
		if(i_Codec_Write(ES8388_REG_DAC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 2:
		//ADC_PWR	: 0xFF
		wr = 0xFF;
		if(i_Codec_Write(ES8388_REG_ADC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 3:
		//CHIP_PWR	: 0xC0
		wr = 0xC0;
		if(i_Codec_Write(ES8388_REG_CHIP_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 4:
		//DAC_CTRL21	: 0x90
		wr = ES8388_RSVD_MASK_DAC_CTRL21(0x90);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL21, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	default:
		return 1;
	}
	return 0;
}


int i_AUDIO_PWR_Up(int* pi_init){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	static uint8_t wr;
	switch(order){
	case 0:
		//DAC_CTRL21	: 0x80
		wr = ES8388_RSVD_MASK_DAC_CTRL21(0x80);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL21, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 1:
		//CHIP_PWR		: 0x00
		wr = 0xC0;
		if(i_Codec_Write(ES8388_REG_CHIP_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 2:
		//ADC_PWR		: 0x00
		wr = 0xC0;
		if(i_Codec_Write(ES8388_REG_ADC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 3:
		//DAC_PWR		: 0x3C
		wr = ES8388_RSVD_MASK_DAC_PWR(0x3C);
		if(i_Codec_Write(ES8388_REG_DAC_PWR, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	case 4:
		//DAC_CTRL3		: 0xE2
		wr = ES8388_RSVD_MASK_DAC_CTRL3(0xE2);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL3, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
		break;
	default:
		return 1;
	}
	return 0;
}


int i_AUDIO_Mute(int* pi_init){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	uint8_t wr;
	if(order == 0){
		wr = ES8388_RSVD_MASK_DAC_CTRL3(0x04);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL3, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
	}
	else{
		return 1;
	}
	return 0;
}


int i_AUDIO_UnMute(int* pi_init){
	static uint16_t order;
	if(*pi_init){
		*pi_init = 0;
		order = 0;
	}
	uint8_t wr;
	if(order == 0){
		wr = ES8388_RSVD_MASK_DAC_CTRL3(0x00);
		if(i_Codec_Write(ES8388_REG_DAC_CTRL3, &wr, 1) != COMM_STAT_OK){return -1;}
		order++;
	}
	else{
		return 1;
	}
	return 0;
}































