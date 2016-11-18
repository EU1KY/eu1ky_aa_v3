/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#include <stdint.h>

/**
    @brief Alphanumeric keyboard window.
           Automatically stores and restores LCD contents
    @param buffer A buffer with string to be edited
    @param max_len Maximum number of characters to fit in the buffer
    @param header_text Keyboard window header text
    @return 1 if the string in buffer has changed, 0 if not changed
*/
uint32_t KeyboardWindow(char* buffer, uint32_t max_len, const char* header_text);

#endif
