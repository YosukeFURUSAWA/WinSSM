/**************************************************************************************************
	Title			: Configuration Reader
	Copyright		: Copyright (C) Yosuke FURUSAWA. 2004 - 2008
	Since			: 2004/09/15

	Function		: configuration
	Filename		: cfgread.h
	Last up date	: 2008/01/02
	Kanji-Code		: Shift JIS
	TAB Space		: 4
**************************************************************************************************/


#ifndef _CFGREAD_H_
#define _CFGREAD_H_


/*=================================================================================================
プロトタイプ宣言
=================================================================================================*/
int CFG_read(char *filename);
char* CFG_value(char *tag);
unsigned char CFG_check(void);
void CFG_printf(void);

#endif
