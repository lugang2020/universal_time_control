#ifndef _UTC_SHARED_H_
#define _UTC_SHARED_H_

#define CNC_WNDCLASS_NAME L"universaltimecontrol_cnc_wndcls"
#define CNC_WNDNAME_PREFIX L"universaltimecontrol_cnc_wndforpid_"

typedef enum {
	UTC_IPC_CMD_SHOULD_UNLOAD = 1,
	UTC_IPC_CMD_SET_TIMESCALE = 2
} timecontrol_ipc_cmd_type_t;

#include <stdbool.h>
#include <stdint.h>

#define PACKED __attribute__((packed))

typedef struct {
	timecontrol_ipc_cmd_type_t cmd_type PACKED;
	float timeScale PACKED;
} PACKED timecontrol_ipc_cmd_t;

typedef char str31[32];

#undef PACKED
#define PACKED

typedef struct {
	uint32_t v_code    PACKED;
	uint32_t scan_code PACKED;
	str31    desc_utf8 PACKED;
	bool     ctrl      PACKED;
	bool     alt       PACKED;
	bool     shift     PACKED;
} PACKED key_t;

#undef PACKED

#define WM_COPYDATA_ID 42

#endif