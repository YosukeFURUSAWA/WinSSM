/**************************************************************************************************
	Title			: Configuration Reader
	Copyright		: Copyright (C) Yosuke FURUSAWA. 2004 - 2009
	Since			: 2004/09/14

	Function		: configuration
	Filename		: cfgread.c
	Last up date	: 2009/02/28
	Kanji-Code		: Shift JIS
	TAB Space		: 4
**************************************************************************************************/


/*=================================================================================================
�w�b�_�t�@�C���̃C���N���[�h
=================================================================================================*/
#include <windows.h>

#include <stdio.h>
#include <io.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys\types.h>
#include <sys\stat.h>

#include "cfgread.h"


/*=================================================================================================
�}�N����`
=================================================================================================*/
#define CFG_BUFFERS		256
#define CFG_COMMENT		';'
#define CFG_DEFINE		'='

#define CFG_VERSION		"1.1.2"


/*=================================================================================================
�\����
=================================================================================================*/
typedef struct {
	char *tag;
	char *value;
} Config;


/*=================================================================================================
�O���[�o���ϐ�
=================================================================================================*/
static Config gconfig[] = {
	{ "ECU_USE",							NULL,	},
	{ "ECU_PORT",							NULL,	},
	{ "ECU_LOG",							NULL,	},

	{ "MISSION_USE",						NULL,	},
	{ "MISSION_FONT",						NULL,	},
	{ "MISSION_X",							NULL,	},
	{ "MISSION_Y",							NULL,	},
	{ "MISSION_TIRE_WIDTH",					NULL,	},
	{ "MISSION_TIRE_FLAT",					NULL,	},
	{ "MISSION_TIRE_INCH",					NULL,	},
	{ "MISSION_FIRST",						NULL,	},
	{ "MISSION_SECOND",						NULL,	},
	{ "MISSION_THIRD",						NULL,	},
	{ "MISSION_FOURTH",						NULL,	},
	{ "MISSION_FIFTH",						NULL,	},
	{ "MISSION_SIXTH",						NULL,	},
	{ "MISSION_FINAL_GEAR_RATIO",			NULL,	},

	{ "METER_RPM_USE",						NULL,	},
	{ "METER_RPM_TYPE",						NULL,	},
	{ "METER_RPM_CIRCLE_SIZE",				NULL,	},
	{ "METER_RPM_CIRCLE_FONT",				NULL,	},
	{ "METER_RPM_CIRCLE_X",					NULL,	},
	{ "METER_RPM_CIRCLE_Y",					NULL,	},

	{ "METER_THROTTLE_USE",					NULL,	},
	{ "METER_THROTTLE_TYPE",				NULL,	},
	{ "METER_THROTTLE_CIRCLE_SIZE",			NULL,	},
	{ "METER_THROTTLE_CIRCLE_FONT",			NULL,	},
	{ "METER_THROTTLE_CIRCLE_X",			NULL,	},
	{ "METER_THROTTLE_CIRCLE_Y",			NULL,	},

	{ "METER_VEHICLE_USE",					NULL,	},
	{ "METER_VEHICLE_TYPE",					NULL,	},
	{ "METER_VEHICLE_CIRCLE_SIZE",			NULL,	},
	{ "METER_VEHICLE_CIRCLE_FONT",			NULL,	},
	{ "METER_VEHICLE_CIRCLE_X",				NULL,	},
	{ "METER_VEHICLE_CIRCLE_Y",				NULL,	},

	{ "METER_BOOST_USE",					NULL,	},
	{ "METER_BOOST_TYPE",					NULL,	},
	{ "METER_BOOST_CIRCLE_SIZE",			NULL,	},
	{ "METER_BOOST_CIRCLE_FONT",			NULL,	},
	{ "METER_BOOST_CIRCLE_X",				NULL,	},
	{ "METER_BOOST_CIRCLE_Y",				NULL,	},

	{ "METER_COOLANT_USE",					NULL,	},
	{ "METER_COOLANT_TYPE",					NULL,	},
	{ "METER_COOLANT_CIRCLE_SIZE",			NULL,	},
	{ "METER_COOLANT_CIRCLE_FONT",			NULL,	},
	{ "METER_COOLANT_CIRCLE_X",				NULL,	},
	{ "METER_COOLANT_CIRCLE_Y",				NULL,	},

	{ "METER_INTAKEAIR_USE",				NULL,	},
	{ "METER_INTAKEAIR_TYPE",				NULL,	},
	{ "METER_INTAKEAIR_CIRCLE_SIZE",		NULL,	},
	{ "METER_INTAKEAIR_CIRCLE_FONT",		NULL,	},
	{ "METER_INTAKEAIR_CIRCLE_X",			NULL,	},
	{ "METER_INTAKEAIR_CIRCLE_Y",			NULL,	},

	{ "METER_BATTERY_USE",					NULL,	},
	{ "METER_BATTERY_TYPE",					NULL,	},
	{ "METER_BATTERY_CIRCLE_SIZE",			NULL,	},
	{ "METER_BATTERY_CIRCLE_FONT",			NULL,	},
	{ "METER_BATTERY_CIRCLE_X",				NULL,	},
	{ "METER_BATTERY_CIRCLE_Y",				NULL,	},

	{ "METER_MAF_USE",						NULL,	},
	{ "METER_MAF_TYPE",						NULL,	},
	{ "METER_MAF_CIRCLE_SIZE",				NULL,	},
	{ "METER_MAF_CIRCLE_FONT",				NULL,	},
	{ "METER_MAF_CIRCLE_X",					NULL,	},
	{ "METER_MAF_CIRCLE_Y",					NULL,	},

	{ "METER_AFR_USE",						NULL,	},
	{ "METER_AFR_TYPE",						NULL,	},
	{ "METER_AFR_CIRCLE_SIZE",				NULL,	},
	{ "METER_AFR_CIRCLE_FONT",				NULL,	},
	{ "METER_AFR_CIRCLE_X",					NULL,	},
	{ "METER_AFR_CIRCLE_Y",					NULL,	},

	{ "METER_FUEL_USE",						NULL,	},
	{ "METER_FUEL_TYPE",					NULL,	},
	{ "METER_FUEL_CIRCLE_SIZE",				NULL,	},
	{ "METER_FUEL_CIRCLE_FONT",				NULL,	},
	{ "METER_FUEL_CIRCLE_X",				NULL,	},
	{ "METER_FUEL_CIRCLE_Y",				NULL,	},

	{ "METER_FONT_RED",						NULL,	},
	{ "METER_FONT_GREEN",					NULL,	},
	{ "METER_FONT_BLUE",					NULL,	},
	{ "METER_FONT_EDGE_RED",				NULL,	},
	{ "METER_FONT_EDGE_GREEN",				NULL,	},
	{ "METER_FONT_EDGE_BLUE",				NULL,	},
	{ "METER_CIRCLE_RED",					NULL,	},
	{ "METER_CIRCLE_GREEN",					NULL,	},
	{ "METER_CIRCLE_BLUE",					NULL,	},
	{ "METER_CIRCLE_EDGE_RED",				NULL,	},
	{ "METER_CIRCLE_EDGE_GREEN",			NULL,	},
	{ "METER_CIRCLE_EDGE_BLUE",				NULL,	},
	{ "METER_CIRCLE_MIN_RED",				NULL,	},
	{ "METER_CIRCLE_MIN_GREEN",				NULL,	},
	{ "METER_CIRCLE_MIN_BLUE",				NULL,	},
	{ "METER_CIRCLE_MAX_RED",				NULL,	},
	{ "METER_CIRCLE_MAX_GREEN",				NULL,	},
	{ "METER_CIRCLE_MAX_BLUE",				NULL,	},
	{ "METER_CIRCLE_VALUE_RED",				NULL,	},
	{ "METER_CIRCLE_VALUE_GREEN",			NULL,	},
	{ "METER_CIRCLE_VALUE_BLUE",			NULL,	},

	{ "LAPTIME_USE",						NULL,	},
	{ "LAPTIME_TOTAL_FONT",					NULL,	},
	{ "LAPTIME_TOTAL_X",					NULL,	},
	{ "LAPTIME_TOTAL_Y",					NULL,	},
	{ "LAPTIME_LAP_FONT",					NULL,	},
	{ "LAPTIME_LAP_X",						NULL,	},
	{ "LAPTIME_LAP_Y",						NULL,	},

	{ "SCREEN_SIZE_X",						NULL,	},
	{ "SCREEN_SIZE_Y",						NULL,	},

	{ "VIEWER_USE",							NULL,	},
	{ "VIEWER_FPS",							NULL,	},
	{ "VIEWER_SIZE_X",						NULL,	},
	{ "VIEWER_SIZE_Y",						NULL,	},

	{ "CAMERA_USE",							NULL,	},
	{ "CAMERA_0_DEVICE",					NULL,	},
	{ "CAMERA_0_SIZE_X",					NULL,	},
	{ "CAMERA_0_SIZE_Y",					NULL,	},
	{ "CAMERA_1_DEVICE",					NULL,	},
	{ "CAMERA_1_SIZE_X",					NULL,	},
	{ "CAMERA_1_SIZE_Y",					NULL,	},
	{ "CAMERA_2_DEVICE",					NULL,	},
	{ "CAMERA_2_SIZE_X",					NULL,	},
	{ "CAMERA_2_SIZE_Y",					NULL,	},
	{ "CAMERA_3_DEVICE",					NULL,	},
	{ "CAMERA_3_SIZE_X",					NULL,	},
	{ "CAMERA_3_SIZE_Y",					NULL,	},

	{ "RECORD_USE",							NULL,	},
	{ "RECORD_CODEC",						NULL,	},
	{ "RECORD_SIZE_X",						NULL,	},
	{ "RECORD_SIZE_Y",						NULL,	},
	{ "RECORD_FPS",							NULL,	},
	{ "RECORD_0_USE",						NULL,	},

	{ "MIC_USE",							NULL,	},
	{ "MIC_CODEC",							NULL,	},
	{ "MIC_SAMPLING",						NULL,	},
	{ "MIC_BITRATE",						NULL,	},
	{ "MIC_CHANNEL",						NULL,	},

	{ "DEBUG",								NULL,	},
	{ "VERSION",							NULL,	},

	{ NULL,									NULL,	},
};


