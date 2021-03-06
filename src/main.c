/**************************************************************************************************
	Title			: WinSSM (Subaru Select Monitor for Windows)
	Copyright		: Copyright (C) Yosuke FURUSAWA. 2008-2009
	Since			: 2008/01/01

	Function		: Main
	Filename		: main.c
	Last up date	: 2009/02/28
	Kanji-Code		: Shift JIS
	TAB Space		: 4
**************************************************************************************************/


/*=================================================================================================
ヘッダファイルのインクルード
=================================================================================================*/
#include <windows.h>
#include <process.h>

#include <stdio.h>
#include <math.h>
#include <time.h>

#include <cv.h>
#include <highgui.h>
#pragma comment(lib, "cv.lib")
#pragma comment(lib, "cvaux.lib")
#pragma comment(lib, "cvcam.lib")
#pragma comment(lib, "cvhaartraining.lib")
#pragma comment(lib, "highgui.lib")

#include "libserial.h"
#include "libssm.h"
#include "cfgread.h"
#include "main.h"


/*=================================================================================================
マクロ定義
=================================================================================================*/
#define VERSION			"WinSSM Version 1.1.2 RELEASE build 2009/02/28"
#define WINDOW_NAME		"WinSSM"

#define ECU_MIN		0
#define ECU_VALUE	1
#define ECU_MAX		2


/*=================================================================================================
構造体
=================================================================================================*/
typedef struct {
	double engine_speed[3];
	double vehicle_speed[3];
	double boost[3];
	double coolant[3];
	double intakeair[3];
	double throttle[3];
	unsigned char gear[3];
	double battery[3];
	double maf[3];
	double afr[3];
	double fuel[3];
} ECU;
typedef ECU* pECU;


typedef struct {
	double min;
	double value;
	double max;

	double limit_min;
	double limit_max;

	char label[32];
	unsigned int locate[2];
	unsigned int size;
	CvFont font, font_;
} METER;
typedef METER* pMETER;

typedef struct {
	unsigned char font_color[3];
	unsigned char font_edge_color[3];
	unsigned char circle_color[3];
	unsigned char circle_edge_color[3];
	unsigned char circle_value_color[3];
	unsigned char circle_min_color[3];
	unsigned char circle_max_color[3];
} DRAW_METER;
typedef DRAW_METER* pDRAW_METER;

typedef struct {
	unsigned char run;
	unsigned long total_start;
	unsigned long total_end;
	unsigned long laptime;
	unsigned long lap_start;
	unsigned long lap_end;
} LAPTIME;
typedef LAPTIME* pLAPTIME;


/*=================================================================================================
グローバル変数
=================================================================================================*/
pSSM ssm;
pECU ecu;
ECU ecu_[2];
LAPTIME laptime;
CvCapture *camera;
CvVideoWriter *record;

CvArr *img_camera, *img_screen, *img_viewer;
unsigned char thread_main, thread_ssm, thread_camera, thread_screen, thread_viewer, thread_record;

char sem_img_camera, sem_img_screen, sem_img_viewer;


/*=================================================================================================
プロトタイプ宣言
=================================================================================================*/
void THREAD_ssm(void *data);
void THREAD_camera(void *data);
void THREAD_screen(void *data);
void THREAD_viewer(void *data);
void THREAD_record(void *data);

void DRAW_meter(CvArr* img, pMETER meter, pDRAW_METER design);
void DRAW_param(CvArr* img, pMETER meter, pDRAW_METER design);
void DRAW_mission(CvArr* img, unsigned char gear, pDRAW_METER design);
void DRAW_laptime(CvArr* img, unsigned long total, unsigned long lap, pDRAW_METER design);

void ecu_init(void);


/**************************************************************************************************
スタートアップ関数
**************************************************************************************************/
int main(int argc, char *argv[])
{
	/* セマフォ */
	sem_img_camera = 0;
	sem_img_screen = 0;
	sem_img_viewer = 0;

	/* ラップタイム保持変数の初期化 */
	laptime.run = FALSE;
	laptime.total_start = 0;
	laptime.total_end = 0;
	laptime.laptime;
	laptime.lap_start = 0;
	laptime.lap_end = 0;


	/* 引数チェック */
	if(argc < 2){
		printf("usage : %s [Configuration File]\n", argv[0]);
		return(-1);
	}

	/* 設定ファイルを読み込む */
	if(!CFG_read(argv[1]))	return(-2);
	if(!CFG_check())		return(-3);

	/* 録画するときは、引数にファイル名を求める */
	if(atoi(CFG_value("RECORD_USE")) == 1){
		if(argc < 3){
			printf("usage : %s [Configuration File] [Video File]\n", argv[0]);
			return(-4);
		}
	}

	/* エラー通知の設定 */
	cvSetErrMode(CV_ErrModeSilent);

	/* ウィンドウ生成、キー入力のため必須 */
	cvNamedWindow(WINDOW_NAME, 1);
	cvResizeWindow(WINDOW_NAME, atoi(CFG_value("VIEWER_SIZE_X")), atoi(CFG_value("VIEWER_SIZE_Y")));

	/* スレッドの起動 */
	ssm = NULL;
	if(atoi(CFG_value("ECU_USE")) == 1){
		serial.port = atoi(CFG_value("ECU_PORT"));
		if((ssm = SSM_open()) == NULL){
			printf("Can't open SSM Device\n");
			return(-5);
		}
		SERIAL_flash(&serial);
		_beginthread(THREAD_ssm, 0, 0);
	} else {
		ecu_init();
	}

	if(atoi(CFG_value("CAMERA_USE")) >= 1){
		if( (camera = cvCreateCameraCapture( atoi(CFG_value("CAMERA_0_DEVICE")) )) == NULL){
			printf("Can't open CAMERA0 Device\n");
			return(-6);
		}

		_beginthread(THREAD_camera, 0 ,0);
	} else {
		img_camera = cvCreateImage(cvSize(atoi(CFG_value("CAMERA_0_SIZE_X")), atoi(CFG_value("CAMERA_0_SIZE_Y"))), IPL_DEPTH_8U, 3);
	}

	if(atoi(CFG_value("RECORD_USE")) == 1){
		if( (record = cvCreateVideoWriter(argv[2], -1, (1000.0 / atof(CFG_value("RECORD_FPS"))), cvSize(	atoi(CFG_value("RECORD_SIZE_X")), atoi(CFG_value("RECORD_SIZE_Y")) ), -1)) == NULL){
			printf("Can't open %s\n", argv[2]);
			return(-7);
		}

		_beginthread(THREAD_record, 0, 0);
	}

	if(atoi(CFG_value("VIEWER_USE")) == 1) thread_viewer = TRUE;
	_beginthread(THREAD_screen, 0, 0);


	/* キーイベントを見張る */
	thread_main = TRUE;
	while(thread_main){
		switch(cvWaitKey(1)){

		/* ビューア表示切り替え */
		case 'v':
		case 'V':
			if(!thread_viewer)	thread_viewer = TRUE;
			else				thread_viewer = FALSE;
			Sleep(30);
			break;

		/* Min/Maxのリセット */
		case 'r':
		case 'R':
			if(thread_ssm) ecu_init();
			break;

		/* ラップタイム */
		case 's':
		case 'S':
			if(!laptime.run){
				laptime.run = TRUE;
				laptime.total_start = GetTickCount();
				laptime.lap_start = GetTickCount();
				laptime.laptime = 0;
			} else {
				laptime.run = FALSE;
				laptime.total_end = GetTickCount();
				laptime.lap_end = GetTickCount();
			}
			break;

		case 'l':
		case 'L':
			if(laptime.run){
				laptime.lap_end = GetTickCount();
				laptime.laptime = laptime.lap_end - laptime.lap_start;
				laptime.lap_start = GetTickCount();
			}
			break;

		/* 終了 */
		case 'q':
		case 'Q':
			thread_main = FALSE;
			break;

		default:
			break;
		}

		if(laptime.run){
			laptime.total_end = GetTickCount();
			laptime.lap_end = GetTickCount();
		}

		Sleep(10);
	}


	/* 終了処理 */
	thread_viewer = FALSE;
	Sleep(100);
	thread_record = FALSE;
	Sleep(100);
	thread_screen = FALSE;
	Sleep(100);
	thread_camera = FALSE;
	Sleep(100);
	thread_ssm = FALSE;
	Sleep(100);
	if(ssm != NULL) SSM_close(ssm);
	if(atoi(CFG_value("RECORD_USE")) == 1) cvReleaseVideoWriter(&record);
	cvDestroyWindow(WINDOW_NAME);
	return(0);
}


