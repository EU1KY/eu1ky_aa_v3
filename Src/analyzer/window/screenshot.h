/*
 *   (c) Yury Kuchura
 *   kuchura@gmail.com
 *
 *   This code can be used on terms of WTFPL Version 2 (http://www.wtfpl.net/).
 */

#ifndef SCREENSHOT_H_
#define SCREENSHOT_H_

#define SCREENSHOT_FILE_SIZE 391734

void SCREENSHOT_Show(const char* fname);
char* SCREENSHOT_SelectFileName(void);
int16_t SCREENSHOT_SelectFileNames(int k);
void SCREENSHOT_DeleteOldest(void);
void SCREENSHOT_Save(const char *fname);
void SCREENSHOT_SavePNG(const char *fname);
void SCREENSHOT_ShowPicture(uint16_t Pointer1);
void SCREENSHOT_DeleteFile(uint16_t Pointer1);
void Date_Time_Stamp(void);
int32_t ShowLogo(void);
#endif
