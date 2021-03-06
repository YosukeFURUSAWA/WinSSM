/**************************************************************************************************
	Title			: WinSSM (Subaru Select Monitor for Windows)
	Copyright		: Copyright (C) Yosuke FURUSAWA. 2008-2009
	Since			: 2008/01/01

	Function		: SSM Protocol
	Filename		: libssm.c
	Last up date	: 2009/02/27
	Kanji-Code		: Shift JIS
	TAB Space		: 4
**************************************************************************************************/


/*=================================================================================================
ヘッダファイルのインクルード
=================================================================================================*/
#include <windows.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

#include "libserial.h"
#include "libssm.h"
#include "cfgread.h"
#include "main.h"


/**************************************************************************************************
SSMデバイスを開く
**************************************************************************************************/
pSSM SSM_open(void)
{
	unsigned char i;
	unsigned char write_packet[128];
	unsigned char read_packet[128];
	pSSM ssm;

	for(i = 0; i < 128; i++){
		write_packet[i] = 0x00;
		read_packet[i]  = 0x00;
	}

	/* メモリ確保 */
	if( (ssm = malloc(sizeof(SSM))) == NULL ){
		perror("Can't allowcate of memory\n");
		return(NULL);
	}

	/* デバイスを開く */
	if(!SERIAL_open(&serial)){
		perror("Can't open device\n");
		free(ssm);
		return(NULL);
	}

	DEBUG2("SSM_open()\n");
	write_packet[0] = 0x80;
	write_packet[1] = 0x10;
	write_packet[2] = 0xf0;
	write_packet[3] = 0x01;
	write_packet[4] = 0xbf;
	write_packet[5] = 0x40;
	SERIAL_write(&serial, write_packet, 6);
	Sleep(1000);
	SERIAL_read(&serial, &read_packet, 128);
	for(i = 0; i < 128; i++) DEBUG3_("%x ", read_packet[i]);
	DEBUG3("\n");

	return(ssm);
}


/**************************************************************************************************
SSMデバイスを閉じる
**************************************************************************************************/
int SSM_close(pSSM ssm)
{
	SERIAL_close(&serial);
	free(ssm);
	return(TRUE);
}


/**************************************************************************************************
データを取得する
**************************************************************************************************/
int SSM_read(pSSM ssm, unsigned int address)
{
	unsigned char i;
	unsigned char write_packet[128];
	unsigned char read_packet[128];

	DEBUG2("SSM_read()\n");

	for(i = 0; i < 128; i++){
		write_packet[i] = 0x00;
		read_packet[i]  = 0x00;
	}

	/* 引数チェック */
	if(ssm == NULL){
		perror("Can't write SSM command\n");
		return(-1);
	}

	/* 読み込みコマンドの送信 */
	write_packet[0] = 0x80;
	write_packet[1] = 0x10;
	write_packet[2] = 0xf0;
	write_packet[3] = 0x05;
	write_packet[4] = 0xa8;
	write_packet[5] = 0x00;
	write_packet[6] = (address >> 16) & 0xff;
	write_packet[7] = (address >>  8) & 0xff;
	write_packet[8] = (address      ) & 0xff;
	write_packet[9] = 0x00;
	for(i = 0; i < 9; i++) write_packet[9] += write_packet[i];

	DEBUG3("Write Packet\n");
	for(i = 0; i < 10; i++) DEBUG3_("%x\n", write_packet[i]);

	SERIAL_write(&serial, write_packet, 10);


	/* ウェイト処理... 適当 (^^; */
	Sleep(145);

	/* ECU応答の受信 */
	SERIAL_read(&serial, &read_packet, 17);
	DEBUG3("Read Packet\n");
	for(i = 0; i < 17; i++) DEBUG3_("%x\n", read_packet[i]);
	return(read_packet[15]);

	if(read_packet[0] != 0x80) return(-1);				/* コマンドヘッダ */
	if(read_packet[1] != 0xf0) return(-1);				/* 通信方向 MSB */
	if(read_packet[2] != 0x10) return(-1);				/* 通信方向 LSB */
	if(read_packet[3] != 0x02) return(-1);				/* コマンド + データのサイズ */
	if(read_packet[4] != 0xe8) return(-1);				/* コマンド */

	return(read_packet[5]);
}