/**************************************************************************************************
ECUからデータを取得するスレッド
**************************************************************************************************/
void THREAD_ssm(void *data)
{
	unsigned int loop;
	unsigned long fps;
	time_t long_time;
	struct tm *now_time;
	pECU now;

	DEBUG("Running THREAD_ssm()...\n");
	if(atoi(CFG_value("ECU_LOG")) > 0){
		printf(	"\"Date\","
				"\"Engine Speed(rpm)\","
				"\"Throttle (%%)\","
				"\"Vehicle Speed(km/h)\","
				"\"Gear Position\","
				"\"Boost(kg/cm2)\","
				"\"Coolant Temprature(C)\","
				"\"IntakeAir Temprature(C)\","
				"\"Mass Air Flow(g/s)\","
				"\"A/F Sensor(AFR)\","
				"\"Fuel(km/L)\","
				"\"Battery(V)\","
				"\n");
	}

	ecu_init();

	now = &ecu_[0];
	ecu = now;

	loop = 0;
	fps = GetTickCount();
	thread_ssm = TRUE;
	while(thread_ssm){
		if(ssm){
			if(SSM_block_read(ssm)){
				/* タコメータ */
				now->engine_speed[ ECU_VALUE ] = ssm->engine_speed;
				if(now->engine_speed[ ECU_VALUE ] < now->engine_speed[ ECU_MIN ]) now->engine_speed[ ECU_MIN ] = now->engine_speed[ ECU_VALUE ];
				if(now->engine_speed[ ECU_VALUE ] > now->engine_speed[ ECU_MAX ]) now->engine_speed[ ECU_MAX ] = now->engine_speed[ ECU_VALUE ];

				/* アクセル開度 */
				now->throttle[ ECU_VALUE ] = ssm->throttle;
				if(now->throttle[ ECU_VALUE ] < now->throttle[ ECU_MIN ]) now->throttle[ ECU_MIN ] = now->throttle[ ECU_VALUE ];
				if(now->throttle[ ECU_VALUE ] > now->throttle[ ECU_MAX ]) now->throttle[ ECU_MAX ] = now->throttle[ ECU_VALUE ];

				/* スピードメータ */
				now->vehicle_speed[ ECU_VALUE ] = ssm->vehicle_speed;
				if(now->vehicle_speed[ ECU_VALUE ] < now->vehicle_speed[ ECU_MIN ]) now->vehicle_speed[ ECU_MIN ] = now->vehicle_speed[ ECU_VALUE ];
				if(now->vehicle_speed[ ECU_VALUE ] > now->vehicle_speed[ ECU_MAX ]) now->vehicle_speed[ ECU_MAX ] = now->vehicle_speed[ ECU_VALUE ];

				/* ギア比 */
				now->gear[ ECU_VALUE ] = ssm->gear;

				/* ブーストメータ */
				now->boost[ ECU_VALUE ] = ssm->boost;
				if(now->boost[ ECU_VALUE ] < now->boost[ ECU_MIN ]) now->boost[ ECU_MIN ] = now->boost[ ECU_VALUE ];
				if(now->boost[ ECU_VALUE ] > now->boost[ ECU_MAX ]) now->boost[ ECU_MAX ] = now->boost[ ECU_VALUE ];

				/* 水温計 */
				now->coolant[ ECU_VALUE ] = ssm->coolant;
				if(now->coolant[ ECU_VALUE ] < now->coolant[ ECU_MIN ]) now->coolant[ ECU_MIN ] = now->coolant[ ECU_VALUE ];
				if(now->coolant[ ECU_VALUE ] > now->coolant[ ECU_MAX ]) now->coolant[ ECU_MAX ] = now->coolant[ ECU_VALUE ];

				/* 吸気温計 */
				now->intakeair[ ECU_VALUE ] = ssm->intakeair;
				if(now->intakeair[ ECU_VALUE ] < now->intakeair[ ECU_MIN ]) now->intakeair[ ECU_MIN ] = now->intakeair[ ECU_VALUE ];
				if(now->intakeair[ ECU_VALUE ] > now->intakeair[ ECU_MAX ]) now->intakeair[ ECU_MAX ] = now->intakeair[ ECU_VALUE ];

				/* バッテリー電圧 */
				now->battery[ ECU_VALUE ] = ssm->battery;
				if(now->battery[ ECU_VALUE ] < now->battery[ ECU_MIN ]) now->battery[ ECU_MIN ] = now->battery[ ECU_VALUE ];
				if(now->battery[ ECU_VALUE ] > now->battery[ ECU_MAX ]) now->battery[ ECU_MAX ] = now->battery[ ECU_VALUE ];

				/* Mass Air Flow */
				now->maf[ ECU_VALUE ] = ssm->maf;
				if(now->maf[ ECU_VALUE ] < now->maf[ ECU_MIN ]) now->maf[ ECU_MIN ] = now->maf[ ECU_VALUE ];
				if(now->maf[ ECU_VALUE ] > now->maf[ ECU_MAX ]) now->maf[ ECU_MAX ] = now->maf[ ECU_VALUE ];

				/* A/F Sensor */
				now->afr[ ECU_VALUE ] = ssm->afr;
				if(now->afr[ ECU_VALUE ] < now->afr[ ECU_MIN ]) now->afr[ ECU_MIN ] = now->afr[ ECU_VALUE ];
				if(now->afr[ ECU_VALUE ] > now->afr[ ECU_MAX ]) now->afr[ ECU_MAX ] = now->afr[ ECU_VALUE ];

				/* 燃費 */
				now->fuel[ ECU_VALUE ] = ssm->fuel;
				if(now->fuel[ ECU_VALUE ] < now->fuel[ ECU_MIN ]) now->fuel[ ECU_MIN ] = now->fuel[ ECU_VALUE ];
				if(now->fuel[ ECU_VALUE ] > now->fuel[ ECU_MAX ]) now->fuel[ ECU_MAX ] = now->fuel[ ECU_VALUE ];

			} else {
				SERIAL_flash(&serial);
			}
		}

/*
		else {
			now->engine_speed[ ECU_VALUE ]	= 9000.0 * (sin((double)(loop * 3.1415)) / 360.0);
			now->vehicle_speed[ ECU_VALUE ]	=  255.0 * (sin((double)(loop * 3.1415)) / 180.0);
			now->boost[ ECU_VALUE ]			=          (cos((double)(loop * 3.1415)) / 180.0);
			now->coolant[ ECU_VALUE ]		=  100.0 * (sin((double)(loop * 3.1415)) / 180.0);
			now->intakeair[ ECU_VALUE ]		=  100.0 * (sin((double)(loop * 3.1415)) / 180.0);
		}

		DEBUG2_("Engine Speed : %frpm\n", now->engine_speed[ ECU_VALUE ]);
		DEBUG2_("Vehicle Speed : %fkm/h\n", now->vehicle_speed[ ECU_VALUE ]);
		DEBUG2_("Boost : %fkg/cm2\n", now->boost[ ECU_VALUE ]);
		DEBUG2_("Coolant Temprature : %fC\n", now->coolant[ ECU_VALUE ]);
		DEBUG2_("Intakeair Temprature : %fC\n", now->intakeair[ ECU_VALUE ]);
*/
		if(atoi(CFG_value("ECU_LOG")) > 0){
			time(&long_time);
			now_time = localtime(&long_time);
			printf("\"%04d/%02d/%02d %02d:%02d:%02d.%03d\",",
					now_time->tm_year + 1900,
					now_time->tm_mon + 1,
					now_time->tm_mday,
					now_time->tm_hour,
					now_time->tm_min,
					now_time->tm_sec,
					GetTickCount() % 1000);
			printf("\"%f\",", now->engine_speed[ ECU_VALUE ]);
			printf("\"%f\",", now->throttle[ ECU_VALUE ]);
			printf("\"%f\",", now->vehicle_speed[ ECU_VALUE ]);
			printf("\"%d\",", now->gear[ ECU_VALUE ]);
			printf("\"%f\",", now->boost[ ECU_VALUE ]);
			printf("\"%f\",", now->coolant[ ECU_VALUE ]);
			printf("\"%f\",", now->intakeair[ ECU_VALUE ]);
			printf("\"%f\",", now->maf[ ECU_VALUE ]);
			printf("\"%f\",", now->afr[ ECU_VALUE ]);
			printf("\"%f\",", now->fuel[ ECU_VALUE ]);
			printf("\"%f\",", now->battery[ ECU_VALUE]);
			printf("\n");
		}

		loop++;
		Sleep(1);

		DEBUG_("THREAD_ssm() : %dms\n", GetTickCount() - fps);
		fps = GetTickCount();
	}

	_endthread();
	thread_ssm = FALSE;
	return;
}


