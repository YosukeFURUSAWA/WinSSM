#ifndef _MAIN_H_
#define _MAIN_H_


#define DEBUG(x)		if(atoi(CFG_value("DEBUG")) > 0) fprintf(stderr, x);
#define DEBUG_(x,y)		if(atoi(CFG_value("DEBUG")) > 0) fprintf(stderr, x,y);
#define DEBUG2(x)		if(atoi(CFG_value("DEBUG")) > 1) fprintf(stderr, x);
#define DEBUG2_(x,y)	if(atoi(CFG_value("DEBUG")) > 1) fprintf(stderr, x,y);
#define DEBUG3(x)		if(atoi(CFG_value("DEBUG")) > 2) fprintf(stderr, x);
#define DEBUG3_(x,y)	if(atoi(CFG_value("DEBUG")) > 2) fprintf(stderr, x,y);

#endif


