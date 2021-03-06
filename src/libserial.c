/**************************************************************************************************
	Title			: WinSSM (Subaru Select Monitor for Windows)
	Programer		: Yosuke FURUSAWA.
	Copyright		: Copyright (C) 2006 - 2008, Yosuke FURUSAWA.
	Since			: 2006/02/08

	Function		: �V���A���ʐM
	Filename		: libserial.c
	Last up date	: 2008/05/03
	Kanji-Code		: Shift JIS
	TAB Space		: 4
**************************************************************************************************/



/* SERIAL_read(), SERIAL_readline() �Ɋւ��郁�� ���܂�;;�E�́E)��
**
** �E���}�Ƀ^�C���A�E�g�@�\��t���邱�ƁI
*/



/*=================================================================================================
�w�b�_�t�@�C���̃C���N���[�h
=================================================================================================*/
#include <windows.h>
#include <commctrl.h>

#include "libserial.h"


/*=================================================================================================
�O���[�o���ϐ�
=================================================================================================*/
SERIAL serial = {
	1, 3, 1, 2, 0, 2, {1000, 100, 5000, 100, 5000},
};


/**************************************************************************************************
�V���A���|�[�g���J��
**************************************************************************************************/
BOOL SERIAL_open(pSERIAL pserial)
{
	/* DCB�ɐݒ肷�� */
	wsprintf(pserial->portname, "\\\\.\\COM%d", pserial->port);
	switch(pserial->baudrate){
	case  0:	pserial->dcb.BaudRate =    600;	break;
	case  1:	pserial->dcb.BaudRate =   1200;	break;
	case  2:	pserial->dcb.BaudRate =   2400;	break;
	case  3:	pserial->dcb.BaudRate =   4800;	break;
	case  4:	pserial->dcb.BaudRate =   9600;	break;
	case  5:	pserial->dcb.BaudRate =  14400;	break;
	case  6:	pserial->dcb.BaudRate =  19200;	break;
	case  7:	pserial->dcb.BaudRate =  38400;	break;
	case  8:	pserial->dcb.BaudRate =  57600;	break;
	case  9:	pserial->dcb.BaudRate = 115200;	break;
	case 10:	pserial->dcb.BaudRate = 230400;	break;
	case 11:	pserial->dcb.BaudRate = 460800;	break;
	case 12:	pserial->dcb.BaudRate = 921600;	break;
	}

	switch(pserial->parity){
	case  0:	pserial->dcb.fParity = TRUE;	pserial->dcb.Parity = EVENPARITY;	break;
	case  1:	pserial->dcb.fParity = TRUE;	pserial->dcb.Parity = ODDPARITY;	break;
	case  2:	pserial->dcb.fParity = FALSE;	pserial->dcb.Parity = NOPARITY;		break;
	}

	switch(pserial->flow){
	case  0:
		pserial->dcb.fOutX			= TRUE;
		pserial->dcb.fInX			= TRUE;
		pserial->dcb.XonLim			= SERIAL_XON_LIMIT_SIZE;
		pserial->dcb.XoffLim		= SERIAL_XOFF_LIMIT_SIZE;
		pserial->dcb.XonChar		= SERIAL_XON_CHAR;
		pserial->dcb.XoffChar		= SERIAL_XOFF_CHAR;
		pserial->dcb.fOutxCtsFlow	= FALSE;
		pserial->dcb.fRtsControl	= 0;
		break;

	case  1:
		pserial->dcb.fOutX			= FALSE;
		pserial->dcb.fInX			= FALSE;
		pserial->dcb.XonLim			= 0;
		pserial->dcb.XoffLim		= 0;
		pserial->dcb.XonChar		= 0;
		pserial->dcb.XoffChar		= 0;
		pserial->dcb.fOutxCtsFlow	= TRUE;
		pserial->dcb.fRtsControl	= RTS_CONTROL_HANDSHAKE;
		break;

	case  2:
		pserial->dcb.fOutX			= FALSE;
		pserial->dcb.fInX			= FALSE;
		pserial->dcb.XonLim			= 0;
		pserial->dcb.XoffLim		= 0;
		pserial->dcb.XonChar		= 0;
		pserial->dcb.XoffChar		= 0;
		pserial->dcb.fOutxCtsFlow	= FALSE;
		pserial->dcb.fRtsControl	= 0;
		break;
	}

	switch(pserial->data){
	case  0:	pserial->dcb.ByteSize = 7;	break;
	case  1:	pserial->dcb.ByteSize = 8;	break;
	}

	switch(pserial->stopbit){
	case  0:	pserial->dcb.StopBits = ONESTOPBIT;		break;
	case  1:	pserial->dcb.StopBits = TWOSTOPBITS;	break;
	}

	pserial->dcb.fBinary = TRUE;
	pserial->dcb.fDtrControl = DTR_CONTROL_ENABLE;

	pserial->timeout.ReadTotalTimeoutConstant = 500;
	pserial->timeout.ReadTotalTimeoutMultiplier = 10;
	pserial->timeout.ReadIntervalTimeout = 10;


	/* �|�[�g���J�� */
	if((pserial->hComm = CreateFile(
		pserial->portname,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		0,
		NULL)) == INVALID_HANDLE_VALUE) return(FALSE);

	/* �G���[�ƃo�b�t�@�N���A */
	PurgeComm(pserial->hComm, PURGE_TXCLEAR | PURGE_RXCLEAR);
	FlushFileBuffers(pserial->hComm);
	ClearCommError(pserial->hComm, NULL, NULL);

	/* �|�[�g�ݒ� */
	if(!SetCommState(pserial->hComm, &pserial->dcb)){
		CloseHandle(pserial->hComm);
		return(FALSE);
	}

	/* �^�C���A�E�g�ݒ� */
	if(!SetCommTimeouts(pserial->hComm, &pserial->timeout)){
		CloseHandle(pserial->hComm);
		return(FALSE);
	}

	/* �o�b�t�@�ݒ� */
	if(!SetupComm(pserial->hComm, SERIAL_IN_QUEUE_SIZE, SERIAL_IN_QUEUE_SIZE)){
		CloseHandle(pserial->hComm);
		return(FALSE);
	}

	return(TRUE);
}