/**************************************************************************************************
ビデオキャプチャをするスレッド
**************************************************************************************************/
void THREAD_camera(void *data)
{
	unsigned char loop;
	unsigned long fps;
	IplImage *frame[2];

	DEBUG("Running THREAD_camera()...\n");


	cvSetCaptureProperty(camera, CV_CAP_PROP_FRAME_WIDTH,  atof(CFG_value("CAMERA_0_SIZE_X")));
	cvSetCaptureProperty(camera, CV_CAP_PROP_FRAME_HEIGHT, atof(CFG_value("CAMERA_0_SIZE_Y")));
	cvSetCaptureProperty(camera, CV_CAP_PROP_FPS, 30.0);

	loop = 0;
	fps = GetTickCount();
	thread_camera = TRUE;
	while(thread_camera){
		if(camera != NULL){
			/* ビデオキャプチャ */
			if( !(frame[loop % 2] = cvQueryFrame(camera)) ){
				thread_camera = 0;
				thread_main = 0;
				_endthread();
				return;
			}

			/* ダブルバッファの切り替え */
			while(sem_img_camera > 0) Sleep(1);
			img_camera = frame[loop % 2];
			loop++;

			Sleep(1);
			DEBUG_("THREAD_camera() : %dms\n", GetTickCount() - fps);
			fps = GetTickCount();
		}
	}

	_endthread();
	thread_camera = FALSE;
	return;
}


