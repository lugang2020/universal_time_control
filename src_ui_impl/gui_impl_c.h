#pragma once

#if defined __cplusplus
    #define EXTERN_C extern "C"
#else
	#include <stdbool.h>
    #define EXTERN_C extern
#endif

#ifndef GUI_IMPL_API
// #define GUI_IMPL_API __declspec(dllimport)
#define GUI_IMPL_API
#endif

#include "../shared.h"

GUI_IMPL_API bool   gui_impl_init(const char* title);
GUI_IMPL_API void   gui_impl_cleanup();
GUI_IMPL_API bool   gui_impl_begin_frame_and_return_should_quit();
GUI_IMPL_API void   gui_impl_end_frame();
GUI_IMPL_API float  gui_impl_get_scale();
GUI_IMPL_API void   gui_impl_set_clear_colour(int r, int g, int b);


GUI_IMPL_API void   gui_impl_enter_get_key_mode();
GUI_IMPL_API void   gui_impl_leave_get_key_mode();
GUI_IMPL_API bool   gui_impl_is_get_key_mode();
GUI_IMPL_API bool   gui_impl_get_set_key(key_t* out);

// GUI_IMPL_API void  gui_impl_scale_styles_for_dpi();