/*=================================================================================================
�v���g�^�C�v�錾
=================================================================================================*/
static int CFG_readline(int fd, char *buf);
static int CFG_iscomment(const char *buf);
static int CFG_isdefine(const char *buf);
static int CFG_tagsearch(const char *buf);
static int CFG_delspace(char *buf);
static char* CFG_getvalue(char *buf);


/**************************************************************************************************
�R���t�B�O���[�V�����t�@�C���̓ǂݍ���
**************************************************************************************************/
int CFG_read(char *filename)
{
	int i, fd, tag, error;
	char buf[CFG_BUFFERS];

	/* �����`�F�b�N */
	if(filename == NULL){
		return(FALSE);
	}

	/* �t�@�C���I�[�v�� */
	if((fd = open(filename, O_RDONLY)) < 0){
		fprintf(stderr, "Can't open %s\n", filename);
		return(FALSE);
	}

	/* �t�@�C���ǂݍ��� */
	i = 0;
	while(CFG_readline(fd, buf)){
		i++;
		if(CFG_iscomment(buf)) continue;
		if(!CFG_isdefine(buf)) continue;

		/* ����`�̒�`�� */
		if((tag = CFG_tagsearch(buf)) < 0){
			fprintf(stderr, "Can't define : line %d\n%s\n", i, buf);
			return(FALSE);
		}

		/* ���d��` */
		if(gconfig[tag].value != NULL){
			fprintf(stderr, "Multiplex definition : line %d\n%s\n", i, buf);
			return(FALSE);
		}

		CFG_delspace(buf);
		gconfig[tag].value = CFG_getvalue(buf);
	}

	/* ����`�̒萔�����邩�H */
	i = 0;
	error = 0;
	while(gconfig[i].tag != NULL){
		if(gconfig[i].value == NULL){
			fprintf(stderr, "The value is not defined : %s\n", gconfig[i].tag);
			error++;
		}

		i++;
	}

	if(error != 0)	return(FALSE);
	return(TRUE);
}