/**************************************************************************************************
WinSSM専用ブロック読み込み
**************************************************************************************************/
int SSM_block_read(pSSM ssm)
{
	unsigned char i;
	unsigned char write_packet[128];
	unsigned char read_packet[128];

	DEBUG2("SSM_block_read()\n");

	for(i = 0; i < 128; i++){
		write_packet[i] = 0x00;
		read_packet[i]  = 0x00;
	}

	/* 引数チェック */
	if(ssm == NULL){
		perror("Can't write SSM command\n");
		return(FALSE);
	}

	/* 読み込みコマンドの送信 */
	write_packet[ 0] = 0x80;
	write_packet[ 1] = 0x10;
	write_packet[ 2] = 0xf0;
	write_packet[ 3] = 0x26;
	write_packet[ 4] = 0xa8;
	write_packet[ 5] = 0x00;

	/* Engine Speed */
	write_packet[ 6] = 0x00;
	write_packet[ 7] = 0x00;
	write_packet[ 8] = 0x0e;
	write_packet[ 9] = 0x00;
	write_packet[10] = 0x00;
	write_packet[11] = 0x0f;

	/* Throttle */
	write_packet[12] = 0x00;
	write_packet[13] = 0x00;
	write_packet[14] = 0x15;

	/* Vehicle Speed */
	write_packet[15] = 0x00;
	write_packet[16] = 0x00;
	write_packet[17] = 0x10;

	/* Boost */
	write_packet[18] = 0x00;
	write_packet[19] = 0x00;
	write_packet[20] = 0x24;

	/* Coolant */
	write_packet[21] = 0x00;
	write_packet[22] = 0x00;
	write_packet[23] = 0x08;

	/* IntakeAir */
	write_packet[24] = 0x00;
	write_packet[25] = 0x00;
	write_packet[26] = 0x12;

	/* Battery */
	write_packet[27] = 0x00;
	write_packet[28] = 0x00;
	write_packet[29] = 0x1c;

	/* switch */
	write_packet[30] = 0x00;
	write_packet[31] = 0x00;
	write_packet[32] = 0x62;

	/* Mass Air Flow */
	write_packet[33] = 0x00;
	write_packet[34] = 0x00;
	write_packet[35] = 0x13;
	write_packet[36] = 0x00;
	write_packet[37] = 0x00;
	write_packet[38] = 0x14;

	/* A/F Sensor #1 */
	write_packet[39] = 0x00;
	write_packet[40] = 0x00;
	write_packet[41] = 0x46;

	write_packet[42] = 0x00;
	for(i = 0; i < 42; i++) write_packet[42] += write_packet[i];

	DEBUG3("Write Packet\n");
	for(i = 0; i < 43; i++) DEBUG3_("%02x ", write_packet[i]);
	DEBUG3("\n");

	SERIAL_write(&serial, write_packet, 43);


	/* ウェイト処理... 適当 (^^; */
	Sleep(200);

	/* ECU応答の受信 */
	SERIAL_read(&serial, &read_packet, 61);
	DEBUG3("Read Packet\n");
	for(i = 0; i < 61; i++) DEBUG3_("%02x ", read_packet[i]);
	DEBUG3("\n");

	/* パケットの確認 */
	for(i = 0; i < 43; i++){
		if(read_packet[i] != write_packet[i]) return(FALSE);
	}
	if(read_packet[43] != 0x80) return(FALSE);				/* コマンドヘッダ */
	if(read_packet[44] != 0xf0) return(FALSE);				/* 通信方向 MSB */
	if(read_packet[45] != 0x10) return(FALSE);				/* 通信方向 LSB */
	if(read_packet[46] != 0x0d) return(FALSE);				/* コマンド + データのサイズ */
	if(read_packet[47] != 0xe8) return(FALSE);				/* コマンド */

	ssm->engine_speed	= (double)((read_packet[48] << 8) + read_packet[49]) / 4.0;
	ssm->throttle		= (double)(read_packet[50] * 100) / 255.0;
	ssm->vehicle_speed	= (double)read_packet[51];
	ssm->gear			= SSM_calc_gear(ssm->vehicle_speed, ssm->engine_speed);
	ssm->boost			= (((double)read_packet[52] - 128.0) * 37.0) / 3570.0;
	ssm->coolant		= (double)read_packet[53] - 40.0;
	ssm->intakeair		= (double)read_packet[54] - 40.0;
	ssm->battery		= (double)read_packet[55] * 0.08;
	ssm->sw				= read_packet[56] & 0x80;
	ssm->maf			= (double)(((unsigned int)read_packet[57] << 8) + (unsigned int)read_packet[58]) / 100.0;
	ssm->afr			= ((double)read_packet[59] / 128.0) * 14.7;
	ssm->fuel			= ((double)ssm->vehicle_speed / 3600.0) / ((ssm->maf / ssm->afr) / 761.0);

	return(TRUE);

}


/**************************************************************************************************
ギアを推測する
**************************************************************************************************/
int SSM_calc_gear(double vehicle, double rpm)
{
	double tire_r;
	double gear[5];
	double ratio;
	double shift;

	/* 各ギア比の中間をもって、シフトの境目とする */
	gear[0] = atof(CFG_value("MISSION_FIRST"))		- (atof(CFG_value("MISSION_FIRST"))		- atof(CFG_value("MISSION_SECOND")))	/ 2.0;
	gear[1] = atof(CFG_value("MISSION_SECOND"))		- (atof(CFG_value("MISSION_SECOND"))	- atof(CFG_value("MISSION_THIRD")))		/ 2.0;
	gear[2] = atof(CFG_value("MISSION_THIRD"))		- (atof(CFG_value("MISSION_THIRD"))		- atof(CFG_value("MISSION_FOURTH")))	/ 2.0;
	gear[3] = atof(CFG_value("MISSION_FOURTH"))		- (atof(CFG_value("MISSION_FOURTH"))	- atof(CFG_value("MISSION_FIFTH")))		/ 2.0;
	gear[4] = atof(CFG_value("MISSION_FIFTH"))		- (atof(CFG_value("MISSION_FIFTH"))		- atof(CFG_value("MISSION_SIXTH")))		/ 2.0;

	/* タイヤの直径を求める */
	tire_r	= ( ((atof(CFG_value("MISSION_TIRE_WIDTH")) * atof(CFG_value("MISSION_TIRE_FLAT"))) / 50.0)
			+    (atof(CFG_value("MISSION_TIRE_INCH"))  * 25.4) ) * 3.1415926;

	/* タイヤサイズ、車速、エンジン回転数からギア比を求める */
	ratio = 0;
	if(rpm > 1000 && vehicle > 0){
		ratio  = (rpm / (vehicle * atof(CFG_value("MISSION_FINAL_GEAR_RATIO"))) * tire_r * 60.0);
		ratio /= 1000000.0;
	}

	/* シフトを求める */
	if(gear[0] <= ratio) shift = 1;
	if(gear[0] >  ratio) shift = 2;
	if(gear[1] >  ratio) shift = 3;
	if(gear[2] >  ratio) shift = 4;
	if(gear[3] >  ratio) shift = 5;
	if(gear[4] >  ratio) shift = 6;
	if(ratio == 0) shift = 0;

	return(shift);
}
