/*
 * debug.h
 *
 *  Created on: Jul 29, 2013
 *      Author: heavey
 */

#ifndef DEBUG_FUNCTIONS_H_
#define DEBUG_FUNCTIONS_H_

#define Error(...) do{ \
	printf("%s:%u:ERROR: ", __FILE__, __LINE__);\
	printf(" (%s)", strerror(errno));\
	printf(__VA_ARGS__); \
	printf(" [%s]\n", __FUNCTION__);\
	}while(0)

#ifdef DEBUG_FUNCTIONS_Trace
#define Trace(...) do{ \
	printf("%s:%u:TRACE: ", __FILE__, __LINE__);\
	printf(__VA_ARGS__); \
	printf(" [%s]\n", __FUNCTION__);\
	}while(0)
#else
#define Trace(...) do{ }while(0)
#endif

#ifdef DEBUG_FUNCTIONS_Info
#define Info(...) do{ \
	printf("%s:%u:INFO: ", __FILE__, __LINE__);\
	printf(__VA_ARGS__); \
	printf(" [%s]\n", __FUNCTION__);\
	}while(0)
#else
#define Info(...) do{ }while(0)
#endif

#ifdef DEBUG_FUNCTIONS_Debug
#define Debug(...) do{ \
	printf("%s:%u:DEBUG: ", __FILE__, __LINE__);\
	printf(__VA_ARGS__); \
	printf(" [%s]\n", __FUNCTION__);\
	}while(0)
#else
#define Debug(...) do{ }while(0)
#endif

#endif /* DEBUG_FUNCTIONS_H_ */