/**************************************************************************************************
画面を作るスレッド
**************************************************************************************************/
void THREAD_screen(void *data)
{
	unsigned char loop;
	CvArr *img, *frame[2];
	char buf[256];
	unsigned long fps;
	time_t long_time;
	struct tm *now_time;
	CvFont font_time,  font_time_;
	METER engine_speed, throttle, vehicle_speed, boost, coolant, intakeair, battery, maf, afr, fuel;
	DRAW_METER design;

	DEBUG("Running THREAD_screen()...\n");

	/* フォント初期化 */
	cvInitFont(&font_time,			CV_FONT_HERSHEY_DUPLEX, 0.4 * atof(CFG_value("SCREEN_SIZE_X")) / 640.0,  0.4 * atof(CFG_value("SCREEN_SIZE_Y")) / 480.0,  0, 1, CV_AA);
	cvInitFont(&font_time_,			CV_FONT_HERSHEY_DUPLEX, 0.4 * atof(CFG_value("SCREEN_SIZE_X")) / 640.0,  0.4 * atof(CFG_value("SCREEN_SIZE_Y")) / 480.0,  0, 2, CV_AA);

	/* メータデザイン */
	design.font_color[0]				= atoi(CFG_value("METER_FONT_RED"));
	design.font_color[1]				= atoi(CFG_value("METER_FONT_GREEN"));
	design.font_color[2]				= atoi(CFG_value("METER_FONT_BLUE"));
	design.font_edge_color[0]			= atoi(CFG_value("METER_FONT_EDGE_RED"));
	design.font_edge_color[1]			= atoi(CFG_value("METER_FONT_EDGE_GREEN"));
	design.font_edge_color[2]			= atoi(CFG_value("METER_FONT_EDGE_BLUE"));
	design.circle_color[0]				= atoi(CFG_value("METER_CIRCLE_RED"));
	design.circle_color[1]				= atoi(CFG_value("METER_CIRCLE_GREEN"));
	design.circle_color[2]				= atoi(CFG_value("METER_CIRCLE_BLUE"));
	design.circle_edge_color[0]			= atoi(CFG_value("METER_CIRCLE_EDGE_RED"));
	design.circle_edge_color[1]			= atoi(CFG_value("METER_CIRCLE_EDGE_GREEN"));
	design.circle_edge_color[2]			= atoi(CFG_value("METER_CIRCLE_EDGE_BLUE"));
	design.circle_value_color[0]		= atoi(CFG_value("METER_CIRCLE_VALUE_RED"));
	design.circle_value_color[1]		= atoi(CFG_value("METER_CIRCLE_VALUE_GREEN"));
	design.circle_value_color[2]		= atoi(CFG_value("METER_CIRCLE_VALUE_BLUE"));
	design.circle_min_color[0]			= atoi(CFG_value("METER_CIRCLE_MIN_RED"));
	design.circle_min_color[1]			= atoi(CFG_value("METER_CIRCLE_MIN_GREEN"));
	design.circle_min_color[2]			= atoi(CFG_value("METER_CIRCLE_MIN_BLUE"));
	design.circle_max_color[0]			= atoi(CFG_value("METER_CIRCLE_MAX_RED"));
	design.circle_max_color[1]			= atoi(CFG_value("METER_CIRCLE_MAX_GREEN"));
	design.circle_max_color[2]			= atoi(CFG_value("METER_CIRCLE_MAX_BLUE"));

	/* メータ初期設定 */
	sprintf(engine_speed.label, "rpm");
	engine_speed.limit_min = 0;
	engine_speed.limit_max = 10000;
	engine_speed.locate[0] = atoi(CFG_value("METER_RPM_CIRCLE_X"));
	engine_speed.locate[1] = atoi(CFG_value("METER_RPM_CIRCLE_Y"));
	engine_speed.size      = atoi(CFG_value("METER_RPM_CIRCLE_SIZE"));
	cvInitFont(&engine_speed.font,	CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_RPM_CIRCLE_FONT")), atof(CFG_value("METER_RPM_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&engine_speed.font_,	CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_RPM_CIRCLE_FONT")), atof(CFG_value("METER_RPM_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(throttle.label, "Throttle");
	throttle.limit_min = 0;
	throttle.limit_max = 100;
	throttle.locate[0] = atoi(CFG_value("METER_THROTTLE_CIRCLE_X"));
	throttle.locate[1] = atoi(CFG_value("METER_THROTTLE_CIRCLE_Y"));
	throttle.size      = atoi(CFG_value("METER_THROTTLE_CIRCLE_SIZE"));
	cvInitFont(&throttle.font,	CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_THROTTLE_CIRCLE_FONT")), atof(CFG_value("METER_THROTTLE_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&throttle.font_,	CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_THROTTLE_CIRCLE_FONT")), atof(CFG_value("METER_THROTTLE_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(vehicle_speed.label, "km/h");
	vehicle_speed.limit_min = 0;
	vehicle_speed.limit_max = 255;
	vehicle_speed.locate[0] = atoi(CFG_value("METER_VEHICLE_CIRCLE_X"));
	vehicle_speed.locate[1] = atoi(CFG_value("METER_VEHICLE_CIRCLE_Y"));
	vehicle_speed.size		= atoi(CFG_value("METER_VEHICLE_CIRCLE_SIZE"));
	cvInitFont(&vehicle_speed.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_VEHICLE_CIRCLE_FONT")), atof(CFG_value("METER_VEHICLE_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&vehicle_speed.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_VEHICLE_CIRCLE_FONT")), atof(CFG_value("METER_VEHICLE_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(boost.label, "kg/cm2");
	boost.limit_min = -1.0;
	boost.limit_max =  1.5;
	boost.locate[0] = atoi(CFG_value("METER_BOOST_CIRCLE_X"));
	boost.locate[1] = atoi(CFG_value("METER_BOOST_CIRCLE_Y"));
	boost.size      = atoi(CFG_value("METER_BOOST_CIRCLE_SIZE"));
	cvInitFont(&boost.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_BOOST_CIRCLE_FONT")), atof(CFG_value("METER_BOOST_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&boost.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_BOOST_CIRCLE_FONT")), atof(CFG_value("METER_BOOST_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(coolant.label, "Coolant");
	coolant.limit_min = 0;
	coolant.limit_max = 150;
	coolant.locate[0] = atoi(CFG_value("METER_COOLANT_CIRCLE_X"));
	coolant.locate[1] = atoi(CFG_value("METER_COOLANT_CIRCLE_Y"));
	coolant.size      = atoi(CFG_value("METER_COOLANT_CIRCLE_SIZE"));
	cvInitFont(&coolant.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_COOLANT_CIRCLE_FONT")), atof(CFG_value("METER_COOLANT_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&coolant.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_COOLANT_CIRCLE_FONT")), atof(CFG_value("METER_COOLANT_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(intakeair.label, "IntakeAir");
	intakeair.limit_min = 0;
	intakeair.limit_max = 80;
	intakeair.locate[0] = atoi(CFG_value("METER_INTAKEAIR_CIRCLE_X"));
	intakeair.locate[1] = atoi(CFG_value("METER_INTAKEAIR_CIRCLE_Y"));
	intakeair.size      = atoi(CFG_value("METER_INTAKEAIR_CIRCLE_SIZE"));
	cvInitFont(&intakeair.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_INTAKEAIR_CIRCLE_FONT")), atof(CFG_value("METER_INTAKEAIR_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&intakeair.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_INTAKEAIR_CIRCLE_FONT")), atof(CFG_value("METER_INTAKEAIR_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(battery.label, "Battery");
	battery.limit_min = 10;
	battery.limit_max = 16;
	battery.locate[0] = atoi(CFG_value("METER_BATTERY_CIRCLE_X"));
	battery.locate[1] = atoi(CFG_value("METER_BATTERY_CIRCLE_Y"));
	battery.size      = atoi(CFG_value("METER_BATTERY_CIRCLE_SIZE"));
	cvInitFont(&battery.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_BATTERY_CIRCLE_FONT")), atof(CFG_value("METER_BATTERY_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&battery.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_BATTERY_CIRCLE_FONT")), atof(CFG_value("METER_BATTERY_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(maf.label, "MAF");
	maf.limit_min = 0;
	maf.limit_max = 660;
	maf.locate[0] = atoi(CFG_value("METER_MAF_CIRCLE_X"));
	maf.locate[1] = atoi(CFG_value("METER_MAF_CIRCLE_Y"));
	maf.size      = atoi(CFG_value("METER_MAF_CIRCLE_SIZE"));
	cvInitFont(&maf.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_MAF_CIRCLE_FONT")), atof(CFG_value("METER_MAF_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&maf.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_MAF_CIRCLE_FONT")), atof(CFG_value("METER_MAF_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(afr.label, "AFR");
	afr.limit_min = 0;
	afr.limit_max = 30;
	afr.locate[0] = atoi(CFG_value("METER_AFR_CIRCLE_X"));
	afr.locate[1] = atoi(CFG_value("METER_AFR_CIRCLE_Y"));
	afr.size      = atoi(CFG_value("METER_AFR_CIRCLE_SIZE"));
	cvInitFont(&afr.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_AFR_CIRCLE_FONT")), atof(CFG_value("METER_AFR_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&afr.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_AFR_CIRCLE_FONT")), atof(CFG_value("METER_AFR_CIRCLE_FONT")), 0, 2, CV_AA);

	sprintf(fuel.label, "FUEL");
	fuel.limit_min = 0;
	fuel.limit_max = 30;
	fuel.locate[0] = atoi(CFG_value("METER_FUEL_CIRCLE_X"));
	fuel.locate[1] = atoi(CFG_value("METER_FUEL_CIRCLE_Y"));
	fuel.size      = atoi(CFG_value("METER_FUEL_CIRCLE_SIZE"));
	cvInitFont(&fuel.font,  CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_FUEL_CIRCLE_FONT")), atof(CFG_value("METER_FUEL_CIRCLE_FONT")), 0, 1, CV_AA);
	cvInitFont(&fuel.font_, CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("METER_FUEL_CIRCLE_FONT")), atof(CFG_value("METER_FUEL_CIRCLE_FONT")), 0, 2, CV_AA);

	/* スクリーンバッファの初期化 */
	img_screen = NULL;
	frame[0] = cvCreateImage(cvSize(atoi(CFG_value("SCREEN_SIZE_X")), atoi(CFG_value("SCREEN_SIZE_Y"))), IPL_DEPTH_8U, 3);
	frame[1] = cvCreateImage(cvSize(atoi(CFG_value("SCREEN_SIZE_X")), atoi(CFG_value("SCREEN_SIZE_Y"))), IPL_DEPTH_8U, 3);
	img = frame[0];

	img_viewer = cvCreateImage(cvSize(atoi(CFG_value("VIEWER_SIZE_X")), atoi(CFG_value("VIEWER_SIZE_Y"))), IPL_DEPTH_8U, 3);


	loop = 0;
	fps = GetTickCount();
	thread_screen = TRUE;
	while(thread_screen){
		/* 画像をスクリーンサイズに合わせる */
		if(img_camera){
			sem_img_camera++;
			cvResize(img_camera, img, CV_INTER_LINEAR);
			sem_img_camera--;
		}

		/* ECU情報の表示 */
		if(ecu){
			if(atoi(CFG_value("METER_RPM_USE")) > 0){
				engine_speed.min    = ecu->engine_speed[ ECU_MIN ];
				engine_speed.max    = ecu->engine_speed[ ECU_MAX ];
				engine_speed.value  = ecu->engine_speed[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_RPM_TYPE"))){
				case 0:		DRAW_param(img, &engine_speed, &design);	break;
				default:	DRAW_meter(img, &engine_speed, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_THROTTLE_USE")) > 0){
				throttle.min    = ecu->throttle[ ECU_MIN ];
				throttle.max    = ecu->throttle[ ECU_MAX ];
				throttle.value  = ecu->throttle[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_THROTTLE_TYPE"))){
				case 0:		DRAW_param(img, &throttle, &design);	break;
				default:	DRAW_meter(img, &throttle, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_VEHICLE_USE")) > 0){
				vehicle_speed.min   = ecu->vehicle_speed[ ECU_MIN ];
				vehicle_speed.max   = ecu->vehicle_speed[ ECU_MAX ];
				vehicle_speed.value = ecu->vehicle_speed[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_VEHICLE_TYPE"))){
				case 0:		DRAW_param(img, &vehicle_speed, &design);	break;
				default:	DRAW_meter(img, &vehicle_speed, &design);	break;
				}
			}

			if(atoi(CFG_value("MISSION_USE")) > 0){
				DRAW_mission(img, ecu->gear[ ECU_VALUE ], &design);
			}

			if(atoi(CFG_value("METER_BOOST_USE")) > 0){
				boost.min   = ecu->boost[ ECU_MIN ];
				boost.max   = ecu->boost[ ECU_MAX ];
				boost.value = ecu->boost[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_BOOST_TYPE"))){
				case 0:		DRAW_param(img, &boost, &design);	break;
				default:	DRAW_meter(img, &boost, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_COOLANT_USE")) > 0){
				coolant.min   = ecu->coolant[ ECU_MIN ];
				coolant.max   = ecu->coolant[ ECU_MAX ];
				coolant.value = ecu->coolant[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_COOLANT_TYPE"))){
				case 0:		DRAW_param(img, &coolant, &design);	break;
				default:	DRAW_meter(img, &coolant, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_INTAKEAIR_USE")) > 0){
				intakeair.min   = ecu->intakeair[ ECU_MIN ];
				intakeair.max   = ecu->intakeair[ ECU_MAX ];
				intakeair.value = ecu->intakeair[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_INTAKEAIR_TYPE"))){
				case 0:		DRAW_param(img, &intakeair, &design);	break;
				default:	DRAW_meter(img, &intakeair, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_BATTERY_USE")) > 0){
				battery.min   = ecu->battery[ ECU_MIN ];
				battery.max   = ecu->battery[ ECU_MAX ];
				battery.value = ecu->battery[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_BATTERY_TYPE"))){
				case 0:		DRAW_param(img, &battery, &design);	break;
				default:	DRAW_meter(img, &battery, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_MAF_USE")) > 0){
				maf.min   = ecu->maf[ ECU_MIN ];
				maf.max   = ecu->maf[ ECU_MAX ];
				maf.value = ecu->maf[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_MAF_TYPE"))){
				case 0:		DRAW_param(img, &maf, &design);	break;
				default:	DRAW_meter(img, &maf, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_AFR_USE")) > 0){
				afr.min   = ecu->afr[ ECU_MIN ];
				afr.max   = ecu->afr[ ECU_MAX ];
				afr.value = ecu->afr[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_AFR_TYPE"))){
				case 0:		DRAW_param(img, &afr, &design);	break;
				default:	DRAW_meter(img, &afr, &design);	break;
				}
			}

			if(atoi(CFG_value("METER_FUEL_USE")) > 0){
				fuel.min   = ecu->fuel[ ECU_MIN ];
				fuel.max   = ecu->fuel[ ECU_MAX ];
				fuel.value = ecu->fuel[ ECU_VALUE ];
				switch(atoi(CFG_value("METER_FUEL_TYPE"))){
				case 0:		DRAW_param(img, &fuel, &design);	break;
				default:	DRAW_meter(img, &fuel, &design);	break;
				}
			}
		}

		/* ラップタイム表示 */
		if(atoi(CFG_value("LAPTIME_USE")) > 0){
			DRAW_laptime(img, laptime.total_end - laptime.total_start, laptime.laptime, &design);
		}

		/* タイトルと時計の表示 */
		time(&long_time);
		now_time = localtime(&long_time);
		sprintf(buf, "%s : Date %04d/%02d/%02d %02d:%02d:%02d",
				VERSION,
				now_time->tm_year + 1900,
				now_time->tm_mon + 1,
				now_time->tm_mday,
				now_time->tm_hour,
				now_time->tm_min,
				now_time->tm_sec);
		cvPutText(img, buf, cvPoint(3, atoi(CFG_value("SCREEN_SIZE_Y")) - 5), &font_time_, CV_RGB(atoi(CFG_value("METER_FONT_EDGE_RED")), atoi(CFG_value("METER_FONT_EDGE_GREEN")), atoi(CFG_value("METER_FONT_EDGE_BLUE"))));
		cvPutText(img, buf, cvPoint(3, atoi(CFG_value("SCREEN_SIZE_Y")) - 5), &font_time,  CV_RGB(atoi(CFG_value("METER_FONT_RED")),      atoi(CFG_value("METER_FONT_GREEN")),      atoi(CFG_value("METER_FONT_BLUE"))));

		/* ダブルバッファの切り替え */
		while(sem_img_screen > 0) Sleep(1);
		img_screen = img;
		loop++;
		img = frame[loop % 2];

		/* 表示 */
		if(thread_viewer){
			sem_img_screen++;
			cvResize(img_screen, img_viewer, CV_INTER_LINEAR);
			sem_img_screen--;
			cvShowImage(WINDOW_NAME, img_viewer);
		}

		/* FPS計算 */
		while((GetTickCount() - fps) < atol(CFG_value("VIEWER_FPS"))) Sleep(1);
		DEBUG_("THREAD_screen() : %dms\n", GetTickCount() - fps);
		fps = GetTickCount();
	}

	_endthread();
	thread_screen = FALSE;
	return;
}


/**************************************************************************************************
録画(エンコード)をするスレッド
**************************************************************************************************/
void THREAD_record(void *data)
{
	CvArr *img;
	unsigned long fps;

	DEBUG("Running THREAD_record()...\n");

	img = cvCreateImage(cvSize(atoi(CFG_value("RECORD_SIZE_X")), atoi(CFG_value("RECORD_SIZE_Y"))), IPL_DEPTH_8U, 3);

	while(!img_screen) Sleep(1);

	thread_record = TRUE;
	fps = GetTickCount();
	while(thread_record){
		if(record != NULL){
			while(!img_screen) Sleep(0);
			sem_img_screen++;
			cvResize(img_screen, img, CV_INTER_LINEAR);
			sem_img_screen--;
			cvWriteFrame(record, img);
		}

		/* FPS計算 */
		while((GetTickCount() - fps) < atol(CFG_value("RECORD_FPS"))) Sleep(0);
		DEBUG_("THREAD_record() : %dms\n", GetTickCount() - fps);
		fps = GetTickCount();
	}

	_endthread();
	thread_record = FALSE;
	return;
}


/**************************************************************************************************
メータを描画する
**************************************************************************************************/
void DRAW_meter(CvArr* img, pMETER meter, pDRAW_METER design)
{
	char buf[128], i;
	double theta[3];
	double degree;
	double value[3], value_;
	int x, y;

	x = meter->locate[0];
	y = meter->locate[1];

	value[0] = meter->value;
	value[1] = meter->min;
	value[2] = meter->max;

	for(i = 0; i < 3; i++){

		/* 最大値と最小値の範囲に入れる */
		value_ = value[i];
		if(value_ > meter->limit_max) value_ = meter->limit_max;
		if(value_ < meter->limit_min) value_ = meter->limit_min;

		/* 0 ～ 270度の範囲に変換 */
		if(meter->limit_max >= 0 && meter->limit_min >= 0){
			degree = (value_ * 270.0) / (meter->limit_max - meter->limit_min);
			if		(meter->limit_min == 0)	{ degree +=  90; }
			else if	(meter->limit_min  > 0)	{ degree += 100; }
		} else if(meter->limit_max >= 0 && meter->limit_min < 0){
			degree = ((value_ - meter->limit_min) * 270.0) / (meter->limit_max - meter->limit_min);
			degree += 90;
		} else if(meter->limit_max  < 0 && meter->limit_min < 0){
			degree = ( ((-1) * (value_ - meter->limit_max)) * 270.0) / ((-1) * (meter->limit_min - meter->limit_max));
			degree += 90;
		}

		/* 度 -> ラジアン */
		theta[i] = (double)((degree) * 2.0 * 3.1415) / 360.0;
	}


	/* メータを描く */
	cvCircle(img, cvPoint(x, y), meter->size, CV_RGB(design->circle_edge_color[0],design->circle_edge_color[1],design->circle_edge_color[2]), 2, CV_AA, 0);
	cvLine(img,   cvPoint((int)(x + meter->size * cos(0)),          (int)(y + meter->size * sin(0))),			cvPoint((int)(x + (meter->size * 0.7) * cos(0)),          (int)(y + (meter->size * 0.7) * sin(0))),          CV_RGB(design->circle_edge_color[0],design->circle_edge_color[1],design->circle_edge_color[2]),  2, CV_AA, 0);
	cvLine(img,   cvPoint((int)(x + meter->size * cos(3.1415 / 2)), (int)(y + meter->size * sin(3.1415 / 2))),	cvPoint((int)(x + (meter->size * 0.7) * cos(3.1415 / 2)), (int)(y + (meter->size * 0.7) * sin(3.1415 / 2))), CV_RGB(design->circle_edge_color[0],design->circle_edge_color[1],design->circle_edge_color[2]),  2, CV_AA, 0);

	cvCircle(img, cvPoint(x, y), meter->size, CV_RGB(design->circle_color[0],design->circle_color[1],design->circle_color[2]), 1, CV_AA, 0);
	cvLine(img,   cvPoint((int)(x + meter->size * cos(0)),          (int)(y + meter->size * sin(0))),			cvPoint((int)(x + (meter->size * 0.7) * cos(0)),          (int)(y + (meter->size * 0.7) * sin(0))),          CV_RGB(design->circle_color[0],design->circle_color[1],design->circle_color[2]),  1, CV_AA, 0);
	cvLine(img,   cvPoint((int)(x + meter->size * cos(3.1415 / 2)), (int)(y + meter->size * sin(3.1415 / 2))),	cvPoint((int)(x + (meter->size * 0.7) * cos(3.1415 / 2)), (int)(y + (meter->size * 0.7) * sin(3.1415 / 2))), CV_RGB(design->circle_color[0],design->circle_color[1],design->circle_color[2]),  1, CV_AA, 0);

	cvLine(img,   cvPoint((int)(x + meter->size * cos(theta[1])),   (int)(y + meter->size * sin(theta[1]))),	cvPoint((int)(x + (meter->size * 0.7) * cos(theta[1])),   (int)(y + (meter->size * 0.7) * sin(theta[1]))),   CV_RGB(design->circle_min_color[0],design->circle_min_color[1],design->circle_min_color[2]),  2, CV_AA, 0);
	cvLine(img,   cvPoint((int)(x + meter->size * cos(theta[2])),   (int)(y + meter->size * sin(theta[2]))),	cvPoint((int)(x + (meter->size * 0.7) * cos(theta[2])),   (int)(y + (meter->size * 0.7) * sin(theta[2]))),   CV_RGB(design->circle_max_color[0],design->circle_max_color[1],design->circle_max_color[2]),  2, CV_AA, 0);
	cvLine(img,   cvPoint((int)(x + meter->size * cos(theta[0])),   (int)(y + meter->size * sin(theta[0]))),	cvPoint((int)(x + (meter->size * 0.7) * cos(theta[0])),   (int)(y + (meter->size * 0.7) * sin(theta[0]))),   CV_RGB(design->circle_value_color[0],design->circle_value_color[1],design->circle_value_color[2]),  2, CV_AA, 0);

	sprintf(buf, "%7.02f", value[0]);
	cvPutText(img, buf, cvPoint(x - (meter->size * 0.7), y),						&meter->font_, CV_RGB(design->font_edge_color[0], design->font_edge_color[1], design->font_edge_color[2]));
	cvPutText(img, buf, cvPoint(x - (meter->size * 0.7), y),						&meter->font,  CV_RGB(design->font_color[0],      design->font_color[1],      design->font_color[2]));
	sprintf(buf, "%s", meter->label);
	cvPutText(img, buf, cvPoint(x - (meter->size * 0.5), y + (meter->size / 4)),	&meter->font_, CV_RGB(design->font_edge_color[0], design->font_edge_color[1], design->font_edge_color[2]));
	cvPutText(img, buf, cvPoint(x - (meter->size * 0.5), y + (meter->size / 4)),	&meter->font,  CV_RGB(design->font_color[0],      design->font_color[1],      design->font_color[2]));

	return;
}


/**************************************************************************************************
パラメータ表示
**************************************************************************************************/
void DRAW_param(CvArr* img, pMETER meter, pDRAW_METER design)
{
	char buf[128];
	sprintf(buf, "%7.02f %s", meter->value, meter->label);
	cvPutText(img, buf, cvPoint(meter->locate[0], meter->locate[1]),						&meter->font_, CV_RGB(design->font_edge_color[0], design->font_edge_color[1], design->font_edge_color[2]));
	cvPutText(img, buf, cvPoint(meter->locate[0], meter->locate[1]),						&meter->font,  CV_RGB(design->font_color[0],      design->font_color[1],      design->font_color[2]));

	return;
}



/**************************************************************************************************
ミッションを描画する
**************************************************************************************************/
void DRAW_mission(CvArr* img, unsigned char gear, pDRAW_METER design)
{
	char buf[3];
	CvFont font, font_;

	if(gear == 0)	sprintf(buf, "N");
	else			sprintf(buf, "%d", gear);

	cvInitFont(&font,	CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("MISSION_FONT")), atof(CFG_value("MISSION_FONT")), 0, 1, CV_AA);
	cvInitFont(&font_,	CV_FONT_HERSHEY_DUPLEX, atof(CFG_value("MISSION_FONT")), atof(CFG_value("MISSION_FONT")), 0, 2, CV_AA);

	cvPutText(img, buf, cvPoint(atoi(CFG_value("MISSION_X")), atoi(CFG_value("MISSION_Y"))), &font_, CV_RGB(design->font_edge_color[0], design->font_edge_color[1], design->font_edge_color[2]));
	cvPutText(img, buf, cvPoint(atoi(CFG_value("MISSION_X")), atoi(CFG_value("MISSION_Y"))), &font,  CV_RGB(design->font_color[0],      design->font_color[1],      design->font_color[2]));
}



/**************************************************************************************************
ラップタイムを表示する
**************************************************************************************************/
void DRAW_laptime(CvArr* img, unsigned long total, unsigned long lap, pDRAW_METER design)
{
	char buf[30];
	unsigned int hour, min, sec, msec;
	CvFont total_font, total_font_;
	CvFont lap_font, lap_font_;

	cvInitFont(&total_font,	 CV_FONT_HERSHEY_TRIPLEX , atof(CFG_value("LAPTIME_TOTAL_FONT")), atof(CFG_value("LAPTIME_TOTAL_FONT")), 0, 1, CV_AA);
	cvInitFont(&total_font_, CV_FONT_HERSHEY_TRIPLEX , atof(CFG_value("LAPTIME_TOTAL_FONT")), atof(CFG_value("LAPTIME_TOTAL_FONT")), 0, 2, CV_AA);
	cvInitFont(&lap_font,	 CV_FONT_HERSHEY_TRIPLEX , atof(CFG_value("LAPTIME_LAP_FONT")),   atof(CFG_value("LAPTIME_LAP_FONT")),   0, 1, CV_AA);
	cvInitFont(&lap_font_,   CV_FONT_HERSHEY_TRIPLEX , atof(CFG_value("LAPTIME_LAP_FONT")),   atof(CFG_value("LAPTIME_LAP_FONT")),   0, 2, CV_AA);

	hour = 0;
	min  = 0;
	sec  = 0;
	msec = 0;
	if(total > 3600000) hour =  total / 3600000;
	if(total >   60000) min  = (total /   60000) %   60;
	if(total >    1000) sec  = (total /    1000) %   60;
	if(total >       0) msec =  total            % 1000;
	sprintf(buf, "T : %02d:%02d:%02d.%03d", hour, min, sec, msec);

	cvPutText(img, buf, cvPoint(atoi(CFG_value("LAPTIME_TOTAL_X")), atoi(CFG_value("LAPTIME_TOTAL_Y"))), &total_font_, CV_RGB(design->font_edge_color[0], design->font_edge_color[1], design->font_edge_color[2]));
	cvPutText(img, buf, cvPoint(atoi(CFG_value("LAPTIME_TOTAL_X")), atoi(CFG_value("LAPTIME_TOTAL_Y"))), &total_font,  CV_RGB(design->font_color[0],      design->font_color[1],      design->font_color[2]));

	hour = 0;
	min  = 0;
	sec  = 0;
	msec = 0;
	if(lap > 3600000) hour =  lap / 3600000;
	if(lap >   60000) min  = (lap /   60000) %   60;
	if(lap >    1000) sec  = (lap /    1000) %   60;
	if(lap >       0) msec =  lap            % 1000;
	sprintf(buf, "L : %02d:%02d:%02d.%03d", hour, min, sec, msec);
	cvPutText(img, buf, cvPoint(atoi(CFG_value("LAPTIME_LAP_X")), atoi(CFG_value("LAPTIME_LAP_Y"))), &lap_font_, CV_RGB(design->font_edge_color[0], design->font_edge_color[1], design->font_edge_color[2]));
	cvPutText(img, buf, cvPoint(atoi(CFG_value("LAPTIME_LAP_X")), atoi(CFG_value("LAPTIME_LAP_Y"))), &lap_font,  CV_RGB(design->font_color[0],      design->font_color[1],      design->font_color[2]));

}


void ecu_init(void)
{
	ecu = &ecu_[0];

	/* 初期値代入 */
	ecu_[0].engine_speed[0]		= 9000;
	ecu_[0].engine_speed[1]		= 1000;
	ecu_[0].engine_speed[2]		= 0;
	ecu_[1].engine_speed[0]		= 9000;
	ecu_[1].engine_speed[1]		= 1000;
	ecu_[1].engine_speed[2]		= 0;

	ecu_[0].vehicle_speed[0]	= 255;
	ecu_[0].vehicle_speed[1]	= 0;
	ecu_[0].vehicle_speed[2]	= 0;
	ecu_[1].vehicle_speed[0]	= 255;
	ecu_[1].vehicle_speed[1]	= 0;
	ecu_[1].vehicle_speed[2]	= 0;

	ecu_[0].boost[0]			= 2.0;
	ecu_[0].boost[1]			= 0.0;
	ecu_[0].boost[2]			= -1.0;
	ecu_[1].boost[0]			= 2.0;
	ecu_[1].boost[1]			= 0.0;
	ecu_[1].boost[2]			= -1.0;

	ecu_[0].coolant[0]			= 150;
	ecu_[0].coolant[1]			= 0;
	ecu_[0].coolant[2]			= 0;
	ecu_[1].coolant[0]			= 150;
	ecu_[1].coolant[1]			= 0;
	ecu_[1].coolant[2]			= 0;

	ecu_[0].intakeair[0]		= 150;
	ecu_[0].intakeair[1]		= 0;
	ecu_[0].intakeair[2]		= 0;
	ecu_[1].intakeair[0]		= 150;
	ecu_[1].intakeair[1]		= 0;
	ecu_[1].intakeair[2]		= 0;

	ecu_[0].throttle[0]			= 100;
	ecu_[0].throttle[1]			= 0;
	ecu_[0].throttle[2]			= 0;
	ecu_[1].throttle[0]			= 100;
	ecu_[1].throttle[1]			= 0;
	ecu_[1].throttle[2]			= 0;

	ecu_[0].gear[0]				= 0;
	ecu_[0].gear[1]				= 0;
	ecu_[0].gear[2]				= 0;
	ecu_[1].gear[0]				= 0;
	ecu_[1].gear[1]				= 0;
	ecu_[1].gear[2]				= 0;

	return;
}
