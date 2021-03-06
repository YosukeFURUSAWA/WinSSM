/**************************************************************************************************
	Title			: CameraSSM
	Programer		: Yosuke FURUSAWA.
	Copyright		: Copyright (C) 2006 - 2007, Yosuke FURUSAWA.
	Since			: 2006/02/08

	Function		: �V���A���ʐM
	Filename		: libserial.h
	Last up date	: 2007/12/09
	Kanji-Code		: Shift JIS
	TAB Space		: 4
**************************************************************************************************/


#ifndef _LIBSERIAL_H_
#define _LIBSERIAL_H_


/*=================================================================================================
�}�N����`
=================================================================================================*/
#define SERIAL_IN_QUEUE_SIZE		8192		/* RX �o�b�t�@�T�C�Y */
#define SERIAL_OUT_QUEUE_SIZE		2048		/* TX �o�b�t�@�T�C�Y */
#define SERIAL_XON_LIMIT_SIZE		2048
#define SERIAL_XOFF_LIMIT_SIZE		2048
#define SERIAL_XON_CHAR				0x11
#define SERIAL_XOFF_CHAR			0x13


/*=================================================================================================
�\����
=================================================================================================*/
typedef struct {
	int port;
	int baudrate;
	int data;
	int parity;
	int stopbit;
	int flow;
	COMMTIMEOUTS timeout;
	char portname[6];
	DCB dcb;
	HANDLE hComm;
} SERIAL;
typedef SERIAL* pSERIAL;


/*=================================================================================================
�O���[�o���ϐ�
=================================================================================================*/
extern SERIAL serial;


/*=================================================================================================
�v���g�^�C�v�錾
=================================================================================================*/
BOOL SERIAL_open(pSERIAL pserial);
BOOL SERIAL_close(pSERIAL pserial);
BOOL SERIAL_flash(pSERIAL pserial);
DWORD SERIAL_buffer(pSERIAL pserial);
DWORD SERIAL_read(pSERIAL pserial, char *buf, int length);
DWORD SERIAL_readline(pSERIAL pserial, char *buf, int size, char split);
DWORD SERIAL_write(pSERIAL pserial, char *buf, int length);


#endif
