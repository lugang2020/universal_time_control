#pragma once
#include "../src_main/windows_include.h"

#include <unordered_set>

#ifndef GUI_IMPL_API
// #define GUI_IMPL_API __declspec(dllimport)
#define GUI_IMPL_API extern "C" __declspec(dllimport)
// #define GUI_IMPL_API
#endif

#include "../shared.h"

#include <string>

#include "../third_party/mingw-std-threads/mingw.condition_variable.h"


namespace std {
  template <> struct hash<key_t>
  {
    size_t operator()(const key_t& a) const
    {
        const std::string str =
            std::string( reinterpret_cast<const std::string::value_type*>( &a ), sizeof(key_t) );
        return std::hash<std::string>()( str );
    }
  };

  template <> struct equal_to<key_t>
  {
    bool operator()(const key_t& a, const key_t& b) const
    {
    	return memcmp(&a, &b, sizeof(key_t)) == 0;
    }
  };

}

typedef std::unordered_set<key_t> set_of_keys;


GUI_IMPL_API void gui_impl_get_keys_data(set_of_keys* out_keys_down, set_of_keys* out_keys_just_pressed);
// namespace std {
//   class condition_variable;
// }

GUI_IMPL_API std::condition_variable* gui_impl_get_keys_cv();
GUI_IMPL_API std::mutex* gui_impl_get_keys_mutex();



// GUI_IMPL_API void  gui_impl_scale_styles_for_dpi();