/**************************************************************************************************
�^�O������f�[�^�𓾂�
**************************************************************************************************/
char* CFG_value(char *tag)
{
	int i;

	i = 0;
	while(gconfig[i].tag != NULL){
		if(strcmp(gconfig[i].tag, tag) == 0){
			return(gconfig[i].value);
		}

		i++;
	}

	printf("CFG_value() : Can't find %s\n", tag);
	exit(1);
	return(NULL);
}


/**************************************************************************************************
�ݒ�l���������͈͂Ɏ��܂��Ă��邩�A�`�F�b�N����
**************************************************************************************************/
unsigned char CFG_check(void)
{
	unsigned char error = TRUE;

	if(strcmp(CFG_value("VERSION"), CFG_VERSION) != 0){
		printf("Value error of configuration file : VERSION\n");
		error = FALSE;
	}

	return(error);
}


/**************************************************************************************************
�^�O������f�[�^�𓾂�
**************************************************************************************************/
void CFG_printf(void)
{
	int i;

	i = 0;
	while(gconfig[i].tag != NULL){
		printf("%s = %s\n", gconfig[i].tag, gconfig[i].value);
		i++;
	}

	return;
}


/*-------------------------------------------------------------------------------------------------
�f�B�X�N���v�^����P�s�ǂݍ���
-------------------------------------------------------------------------------------------------*/
static int CFG_readline(int fd, char *buf)
{
	char c, *temp;

	temp = buf;
	while(read(fd, &c, 1)){

		/* ���s���������o */
		if(c == '\r' || c == '\n'){
			*temp = '\0';
			return(TRUE);
		}

		*temp = c;
		temp++;
	}

	return(FALSE);
}


