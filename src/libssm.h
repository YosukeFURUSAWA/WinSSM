#ifndef _LIBSSM_H_
#define _LIBSSM_H_

/*
#define TRUE		(1)
#define FALSE		(0)
*/

typedef struct {
	int fd;
	double engine_speed;
	double throttle;
	double vehicle_speed;
	double boost;
	double coolant;
	double intakeair;
	double battery;
	unsigned char sw;
	unsigned char gear;
	double maf;
	double afr;
	double fuel;

} SSM;
typedef SSM* pSSM;


extern pSSM SSM_open(void);
extern int SSM_close(pSSM ssm);
extern int SSM_read(pSSM ssm, unsigned int address);
extern int SSM_block_read(pSSM ssm);
extern int SSM_calc_gear(double vehicle, double rpm);

#endif