/**************************************************************************************************
�V���A���|�[�g�����
**************************************************************************************************/
BOOL SERIAL_close(pSERIAL pserial)
{
	if(pserial == NULL || pserial->hComm == NULL)					return(FALSE);
	if(!PurgeComm(pserial->hComm, PURGE_TXCLEAR | PURGE_RXCLEAR))	return(FALSE);
	if(!FlushFileBuffers(pserial->hComm))							return(FALSE);
	if(!ClearCommError(pserial->hComm, NULL, NULL))					return(FALSE);

	return(CloseHandle(pserial->hComm));
}


/**************************************************************************************************
�o�b�t�@���t���b�V��
**************************************************************************************************/
BOOL SERIAL_flash(pSERIAL pserial)
{
	if(pserial == NULL || pserial->hComm == NULL)					return(FALSE);
	if(!PurgeComm(pserial->hComm, PURGE_TXCLEAR | PURGE_RXCLEAR))	return(FALSE);
	if(!FlushFileBuffers(pserial->hComm))							return(FALSE);
	if(!ClearCommError(pserial->hComm, NULL, NULL))					return(FALSE);

	return(TRUE);
}


/**************************************************************************************************
�o�b�t�@�ɂ���f�[�^�o�C�g��
**************************************************************************************************/
DWORD SERIAL_buffer(pSERIAL pserial)
{
	DWORD error;
	COMSTAT status;

	ClearCommError(pserial->hComm, &error, &status);

	return(status.cbInQue);
}


/**************************************************************************************************
�ǂݍ���
**************************************************************************************************/
DWORD SERIAL_read(pSERIAL pserial, char *buf, int length)
{
	DWORD size;

	if(!ReadFile(pserial->hComm, buf, length, &size, NULL)) return(-1);

	return(size);
}


/**************************************************************************************************
�P�s�����ǂݍ���
---------------------------------------------------------------------------------------------------
split		�I�[����
**************************************************************************************************/
DWORD SERIAL_readline(pSERIAL pserial, char *buf, int size, char split)
{
	int i;
	DWORD foo;

	i = 0;
	do {
		if(!ReadFile(pserial->hComm, &buf[i], 1, &foo, NULL)){
			break;
		}

		if(buf[i] == split){
			break;
		}
	} while(i++ < (size - 1));

	buf[i] = '\0';
	return(i);
}


/**************************************************************************************************
��������
**************************************************************************************************/
DWORD SERIAL_write(pSERIAL pserial, char *buf, int length)
{
	DWORD size;

	if(!WriteFile(pserial->hComm, buf, length, &size, NULL)) return(-1);

	return(size);
}
