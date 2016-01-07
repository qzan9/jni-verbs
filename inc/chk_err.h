/*
 * Copyright (c) 2015, AZQ. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef _CHK_ERR_H_
#define _CHK_ERR_H_

// if x is ZERO, return -1.
#define CHK_ZI(x)\
	do {\
		if (!(x)) return -1;\
	} while (0)

// if x is NEGATIVE, return -1.
#define CHK_NI(x)\
	do {\
		if ((x)<0) return -1;\
	} while (0)

// if x is NON-ZERO, return -1.
#define CHK_NZI(x)\
	do {\
		if ((x)) return -1;\
	} while (0)

// if x is ZERO, print msg to stderr and return -1.
#define CHK_ZPI(x,msg)\
	do {\
		if (!(x)) {\
			fprintf(stderr, "error: %s\n", (msg));\
			return -1;\
		}\
	} while (0)

// if x is NEGATIVE, print msg to stderr and return -1.
#define CHK_NPI(x,msg)\
	do {\
		if ((x)<0) {\
			fprintf(stderr, "error: %s\n", (msg));\
			return -1;\
		}\
	} while (0)

// if x is NON-ZERO, print msg to stderr and return -1.
#define CHK_NZPI(x,msg)\
	do {\
		if ((x)) {\
			fprintf(stderr, "error: %s\n", (msg));\
			return -1;\
		}\
	} while (0)

// if x is ZERO, print errno and msg to stderr and return -1.
#define CHK_ZEI(x,msg)\
	do {\
		if (!(x)) {\
			fprintf(stderr, "error: %s - %s\n", strerror(errno), (msg));\
			return -1;\
		}\
	} while (0)

// if x is NEGATIVE, print errno and msg to stderr and return -1.
#define CHK_NEI(x,msg)\
	do {\
		if ((x)<0) {\
			fprintf(stderr, "error: %s - %s\n", strerror(errno), (msg));\
			return -1;\
		}\
	} while (0)

// if x is NON-ZERO, print errno and msg to stderr and return -1.
#define CHK_NZEI(x,msg)\
	do {\
		if ((x)) {\
			fprintf(stderr, "error: %s - %s\n", strerror(errno), (msg));\
			return -1;\
		}\
	} while (0)

#endif /* _CHK_ERR_H_ */