/*-------------------------------------------------------------------------------------------------
�R�����g�H
-------------------------------------------------------------------------------------------------*/
static int CFG_iscomment(const char *buf)
{
	if(strchr(buf, CFG_COMMENT) != NULL){
		return(TRUE);
	}

	return(FALSE);
}


/*-------------------------------------------------------------------------------------------------
��`�s�H
-------------------------------------------------------------------------------------------------*/
static int CFG_isdefine(const char *buf)
{
	if(strchr(buf, CFG_DEFINE) != NULL){
		return(TRUE);
	}

	return(FALSE);
}


/*-------------------------------------------------------------------------------------------------
�o�b�t�@����^�O�̏ꏊ��T��
-------------------------------------------------------------------------------------------------*/
static int CFG_tagsearch(const char *buf)
{
	int i;

	i = 0;
	while(gconfig[i].tag != NULL){
		if(!strncmp(buf, gconfig[i].tag, strlen(gconfig[i].tag))){
			return(i);
		}

		i++;
	}

	return(-1);

}


/*-------------------------------------------------------------------------------------------------
�X�y�[�X�ƃ^�u���폜����
-------------------------------------------------------------------------------------------------*/
static int CFG_delspace(char *buf)
{
	char *temp;

	temp = buf;
	while(*temp != '\0'){
		if(*temp == ' ' || *temp == '\t'){
			strcpy(temp, temp + 1);
		}
		temp++;
	}

	return(TRUE);
}


/*-------------------------------------------------------------------------------------------------
�o�b�t�@����l���������o��
-------------------------------------------------------------------------------------------------*/
static char* CFG_getvalue(char *buf)
{
	char *temp, *value;

	/* CFG_DEFINE�܂Ń|�C���^��i�߂� */
	temp = buf;
	while(*temp != '\0'){
		if(*temp == CFG_DEFINE) break;
		temp++;
	}

	/* CFG_DEFINE�̎��̕����ɐi�߂� */
	temp++;

	/* ���������m�� */
	if((value = (char *)malloc(sizeof(strlen(temp) + 1))) == NULL){
		perror("malloc(CFG_getvalue)");
		return(NULL);
	}

	strcpy(value, temp);

	return(value);
}
