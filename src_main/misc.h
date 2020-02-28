#pragma once



#define MALLOC_TYPE(type, name) type* name = (type*)malloc(sizeof(type));



#ifdef __cplusplus
	#define SEPPLES_GUARD_START extern "C" {
	#define SEPPLES_GUARD_END }
	// #define TYPEDEF_STRUCT(name) struct ##name##_t;
	#define TYPEDEF_STRUCT(name) typedef struct _##name name##_t;
#else
	#define SEPPLES_GUARD_START
	#define SEPPLES_GUARD_END
	#define TYPEDEF_STRUCT(name) typedef struct _##name name##_t;
#endif


