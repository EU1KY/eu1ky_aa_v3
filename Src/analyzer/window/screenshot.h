/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

void SCREENSHOT_Show(const char* fname);
char* SCREENSHOT_SelectFileName(void);
void SCREENSHOT_DeleteOldest(void);
void SCREENSHOT_Save(const char *fname);

#endif
