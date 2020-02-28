#include "windows_include.h"
#include "util.h"

#include <stdio.h>

// #ifdef __STDC_NO_THREADS__
#include "../third_party/tinycthread/tinycthread.h"
// #endif

#include "../third_party/utf8proc/utf8proc.h"

void util_sleep_for_ms(unsigned ms) {
    struct timespec sleep_for;
    sleep_for.tv_sec = 0;
    sleep_for.tv_nsec = (long)ms * (long)1000000;
    thrd_sleep(&sleep_for, NULL);
}

void util_wide_to_utf8(wchar_t* wide, size_t wide_len, char* utf8, size_t utf8_len) {
    int utf8_result_len = WideCharToMultiByte(
        CP_UTF8,
        WC_NO_BEST_FIT_CHARS,
        wide,
        wide_len,
        utf8,
        utf8_len-1,
        NULL,
        NULL
    );

    utf8[utf8_result_len] = '\0';
}

// bool util_compare_utf8_ci(char* a, char* b) {

// }

static utf8proc_ssize_t _dontallocate_utf8proc_map_custom(
    const utf8proc_uint8_t *str,
    utf8proc_ssize_t strlen,
    char            *dstptr,
    size_t           dstsize,
    utf8proc_option_t options,
    utf8proc_custom_func custom_func,
    void *custom_data
) {
    utf8proc_int32_t *buffer;
    utf8proc_ssize_t result;
    // *dstptr = NULL;
    result = utf8proc_decompose_custom(str, strlen, NULL, 0, options, custom_func, custom_data);
    if (result < 0) {
        printf("casefold err1\n");
        return result;
    }

    size_t buffer_len_needed = result * sizeof(utf8proc_int32_t) + 1;
    // if (buffer_len_needed >= dstsize) {
    //     printf("casefold err2\n");
    //     return -1;
    // }

    uint8_t _buffer[buffer_len_needed];
    buffer = (utf8proc_int32_t *) _buffer;
    if (!buffer) return UTF8PROC_ERROR_NOMEM;
    result = utf8proc_decompose_custom(str, strlen, buffer, result, options, custom_func, custom_data);
    if (result < 0) {
        printf("casefold err3\n");
        return result;
    }
    result = utf8proc_reencode(buffer, result, options);
    if (result < 0) {
        printf("casefold err4\n");
        return result;
    }
    if (((size_t)result) >= dstsize) {
        printf("casefold err2\n");
        return -1;
    }

    strcpy(dstptr, (char*)buffer);
    return result;
}



void util_casefold_utf8(const char* in, char* out, size_t out_len) {
    utf8proc_ssize_t res = _dontallocate_utf8proc_map_custom(
        (uint8_t*)in,
        strlen(in),
        out,
        out_len,
        UTF8PROC_CASEFOLD,
        NULL,
        NULL
    );
    (void)res;
    // printf("casefold result: %i\n", (int)res);
}