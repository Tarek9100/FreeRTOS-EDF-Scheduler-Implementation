#ifndef STD_TYPES_H
#define STD_TYPES_H

typedef unsigned char u8;
typedef signed char s8;
typedef unsigned short int u16;
typedef signed short int s16;
typedef unsigned int u32;
typedef signed int s32;
typedef float f32;
typedef double f64;


#define NULL 	((void*)0)

typedef struct{

char ButtonID;
signed Edge_Flag;	
	
}ButtonState_t;
#endif