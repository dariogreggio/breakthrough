/*
 * BREAKTHROUGH 2.0
 * (C) G.Dar 1987-2023 :) !
 * (portions inspired by Windows, since 1990)
 * 
 * http://nano-x.org/ ahah siete morti :D https://github.com/moovel/microwindows
 * https://minigui.fmsoft.cn/api_ref/3.0.12_threads/index.html boh...?
 */
 
#include <xc.h>
//#include "compiler.h"
#include "pc_pic_cpu.h"
#include <stdio.h>
#include <string.h>

#define BREAKTHROUGH_COPYRIGHT_STRING "Breakthrough v2.0.7 - build 3/2/2023"


#define Color565(red, green, blue)    (GFX_COLOR) ((((GFX_COLOR)(red) & 0xF8) << 8) | (((GFX_COLOR)(green) & 0xFC) << 3) | (((GFX_COLOR)(blue) & 0xF8) >> 3))
#define ColorBGR(color)    (DWORD) ( ((color & 0xf800) >> 11) | ((color & 0x07e0) << 5) | ((((DWORD)color) & 0x001f) << 16) )
#define ColorRGB(color)    (DWORD) ( ((((DWORD)color) & 0xf800) << 8) | ((((DWORD)color) & 0x07e0) << 5) | ((((DWORD)color) & 0x001f) << 3) )
#define ColorR(color)    ((color & 0xf800) >> 8)        // left-justified, diciamo..
#define ColorG(color)    ((color & 0x07e0) >> 3)
#define ColorB(color)    ((color & 0x001f) << 3)
#define Color565To332(color)    (BYTE) ((WORD)( ((color & 0xe000) >> 8) | ((color & 0x0700) >> 6) | ((color & 0x0018) >> 3) ))
#define Color565To222(color)    (BYTE) ((WORD)( ((color & 0xc000) >> 10) | ((color & 0x0600) >> 7) | ((color & 0x0018) >> 3) ))
//#define Color24To565(color)    ((((color >> 16) & 0xFF) / 8) << 11) | ((((color >> 8) & 0xFF) / 4) << 5) | (((color) &  0xFF) / 8)
  //convert 24bit color into packet 16 bit one (credits for this are all mine) GD made a macro!

#include "breakthrough.h"
#include "fat_sd/FSIO.h"
#if defined(USA_USB_HOST_MSD)
#include "../harmony_pic32mz/usb_host_msd.h"
#include "../harmony_pic32mz/usb_host_scsi.h"
#endif
#include "fat_ide/idefsio.h"
#include "superfile.h"

#include "picojpeg.h"
#include "kommissarRexx/kommissarRexx.h"

extern enum FILE_DEVICE m_stdout,m_stdin,m_stderr;
extern const char *ASTERISKS;
extern char prompt[16];
extern signed char currDrive;

GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;
extern const unsigned char font[];
extern const GFXfont FreeSans9pt7b[],FreeSans12pt7b[],FreeSerif9pt7b[],FreeSerif12pt7b[];
BYTE wrap,textsize,_cp437;
GFXfont *gfxFont;
HWND rootWindows=NULL,taskbarWindow=NULL;
HWND desktopWindow=NULL;
WNDCLASS *rootClasses=NULL;
THREAD *rootThreads=NULL,*winManagerThreadID;
S_POINT mousePosition;
CURSOR mouseCursor;
GFX_COLOR savedCursorArea[16*16];
MENU *activeMenu=NULL;
BYTE eXtra;     // per abilitare mouse,suoni e altro..
const char *profileFile="WIN.INI";
        
extern volatile unsigned long now;
extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern DWORD extRAMtot;
extern struct KEYPRESS keypress;
extern struct MOUSE mouse;


const GFX_COLOR standardIcon[]={
  WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,
  WHITE,LIGHTRED,WHITE,RED,WHITE,RED,WHITE,LIGHTRED,
  WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,
  LIGHTGREEN,BLACK,LIGHTGREEN,BLACK,LIGHTGREEN,BLACK,LIGHTGREEN,BLACK,
  WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,
  WHITE,LIGHTBLUE,WHITE,LIGHTBLUE,WHITE,LIGHTBLUE,WHITE,LIGHTBLUE,
  WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,
  WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,WHITE,BLACK,
  };
#define CUR_INV 3
#define CUR_WHT WHITE     //2
#define CUR_BLK BLACK +1 /*+1 se usata trasparenza*/    //1
#define CUR_TRNSP BLACK   //  0 
const GFX_COLOR /*BYTE */standardCursor[]={
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT
  };
const GFX_COLOR standardCursorSm[]={
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT
  };
const GFX_COLOR halfSquareCursor[]={
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP
  };
const GFX_COLOR halfSquareCursorSm[]={
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  };
const GFX_COLOR standardCaret[]={
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  BLACK,BLACK,BLACK,CUR_TRNSP,
  CUR_TRNSP,BLACK,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,BLACK,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,BLACK,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,BLACK,CUR_TRNSP,CUR_TRNSP,
  BLACK,BLACK,BLACK,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  };
const GFX_COLOR redBallIcon[]={   //https://www.digole.com/tools/PicturetoC_Hex_converter.php
  BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,
  BLACK,0x2110,0xa989,LIGHTMAGENTA,LIGHTMAGENTA,0xa989,0x2110,BLACK,
  BLACK,0xa989,LIGHTMAGENTA,LIGHTMAGENTA,0x09f0,RED,RED,BLACK,
  BLACK,BRIGHTRED,BRIGHTRED,0xd4fa,RED,RED,RED,BLACK,
  BLACK,BRIGHTRED,0x07d0,RED,RED,RED,RED,BLACK,
  BLACK,0xa981,RED,RED,RED,RED,RED,BLACK,
  BLACK,0x2110,RED,RED,RED,RED,0x0008,BLACK,
  BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK
  };
const GFX_COLOR recyclerIcon[]={
  BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,BLACK,
  BLACK,LIGHTBLUE,LIGHTBLUE,LIGHTBLUE,LIGHTBLUE,LIGHTBLUE,LIGHTBLUE,BLACK,
  BLACK,LIGHTBLUE,BLACK,BLACK,BLACK,BLACK,LIGHTBLUE,BLACK,
  BLACK,LIGHTBLUE,BLACK,BLACK,BLACK,BLACK,LIGHTBLUE,BLACK,
  BLACK,BLACK,LIGHTBLUE,BLACK,BLACK,LIGHTBLUE,BLACK,BLACK,
  BLACK,BLACK,LIGHTBLUE,BLACK,BLACK,LIGHTBLUE,BLACK,BLACK,
  BLACK,BLACK,BLACK,LIGHTBLUE,LIGHTBLUE,BLACK,BLACK,BLACK,
  BLACK,BLACK,BLACK,LIGHTBLUE,LIGHTBLUE,BLACK,BLACK,BLACK
  };
const GFX_COLOR folderIcon8[]={
  BROWN,BROWN,      BROWN,      SIENNA,       DARKGRAY,       DARKGRAY,       DARKGRAY,       DARKGRAY,
  BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,      SIENNA,       DARKGRAY,       DARKGRAY,       DARKGRAY,
  BROWN,BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      SIENNA,
  BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,
  BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,
  BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,
  BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,
  SIENNA, BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      SIENNA
  };
const GFX_COLOR fileIcon8[]={
  DARKGRAY,WHITE,    WHITE,    WHITE,    WHITE,    WHITE,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,     CYAN,     CYAN,     CYAN,    WHITE,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,     CYAN,    WHITE,    WHITE,     CYAN,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,     CYAN,     CYAN,     CYAN,     CYAN,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,     CYAN,    WHITE,    WHITE,     CYAN,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,     CYAN,     WHITE,     WHITE,     CYAN,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,     CYAN,     CYAN,     CYAN,     CYAN,   WHITE,DARKGRAY,
  DARKGRAY,WHITE,    WHITE,    WHITE,    WHITE,    WHITE,   WHITE,DARKGRAY
  };
const GFX_COLOR folderIcon[]={
  BROWN,BROWN,BROWN, BROWN,BROWN,BROWN,      SIENNA,SIENNA,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,
  BROWN,BROWN,BROWN, BROWN,BROWN,BROWN,      SIENNA,SIENNA,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,      SIENNA,SIENNA,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,      SIENNA,SIENNA,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,       DARKGRAY,DARKGRAY,
  BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      SIENNA,SIENNA,
  BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      SIENNA,SIENNA,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  BROWN,BROWN,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BRIGHTYELLOW,BROWN,BROWN,
  SIENNA, SIENNA, BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      SIENNA,      SIENNA,
  SIENNA, SIENNA, BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      BROWN,      SIENNA,      SIENNA
  };
const GFX_COLOR fileIcon[]={
  DARKGRAY,DARKGRAY,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,WHITE,CYAN, WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,WHITE,WHITE,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,WHITE,WHITE,WHITE,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,WHITE,CYAN, WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,WHITE,WHITE,CYAN,CYAN,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN, CYAN, WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN,CYAN, CYAN, WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,DARKGRAY,DARKGRAY,
  DARKGRAY,DARKGRAY,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,DARKGRAY,DARKGRAY
  };
const GFX_COLOR windowIcon[]={
  GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,
  GRAY95,GRAY2,GRAY2,GRAY2,GRAY2,GRAY2,GRAY2,GRAY95,
  GRAY95,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,GRAY95,
  GRAY95,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,GRAY95,
  GRAY95,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,GRAY95,
  GRAY95,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,GRAY95,
  GRAY95,WHITE,WHITE,WHITE,WHITE,WHITE,WHITE,GRAY95,
  GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,GRAY95,GRAY95
  };

const MENU systemMenu={{   // MF_SYSMENU mettere? dove?
  {"Ripristina", SC_RESTORE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Sposta", SC_MOVE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Ridimensiona", SC_SIZE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Riduci a icona", SC_MINIMIZE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Ingrandisci", SC_MAXIMIZE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Chiudi", SC_CLOSE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},   // se CS_NOCLOSE deve sparire!
  {"\0\0\0\0", 0, 0, NULL}
  },
  {
  {0}
  }
  };
// MNS_NOTIFYBYPOS v., volendo
const MENU explorerMenu3={{
  {"A:", 32+1, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"C:", 32+2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
#if defined(USA_USB_HOST_MSD)
  {"E:", 32+3, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
#endif
#ifdef USA_RAM_DISK 
  {"R:", 32+4, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
#endif
  {"\0\0\0\0", 0, 0, NULL},{0}
  },
  {
  {'A',0b00000001},{'C',0b00000001},{'E',0b00000001},{'R',0b00000001},
  }
  };
const MENU explorerMenu1={{
  {"Apri", 1, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&explorerMenu3},
  {"Copia...", 2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Formatta", 3, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Altro", 4, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Esci", 5, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
const MENU explorerMenu2={{
  {"Icone grandi", 16+1, MF_ENABLED | MF_STRING | MF_CHECKED, NULL},
  {"Icone piccole", 16+2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Dettagli", 16+3, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Aggiorna", 16+4, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
const MENU explorerMenu={{    // il titolo :)
  {"File", 0, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&explorerMenu1},
  {"Visualizza", 0, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&explorerMenu2},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
const MENU cmdShellMenu1={{
  {"Esegui...", 1, MF_ENABLED | MF_STRING | MF_CHECKED, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Esci", 2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},{0}
  },
  {
  {0}
  }
  };
const MENU cmdShellMenu2={{
  {"Cut", 16+1, MF_GRAYED | MF_STRING | MF_CHECKED, NULL},
  {"Copy", 16+2, MF_GRAYED | MF_STRING | MF_CHECKED, NULL},
  {"Paste", 16+3, MF_GRAYED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
const MENU cmdShellMenu={{    // il titolo :)
  {"File", 0, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&cmdShellMenu1},
  {"Modifica", 0, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&cmdShellMenu2},
  {"\0\0\0\0", 0, 0, NULL},{0}
  },
  {
  {0}
  }
  };



#define MAX_TIMERS 8
TIMER_DATA timerData[MAX_TIMERS];    // diciamo 8

static void DrawCharWindow(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y, unsigned char c);
static BOOL DrawPixelWindow(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y);
static BOOL DrawPixelWindowColor(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y,GFX_COLOR c);
static void DrawLineWindow(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
static void DrawLineWindowColor(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
static void DrawHorizLineWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T x2);
static void DrawHorizLineWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,GFX_COLOR c);
static void DrawVertLineWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2);
static void DrawVertLineWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2,GFX_COLOR c);
static void DrawRectangleWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
static void DrawRectangleWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
static void FillRectangleWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
static void FillRectangleWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
static void DrawCircleWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
static inline BOOL __attribute__((always_inline)) isPointVisible(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y);
static BOOL nonClientPaint(HWND hWnd,const RECT *rc);
static BOOL clientPaint(HWND hWnd,const RECT *rc);
static void calculateClientArea(HWND hWnd,RECT *rc);
static void calculateNonClientArea(HWND hWnd,RECT *rc);
static void activateChildren(HWND hWnd);
static HWND getLastChildWindow(HWND hWnd);
static BOOL drawMenu(HWND hWnd,MENU *menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y);
static BOOL drawMenuPopup(HWND hWnd,MENU *menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y);
static uint16_t getMenuFromPoint(HWND hWnd,POINT pt,MENU **inMenu);
static uint16_t getMenuPopupFromPoint(HWND hWnd,MENU *menu,POINT pt,MENU **inMenu);
static POINT getMenuPosition(MENU *menu2,MENU *menuorig);

static BOOL addHeadWindow(HWND,struct _WINDOW **head);
static BOOL addTailWindow(HWND,struct _WINDOW **head);
static BOOL insertWindow(HWND,HWND,struct _WINDOW **head);
static BOOL removeWindow(HWND,struct _WINDOW **head);
//static void SortLinkedList(struct _WINDOW *head);
static void list_bubble_sort(struct _WINDOW **head);
static void printWindows(HWND);

LRESULT DefWindowProc(HWND,uint16_t,WPARAM ,LPARAM);
LRESULT SendMessage(HWND,uint16_t,WPARAM ,LPARAM);
static HWND setActive(HWND hWnd,BOOL state);


static void setTextSize(BYTE n);
void DrawWindow(UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,GFX_COLOR f,GFX_COLOR b);
void MovePointer(UGRAPH_COORD_T x,UGRAPH_COORD_T y);

VARIABLESTRING *findEnvVariable(const char *);


static DWORD jpegLen;
static BYTE *jpegPtr;
static SUPERFILE *jpegFile;   // metterli tutti dentro la classe desktopclass... union..
static unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data) {
  uint32_t n;

  if(pCallback_data) {
    n=SuperFileRead((SUPERFILE *)jpegPtr,pBuf,buf_size);
    if(n != buf_size)
      return PJPG_STREAM_READ_ERROR;
    *pBytes_actually_read = (unsigned char)n;
    }
  else {
    n = min(jpegLen, buf_size);
    if(n != buf_size)
      return PJPG_STREAM_READ_ERROR;
    memcpy(pBuf,jpegPtr,n);
    *pBytes_actually_read = (unsigned char)n;
    jpegPtr += n;
    jpegLen -= n;
    }
  return 0;
  }
static pjpeg_image_info_t JPG_Info;

//-------------------------windows----------------------------------------------
int8_t InitWindows(GFX_COLOR bgcolor /*DARKBLUE*/,enum ORIENTATION orientation,BYTE xtra,const char *sfondo) {
  char buf[16];
  CREATESTRUCT cs,cs2;
  
  eXtra=xtra;
  
  windowForeColor=BRIGHTCYAN;
  windowInactiveForeColor=GRAY160;
  windowBackColor=DARKGRAY;
  desktopColor=bgcolor;
  rootWindows=NULL;
  memset(timerData,0,sizeof(timerData));

  desktopWindow=malloc(sizeof(struct _WINDOW)+2+2+sizeof(FONT)+16+sizeof(S_POINT)*16+1);    // v. desktop class
  if(!desktopWindow)
    return 0;
  cs.class=desktopWindow->class=MAKECLASS(WC_DESKTOPCLASS);
LRESULT DefWindowProcDC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
  desktopWindow->windowProc=(WINDOWPROC*)DefWindowProcDC;
  strcpy(desktopWindow->caption,"desktop");   //When Windows first starts, the desktop's Class is "Progman"
  cs.lpszName=desktopWindow->caption;
  cs.style=desktopWindow->style=WS_VISIBLE;   // active qua boh
  cs.x=cs.y=desktopWindow->nonClientArea.top=desktopWindow->nonClientArea.left=0;
  desktopWindow->nonClientArea.right=Screen.cx-1;
  desktopWindow->nonClientArea.bottom=Screen.cy-1;
  desktopWindow->clientArea.top=desktopWindow->nonClientArea.top;
  desktopWindow->clientArea.left=desktopWindow->nonClientArea.left; 
  desktopWindow->clientArea.right=desktopWindow->nonClientArea.right; 
  desktopWindow->clientArea.bottom=desktopWindow->nonClientArea.bottom; 
  cs.cx=desktopWindow->clientArea.right-desktopWindow->clientArea.left;
  cs.cy=desktopWindow->clientArea.bottom-desktopWindow->clientArea.top;
  SetRectEmpty(&desktopWindow->paintArea);
  desktopWindow->parent=desktopWindow->children=NULL;
  desktopWindow->cursor=standardCursor;
  desktopWindow->zOrder=0;
  desktopWindow->next=NULL;
  desktopWindow->messageQueue=NULL;
  desktopWindow->status=0; desktopWindow->visible=1; 
  desktopWindow->icon=NULL;
  desktopWindow->internalState=0;
  cs.menu=desktopWindow->menu=NULL;
  desktopWindow->scrollSizeX=desktopWindow->scrollSizeY=0;
  desktopWindow->scrollPosX=desktopWindow->scrollPosY=0;
  
  taskbarWindow=malloc(sizeof(struct _WINDOW)+4+4+1);    // v. taskbar class
  if(!taskbarWindow)
    return 0;
  cs2.class=taskbarWindow->class=MAKECLASS(WC_TASKBARCLASS);
LRESULT DefWindowProcTaskBar(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
  taskbarWindow->windowProc=(WINDOWPROC*)DefWindowProcTaskBar;
  strcpy(taskbarWindow->caption,"taskbar");   //When Windows first starts, the desktop's Class is "Progman"
  cs2.lpszName=taskbarWindow->caption;
  cs2.style=taskbarWindow->style=WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN;
  cs2.x=taskbarWindow->nonClientArea.left=0;
  cs2.y=taskbarWindow->nonClientArea.top=Screen.cy-13-1;
  taskbarWindow->nonClientArea.right=Screen.cx-1;
  taskbarWindow->nonClientArea.bottom=Screen.cy-1;
  // bah qua manca un pixel, sulla x ce n'è uno di troppo... verificare
  
  taskbarWindow->clientArea.top=taskbarWindow->nonClientArea.top;
  taskbarWindow->clientArea.left=taskbarWindow->nonClientArea.left; 
  taskbarWindow->clientArea.right=taskbarWindow->nonClientArea.right; 
  taskbarWindow->clientArea.bottom=taskbarWindow->nonClientArea.bottom; 
  cs2.cx=taskbarWindow->clientArea.right-taskbarWindow->clientArea.left;
  cs2.cy=taskbarWindow->clientArea.bottom-taskbarWindow->clientArea.top;
  SetRectEmpty(&taskbarWindow->paintArea);
  taskbarWindow->parent=taskbarWindow->children=NULL;   // o figlia di desktop?
  taskbarWindow->cursor=standardCursor;
  taskbarWindow->zOrder=  0   /* on top, v. attrib */;
  taskbarWindow->next=NULL;
  taskbarWindow->messageQueue=NULL;
  taskbarWindow->status=0; taskbarWindow->visible=1; 
  taskbarWindow->icon=NULL;
  taskbarWindow->internalState=0;
  cs2.menu=taskbarWindow->menu=NULL;
  taskbarWindow->scrollSizeX=taskbarWindow->scrollSizeY=0;
  taskbarWindow->scrollPosX=taskbarWindow->scrollPosY=0;

	switch(orientation) {
		case LANDSCAPE:
		case LANSCAPE_FLIPPED:
			break;

		case NONE:
			break;

		case PORTRAIT:
		case PORTRAIT_FLIPPED:
			break;
		}

  
#ifndef USING_SIMULATOR
  Cls();
  SetColors(WHITE,desktopColor);    // ok così, 
  DrawRectangle((Screen.cx-18*8)/2,Screen.cy/2-40,(Screen.cx+18*8)/2,Screen.cy/2+40,LIGHTGREEN);
  setTextSize(2);
  SetXY(0,(Screen.cx-12*8)/2,Screen.cy/2-30);
  print("BREAKTHROUGH");
  SetXY(0,(Screen.cx-4*8)/2,Screen.cy/2-5);
  sprintf(buf,"%u.%02u",BREAKTHROUGH_VERSION_H,BREAKTHROUGH_VERSION_L);
  print(buf);
  setTextSize(1);
  SetColors(LIGHTGRAY,desktopColor);
  SetXY(0,(Screen.cx-13*8)/2,Screen.cy/2+46);
  print("(C) 1988-2023");
  SetColors(LIGHTGRAY,desktopColor);
//  setTextSize(1); fare font + piccolo!
  SetXY(0,(Screen.cx-sizeof(BREAKTHROUGH_COPYRIGHT_STRING)*8)/2,Screen.cy/2+58);
  print(BREAKTHROUGH_COPYRIGHT_STRING);
  SetColors(BRIGHTCYAN,desktopColor);    // 
  setTextSize(2);
  sprintf(buf,"%uKB available",getTotRAM()/1024,extRAMtot/1024);
  SetXY(0,(Screen.cx-strlen(buf)*8)/2,Screen.cy/2+20);
  print(buf);
  SetXY(0,0,Screen.cy/2+90);
  SetColors(LIGHTRED,desktopColor);    // 
	if(!(GetStatus(SOUTH_BRIDGE,NULL) & 0b01000101)) 		// 
		err_puts("Keyboard not present");
	if(eXtra & 1 || !(GetStatus(SOUTH_BRIDGE,NULL) & 0b10001010)) 		// 
		err_puts("No mouse found");

#endif
  
//        m_stdout=m_stdin=m_stderr=DEVICE_WINDOWS;   // mettere, direi, dopo debug

  __delay_ms(2000);
  ClrWdt();
  mousePosition.x=Screen.cx/2; mousePosition.y=Screen.cy/2;
  mouseCursor=standardCursor;
  CreateSprite(0,8,8,mousePosition.x,mousePosition.y,1  | 8 /* trasparenza */);
  DefineSprite(0,(GFX_COLOR*)standardCursorSm);			//
  
  eXtra=GetProfileInt(profileFile,"OPZIONI","EXTRA",1);   // va bene qua (v.sopra)
  err_printf("xtra=%u\n",eXtra);
  if(eXtra & 16 && GetID(AUDIO_CARD) == 'A') {
    /// fare musichette :D
    SetAudioWave(0,1,440,0,8,70,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,480,0,8,70,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,540,0,8,80,0,0);
    __delay_ms(300);
    SetAudioWave(0,0,0,0,8,100,0,0);
    }
  
//  cs.dwExStyle=hWnd->styleEx;
  char buf1[32],*key="DESKTOP";
  BYTE sfondoTiled;
    sfondoTiled=GetProfileInt(profileFile,key,"TILED",0);
    err_printf("sfondoTiled=%u\n",sfondoTiled);
    __delay_ms(1000);
  if(GetProfileString(profileFile,key,"WALLPAPER",buf1,NULL)) {
    sfondo=buf1;
    }
  else {
    if(sfondo) {
      switch(*sfondo) {
        case '1':
          sfondo="w95train.jpg";
          break;
        case '2':
          sfondo="w95lion.jpg";
          break;
        case '3':
          sfondo="windowxp.jpg";
          break;
        case '4':
          sfondo="w3tartan.jpg";
          break;
        default:
          sfondo="prova";
          break;
        }
      }
    else {
      sfondo="DARIO";
      }
    }
  cs.lpCreateParams=(char*)sfondo;
  SetWindowByte(desktopWindow,2+2+sizeof(FONT)+16+sizeof(S_POINT)*16,sfondoTiled);  //v. cmq Create..
  if(!SendMessage(desktopWindow,WM_NCCREATE,0,(LPARAM)&cs))
    return 0;
  if(SendMessage(desktopWindow,WM_CREATE,0,(LPARAM)&cs) < 0)
    return 0;

  cs2.lpCreateParams=(void*)1;    // 1=orologio, 2=on top
  if(!SendMessage(taskbarWindow,WM_NCCREATE,0,(LPARAM)&cs2))
    return 0;
  if(SendMessage(taskbarWindow,WM_CREATE,0,(LPARAM)&cs2) < 0)
    return 0;

#ifndef USING_SIMULATOR
  PaintDesktop(NULL,0);
//  InvalidateRect(desktopWindow,NULL,TRUE);
#endif
  ClrWdt();
  // oppure 
  // rootWindows=desktopWindow=CreateWindow(WC_DESKTOPCLASS,NULL,WS_ACTIVE | WS_VISIBLE,0,0,_width,_height,NULL,NULL,0);
  // e anche
  // taskbarWindow=CreateWindow(WC_TASKBARCLASS,"",WS_ACTIVE | WS_VISIBLE,0,_height-_height/16,_width,_height/16,NULL,NULL,0);
  // NON faccio Create per non inserire nella lista windows!

  DrawCursor(mousePosition.x,mousePosition.y,standardCursor,0);

	winManagerThreadID=BeginThread((void *)manageWindows);

  Yield(winManagerThreadID);

  }

void EndWindows(void) {
  HWND w;
  WNDCLASS *c;
  BYTE i;
  
  if(eXtra & 16 && GetID(AUDIO_CARD) == 'A') {
    /// fare musichette :D
    SetAudioWave(0,1,440,0,8,80,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,340,0,8,70,0,0);
    __delay_ms(300);
    SetAudioWave(0,1,280,0,8,60,0,0);
    __delay_ms(300);
    SetAudioWave(0,0,0,0,8,100,0,0);
    }
  
  CreateSprite(0,0,0,0,0,0);
  for(i=0; i<MAX_TIMERS; i++) {
    timerData[i].uEvent=0;
    timerData[i].hWnd=0;
    timerData[i].timeout=0;
    timerData[i].tproc=NULL;
    timerData[i].time_cnt=0;
    }
  if(desktopWindow) {
    while(rootWindows) {
      w=rootWindows->next;
      free(rootWindows);
      rootWindows=w;
      }
    rootWindows=NULL;
    while(rootClasses) {
      c=rootClasses->next;
      free(rootClasses);
      rootClasses=c;
      }
    rootClasses=NULL;
    free(taskbarWindow); taskbarWindow=NULL;
    free(desktopWindow); desktopWindow=NULL;
    }
  EndThread(winManagerThreadID);
  }

BOOL IsWindowUnicode(HWND hWnd) { return FALSE; }   //;)

GFX_COLOR GetSysColor(int nIndex) {
  
  switch(nIndex) {
    case COLOR_ACTIVECAPTION:
    case COLOR_ACTIVEBORDER:
    case COLOR_BTNFACE:
    case COLOR_CAPTIONTEXT:
    case COLOR_BTNTEXT:
    case COLOR_MENU:
    case COLOR_MENUBAR:
    case COLOR_SCROLLBAR:
    case COLOR_WINDOWFRAME:
    case COLOR_WINDOWTEXT:
      return windowForeColor;
      break;
    case COLOR_GRAYTEXT:
    case COLOR_INACTIVEBORDER:
    case COLOR_INACTIVECAPTION:
    case COLOR_INACTIVECAPTIONTEXT:
      return windowInactiveForeColor;
      break;
    case COLOR_WINDOW:
      return windowBackColor;
      break;
//    case COLOR_BACKGROUND:
    case COLOR_DESKTOP:
      return desktopColor;
      break;
    }
  }

GFX_COLOR GetSystemMetrics(int nIndex) {
  
  switch(nIndex) {
    case SM_CXSCREEN:
      return Screen.cx;
      break;
    case SM_CYSCREEN:
      return Screen.cy;
    case SM_CXVSCROLL:
      return SCROLL_SIZE;
      break;
    case SM_CYHSCROLL:
      return SCROLL_SIZE;
      break;
    case SM_CYCAPTION:
      return TITLE_HEIGHT;
      break;
    case SM_CXBORDER:
      return 1;
      break;
    case SM_CYBORDER:
      return 1;
      break;
    case SM_CYVTHUMB:
      break;
    case SM_CXHTHUMB:
      break;
    case SM_CXICON:
      return 8;
      break;
    case SM_CYICON:
      return 8;
      break;
    case SM_CXCURSOR:
      return 8;
      break;
    case SM_CYCURSOR:
      return 8;
      break;
    case SM_CYMENU:
      return MENU_HEIGHT;
      break;
    case SM_CXFULLSCREEN:
      return Screen.cx;
      break;
    case SM_CYFULLSCREEN:
      return Screen.cy;
      break;
    case SM_CYKANJIWINDOW:
      return FALSE;
      break;
    case SM_MOUSEPRESENT:
      return TRUE;
      break;
    case SM_CYVSCROLL:
      break;
    case SM_CXHSCROLL:
      break;
    case SM_DEBUG:
#ifdef _DEBUG
      return TRUE;
#else
      return FALSE;
#endif
      break;
    case SM_SWAPBUTTON:
      return FALSE;
      break;
    case SM_RESERVED1:
      return 0;
      break;
    case SM_RESERVED2:
      return 0;
      break;
    case SM_RESERVED3:
      return 0;
      break;
    case SM_RESERVED4:
      return 0;
      break;
    case SM_CXMIN:
      break;
    case SM_CYMIN:
      break;
    case SM_CXSIZE:
      break;
    case SM_CYSIZE:
      break;
//    case SM_CXDLGFRAME:
//    case SM_CXSIZEFRAME:
    case SM_CXFRAME:
      return 2;
      break;
//    case SM_CYDLGFRAME:
//    case SM_CYSIZEFRAME:
    case SM_CYFRAME:
      return 2;
      break;
    case SM_CXMINTRACK:
      break;
    case SM_CYMINTRACK:
      break;
    case SM_CXDOUBLECLK:
      break;
    case SM_CYDOUBLECLK:
      break;
    case SM_CXICONSPACING:
      break;
    case SM_CYICONSPACING:
      break;
    case SM_MENUDROPALIGNMENT:
      break;
    case SM_PENWINDOWS:
    case SM_TABLETPC:
      return FALSE;
      break;
    case SM_DBCSENABLED:
      return FALSE;
      break;
    case SM_CMOUSEBUTTONS:
      return 2;
      break;
    case SM_CXFIXEDFRAME:
      break;
    case SM_CYFIXEDFRAME:
      break;
    case SM_SECURE:
      return FALSE;
      break;
    case SM_CXEDGE:
      break;
    case SM_CYEDGE:
      break;
    case SM_CXMINSPACING:
      break;
    case SM_CYMINSPACING:
      break;
    case SM_CXSMICON:
      return 8;
      break;
    case SM_CYSMICON:
      return 8;
      break;
    case SM_CYSMCAPTION:
      break;
    case SM_CXSMSIZE:
      break;
    case SM_CYSMSIZE:
      break;
    case SM_CXMENUSIZE:
      break;
    case SM_CYMENUSIZE:
      break;
    case SM_ARRANGE:
      break;
    case SM_CXMINIMIZED:
      break;
    case SM_CYMINIMIZED:
      break;
    case SM_CXMAXTRACK:
      //boh?
      break;
    case SM_CYMAXTRACK:
      break;
    case SM_CXMAXIMIZED:
      return Screen.cx;
      break;
    case SM_CYMAXIMIZED:
      return Screen.cy;
      break;
    case SM_NETWORK:
      return TRUE;
      return FALSE;
      break;
    case SM_CLEANBOOT:
#ifdef _DEBUG
      return TRUE;
#else
      return FALSE;
#endif
      break;
    case SM_CXDRAG:
      break;
    case SM_CYDRAG:
      break;
    case SM_SHOWSOUNDS:
      return FALSE;
      break;
    case SM_CXMENUCHECK:
      return 8;   // fisso...
      break;
    case SM_CYMENUCHECK:
      return 8;
      break;
    case SM_SLOWMACHINE:
      return FALSE; //:D suca
      break;
    case SM_MIDEASTENABLED:
      return FALSE;   // porcoallah :D :D
      break;
    case SM_MOUSEWHEELPRESENT:
      return FALSE;
      break;
    case SM_XVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_YVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_CXVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_CYVIRTUALSCREEN:
      return FALSE;
      break;
    case SM_CMONITORS:
      return 1;
      break;
    case SM_SAMEDISPLAYFORMAT:
      return FALSE;
      break;
    case SM_CMETRICS:
      return ;
      break;
    }
  }

HWND GetDesktopWindow(void) { return desktopWindow; }
HWND GetShellWindow(void) { return taskbarWindow; }   // o forse è sempre il desktop? https://stackoverflow.com/questions/8364758/get-handle-to-desktop-shell-window
HWND GetRootWindow(void) { return rootWindows; }    // uso interno...

int GetDeviceCaps(HDC hDC,int index) {
  
  switch(index) {
    case DRIVERVERSION:
      return MAKEWORD(BREAKTHROUGH_VERSION_L,BREAKTHROUGH_VERSION_H);
      break;
    case TECHNOLOGY:
      break;
    case HORZSIZE:
    case HORZRES:
      return Screen.cy;
      break;
    case VERTRES:
    case VERTSIZE:
      return Screen.cx;
      break;
    case BITSPIXEL:
      return 16;
      break;
    case PLANES:
      return 3;
      break;
    case NUMBRUSHES:
      return 1;
      break;
    case NUMPENS:
      return 1;
      break;
    case NUMMARKERS:
      return 0;
      break;
    case NUMFONTS:
//      #ifdef USE_CUSTOM_FONTS 
        return 5;
//      #else
//        return 2;
//      #endif
      break;
    case NUMCOLORS:
      return 65536L;
      break;
    case CURVECAPS:
      return 1;
      break;
    case LINECAPS:
      return 1;
      break;
    case POLYGONALCAPS:
      return 1;
      break;
    case TEXTCAPS:
      return 1;
      break;
    case CLIPCAPS:
      return 1;
      break;
    case RASTERCAPS:
      return 1;
      break;
    case VREFRESH:
      return 50;
      break;
      
    default:
      break;
    }
  }

BOOL SetCursorPos(int x,int y) {

  mousePosition.x=x; mousePosition.y=y;
  }
BOOL GetCursorPos(POINT *lpPoint) {
  POINT pt;
  pt.x=mousePosition.x;
  pt.y=mousePosition.y;
  *lpPoint=pt;
  }
CURSOR SetCursor(CURSOR hCursor) {
  
  mouseCursor=hCursor;
  DefineSprite(0,(GFX_COLOR*)mouseCursor);			// questa incasina il video: strano, in effetti non fa nulla (v. video( 28/9/22
  }
BOOL SetCaretPos(int X,int Y) {
  
  }
BOOL SetCaretBlinkTime(UINT uMSeconds) {
  }
BOOL HideCaret(HWND hWnd) {
  }

HWND CreateWindow(CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,MENU *Menu,void *lpParam) {
  HWND hWnd;
  WNDCLASS *wndClass;

  if(!GetClassInfo(0,Class,&wndClass))
    return NULL;

  if(wndClass->style & CS_OWNDC)
    hWnd=malloc(sizeof(struct _WINDOW)+wndClass->cbWndExtra+sizeof(HDC));
  else
    hWnd=malloc(sizeof(struct _WINDOW)+wndClass->cbWndExtra);
  if(!hWnd)
    return NULL;
  hWnd->class=Class;
  hWnd->next=NULL;
  hWnd->status=0;
  hWnd->nonClientArea.top=Y;
  hWnd->nonClientArea.left=X;
  if(wndClass->style & CS_BYTEALIGNWINDOW) {
    nWidth = (nWidth + 7) & 0xfff8;   // boh credo :)
    }
  hWnd->nonClientArea.bottom=Y+nHeight;
  hWnd->nonClientArea.right=X+nWidth;
  hWnd->icon=wndClass->icon;
  hWnd->cursor=wndClass->cursor;
  hWnd->windowProc=wndClass->lpfnWndProc;
  if(lpWindowName) {
    strncpy(hWnd->caption,lpWindowName,sizeof(hWnd->caption)-1);
    hWnd->caption[sizeof(hWnd->caption)-1]=0;
    }
  else
    hWnd->caption[0]=0;
  hWnd->style=dwStyle;    // 
//  hWnd->styleEx=0;
  hWnd->menu=Menu ? Menu : wndClass->menu;
  hWnd->children=NULL;
  if(hWndParent) {
    hWnd->parent=hWndParent->style & WS_CHILD ? hWndParent->parent : hWndParent; // andrebbe fatto ricorsivo a salire..
//    hWnd->parent=hWndParent; // però mi incasino! FINIRE!
    }
  if(dwStyle & WS_CHILD) {    // safety
		dwStyle &= ~(WS_EX_TOPMOST /*| WS_ACTIVE mah, e poi children??*/);
    hWnd->topmost=0;
    }
  
  { CREATESTRUCT cs;
  cs.x=hWnd->nonClientArea.left;
  cs.y=hWnd->nonClientArea.top;
  cs.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  cs.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
  cs.class=Class;
  cs.lpszName=hWnd->caption;
  cs.menu=hWnd->menu;
  cs.style=hWnd->style;
//  cs.dwExStyle=hWnd->styleEx;
  cs.lpCreateParams=lpParam;
  if(!SendMessage(hWnd,WM_NCCREATE,0,(LPARAM)&cs))
    goto no_creata;
  if(SendMessage(hWnd,WM_CREATE,0,(LPARAM)&cs) < 0) {
no_creata:
    free(hWnd);
    return NULL;
    }
  else {
    hWnd->style=cs.style;
    hWnd->menu=cs.menu;
    hWnd->nonClientArea.left=cs.x;
    hWnd->nonClientArea.top=cs.y;
    hWnd->nonClientArea.right=hWnd->nonClientArea.left+cs.cx;
    hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+cs.cy;
    }
  }
  hWnd->scrollPosX=hWnd->scrollPosY=0; hWnd->scrollSizeX=hWnd->scrollSizeY=0;
  calculateNonClientArea(hWnd,&hWnd->nonClientArea);
  calculateClientArea(hWnd,&hWnd->clientArea);
  hWnd->enabled=hWnd->style & WS_DISABLED ? 0 : 1;
  if(!(hWnd->style & WS_CHILD)) {
    if(hWnd->enabled) {
      HWND myWnd=GetForegroundWindow();
      hWnd->zOrder= myWnd ? myWnd->zOrder+1 : 1;
      addTailWindow(hWnd,&rootWindows);
      }
    else {
      hWnd->zOrder=1;
      addHeadWindow(hWnd,&rootWindows);    // così rispettiamo lo Z-order subito!
      }
    }
  else {
    HWND myWnd=getLastChildWindow(hWnd);
    hWnd->zOrder= myWnd ? myWnd->zOrder+1 : 1;
    addTailWindow(hWnd,&hWnd->parent->children);
    }
  if(hWnd->style & WS_EX_TOPMOST)
    hWnd->topmost=1;
  if(!(hWnd->style & WS_CHILD))   // direi
    setActive(hWnd,hWnd->style & WS_DISABLED ? 0 : 1);
  
  if(hWnd->style & WS_VISIBLE) {
    hWnd->visible=1;
#ifndef USING_SIMULATOR
    if(!(hWnd->style & WS_CHILD)) {
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
      InvalidateRect(hWnd,NULL,TRUE);
      }
    else {
    //if(!WS_EX_NOPARENTNOTIFY) mettere
      SendMessage(hWnd->parent,WM_PARENTNOTIFY,MAKEWORD(WM_CREATE,(WORD)(DWORD)hWnd->menu),(LPARAM)hWnd);   // tranne WS_EX_NOPARENTNOTIFY  ..
      InvalidateRect(hWnd->parent,NULL,FALSE /*TRUE*/);    // direi così MA FORSE SI POTREBBE SPOSTARE NEL PARENTNORIFY??
//      SendMessage(hWnd->parent,WM_PAINT,0,0);   // direi così
      }
#endif
    }

  return hWnd;
  }
HWND CreateWindowEx(DWORD dwExStyle,CLASS Class,const char *lpWindowName,
  DWORD dwStyle,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hWndParent,MENU *Menu,void *lpParam) {   // fare :)
  HWND w=CreateWindow(Class,lpWindowName,dwStyle,X,Y,nWidth,nHeight,
    hWndParent,Menu,lpParam);
  if(w)
    ;     // w->dwExStyle=dwExStyle;
  return w;
  }

static BOOL addTailWindow(HWND hWnd,struct _WINDOW **root) {   // per insertSorted: https://www.geeksforgeeks.org/given-a-linked-list-which-is-sorted-how-will-you-insert-in-sorted-way/
  HWND myWnd;
  
  if(!*root)
    *root=hWnd;
  else {
    myWnd=*root;
    while(myWnd && myWnd->next) {
      myWnd=myWnd->next;
      }
    myWnd->next=hWnd;
    }
  return 1;
  }
static BOOL addHeadWindow(HWND hWnd,struct _WINDOW **root) {
  HWND myWnd;
  
  if(!*root)
    *root=hWnd;
  else {
    myWnd=hWnd->next=*root;
    *root=hWnd;
    (*root)->zOrder=1;    // questa diventa la più indietro...
    while(myWnd) {
      myWnd->zOrder++;    // ...e le scrollo tutte avanti di uno
      myWnd=myWnd->next;
      }
    }
  return 1;
  }
static BOOL insertWindow(HWND hWnd,HWND hWndAfter,struct _WINDOW **root) {
  HWND myWnd,myWnd2;
  
  removeWindow(hWnd,root);
  myWnd=*root;
  while(myWnd) {
    if(myWnd == hWndAfter) {
      myWnd2=hWndAfter->next;
      hWndAfter->next=hWnd;
      hWnd->next=myWnd2;
      return 1;
      }
    myWnd=myWnd->next;
    }
  return 0;
  }
static BOOL removeWindow(HWND hWnd,struct _WINDOW **root) {
  HWND myWnd,myWnd2;
  // https://stackoverflow.com/questions/47491406/how-to-delete-a-node-in-a-linked-list

  if(hWnd==*root) {
    *root=hWnd->next;
    myWnd=*root;
		goto riordina;
		}

  myWnd=*root;
  while(myWnd != hWnd) {
    myWnd2=myWnd;
    myWnd=myWnd->next;
		}
	if(myWnd2) {
	  myWnd2->next=myWnd->next;
		myWnd=myWnd2->next;

riordina:
		while(myWnd) {
			myWnd->zOrder--;
			myWnd=myWnd->next;
			}
	  return 1;
		}

  return 0;
  }

LRESULT SendMessage(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  HWND myWnd;

	if(hWnd==HWND_BROADCAST) {
    myWnd=rootWindows;
    while(myWnd) {
			myWnd->windowProc(myWnd,message,wParam,lParam);
      myWnd=myWnd->next;
      }
    if(taskbarWindow)   // mah, diciamo.. NON dovrebbe essere coperto, ma anche solo per aggiornare le icone
      // o controllare ON TOP?? in generale
			taskbarWindow->windowProc(myWnd,message,wParam,lParam);   
    }
	else
		return hWnd->windowProc(hWnd,message,wParam,lParam);   // per ora così :)
  }
void PostQuitMessage(int nExitCode) {
//  EndThread();
  }
LRESULT DefWindowProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_NCPAINT:
      //wParam = region
      return nonClientPaint(hWnd,(RECT *)wParam);
      break;
    case WM_PAINT:
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_SYNCPAINT:
      SendMessage(hWnd,WM_NCPAINT,0,0);
      SendMessage(hWnd,WM_PAINT,0,0);
      break;
    case WM_SETREDRAW:
      if(wParam)      // gestire blocco/sblocco redraw...
        ;
      break;
    case WM_CTLCOLOR:   // è deprecated ma per ora lo uso!
      return 0xffffffff;   // 
      break;
    case WM_PARENTNOTIFY:   // dovrebbe essere ricorsiva a salire, dice...
      switch(LOWORD(wParam)) {
        case WM_CREATE:
          break;
        case WM_DESTROY:
          break;
        case WM_LBUTTONDOWN:
          break;
        case WM_RBUTTONDOWN:
          break;
        case WM_MBUTTONDOWN:
          break;
        }
      break;
    case WM_SETICON:
      hWnd->icon=(const GFX_COLOR *)lParam;
      break;
    case WM_SETFONT:
      break;
    case WM_GETFONT:
      break;
    case WM_ERASEBKGND:   // http://www.catch22.net/tuts/win32/flicker-free-drawing#
//     If hbrBackground is NULL, the application should process the WM_ERASEBKGND 
      { RECT rc;
        HDC *hDC=(HDC *)wParam;
        int c=hDC->brush.color;
//      hWnd->paintArea=hWnd->clientArea;
        
        if(!hDC->brush.size && hDC->brush.style==BS_NULL)   // If hbrBackground is NULL, the application should process the WM_ERASEBKGND 
          return 0;
        if(hWnd->style & WS_CHILD) {
          if((c=SendMessage(hWnd->parent,WM_CTLCOLOR,(WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
            c=hDC->brush.color;
            }
          }

        if(1 /*hWnd->hDC.brush*/)   // qua, sempre..
          {
          GetClientRect(hWnd,&rc);
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
          return 1;
          }
        else
          return 0;
      }
      break;
    case WM_KEYDOWN:
      break;
    case WM_KEYUP:
      break;
    case WM_CHAR:
      {char buf[8];    // TOGLIERE POI! :D
      int i;
      if(hWnd->enabled) {
//        buf[0]=wParam; buf[1]=0;
//      TextOut(&hWnd->hDC,hWnd->hDC.cursor.x,hWnd->hDC.cursor.y,buf);
      
        switch(wParam) {
          case VK_SPACE:
            if(lParam & 0x20000000)   // ALT
              SendMessage(hWnd,WM_INITMENUPOPUP,0,MAKELONG(0,1));
            break;
          case VK_ESCAPE:
            if(activeMenu)   // 
              SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)activeMenu,
                MAKELONG(0,activeMenu==&systemMenu /*bah :D */ ? MF_SYSMENU : 0));
            break;
          case VK_RIGHT:
            if(activeMenu)   // 
              ;
            break;
          case VK_LEFT:
            if(activeMenu)   // 
              ;
            break;
          default:
						if(GetAsyncKeyState(VK_CONTROL)) {
							if(hWnd->menu) {
                if(i=matchAccelerator(hWnd->menu,LOBYTE(wParam)))
 									SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),MAKELONG(0,0));
								}
							}
						else {
							if(hWnd->parent)
								return DefWindowProc(hWnd->parent,message,wParam,lParam);
							}
            break;
          }
        }
      }
      return 0;
      break;
    case WM_SYSCHAR:
      if(hWnd->enabled) {
        }
      else {
        if(hWnd->parent)
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      break;
    case WM_NCMOUSEMOVE:
      break;
    case WM_MOUSEMOVE:
      break;
    case WM_LBUTTONDOWN:
      if(hWnd->enabled) {
        }
      else {
        if(hWnd->parent)
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      break;
    case WM_MOUSELEAVE:
      break;
    case WM_SETCURSOR:
      // wparam=handle to window, L-lparam=hittest H-lparam=wm_mousemove o altro
      // impostare cursore a class-cursor se in client area
      return (LRESULT)hWnd->cursor;
      break;
    case WM_TIMER:
      break;
    case WM_HSCROLL:
      break;
    case WM_VSCROLL:
      break;
    case WM_GETMINMAXINFO:
    { MINMAXINFO *mmi=(MINMAXINFO *)lParam;
      mmi->ptMaxSize.x=Screen.cx; mmi->ptMaxSize.y=Screen.cy;
      mmi->ptMaxPosition.x=0; mmi->ptMaxPosition.y=Screen.cy;
      mmi->ptMinTrackSize.x=GetSystemMetrics(SM_CXMINTRACK);
      mmi->ptMinTrackSize.y=GetSystemMetrics(SM_CYMINTRACK);
      mmi->ptMaxTrackSize.x=GetSystemMetrics(SM_CXMAXTRACK);
      mmi->ptMaxTrackSize.y=GetSystemMetrics(SM_CYMAXTRACK);
    }
      break;
    case WM_NCCALCSIZE:
      if(wParam) {
        struct NCCALCSIZE_PARAMS *ncp=(struct NCCALCSIZE_PARAMS *)lParam;
        }
      else {
        RECT *rc=(RECT *)lParam;
        calculateNonClientArea(hWnd,rc);
        calculateClientArea(hWnd,rc);
        }
      break;
    case WM_NCHITTEST:    //https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-nchittest
      {POINT pt;
      pt.x=LOWORD(lParam); pt.y=HIWORD(lParam);
      if(hWnd->parent) {
        pt.x -= hWnd->parent->clientArea.left; pt.y -= hWnd->parent->clientArea.top;
/*          printf("children: pt=%u,%u / %u,%u,%u,%u\r\n",pt.x,pt.y,
                  hWnd->nonClientArea.left ,hWnd->nonClientArea.top,
            hWnd->nonClientArea.right ,hWnd->nonClientArea.bottom);*/
        if(hWnd->parent->parent) {// per ora 2, fare ricorsivo? v. anche WindowFromPoint
          pt.x -= hWnd->parent->parent->clientArea.left; pt.y -= hWnd->parent->parent->clientArea.top;
/*          printf("children-children: pt=%u,%u / %u,%u,%u,%u\r\n",pt.x,pt.y,
                  hWnd->nonClientArea.left ,hWnd->nonClientArea.top,
            hWnd->nonClientArea.right ,hWnd->nonClientArea.bottom);*/
          }
        }
      if(hWnd->minimized) {
        if(pt.x>=hWnd->nonClientArea.left && pt.x<=hWnd->nonClientArea.right &&
          pt.y>=hWnd->nonClientArea.top && pt.y<=hWnd->nonClientArea.bottom) {
          return HTCLIENT;
          }
        else {
          return HTNOWHERE;
          }
        }
      else {
        if(pt.x>=hWnd->nonClientArea.left && pt.y>=hWnd->nonClientArea.top) {
          if(pt.y==hWnd->nonClientArea.top) {
            if(pt.x==hWnd->nonClientArea.left)
              return HTTOPLEFT;
            else if(pt.x==hWnd->nonClientArea.right)
              return HTTOPRIGHT;
            else
              return HTTOP;
            }
          else if(pt.y<hWnd->nonClientArea.bottom) {
            if(pt.x<=hWnd->nonClientArea.left+BORDER_SIZE) {  // gestire thick
              return hWnd->style & WS_THICKFRAME ? HTLEFT : HTBORDER;   // solo se resizable...
              }
            else if(pt.x>=hWnd->nonClientArea.right-BORDER_SIZE) {  // gestire thick
              return hWnd->style & WS_THICKFRAME ? HTRIGHT : HTBORDER;   // solo se resizable...
              }
            else if(pt.y<=hWnd->nonClientArea.top+TITLE_HEIGHT) {
              if(hWnd->style & WS_CAPTION) {
                if(pt.x>=hWnd->nonClientArea.left && pt.x<=hWnd->nonClientArea.left+TITLE_ICON_WIDTH) {
                  return hWnd->style & WS_SYSMENU ? HTSYSMENU : HTCAPTION;
                  }
                else if(pt.x>=hWnd->nonClientArea.right-TITLE_ICON_WIDTH) {
                  if(hWnd->style & WS_SYSMENU)
                    return HTCLOSE;
                  else if(hWnd->style & WS_MAXIMIZEBOX)
                    return HTMAXBUTTON;
                  else
                    return HTCAPTION;
                  }
                else if(pt.x>=(hWnd->nonClientArea.right-TITLE_ICON_WIDTH*2+1)) {
                  if(hWnd->style & WS_MAXIMIZEBOX)
                    return HTMAXBUTTON;
                  else if(hWnd->style & WS_MINIMIZEBOX)
                    return HTMINBUTTON;
                  else
                    return HTCAPTION;
                  }
                else if(pt.x>=(hWnd->nonClientArea.right-TITLE_ICON_WIDTH*3+2)) {
                  if(hWnd->style & WS_MINIMIZEBOX)
                    return HTMINBUTTON;
                  else
                    return HTCAPTION;
                  }
                else {
                  return HTCAPTION;
                  }
                }
              else if(hWnd->menu) {
                goto ismenu;
                }
              else {
                return HTCLIENT;
                }
              }
            else if(hWnd->menu && pt.y<=hWnd->nonClientArea.top+TITLE_HEIGHT+MENU_HEIGHT) {
ismenu:
  // finire.. .QUALE menu
              if(activeMenu)
                ;
              //else return HTCLIENT;

              if(!(hWnd->style & WS_CHILD))
                return HTMENU ;     // FINIRE!
              }
            else if(pt.y<=hWnd->nonClientArea.bottom-SCROLL_SIZE) {
              if(pt.x>=hWnd->nonClientArea.right-SCROLL_SIZE)
                return hWnd->style & WS_HSCROLL ? HTHSCROLL : HTCLIENT;
              else
                return HTCLIENT;
              }
            else if(pt.y<=hWnd->nonClientArea.bottom-BORDER_SIZE) {
              if(hWnd->style & WS_SIZEBOX) {
                if(pt.x>=hWnd->nonClientArea.right-SCROLL_SIZE)
                  return HTSIZE;
                else
                  return hWnd->style & WS_HSCROLL ? HTHSCROLL : HTCLIENT;
                }
              else {
                return hWnd->style & WS_VSCROLL ? HTVSCROLL : HTCLIENT;
                }
              }
            else if(pt.x<hWnd->nonClientArea.right-BORDER_SIZE) {
              return HTCLIENT;
              }
            }
          else if(pt.y==hWnd->nonClientArea.bottom) {
            if(pt.x==hWnd->nonClientArea.left+BORDER_SIZE) {  // gestire thick
              return hWnd->style & WS_THICKFRAME ? HTBOTTOMLEFT : HTBOTTOM;   // solo se resizable...
              }
            else if(pt.x==hWnd->nonClientArea.right-BORDER_SIZE) {  // gestire thick
              return hWnd->style & WS_THICKFRAME ? HTBOTTOMRIGHT : HTBOTTOM;   // solo se resizable...
              }
            else {
              return hWnd->style & WS_THICKFRAME ? HTBOTTOM : HTBORDER;   // solo se resizable...
              }
            }
          }
        else {
          return HTNOWHERE;
          }
        }
/*      
HTBORDER 18 //	In the border of a window that does not have a sizing border.
HTBOTTOM 15 //	In the lower-horizontal border of a resizable window (the user can click the mouse to resize the window vertically).
HTBOTTOMLEFT 16 //	In the lower-left corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
HTBOTTOMRIGHT 17 //	In the lower-right corner of a border of a resizable window (the user can click the mouse to resize the window diagonally).
HTCAPTION 2	//In a title bar.
HTCLIENT 1 //	In a client area.
HTCLOSE 20 //In a Close button.
HTERROR -2 //	On the screen background or on a dividing line between windows (same as HTNOWHERE, except that the DefWindowProc function produces a system beep to indicate an error).
HTGROWBOX 4 //	In a size box (same as HTSIZE).
HTHELP 21 //	In a Help button.
HTHSCROLL 6 //	In a horizontal scroll bar.
HTLEFT 10 //	In the left border of a resizable window (the user can click the mouse to resize the window horizontally).
HTMENU 5 //	In a menu.
HTMAXBUTTON 9 //	In a Maximize button.
HTMINBUTTON 8 //	In a Minimize button.
HTNOWHERE 0 //	On the screen background or on a dividing line between windows.
HTREDUCE 8 //	In a Minimize button.
HTRIGHT 11 //	In the right border of a resizable window (the user can click the mouse to resize the window horizontally).
HTSIZE 4 //	In a size box (same as HTGROWBOX).
HTSYSMENU 3 //	In a window menu or in a Close button in a child window.
HTTOP 12 //	In the upper-horizontal border of a window.
HTTOPLEFT 13 //	In the upper-left corner of a window border.
HTTOPRIGHT 14 //	In the upper-right corner of a window border.
HTTRANSPARENT -1 //	In a window currently covered by another window in the same thread (the message will be sent to underlying windows in the same thread until one of them returns a code that is not HTTRANSPARENT).
HTVSCROLL 7 //	In the vertical scroll bar.
HTZOOM 9 //	In a Maximize button
 * */
      return HTNOWHERE;
      }
      break;
    case WM_NCCREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      }
      return 0;
      break;
    case WM_NCDESTROY:
      return 0;
      break;
    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;
      break;
    case WM_MOVE:
      //xPos = (int)(short) LOWORD(lParam);   // horizontal position 
      //yPos = (int)(short) HIWORD(lParam);   // vertical position 
      activateChildren(hWnd);
      return 0;
      break;
//    case WM_MOVING:
//      break;
    case WM_SIZE:
      switch(wParam) {
        case SIZE_MAXHIDE:
          break;
        case SIZE_MAXIMIZED:
          break;
        case SIZE_MAXSHOW:
          break;
        case SIZE_MINIMIZED:
          break;
        case SIZE_RESTORED:
          break;
        }
      //UINT width = LOWORD(lParam);
      //UINT height = HIWORD(lParam);
      activateChildren(hWnd);
      return 0;
      break;
//    case WM_SIZING:
//      break;
    case WM_CLOSE:
      DestroyWindow(hWnd);
      break;
    case WM_NCACTIVATE:
      // https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-ncactivate
      return 1;   // per ora così
      break;
    case WM_ACTIVATE:
//   		setActive(hWnd,wParam == WA_ACTIVE);
//   	  hWnd->active=wParam == WA_ACTIVE;
      return 0;
      break;
    case WM_MOUSEACTIVATE:
//mah..      if(!hWnd->active) {
      if(hWnd->enabled) {
        ShowWindow(hWnd,SW_SHOWNORMAL);
        if(wParam)
          SendMessage((HWND)wParam,WM_MOUSEACTIVATE,(WPARAM)((HWND)wParam)->parent,lParam);
        }
      else {
        if(hWnd->parent)
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      return MA_ACTIVATE;
      break;
    case WM_CHILDACTIVATE:
      return 0;
      break;
    case WM_DISPLAYCHANGE:
      break;
    case WM_PAINTICON:    // solo windows 3.x :)
      break;
    case WM_QUERYOPEN:
			return 1;
      break;
    case WM_SHOWWINDOW:
			return 0;
      break;
    case WM_WINDOWPOSCHANGING:
      return 0;
      break;
    case WM_WINDOWPOSCHANGED:
      return 0;
      break;
    case WM_EXITSIZEMOVE:
      break;
    case WM_SETFOCUS:
      hWnd->focus=1;
      break;
    case WM_KILLFOCUS:
      hWnd->focus=0;
      break;
    case WM_SETTEXT:
    {
      RECT rc;
      
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      rc.left=hWnd->nonClientArea.left;
      rc.right=hWnd->nonClientArea.right;
      rc.top=hWnd->nonClientArea.top;
      rc.bottom=hWnd->nonClientArea.top+TITLE_HEIGHT;
      return SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
    }
      break;
    case WM_GETTEXT:
      {
      int i=min(sizeof(hWnd->caption),wParam);
      strncpy((char *)lParam,hWnd->caption,i-1);
      ((char *)lParam)[i]=0;
      return i;
      }
      break;
    case WM_GETTEXTLENGTH:
      return strlen(hWnd->caption);
      break;
    case WM_ENABLE:
      hWnd->enabled=wParam;
      // c'entra con active? https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-enablewindow
      if(!hWnd->enabled) {
    		if(!(hWnd->style & WS_CHILD))
          setActive(hWnd,FALSE);
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
/*        RECT rc;
        rc.left=hWnd->nonClientArea.left;
        rc.right=hWnd->nonClientArea.right;
        rc.top=hWnd->nonClientArea.top;
        rc.bottom=hWnd->nonClientArea.top+TITLE_HEIGHT; */
        InvalidateRect(hWnd,NULL,TRUE);   // se NCPAINT non pulisse il rettangolone, si potrebbe togliere...
				}
      break;
    case WM_STYLECHANGED:
      break;
    case WM_STYLECHANGING:
      break;
      
    case WM_NOTIFY:
      break;
    case WM_INITDIALOG:
      break;
    case WM_INITMENU:
      {MENU *m=(MENU*)wParam;
      if(m->menuItems[0].flags & MF_POPUP) {		// finire ...
				m=m->menuItems[0].menu;		//finire
				if(m) {
					if(hWnd->active) {   // direi ovvio cmq!
						drawMenuPopup(hWnd,m,0,TITLE_HEIGHT+MENU_HEIGHT+1);    // finire
						activeMenu=m;
						SendMessage(hWnd,WM_ENTERIDLE,MSGF_MENU,(LPARAM)activeMenu);   //FINIRE, gestire
  					}
					}
				}
			else {
				}		// finire ...
      }
      return 0;
      break;
    case WM_INITMENUPOPUP:
      {MENU *m=(MENU*)wParam;
      if(hWnd->active) {    // direi ovvio cmq!
        if(HIWORD(lParam)) {   // in LOWORD l'indice del menu
          if(hWnd->style & WS_SYSMENU) {    // mah sì, e quindi eccezione??
            m=(MENU*)&systemMenu;   //bah per ora sì
            drawMenuPopup(hWnd,m,0,TITLE_HEIGHT);
            activeMenu=m;
            SendMessage(hWnd,WM_ENTERIDLE,MSGF_MENU,(LPARAM)activeMenu);   //FINIRE, gestire
            }
          }
        else {
          // forse lParam dovrebbe contenere la Posizione come 0..1..2 e non la X, ma per ora faccio così!
          drawMenuPopup(hWnd,m,LOWORD(lParam),TITLE_HEIGHT+MENU_HEIGHT+1);
          activeMenu=m;
          SendMessage(hWnd,WM_ENTERIDLE,MSGF_MENU,(LPARAM)activeMenu);   //FINIRE, gestire
          }
        }
      }
      return 0;
      break;
    case WM_UNINITMENUPOPUP:
      {MENU *m=(MENU*)wParam;
      if(activeMenu) {
        RECT rc;
        //getMenuArea(m,&rc);
        if(HIWORD(lParam) == MF_SYSMENU)
          DrawMenuBar(hWnd);  // 
        InvalidateRect(hWnd,/*rc*/ NULL,TRUE);   // per ora così :)
        activeMenu=NULL;
        }
      
      
        InvalidateRect(hWnd,/*rc*/ NULL,TRUE);   // per ora così :)
#warning test menu
        
      }
      break;
    case WM_ENTERMENULOOP:
      // wparam=true entrato da trackpopupmenu opp false
      break;
    case WM_EXITMENULOOP:
      // wparam=true se shortcut opp false
      break;
    case WM_ENTERIDLE:
      // wparam= 
			hWnd->internalState=MSGF_MENU;
      break;
    case WM_KICKIDLE:   // in effetti pare solo AFX
      break;
    case WM_COMMAND:  //HI-wparam: 0/1 	LO-wparam: menu/accelerator id lparam:0
      break;
    case WM_SYSCOMMAND:
      return 1;
      break;
    case WM_MENUSELECT:
      {MENU *m=(MENU*)lParam;
      }
      break;
    case WM_NEXTMENU:   // MDI soprattutto
      break;
/*    case WM_QUERYCENTERWINDOW:    // afx ecc
    case WM_SETMESSAGESTRING:
    case WM_QUERYAFXWNDPROC:
    case WM_FLOATSTATUS:
    case WM_SIZEPARENT:
    case WM_SIZECHILD:
    case WM_INITIALUPDATE:
    case WM_RECALCPARENT:
      break;*/

    case WM_ENDSESSION:
      return 0;
      break;
    case WM_QUERYENDSESSION:
      return TRUE;
      break;
    case WM_QUIT:
      break;
    case WM_NULL:
      break;

    }
  return 0;
  }

LRESULT DefWindowProcStaticWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { RECT rc;
      int i;
      
      PAINTSTRUCT ps;
      HDC *hDC=BeginPaint(hWnd,&ps);
      
      GetClientRect(hWnd,&rc);
      switch(hWnd->style & 0xff) {
        case SS_LEFT:
          SetTextColor(hDC,WHITE);
          SetBkColor(hDC,hDC->brush.color);
          TextOut(hDC,0,0,hWnd->caption);
          break;
        case SS_CENTER:
          SetTextColor(hDC,WHITE);
          SetBkColor(hDC,hDC->brush.color);
          i=strlen(hWnd->caption)*6*hDC->font.size;
          if(i>=rc.right)
            i=rc.right;
          TextOut(hDC,(rc.right-i)/2,0,hWnd->caption);
          break;
        case SS_RIGHT:
          SetTextColor(hDC,WHITE);
          SetBkColor(hDC,hDC->brush.color);
          i=strlen(hWnd->caption)*6*hDC->font.size;
          if(i>=rc.right)
            i=rc.right;
          TextOut(hDC,rc.right-i,0,hWnd->caption);
          break;
        case SS_ICON:
          DrawIcon8(hDC,0,0,hWnd->icon);
          break;
        case SS_BITMAP:
//          DrawIcon(hDC,0,0,hWnd->icon);
          break;
        case SS_BLACKFRAME:
          hDC->pen=CreatePen(PS_SOLID,1,BLACK);
          DrawRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
          break;
        case SS_GRAYFRAME:
          hDC->pen=CreatePen(PS_SOLID,1,LIGHTGRAY);
          DrawRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
          break;
        case SS_WHITEFRAME:
          hDC->pen=CreatePen(PS_SOLID,1,WHITE);
          DrawRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
          break;
        case SS_BLACKRECT:
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,BLACK);
          break;
        case SS_GRAYRECT:
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,GRAY192);
          break;
        case SS_WHITERECT:
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,WHITE);
          break;
        case SS_SUNKEN:
          break;
        }
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      int c=hDC->brush.color;
      GetClientRect(hWnd,&rc);
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR,(WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
#ifndef USING_SIMULATOR
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
#endif
      // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,1));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      *(FONT*)GET_WINDOW_OFFSET(hWnd,0)=GetStockObject(SYSTEM_FIXED_FONT).font;
      cs->style |= WS_DISABLED;   // direi
      if((cs->style & 0xff) == SS_ICON)
        cs->cx=cs->cy=8 + (hWnd->style & WS_BORDER ? 2 : 0); // type ecc..
      else
        cs->cy=((FONT*)GET_WINDOW_OFFSET(hWnd,0))->size*8 + (hWnd->style & WS_BORDER ? 2 : 0); // 
      }
      return 0;
      break;
    case WM_SETFONT:
      {
      *(FONT*)GET_WINDOW_OFFSET(hWnd,0)=*(FONT *)wParam;
      if(LOWORD(lParam))
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case WM_GETFONT:
      return (LRESULT)GET_WINDOW_OFFSET(hWnd,0) /*occhio..*/;    //or NULL if the control is using the system font.
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcEditWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      RECT rc;
      char *s;
      BYTE x,startSel,endSel;
      int i;
      FONT font;
      PAINTSTRUCT ps;
      
      GetClientRect(hWnd,&rc);
      HDC *hDC=BeginPaint(hWnd,&ps);
      font=*(FONT*)GET_WINDOW_OFFSET(hWnd,0);
      
      s=hWnd->caption;
      startSel=GetWindowByte(hWnd,sizeof(FONT)+1);
      endSel=GetWindowByte(hWnd,sizeof(FONT)+2);
      x=0;
      while(*s) {
        if(startSel || endSel) {
          if(x>=startSel && x<=endSel)
            SetTextColor(hDC,BLACK);
          else
            SetTextColor(hDC,WHITE);
          }
        else
          SetTextColor(hDC,WHITE);
        DrawCharWindow(hDC,x*6*hDC->font.size,rc.top+1,*s++);
        x++;
        }
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      int c=hDC->brush.color;
      GetClientRect(hWnd,&rc);
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR,(WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
#ifndef USING_SIMULATOR
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
#endif
      // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,1));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      *(FONT*)GET_WINDOW_OFFSET(hWnd,0)=GetStockObject(SYSTEM_FIXED_FONT).font;
      }
      return 0;
      break;
    case WM_LBUTTONDOWN:
      //startsel ecc
      break;
    case WM_CHAR:
      { char buf[8];
      int i;
      HDC myDC,*hDC;
      POINT pt;
      
      if(hWnd->enabled) {
        switch(wParam) {
          case VK_RETURN:   // gestire tasti strani..
          default:
            buf[0]=wParam; buf[1]=0;
            hDC=GetDC(hWnd,&myDC);
            pt.x=GetWindowByte(hWnd,sizeof(FONT)) * hDC->font.size;   // curpos
            pt.y=1;   // curpos
            TextOut(hDC,pt.x,pt.y,buf);
            ReleaseDC(hWnd,hDC);
            if(pt.x<32   )       // curpos
              pt.x++;
            SetWindowByte(hWnd,sizeof(FONT),pt.x);   // 
            break;
          }
      
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd);
        }
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_SETFOCUS:
      if(!hWnd->focus) {
        RECT rc;
        int i;
        GetClientRect(hWnd,&rc);
        i=GetWindowByte(hWnd,sizeof(FONT)) * 6;   // curpos
        i=rc.left+i;
        if(i>=rc.right)
          i=rc.right;
        DrawCaret(hWnd,i,rc.top+1,standardCaret);
        }
      hWnd->focus=1;
      break;
    case WM_KILLFOCUS:
      hWnd->focus=0;
      break;
    case WM_SETFONT:
      { int i;
      *(FONT*)GET_WINDOW_OFFSET(hWnd,0)=*(FONT*)wParam;
      if(LOWORD(lParam))
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case WM_GETFONT:
      return (LRESULT)GET_WINDOW_OFFSET(hWnd,0) /*occhio..*/;    //or NULL if the control is using the system font.
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      SetWindowByte(hWnd,sizeof(FONT),0);   // curpos ecc
      SetWindowByte(hWnd,sizeof(FONT)+1,0);   // curpos ecc
      SetWindowByte(hWnd,sizeof(FONT)+2,0);   // curpos ecc
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcListboxWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC *hDC=BeginPaint(hWnd,&ps);

      SetTextColor(hDC,WHITE);
      TextOut(hDC,0,0,hWnd->caption);    // ovviamente finire :)
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      int c=hDC->brush.color;
      GetClientRect(hWnd,&rc);
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR,(WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
#ifndef USING_SIMULATOR
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
#endif
      // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,1));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      *((FONT*)(GET_WINDOW_OFFSET(hWnd,0)))=GetStockObject(SYSTEM_FIXED_FONT).font;
      }
      return 0;
      break;
    case WM_CHAR:
    { char buf[8];
      if(hWnd->enabled) {
        HDC *hDC,myDC;
        hDC=GetDC(hWnd,&myDC);
        buf[0]=wParam; buf[1]=0;
        TextOut(hDC,0,0,buf);    // prova!!
        ReleaseDC(hWnd,hDC);
        }
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
    }
      return 0;
      break;
    case WM_SETFOCUS:
      if(hWnd->enabled)
        hWnd->focus=1;
      break;
    case WM_KILLFOCUS:
      if(hWnd->enabled)
        hWnd->focus=0;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcButtonWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {RECT rc;
      int i;
      PAINTSTRUCT ps;
      
      GetClientRect(hWnd,&rc);
      HDC *hDC=BeginPaint(hWnd,&ps);

      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
            SetTextColor(hDC,WHITE);
            }
          else {
            SetTextColor(hDC,BLACK);
            }
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              TextOut(hDC,0,0,hWnd->caption);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,(rc.right-i)/2,0,hWnd->caption);
              break;
            case BS_RIGHT:
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,rc.right-i,0,hWnd->caption);
              break;
            case BS_TOP:
              TextOut(hDC,(rc.right-i)/2,0,hWnd->caption);
              break;
            case BS_BOTTOM:
              TextOut(hDC,(rc.right-i)/2,rc.bottom-hDC->font.size*8,hWnd->caption);
              break;
            case BS_VCENTER:
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,(rc.right-i)/2,(rc.bottom-hDC->font.size*8)/2,hWnd->caption);
              break;
            case BS_ICON:
              DrawIcon8(hDC,0,0,hWnd->icon);
              break;
            }
          if(hWnd->focus)
            DrawHorizLineWindowColor(hDC,rc.left+1,rc.bottom-1,rc.right-1,CYAN);
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          i=rc.bottom-rc.top-2;
          hDC->pen.color=hWnd->active ? WHITE : GRAY192;
          DrawRectangleWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
          // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,1));
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                DrawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              TextOut(hDC,i+1,0,hWnd->caption);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                DrawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                DrawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,rc.right-strlen(hWnd->caption)*6*hDC->font.size,0,hWnd->caption);
              break;
            case BS_TOP:
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_BOTTOM:
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,rc.bottom-hDC->font.size*8,hWnd->caption);
              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                DrawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,(rc.bottom-hDC->font.size*8)/2,hWnd->caption);
              break;
            case BS_ICON:
              DrawIcon8(hDC,0,0,hWnd->icon);
              break;
            }
          if(hWnd->focus)
            ;
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          hDC->pen.color=hWnd->active ? WHITE : GRAY192;
          DrawCircleWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
          // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,1));
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawCircleWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
                // FILLED!
                }
              TextOut(hDC,0,0,hWnd->caption);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawCircleWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
                // FILLED!
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawCircleWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
                // FILLED!
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,rc.right-strlen(hWnd->caption)*6*hDC->font.size,0,hWnd->caption);
              break;
            case BS_TOP:
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_BOTTOM:
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,rc.bottom-hDC->font.size*8,hWnd->caption);
              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,sizeof(FONT)+1)) {   // il secondo byte è lo stato
                DrawCircleWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
                // FILLED!
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=rc.right)
                i=rc.right;
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*6*hDC->font.size)/2,(rc.bottom-hDC->font.size*8)/2,hWnd->caption);
              break;
            case BS_ICON:
              DrawIcon8(hDC,0,0,hWnd->icon);
              break;
            }
          if(hWnd->focus)
            ;
          break;
        }
        EndPaint(hWnd,&ps);
        }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      int i;
      int c=hDC->brush.color;
      
      GetClientRect(hWnd,&rc);
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR,(WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
          // USARE DrawFrameControl(&hWnd->hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,1));
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,c);
          break;
        }
      }
      return 1;
      break;
    case WM_LBUTTONDOWN:
      if(hWnd->enabled) {
        if(hWnd->parent)
          SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
        }
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_KEYDOWN:
      break;
    case WM_KEYUP:
      break;
    case WM_CHAR:
    { 
      if(hWnd->enabled) {
        switch(hWnd->style & 0x0f) {
          case BS_PUSHBUTTON:
          case BS_DEFPUSHBUTTON:
            switch(wParam) {
              case VK_SPACE:
                SetWindowByte(hWnd,sizeof(FONT)+1,!GetWindowByte(hWnd,sizeof(FONT)+1));
                InvalidateRect(hWnd,NULL,TRUE);
                if(hWnd->parent)
                  SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
                break;
              case VK_RETURN:
                break;
              default:  // hot key??
                if(GetWindowByte(hWnd,sizeof(FONT)+2)) {   // il terzo byte è hotkey
                  }
                break;
              }
            break;
          case BS_CHECKBOX:   // BUTTON FINIRE ECC
          case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
            switch(wParam) {
              default:  // hot key??
                if(GetWindowByte(hWnd,sizeof(FONT)+2)) {   // il terzo byte è hotkey
                  }
                
//                  InvalidateRect(hWnd,NULL,TRUE);

                break;
              }
            break;
          case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
            switch(wParam) {
              case VK_RIGHT:
              case VK_DOWN:
                break;
              case VK_LEFT:
              case VK_UP:
                break;
              default:  // hot key??
                if(GetWindowByte(hWnd,sizeof(FONT)+2)) {   // il terzo byte è hotkey
                  }
                
//                  InvalidateRect(hWnd,NULL,TRUE);
                
                break;
              }
            break;
          }
        }
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
//      SetWindowByte(hWnd,sizeof(FONT),cs->style & 0x0f);   // salvo il tipo, anche se in effetti sarebbe anche in style..
      int i;
      *((FONT*)GET_WINDOW_OFFSET(hWnd,0))=GetStockObject(SYSTEM_FIXED_FONT).font;
      }
      return 0;
      break;
    case WM_SETFONT:
      {
      *(FONT*)GET_WINDOW_OFFSET(hWnd,0)=*(FONT*)wParam;
      if(LOWORD(lParam))
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case WM_GETFONT:
      return (LRESULT)GET_WINDOW_OFFSET(hWnd,0) /*occhio..*/;    //or NULL if the control is using the system font.
      break;
    case WM_SETTEXT:
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;

    case BM_SETSTATE:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          SetWindowByte(hWnd,sizeof(FONT)+1,wParam);
          InvalidateRect(hWnd,NULL,TRUE);
          if(hWnd->enabled) {
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
      // hmmm bah sì
            }
          else
            return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      return 0;
      break;
    case BM_GETSTATE:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          return GetWindowByte(hWnd,sizeof(FONT)+1) ? BST_PUSHED : 0;
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          break;
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          break;
        }
      break;
    case BM_SETCHECK:   // questo è per radiobutton e checkbox...
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          switch(wParam) {
            case BST_CHECKED:
              break;
            case BST_UNCHECKED:
              break;
            case BST_INDETERMINATE:
              break;
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      break;
    case BM_GETCHECK:   // questo è per radiobutton e checkbox...
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          break;
        }
      break;

    case WM_SETFOCUS:
      hWnd->focus=1;
      if(!hWnd->focus) {
        InvalidateRect(hWnd,NULL,TRUE);
        }
      return 0;
      break;
    case WM_KILLFOCUS:
      hWnd->focus=0;
      if(hWnd->focus)
        InvalidateRect(hWnd,NULL,TRUE);
      return 0;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

BOOL DrawFrameControl(HDC *hDC,RECT *rc,UINT type,UINT state) {
  
  switch(type) {
    case DFC_BUTTON:
      switch(state) {
        case DFCS_PUSHED:
          SetTextColor(hDC,DFCS_PUSHED ? WHITE : GRAY192);
          break;
        }
      break;
    }
  // finire, usare
  FillRectangleWindow(hDC,rc->left,rc->top,rc->right,rc->bottom);

  }

void DialogBox(HINSTANCE hInstance,const void *lpTemplate,HWND hWndParent,DIALOGPROC *lpDialogFunc) {
  HWND hWnd,focusWnd;
  DLGTEMPLATE *t=(DLGTEMPLATE*)lpTemplate;
  int i;
  
	hWnd=CreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style,
    t->x,t->y,t->cx,t->cy,
    hWndParent,(MENU *)NULL,(void*)256
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    HWND myWnd,focusWnd;
    myWnd=CreateWindow(t2->class,t2->caption,t2->style,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(MENU *)(int)t2->id,NULL
      );
    focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }
  
  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,0);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)t->font,TRUE);
  ShowWindow(hWnd,SW_SHOW);
  }

void CreateDialog(HINSTANCE hInstance,const char *lpName,HWND hWndParent,DIALOGPROC *lpDialogFunc) {
  DLGTEMPLATE *t;
  HWND hWnd,focusWnd;
  int i;
  
  t=(DLGTEMPLATE*)lpName;   // qua SOLO con struct! no risorse..
  
	hWnd=CreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style,
    t->x,t->y,t->cx,t->cy,
    hWndParent,(MENU *)NULL,(void*)256
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    HWND myWnd,focusWnd;
    myWnd=CreateWindow(t2->class,t2->caption,t2->style,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(MENU *)(int)t2->id,NULL
      );
    focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }

  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,0);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)t->font,TRUE);
  ShowWindow(hWnd,SW_SHOW);
  }

void CreateDialogParam(HINSTANCE hInstance,const char *lpName,HWND hWndParent,DIALOGPROC *lpDialogFunc, LPARAM dwInitParam) {
  DLGTEMPLATE *t;
  HWND hWnd,focusWnd;
  int i;
  
  t=(DLGTEMPLATE*)lpName;   // qua SOLO con struct! no risorse..
  
	hWnd=CreateWindow(MAKECLASS(WC_DIALOG),t->caption,t->style,
    t->x,t->y,t->cx,t->cy,
    hWndParent,(MENU *)NULL,(void*)dwInitParam
    );
  for(i=0; i<t->cdit; i++) {
    DLGITEMTEMPLATE *t2=t->item[i];
    HWND myWnd,focusWnd;
    myWnd=CreateWindow(t2->class,t2->caption,t2->style,
      t2->x,t2->y,t2->cx,t2->cy,
      hWnd,(MENU *)(int)t2->id,NULL
      );
    focusWnd=myWnd;     // SCEGLIERE QUALE! il più basso come ID??
    }

  SendMessage(hWnd,WM_INITDIALOG,(WPARAM)focusWnd,0);
  SendMessage(hWnd,WM_SETFONT,(WPARAM)t->font,TRUE);
  ShowWindow(hWnd,SW_SHOW);
  }

LRESULT DefDlgProc(HWND hDlg,UINT Msg,WPARAM wParam,LPARAM lParam) {
  
  return DefWindowProcDlgWC(hDlg,Msg,wParam,lParam);
  }
  
BOOL EndDialog(HWND hDlg,int nResult) {
  
  SetWindowLong(hDlg,DWL_MSGRESULT,nResult);
  SendMessage(hDlg,WM_CLOSE,0,0);
  return TRUE;
  }

HWND GetDlgItem(HWND hDlg,uint16_t nIDDlgItem) {
  HWND myWnd=hDlg->children;
  
  while(myWnd) {
    if((uint16_t)(DWORD)myWnd->menu == nIDDlgItem)
      return myWnd;
    myWnd=myWnd->next;
    }
  return NULL;
  }

uint16_t GetDlgCtrlID(HWND hWnd) {
  
  return (uint16_t)(DWORD)hWnd->menu;
  }

UINT SetDlgItemInt(HWND hDlg,uint16_t nIDDlgItem,UINT uValue,BOOL bSigned) {
  char buf[20];
  
  sprintf(buf,bSigned ? "%d" : "%u",uValue);
  return SetWindowText(GetDlgItem(hDlg,nIDDlgItem),buf);
  }

UINT GetDlgItemInt(HWND hDlg,uint16_t nIDDlgItem,BOOL *lpTranslated,BOOL bSigned) {
  int32_t n;
  
  n=atoi(hDlg->caption);
  if(lpTranslated)
    *lpTranslated=TRUE;
  if(!bSigned && n<0)
    n=-n;
    
  return n;
  }

UINT SetDlgItemText(HWND hDlg,uint16_t nIDDlgItem,const char *lpString) {
  
  return SetWindowText(GetDlgItem(hDlg,nIDDlgItem),lpString);
  }

UINT GetDlgItemText(HWND hDlg,uint16_t nIDDlgItem,char *lpString,int cchMax) {
  
  return GetWindowText(hDlg,lpString,cchMax);
  }

LRESULT DefWindowProcDlgWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      GetClientRect(hWnd,&rc);
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,hDC->brush.color);
      }
      return 1;
      break;
    case WM_CTLCOLORDLG:
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
//      SetWindowByte(hWnd,sizeof(FONT),cs->style & 0x0f);   // salvo il tipo, anche se in effetti sarebbe anche in style..
      int i;
      *(FONT*)GET_WINDOW_OFFSET(hWnd,DWL_FONT)=GetStockObject(SYSTEM_FIXED_FONT).font;
      }
      return 0;
      break;
    case WM_INITDIALOG:
      //wParam=A handle to the control to receive the default keyboard focus. The system assigns the default keyboard focus only if the dialog box procedure returns TRUE.
      //lParam=Additional initialization data. This data is passed to the system as the lParam parameter 
      if(wParam)
        SetFocus((HWND)wParam);
      SetWindowByte(hWnd,DWL_INTERNAL,1);   // finire
      // p.es. CheckDlgButton(hwndDlg, ID_ABSREL, fRelative); 
      return 1;
      break;
    case WM_NEXTDLGCTL:   // WS_TABSTOP ecc
      {HWND myWnd;

      if(LOWORD(lParam)) {
        HWND myWnd2=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));
        myWnd=(HWND)wParam;
        if(myWnd->visible && myWnd->enabled /*&& myWnd->style & WS_TABSTOP*/) {   // bah io direi così...
          if(myWnd2)
            SendMessage(myWnd2,WM_KILLFOCUS,(WPARAM)myWnd,0);
          SendMessage(myWnd,WM_SETFOCUS,(WPARAM)myWnd2,0);
          SetWindowByte(hWnd,DWL_INTERNAL,(BYTE)(DWORD)myWnd->menu);   // verificare il tutto
          }
        }
      else {
        myWnd=hWnd->children;
        while(myWnd) {
          HWND myWnd2;
          //if(!wParam) //successivo
          //else //precedente! FARE
          myWnd2=myWnd->next;
          if(!myWnd2)
            myWnd2=hWnd->children;
          if(myWnd->visible && myWnd->enabled && myWnd->style & WS_TABSTOP && myWnd->focus) {
            SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)myWnd2,0);
            SendMessage(myWnd2,WM_SETFOCUS,(WPARAM)myWnd,0);
            SetWindowByte(hWnd,DWL_INTERNAL,(BYTE)(DWORD)myWnd2->menu);   // verificare il tutto
            break;
            }
          myWnd=myWnd->next;
          }
        }
      printf("nextDlgCtl: %u\r\n",GetWindowByte(hWnd,DWL_INTERNAL));
      }
      break;
    case WM_ACTIVATE: // save or restore control focus..
      break;
    case WM_SHOWWINDOW:  // saves
      break;
    case WM_NCDESTROY:
      break;
//    case WM_CLOSE:
      // se serve ... SetWindowLong(hwndDlg, DWL_MSGRESULT, lResult) ma v. EndDialog
//      PostMessage(hWnd,WM_COMMAND,MAKEWORD(IDCANCEL,BN_CLICKED),(LPARAM)NULL);
//      DestroyWindow(hWnd);
//      break;
    case DM_GETDEFID:
      return GetWindowByte(hWnd,DWL_INTERNAL);   // finire
      break;
    case DM_SETDEFID:
      SetWindowByte(hWnd,DWL_INTERNAL,1);   // finire
      break;
    case DM_REPOSITION:
      break;
    case WM_COMPAREITEM:
      return 0;
      break;
    case WM_SETFONT:
      {
      *(FONT*)GET_WINDOW_OFFSET(hWnd,DWL_FONT)=*(FONT*)wParam;
      if(LOWORD(lParam))
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case WM_GETFONT:
      return (LRESULT)GET_WINDOW_OFFSET(hWnd,DWL_FONT) /*occhio..*/;    //or NULL if the control is using the system font.
      break;
      
    case WM_COMMAND:  //HI-wparam: Control-defined notification code 	LO-wparam: Control identifier 	lparam:Handle to the control window
      switch(HIWORD(wParam)) { 
        case BN_CLICKED:
          switch(LOWORD(wParam)) { 
            case IDOK: 

                // Notify the owner window to carry out the task. 

              EndDialog(hWnd,1);
              return TRUE; 
              break;

            case IDCANCEL: 
    //          DestroyWindow(hWnd); 
              EndDialog(hWnd,0);
              return TRUE; 
              break;
            } 
          break;
        } 
      break;
    case WM_MENUCOMMAND:  //wParam=The zero-based index of the item selected. lParam=A handle to the menu for the item selected.
      // al posto di WM_COMMAND se menu ha MNS_NOTIFYBYPOS nello stile..
      break;
    case WM_NOTIFY:
      //SetWindowLong ( DWL_MSGRESULT 
      break;
    case WM_CHAR:
      if(hWnd->enabled) {
        switch(wParam) {
          case VK_RETURN:
          {
            HWND myWnd=GetDlgItem(hWnd,IDOK);
            if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/)    // solo se c'è... ed è DEFPUSHBUTTON?
              EndDialog(hWnd,1);
  // hmm no serve per cose particolari..          if(hWnd->parent)
  //            SendMessage(hWnd->parent,WM_NOTIFY,0, 0 /*NMHDR struct*/);
          }
            break;
          case VK_ESCAPE:
          {
            HWND myWnd=GetDlgItem(hWnd,IDCANCEL);
            if(myWnd)
              EndDialog(hWnd,0);
  // hmm no serve per cose particolari..          if(hWnd->parent)
  //            SendMessage(hWnd->parent,WM_NOTIFY,0, 0 /*NMHDR struct*/);
          }
            break;
          case VK_TAB:
            SendMessage(hWnd,WM_NEXTDLGCTL,0,MAKELONG(0,0));
            break;
  /* no :) ..        case VK_SPACE:
          {HWND myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));
        printf("premo SPAZIO: %u\r\n",myWnd);
          if(myWnd)
            SendMessage(hWnd,WM_COMMAND,MAKELONG((DWORD)myWnd->tag,BN_CLICKED),myWnd);
          }
            break;*/
          default:
            // direi di passare a defwindowproc, o almeno gestire ESCAPE e frecce...
            return DefWindowProc(hWnd,message,wParam,lParam);
            break;
          }
        }
      else {
        if(hWnd->parent)
          return DefWindowProc(hWnd->parent,message,wParam,lParam);
        }
      return 0;
      break;
    case WM_CHARTOITEM:
      return 0;
      break;
    case WM_VKEYTOITEM:
      return 0;
      break;
    case WM_SYSCOMMAND:   // saves focus
//      SetWindowByte(hWnd,DWL_INTERNAL,1);   // finire
      break;
      
    case WM_GETDLGCODE:
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }

  }

LRESULT DefWindowProcDlgMessageBox(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_CHAR:
      switch(wParam) {
        case VK_RETURN:
        {
          HWND myWnd=GetDlgItem(hWnd,IDOK);
          if(myWnd /*&& myWnd->style & BS_DEFPUSHBUTTON*/)    // solo se c'è... ed è DEFPUSHBUTTON?
            EndDialog(hWnd,1);
        }
          break;
        case VK_ESCAPE:
          EndDialog(hWnd,0);
          break;
/*        case VK_SPACE:
        {HWND myWnd=GetDlgItem(hWnd,GetWindowByte(hWnd,DWL_INTERNAL));
        if(myWnd)
          SendMessage(hWnd,WM_COMMAND,MAKELONG((DWORD)myWnd->tag,BN_CLICKED),myWnd);
        }
          break;*/
        default:
          return DefWindowProcDlgWC(hWnd,message,wParam,lParam);
          break;
        }
      break;
    default:
      return DefWindowProcDlgWC(hWnd,message,wParam,lParam);
      break;
    }
  }

LRESULT DefWindowProcFileDlgWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      SearchRec rec;
      SYS_FS_FSTAT stat;
      FS_DISK_PROPERTIES disk_properties;
      SYS_FS_HANDLE myFileHandle;
      int i; 
      UGRAPH_COORD_T x,y;
      RECT rc;
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
      char label[16 /*TOTAL_FILE_SIZE+1*/];
      WORD totFiles;
      
      HDC *hDC=BeginPaint(hWnd,&ps);

      GetClientRect(hWnd,&rc);
      if(!GetWindowByte(hWnd,DWL_INTERNAL+2))
        FillRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
      totFiles=0;

      switch(ofn->disk) {
        case 'A':
          if(!SDcardOK) {
no_disc:
            SetWindowByte(hWnd,DWL_INTERNAL+2,1);
            SetWindowText(hWnd,"Disco");
            TextOut(hDC,rc.left+10,rc.top+10,"Inserire disco:");
            goto fine;
            }
          if(!FindFirst(ASTERISKS, ATTR_VOLUME, &rec))
            strcpy(label,rec.filename);
          else
            *label=0;
          if(*ofn->path)
            i=FindFirst(ofn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=FindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        case 'B':
          break;
        case 'C':
          if(!HDOK)
            goto no_disc;
          if(!IDEFindFirst(ASTERISKS, ATTR_VOLUME, &rec))
            strcpy(label,rec.filename);
          else
            *label=0;
          if(*ofn->path)
            i=IDEFindFirst(ofn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=IDEFindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        case 'D':
          break;
#if defined(USA_USB_HOST_MSD)
        case 'E':
          {
          uint32_t sn,timestamp=0;
          i=SYS_FS_DriveLabelGet(NULL, label, &sn, &timestamp);
          i=1;
          if((myFileHandle=SYS_FS_DirOpen("/")) != SYS_FS_HANDLE_INVALID) {
            i=SYS_FS_DirSearch(myFileHandle,*ofn->path ? ofn->path : ASTERISKS,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
            if(!i) {
              strcpy(rec.filename,stat.fname);
              rec.attributes=stat.fattrib;
              rec.timestamp=MAKELONG(stat.fsize,stat.ftime);    // VERIFICARE!
              }
            }
          }
          break;
#endif
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
          if(!RAMdiscArea)
            goto no_disc;
          if(!RAMFindFirst(ASTERISKS, ATTR_VOLUME, &rec))
            strcpy(label,rec.filename);
          else
            *label=0;
          if(*ofn->path)
            i=RAMFindFirst(ofn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=RAMFindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
        case DEVICE_NETWORK:
          break;
#endif
        default:
          goto no_disc;
          break;
        }
      
      if(!i) {
        x=0; y=0;
        if(GetWindowByte(hWnd,DWL_INTERNAL+2)<2)
          SetWindowByte(hWnd,DWL_INTERNAL+2,2);
        
loop:
					totFiles++;
          if(y<rc.bottom) {
            switch(GetWindowByte(hWnd,DWL_INTERNAL+1)) {
              case 0:     // icone piccole
                if(strlen(rec.filename)<6) {
                  TextOut(hDC,x+4+(6-strlen(rec.filename))*6/2,y+4+8+2,rec.filename); 
                  }
                else {
                  rec.filename[12]=0;
                  TextOut(hDC,x+4+(6-strlen(rec.filename+6))*6/2,y+4+8+8+2,rec.filename+6); 
                  rec.filename[6]=0;
                  TextOut(hDC,x+4+(6-strlen(rec.filename))*6/2,y+4+8+2,rec.filename); 
                  }
                if(rec.attributes & ATTR_DIRECTORY) {
                  DrawIcon8(hDC,x+18,y+4,folderIcon8);
                  }
                else {
                  DrawIcon8(hDC,x+18,y+4,fileIcon8);
                  }
                x+=40;
                if(x>=rc.right) {
                  x=0;
                  y+=32;
                  }
                break;
              case 1:     // icone grandi
                if(strlen(rec.filename)<9) {
                  TextOut(hDC,x+4+(9-strlen(rec.filename))*6/2,y+4+16+2,rec.filename);
                  }
                else {
                  rec.filename[18]=0;
                  TextOut(hDC,x+4+(9-strlen(rec.filename+9))*6/2,y+4+16+8+2,rec.filename+9);
                  rec.filename[9]=0;
                  TextOut(hDC,x+4+(9-strlen(rec.filename))*6/2,y+4+16+2,rec.filename);
                  }
                if(rec.attributes & ATTR_DIRECTORY) {
                  DrawIcon(hDC,x+25,y+4,folderIcon);
                  }
                else {
                  DrawIcon(hDC,x+25,y+4,fileIcon);
                  }
                x+=60;
                if(x>=rc.right) {
                  x=0;
                  y+=40;
                  }
                break;
              case 2:     // dettagli
//                rec.filename[16]=0; INUTILE nomi file corti :)
                TextOut(hDC,x+2,y+2,rec.filename); 
                if(rec.attributes & ATTR_DIRECTORY) {
                  TextOut(hDC,x+13*6,y+2,"DIR");
                  }
                else {
                  char buf[32];
                  sprintf(buf,"%u",rec.filesize);
                  TextOut(hDC,x+13*6,y+2,buf);
                  sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",
                    (rec.timestamp >> 16) & 31,
                    (rec.timestamp >> (5+16)) & 15,
                    (rec.timestamp >> (9+16)) + 1980,
                    (rec.timestamp >> 11) & 31,
                    (rec.timestamp >> 5) & 63,
                    rec.timestamp & 63);
                  TextOut(hDC,x+(14+10)*6,y+2,buf);
                  }
                y+=8;
                break;
              }
            }
          switch(ofn->disk) {
            case 'A':
              if(!FindNext(&rec))
                goto loop;
              break;
            case 'B':
              break;
            case 'C':
              if(!IDEFindNext(&rec))
                goto loop;
              break;
            case 'D':
              break;
#if defined(USA_USB_HOST_MSD)
            case 'E':
              i=SYS_FS_DirSearch(myFileHandle,*ofn->path ? ofn->path : ASTERISKS,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
              if(!i)
                goto loop;
              else
                SYS_FS_DirClose(myFileHandle);
              break;
#endif
#ifdef USA_RAM_DISK 
            case DEVICE_RAMDISK:
              if(!RAMFindNext(&rec))
                goto loop;
              break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
            case DEVICE_NETWORK:
              break;
#endif
            }

        switch(GetWindowByte(hWnd,DWL_INTERNAL+1)) {
          case 0:
            if(((totFiles*32)/40)>(rc.bottom-rc.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top) / ((totFiles*8)/(rc.bottom-rc.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top)*(rc.bottom-rc.top) / ((totFiles*32)/40),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            hWnd->style &= ~WS_HSCROLL;
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            break;
          case 1:
            if(((totFiles*60)/40)>(rc.bottom-rc.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top) / ((totFiles*8)/(rc.bottom-rc.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top)*(rc.bottom-rc.top) / ((totFiles*60)/40),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            hWnd->style &= ~WS_HSCROLL;
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            break;
          case 2:
            if((totFiles*8)>(rc.bottom-rc.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top) / ((totFiles*8)/(rc.bottom-rc.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top)*(rc.bottom-rc.top) / (totFiles*8),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            if((40*6)>(rc.right-rc.left)) {   // v. testo sopra
              hWnd->style |= WS_HSCROLL;
              SetScrollRange(hWnd,SB_HORZ,0,(rc.right-rc.left)*(rc.right-rc.left) / (40*6),FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_HSCROLL;
              SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              }
            break;
// SISTEMARE! se faccio redraw nonclient poi forza redrawclient e va tutto in vacca...
          }
        }

      switch(GetWindowByte(hWnd,DWL_INTERNAL+1)) {
        case 0:
          y+=32;
          break;
        case 1:
          y+=40;
          break;
        case 2:
          y+=10;
          break;
        }
      if(GetWindowByte(hWnd,DWL_INTERNAL+2)<3) {
        ofn->fsdp.new_request=1;
        switch(ofn->disk) {
          case 'A':
            do {
              FSGetDiskProperties(&ofn->fsdp);
              ClrWdt();
              } while(ofn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
          case 'B':
            break;
          case 'C':
            do {
              IDEFSGetDiskProperties(&ofn->fsdp);
              ClrWdt();
              } while(ofn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
          case 'D':
            break;
#if defined(USA_USB_HOST_MSD)
          case 'E':
            {
            uint32_t freeSectors,totalSectors,sectorSize;
            if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS)
              ofn->fsdp.results.free_clusters=freeSectors;
              ofn->fsdp.results.sectors_per_cluster=1;
              ofn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
            }
            break;
#endif
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
            do {
              RAMFSGetDiskProperties(&ofn->fsdp);
              ClrWdt();
              } while(ofn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case DEVICE_NETWORK:
            break;
#endif
          }
        }

      if(hWnd->parent)
        SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_INITDONE /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser

fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      GetClientRect(hWnd,&rc);
      
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
      if(!GetWindowByte(hWnd,DWL_INTERNAL+2)) {
        TextOut(hDC,10,0,"attendere prego...");
        //e fare magari pure un Mount o FSinit..
        }
          
//          DrawIcon8(&hWnd->hDC,0,16,folderIcon);
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      // forzare font piccolo??
      int i;
      *(FONT*)GET_WINDOW_OFFSET(hWnd,DWL_FONT)=GetStockObject(SYSTEM_FIXED_FONT).font;

      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
			memset(GET_WINDOW_DLG_OFFSET(hWnd,0),sizeof(OPENFILENAME),0);
      }
      return 0;
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_SPACE:
          if(hWnd->parent)
            SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_FILEOK /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser
          // in effetti sa
          break;
        case 'i':
        case 'I':
          SendMessage(hWnd,WM_NOTIFY,0,CDN_TYPECHANGE);
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }

      break;

    case WM_NOTIFY:
      switch(lParam) {
        case CDN_TYPECHANGE:
          {
          OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
          SetWindowByte(hWnd,DWL_INTERNAL+1,(GetWindowByte(hWnd,DWL_INTERNAL+1)+1) % 3);
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        }
      break;

    case WM_FILECHANGE:
      break;
      
    default:
//      return DefWindowProc(hWnd,message,wParam,lParam);
      return DefWindowProcDlgWC(hWnd,message,wParam,lParam);
      break;
    }
  }

LRESULT DefWindowProcDirWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      SYS_FS_FSTAT stat;
      FS_DISK_PROPERTIES disk_properties;
      SYS_FS_HANDLE myFileHandle;
      SearchRec rec;
      int i; 
      UGRAPH_COORD_T x,y;
      RECT rc;
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,4);
      char label[16 /*TOTAL_FILE_SIZE+1*/];
      WORD totFiles;
      
      HDC *hDC=BeginPaint(hWnd,&ps);

      GetClientRect(hWnd,&rc);
      if(!GetWindowByte(hWnd,2))
        FillRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
      totFiles=0;

      switch(dfn->disk) {
        case 'A':
          if(!SDcardOK) {
no_disc:
            SetWindowByte(hWnd,2,1);
            SetWindowText(hWnd,"Disco");
            TextOut(hDC,rc.left+10,rc.top+10,"Inserire disco:");
            goto fine;
            }
          if(!FindFirst(ASTERISKS, ATTR_VOLUME, &rec))
            strcpy(label,rec.filename);
          else
            *label=0;
          if(*dfn->path)
            i=FindFirst(dfn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=FindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        case 'B':
          break;
        case 'C':
          if(!HDOK)
            goto no_disc;
          if(!IDEFindFirst(ASTERISKS, ATTR_VOLUME, &rec))
            strcpy(label,rec.filename);
          else
            *label=0;
          if(*dfn->path)
            i=IDEFindFirst(dfn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=IDEFindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        case 'D':
          break;
#if defined(USA_USB_HOST_MSD)
        case 'E':
          {
          uint32_t sn,timestamp=0;
          i=SYS_FS_DriveLabelGet(NULL, label, &sn, &timestamp);
          i=1;
          if((myFileHandle=SYS_FS_DirOpen("/")) != SYS_FS_HANDLE_INVALID) {
            i=SYS_FS_DirSearch(myFileHandle,*dfn->path ? dfn->path : ASTERISKS,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
            if(!i) {
              strcpy(rec.filename,stat.fname);
              rec.attributes=stat.fattrib;
              rec.timestamp=MAKELONG(stat.fsize,stat.ftime);    // VERIFICARE!
              }
            }
          }
          break;
#endif
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
          if(!RAMdiscArea)
            goto no_disc;
          if(!RAMFindFirst(ASTERISKS, ATTR_VOLUME, &rec))
            strcpy(label,rec.filename);
          else
            *label=0;
          if(*dfn->path)
            i=RAMFindFirst(dfn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=RAMFindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
        case DEVICE_NETWORK:
          break;
#endif
        default:
          goto no_disc;
          break;
        }
      
      if(!i) {
        x=0; y=0;
        if(GetWindowByte(hWnd,2)<2)
          SetWindowByte(hWnd,2,2);
        
loop:
					totFiles++;
          if(y<rc.bottom) {
            switch(GetWindowByte(hWnd,1)) {
              case 0:     // icone piccole
                if(strlen(rec.filename)<6) {
                  TextOut(hDC,x+4+(6-strlen(rec.filename))*6/2,y+4+8+2,rec.filename); 
                  }
                else {
                  rec.filename[12]=0;
                  TextOut(hDC,x+4+(6-strlen(rec.filename+6))*6/2,y+4+8+8+2,rec.filename+6); 
                  rec.filename[6]=0;
                  TextOut(hDC,x+4+(6-strlen(rec.filename))*6/2,y+4+8+2,rec.filename); 
                  }
                if(rec.attributes & ATTR_DIRECTORY) {
                  DrawIcon8(hDC,x+18,y+4,folderIcon8);
                  }
                else {
                  DrawIcon8(hDC,x+18,y+4,fileIcon8);
                  }
                x+=40;
                if(x>=rc.right) {
                  x=0;
                  y+=32;
                  }
                break;
              case 1:     // icone grandi
                if(strlen(rec.filename)<9) {
                  TextOut(hDC,x+4+(9-strlen(rec.filename))*6/2,y+4+16+2,rec.filename);
                  }
                else {
                  rec.filename[18]=0;
                  TextOut(hDC,x+4+(9-strlen(rec.filename+9))*6/2,y+4+16+8+2,rec.filename+9);
                  rec.filename[9]=0;
                  TextOut(hDC,x+4+(9-strlen(rec.filename))*6/2,y+4+16+2,rec.filename);
                  }
                if(rec.attributes & ATTR_DIRECTORY) {
                  DrawIcon(hDC,x+25,y+4,folderIcon);
                  }
                else {
                  DrawIcon(hDC,x+25,y+4,fileIcon);
                  }
                x+=60;
                if(x>=rc.right) {
                  x=0;
                  y+=40;
                  }
                break;
              case 2:     // dettagli
//                rec.filename[16]=0; INUTILE nomi file corti :)
                TextOut(hDC,x+2,y+2,rec.filename); 
                if(rec.attributes & ATTR_DIRECTORY) {
                  TextOut(hDC,x+13*6,y+2,"DIR");
                  }
                else {
                  char buf[32];
                  sprintf(buf,"%u",rec.filesize);
                  TextOut(hDC,x+13*6,y+2,buf);
                  sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",
                    (rec.timestamp >> 16) & 31,
                    (rec.timestamp >> (5+16)) & 15,
                    (rec.timestamp >> (9+16)) + 1980,
                    (rec.timestamp >> 11) & 31,
                    (rec.timestamp >> 5) & 63,
                    rec.timestamp & 63);
                  TextOut(hDC,x+(14+10)*6,y+2,buf);
                  }
                y+=8;
                break;
              }
            }
          switch(dfn->disk) {
            case 'A':
              if(!FindNext(&rec))
                goto loop;
              break;
            case 'B':
              break;
            case 'C':
              if(!IDEFindNext(&rec))
                goto loop;
              break;
            case 'D':
              break;
#if defined(USA_USB_HOST_MSD)
            case 'E':
              i=SYS_FS_DirSearch(myFileHandle,*dfn->path ? dfn->path : ASTERISKS,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
              if(!i)
                goto loop;
              else
                SYS_FS_DirClose(myFileHandle);
              break;
#endif
#ifdef USA_RAM_DISK 
            case DEVICE_RAMDISK:
              if(!RAMFindNext(&rec))
                goto loop;
              break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
            case DEVICE_NETWORK:
              break;
#endif
            }

        switch(GetWindowByte(hWnd,1)) {
          case 0:
            if(((totFiles*32)/40)>(rc.bottom-rc.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top) / ((totFiles*8)/(rc.bottom-rc.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top)*(rc.bottom-rc.top) / ((totFiles*32)/40),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            hWnd->style &= ~WS_HSCROLL;
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            break;
          case 1:
            if(((totFiles*60)/40)>(rc.bottom-rc.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top) / ((totFiles*8)/(rc.bottom-rc.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top)*(rc.bottom-rc.top) / ((totFiles*60)/40),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            hWnd->style &= ~WS_HSCROLL;
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            break;
          case 2:
            if((totFiles*8)>(rc.bottom-rc.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top) / ((totFiles*8)/(rc.bottom-rc.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(rc.bottom-rc.top)*(rc.bottom-rc.top) / (totFiles*8),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            if((40*6)>(rc.right-rc.left)) {   // v. testo sopra
              hWnd->style |= WS_HSCROLL;
              SetScrollRange(hWnd,SB_HORZ,0,(rc.right-rc.left)*(rc.right-rc.left) / (40*6),FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_HSCROLL;
              SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              }
            break;
// SISTEMARE! se faccio redraw nonclient poi forza redrawclient e va tutto in vacca...
          }
        }

      switch(GetWindowByte(hWnd,1)) {
        case 0:
          y+=24;
          break;
        case 1:
          y+=36;
          break;
        case 2:
          y+=6;
          break;
        }
      if(GetWindowByte(hWnd,2)<3) {
        dfn->fsdp.new_request=1;
        switch(dfn->disk) {
          case 'A':
            do {
              FSGetDiskProperties(&dfn->fsdp);
              ClrWdt();
              } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
          case 'B':
            break;
          case 'C':
            do {
              IDEFSGetDiskProperties(&dfn->fsdp);
              ClrWdt();
              } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
          case 'D':
            break;
#if defined(USA_USB_HOST_MSD)
          case 'E':
            {
            uint32_t freeSectors,totalSectors,sectorSize;
            if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
              dfn->fsdp.results.free_clusters=freeSectors;
              dfn->fsdp.results.sectors_per_cluster=1;
              dfn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
              }
            }
            break;
#endif
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
            do {
              RAMFSGetDiskProperties(&dfn->fsdp);
              ClrWdt();
              } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case DEVICE_NETWORK:
            break;
#endif
          }
        }
      else
        goto print_totals;
      if(dfn->fsdp.properties_status == FS_GET_PROPERTIES_NO_ERRORS) {
        char buf[32];

print_totals:          
        sprintf(buf,"%lu",dfn->fsdp.results.free_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024); 
        TextOut(hDC,4,y+2,buf); 
        SetWindowByte(hWnd,2,3);

        sprintf(buf,"%s(%c) %s",*label ? label : "Disk",toupper(dfn->disk),*dfn->path ? dfn->path : ASTERISKS);
        SetWindowText(hWnd,buf);
        }
      else
        TextOut(hDC,4,y+2,"?"); 
      TextOut(hDC,64,y+2," Kbytes free");

      if(hWnd->parent)
        SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_INITDONE /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser

fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {RECT rc;
      HDC *hDC=(HDC *)wParam;
      GetClientRect(hWnd,&rc);
      
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,4);
      if(!GetWindowByte(hWnd,2)) {
        TextOut(hDC,10,0,"attendere prego...");
        //e fare magari pure un Mount o FSinit..
        }
          
//          DrawIcon8(&hWnd->hDC,0,16,folderIcon);
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      // forzare font piccolo??
      int i;
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,4);
			memset(GET_WINDOW_OFFSET(hWnd,0),4+sizeof(DIRLIST),0);
      }
      return 0;
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_SPACE:
          if(hWnd->parent)
            SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_FILEOK /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser
          // in effetti sa
          break;
        case 'i':
        case 'I':
          SendMessage(hWnd,WM_NOTIFY,0,CDN_TYPECHANGE);
          break;
          
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }

      break;

    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,4);
        switch(LOWORD(wParam)) { 
          case 32+1:
            dfn->disk='A';
            break;
          case 32+2:
            dfn->disk='C';
            break;
          case 32+3:
            dfn->disk='E';
            break;
          case 32+4:
            dfn->disk='R';
            break;
          }
        InvalidateRect(hWnd,NULL,TRUE);
        }

      break;
    case WM_NOTIFY:
      switch(lParam) {
        case CDN_TYPECHANGE:
          {
          DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,4);
          SetWindowByte(hWnd,1,(GetWindowByte(hWnd,1)+1) % 3);
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        }
      break;

    case WM_FILECHANGE:
      // CheckMenuItem(explorerMenu3,)
      // UncheckMenuItem(explorerMenu3,)
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }

  }

LRESULT DefWindowProcCmdShellWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int i;
  char ch;
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      BYTE x,y;
      RECT rc;
      char *cmdline;
      
      HDC *hDC=BeginPaint(hWnd,&ps);
      GetClientRect(hWnd,&rc);
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,3) & 15]);
			x=GetWindowByte(hWnd,0); y=GetWindowByte(hWnd,1);
      FillRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
      TextOut(hDC,rc.left+2+x*6,rc.top+1+y*8,prompt);
      cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
      TextOut(hDC,rc.left+1+6*strlen(prompt),rc.top+1,cmdline);
      EndPaint(hWnd,&ps);

      }
      return 1;
      break;
    case WM_TIMER:
      {
      HDC myDC,*hDC=GetDC(hWnd,&myDC);
      RECT rc;
      BYTE x,y;
      GetClientRect(hWnd,&rc);    // fingo cursore :D
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,3) & 15]);
			x=GetWindowByte(hWnd,0); y=GetWindowByte(hWnd,1);
//      FillRectangleWindow(hDC,rc.left+2+6*3+x*6,rc.top+2+y*8,rc.left+2+6*3+x*6+6,rc.top+2+y*8);
      TextOut(hDC,rc.left+2+6*strlen(prompt)+x*6,rc.top+2+y*8,GetWindowByte(hWnd,4) ? "_" : " ");
      SetWindowByte(hWnd,4,!GetWindowByte(hWnd,4));
      ReleaseDC(hWnd,hDC);
      }
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      SetWindowLong(hWnd,0,0);   //
      SetWindowByte(hWnd,2,15);   //white
      SetWindowByte(hWnd,3,0);   //black
      SetWindowLong(hWnd,4,0);   // cursore & stringa!
      SetTimer(hWnd,1,500,NULL);    // per cursore...
      }
      return 0;
      break;
    case WM_CLOSE:
      KillTimer(hWnd,1);
      break;
      
    case WM_CHAR:
      {char *cmdline;
      BYTE i;
      BYTE x,y;
      HDC myDC,*hDC;
      RECT rc;
      GetClientRect(hWnd,&rc);
//      SetTextColor(hDC,textColors[GetWindowByte(hWnd,2) & 15]);
			x=GetWindowByte(hWnd,0); y=GetWindowByte(hWnd,1);
      switch(wParam) {
        case VK_ESCAPE:
          if(activeMenu)   // 
            return DefWindowProc(hWnd,message,wParam,lParam);
          else {
            cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
            cmdline[0]=0;
						x=0;
						y++;
            }
          break;
        case VK_BACK:
          cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
          i=strlen(cmdline);
          if(i > 0) {
            cmdline[--i]=0;
						x--;
            }
          ch=' ';
          goto putchar;
          break;
        case VK_RETURN:
					x=0; y++;
          if(y>rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
    			SetWindowByte(hWnd,0,x); SetWindowByte(hWnd,1,y);
          KillTimer(hWnd,1);
          
          hDC=GetDC(hWnd,&myDC);
//      TextOut(hDC,rc.left+2+6*strlen(prompt)+x*6,rc.top+2+y*8," "); FARE per pulire cursore!
          m_stdout=/*m_stdin=*/m_stderr=DEVICE_WINDOW;
          cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
          execCmd(cmdline,NULL,cmdline);
          cmdline[0]=0;
          m_stdout=m_stdin=m_stderr=DEVICE_CON;   // direi di togliere e lasciare fisso DEVICE_WINDOW (vedi InitWindows
     			x=GetWindowByte(hWnd,0); y=GetWindowByte(hWnd,1);
          SetTimer(hWnd,1,500,NULL);    // per cursore...
					x=0; y++;
          if(y>rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
          TextOut(hDC,rc.left+2+x*6,rc.top+1+y*8,prompt);
          ReleaseDC(hWnd,hDC);
          break;
        default:
          ch=LOBYTE(wParam);
          if(isprint(ch)) {
            cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
            i=strlen(cmdline);
            if(i < 31) {
              cmdline[i++]=ch;
              cmdline[i]=0;
							x++;
              lParam=0xffffffff;
              goto putchar;
              }
            else {
              MessageBeep(0);   // bah togliere per tasti sistema :)
              return DefWindowProc(hWnd,message,wParam,lParam);
              }
            }
          break;
        }
			if(y>rc.bottom/8) {
				y--;
        ScrollWindow(hWnd,0,-8,&rc,NULL);
				}
			SetWindowByte(hWnd,0,x); SetWindowByte(hWnd,1,y);
      }
      break;
      
    case WM_PRINTCHAR:
      {
      HDC myDC,*hDC;
      BYTE x,y;
      RECT rc;
      i=WHITE;
      ch=LOBYTE(wParam);
      
putchar:
      // o FARE InvalidateRect(hWnd,NULL,TRUE) ??
      x=GetWindowByte(hWnd,0); y=GetWindowByte(hWnd,1);
      GetClientRect(hWnd,&rc);
      hDC=GetDC(hWnd,&myDC);
      if(lParam!=0xffffffff)
        SetTextColor(hDC,LOWORD(lParam));
      else
        SetTextColor(hDC,textColors[GetWindowByte(hWnd,2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,3) & 15]);
      switch(ch) {
        case '\t':
//fare          x=0;
          break;
        case 0x0c:
          FillRect(hDC,&rc,CreateSolidBrush(textColors[GetWindowByte(hWnd,3) & 15])); 
          // o fare semplicemente un InvalidateRect(mInstance->hWnd,NULL,TRUE) ??
          x=y=0;
     			SetWindowByte(hWnd,0,0); SetWindowByte(hWnd,1,0);
          break;
        case '\r':
          x=0;
          break;
        case '\n':
          x=0; y++;
          if(y>rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
          break;
        default:
          if(isprint(ch)) {
            DrawCharWindow(hDC,2+6*strlen(prompt)+x*6,2+y*8,ch);
            x++;
            if(x>=rc.right/6) {
              x=0;
              if(y>rc.bottom/8) {
                y--;
                ScrollWindow(hWnd,0,-8,&rc,NULL);
                }
              }
            }
          break;
        }
      SetWindowByte(hWnd,0,x); SetWindowByte(hWnd,1,y);
      ReleaseDC(hWnd,hDC);
      }
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }

  }

LRESULT DefWindowProcDC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC *hDC=BeginPaint(hWnd,&ps);
      RECT rc;
			BYTE i;
      BYTE attrib=GetWindowByte(hWnd,2+2+sizeof(FONT)+16+sizeof(S_POINT)*16);
      HWND myWnd;

      
      hDC->pen=CreatePen(PS_SOLID,1,WHITE);
      hDC->brush=CreateSolidBrush(desktopColor);
      SetTextColor(hDC,WHITE);
      SetBkColor(hDC,desktopColor);
      hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font;
//      hDC->cursor.x=Screen.cx/2; hDC->cursor.y=Screen.cy/2;
      GetClientRect(hWnd,&rc);

      myWnd=rootWindows;
      while(myWnd) {
        if(myWnd->visible) {
          HWND myWnd2=GetForegroundWindow();
          DrawIcon8(hDC,8+((myWnd2 ? myWnd2->zOrder : 1)-myWnd->zOrder)*12 /* :) */,
                  Screen.cy-16-12,myWnd->icon);  // v. anche Desktopclass e taskbar e nonclientpaint
//          SPOSTARE IN TASKBAR?? stile xp
          }
        myWnd=myWnd->next;
        }
      
      EndPaint(hWnd,&ps);
      }
      return 1;
      break;
    case WM_ERASEBKGND:
      { 
      RECT rc;
      HDC *hDC=(HDC *)wParam;
      GetClientRect(hWnd,&rc);
      BYTE attrib=GetWindowByte(hWnd,2+2+sizeof(FONT)+16+sizeof(S_POINT)*16);
      GFX_COLOR f=MAKEWORD(GetWindowByte(hWnd,0),GetWindowByte(hWnd,1)),b=MAKEWORD(GetWindowByte(hWnd,2),GetWindowByte(hWnd,3));
			char *fileWallpaper=(char *)GET_WINDOW_OFFSET(hWnd,2+2+sizeof(FONT));
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      jpegPtr=NULL;
      
      DrawWindow(rc.left,rc.top,rc.right,rc.bottom,b,b);
      
extern const unsigned char W95train[45398];
extern const unsigned char W95lion[42090];
extern const unsigned char W3tartan[5132];
extern const unsigned char WindowXP[49741];

      if(!strcmp(fileWallpaper,"w95lion.jpg")) {
        jpegPtr=(BYTE*)W95lion; jpegLen=sizeof(W95lion);
        }
      else if(!strcmp(fileWallpaper,"w95train.jpg")) {
        jpegPtr=(BYTE*)W95train; jpegLen=sizeof(W95train);
        }
      else if(!strcmp(fileWallpaper,"w3tartan.jpg")) {
        jpegPtr=(BYTE*)W3tartan; jpegLen=sizeof(W3tartan);
        }
      else if(!strcmp(fileWallpaper,"windowxp.jpg")) {
        jpegPtr=(BYTE*)WindowXP; jpegLen=sizeof(WindowXP);
        }
      else {
        jpegFile=malloc(sizeof(SUPERFILE));
        if(SuperFileOpen(jpegFile,fileWallpaper,'r')) {   // aprire file "vero" !
          }
        else {
          free(jpegFile); jpegFile=NULL;
          hDC->font=CreateFont(32,20,0,0,FW_NORMAL /*FW_BOLD*/,TRUE,FALSE,FALSE,ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_ROMAN,NULL);
          img_ofs_y=(rc.bottom-16)/2; img_ofs_x=(rc.right-strlen(fileWallpaper)*20)/2;
          SetTextColor(hDC,BRIGHTYELLOW);
          SetBkColor(hDC,desktopColor);
          TextOut(hDC,img_ofs_x,img_ofs_y,fileWallpaper);
          hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font;

//        SetTextColor(hDC,WHITE);
//        TextOut(hDC,(rc.right-rc.left-12*6)/2,0,"BREAKTHROUGH");
          }
        }
			if(jpegPtr || jpegFile) {
				int x,x1,mcu_x=0;
				int y,y1,mcu_y=0;
	      pjpeg_image_info_t JPG_Info;
        bool status;
//				status=pjpeg_decode_init(&JPG_Info, pjpeg_need_bytes_callback, NULL,isFile);
				if(!(status=pjpeg_decode_init(&JPG_Info,pjpeg_need_bytes_callback,NULL,(DWORD)jpegFile))) {
    			if(attrib & 1) {   // tiled
            img_ofs_y=img_ofs_x=0;
            }
          else {
            img_ofs_y=(rc.bottom-JPG_Info.m_height)/2; img_ofs_x=(rc.right-JPG_Info.m_width)/2;
            }
          if(img_ofs_x<0) img_ofs_x=0;
          if(img_ofs_y<0) img_ofs_y=0;
          
rifo:
					for(;;) {
						if(status = pjpeg_decode_mcu())
							goto error_compressed;

						for(y=0; y < JPG_Info.m_MCUHeight; y += 8) {
							y1=(mcu_y*JPG_Info.m_MCUHeight) + y;
							for(x=0; x < JPG_Info.m_MCUWidth; x += 8) {
								x1=(mcu_x*JPG_Info.m_MCUWidth) + x  /* * JPG_Info.m_comps*/;

								// Compute source byte offset of the block in the decoder's MCU buffer.
								uint32_t src_ofs = (x * 8) + (y * 16);
								const uint8_t *pSrcR = JPG_Info.m_pMCUBufR + src_ofs;
								const uint8_t *pSrcG = JPG_Info.m_pMCUBufG + src_ofs;
								const uint8_t *pSrcB = JPG_Info.m_pMCUBufB + src_ofs;

								uint8_t bx,by;
								for(by=0; by<8; by++) {
									for(bx=0; bx<8; bx++) {
										DrawPixel(img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
										// USARE BitBlt(img_ofs_x+x1,img_ofs_y+y1,8,8, )  !
										pSrcR++; pSrcG++; pSrcB++;
										}
									}
								}
	//                  x1+=JPG_Info.m_MCUWidth;
							}
	//                y1+=JPG_Info.m_MCUHeight;

						mcu_x++;      // in x ogni blocco è già 16 pixel (con YUV, pare)
						if(mcu_x >= JPG_Info.m_MCUSPerRow) {
							mcu_x=0;
							mcu_y++;
	//                  if(mcu_y == JPG_Info.m_MCUSPerCol) {
	//                    break;
	//                   }
							}
						ClrWdt();
						}
					}
   			if(attrib & 1) {   // tiled
  				if(!(status=pjpeg_decode_init(&JPG_Info, pjpeg_need_bytes_callback, NULL,0))) {
            img_ofs_x+=JPG_Info.m_width;
            if(img_ofs_x>rc.right+JPG_Info.m_width) {
              img_ofs_x=0;
              img_ofs_y+=JPG_Info.m_height;
              if(img_ofs_y<rc.bottom+JPG_Info.m_height)
                goto rifo;
              }
            }
          }
        
error_compressed:
error_jpeg:
					;
        if(jpegFile) {
          SuperFileClose(jpegFile);
          free(jpegFile);
          jpegFile=NULL;
          }
				}
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
			memset(GET_WINDOW_OFFSET(hWnd,0),2+2+sizeof(FONT)+16+sizeof(S_POINT)*16+1,0);
      if(cs->lpCreateParams)
        strcpy(GET_WINDOW_OFFSET(hWnd,2+2+sizeof(FONT)),cs->lpCreateParams);
      SetWindowLong(hWnd,0,MAKELONG(BRIGHTYELLOW,desktopColor));
      }
      return 0;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcTaskBar(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
#if 0
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC *hDC=BeginPaint(hWnd,&ps);
      RECT rc;
			BYTE i;
      BYTE attrib=GetWindowByte(hWnd,4+4);

#if 0
      myWnd=rootWindows;
      while(myWnd) {
        if(myWnd->visible) {
          HWND myWnd2=GetForegroundWindow();
          DrawIcon8(hDC,8+((myWnd2 ? myWnd2->zOrder : 1)-myWnd->zOrder)*12 /* :) */,
                  Screen.cy-16-12,myWnd->icon);  // v. anche Desktopclass e taskbar e nonclientpaint
          }
        myWnd=myWnd->next;
        }
#endif
			if(i & 2) {
//				on top
				}

      EndPaint(hWnd,&ps);
      }
      return 1;
      break;
#endif
    case WM_ERASEBKGND:
      { 
      RECT rc;
      HDC *hDC=(HDC *)wParam;
      BYTE attrib=GetWindowByte(hWnd,4+4);
      GetClientRect(hWnd,&rc);
      FillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,hDC->brush.color);
      return 1;
      }
      break;
    case WM_SHOWWINDOW:
      InvalidateRect(hWnd,NULL,TRUE);     // qua gestisco il caso particolare... SW_OTHERUNZOOM SW_OTHERZOOM
			return 0;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
			memset(GET_WINDOW_OFFSET(hWnd,0),4+4+1,0);
      BYTE attrib=(DWORD)cs->lpCreateParams;
      SetWindowByte(hWnd,4+4,attrib);
      HWND myWnd=CreateWindow(MAKECLASS(WC_BUTTON),"Start",WS_BORDER | WS_VISIBLE | WS_TABSTOP |
        WS_CHILD | BS_CENTER | BS_PUSHBUTTON,
        1,1,5*6+2,10,
        hWnd,(MENU *)1,NULL
        );
      SetWindowLong(hWnd,0,(DWORD)myWnd);
			if(attrib & 1) {   // orologio
        myWnd=CreateWindow(MAKECLASS(WC_STATIC),"00:00",WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED | SS_CENTER,  // in effetti potrebbe essere attivo per set clock..
          cs->cx-5*6-5,1,5*6+2,10,
          hWnd,(MENU *)2,NULL
          );
   			SetWindowLong(hWnd,4,(DWORD)myWnd);
        SetTimer(hWnd,1,60000,NULL);
				}
      if(attrib & 2) {
//				on top
				}
      }
      return 0;
      break;
    case WM_TIMER:
      {
 			HWND myWnd=(HWND)GetWindowLong(hWnd,4);
      if(myWnd) {
        PIC32_DATE date;
        PIC32_TIME time;
        char buf[16];
        SetTimeFromNow(now,&date,&time);
        sprintf(buf,"%02u:%02u",time.hour,time.min);
        SetWindowText(myWnd,buf);
        }
      }
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

  
static void calculateClientArea(HWND hWnd,RECT *rc) {
  
  *rc=hWnd->nonClientArea;
  if(!hWnd->minimized) {
    if(hWnd->style & WS_BORDER) {
      rc->top++;
      rc->bottom--;
      rc->left++;
      rc->right--;
      if(hWnd->style & WS_THICKFRAME) {
        rc->top++;
        rc->bottom--;
        rc->left++;
        rc->right--;
        }
      }
    if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION)) {
      rc->top+=TITLE_HEIGHT;
      }
    if(!(hWnd->style & WS_CHILD) && hWnd->menu) {
      rc->top+=MENU_HEIGHT+1;
      }
    if(hWnd->style & WS_HSCROLL) {
      rc->bottom-= hWnd->style & WS_BORDER ? SCROLL_SIZE-1 : SCROLL_SIZE; // se c'è bordo, lo scroll usa il bordo, se no no!
      }
    if(hWnd->style & WS_VSCROLL) {
      rc->right-= hWnd->style & WS_BORDER ? SCROLL_SIZE-1 : SCROLL_SIZE;
      }
    }
  
  else {
    }
  
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_BYTEALIGNCLIENT) {
    rc->left = (rc->left + 7) & 0xfff8;   // boh credo :)
    rc->right &= 0xfff8;
    }
  
/*  if(!IsRectEmpty(&hWnd->paintArea)) {    // patchina per chi non fa GetDC (v. minibasic..)
    hWnd->paintArea.left=hWnd->clientArea.left;
    hWnd->paintArea.top=hWnd->clientArea.top;
    hWnd->paintArea.right=hWnd->clientArea.right;
    hWnd->paintArea.bottom=hWnd->clientArea.bottom;
    }*/

  }

static void calculateNonClientArea(HWND hWnd,RECT *rc) {

  if(!hWnd->minimized) {
    if(hWnd->style & (WS_HSCROLL | WS_VSCROLL)) {
  //    hWnd->scrollSizeX= rc->right-rc->left - (hWnd->style & WS_VSCROLL ? SCROLL_SIZE : 0);
      // forse pure -border...
  //    hWnd->scrollSizeX /= 3;   // test..
  //    hWnd->scrollSizeY=rc->bottom-rc->top  - (hWnd->style & WS_HSCROLL ? SCROLL_SIZE : 0)
  //      - (hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION) ? TITLE_HEIGHT : 0);
  //    hWnd->scrollSizeY /= 3;   // test..
      }
    else {
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      }
    }
  }

BOOL GetWindowInfo(HWND hWnd,WINDOWINFO *pwi) {
  
  pwi->wCreatorVersion=MAKEWORD(BREAKTHROUGH_VERSION_L,BREAKTHROUGH_VERSION_H);
  pwi->atomWindowType=0;
  pwi->rcWindow=hWnd->nonClientArea;
  pwi->rcClient=hWnd->clientArea;
  pwi->dwStyle=hWnd->style;
//  pwi->dwExStyle=hWnd->styleEx;
  pwi->dwWindowStatus=hWnd->status;
  pwi->cxWindowBorders=BORDER_SIZE;
  pwi->cyWindowBorders=BORDER_SIZE;
  
  return TRUE;
  }

// ----------------------------------------------------------------------------
int RegisterClass(const WNDCLASS *lpWndClass) {
  WNDCLASS *myClass;
  
  if(GetClassInfo(0,lpWndClass->class,&myClass))    // controllo che non ci sia già...
    return 0;
  if(!rootClasses)
    rootClasses=(WNDCLASS *)lpWndClass;
  else {
    myClass=rootClasses;
    while(myClass && myClass->next) {
      myClass=myClass->next;
      }
    myClass->next=(WNDCLASS *)lpWndClass;
    }
  return 1;
  }

BOOL UnregisterClass(CLASS Class, HINSTANCE hInstance) {
  WNDCLASS *myClass,*myClass2,*theClass;
  
  if(GetClassInfo(0,Class,&theClass)) {    // DOVREBBE SALTARE quelle predefinite!
    myClass=rootClasses;
    while(myClass) {
      myClass2=myClass;
      if(myClass==theClass) {
        if(myClass==rootClasses)
          rootClasses=myClass->next;
        else
          myClass2->next=myClass->next;
        return 1;
        }
      myClass=myClass->next;
      }
    }
  return 0;
  }

HWND FindWindow(const CLASS *Class, const char *lpWindowName) {
  HWND myWnd;
  
  myWnd=rootWindows;
  while(myWnd) {
    if(Class) {
      if(myWnd->class.class==Class->class) {
        return myWnd;
        }
      else 
        continue;
      }
    if(lpWindowName) {
      if(!stricmp(myWnd->caption,lpWindowName)) {
        return myWnd;
        }
      else 
        continue;
      }
    myWnd=myWnd->next;
    }
  
  return NULL;
  }

HWND FindWindowEx(HWND hWndParent,HWND hWndChildAfter,const CLASS *Class, const char *lpWindowName) {
  HWND myWnd;
  
  if(hWndParent)
    myWnd=hWndParent;
  else
    myWnd=rootWindows /* FINIRE GetDesktopWindow()*/;
  myWnd=myWnd->children;
//  if(!myWnd)
//    return NULL;
  if(hWndChildAfter) {
    while(myWnd) {
      if(myWnd==hWndChildAfter) {
        myWnd=myWnd->next;
        break;
        }
      myWnd=myWnd->next;
      }
    }
  while(myWnd) {
    if(Class) {
      if(myWnd->class.class==Class->class) {
        return myWnd;
        }
      else 
        continue;
      }
    if(lpWindowName) {
      if(!stricmp(myWnd->caption,lpWindowName)) {
        return myWnd;
        }
      else 
        continue;
      }
    myWnd=myWnd->next;
    }
  
  return NULL;
  }

int GetClassName(HWND hWnd,CLASS *Class) {
  
  *Class=hWnd->class;
  }

DWORD GetClassLong(HWND hWnd,int nIndex) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  return *((DWORD *)GET_CLASS_OFFSET(wc,nIndex));
  }

WORD GetClassWORD(HWND hWnd,int nIndex) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  return MAKEWORD(*GET_CLASS_OFFSET(wc,nIndex),*GET_CLASS_OFFSET(wc,nIndex+1));
  }

DWORD GetWindowLong(HWND hWnd,int nIndex) {
  
  return *((DWORD *)GET_WINDOW_OFFSET(hWnd,nIndex));
  }

void SetClassLong(HWND hWnd,int nIndex,DWORD value) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  *((DWORD *)GET_CLASS_OFFSET(wc,nIndex))=value;
  }

void SetWindowLong(HWND hWnd,int nIndex,DWORD value) {
  
  *((DWORD *)GET_WINDOW_OFFSET(hWnd,nIndex))=value;
  }

BYTE GetClassByte(HWND hWnd,int nIndex) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  return *((BYTE *)GET_CLASS_OFFSET(wc,nIndex));
  }

BYTE GetWindowByte(HWND hWnd,int nIndex) {
  
  return *((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex));
  }

void SetClassByte(HWND hWnd,int nIndex,BYTE value) {
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  *((BYTE *)GET_CLASS_OFFSET(wc,nIndex))=value;
  }

void SetWindowByte(HWND hWnd,int nIndex,BYTE value) {
  
  *((BYTE *)GET_WINDOW_OFFSET(hWnd,nIndex))=value;
  }


static BOOL nonClientPaint(HWND hWnd,const RECT *rc) {
  WORD newtop,newbottom,newleft,newright;
  HDC myDC,*hDC;
  UGRAPH_COORD_T myx,myy;
  union {
    struct {
      unsigned int drawCaption:1;
      unsigned int drawMenu:1;
      unsigned int drawBorder:1;
      unsigned int unused3:1;
      unsigned int drawHScroll:1;
      unsigned int drawVScroll:1;
      unsigned int drawClient:1;
      unsigned int drawBackground:1;
      };
    BYTE b;
    } mask;    //

//#ifndef USING_SIMULATOR
  if(!(hWnd->style & WS_VISIBLE))
    return 0;

  newtop=0;
  newbottom=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
  newleft=0;
  newright=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  
  if(!rc)
    mask.b=0xff;
  else {
    mask.b=0;
    if(rc->top>=hWnd->nonClientArea.top && rc->bottom<=hWnd->nonClientArea.top+TITLE_HEIGHT)
      mask.drawCaption = 1;
    if(rc->top>=hWnd->nonClientArea.top+TITLE_HEIGHT && rc->bottom<=hWnd->nonClientArea.top+TITLE_HEIGHT+MENU_HEIGHT)
      // in effetti, andrebbe controllato se c'è il titolo o il menu..
      mask.drawMenu = 1;
    if(rc->top>=hWnd->nonClientArea.bottom-SCROLL_SIZE && rc->bottom<=hWnd->nonClientArea.bottom)
      mask.drawHScroll = 1;
    if(rc->top>=hWnd->nonClientArea.top+TITLE_HEIGHT && rc->bottom<=hWnd->nonClientArea.bottom) {
      if(rc->left>=hWnd->nonClientArea.right-SCROLL_SIZE && rc->right<=hWnd->nonClientArea.right)
        mask.drawVScroll = 1;
      mask.drawClient=1;
      }
    }
  
  hDC=GetWindowDC(hWnd,&myDC);
  
  if(hWnd->minimized) {
    // FINIRE! disegnare icona dove??
//    DrawIcon8(&m_Wnd3->hDC,0,0,hWnd->icon);
//    DrawIcon8(&hWnd->hDC,4+(maxWindows-hWnd->zOrder)*10 /* :) */,_height-16,hWnd->icon);
//    DrawIcon8(GetWindowDC(GetDesktopWindow)),4+(maxWindows-hWnd->zOrder)*10 /* :) */,_height-16,hWnd->icon);
    
    ReleaseDC(hWnd,hDC);
    
    HWND myWnd=GetForegroundWindow();
    int x=myWnd ? myWnd->zOrder : 1;    // v. desktopclass
    hDC=GetWindowDC(GetDesktopWindow(),&myDC);
    DrawIcon8(hDC,8+(x-hWnd->zOrder)*12 /* :) */,Screen.cy-16-12,
      hWnd->icon ? hWnd->icon : standardIcon);
    
    ReleaseDC(GetDesktopWindow(),hDC);
    return;
    }
  if(hWnd->maximized) {
    // FINIRE! o forse non serve altro, una volta cambiate le dimensioni..
    }
  
  hDC->foreColor=hDC->pen.color=hWnd->active ? windowForeColor : windowInactiveForeColor;
  hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font; /*cmq lo stock? o anche custom?? */
  // cmq già in GetWindowDC ..
  
  // e hWnd->enabled ??
  
  if(mask.drawBackground) {
    FillRectangleWindowColor(hDC,newleft,newtop,newright-1,newbottom-1,windowBackColor);
    }
  if(mask.drawBorder) {
    if(hWnd->style & WS_BORDER) {		// questo sempre
      DrawRectangleWindow(hDC,newleft,newtop,newright-1,newbottom-1);
      if(hWnd->style & WS_THICKFRAME) {
        DrawRectangleWindow(hDC,newleft+1,newtop+1,newright-1-1,newbottom-1-1);
        }
      }
    }
  if(hWnd->style & WS_THICKFRAME) {
    newtop++;
    newbottom--;
    newleft++;
    newright--;
    }
  
  if(mask.drawCaption) {
    if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION)) {
      DrawVertLineWindow(hDC,newleft+TITLE_ICON_WIDTH+2,newtop,newtop+TITLE_HEIGHT);
      }
    if(hWnd->style & WS_SYSMENU) {
      if(!(hWnd->style & WS_CHILD)) {
        FillRectangleWindowColor(hDC,newleft+2,newtop+2,newleft+TITLE_ICON_WIDTH,newtop+TITLE_ICON_WIDTH,
              hDC->pen.color);
        newleft=newleft+TITLE_ICON_WIDTH+2;
        }
      DrawLineWindow(hDC,newright-3,newtop+3,newright-TITLE_ICON_WIDTH+1,newtop+TITLE_ICON_WIDTH-1);
      DrawLineWindow(hDC,newright-TITLE_ICON_WIDTH+1,newtop+3,newright-3,newtop+TITLE_ICON_WIDTH-1);
      DrawVertLineWindow(hDC,newright-(TITLE_ICON_WIDTH+1),newtop,newtop+TITLE_HEIGHT);
      newright -= TITLE_ICON_WIDTH+1;
      }
    if(hWnd->style & WS_MAXIMIZEBOX) {
      if(hWnd->maximized) {
        DrawRectangleWindow(hDC,newright-(TITLE_ICON_WIDTH-1)+1,newtop+3,newright-2,newtop+6);
        }
      else {
        DrawLineWindow(hDC,newright-2,newtop+TITLE_ICON_WIDTH/2+2,newright-(TITLE_ICON_WIDTH-1)/2-1,newtop+3);
        DrawLineWindow(hDC,newright-(TITLE_ICON_WIDTH-1)/2-1,newtop+3,newright-(TITLE_ICON_WIDTH-1)+1,newtop+TITLE_ICON_WIDTH/2+2);
        }
      DrawVertLineWindow(hDC,newright-TITLE_ICON_WIDTH,newtop,newtop+TITLE_HEIGHT);
      newright -= TITLE_ICON_WIDTH;
      }
    if(hWnd->style & WS_MINIMIZEBOX) {
      DrawLineWindow(hDC,newright-2,newtop+3,newright-(TITLE_ICON_WIDTH-1)/2-1,newtop+TITLE_ICON_WIDTH/2+2);
      DrawLineWindow(hDC,newright-(TITLE_ICON_WIDTH-1)/2-1,newtop+TITLE_ICON_WIDTH/2+2,newright-(TITLE_ICON_WIDTH-1)+1,newtop+3);
      DrawVertLineWindow(hDC,newright-TITLE_ICON_WIDTH,newtop,newtop+TITLE_HEIGHT);
      newright -= TITLE_ICON_WIDTH;
      }
    if(hWnd->style & WS_CAPTION) {
      // USARE DrawFrameControl(hDC,&rc,DFC_CAPTION,0);
      char *s=hWnd->caption;
      myx=newleft+2; myy=newtop+2;
      while(*s) {   // nonclient area, non uso TextOut...
        DrawCharWindow(hDC,myx,myy,*s++);
        myx += 6;
        if(myx>=(newright-6))   // 1 sola riga, trim a dx
          break;
        }
      while(myx<(newright-6)) {   // pulisco! potrei fare un rettangolo pieno prima... maok
        DrawCharWindow(hDC,myx,myy,' ');
        myx += 6;
        }
      }
    }

  // ripristino
  newleft=0;
  newright=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  if(hWnd->style & WS_THICKFRAME) {
    newleft++;
    newright--;
    }
  
  if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION)) {
    if(mask.drawCaption || mask.drawMenu) {
      DrawHorizLineWindow(hDC,newleft,newtop+TITLE_HEIGHT,newright-1);
      }
    newtop=newtop+TITLE_HEIGHT;
    }
  if(hWnd->menu) {
    if(!(hWnd->style & WS_CHILD)) {
      if(mask.drawMenu) {
        DrawHorizLineWindow(hDC,newleft,newtop+MENU_HEIGHT+1,newright-1);
        DrawMenuBar(hWnd);  // o dovrebbe mandare WM_INITMENU?
        }
      newtop=newtop+MENU_HEIGHT+1;
      }
    }
  if(mask.drawHScroll) {
    if(hWnd->style & WS_HSCROLL) {
      DrawHorizLineWindow(hDC,newleft,newbottom-SCROLL_SIZE,newright-1);
      FillRectangleWindowColor(hDC,newleft+2+hWnd->scrollPosX,newbottom-SCROLL_SIZE+2,
              newleft+2+hWnd->scrollPosX+hWnd->scrollSizeX,newbottom-2,hDC->pen.color);
      }
    else {
//      FillRectangleWindowColor(hDC,newleft+2,newbottom-SCROLL_SIZE, // non è perfetto... il colore.. forse basterebbe lasciare alla client area...
 //             newright-2,newbottom-2,hDC->brush.color);
      }
    }
  if(mask.drawVScroll) {
    if(hWnd->style & WS_VSCROLL) {
      DrawVertLineWindow(hDC,newright-SCROLL_SIZE,newtop,newbottom-1);
      FillRectangleWindowColor(hDC,newright-SCROLL_SIZE+2,newtop+2+hWnd->scrollPosY,
              newright-2,newtop+2+hWnd->scrollPosY+hWnd->scrollSizeY-1,hDC->pen.color);
      }
    else {
//      FillRectangleWindowColor(hDC,newright-SCROLL_SIZE,newtop+2,
 //             newright-2,newbottom-2,hDC->brush.color);
      }
    }
  if(hWnd->style & WS_SIZEBOX) {
    if(mask.b) {    // bah diciamo :)
      DrawLineWindow(hDC,newright-1,newbottom-TITLE_ICON_WIDTH,newright-TITLE_ICON_WIDTH,newbottom-1);
      }
    }

  ReleaseDC(hWnd,hDC);
//#endif
  
  calculateClientArea(hWnd,&hWnd->clientArea);    // mah io penso sia giusto sempre qua..

  return 1;
  }

static BOOL clientPaint(HWND hWnd,const RECT *rc) {
  HWND myWnd;
  
  if(!rc) {
    rc=&hWnd->paintArea;
    }
  
  if(!(hWnd->style & WS_VISIBLE))
    return;
  
  // ora ridisegno tutte le child
  myWnd=hWnd->children;
  while(myWnd) {
    RECT rc2,rc3;

    if(myWnd->visible) {
      OffsetRect2(&rc2,&myWnd->nonClientArea,hWnd->clientArea.left,hWnd->clientArea.top);
            
      WS_CLIPSIBLINGS; //gestire


      if(IntersectRect(&rc3,&rc2,rc)) {
        SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);
        InvalidateRect(myWnd,NULL     /*&rc3*/,TRUE);    // 
        
        HWND myWnd2=myWnd->children;
        while(myWnd2) {
          RECT rc4,rc5;

          if(myWnd2->visible) {
            OffsetRect2(&rc4,&myWnd2->nonClientArea,
                    rc2.left /*+myWnd->clientArea.left*/,rc2.top /*+myWnd->clientArea.top*/);
            // v. anche windowfrompoint

            WS_CLIPSIBLINGS; //gestire


            if(IntersectRect(&rc5,&rc4,&rc3)) {
              SendMessage(myWnd2,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);
              InvalidateRect(myWnd2,NULL     /*&rc3*/,TRUE);
              }
            }
          myWnd2=myWnd2->next;
          }

        }
      }
    myWnd=myWnd->next;
    }

	SetRectEmpty(&hWnd->paintArea);   // facendo così poi tutte le line/ecc non vanno, giustamente...
  // non saprei come fare, per ora così ossia sempre tutto abilitato, v. BeginPaint
//    hWnd->paintArea=hWnd->hDC.area;

  return 1;
  }

BOOL InvalidateRect(HWND hWnd,const RECT *rc,BOOL bErase) {

  if(!hWnd) {   // https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-invalidaterect
    PaintDesktop(NULL,0);
    
    {HWND myWnd=rootWindows;
    while(myWnd) {
      SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);		// ncpaint fa/implica pure paint qua...
      InvalidateRect(myWnd,NULL,TRUE);
      
#warning usare qua, ma dentro nc_paint
			// magari legato a CS_OWNDC o CS_SAVEBITS ?
//      DrawWindow(myWnd->nonClientArea.left,myWnd->nonClientArea.top,
//              myWnd->nonClientArea.right,myWnd->nonClientArea.bottom,
//              windowInactiveForeColor,windowBackColor); // se la finestra è sotto le altre, come qua!
      
      myWnd=myWnd->next;
      }
    }

    DrawCursor(mousePosition.x,mousePosition.y,standardCursor,0);
    // in teoria prendere quello di WindowFromPoint(mousePosition);

    }
  else {
    if(!rc) {
      rc=&hWnd->clientArea;
//      GetClientRect(hWnd,&hWnd->hDC.paintArea);
      }
    UnionRect(&hWnd->paintArea,&hWnd->paintArea,rc);    // accumulo le aree paint...
    SendMessage(hWnd,WM_PAINT,0,0);   // normalmente non sarebbe qua ma fatto dal sistema quando la coda è vuota...
    // v. anche updatewindow
    }
  
  return 1;
  }

BOOL GetUpdateRect(HWND hWnd,RECT *lpRect,BOOL bErase) {

	if(lpRect)
		*lpRect=hWnd->paintArea;
	else
		return !IsRectEmpty(&hWnd->paintArea);

	if(bErase && !IsRectEmpty(&hWnd->paintArea)) {
    PAINTSTRUCT ps;
    HDC *hDC=BeginPaint(hWnd,&ps);
//		SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
    EndPaint(hWnd,&ps);
    }
	}


WORD SetTimer(HWND hWnd,WORD nIDEvent,WORD uElapse,TIMERPROC *lpTimerFunc) {
  BYTE i,j;
  
  if(hWnd) {
    for(j=0; j<MAX_TIMERS; j++) {
      if(hWnd==timerData[j].hWnd && timerData[j].uEvent == nIDEvent) {
        i=j;
        goto replacetimer;
        break;
        }
      }
    }
  else {
    for(j=0; j<MAX_TIMERS; j++) {
      if(timerData[j].uEvent == nIDEvent) {
        i=j;
        goto replacetimer;
        break;
        }
      }
    }
//bah... If the hWnd parameter is NULL, and the nIDEvent does not match an existing timer then it is ignored and a new timer ID is generated.
  for(i=0; i<MAX_TIMERS; i++) {
    

    if(!timerData[i].uEvent) {
      timerData[i].hWnd=hWnd;
      timerData[i].uEvent=nIDEvent;
replacetimer:      
      uElapse=max(uElapse,USER_TIMER_MINIMUM);
      uElapse=min(uElapse,USER_TIMER_MAXIMUM);
      timerData[i].time_cnt=timerData[i].timeout=uElapse;  
      timerData[i].tproc=lpTimerFunc;
      return i+1;
      }
    }
  return 0;
  }

BOOL KillTimer(HWND hWnd,WORD uIDEvent) {
  BYTE i;
  
  for(i=0; i<MAX_TIMERS; i++) {
    if(hWnd) {
      if(hWnd==timerData[i].hWnd && timerData[i].uEvent == uIDEvent)
        goto delete;
      }
    else {
      if(timerData[i].uEvent == uIDEvent) {
delete:
        timerData[i].uEvent=0;
        timerData[i].hWnd=0;
        timerData[i].timeout=0;
        timerData[i].tproc=NULL;
        timerData[i].time_cnt=0;
        return TRUE;
        }
      }
    }
  return FALSE;
  }

BOOL EnableWindow(HWND hWnd,BOOL bEnable) {
  
  SendMessage(hWnd,WM_ENABLE,bEnable,0);
  }

BOOL IsWindowVisible(HWND hWnd) {
  
  return hWnd->visible;
  }

BOOL IsWindowEnabled(HWND hWnd) {
  
  return hWnd->enabled;
  }

BOOL RectVisible(HDC *hDC,const RECT *lprect) {
  
  return lprect->left>hDC->area.left && lprect->right<hDC->area.right &&   // o hWnd->cliparea??
          lprect->top>hDC->area.top && lprect->bottom<hDC->area.bottom;
  }

BOOL PtVisible(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  
  return x>hDC->area.left && x<hDC->area.right &&   // o hWnd->cliparea??
          y>hDC->area.top && y<hDC->area.bottom;
  }

BOOL DestroyWindow(HWND hWnd) {

  SendMessage(hWnd,WM_DESTROY,0,0);
  SendMessage(hWnd,WM_NCDESTROY,0,0);
  
  if(hWnd->style & WS_CHILD) {
    //if(!WS_EX_NOPARENTNOTIFY) mettere
    SendMessage(hWnd->parent,WM_PARENTNOTIFY,MAKEWORD(WM_DESTROY,(WORD)(DWORD)hWnd->menu),(LPARAM)hWnd); // tranne WS_EX_NOPARENTNOTIFY ..
		// child di child no?...
    if(hWnd->parent->style & WS_CHILD) {
//      DestroyWindow(hWnd);
      }
    
    HWND myWnd=hWnd->children;
    HWND myWnd2;
    while(myWnd) {
      myWnd2=myWnd->next;
      DestroyWindow(myWnd);
      myWnd=myWnd2;
      }
    
//    printf("remove child %X, %X\r\n",hWnd,hWnd->parent);
    
    removeWindow(hWnd,&hWnd->parent->children);
    list_bubble_sort(&hWnd->parent->children);
    }
  else {
    HWND myWnd=hWnd->children;
    HWND myWnd2;
    while(myWnd) {
      myWnd2=myWnd->next;
      DestroyWindow(myWnd);
      myWnd=myWnd2;
      }
    removeWindow(hWnd,&rootWindows);
    list_bubble_sort(&rootWindows);
    
    myWnd2=NULL;
    myWnd=rootWindows;
    while(myWnd) {    // ora attivo la più in alto rimasta
      myWnd2=myWnd;
//			printf("destr HWND: %x\r\n",myWnd);
      myWnd=myWnd->next;
      }
    if(myWnd2)    // direi che ci sarà sempre ;) ma...
      setActive(myWnd2,TRUE);
    }
  free(hWnd);
  
  InvalidateRect(NULL,NULL,TRUE);
  //usare interesectrect per ridisegnare desktop dopo destroy ecc
  }

BOOL CloseWindow(HWND hWnd) {
  
  ShowWindow(hWnd,SW_MINIMIZE);
  /*
  hWnd->minimized=1;
  SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
  SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
  SendMessage(hWnd,WM_PAINT,0,0);
  InvalidateRect( */
  }

BOOL RedrawWindow(HWND hWnd,const RECT *lprcUpdate,int /*HRGN*/ hrgnUpdate,WORD flags) {
  
  if(flags & RDW_ERASE) {
//    SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
    hWnd->paintArea=hWnd->clientArea;
    }
  if(flags & RDW_FRAME) {
    hWnd->paintArea=hWnd->nonClientArea;    // bah per ora ok
    }
  if(flags & RDW_INVALIDATE) {
//    lprcUpdate=hWnd->clientArea;    // tutto da verificare..
    }
  if(flags & RDW_ERASENOW) {
    if(flags & RDW_ALLCHILDREN) {
      }
    if(flags & RDW_NOCHILDREN) {
      }
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    InvalidateRect(hWnd,NULL,TRUE);
//    PostMessage(hWnd,WM_PAINT,0,0);
    }
  if(flags & RDW_UPDATENOW) {
    if(flags & RDW_ALLCHILDREN) {
      }
    if(flags & RDW_NOCHILDREN) {
      }
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    InvalidateRect(hWnd,NULL,TRUE);
//    SendMessage(hWnd,WM_PAINT,0,0);
    }
  }

#define BLOCK_LINES_2_SCROLL 8
BOOL ScrollWindow(HWND hWnd,int16_t XAmount,int16_t YAmount,const RECT *lpRect,const RECT *lpClipRect) {
  GFX_COLOR savedpixels[640*2*BLOCK_LINES_2_SCROLL];    // ovviamente cambiare con schermo più grande...
  int i,j,x,y,x1,y1,h,w,yResidual;
  GFX_COLOR *p3;
  RECT rc;
  
  if(!lpRect)
    GetClientRect(hWnd,&rc);
  else
    rc=*lpRect;
  OffsetRect(&rc,hWnd->clientArea.left,hWnd->clientArea.top);
  
  ShowCursor(FALSE); HideCaret(hWnd);
  
  x1=rc.left; y1=rc.top; w=rc.right-rc.left; h=rc.bottom-rc.top;
  
  Scroll(rc.left,rc.top,rc.right,rc.bottom,XAmount,YAmount);
  
  
  // finire! ovviamente con asse Z, ev. usare BitBlt
#if 0  
  do {
    yResidual=min(YAmount,BLOCK_LINES_2_SCROLL);
    // PER ORA SOLO IN SU! 30/11/21 quind sarebbe YAmount negativo :)
    p3=savedpixels;
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindowForRead(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        *p3++=readdata16();
        }
      }
    END_WRITE();
     * */

    p3=((GFX_COLOR *)savedCursorArea)+yResidual*w;
    // in teoria qua si potrebbe pure usare la Write in blocco, tanto siamo SOPRA tutto il resto!
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindow(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        writedata16(*p3++);
        }
      }
    END_WRITE();
     * */
    
    YAmount-=y;
    
    } while(YAmount>0);
#endif
  ShowCursor(TRUE); // DrawCaret();
  
  //The area uncovered by ScrollWindow is not repainted, but it is combined into the window's update region. 
  if(XAmount>0) {
    rc.left=0;
    rc.right=XAmount;
    }
  else if(XAmount<0) {
    rc.left=rc.right-XAmount;
    }
  if(YAmount>0) {
    rc.top=0;
    rc.bottom=XAmount;
    }
  else if(YAmount>0) {
    rc.top=rc.bottom-YAmount;
    }
  UnionRect(&hWnd->paintArea,&hWnd->paintArea,&rc);
  return TRUE;
  }

int ScrollWindowEx(HWND hWnd,int16_t dx,int16_t dy,const RECT *prcScroll,const RECT *prcClip,
  int /*HRGN*/ hrgnUpdate,RECT *prcUpdate,WORD flags) {
  GFX_COLOR savedpixels[320*2*BLOCK_LINES_2_SCROLL];    // ovviamente cambiare con schermo più grande...
  int i,j,x,y,x1,y1,h,w,yResidual;
  GFX_COLOR *p3;
  RECT rc;
  
  if(!prcScroll)
    GetClientRect(hWnd,&rc);
  else
    rc=*prcScroll;
  OffsetRect(&rc,hWnd->clientArea.left,hWnd->clientArea.top);
  
  ShowCursor(FALSE); HideCaret(hWnd);
  
  x1=rc.left; y1=rc.top; w=rc.right-rc.left; h=rc.bottom-rc.top;
  
  Scroll(rc.left,rc.top,rc.right,rc.bottom,dx,dy);
  
  // FINIRE! v. sopra
  do {
    yResidual=min(dy,BLOCK_LINES_2_SCROLL);
    // PER ORA SOLO IN SU! 30/11/21
    p3=savedpixels;
    
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindowForRead(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        *p3++=readdata16();
        }
      }
    END_WRITE();
     * */

    p3=((GFX_COLOR *)savedCursorArea)+yResidual*w;
    // in teoria qua si potrebbe pure usare la Write in blocco, tanto siamo SOPRA tutto il resto!
    /* GESTIRE SCROLL!
    START_WRITE();
    setAddrWindow(x1,y1,w,h);
    for(y=0; y<yResidual; y++) {
      for(i=0; i<w; i++) {
        writedata16(*p3++);
        }
      }
    END_WRITE();
     * */
    
    dy-=y;
    
    } while(dy>0);
    
  rc.top=rc.bottom-dy;     // FINIRE!

  if(prcUpdate) {
    *prcUpdate=rc;
    }
          
  if(flags & SW_ERASE) {
    InvalidateRect(hWnd,NULL /*hrgnUpdate*/,flags & SW_ERASE);
    }
          
  ShowCursor(TRUE); // DrawCaret();
  
  return TRUE;
  }

BOOL UpdateWindow(HWND hWnd) {
  
  return InvalidateRect(hWnd,NULL,TRUE);
  }
  
BOOL SetWindowPos(HWND hWnd,HWND hWndInsertAfter,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,
  UGRAPH_COORD_T cx,UGRAPH_COORD_T cy, DWORD uFlags) {
  RECT oldNCarea,oldArea;
  int temp;
  
  if(uFlags) {    // bah, controllare
    WINDOWPOS wpos;
    wpos.hwnd=hWnd;
    wpos.hwndInsertAfter=hWndInsertAfter;
    wpos.x=X;
    wpos.y=Y;
    wpos.cx=cx;
    wpos.cy=cy;
    wpos.flags=uFlags;
    SendMessage(hWnd,WM_WINDOWPOSCHANGING,0,(LPARAM)&wpos);
    }
  if(!(uFlags & SWP_NOACTIVATE)) {
    if(hWnd->enabled && !(hWnd->style & WS_CHILD))
  		setActive(hWnd,TRUE);
		}
  if(uFlags & SWP_HIDEWINDOW)
    hWnd->visible=0;
  if(uFlags & SWP_SHOWWINDOW)   // bah :) cmq vince questo
    hWnd->visible=1;
  if(!(uFlags & SWP_NOREPOSITION)) {
    oldNCarea=hWnd->nonClientArea;
    oldArea=hWnd->clientArea;
    if(!(uFlags & SWP_NOMOVE)) {
      hWnd->nonClientArea.left=X;
      hWnd->nonClientArea.top=Y;
      hWnd->clientArea.left+=hWnd->nonClientArea.left-oldNCarea.left;
      hWnd->clientArea.top+=hWnd->nonClientArea.top-oldNCarea.top;
      SendMessage(hWnd,WM_MOVE,0,MAKEWORD(hWnd->clientArea.left,hWnd->clientArea.top));
      }
    if(!(uFlags & SWP_NOSIZE)) {
      hWnd->nonClientArea.right=hWnd->nonClientArea.left+cx;
      hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+cy;
      hWnd->clientArea.right+=hWnd->nonClientArea.right-oldNCarea.right;
      hWnd->clientArea.bottom+=hWnd->nonClientArea.bottom-oldNCarea.bottom;
      SendMessage(hWnd,WM_SIZE,SIZE_RESTORED,MAKEWORD(hWnd->clientArea.right-hWnd->clientArea.left,hWnd->clientArea.bottom-hWnd->clientArea.top));
      }
    else {
      hWnd->nonClientArea.right+=hWnd->nonClientArea.left-oldNCarea.left;
      hWnd->nonClientArea.bottom+=hWnd->nonClientArea.top-oldNCarea.top;
      hWnd->clientArea.right+=hWnd->nonClientArea.left-oldNCarea.left;
      hWnd->clientArea.bottom+=hWnd->nonClientArea.top-oldNCarea.top;
      }
    }
  if(!(uFlags & SWP_NOZORDER /* SWP_NOOWNERZORDER che è? */)) {
    switch((int)hWndInsertAfter) {
      case (int)HWND_BOTTOM:
        hWnd->topmost=0;
        temp=rootWindows->zOrder;
        rootWindows->zOrder=hWnd->zOrder;
        hWnd->zOrder=temp;
        break;
      case (int)HWND_NOTOPMOST:
        hWnd->topmost=0;
				hWnd->style &= ~WS_EX_TOPMOST;
        break;
      case (int)HWND_TOP:
        hWnd->topmost=0;
hwnd_top:
        {
        HWND myWnd=rootWindows;
        while(myWnd)
          myWnd=myWnd->next;
        temp=myWnd->zOrder;
        myWnd->zOrder=hWnd->zOrder;
        }
        hWnd->zOrder=temp;
        break;
      case (int)HWND_TOPMOST:
        hWnd->topmost=1;
				hWnd->style |= WS_EX_TOPMOST;
        goto hwnd_top;
        break;
      default:
        insertWindow(hWnd,hWndInsertAfter,&rootWindows);
        break;
      }
    list_bubble_sort(&rootWindows);
    }
  {
  WINDOWPOS wpos;
  wpos.hwnd=hWnd;
  wpos.hwndInsertAfter=NULL /*boh*/;
  wpos.x=hWnd->nonClientArea.left;
  wpos.y=hWnd->nonClientArea.top;
  wpos.cx=hWnd->nonClientArea.right-hWnd->nonClientArea.left;
  wpos.cy=hWnd->nonClientArea.bottom-hWnd->nonClientArea.top;
  wpos.flags=hWnd->style;
  SendMessage(hWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);    // boh sempre
  }
  
  if(!(uFlags & SWP_NOREDRAW)) {
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    InvalidateRect(hWnd,NULL,TRUE);
    }
  return 1;
  }

BOOL GetWindowPlacement(HWND hWnd,WINDOWPLACEMENT *lpwndpl) {
  
  if(hWnd->maximized)
    lpwndpl->rcNormalPosition=hWnd->savedArea;
  else
    lpwndpl->rcNormalPosition=hWnd->nonClientArea;
  lpwndpl->ptMaxPosition=MAKEPOINT(0,0);
  HWND myWnd=GetForegroundWindow();
  lpwndpl->ptMinPosition=MAKEPOINT(4+((myWnd ? myWnd->zOrder : 1) -hWnd->zOrder)*10 ,Screen.cy-16);    // v. desktopclass
  if(hWnd->maximized)
    lpwndpl->showCmd=SW_SHOWMAXIMIZED;
  else if(hWnd->minimized)
    lpwndpl->showCmd=SW_SHOWMINIMIZED;
  else
    lpwndpl->showCmd=SW_SHOWNORMAL;
  lpwndpl->flags=0;   // bah finire
  }

BOOL MoveWindow(HWND hWnd,UGRAPH_COORD_T X,UGRAPH_COORD_T Y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,BOOL bRepaint) {
  
  return SetWindowPos(hWnd,NULL,X,Y,nWidth,nHeight,
    SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOOWNERZORDER | SWP_NOACTIVATE
    | (bRepaint ? 0 : SWP_NOREDRAW)
    );
  }

BOOL OpenIcon(HWND hWnd) {
  
  return ShowWindow(hWnd,SW_SHOWNORMAL);
  }

WORD TileWindows(HWND hWndParent,UINT wHow,const RECT *lpRect,UINT cKids,const HWND *lpKids) {
  HWND myWnd;
  //fare...
  if(hWndParent)
    myWnd=hWndParent;
  else
    myWnd= rootWindows /* FINIRE! GetDesktopWindow()*/;
  if(lpKids) {
    myWnd=myWnd->children;
    while(myWnd) {
      myWnd=myWnd->next;
      }
    }
  
  myWnd=rootWindows;
  while(myWnd) {
    if(myWnd->maximized) {
      myWnd->maximized=0;
      UpdateWindow(myWnd);
      }
  //Calling TileWindows causes all maximized windows to be restored to their previous size.
    myWnd=myWnd->next;
    }
  }

BOOL PhysicalToLogicalPoint(HWND hWnd,POINT *lpPoint) {
  
  lpPoint;  //fare..
  }

BOOL ShowWindow(HWND hWnd,BYTE nCmdShow) {
  BYTE alreadyUpdated=FALSE;
  
  switch(nCmdShow) {
    case SW_HIDE:
      hWnd->visible=0;
			if(!(hWnd->style & WS_CHILD)) {
        hWnd->topmost=0;
				hWnd->style &= ~WS_EX_TOPMOST;
    		setActive(hWnd,FALSE);
				}
      break;
    case SW_SHOWNORMAL:
//    case SW_NORMAL:
//boh?      hWnd->topmost=0;
//			hWnd->style &= ~WS_EX_TOPMOST;
      if(hWnd->maximized || hWnd->minimized)
        hWnd->nonClientArea=hWnd->savedArea;
      if(hWnd->maximized) {
        hWnd->maximized=0;
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,TRUE,SW_OTHERUNZOOM);		// mandare a TUTTI!
  //usare interesectrect per ridisegnare desktop dopo destroy ecc
        InvalidateRect(NULL,NULL,TRUE);
// ?? sarebbe meglio aggiornare TUTTE le altre tranne questa,        alreadyUpdated=TRUE;
        }
      if(hWnd->minimized) {
        hWnd->minimized=0;
        HWND myWnd=hWnd->children;
        while(myWnd) {
  				SendMessage(myWnd,WM_SHOWWINDOW,TRUE,SW_PARENTOPENING);
          myWnd=myWnd->next;
          }
        }
      calculateNonClientArea(hWnd,&hWnd->nonClientArea);
      calculateClientArea(hWnd,&hWnd->clientArea);
      // prosegue
    case SW_SHOW:
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
        setActive(hWnd,TRUE);
      alreadyUpdated=TRUE;
      break;
    case SW_SHOWMINIMIZED:
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
  			setActive(hWnd,TRUE);
    case SW_MINIMIZE:
      if(hWnd->style & WS_CHILD)    // v. MDI ...
        break;
      hWnd->topmost=0;
			hWnd->style &= ~WS_EX_TOPMOST;
      if(hWnd->maximized) {
// bah assorbito sotto...				SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,TRUE,SW_OTHERUNZOOM);		// mandare a TUTTI!
  //usare interesectrect per ridisegnare desktop dopo destroy ecc
//assorbito sotto        InvalidateRect(NULL,NULL,TRUE);
        }
      if(!hWnd->minimized) {
        HWND myWnd=hWnd->children;
        while(myWnd) {
  				SendMessage(myWnd,WM_SHOWWINDOW,FALSE,SW_PARENTCLOSING);
          myWnd=myWnd->next;
          }
        if(!hWnd->maximized)
          hWnd->savedArea=hWnd->nonClientArea;
        hWnd->minimized=1;
        hWnd->maximized=0;
        hWnd->nonClientArea.top=Screen.cy-16;    // v. anche desktopclass
        hWnd->nonClientArea.left=hWnd->zOrder*10;
        hWnd->nonClientArea.bottom=hWnd->nonClientArea.top+16;
        hWnd->nonClientArea.right=hWnd->nonClientArea.left+16;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
//			SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
        InvalidateRect(NULL,NULL,TRUE);
        alreadyUpdated=TRUE;
        }
      hWnd->maximized=0;
      break;
    case SW_SHOWMAXIMIZED:
//    case SW_MAXIMIZE:
      if(hWnd->style & WS_CHILD)  // v. MDI ...
        break;
      if(!hWnd->maximized) {
        if(!hWnd->minimized)
          hWnd->savedArea=hWnd->nonClientArea;
        hWnd->nonClientArea.top=0;
        hWnd->nonClientArea.left=0;
        hWnd->nonClientArea.bottom=Screen.cy;
        hWnd->nonClientArea.right=Screen.cx;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
				SendMessage(HWND_BROADCAST,WM_SHOWWINDOW,FALSE,SW_OTHERZOOM);		// mandare a TUTTI!
        HWND myWnd=hWnd->children;
        while(myWnd) {
          SendMessage(myWnd,WM_SHOWWINDOW,TRUE,SW_PARENTOPENING);
          myWnd=myWnd->next;
          }
        }
      hWnd->maximized=1;
      hWnd->minimized=0;
      hWnd->visible=1;
      if(hWnd->enabled && !(hWnd->style & WS_CHILD))
  			setActive(hWnd,TRUE);
      alreadyUpdated=TRUE;
      break;
    case SW_SHOWNOACTIVATE:
    case SW_SHOWNA:
      hWnd->visible=1;
      hWnd->maximized=hWnd->minimized=0;
      break;
    case SW_SHOWMINNOACTIVE:
      hWnd->visible=1;
      if(hWnd->enabled)
  			setActive(hWnd,FALSE);
      hWnd->minimized=1;
      hWnd->maximized=0;
      alreadyUpdated=TRUE;
      
      // credo che dovremmo andare a fare il vero MINIMIZE sopra...
      
      break;
    case SW_RESTORE:
      if(hWnd->maximized) {
        hWnd->nonClientArea=hWnd->savedArea;
        calculateNonClientArea(hWnd,&hWnd->nonClientArea);
        calculateClientArea(hWnd,&hWnd->clientArea);
//				SendMessage(HWND_BROADCAST,WM_NCPAINT,(WPARAM)NULL,0);
        InvalidateRect(NULL,NULL,TRUE);
        alreadyUpdated=TRUE;
        }
      hWnd->maximized=hWnd->minimized=0;
      break;
    default:
      break;
    }
  
  if(1   || !alreadyUpdated) {
    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
    return InvalidateRect(hWnd,NULL,TRUE);
    }
  else
    return TRUE;
  }

BOOL BringWindowToTop(HWND hWnd) {
  HWND myWnd;
  BYTE temp;
  
  myWnd=GetForegroundWindow();
  if(myWnd && myWnd != hWnd) {
    temp=hWnd->zOrder;
    hWnd->zOrder=myWnd->zOrder;
    myWnd->zOrder=temp;
    list_bubble_sort(&rootWindows);
    return 1;
    }
  return 0;   //NON deve accadere
  }

HWND GetTopWindow(HWND hWnd) {
  HWND myWnd,myWnd2;
//  BYTE minZorder=255;
  
  if(hWnd) {    // child windows...
    }
  else {
    myWnd=rootWindows;
    while(myWnd) {
      myWnd2=myWnd;
      myWnd=myWnd->next;
      }
    return myWnd2;
    }
  }

BOOL SetForegroundWindow(HWND hWnd) {
  HWND myWnd;
  BYTE temp;

	if(hWnd->style & WS_CHILD)
    return FALSE;
  // basterebbe setActive? o no??

	// e CHE FARE con topmost?? https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setforegroundwindow
	// https://stackoverflow.com/questions/3940346/foreground-vs-active-window

  myWnd=rootWindows;
  while(myWnd && myWnd->next) {   // vado al fondo
    myWnd=myWnd->next;
    }
  temp=myWnd->zOrder;
  myWnd->zOrder=hWnd->zOrder;
  hWnd->zOrder=temp;
  list_bubble_sort(&rootWindows);
  }

HWND SetFocus(HWND hWnd) {
  HWND myWnd=GetFocus();
  
  if(myWnd)
    SendMessage(myWnd,WM_KILLFOCUS,(WPARAM)hWnd,0);
	SendMessage(hWnd,WM_SETFOCUS,(WPARAM)myWnd,0);
	return myWnd;
	}

HWND GetFocus(void) {
  HWND myWnd;
  
  myWnd=rootWindows;    // dovrebbe SOLO essere quella Foreground e attiva...
  while(myWnd) {
    if(myWnd->focus)
      return myWnd;
    HWND myWnd2=myWnd->children;
    while(myWnd2) {
      if(myWnd->focus)
        return myWnd2;
      myWnd2=myWnd2->next;
      }
    myWnd=myWnd->next;
    }
  return NULL;
	}

HWND SetActiveWindow(HWND hWnd) {
  HWND oWnd=NULL;

// idem come sopra...  
  if(!(hWnd->style & WS_CHILD)) {
    oWnd=setActive(hWnd,TRUE);
    activateChildren(hWnd);
    }
  return oWnd;
  }

HWND GetForegroundWindow(void) {
  HWND myWnd;
  
  myWnd=rootWindows;
  while(myWnd && myWnd->next) {   // vado al fondo
    myWnd=myWnd->next;
    }
  return myWnd;
  }

HWND SetParent(HWND hWndChild,HWND hWndNewParent) {
  HWND hWnd=hWndChild->parent;
  
  if(hWndChild->style & WS_CHILD && hWndNewParent!=hWndChild /* frocio :D */)
    hWndChild->parent=hWndNewParent;
  return hWnd;
  }

HWND GetParent(HWND hWnd) {
  
  return hWnd->parent;
  }

BOOL GetClientRect(HWND hWnd,RECT *lpRect) {
  
  lpRect->left=lpRect->top=0;
  lpRect->right=hWnd->clientArea.right-hWnd->clientArea.left;
  lpRect->bottom=hWnd->clientArea.bottom-hWnd->clientArea.top;
  }

BOOL GetWindowRect(HWND hWnd,RECT *lpRect) {
  
  *lpRect=hWnd->nonClientArea;
  }

BOOL SetIcon(HWND hWnd,const GFX_COLOR *icon,BYTE type) {

  return SendMessage(hWnd,WM_SETICON,type,(LPARAM)icon);
  }

BOOL SetFont(HWND hWnd,FONT font) { // questa in effetti non esiste, si userebbero i SelectObject...

  SendMessage(hWnd,WM_SETFONT,(WPARAM)&font,TRUE);
  }

BOOL IsWindow(HWND hWnd) {
  HWND myWnd;
  
  myWnd=rootWindows;
  while(myWnd) {
    if(myWnd == hWnd)
      return TRUE;
    myWnd=myWnd->next;
    }
  return FALSE;
  }

BOOL IsZoomed(HWND hWnd) { 
  return hWnd->maximized;
  }

BOOL IsIconic(HWND hWnd) {
  return hWnd->minimized;
  }

BOOL AdjustWindowRect(RECT *lpRect,DWORD dwStyle,BOOL bMenu) {
  }
  
UINT ArrangeIconicWindows(HWND hWnd) {
  }

HWND setActive(HWND hWnd,BOOL state) {
  HWND myWnd;
  BYTE temp;

  if(state) {
    myWnd=rootWindows;
    while(myWnd) {
      if(myWnd != hWnd && !myWnd->parent && myWnd->active) {  // tendenzialmente dovrebbe essere sempre l'ultima, quella attiva...
		    if(myWnd->topmost) {	// bah direi così
					return NULL;			// segnalo nulla di fatto
					}
        SendMessage(myWnd,WM_ACTIVATE,WA_INACTIVE,(LPARAM)hWnd);
        myWnd->active=0;
        SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL,0);
        InvalidateRect(myWnd,NULL,TRUE);
        if(hWnd->zOrder < myWnd->zOrder) {
          temp=hWnd->zOrder;			// mah non è una grande idea lo scambio, sarebbe meglio decrementare tutte le altre
          hWnd->zOrder=myWnd->zOrder;
          myWnd->zOrder=temp;
          list_bubble_sort(&rootWindows);
          }
        break;
        }
      myWnd=myWnd->next;
      }
    hWnd->active=1;
//    SendMessage(hWnd,WM_ACTIVATE,WA_ACTIVE,(LPARAM)myWnd);
    
    activateChildren(hWnd);
    
//    SendMessage(hWnd,WM_NCPAINT,(WPARAM)NULL,0);
//  SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&hWnd->hDC,0);
//    SendMessage(hWnd,WM_PAINT,0,0);
    // c'è già nel/nei chiamante, forse togliere..
    return hWnd;
    }
  else {
    if(hWnd->topmost) {	// bah direi così
			return NULL;			// segnalo nulla di fatto
			}
		//GetForegroundWindow
    if(hWnd->active) {
      // in teoria quella attiva in precedenza è l'ultima ossia quella in cima a Z-order...PENULTIMA!
      myWnd=rootWindows;
      while(myWnd) {
        if(myWnd != hWnd && !myWnd->parent && !myWnd->active) {
 				  myWnd->active=1;
          SendMessage(myWnd,WM_ACTIVATE,WA_ACTIVE,(LPARAM)hWnd);
          myWnd->active=1;
          SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL,0);
          InvalidateRect(myWnd,NULL,TRUE);
          temp=hWnd->zOrder;		// e idem v.sopra, non scambiare
          hWnd->zOrder=myWnd->zOrder;
          myWnd->zOrder=temp;
          list_bubble_sort(&rootWindows);
          break;
          }
        myWnd=myWnd->next;
        }
  	  hWnd->active=0;
//      SendMessage(hWnd,WM_ACTIVATE,WA_INACTIVE,(LPARAM)hWnd);
      
      return hWnd;
      }
    }
  
  return NULL;    //NON deve accadere!
	}

    
void activateChildren(HWND hWnd) {
  HWND myWnd;
  
  myWnd=hWnd->children;
  while(myWnd) {
    SendMessage(myWnd,WM_CHILDACTIVATE,0,0);
    myWnd=myWnd->next;
    }
  }

int SetScrollPos(HWND hWnd,int nBar,int nPos,BOOL bRedraw) {
  RECT rc;
  
  switch(nBar) {
    case SB_CTL:
      break;
    case SB_HORZ:
      hWnd->scrollPosX=nPos;
      rc.left=hWnd->nonClientArea.left;
      rc.bottom=hWnd->clientArea.bottom;
      rc.right=hWnd->nonClientArea.right;
      rc.top=hWnd->clientArea.bottom-SCROLL_SIZE;
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
      break;
    case SB_VERT:
      hWnd->scrollPosY=nPos;
      rc.top=hWnd->clientArea.top;
      rc.bottom=hWnd->clientArea.bottom;
      rc.right=hWnd->nonClientArea.right;
      rc.left=hWnd->nonClientArea.right-SCROLL_SIZE;
      SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
      break;
    }
  if(bRedraw)
    return InvalidateRect(hWnd,NULL,TRUE);
  else
    return 1;
  }

int SetScrollInfo(HWND hWnd,int nBar,SCROLLINFO *lpsi,BOOL bRedraw) {
  
  switch(nBar) {
    case SB_CTL:
      break;
    case SB_HORZ:
      if(lpsi->fMask & SIF_POS)
        hWnd->scrollPosX=lpsi->nPos;
      if(lpsi->fMask & SIF_PAGE)
        ;
      if(lpsi->fMask & SIF_RANGE)
        hWnd->scrollSizeX=lpsi->nPos;
      if(lpsi->fMask & SIF_DISABLENOSCROLL)
        ;
      break;
    case SB_VERT:
      if(lpsi->fMask & SIF_POS)
        hWnd->scrollPosY=lpsi->nPos;
      if(lpsi->fMask & SIF_PAGE)
        ;
      if(lpsi->fMask & SIF_RANGE)
        hWnd->scrollSizeY=lpsi->nPos;
      if(lpsi->fMask & SIF_DISABLENOSCROLL)
        ;
      break;
    }
  if(bRedraw)
    return InvalidateRect(hWnd,NULL,TRUE);
  else
    return 1;
  }

BOOL SetScrollRange(HWND hWnd,int nBar,int nMinPos,int nMaxPos,BOOL bRedraw) {
  RECT rc;
  int oldsize;
  
  switch(nBar) {
    case SB_CTL:
      break;
    case SB_HORZ:
      oldsize=hWnd->scrollSizeX;
      hWnd->scrollSizeX=(nMaxPos-nMinPos);   // boh finire..
      if(oldsize) {   // evito rientro/ridisegno (specie quando viene disabilitato)
        rc.left=hWnd->nonClientArea.left;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.top=hWnd->clientArea.bottom-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    case SB_VERT:
      oldsize=hWnd->scrollSizeY;
      hWnd->scrollSizeY=(nMaxPos-nMinPos);
      if(oldsize) {
        rc.top=hWnd->clientArea.top;
        rc.bottom=hWnd->clientArea.bottom;
        rc.right=hWnd->nonClientArea.right;
        rc.left=hWnd->nonClientArea.right-SCROLL_SIZE;
        SendMessage(hWnd,WM_NCPAINT,(WPARAM)&rc,0);
        }
      break;
    }
  if(bRedraw)
    return InvalidateRect(hWnd,NULL,TRUE);
  else
    return 1;
  }

BOOL SetWindowText(HWND hWnd,const char *title) {

  return SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)title);
  }

int GetWindowText(HWND hWnd,char *lpString,int nMaxCount) {
  
  return SendMessage(hWnd,WM_GETTEXT,nMaxCount,(LPARAM)lpString);
  }

int InternalGetWindowText(HWND hWnd,char *lpString,int nMaxCount) {
  
  int i=min(sizeof(hWnd->caption),nMaxCount);
  strncpy(lpString,hWnd->caption,i-1);
  lpString[i]=0;
  }

BOOL GetTitleBarInfo(HWND hWnd,TITLEBARINFO *pti) {
  
  // if(pti->cbSize=
  pti->rcTitleBar.top=hWnd->nonClientArea.top;
  pti->rcTitleBar.left=hWnd->nonClientArea.left;
  pti->rcTitleBar.right=hWnd->nonClientArea.right;
  if(hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION))
    pti->rcTitleBar.bottom=pti->rcTitleBar.top+=TITLE_HEIGHT;
  else
    pti->rcTitleBar.bottom=pti->rcTitleBar.top;

  pti->rgstate[0]=hWnd->style & (WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_CAPTION) ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[2]=hWnd->style & WS_MINIMIZEBOX ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[3]=hWnd->style & WS_MAXIMIZEBOX  ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[4]=hWnd->style & 0 /*WS_HELP*/ ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  pti->rgstate[5]=hWnd->style & WS_SYSMENU ? STATE_SYSTEM_FOCUSABLE : STATE_SYSTEM_UNAVAILABLE;
  }


int PaintDesktop(HDC *hDC, signed char effects) {   // non è esattamente così in Windows, ma ok
    
  if(effects>0) {
    FillRectangle(0,0,Screen.cx,Screen.cy,DARKGRAY);
    }
//#warning  usare DrawWindow togliere fillrectangle unire
  
//  FillRectangle(0,0,Screen.cx,Screen.cy,desktopColor);
//  SendMessage(desktopWindow,WM_NCPAINT,0,0);
  SendMessage(desktopWindow,WM_PAINT,0,0);
//  InvalidateRect(desktopWindow,NULL,TRUE);
  SendMessage(taskbarWindow,WM_NCPAINT,0,0);
  SendMessage(taskbarWindow,WM_PAINT,0,0);
//  InvalidateRect(taskbarWindow,NULL,TRUE);
      
  if(effects<0) {
    FillRectangle(0,0,Screen.cx,Screen.cy,DARKGRAY);
    FillRectangle(0,0,Screen.cx,Screen.cy,BLACK);
    }
    
  return 1;
  }

BOOL MessageBeep(BYTE uType) {
  BYTE isWave=(eXtra & 16 && GetID(AUDIO_CARD) == 'A');

	switch(uType & 0xf0) {
		case MB_ICONEXCLAMATION:
      if(isWave) {
        SetAudioWave(0,2,1000,0,8,90,0,0);
        __delay_ms(400);
        }
      else {
  			StdBeep(400);
        }
			break;
		case MB_ICONHAND:
      if(isWave) {
        SetAudioWave(0,2,660,0,8,90,0,0);
        __delay_ms(800);
        }
      else {
  			StdBeep(800);
        }
			break;
		case MB_ICONQUESTION:
      if(isWave) {
        SetAudioWave(0,2,1200,0,8,90,0,0);
        __delay_ms(400);
        }
      else {
  			StdBeep(800);
        }
			break;
		case MB_ICONASTERISK:
      if(isWave) {
        SetAudioWave(0,1,1000,0,8,90,0,0);
        __delay_ms(800);
        }
      else {
  			StdBeep(800);
        }
			break;
		default:
      if(isWave) {
        SetAudioWave(0,2,880,0,8,90,0,0);
        __delay_ms(500);
        }
      else {
  			StdBeep(500);
        }
			break;
		}
  if(isWave)
    SetAudioWave(0,0,0,0,8,90,0,0);

	}	

int MessageBox(HWND hWnd,const char *lpText,const char *lpCaption,WORD uType) {
	RECT rc;
	SIZE ds;


	if(hWnd) {
		RECT rc2;
		GetClientRect(hWnd,&rc2);
  	ds.cx=max(rc2.right/3,100);
    ds.cy=max(rc2.bottom/3,60);
		rc.top=(rc2.bottom-ds.cy)/2;
		rc.left=(rc2.right-ds.cx)/2;
		}
	else {
  	ds.cx=max(Screen.cx/3,100);
    ds.cy=max(Screen.cy/3,60);
		rc.top=(Screen.cy-ds.cy)/2;
		rc.left=(Screen.cx-ds.cx)/2;
		}
	rc.bottom=rc.top+ds.cy;
	rc.right=rc.left+ds.cx;
	HWND myDlg=CreateWindow(MAKECLASS(WC_DIALOG),lpCaption,WS_POPUP | WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_DLGFRAME | 
    WS_CAPTION | (hWnd ? WS_CHILD : 0 /*boh direi per ora*/)  | WS_CLIPCHILDREN,
//	HWND myDlg=CreateWindow(MAKECLASS(WC_DIALOG),lpCaption,WS_ACTIVE | WS_EX_TOPMOST | WS_BORDER | WS_SYSMENU | WS_VISIBLE | WS_DLGFRAME | 
//    WS_CAPTION | (hWnd ? WS_CHILD : 0 /*boh direi per ora*/),
    rc.left,rc.top,ds.cx,ds.cy,
    hWnd,(MENU *)NULL,(void*)256
    );
  SetWindowLong(myDlg,DWL_DLGPROC,(DWORD)DefWindowProcDlgMessageBox);
  myDlg->windowProc=(WINDOWPROC*)GetWindowLong(myDlg,DWL_DLGPROC);   // questa cosa andrebbe in CreateDialog/DialogBox, direi!

	HWND myWnd2,myWnd3,myWnd4, focusWnd;
  POINT bpos; SIZE bsiz;
  bpos.y=ds.cy-28 /*considera CAPTION*/; bsiz.cy=12;
	switch(uType & 0xf) {
		case MB_OK:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"OK",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_DEFPUSHBUTTON | BS_CENTER,
				ds.cx/2-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDOK,NULL
				);
      focusWnd=myWnd2;
			break;
		case MB_OKCANCEL:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"OK",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				ds.cx/3-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDOK,NULL
				);
			myWnd3=CreateWindow(MAKECLASS(WC_BUTTON),"Annulla",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				(ds.cx*2)/3-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDCANCEL,NULL
				);
			switch(uType & 0xf00) {
				case MB_DEFBUTTON1:
          focusWnd=myWnd2;
          myWnd2->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON2:
          focusWnd=myWnd3;
          myWnd3->style |= BS_DEFPUSHBUTTON;
					break;
				}
			break;
		case MB_YESNO:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"Sì",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				ds.cx/3-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDYES,NULL
				);
			myWnd3=CreateWindow(MAKECLASS(WC_BUTTON),"No",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				(ds.cx*2)/3-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDNO,NULL
				);
			switch(uType & 0xf00) {
				case MB_DEFBUTTON1:
          focusWnd=myWnd2;
          myWnd2->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON2:
          focusWnd=myWnd3;
          myWnd3->style |= BS_DEFPUSHBUTTON;
					break;
				}
			break;
		case MB_YESNOCANCEL:
			myWnd2=CreateWindow(MAKECLASS(WC_BUTTON),"Sì",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				ds.cx/4-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDYES,NULL
				);
			myWnd3=CreateWindow(MAKECLASS(WC_BUTTON),"No",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				(ds.cx*2)/4-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDNO,NULL
				);
			myWnd4=CreateWindow(MAKECLASS(WC_BUTTON),"Annulla",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER,
				(ds.cx*3)/4-16,bpos.y,30,bsiz.cy,
				myDlg,(MENU *)IDCANCEL,NULL
				);
			switch(uType & 0xf00) {
				case MB_DEFBUTTON1:
          focusWnd=myWnd2;
          myWnd2->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON2:
          focusWnd=myWnd3;
          myWnd3->style |= BS_DEFPUSHBUTTON;
					break;
				case MB_DEFBUTTON3:
          focusWnd=myWnd4;
          myWnd4->style |= BS_DEFPUSHBUTTON;
					break;
				}
			break;
		}
	switch(uType & 0xf0) {
		case MB_ICONEXCLAMATION:
//			DrawIcon8();
			break;
		}

  SendMessage(myDlg,WM_INITDIALOG,(WPARAM)focusWnd,0);
//  SendMessage(myDlg,WM_SETFONT,(WPARAM)NULL /*sarebbe font da template*/,TRUE);
  ShowWindow(myDlg,SW_SHOW);
          
  {
  GetClientRect(myDlg,&rc);
  HDC myDC,*hDC;
  hDC=GetDC(myDlg,&myDC);
  SetTextColor(hDC,GREEN);
  TextOut(hDC,1,4,lpText);
  ReleaseDC(myDlg,hDC);

	// forse creare uno STATIC...
  }

// dovrebbe restare qua e ritornare GetWindowLong(myDlg,DWL_MSGRESULT) 
	return 1;

	}

int DrawIcon8(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR icon[]) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i,j;
  const GFX_COLOR *p2=icon;
  
  w=8; h=8;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      DrawPixelWindowColor(hDC,x1+i,y1+j,*p2++);
      }
    }
  }

int DrawIcon(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR icon[]) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i, j;
  const GFX_COLOR *p2=icon;
  
  w=16; h=16;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      DrawPixelWindowColor(hDC,x1+i,y1+j,*p2++);
      }
    }
  }

int DrawBitmap(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const GFX_COLOR bitmap[],UGRAPH_COORD_T xs, UGRAPH_COORD_T ys) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i, j;
  const GFX_COLOR *p2=bitmap;
  
  w=xs; h=ys;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
      DrawPixelWindowColor(hDC,x1+i,y1+j,*p2++);
      }
    }
  }

int DrawCaret(HWND hWnd,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const CURSOR caret) {
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i, j;
  const GFX_COLOR *p2=caret;
  
  w=4; h=8;
  for(j=0; j<h; j++) {
    for(i=0; i<w; i++) {
//      DrawPixelWindowColor(hDC,x1+i, y1+j, *p2++); fare..
      }
    }
  }

void DrawCursor(UGRAPH_COORD_T x,UGRAPH_COORD_T y,const CURSOR cursor,BYTE size) {
  UGRAPH_COORD_T w,h;
// v. anche DrawIconEx, windows fa così..  
  
  if(size) {
    w=16; h=16;
    }
  else {
    w=8; h=8;
    }
  
//  CreateSprite(0,w,h,x,y,1);    // OVVIAMENTE fare move!
  MovePointer(x,y);

  }

int ShowCursor(BOOL m) {
  GFX_COLOR *p3=savedCursorArea;
  UGRAPH_COORD_T w,h;
  GRAPH_COORD_T i,j;

  w=h=8;    //per ora SIZE fissa!

  CreateSprite(0,0,mousePosition.x,mousePosition.y,0,(m ? 1 : 0)   | 8);    // OVVIAMENTE fare setattrib!
  
  }

BOOL DrawIconEx(HDC *hDC,UGRAPH_COORD_T xLeft,UGRAPH_COORD_T yTop,ICON hIcon,UGRAPH_COORD_T cxWidth,UGRAPH_COORD_T cyWidth,
  UINT istepIfAniCur,BRUSH hbrFlickerFreeDraw,UINT diFlags) {
  
  if(!hDC) {    // vado sul desktop o desktopWindow
    }
  }

BOOL TextOut(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y,const char *s) {
  char c;
  
#ifdef USING_SIMULATOR
  return;
#endif  
  
 
  while(c=*s++) {
    
    if(!hDC->font.font) { // 'Classic' built-in font
      switch(c) {
        case '\n':
          y += (hDC->font.size)*8;
          x = 0;
          break;
        case '\r':
          // skip em
          break; 
        default:
          if(wrap && ((x + (hDC->font.size) * 6) >= hDC->area.right)) { 	// Heading off edge?
            // TENDENZIALMENTE QUA NON CI INTERESSA! textout fa una sola riga
  //          x  = hDC->area.left;            // Reset x 
  //          y += (hDC->fontSimple) * 8; // Advance y one line
            }
          DrawCharWindow(hDC,x, y, c);
          x += (hDC->font.size) * 6;
          break; 
        }
      } 
    else { // Custom font
      gfxFont=hDC->font.font;
      
      switch(c) {
        case '\n':
          x = 0;
          y += (GRAPH_COORD_T)hDC->font.size * gfxFont->yAdvance;
          break;
        case '\r':
          break;
        default:
          {
          UINT8 first = gfxFont->first;
          if(c >= first && c <= gfxFont->last) {
            UINT8   c2 = c - gfxFont->first;
            GFXglyph *glyph = &gfxFont->glyph[c2];
            UINT8   w=glyph->width, h=glyph->height;
            if(w>0 && h>0) { // Is there an associated bitmap?
              GRAPH_COORD_T xo = glyph->xOffset; // sic
              if(wrap && ((x + textsize * (xo+w)) >= Screen.cx)) {
                // Drawing character would go off right edge; wrap to new line
                x = 0;
                y += (GRAPH_COORD_T)hDC->font.size * gfxFont->yAdvance;
                }
              DrawCharWindow(hDC,x,y,c);
  //						ovviamente farne un altra!
              }
            x += glyph->xAdvance * (GRAPH_COORD_T)hDC->font.size;
            }
          }
          break;
        }

      }

    }
  return 1;
  }

BOOL ExtTextOut(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,UINT options,const RECT *lpRect,const char *s,UINT n,const INT *lpDx) {
  char c;
  UINT cnt;
  
  while(cnt<n) {
    c=*s++;
    
  if(!hDC->font.font) { // 'Classic' built-in font

    switch(c) {
      case '\n':
        y += (hDC->font.size)*8;
        x = 0;
        break;
      case '\r':
        // skip em
        break; 
      default:
        if(wrap && ((x + (hDC->font.size) * 6) >= hDC->area.right)) { 	// Heading off edge?
          // TENDENZIALMENTE QUA NON CI INTERESSA! textout fa una sola riga
//          x  = hDC->area.left;            // Reset x 
//          y += (hDC->fontSimple) * 8; // Advance y one line
          }
        if(options & ETO_NUMERICSLOCAL)
          ;
        if(options & ETO_SMALL_CHARS)   // questa roba sembra per windows + recenti..
          ;
        DrawCharWindow(hDC,x,y,c);
        x += (hDC->font.size) * 6;
        if(options & ETO_CLIPPED /*&& ! ETO_NO_RECT*/) {
          if(x>=lpRect->right)
            break;
          }
        if(lpDx)
          x+=lpDx[cnt];
        break; 
    	}

  	} 
	else { // Custom font
    
      gfxFont=hDC->font.font;
      
      switch(c) {
        case '\n':
          x = 0;
          y += (GRAPH_COORD_T)hDC->font.size * gfxFont->yAdvance;
          break;
        case '\r':
          break;
        default:
          {
          UINT8 first = gfxFont->first;
          if(c >= first && c <= gfxFont->last) {
            UINT8   c2 = c - gfxFont->first;
            GFXglyph *glyph = &gfxFont->glyph[c2];
            UINT8   w=glyph->width, h=glyph->height;
            if(w>0 && h>0) { // Is there an associated bitmap?
              GRAPH_COORD_T xo = glyph->xOffset; // sic
              if(wrap && ((x + textsize * (xo+w)) >= Screen.cx)) {
                // Drawing character would go off right edge; wrap to new line
                x = 0;
                y += (GRAPH_COORD_T)hDC->font.size * gfxFont->yAdvance;
                }
              if(options & ETO_NUMERICSLOCAL)
                ;
              if(options & ETO_SMALL_CHARS)   // questa roba sembra per windows + recenti..
                ;
              DrawCharWindow(hDC,x,y,c);
  //						ovviamente farne un altra!
              }
            x += glyph->xAdvance * (GRAPH_COORD_T)hDC->font.size;
            if(options & ETO_CLIPPED /*&& ! ETO_NO_RECT*/) {
              if(x>=lpRect->right)
                break;
              }
            }
          }
          break;
        }
    	}

    cnt++;
    }
  return 1;
  }

int DrawText(HDC *hDC,const char *lpchText,int cchText,RECT *lprc,UINT format) {

  return ExtTextOut(hDC,lprc->left,lprc->top,format /*CONTROLLARE!*/,NULL,lpchText,cchText,NULL);
  }

void SetWindowTextCursor(HDC *hDC,BYTE col,BYTE row) {
  
  SetXY(0,col*hDC->font.size*6,row*hDC->font.size*8);
  #warning cazzata
  }

BOOL MoveToEx(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,POINT *lppt) {
  
  if(lppt) {
    lppt->x=hDC->cursor.x;
    lppt->y=hDC->cursor.y;
    }
  hDC->cursor.x=x;
  hDC->cursor.y=y;
  return 1;
  }

BOOL LineTo(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  
  DrawLineWindow(hDC,hDC->cursor.x, hDC->cursor.y,x,y);
  hDC->cursor.x=x;
  hDC->cursor.y=y;
  return 1;
  }

GFX_COLOR SetPixel(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR c) {

  DrawPixelWindowColor(hDC,x,y,c);
  return c;   // forse vuole il colore precedente, finire quando ci spostiamo in RAM
  }
  
GFX_COLOR GetPixel(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  
//  readPixel(x1,y1);
  return (GFX_COLOR)CLR_INVALID;   // FINIRE in ram ecc
  }
  
BOOL Rectangle(HDC *hDC,UGRAPH_COORD_T left,UGRAPH_COORD_T top,UGRAPH_COORD_T right,UGRAPH_COORD_T bottom) {

  if(hDC->brush.style != BS_NULL)
    FillRectangleWindow(hDC,left,top,right,bottom);
  if(hDC->pen.style == PS_NULL) {
    right--;bottom--;
  //If a PS_NULL pen is used, the dimensions of the rectangle are 1 pixel less in height and 1 pixel less in width.
    }
  DrawRectangleWindow(hDC,left,top,right,bottom);
  }

BOOL FillRect(HDC *hDC,const RECT *lprc,BRUSH hbr) {
  
  FillRectangleWindowColor(hDC,lprc->left,lprc->top,lprc->right,lprc->bottom,hbr.color);
  return TRUE;
  }

BOOL FrameRect(HDC *hDC,const RECT *lprc,BRUSH hbr) {
  int i=hbr.size;

  do {
    DrawRectangleWindowColor(hDC,lprc->left+i,lprc->top+i,lprc->right-i,lprc->bottom-i,hbr.color);
    // sarebbe giusto centrare lo spessore...
    } while(--i);

  return TRUE;
  }

BOOL Ellipse(HDC *hDC,UGRAPH_COORD_T left,UGRAPH_COORD_T top,UGRAPH_COORD_T right,UGRAPH_COORD_T bottom) {

  if(hDC->brush.style != BS_NULL)   // fare pieno...
    ;
  DrawCircleWindow(hDC,left,top,right,bottom);
  }

BOOL Arc(HDC *hDC,UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,
  UGRAPH_COORD_T x3,UGRAPH_COORD_T y3,UGRAPH_COORD_T x4,UGRAPH_COORD_T y4) {
  
//  DrawCircleWindow(hDC, left, top, right, bottom);
  }

BOOL Polygon(HDC *hDC,const POINT *apt,int cpt) {
  POINT oldPt=hDC->cursor;
  
  MoveTo(hDC,apt->x,apt->y);
  while(--cpt) {
    apt++;
    LineTo(hDC,apt->x,apt->y);
    }
  hDC->cursor=oldPt;
  return TRUE;
  }

BOOL Polyline(HDC *hDC,const POINT *apt,int cpt) {
  POINT oldPt=hDC->cursor;
  
  MoveTo(hDC,apt->x,apt->y);
  while(--cpt) {
    apt++;
    LineTo(hDC,apt->x,apt->y);
    }
  hDC->cursor=oldPt;
  return TRUE;
  }

BOOL PolylineTo(HDC *hDC,const POINT *apt,DWORD cpt) {
  
  MoveTo(hDC,apt->x,apt->y);
  while(--cpt) {
    apt++;
    LineTo(hDC,apt->x,apt->y);
    }
  return TRUE;
  }

BOOL PolyTextOut(HDC *hDC,const POLYTEXT *ppt,int nstrings) {
  
  while(--nstrings) {
    ExtTextOut(hDC,ppt->x,ppt->y,ppt->uiFlags,&ppt->rcl,ppt->lpstr,ppt->n,ppt->pdx);
    ppt++;
    }
  return TRUE;
  }

BOOL FillPath(HDC *hDC) {
  }

BOOL FloodFill(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color) {
  
  ExtFloodFill(hDC,x,y,color,FLOODFILLBORDER);
  }

#define MAX_WIDTH 1024
#define MAX_HEIGHT 768
#define MAX_FILL_STACK 512
BOOL ExtFloodFill(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,GFX_COLOR color,UINT type) {
  BYTE up,down;
  GFX_COLOR bg;
  S_POINT pt;
  
	if(type==FLOODFILLBORDER) {
    bg = color;// if bg is None else bg
    S_POINT stack[MAX_FILL_STACK];
    int stack_ptr=0;
    
    pt.x=x; pt.y=y;
    stack[stack_ptr++]=pt;
    while(stack_ptr>0) {
      pt = stack[--stack_ptr];
      if(GetPixel(hDC,pt.x,pt.y) != bg)
        continue;
      // find left boundary
      while(pt.x > 0 && GetPixel(hDC,pt.x-1,pt.y) != bg)
        pt.x--;        
      // scan to right
      up=TRUE; down=TRUE;
      while(pt.x < MAX_WIDTH && GetPixel(hDC,pt.x,pt.y) != bg) {
        DrawPixelWindowColor(hDC,pt.x,pt.y,hDC->brush.color);
    // detect color change above
        if((pt.y+1) < MAX_HEIGHT) {
          if(up && GetPixel(hDC,pt.x,pt.y+1) != bg) {
            S_POINT pt2;
            pt2.x=pt.x; pt2.y=pt.y+1;
            stack[stack_ptr++]=pt2;
            }
          up = GetPixel(hDC,pt.x,pt.y+1) == bg;
          }
    // detect color change below
        if(pt.y>0) {
          if(down && GetPixel(hDC,pt.x,pt.y-1) != bg) {
            S_POINT pt2;
            pt2.x=pt.x; pt2.y=pt.y-1;
            stack[stack_ptr++]=pt2;
            }
          down = GetPixel(hDC,pt.x, pt.y-1) == bg;       
          }
        pt.x++;
        }
      if(stack_ptr>=MAX_FILL_STACK)
        return 0;
      }

  	return 1;
    }
	else if(type==FLOODFILLSURFACE) {
    pt.x=x; pt.y=y;
    bg = hDC->foreColor ;// if bg is None else bg
    S_POINT stack[MAX_FILL_STACK];
    //sembra andare bene, 3/10/22: 500 dovrebbe bastare.. ogni cambio direzione conta 1...
    int stack_ptr=0;
    
    stack[stack_ptr++]=pt;
    while(stack_ptr>0) {
      pt = stack[--stack_ptr];
      if(GetPixel(hDC,pt.x,pt.y) != bg)
        continue;
      // find left boundary
      while(pt.x > 0 && GetPixel(hDC,pt.x-1,pt.y) == bg)
        pt.x--;        
      // scan to right
      up=TRUE; down=TRUE;
      while(pt.x < MAX_WIDTH && GetPixel(hDC,pt.x,pt.y) == bg) {
        DrawPixelWindowColor(hDC,pt.x,pt.y,hDC->brush.color);
    // detect color change above
        if((pt.y+1) < MAX_HEIGHT) {
          if(up && GetPixel(hDC,pt.x,pt.y+1) == bg) {
            S_POINT pt2;
            pt2.x=pt.x; pt2.y=pt.y+1;
            stack[stack_ptr++]=pt2;
            }
          up = GetPixel(hDC,pt.x,pt.y+1) != bg;
          }
    // detect color change below
        if(pt.y>0) {
          if(down && GetPixel(hDC,pt.x,pt.y-1) == bg) {
            S_POINT pt2;
            pt2.x=pt.x; pt2.y=pt.y-1;
            stack[stack_ptr++]=pt2;
            }
          down = GetPixel(hDC,pt.x, pt.y-1) != bg;       
          }
        pt.x++;
        }
      if(stack_ptr>=MAX_FILL_STACK)
        return 0;
      }
    }

  return 1;
  }

PEN CreatePen(BYTE iStyle,BYTE cWidth,GFX_COLOR color) {
  PEN myPen;
  
  myPen.style=iStyle;
  myPen.size=cWidth;
  myPen.color=color;
  return myPen;
	}

GFX_COLOR SetDCPenColor(HDC *hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->pen.color;
  
  hDC->pen.color=color;
  // pure style?
  return c;
  }
  
BRUSH CreateSolidBrush(GFX_COLOR color) {
  BRUSH myBrush;
  
  myBrush.style=BS_SOLID;
  myBrush.size=1;
  myBrush.color=color;
  return myBrush;
	}

BRUSH CreateHatchBrush(BYTE iHatch,GFX_COLOR color) {
  BRUSH myBrush;
  
  myBrush.style=iHatch;
  myBrush.size=1;
  myBrush.color=color;
  return myBrush;
	}

BRUSH CreatePatternBrush(BITMAP *hbm) {
  BRUSH myBrush;
  
  myBrush.style=BS_PATTERN;
  myBrush.size=1;
  myBrush.color=0;
  return myBrush;
	}

BRUSH GetSysColorBrush(int nIndex) {
  
  return CreateSolidBrush(GetSysColor(nIndex));
  }

GFX_COLOR SetDCBrushColor(HDC *hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->brush.color;
  
  hDC->brush.color=color;
  // pure style?
  return c;
  }

GDIOBJ GetStockObject(int i) {
  GDIOBJ g;
  
  switch(i) {
    case WHITE_BRUSH:
      g.brush=CreateSolidBrush(WHITE);
      return g;
      break;
    case LTGRAY_BRUSH:
      g.brush=CreateSolidBrush(LIGHTGRAY);
      return g;
      break;
    case GRAY_BRUSH:
      g.brush=CreateSolidBrush(GRAY192);
      return g;
      break;
    case DKGRAY_BRUSH:
      g.brush=CreateSolidBrush(DARKGRAY);
      return g;
      break;
    case BLACK_BRUSH:
      g.brush=CreateSolidBrush(BLACK);
      return g;
      break;
    case NULL_BRUSH:
//    case HOLLOW_BRUSH:
      g.brush=CreateSolidBrush(0);
      g.brush.style=0;
      return g;
      break;
    case WHITE_PEN:
      g.pen=CreatePen(PS_SOLID,1,WHITE);
      return g;
      break;
    case BLACK_PEN:
      g.pen=CreatePen(PS_SOLID,1,BLACK);
      return g;
      break;
    case NULL_PEN:
      g.pen=CreatePen(PS_SOLID,0,0);
      return g;
      break;
    case DC_BRUSH:
      g.brush=CreateSolidBrush(BLACK);    // CONTRARIO DI WINDOWS :)
      return g;
      break;
    case DC_PEN:
      g.pen=CreatePen(PS_SOLID,1,WHITE);    // contrario di windows !
      return g;
      break;
    case SYSTEM_FIXED_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case SYSTEM_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case OEM_FIXED_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case ANSI_FIXED_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case ANSI_VAR_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case DEVICE_DEFAULT_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case DEFAULT_GUI_FONT:
      g.font=CreateFont(8,6,0,0,FW_NORMAL,FALSE,FALSE,FALSE,ANSI_CHARSET,
              OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      return g;
      break;
    case DEFAULT_PALETTE:
      break;
    }
  }
  
FONT CreateFont(BYTE cHeight,BYTE cWidth,BYTE cEscapement,BYTE cOrientation,WORD cWeight,
  BYTE bItalic,BYTE bUnderline,BYTE bStrikeOut,enum FONTCHARSET iCharSet,enum FONTPRECISION iOutPrecision,
  enum FONTCLIPPRECISION iClipPrecision, WORD iQuality, enum FONTPITCHANDFAMILY iPitchAndFamily,const char *pszFaceName) {
  FONT myFont;
  
  switch(iPitchAndFamily & 0xf0) {
    case FF_ROMAN:
      myFont.font= (void*) (myFont.size>10 ? FreeSerif12pt7b : FreeSerif9pt7b);
      myFont.size=cHeight / (myFont.size>10 ? 12 : 9); 
      if(!myFont.size) myFont.size=1;
      break;
    case FF_SWISS:
      myFont.font= (void*) (myFont.size>10 ? FreeSans12pt7b : FreeSans9pt7b);
      myFont.size=cHeight / (myFont.size>10 ? 12 : 9); 
      if(!myFont.size) myFont.size=1;
      break;
    default:
      if(!pszFaceName) {
        myFont.font=NULL;
        myFont.size=cHeight / 8; 
        }
      else if(!stricmp(pszFaceName,"system")) {
        myFont.font=NULL;
        myFont.size=cHeight / 8; 
        }
      else if(!stricmp(pszFaceName,"arial")) {
        myFont.font= (void*)(myFont.size>10 ? FreeSans12pt7b : FreeSans9pt7b);
        myFont.size=cHeight / (myFont.size>10 ? 12 : 9); 
        if(!myFont.size) myFont.size=1;
        }
      else if(!stricmp(pszFaceName,"times")) {
        myFont.font= (void*)(myFont.size>10 ? FreeSerif12pt7b : FreeSerif9pt7b);
        myFont.size=cHeight / (myFont.size>10 ? 12 : 9); 
        if(!myFont.size) myFont.size=1;
        }
      break;
    }
  myFont.inclination=0; 
  myFont.weight=cWeight/10;   // faccio così, risparmio :)
  myFont.attributes=0;
  myFont.italic=bItalic;
  myFont.bold= myFont.weight > FW_NORMAL/10;
  myFont.strikethrough=bStrikeOut;
  myFont.underline=bUnderline;
  return myFont;
	}

static BOOL getFontInfo(const GFXfont *font,LOGFONT *lf,TEXTMETRIC *lptm) {
  
  if(font==NULL) {
    strcpy(lf->lfFaceName,"System");
    lf->lfCharSet=DEFAULT_CHARSET;   // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-logfonta
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=8;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=FIXED_PITCH | FF_DONTCARE;
    lf->lfQuality=DRAFT_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=6;
    lptm->tmAscent=6;      // https://docs.microsoft.com/en-us/windows/win32/api/wingdi/ns-wingdi-textmetrica
    lptm->tmAveCharWidth=6;
    lptm->tmBreakChar=13;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=2;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=0;
    lptm->tmHeight=8;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=255;
    lptm->tmMaxCharWidth=6;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=0 /*!TMPF_FIXED_PITCH*/ | FF_DONTCARE;
    return TRUE;
    }
  else if(font==FreeSans9pt7b) {
    strcpy(lf->lfFaceName,"Arial");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=9;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_SWISS;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=7;
    lptm->tmAscent=6;
    lptm->tmAveCharWidth=7;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=2;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=9;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=7;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_FIXED_PITCH | FF_SWISS;
    return TRUE;
    }
  else if(font==FreeSans12pt7b) {
    strcpy(lf->lfFaceName,"Arial");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=12;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_SWISS;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=7;
    lptm->tmAscent=9;
    lptm->tmAveCharWidth=7;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=3;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=12;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=7;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_FIXED_PITCH | FF_SWISS;
    return TRUE;
    }
  else if(font==FreeSerif9pt7b) {
    strcpy(lf->lfFaceName,"Times");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=9;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=7;
    lptm->tmAscent=6;
    lptm->tmAveCharWidth=7;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=2;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=9;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=7;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_FIXED_PITCH | FF_ROMAN;
    return TRUE;
    }
  else if(font==FreeSerif12pt7b) {
    strcpy(lf->lfFaceName,"Times");
    lf->lfCharSet=DEFAULT_CHARSET;
    lf->lfClipPrecision=CLIP_DEFAULT_PRECIS;
    lf->lfEscapement;
    lf->lfHeight=12;
    lf->lfItalic=0;
    lf->lfOrientation=0;
    lf->lfOutPrecision=OUT_DEFAULT_PRECIS;
    lf->lfPitchAndFamily=VARIABLE_PITCH | FF_ROMAN;
    lf->lfQuality=PROOF_QUALITY;
    lf->lfStrikeOut=0;
    lf->lfUnderline=0;
    lf->lfWeight=FW_NORMAL;
    lf->lfWidth=7;
    lptm->tmAscent=9;
    lptm->tmAveCharWidth=7;
    lptm->tmBreakChar=0x7e;
    lptm->tmCharSet=ANSI_CHARSET;
    lptm->tmDefaultChar=' ';
    lptm->tmDescent=3;
    lptm->tmDigitizedAspectX=1; lptm->tmDigitizedAspectY=1;
    lptm->tmExternalLeading=0;
    lptm->tmFirstChar=font->first;
    lptm->tmHeight=12;
    lptm->tmInternalLeading=0;
    lptm->tmItalic=0;
    lptm->tmUnderlined=0;
    lptm->tmStruckOut=0;
    lptm->tmLastChar=font->last;
    lptm->tmMaxCharWidth=7;
    lptm->tmOverhang=0;
    lptm->tmPitchAndFamily=TMPF_FIXED_PITCH | FF_ROMAN;
    return TRUE;
    }
  return FALSE;
  }
  
BOOL GetTextMetrics(HDC *hDC,TEXTMETRIC *lptm) {
  LOGFONT lf;
  
  return getFontInfo(hDC->font.font,&lf,lptm);
	}
  
int EnumFonts(HDC *hDC,const char *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam) {
  LOGFONT lf;
  TEXTMETRIC tm;

  if(lpLogfont) {
    if(!stricmp(lpLogfont,"system")) {
      getFontInfo(NULL,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"arial")) {
      getFontInfo(FreeSans9pt7b,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"times")) {
      getFontInfo(FreeSerif9pt7b,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    }
  else {
    getFontInfo(NULL,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    getFontInfo(FreeSans9pt7b,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    getFontInfo(FreeSerif9pt7b,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
  	}
	}

int EnumFontFamilies(HDC *hDC,const char *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam) {
  LOGFONT lf;
  TEXTMETRIC tm;

  if(lpLogfont) {
    if(!stricmp(lpLogfont,"system")) {
      getFontInfo(NULL,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"arial")) {
      getFontInfo(FreeSans9pt7b,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"times")) {
      getFontInfo(FreeSerif9pt7b,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    }
  else {
    getFontInfo(NULL,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    getFontInfo(FreeSans9pt7b,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    getFontInfo(FreeSerif9pt7b,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
  	}
	}

int EnumFontFamiliesEx(HDC *hDC,const char /*LOGFONT*/ *lpLogfont,FONTENUMPROC lpProc,LPARAM lParam,DWORD dwFlags) {
  LOGFONT lf;
  TEXTMETRIC tm;
// in teoria questa usa le strutture simili ma "EX" ... 
  if(lpLogfont) {
    if(!stricmp(lpLogfont,"system")) {
      getFontInfo(NULL,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"arial")) {
      getFontInfo(FreeSans9pt7b,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    else if(!stricmp(lpLogfont,"times")) {
      getFontInfo(FreeSerif9pt7b,&lf,&tm);
      lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
      }
    }
  else {
    getFontInfo(NULL,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    getFontInfo(FreeSans9pt7b,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);

    getFontInfo(FreeSerif9pt7b,&lf,&tm);
    lpProc(&lf,&tm,DEVICE_FONTTYPE,lParam);
    }
	}

int GetTextFace(HDC *hDC,WORD c,char *lpName) {
  LOGFONT lf;
  TEXTMETRIC tm;
  int i;
  
  i=getFontInfo(hDC->font.font,&lf,&tm);
  strncpy(lpName,lf.lfFaceName,c);
  return i;
  }


BOOL SelectObject(HDC *hDC,BYTE type,GDIOBJ g) {

	switch(type) {
		case OBJ_PEN:
			hDC->pen=g.pen;
			return TRUE;
			break;
		case OBJ_BRUSH:
			hDC->brush=g.brush;
			return TRUE;
			break;
		case OBJ_FONT:
			hDC->font=g.font;
			return TRUE;
			break;
		case OBJ_ICON:
//			hDC-> =g.icon;
			break;
		case OBJ_BITMAP:
//			hDC-> =g.bitmap;
			break;
		case OBJ_REGION:
//			hDC-> =g.region;
			break;
		}
	return FALSE;
	}

BOOL DeleteObject(BYTE type,GDIOBJ g) {
  // qua nulla :)
	switch(type) {
		case OBJ_PEN:
			return TRUE;
			break;
		case OBJ_BRUSH:
			return TRUE;
			break;
		case OBJ_FONT:
			return TRUE;
			break;
		case OBJ_ICON:
    	return TRUE;
			break;
		case OBJ_BITMAP:
    	return TRUE;
			break;
		case OBJ_REGION:
    	return TRUE;
			break;
		}
	return FALSE;
	}
  
GFX_COLOR SetTextColor(HDC *hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->foreColor;
  
  hDC->foreColor=color;
  return c;
  }

GFX_COLOR GetTextColor(HDC *hDC) {
  
  return hDC->foreColor;
  }

GFX_COLOR SetBkColor(HDC *hDC,GFX_COLOR color) {
  GFX_COLOR c=hDC->backColor;
  
  hDC->backColor=color;
  return c;
  }

GFX_COLOR GetBkColor(HDC *hDC) {
  
  return hDC->backColor;
  }

int GetBkMode(HDC *hDC) {
  
  return 0 /*OPAQUE, TRANSPARENT*/;
  }

UINT SetTextAlign(HDC *hDC,UINT align) {
  // return prec
  }

UINT GetTextAlign(HDC *hDC) {
  
//  return TA_BASELINE;   // finire ecc
  }

BOOL GetCharWidth(HDC *hDC,uint16_t iFirst,uint16_t iLast,uint16_t *lpBuffer) {
  TEXTMETRIC tm;
  BYTE i;
  
  GetTextMetrics(hDC,&tm);
  for(i=0; iFirst<iLast; iFirst++)
    lpBuffer[i] = hDC->font.size*tm.tmAveCharWidth;
  return TRUE;
  }

BOOL GetCharABCWidths(HDC *hDC,uint16_t wFirst,uint16_t wLast,uint16_t *lpBuffer /*ABC *lpABC*/) { // qua così!
  TEXTMETRIC tm;
  BYTE i;
  
  GetTextMetrics(hDC,&tm);
  for(i=0; wFirst<wLast; wFirst++)
    lpBuffer[i] = hDC->font.size*tm.tmAveCharWidth;
  return TRUE;
  }

BOOL GetTextExtentPoint32(HDC *hDC,const char *lpString,WORD n,SIZE *psizl) {
  TEXTMETRIC tm;
  
  GetTextMetrics(hDC,&tm);
  
  psizl->cx= psizl->cy= 0;
  while(n--) {
    psizl->cx += tm.tmAveCharWidth /* *lpString*/;
    psizl->cy = max(psizl->cy,tm.tmHeight);
    lpString++;
    }
// v. anche void getTextBounds(char *str, UGRAPH_COORD_T x, UGRAPH_COORD_T y, UGRAPH_COORD_T *x1, UGRAPH_COORD_T *y1, UGRAPH_COORD_T *w, UGRAPH_COORD_T *h) {

  return TRUE;
  }

static void DrawCharWindow(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y, unsigned char c) {
	INT8 i,j;
	const BYTE *fontPtr;

  if(!hDC->font.font) { // 'Classic' built-in font

    if(!_cp437 && (c >= 176)) 
			c++; // Handle 'classic' charset behavior
		if(c>=128)
			return;

    fontPtr=font+((uint16_t)c)*5;
    for(i=0; i<6; i++) {
      UINT8 line;
      UGRAPH_COORD_T xpos,ypos;
      BYTE doItalic;

      if(i<5) 
        line = *(fontPtr+i);
      else  
        line = 0x0;
      xpos=x+(i*hDC->font.size);
      for(j=0; j<8; j++, line >>= 1) {
        ypos=y+(j*hDC->font.size);
        if(hDC->font.italic && j<=3)
          doItalic=1;
        else
          doItalic=0;
        if(hDC->font.underline && j==7) {
          if(hDC->font.size == 1) 
            DrawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->foreColor);
          else
            FillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.size,ypos+hDC->font.size,hDC->foreColor);
          }
        else if(hDC->font.strikethrough && j==3) {
          if(hDC->font.size == 1) 
            DrawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->foreColor);
          else
            FillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.size,ypos+hDC->font.size,hDC->foreColor);
          }
        else if(line & 0x1) {
          if(hDC->font.size == 1) {
            DrawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->foreColor);
            if(hDC->font.bold)
              DrawPixelWindowColor(hDC,xpos+doItalic+1,ypos,hDC->foreColor);
            }
          else {
            FillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.size,ypos+hDC->font.size,hDC->foreColor);
            if(hDC->font.bold)
              FillRectangleWindowColor(hDC,xpos+doItalic+1,ypos,xpos+doItalic+1+hDC->font.size,ypos+hDC->font.size,hDC->foreColor);
            }
          } 
        else if(/*hDC->backColor != hDC->foreColor && */    (!hDC->font.bold ) ) {
// GESTIRE SetBkMode
          if(hDC->font.size == 1) 
            DrawPixelWindowColor(hDC,xpos+doItalic,ypos,hDC->backColor);
          else          
            FillRectangleWindowColor(hDC,xpos+doItalic,ypos,xpos+doItalic+hDC->font.size,ypos+hDC->font.size,hDC->backColor);
          }
        }
      }

    } 
  else { // Custom font
    GFXglyph *glyph;
    UINT8  *bitmap;
    UINT16 bo;
    UINT8  w, h, xa;
    INT8   xo, yo;
    UINT8  xx, yy, bits, bit;
    GRAPH_COORD_T  xo16, yo16;

    // Character is assumed previously filtered by write() to eliminate
    // newlines, returns, non-printable characters, etc.  Calling drawChar()
    // directly with 'bad' characters of font may cause mayhem!

    gfxFont=(GFXfont*)hDC->font.font;
    c -= gfxFont->first;
    glyph  = &gfxFont->glyph[c];
    bitmap = gfxFont->bitmap;

    bo = glyph->bitmapOffset;
    w  = glyph->width; h = glyph->height; xa = glyph->xAdvance;
    xo = glyph->xOffset; yo = glyph->yOffset;
    bit = 0;

    if(hDC->font.size > 1) {
      xo16 = xo;
      yo16 = yo;
      }

    // Todo: Add character clipping here

    // NOTE: THERE IS NO 'BACKGROUND' COLOR OPTION ON CUSTOM FONTS.
    // THIS IS ON PURPOSE AND BY DESIGN.  The background color feature
    // has typically been used with the 'classic' font to overwrite old
    // screen contents with new data.  This ONLY works because the
    // characters are a uniform size; it's not a sensible thing to do with
    // proportionally-spaced fonts with glyphs of varying sizes (and that
    // may overlap).  To replace previously-drawn text when using a custom
    // font, use the getTextBounds() function to determine the smallest
    // rectangle encompassing a string, erase the area with fillRect(),
    // then draw new text.  This WILL unfortunately 'blink' the text, but
    // is unavoidable.  Drawing 'background' pixels will NOT fix this,
    // only creates a new set of problems.  Have an idea to work around
    // this (a canvas object type for MCUs that can afford the RAM and
    // displays supporting setAddrWindow() and pushColors()), but haven't
    // implemented this yet.

    for(yy=0; yy<h; yy++) {
      for(xx=0; xx<w; xx++) {
        if(!(bit++ & 7)) {
          bits = bitmap[bo++];
          }
        if(bits & 0x80) {
          if(hDC->font.size == 1) {
            DrawPixelWindowColor(hDC,x+xo+xx, y+yo+yy,hDC->foreColor);
            } 
          else {
            FillRectangleWindowColor(hDC,x+(xo16+xx)*hDC->font.size, y+(yo16+yy)*hDC->font.size, 
                    x+(xo16+xx)*hDC->font.size+hDC->font.size, y+(yo16+yy)*hDC->font.size+hDC->font.size,hDC->foreColor);
            }
          }
        bits <<= 1;
        }
      }

    } // End classic vs custom font
  
	}

inline HWND __attribute__((always_inline)) WindowFromDC(HDC *hDC) {
  
  return hDC->hWnd;
  }

static BOOL DrawPixelWindow(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y) {
  HWND hWnd=WindowFromDC(hDC);
  
  x+=hDC->area.left;
  y+=hDC->area.top;
//    OffsetPoint(POINT *lppt,int16_t dx,int16_t dy);

	if(/*PtInRect*/ ((x < hWnd->paintArea.left) || (y < hWnd->paintArea.top)) || // verifico lo mia paint area..
    ((x >= hWnd->paintArea.right) || (y >= hWnd->paintArea.bottom)))
  	return FALSE;
  
  if(hWnd->parent /*hWnd->style & WS_CHILD*/) {  // se sono figlia, calcolo le mie coordinate reali
    HWND myWnd=hWnd->parent;
    while(myWnd) {
      x+=myWnd->clientArea.left;
      y+=myWnd->clientArea.top;
  //    OffsetRect(RECT *lprc,int16_t dx,int16_t dy);
      myWnd=myWnd->parent;
      }
    }
  
  if(isPointVisible(hDC,x,y)) {
   	DrawPixel(x,y,hDC->pen.color);
    return TRUE;
    }
  else 
    return FALSE;
  }

static BOOL DrawPixelWindowColor(HDC *hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y,GFX_COLOR c) {
  HWND hWnd=WindowFromDC(hDC);
  
  x+=hDC->area.left;
  y+=hDC->area.top;
//    OffsetPoint(POINT *lppt,int16_t dx,int16_t dy);
  
	if(/*PtInRect*/ ((x < hWnd->paintArea.left) || (y < hWnd->paintArea.top)) || // verifico lo mia paint area..
    ((x >= hWnd->paintArea.right) || (y >= hWnd->paintArea.bottom)))
  	return FALSE;
  
  if(hWnd->parent /*hWnd->style & WS_CHILD*/) {  // se sono figlia, calcolo le mie coordinate reali
    HWND myWnd=hWnd->parent;
    while(myWnd) {
      x+=myWnd->clientArea.left;
      y+=myWnd->clientArea.top;
  //    OffsetRect(RECT *lprc,int16_t dx,int16_t dy);
      myWnd=myWnd->parent;
      }
    }
  
  if(isPointVisible(hDC,x,y)) {
   	DrawPixel(x,y,c);
    return TRUE;
    }
  else 
    return FALSE;
  }

static void DrawHorizLineWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y,UGRAPH_COORD_T x2) {

  while(x1<x2)
    DrawPixelWindow(hDC,x1++,y);
  }

static void DrawHorizLineWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y,UGRAPH_COORD_T x2,GFX_COLOR c) {

  while(x1<x2)
    DrawPixelWindowColor(hDC,x1++,y,c);
  }

static void DrawVertLineWindow(HDC *hDC, UGRAPH_COORD_T x, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2) {
  
  while(y1<y2)
    DrawPixelWindow(hDC,x,y1++);
  }

static void DrawVertLineWindowColor(HDC *hDC, UGRAPH_COORD_T x, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2,GFX_COLOR c) {
  
  while(y1<y2)
    DrawPixelWindowColor(hDC,x,y1++,c);
  }

static void DrawLineWindow(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {
 	BOOL steep;
	GRAPH_COORD_T dx,dy;
	GRAPH_COORD_T err;
	GRAPH_COORD_T ystep;
	GRAPH_COORD_T xbegin;

  steep = abs(y2-y1) > abs(x2-x1);
  if(steep) {
    _swap(&x1, &y1);
    _swap(&x2, &y2);
    }
  if(x1>x2) {
    _swap(&x1, &x2);
    _swap(&y1, &y2);
    }

  dx = x2-x1;
  dy = abs(y2-y1);
  err = dx/2;
  ystep = y1<y2 ? 1 : -1;

  xbegin = x1;
  if(steep) {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            DrawVertLineWindow(hDC,y1,xbegin,xbegin+len+1);
          else {
            do {
              FillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size,xbegin+hDC->pen.size,hDC->pen.color);
              xbegin++;
              } while(len--);
            }
          }
        else {
          if(hDC->pen.size==1)
            DrawPixelWindow(hDC,y1,x1);
          else
            FillRectangleWindowColor(hDC,y1,x1,y1+hDC->pen.size,x1+hDC->pen.size,hDC->pen.color);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin+1) {
      if(hDC->pen.size==1)
        DrawVertLineWindow(hDC,y1,xbegin,x1+1);
      else {
        while(xbegin<=x1) {
          FillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size,xbegin+hDC->pen.size,hDC->pen.color);
          xbegin++;
          }
        }
      }
    } 
  else {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            DrawHorizLineWindow(hDC,xbegin,y1,xbegin+len+1);
          else {
            do {
              FillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size,y1+hDC->pen.size,hDC->pen.color);
              xbegin++;
              } while(len--);
            }
          }
        else {
          if(hDC->pen.size==1)
            DrawPixelWindow(hDC,x1,y1);
          else
            FillRectangleWindowColor(hDC,x1,y1,x1+hDC->pen.size,y1+hDC->pen.size,hDC->pen.color);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin+1) {
      if(hDC->pen.size==1)
        DrawHorizLineWindow(hDC,xbegin,y1,x1+1);
      else {
        while(xbegin<=x1) {
          FillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size,y1+hDC->pen.size,hDC->pen.color);
          xbegin++;
          }
        }
      }
    }

  }

static void DrawLineWindowColor(HDC *hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c) {
 	BOOL steep;
	GRAPH_COORD_T dx,dy;
	GRAPH_COORD_T err;
	GRAPH_COORD_T ystep;
	GRAPH_COORD_T xbegin;

  steep = abs(y2-y1) > abs(x2-x1);
  if(steep) {
    _swap(&x1, &y1);
    _swap(&x2, &y2);
    }
  if(x1>x2) {
    _swap(&x1, &x2);
    _swap(&y1, &y2);
    }

  dx = x2-x1;
  dy = abs(y2-y1);
  err = dx/2;
  ystep = y1<y2 ? 1 : -1;

  xbegin = x1;
  if(steep) {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            DrawVertLineWindowColor(hDC,y1,xbegin,xbegin+len+1,c);
          else {
            while(len--) {
              FillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size,xbegin+hDC->pen.size,c);
              xbegin++;
              }
            }
          }
        else {
          if(hDC->pen.size==1)
            DrawPixelWindowColor(hDC,y1,x1,c);
          else
            FillRectangleWindowColor(hDC,y1,x1,y1+hDC->pen.size,x1+hDC->pen.size,c);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin+1) {
      if(hDC->pen.size==1)
        DrawVertLineWindowColor(hDC,y1,xbegin,x1+1,c);
      else {
        while(xbegin<=x1) {
          FillRectangleWindowColor(hDC,y1,xbegin,y1+hDC->pen.size,xbegin+hDC->pen.size,c);
          xbegin++;
          }
        }
      }
    } 
  else {
    for(; x1<=x2; x1++) {
      err -= dy;
      if(err < 0) {
        INT16 len = x1-xbegin;
        if(len) {
          if(hDC->pen.size==1)
            DrawHorizLineWindowColor(hDC,xbegin,y1,xbegin+len+1,c);
          else {
            while(len--) {
              FillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size,y1+hDC->pen.size,c);
              xbegin++;
              }
            }
          }
        else {
          if(hDC->pen.size==1)
            DrawPixelWindowColor(hDC,x1,y1,c);
          else
            FillRectangleWindowColor(hDC,x1,y1,x1+hDC->pen.size,y1+hDC->pen.size,c);
          }
        xbegin = x1+1;
        y1 += ystep;
        err += dx;
        }
      }
    if(x1 > xbegin+1) {
      if(hDC->pen.size==1)
        DrawHorizLineWindowColor(hDC,xbegin,y1,x1+1,c);
      else {
        while(xbegin<=x1) {
          FillRectangleWindowColor(hDC,xbegin,y1,xbegin+hDC->pen.size,y1+hDC->pen.size,c);
          xbegin++;
          }
        }
      }
    }

  }

static void DrawRectangleWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {

  DrawHorizLineWindow(hDC,x1,y1,x2);
  DrawHorizLineWindow(hDC,x1,y2,x2);
  DrawVertLineWindow(hDC,x1,y1,y2);
  DrawVertLineWindow(hDC,x2,y1,y2+1);   // per chiudere
  }

static void DrawRectangleWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c) {

  DrawHorizLineWindowColor(hDC,x1,y1,x2,c);
  DrawHorizLineWindowColor(hDC,x1,y2,x2,c);
  DrawVertLineWindowColor(hDC,x1,y1,y2,c);
  DrawVertLineWindowColor(hDC,x2,y1,y2+1,c);    // per chiudere
  }

static void FillRectangleWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, 
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {

  while(y1<y2)
    DrawHorizLineWindowColor(hDC,x1,y1++,x2,hDC->brush.color);
  }

static void FillRectangleWindowColor(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, 
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c) {
  
  while(y1<y2)
    DrawHorizLineWindowColor(hDC,x1,y1++,x2,c);
  }

static void DrawCircleWindow(HDC *hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,
  UGRAPH_COORD_T x2, UGRAPH_COORD_T y2) {
  UGRAPH_COORD_T r=(x2-x1+y2-y1)/(2*2);
  GRAPH_COORD_T f = 1 - r;
  GRAPH_COORD_T ddF_x = 1;
  GRAPH_COORD_T ddF_y = -2 * r;
  GRAPH_COORD_T x = 0;
  GRAPH_COORD_T y = r;

  
  // IN EFFETTI, il cerchio andrebbe pieno!
  
  x2=(x1+x2)/2;
  y1=(y1+y2)/2;
    
  DrawPixelWindow(hDC,x1  , y1+r);
  DrawPixelWindow(hDC,x1  , y1-r);
  DrawPixelWindow(hDC,x1+r, y1  );
  DrawPixelWindow(hDC,x1-r, y1  );

  while(x<y) {
    if(f >= 0) {
      y--;
      ddF_y += 2;
      f += ddF_y;
      }
    x++;
    ddF_x += 2;
    f += ddF_x;

    DrawPixelWindow(hDC,x1 + x, y1 + y);
    DrawPixelWindow(hDC,x1 - x, y1 + y);
    DrawPixelWindow(hDC,x1 + x, y1 - y);
    DrawPixelWindow(hDC,x1 - x, y1 - y);
    DrawPixelWindow(hDC,x1 + y, y1 + x);
    DrawPixelWindow(hDC,x1 - y, y1 + x);
    DrawPixelWindow(hDC,x1 + y, y1 - x);
    DrawPixelWindow(hDC,x1 - y, y1 - x);
    }
  }

static BOOL isPointVisible(HDC *hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  // entro con coordinate schermo, ossia già traslate rispetto a MIO hDC e poi come figlia (di figlia ecc)
  HWND hWnd,myWnd;
  POINT pt;
  RECT rc;
  
  hWnd=WindowFromDC(hDC);
  
  if(hWnd->active /* o zOrder*/ && !(hWnd->style & (WS_CLIPCHILDREN | WS_CLIPSIBLINGS))) // ..quindi, se sono attiva o senza clipping, do ok
    goto check_cursor;
  
  pt.x=x; pt.y=y;
  if(hWnd->style & WS_CHILD) {    // ora, se sono figlia..
    if(hWnd->style & WS_CLIPSIBLINGS) {   // ..e devo evitare i fratelli, li controllo tutti
      myWnd=hWnd->parent->children;
      while(myWnd) {
        if(myWnd->visible && myWnd != hWnd && myWnd->zOrder > hWnd->zOrder) {    //
          OffsetRect2(&rc,&myWnd->nonClientArea,hWnd->parent->clientArea.left,hWnd->parent->clientArea.top);
          // CREDO Che dovremmo andare ricorsivamente su nei vari parent...
          if(PtInRect(&rc,pt)) {
            return 0;
            }
          }
        myWnd=myWnd->next;
        }
      }
    }
  
  if(hWnd->style & WS_CLIPCHILDREN) {   // ovvero, se devo ritagliare le figlie..
    myWnd=hWnd->children;
    while(myWnd) {
      if(myWnd->visible) {
        OffsetRect2(&rc,&myWnd->nonClientArea,hWnd->clientArea.left,hWnd->clientArea.top);
          // CREDO Che IDEM dovremmo andare ricorsivamente su nei vari parent...
/*                printf("ofs rc: %X rect: %u,%u,%u,%u - %u,%u\r\n",myWnd,
                      rc.left,rc.top,rc.right,rc.bottom,
                      pt.x,pt.y);*/
        if(PtInRect(&rc,pt)) {
          return 0;
          }
        }
      myWnd=myWnd->next;
      }
    }

  myWnd=hWnd->next;   // ora, le top level: inizio da quella subito sopra di me
  while(myWnd) {
    if(myWnd->zOrder > hWnd->zOrder) {    // in teoria questo test è inutile, dato il sort
      if(PtInRect(&myWnd->nonClientArea,pt)) {
        return 0;
        }
      }
    myWnd=myWnd->next;
    }

  if(taskbarWindow) {   // e la taskbar, se on top
    if(GetWindowByte(taskbarWindow,4+4) & 2)
      if(PtInRect(&taskbarWindow->nonClientArea,pt)) 
        return 0;
    }

check_cursor:
    /*
  rc.top=mousePosition.y;     // infine il mouse/puntatore
  rc.bottom=mousePosition.y+8;
  rc.left=mousePosition.x;
  rc.right=mousePosition.x+8;
	if(PtInRect(&rc,pt))
		return 0; con sprite non dovrebbe servire */

  return 1;
  }

HWND WindowFromPoint(POINT pt) { //not retrieve a handle to a hidden or disabled
  HWND myWnd,foundWnd=NULL;
	BYTE myOrder=0;
  
  myWnd=rootWindows;
  while(myWnd) {
//                printf("wfp wnd: %X rect: %u,%u,%u,%u - %u,%u,%u,%u\r\n",myWnd,
//                        myWnd->nonClientArea.left,myWnd->nonClientArea.top,myWnd->nonClientArea.right,myWnd->nonClientArea.bottom,
//                        myWnd->clientArea.left,myWnd->clientArea.top,myWnd->clientArea.right,myWnd->clientArea.bottom);
    if(myWnd->visible && /*myWnd->enabled */ myWnd->zOrder > myOrder && PtInRect(&myWnd->nonClientArea,pt)) {
			myOrder=myWnd->zOrder;
      foundWnd=myWnd;
      HWND myWnd2=myWnd->children;   // e ricorsiva?
      while(myWnd2) {
        RECT rc2;
//                printf(" wfp wnd: %X rc2: %u,%u,%u,%u - %u,%u,%u,%u\r\n",myWnd2,
//                        myWnd2->nonClientArea.left,myWnd2->nonClientArea.top,myWnd2->nonClientArea.right,myWnd2->nonClientArea.bottom,
//                        myWnd2->clientArea.left,myWnd2->clientArea.top,myWnd2->clientArea.right,myWnd2->clientArea.bottom);
        if(myWnd2->visible /*myWnd2->enabled && */ ) {
          OffsetRect2(&rc2,&myWnd2->nonClientArea,myWnd->clientArea.left,myWnd->clientArea.top);
//                printf(" pt: %u,%u rc2: %u,%u,%u,%u\r\n",
//                        pt.x,pt.y,rc2.left,rc2.top,rc2.right,rc2.bottom);
          if(PtInRect(&rc2,pt)) {
            // zOrder qua, o CLIPSIBLINGS o ?
            foundWnd=myWnd2;
            HWND myWnd3=myWnd2->children;   // e ricorsiva? per ora mi limito a 2, i.e. parent, dialog, controls
            while(myWnd3) {
              RECT rc3;
//                printf("  wfp wnd: %X rc3: %u,%u,%u,%u - %u,%u,%u,%u\r\n",myWnd3,
//                        myWnd3->nonClientArea.left,myWnd3->nonClientArea.top,myWnd3->nonClientArea.right,myWnd3->nonClientArea.bottom,
//                        myWnd3->clientArea.left,myWnd3->clientArea.top,myWnd3->clientArea.right,myWnd3->clientArea.bottom);
              if(myWnd3->visible  /*myWnd2->enabled && */ ) {
                OffsetRect2(&rc3,&myWnd3->nonClientArea,rc2.left+myWnd->clientArea.left,rc2.top+myWnd->clientArea.top);
                
                //v. anche clientPaint
//                printf("  pt: %u,%u rc3: %u,%u,%u,%u\r\n",
//                        pt.x,pt.y,rc3.left,rc3.top,rc3.right,rc3.bottom);
                        
                if(PtInRect(&rc3,pt)) {
                  foundWnd=myWnd3;
                  }
                }
              myWnd3=myWnd3->next;
              }
            }
          }
        myWnd2=myWnd2->next;
        }
      }
    myWnd=myWnd->next;
    }
  return foundWnd;
  }

HWND GetWindow(HWND hWnd,UINT uCmd) {
  HWND myWnd;
  
  switch(uCmd) {
    case GW_CHILD:
      // occhio anche a zorder cmq
      // getLastChildWindow()
      return hWnd->children;
      break;

    case GW_ENABLEDPOPUP:
      break;

    case GW_HWNDFIRST:
      // finire...
      break;

    case GW_HWNDLAST:
      break;

    case GW_HWNDNEXT:
      break;

    case GW_HWNDPREV:
      break;
    case GW_OWNER:
      return hWnd->parent;    // ma se parent è WS_CHILD allora salire di parent!
      break;
    }
  }

HWND getLastChildWindow(HWND hWnd) {
  HWND myWnd;
  
  myWnd=hWnd->children;
  while(myWnd && myWnd->next) {
    myWnd=myWnd->next;
    }
  return myWnd;
  }

BOOL IsChild(HWND hWndParent,HWND hWnd) {
  HWND myWnd;
  
  myWnd=hWndParent->children;
  while(myWnd) {
    if(myWnd==hWnd)
      return TRUE;
    myWnd=myWnd->next;
    }
  if(hWndParent->parent)    // bah ricorsiva direi ok così
    return IsChild(hWndParent->parent,hWnd);
  return FALSE;
  }

BOOL EnumChildWindows(HWND hWndParent,WNDENUMPROC lpEnumFunc,LPARAM lParam) {
  HWND myWnd;

  myWnd=hWndParent->children;
  while(myWnd) {
    if(!lpEnumFunc(myWnd,lParam))
      break;
    EnumChildWindows(myWnd,lpEnumFunc,lParam);    // ricorsivo per le child
    myWnd=myWnd->next;
    }
  }

BOOL EnumWindows(WNDENUMPROC lpEnumFunc,LPARAM lParam) {
  HWND myWnd;

  myWnd=rootWindows;
  while(myWnd) {
    if(!lpEnumFunc(myWnd,lParam))
      break;
    myWnd=myWnd->next;
    }
  }

BOOL EnumDesktopWindows(HWND hWndDesk,WNDENUMPROC lpEnumFunc,LPARAM lParam) {
  HWND myWnd;

/*  myWnd=hWndDesk;
  while(myWnd) {
 */
    lpEnumFunc(desktopWindow,lParam);
//    if(!lpEnumFunc(myWnd,lParam))
//      break;
//    myWnd=myWnd->next;
//    }
  }

HWND /*HDESK*/ GetThreadDesktop(DWORD dwThreadId) {
  
  return desktopWindow;   // bah sì :)
  }
HWND /*HDESK*/ OpenDesktop(const char *lpszDesktop,DWORD dwFlags,BOOL fInherit,DWORD /*ACCESS_MASK*/ dwDesiredAccess) {
  
  return desktopWindow;   // idem :)
  }

BOOL SetRect(RECT *lprc,UGRAPH_COORD_T xLeft,UGRAPH_COORD_T yTop,UGRAPH_COORD_T xRight,UGRAPH_COORD_T yBottom) {
  
  lprc->left=xLeft;
  lprc->right=xRight;
  lprc->top=yTop;
  lprc->bottom=yBottom;
  }

BOOL SetRectEmpty(RECT *lprc) {
  
  lprc->left=lprc->right=lprc->top=lprc->bottom=0;
  }

BOOL UnionRect(RECT *rcDst,const RECT *rcSrc1,const RECT *rcSrc2) {
  
  rcDst->left=min(rcSrc1->left,rcSrc2->left);
  rcDst->top=min(rcSrc1->top,rcSrc2->top);
  rcDst->right=max(rcSrc1->right,rcSrc2->right);
  rcDst->bottom=max(rcSrc1->bottom,rcSrc2->bottom);
  
  return !IsRectEmpty(rcDst);
  }

inline void __attribute__((always_inline)) OffsetRect(RECT *lprc,int16_t dx,int16_t dy) {
  
  lprc->left+=dx;
  lprc->right+=dx;
  lprc->top+=dy;
  lprc->bottom+=dy;
//  return TRUE;
  }

inline void __attribute__((always_inline)) OffsetPoint(POINT *lppt,int16_t dx,int16_t dy) {
  
  lppt->x += dx;
  lppt->y += dy;
  }

inline void __attribute__((always_inline)) OffsetRect2(RECT *lprcd,RECT *lprcs,int16_t dx,int16_t dy) {
  
  lprcd->left=lprcs->left+dx;
  lprcd->right=lprcs->right+dx;
  lprcd->top=lprcs->top+dy;
  lprcd->bottom=lprcs->bottom+dy;
//  return TRUE;
  }

BOOL IntersectRect(RECT *lprcDst,const RECT *lprcSrc1,const RECT *lprcSrc2) {
  
  if(lprcDst) {
    lprcDst->left = max(lprcSrc1->left, lprcSrc2->left);
    lprcDst->right = min(lprcSrc1->right, lprcSrc2->right);
    lprcDst->top = max(lprcSrc1->top, lprcSrc2->top);
    lprcDst->bottom= min(lprcSrc1->bottom, lprcSrc2->bottom);
    }
  return ! (lprcSrc2->left > lprcSrc1->right || lprcSrc2->right < lprcSrc1->left
        || lprcSrc2->top > lprcSrc1->bottom  || lprcSrc2->bottom < lprcSrc1->top);
  }

void InflateRect(RECT *lprc,int dx,int dy) {
  
  lprc->left-=dx;
  lprc->right+=dx;
  lprc->top-=dy;
  lprc->bottom+=dy;
//  return TRUE;
  }


BOOL SetMenu(HWND hWnd,MENU *Menu) {
  
  hWnd->menu=Menu;
  }
MENU *GetMenu(HWND hWnd) {
  
  return hWnd->menu;
  }
BOOL GetMenuInfo(MENU *Menu,MENUINFO *mi) {
  
  mi->dwStyle=Menu->menuItems[0].flags;// FINIRE!!
  mi->fMask=Menu->menuItems[0].flags;
  mi->hbrBack=GetStockObject(GRAY_BRUSH).brush;
  mi->cyMax=0;
  }
int GetMenuString(MENU hMenu,UINT uIDItem,LPSTR lpString,int cchMax,UINT flags) {
  }
int GetMenuItemCount(MENU *Menu) {
  BYTE i,nmax=0;
  MENUITEM *m;
  
  m=&Menu->menuItems[i];
  for(i=0; i<MAX_MENUITEMS; i++) { 
    if(m->bitmap || m->command || m->flags) {
      }
    else
      break;
    nmax++;
    }
  }
MENUITEM *GetMenuItemFromCommand(MENU *Menu,uint16_t cmd) {   // mia estensione :)
  BYTE i;
  MENUITEM *m;
  
  m=&Menu->menuItems[i];
  for(i=0; i<MAX_MENUITEMS; i++) { 
    if(m->command==cmd)
      return m;
    }
  }
uint16_t GetMenuItemID(MENU *Menu,BYTE nPos) {
  
  return Menu->menuItems[nPos].command;
  }
UINT GetMenuDefaultItem(MENU *Menu,uint16_t fByPos,uint16_t gmdiFlags) {
  }
UGRAPH_COORD_T GetMenuCheckMarkDimensions() { return 8; }     // v.CXMENUCHECK 
BOOL EnableMenuItem(MENU *hMenu,uint16_t uIDEnableItem,BYTE uEnable) {  
  }
DWORD CheckMenuItem(MENU *hMenu,uint16_t uIDEnableItem,uint16_t uCheck) {
  }
BOOL AppendMenuA(MENU *Menu,uint16_t uFlags,uint16_t uIDNewItem,const char *lpNewItem) {
  }
BOOL ModifyMenu(MENU *Menu,uint16_t uPosition,uint16_t uFlags,DWORD uIDNewItem,const char *lpNewItem) {
  }
BOOL InsertMenu(MENU *Menu,uint16_t uPosition,uint16_t uFlags,DWORD uIDNewItem,const char *lpNewItem) {
  }
BOOL InsertMenuItem(MENU *Menu,UINT item,BOOL fByPosition,MENUITEMINFO *lpmi) {
  }
MENU *GetSystemMenu(HWND hWnd,BOOL bRevert) {
  if(!hWnd)
    return (MENU*)&systemMenu;    // mia aggiunta :)
  else if(hWnd->style & WS_SYSMENU)
    return (MENU*)&systemMenu;    
  else
    return NULL;
  }
MENU *GetActiveMenu(void) {
  return activeMenu;
  }

static BOOL drawMenuPopup(HWND hWnd,MENU *menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  uint16_t xs,ys,x1;
  BYTE i,nmax=0;
  
  xs=0; ys=0;
  MENUITEM *m;
  for(i=0; i<MAX_MENUITEMS; i++) { 
    m=&menu->menuItems[i];
    if(m->bitmap || m->command || m->flags) {
      if(m->flags & MF_BITMAP) {
        if(m->flags & MF_POPUP)
          xs=max(xs,6+16);    // finire...
        else
          xs=max(xs,16);    // finire...
				ys+=MENU_HEIGHT+1;
        }
      else if(*m->text) {
        if(m->flags & MF_POPUP)
  				xs=max(xs,6+strlen(m->text)*6);     // font size fisso...
        else
  				xs=max(xs,strlen(m->text)*6);     // font size fisso...
				ys+=MENU_HEIGHT+1;
				}
      else if(m->flags & MF_SEPARATOR) {
				xs=max(xs,8);     // bah diciamo :) come emergenza
				ys+=2;
				}
      }
    else
      break;
    nmax++;
    }
  
  HDC myDC,*hDC;

  hDC=GetWindowDC(hWnd,&myDC);
  hDC->foreColor=hDC->pen.color=hWnd->active ? windowForeColor : windowInactiveForeColor;
  hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font; /*cmq lo stock? o anche custom?? */
  // cmq già in GetWindowDC
  
  xs+=8;    // per check
  FillRectangleWindowColor(hDC,x,y,x+xs+2,y+ys-1,windowBackColor);
  DrawRectangleWindow(hDC,x,y,x+xs+2,y+ys-1);
	y++;
  for(i=0; i<nmax; i++) { 
    x1=x;
    m=&menu->menuItems[i];
//    if(m->command == SC_CLOSE && hWnd->class == CS_NOCLOSE) disabilitare.. forse fare in INITMENU e ovviamente non in ROM!
    if(m->flags & MF_BITMAP) {
//        DrawBitmap(hDC,x,ys,m->bitmap);
      if(m->flags & MF_POPUP)
        DrawCharWindow(hDC,x1+14,y+1,'>');
			DrawHorizLineWindow(hDC,x,y+MENU_HEIGHT,x+xs+2);
			y+=MENU_HEIGHT+1;
      }
    else if(*m->text) {
      if(m->flags & MF_GRAYED)    //fare
        hDC->font.strikethrough = 1;    // boh :)
      if(m->flags & MF_DISABLED)    //fare
        hDC->font.strikethrough = 1;    // boh :)
      if(m->flags & MF_CHECKED) {
        DrawLineWindow(hDC,x1+2,y+MENU_HEIGHT/2,x1+4,y+MENU_HEIGHT);
        DrawLineWindow(hDC,x1+5,y+MENU_HEIGHT,x1+7,y+1);
        }
      char *s=(char *)m->text;
      while(*s) {   // nonclient area, non uso TextOut... v. anche sopra CAPTION
        DrawCharWindow(hDC,x1+9,y+1,*s++);
        x1 += 6;
        }
      hDC->font.strikethrough = 0;
      if(m->flags & MF_POPUP)
        DrawCharWindow(hDC,x+xs-6,y+1,'>');
      
      
      if(menu->accelerators[i].key[0])      // test/debug (troppo piccoli gli schermi per un testo vero..
        DrawCharWindow(hDC,x1+6,y+1,menu->accelerators[i].key[0]);
      
      
			DrawHorizLineWindow(hDC,x,y+MENU_HEIGHT,x+xs+2);
			y+=MENU_HEIGHT+1;
      }
    else if(m->flags & MF_SEPARATOR) {
			DrawHorizLineWindow(hDC,x,y,x+xs+2);
//      DrawLineWindowColor(hDC,x1+3,y+MENU_HEIGHT/2,x1+xs-2,y+MENU_HEIGHT/2,itemsColor);
			y+=1;
      }
    }
  
  ReleaseDC(hWnd,hDC);
  }

static uint16_t getMenuPopupFromPoint(HWND hWnd,MENU *menu,POINT pt,MENU **inMenu) {
  uint16_t xs,ys,x1;
  BYTE i;
  RECT rc;
  
  xs=0; ys=0;
  MENUITEM *m;
  for(i=0; i<MAX_MENUITEMS; i++) { 
    m=&menu->menuItems[i];
    
    
    if(m->bitmap || m->command || m->flags) {
      rc.top=hWnd->nonClientArea.top+TITLE_HEIGHT+1+MENU_HEIGHT+1;
      // andrà poi fatto ricorsivo per i vari sottopopup..
      rc.left=hWnd->nonClientArea.left+1 /*thickframe..*/;
      if(m->flags & MF_BITMAP) {
        if(m->flags & MF_POPUP)
          rc.right=max(rc.right,rc.right+6+16);     // finire
        else
          rc.right=max(rc.right,rc.right+16);     // 
        rc.bottom=rc.top+16+1;
        }
      else if(*m->text) {
        if(m->flags & MF_POPUP)
          rc.right=max(rc.right,rc.right+6+strlen(m->text)*6);     // font size fisso...
        else
          rc.right=max(rc.right,rc.right+strlen(m->text)*6);     // font size fisso...
        rc.bottom=rc.top+8+1;
        }
      else if(m->flags & MF_SEPARATOR) {
        rc.right=max(rc.right+xs,8);     // bah diciamo :) come emergenza
        rc.bottom=rc.top+2+1;
        }
      
      if(PtInRect(&rc,pt)) {
        if(inMenu)
          *inMenu=(MENU*)m;
        return m->command;
        }
      }
    else
      break;
    }
  
  if(inMenu)
    *inMenu=NULL;
  return 0;
  }

static BOOL drawMenu(HWND hWnd,MENU *menu,UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  uint16_t xs,x1;
  BYTE i;
  HDC myDC,*hDC;
  MENUITEM *m;

  hDC=GetWindowDC(hWnd,&myDC);
  hDC->foreColor=hDC->pen.color=hWnd->active ? windowForeColor : windowInactiveForeColor;

  i=0;

  do {
    m=&menu->menuItems[i];
    if(m && (m->bitmap || m->command || m->flags)) {
      if(m->flags & MF_BITMAP) {
        xs+= 16 +   2;    // per vertical line
        }
      else if(*m->text) {
        xs=strlen(m->text)*6;     // font size fisso...
        xs+=2;    // per vertical line
        }
      else if(m->flags & MF_SEPARATOR) {
        xs=2;
        }
      FillRectangleWindowColor(hDC,x,y+1,x+xs,y+MENU_HEIGHT,windowBackColor);
      // forse servirebbe rettangolone su tutta la barra... v. DrawMenuBar cmq
      DrawVertLineWindow(hDC,x+xs,y,y+MENU_HEIGHT);
      if(m->flags & MF_BITMAP) {
    //        DrawBitmap(hDC,x,ys,m->bitmap);
        }
      else if(*m->text) {
        x1=x+1;
        if(m->flags & MF_GRAYED)    //fare
          ;
        if(m->flags & MF_DISABLED)    //fare
          ;
        char *s=(char *)m->text;
        while(*s) {   // nonclient area, non uso TextOut... v. anche sopra CAPTION
          DrawCharWindow(hDC,x1,y+2,*s++);
          x1 += 6;
          }
        }
      else if(m->flags & MF_SEPARATOR) {
        DrawVertLineWindow(hDC,x+xs+1,y,y+MENU_HEIGHT); // si duplica se ultima ma ok..
        // opp pare + spessa e ok :)
        }

      x+=xs+2;
      }
    else
      break;
    i++;
    } while(m);
// non serve perché la nonclientPaint, però...  DrawHorizVertLineWindowColor(hDC,x,y+MENU_HEIGHT,x+xs+1,itemsColor);
  
  ReleaseDC(hWnd,hDC);
  }

static uint16_t getMenuFromPoint(HWND hWnd,POINT pt,MENU **inMenu) {
  RECT rc;
  BYTE i;
  MENU *menu=hWnd->menu;
  MENUITEM *m;

  i=0;

  rc.left=hWnd->nonClientArea.left+1 /*thickframe..*/;
  rc.right=rc.left;
  rc.top=hWnd->nonClientArea.top+TITLE_HEIGHT+1;
  rc.bottom=rc.top+1+MENU_HEIGHT;
  do {
    m=&menu->menuItems[i];
    if(m && (m->bitmap || m->command || m->flags)) {
      if(m->flags & MF_BITMAP) {
        rc.right += 16+1;  //finire..
        }
      else if(*m->text) {
        rc.right += strlen(m->text)*6 +1;     // font size fisso...
        }
      else if(m->flags & MF_SEPARATOR) {
        }
      
//        printf("cerco menu %u %u %u %u \r\n",rc.left,rc.top,rc.right,rc.bottom);
        
        
      if(PtInRect(&rc,pt)) {
        if(inMenu)
          *inMenu=(MENU*)m;
        
//        printf("trovo menu %x, command=%u, i=%u\r\n",m,m->command,i);
                
                
        return m->command;
        }
      }
    else
      break;
    
    rc.left = rc.right+1;
    rc.right = rc.left;

    i++;
    } while(m);

  if(inMenu)
    *inMenu=NULL;
    
  return 0;
  }

POINT getMenuPosition(MENU *menu2,MENU *menuorig) {
  POINT pt;
  BYTE i;
  MENUITEM *m;
  
  pt.y=0; // poi per popup...
  
  i=0;
  pt.x=0;

  do {
    m=&menuorig->menuItems[i];
//        printf("cerco menupos %x, %x, i=%u\r\n",menu2,m,i);
    if((MENU*)m == menu2)
      break;
    
    if(m && (m->bitmap || m->command || m->flags)) {
      if(m->flags & MF_BITMAP) {
        pt.x += 16 +   2;    // per vertical line
        }
      else if(m->flags & MF_SEPARATOR) {
        pt.x+=2;
        }
      else if(*m->text) {
        pt.x += 6*strlen((char *)m->text) + 1;
        }
      else if(m->flags & MF_SEPARATOR) {
        }
      }
    else  // NON deve accadere!
      break;
    
    i++;
    } while(m);
    
//        printf("trovo menupos pt=%u %u, i=%u\r\n",pt.x,pt.y,i);
        
  return pt;
  }

BOOL DrawMenuBar(HWND hWnd) {
  BYTE i;
  GFX_COLOR itemsColor=hWnd->active ? windowForeColor : windowInactiveForeColor;
  
  if(hWnd->menu) {
    drawMenu(hWnd,hWnd->menu,hWnd->style & WS_THICKFRAME ? 2 : 1,TITLE_HEIGHT);
//    DrawHorizLineWindowColor(&hWnd->hDC,(hWnd->style & WS_THICKFRAME ? 2 : 1),TITLE_HEIGHT+MENU_HEIGHT+1,
//            hWnd->nonClientArea.right - hWnd->nonClientArea.left - (hWnd->style & WS_THICKFRAME ? 2 : 1),itemsColor);    // bah sì, cmq
  // forse servirebbe anche il rettangolone su tutta la barra... v. sopra cmq
  // e forse non serve la linea horiz perché fa già nonclientPaint, idem sopra
    }
  }

static int matchAccelerator(MENU *menu,char ch) {
  int i,j;
  
  for(i=0; i<MAX_MENUITEMS; i++) {
    if(!menu->menuItems[i].bitmap && !menu->menuItems[i].command && !menu->menuItems[i].flags)
      break;
    if(menu->menuItems[i].menu /*implicito && menu->menuItems[i].flags & MF_POPUP*/) {
      if(j=matchAccelerator(menu->menuItems[i].menu,ch))    // ricerca ricorsiva..
        return j;
      }
    else {
      if(toupper(menu->accelerators[i].key[0]) == toupper(ch)) {
//                    hWnd->menu->accelerators[i].key[1] == 0b00000001) { CTRL per ora fisso
        return menu->menuItems[i].command;
        }
      }
    }
  return 0;
  }

HDC *GetWindowDC(HWND hWnd,HDC *hDC) {    // io faccio così, per risparmiare memoria dinamica
  
  hDC->hWnd=hWnd;
  hDC->area=hWnd->nonClientArea;
  hWnd->paintArea=hDC->area;
  
// GetWindowDC assigns default attributes to the window device context each time it retrieves the device context. Previous attributes are lost.
  hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font; /*cmq lo stock? o anche custom?? */;
  hDC->pen.size=1; hDC->pen.style=PS_SOLID;
  hDC->foreColor=hDC->pen.color=windowForeColor;
  hDC->backColor=hDC->brush.color=windowBackColor;
  
  return hDC;
	}

HDC *GetDC(HWND hWnd,HDC *hDC) {
  
  hDC->hWnd=hWnd;
  
  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_OWNDC) {   // fare.. un solo DC allocato per tutta la vita della window
    hDC=(HDC *)GET_WINDOW_OFFSET(hWnd,0);
    }
  if(wc->style & CS_PARENTDC)    // fare..
    ;
  if(wc->style & CS_CLASSDC) {    // fare.. stesso DC per tutte le classi
    hDC=(HDC *)GET_CLASS_OFFSET(hWnd,0);
    }
  
  hDC->area=hWnd->clientArea;
  hWnd->paintArea=hDC->area;
  
  switch(hWnd->class.class) {
    case WC_STATIC:
    case WC_BUTTON:
    case WC_COMBOBOX:
    case WC_EDIT:
    case WC_LISTBOX:
    case WC_SCROLLBAR:
    case WC_PROGRESS:
      hDC->font=*(FONT*)GET_WINDOW_OFFSET(hWnd,0);
      break;
    case WC_DIALOG:
    case WC_FILEDLG:
      hDC->font=*(FONT*)GET_WINDOW_OFFSET(hWnd,DWL_FONT);
      break;
    default:
      hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font; /*cmq lo stock? o anche custom?? MSDN dice stock, system*/;
      break;
    }
  hDC->pen.size=1; hDC->pen.style=PS_SOLID;
  hDC->foreColor=hDC->pen.color=WHITE;    // CONTRARIO di windows, almeno per ora :) e occhio minibasic con sfondo bianco..
  hDC->brush=wc->hbrBackground;
  hDC->backColor=hDC->brush.color /*BLACK*/;
  
	return hDC;
	}

HDC *GetDCEx(HWND hWnd,HDC *hDC,RECT /*HRGN */hrgnClip,DWORD flags) {
  return GetDC(hWnd,hDC);   // fare :)
  }
BOOL ReleaseDC(HWND hWnd,HDC *hDC) {
  
/*  WNDCLASS *wc;
  GetClassInfo(0,hWnd->class,&wc);
  if(wc->style & CS_OWNDC) {   // Class and private DCs do not have to be released.
    }
  if(wc->style & CS_PARENTDC)    // 
    ;
  if(wc->style & CS_CLASSDC) {    //
    }*/
  return TRUE;
	}

HDC *BeginPaint(HWND hWnd,PAINTSTRUCT *lpPaint) {

  GetDC(hWnd,&lpPaint->hDC);
  if(!IsRectEmpty(&hWnd->paintArea)) {
          //=lpPaint->hDC.area;
    lpPaint->fErase=!SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)&lpPaint->hDC,0);
// "BeginPaint sends a WM_ERASEBKGND message to the window". 
    }
  else
    lpPaint->fErase=FALSE;
//If the caret is in the area to be painted, BeginPaint automatically hides the caret to prevent it from being erased.
  return &lpPaint->hDC;
	}

BOOL EndPaint(HWND hWnd,const PAINTSTRUCT *lpPaint) {
  
  if(!lpPaint->fErase)
    SetRectEmpty(&hWnd->paintArea);
  ReleaseDC(hWnd,&(((PAINTSTRUCT *)lpPaint)->hDC));
	}

// -------------------------------------------------------------------------------
WNDCLASS baseClass= {
  MAKECLASS(WC_DEFAULTCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  windowIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProc,
  NULL,
  0,
  0,
  NULL,
  {BS_SOLID,1,DARKGRAY}
  };
struct WNDCLASS_STATIC {
  WNDCLASS baseClass;
  } staticClass = { {
  MAKECLASS(WC_STATIC),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcStaticWC,
  NULL,
  0,
  sizeof(FONT),    // [testo esteso, se si usa]
  NULL,
  {BS_SOLID,1,DARKGRAY}
  }
  };
struct WNDCLASS_BUTTON {
  WNDCLASS baseClass;
  } buttonClass = { {
  MAKECLASS(WC_BUTTON),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcButtonWC,
  NULL,
  0,
  sizeof(FONT)+4, // BYTE [non usato, era stile];  BYTE state;	BYTE hotkey; sistemare ev.
  NULL,
  {BS_SOLID,1,GRAY204}
  }
  };
struct WNDCLASS_EDIT {
  WNDCLASS baseClass;
  } editClass = { {
  MAKECLASS(WC_EDIT),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcEditWC,
  NULL,
  0,
  sizeof(FONT)+4+32,  //	BYTE caretPos;	BYTE selStart,selEnd;  BYTE content[32];
  NULL,
  {BS_SOLID,1,GRAY224}
  }
  };
struct WNDCLASS_LISTBOX {
  WNDCLASS baseClass;
  } listboxClass = { {
  MAKECLASS(WC_LISTBOX),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcListboxWC,
  NULL,
  0,
  sizeof(FONT)+4+4, //   BYTE *data; 	BYTE selectedLine;
  NULL,
  {BS_SOLID,1,GRAY224}
  }   
  };
struct WNDCLASS_DIALOG {
  WNDCLASS baseClass;
  } dialogClass = { {
  MAKECLASS(WC_DIALOG),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  windowIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcDlgWC,
  NULL,
  0,
  DWL_FONT+sizeof(FONT),		// di base, poi FONT
  NULL,
  {BS_SOLID,1,GRAY224}
  }
  };
struct WNDCLASS_FILEBROWSER {
  WNDCLASS WNDCLASS_DIALOG;
  } fileDialogClass= { {
  MAKECLASS(WC_FILEDLG),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcFileDlgWC,
  (MENU*)NULL,
  0,
  DWL_USER+sizeof(OPENFILENAME),   //   OPENFILENAME ofn
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_DIR {
  WNDCLASS baseClass;
  } diskExplorerClass= { {
  MAKECLASS(WC_DIRCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcDirWC,
  (MENU*)&explorerMenu,
  0,
  4+sizeof(DIRLIST),   //   4 generici; DIRLIST
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };
struct WNDCLASS_CMDSHELL {
  WNDCLASS baseClass;
  } cmdShellClass= { {
  MAKECLASS(WC_CMDSHELL),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcCmdShellWC,
  (MENU*)&cmdShellMenu,
  0,
  1+1+1+1+1+32,   //   x,y,coloref,coloreb,statocursore,commandline
  NULL,
  {BS_SOLID,1,BLACK}
  }
  };
struct WNDCLASS_DESKTOP {
  WNDCLASS baseClass;
  } desktopClass = { {
  MAKECLASS(WC_DESKTOPCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcDC,
  NULL,
  0,
  2+2+sizeof(FONT)+16+sizeof(S_POINT)*16+1,    //  colore f/b, font, file wallpaper, S_POINT iconPosition[16] , HWND START, HWND orologio, attributi
// aggiungere DWORD jpegLen;
// BYTE *jpegPtr;
// SUPERFILE *jpegFile;   // metterli tutti dentro la classe desktopclass... union..

  NULL,
  {BS_SOLID,1,DARKBLUE}
  }
  };
struct WNDCLASS_TASKBAR {
  WNDCLASS baseClass;
  } taskbarClass = { {
  MAKECLASS(WC_TASKBARCLASS),
  CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS,
  standardIcon,
  standardCursor,
  (WINDOWPROC *)DefWindowProcTaskBar,
  NULL,
  0,
  4+4+1,    //  HWND START, HWND orologio, attributi
  NULL,
  {BS_SOLID,1,LIGHTGRAY}
  }
  };

BOOL GetClassInfo(HINSTANCE hInstance, CLASS Class, WNDCLASS **lpWndClass) {
  
  switch(Class.class) {
    case WC_STATIC:
      *lpWndClass=(WNDCLASS *)&staticClass;
      break;
    case WC_BUTTON:
      *lpWndClass=(WNDCLASS *)&buttonClass;
      break;
    case WC_COMBOBOX:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_EDIT:
      *lpWndClass=(WNDCLASS *)&editClass;
      break;
    case WC_DIALOG:
      *lpWndClass=(WNDCLASS *)&dialogClass;
      break;
    case WC_LISTBOX:
      *lpWndClass=(WNDCLASS *)&listboxClass;
      break;
    case WC_SCROLLBAR:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_HOTKEY:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_PROGRESS:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_STATUS:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_TRACKBAR:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_UPDOWN:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
    case WC_FILEDLG:
      *lpWndClass=(WNDCLASS *)&fileDialogClass;
      break;
    case WC_DIRCLASS:
      *lpWndClass=(WNDCLASS *)&diskExplorerClass;
      break;

    case WC_CMDSHELL:
      *lpWndClass=(WNDCLASS *)&cmdShellClass;
      break;
    case WC_DESKTOPCLASS:
      *lpWndClass=(WNDCLASS *)&desktopClass;
      break;
    case WC_TASKBARCLASS:
      *lpWndClass=(WNDCLASS *)&taskbarClass;
      break;

    case WC_DEFAULTCLASS:
      *lpWndClass=(WNDCLASS *)&baseClass;
      break;
      
    default:
    {
      WNDCLASS *myClass=rootClasses;
      while(myClass) {
        if(myClass->class.class==Class.class) {
          *lpWndClass=myClass;
          return TRUE;
          break;
          }
        myClass=myClass->next;
        }
      return FALSE;
    }

      break;
    }
  return TRUE;
  }


SHORT GetKeyState(int nVirtKey) {
	}
SHORT GetAsyncKeyState(int vKey) {

	switch(vKey) {
		case VK_CONTROL:
			if(keypress.modifier & 0b00010001)
				return 0x8001;		// finire
			break;
		case VK_MENU:
			if(keypress.modifier & 0b01000100)
				return 0x8001;		// finire
			break;
		case VK_SHIFT:
			if(keypress.modifier & 0b00100010)
				return 0x8001;		// finire
			break;
		case VK_RETURN:
			if(keypress.key == '\n')
				return 0x8001;		// finire
			break;
		default:
			if(keypress.key == vKey)
				return 0x8001;		// finire
			break;
		}
	return 0;    }


// -------------------------------------------------------------------------------
#if 0
void SortLinkedListValue(struct _WINDOW *node) {   //https://stackoverflow.com/questions/35914574/sorting-linked-list-simplest-way
  struct _WINDOW *temp;
  int tempvar,j;  //temp variable to store node data
    
  temp = node->next;//temp node to hold node data and next link
  while(node && node->next) {
    for(j=0; j<maxWindows; j++) {   //[value 5 because I am taking only 5 nodes]
      if(node->zOrder > temp->zOrder) {   //swap node data
        tempvar = node->zOrder;
        node->zOrder= temp->zOrder;
        temp->zOrder = tempvar;
        }
      temp = temp->next;
      }
    node = node->next;
    }
  }
 
static void SortLinkedList(struct _WINDOW *head) {     // https://www.javatpoint.com/program-to-sort-the-elements-of-the-singly-linked-list
  struct _WINDOW *node = head, *i, *j;
  struct _WINDOW *temp;
  
#if 0
  //Node current will point to head  
  struct _WINDOW *current = head, *index = NULL;  
  int temp;  

  if(!head) {  
    return;  
    } 
  else {  
    while(current) {  
      //Node index will point to node next to current  
      index = current->next;  

      while(index ) {  
        //If current node's data is greater than index's node data, swap the data between them  
        if(current->zOrder > index->zOrder) {  
          temp = current->zOrder;  
          current->zOrder = index->zOrder;  
          index->zOrder = temp;  
          }  
        index = index->next;  
        }  
      current = current->next;  
      }      
    }  
#endif
  
  // https://stackoverflow.com/questions/8981823/bubble-sort-algorithm-for-a-linked-list
  i = node;
  while(i) {
    j = node->next;
    while(j) {
      if(i->zOrder > j->zOrder) {
        temp = i->next;
        i->next = j->next;
        j->next = temp;
        }
      j = j->next;
      }
    i = i->next;
    }
  }
#endif

void list_bubble_sort(struct _WINDOW **head) {    //https://stackoverflow.com/questions/21388916/bubble-sort-singly-linked-list-in-c-with-pointers
  int done = 0;         // True if no swaps were made in a pass

  // Don't try to sort empty or single-node lists
  if(!*head || !((*head)->next))
    return;

  while(!done) {
    struct _WINDOW **pv = head;         // "source" of the pointer to the current node in the list struct
    struct _WINDOW *nd = *head;            // local iterator pointer
    struct _WINDOW *nx = (*head)->next;  // local next pointer

    done = 1;

    while(nx) {
      if(nd->zOrder > nx->zOrder) {   // ordino da più basso a più alto (v. anche ZCheckWindow e HWND_TOP ecc)
        nd->next = nx->next;
        nx->next = nd;
        *pv = nx;
        done = 0;
        }
      pv = &nd->next;
      nd = nx;
      nx = nx->next;
      }
    }
  }

void handleWinTimers(void) {
  int i;
  
  for(i=0; i<MAX_TIMERS; i++) {
    if(timerData[i].uEvent) {
      if(timerData[i].time_cnt) {
        timerData[i].time_cnt--;
        if(!timerData[i].time_cnt) {
//      LED1^=1;
//      printf("timer %x,%u,%u\r\n",timerData[i].hWnd,timerData[i].uEvent,timerData[i].timeout);
          if(timerData[i].hWnd)
            SendMessage(timerData[i].hWnd,WM_TIMER,timerData[i].uEvent,(LPARAM)timerData[i].tproc);
          else if(timerData[i].tproc)
            timerData[i].tproc(timerData[i].hWnd,0,0,0);
          else
            goto skip;
          timerData[i].time_cnt=timerData[i].timeout;
skip: ;
          }
        }
      }
    }
  }

BYTE handle_filechanges(char drive,BYTE mode) {
  HWND hwnd=rootWindows;
  BYTE i=0;
  
  while(hwnd) {
    if(hwnd->class.class==WC_DIRCLASS) {
      SendMessage(hwnd,WM_FILECHANGE,drive,MAKELONG(mode,0)); // qua tutti...
      i=1;
      }
    hwnd=hwnd->next;
    }
  return i;
  }

BYTE handle_console_output(char ch,BYTE m) {
  HWND hwnd=rootWindows;
  BYTE i=0;
  
  while(hwnd) {
    if(hwnd->class.class==WC_CMDSHELL) {
      SendMessage(hwnd,WM_PRINTCHAR,ch,m ? MAKELONG(LIGHTRED,0) : -1);
      i=1;
      break;      // direi SOLO UNA! e magari scegliere quale, in caso...
      }
    hwnd=hwnd->next;
    }
  return i;
  }

/*static*/ uint8_t initADC(void) {
// ovviamente verificare con INADC del minibasic...
  
  ADCCON1=0;    // AICPMPEN=0, siamo sopra 2.5V
  CFGCONbits.IOANCPEN=0;    // idem
  ADCCON2=0;
  ADCCON3=0;
  
  //Configure Analog Ports
  ADCCON3bits.VREFSEL = 0; //Set Vref to VREF+/-

  ADCCMPEN1=0x00000000;
  ADCCMPEN2=0x00000000;
  ADCCMPEN3=0x00000000;
  ADCCMPEN4=0x00000000;
  ADCCMPEN5=0x00000000;
  ADCCMPEN6=0x00000000;
  ADCFLTR1=0x00000000;
  ADCFLTR2=0x00000000;
  ADCFLTR3=0x00000000;
  ADCFLTR4=0x00000000;
  ADCFLTR5=0x00000000;
  ADCFLTR6=0x00000000;
  
  ADCFSTAT=0;
  
  ADCTRGMODE=0;
  ADCTRGSNS=0;

  ADCTRG1=0;
  ADCTRG2=0;
  ADCTRG3=0;
  ADCTRGSNSbits.LVL3 = 0; // Edge trigger
  ADCTRGSNSbits.LVL4 = 0; 
  ADCTRG1bits.TRGSRC3 = 0b00001; // Set AN3 to trigger from software
  ADCTRG2bits.TRGSRC4 = 0b00001; // Set AN4 to trigger from software
  
  // I PRIMI 12 POSSONO OVVERO DEVONO USARE gli ADC dedicati! e anche se si usano
  // poi gli SCAN, per quelli >12, bisogna usarli entrambi (e quindi TRGSRC passa a STRIG ossia "common")

  ADCIMCON1bits.DIFF3 = 0; // single ended, unsigned
  ADCIMCON1bits.SIGN3 = 0; // 
  ADCIMCON1bits.DIFF4 = 0;
  ADCIMCON1bits.SIGN4 = 0;
   
  // Initialize warm up time register
  ADCANCON = 0;
  ADCANCONbits.WKUPCLKCNT = 5; // Wakeup exponent = 32 * TADx

  ADCEIEN1 = 0;
    
  ADCCON2bits.ADCDIV = 64; // per SHARED: 2 TQ * (ADCDIV<6:0>) = 64 * TQ = TAD
  ADCCON2bits.SAMC = 5;
    
  ADCCON3bits.ADCSEL = 0;   //0=periph clock 3; 1=SYSCLK
  ADCCON3bits.CONCLKDIV = 4; // 25MHz, sotto è poi diviso 2 per il canale, = max 50MHz come da doc

  ADC3TIMEbits.SELRES=0b10;        // 10 bits
  ADC3TIMEbits.ADCDIV=4;       // 
  ADC3TIMEbits.SAMC=5;        // 
  ADC4TIMEbits.SELRES=0b10;        // 10 bits
  ADC4TIMEbits.ADCDIV=4;       // 
  ADC4TIMEbits.SAMC=5;        //   
  
  ADCCSS1 = 0; // Clear all bits
  ADCCSS2 = 0;

  ADC0CFG=DEVADC0;
  ADC1CFG=DEVADC1;
  ADC2CFG=DEVADC2;
  ADC3CFG=DEVADC3;
  ADC4CFG=DEVADC4;
  ADC7CFG=DEVADC7;

  ADCCON1bits.ON = 1;   //Enable AD
  ClrWdt();
  
  // Wait for voltage reference to be stable 
#ifndef USING_SIMULATOR
  while(!ADCCON2bits.BGVRRDY); // Wait until the reference voltage is ready
  //while(ADCCON2bits.REFFLT); // Wait if there is a fault with the reference voltage
#endif

  // Enable clock to the module.
  
  ADCANCONbits.ANEN3 = 1;
  ADCCON3bits.DIGEN3 = 1;
  ADCANCONbits.ANEN4 = 1;
  ADCCON3bits.DIGEN4 = 1;
#ifndef USING_SIMULATOR
  while(!ADCANCONbits.WKRDY3); // Wait until ADC is ready
  while(!ADCANCONbits.WKRDY4); // Wait until ADC is ready
#endif
  
  // ADCGIRQEN1bits.AGIEN4=1;     // IRQ (anche ev. per DMA))

  return 1;
	}

static uint16_t readADC(uint8_t channel) {

//	__delay_us(50);
  ANSELBbits.ANSB3 = 0;
  ANSELBbits.ANSB4 = 0;
  switch(channel) {
    case 0:   // B3
      ANSELBbits.ANSB3 = 1;
//      ADCCON3bits.ADINSEL = 2;
      break;
    case 1:   // B4
      ANSELBbits.ANSB4 = 1;
//      ADCCON3bits.ADINSEL = 4;
      break;
    }
	__delay_us(30);

  ADCCON3bits.GSWTRG = 1; // Start software trigger

  switch(channel) {
    case 0:   // B3
      while(ADCDSTAT1bits.ARDY3 == 0) // Wait until the measurement run
        ClrWdt();
      ANSELBbits.ANSB3 = 0;
      ANSELBbits.ANSB4 = 0;
      ADCDATA4;
      // PARE che quando mandi il trigger, lui converte TUTTI i canali abilitati,
      // per cui se non pulisco "l'altro" mi becco un RDY e una lettura precedente...
// forse COSI' funzinerebbe, PROVARE      while(ADCCON2bits.EOSRDY == 0) // Wait until the measurement run
//        ClrWdt();
      return ADCDATA3 >> 2;
      break;
    case 1:   // B4
      while(ADCDSTAT1bits.ARDY4 == 0) // Wait until the measurement run
        ClrWdt();
      ANSELBbits.ANSB3 = 0;
      ANSELBbits.ANSB4 = 0;
      ADCDATA3;
      return ADCDATA4 >> 2;
      break;
    }

	}

// il PORCO e il DIO non è così, madonna di merda!!
//#define YP B3 //A2  
//#define XM B2 //A3 
//#define YM E6 //8   
//#define XP E7 //9   
// https://forum.arduino.cc/t/getting-bmps-to-work-on-3-1-2-touch-screen/691080
#define YP B4 //A1
#define XM B3 //A2 
#define XP F5 //6  
#define YM E5 //7   
#define RPLATE 300
#define NUMSAMPLES 2
#define SAMPLE_NOISE 6
BYTE manageTouchScreen(UGRAPH_COORD_T *x,UGRAPH_COORD_T *y,uint8_t *z) {
  int samples[NUMSAMPLES];
  uint8_t i, valid;

  valid = 1;

  TRISBbits.TRISB4=1;    //(_yp, INPUT);
  TRISEbits.TRISE5=1;    //(_ym, INPUT);
  TRISFbits.TRISF5=0;    //(_xp, OUTPUT);
  TRISBbits.TRISB3=0;    //(_xm, OUTPUT);

  LATFbits.LATF5=1;      //(_xp, HIGH);
  LATBbits.LATB3=0;      //(_xm, LOW);

  __delay_us(30); // Fast ARM chips need to allow voltages to settle

  for(i=0; i<NUMSAMPLES; i++) {
    samples[i] = readADC(1 /*_yp*/);
    }

  TRISBbits.TRISB4=0;    //(_yp, OUTPUT);
  TRISEbits.TRISE5=0;    //(_ym, OUTPUT);
  TRISFbits.TRISF5=1;    //(_xp, INPUT);
  TRISBbits.TRISB3=1;    //(_xm, INPUT);

  LATEbits.LATE5=1;      //(_ym, LOW);
  LATBbits.LATB4=0;      //(_yp, HIGH);
  
  
#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   // Allow small amount of measurement noise, because capacitive
   // coupling to a TFT display's signals can induce some noise.
  if(((samples[0] - samples[1]) < -SAMPLE_NOISE) || ((samples[0] - samples[1]) > SAMPLE_NOISE) || 
          samples[0]==0 || samples[0]==1023) {
    valid = 0;
    } 
  else {
    samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
    }
#endif

  *y = 1023-(1023-samples[NUMSAMPLES/2]);
// qui li inverto... x con y e poi la direzione di y (credo dipenda dalla Rotazione)


  __delay_us(30); // Fast ARM chips need to allow voltages to settle

  for(i=0; i<NUMSAMPLES; i++) {
    samples[i] = readADC(0 /*_xm*/);
    }

   // Set X+ to ground
   // Set Y- to VCC
   // Hi-Z X- and Y+
  TRISBbits.TRISB4=1;    //(_yp, INPUT);
  TRISEbits.TRISE5=0;    //(_ym, OUTPUT);
  TRISFbits.TRISF5=0;    //(_xp, OUTPUT);
  TRISBbits.TRISB3=1;    //(_xm, INPUT);

  LATEbits.LATE5=1;      //(_ym, HIGH); 
  LATFbits.LATF5=0;      //(_xp, LOW);
  
  
#if NUMSAMPLES > 2
   insert_sort(samples, NUMSAMPLES);
#endif
#if NUMSAMPLES == 2
   // Allow small amount of measurement noise, because capacitive
   // coupling to a TFT display's signals can induce some noise.
  if(((samples[0] - samples[1]) < -SAMPLE_NOISE) || ((samples[0] - samples[1]) > SAMPLE_NOISE) || 
          samples[0]==0 || samples[0]==1023) {
    valid = 0;
    } 
  else {
    samples[1] = (samples[0] + samples[1]) >> 1; // average 2 samples
    }
#endif

  *x = (1023-samples[NUMSAMPLES/2]);
  
//  *y = readADC(0);
  

  int z1 = readADC(0 /*_xm*/); 
  int z2 = readADC(1 /*_yp*/);

  if(RPLATE != 0) {
    // now read the x 
    float rtouch;
    rtouch = z2;
    rtouch /= z1;
    rtouch -= 1;
    rtouch *= *x;
    rtouch *= RPLATE;
    rtouch /= 1024;
     
    *z = rtouch;
    // abbiamo da 200 a 150 (< se + pressione) con stilo, 300..200 con cotton fioc :) 250..50 con dito
    } 
  else {
    *z = (1023-(z2-z1));
    }

  if(!valid) {
    *z = 0;
    }

  TRISBbits.TRISB3=0;
  TRISBbits.TRISB4=0;
  TRISEbits.TRISE5=0;
  TRISFbits.TRISF5=0;
  ANSELBbits.ANSB3 = 0;
  ANSELBbits.ANSB4 = 0;
//  ANSELEbits.ANSE5 = 0;
//  ANSELFbits.ANSF5 = 0;
  
  return valid;
  }


extern HWND m_Wnd,m_Wnd2,m_Wnd3,m_Wnd4,m_Wnd5,m_Wnd6;
#define TIME_DCLICK 400
int8_t manageWindows(BYTE mouseClick) {
  int i;
  HWND hWnd;
  static DWORD oldclick=0;
  BYTE dclick;
  int nchit;
  
 
#ifdef USA_USB_SLAVE_CDC  
  if(mouseClick) {
    CURSOR myCursor;
    
    hWnd=WindowFromPoint(mousePosition);
    if(hWnd) {
      myCursor=(CURSOR)SendMessage(hWnd,WM_SETCURSOR,0,0);
      if(!myCursor)
        myCursor=standardCursor;
      }
    else
      myCursor=standardCursor;
    DrawCursor(mousePosition.x,mousePosition.y,myCursor,0);

    if(hWnd) {
      i=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
      if(i != HTNOWHERE && i != HTCLIENT)
        SendMessage(hWnd,WM_NCMOUSEMOVE,i,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
      }

    goto is_click;
    }
  
  if(peekUSBUSART()) {
    
    DWORD f;
    static BYTE oldch;
    
    i=(BYTE)getcUSBUSART();
    f=1;          // flag vari, mettere.. https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-char
    f |= i << 16;
    if(i == oldch)
      f |= 0x40000000;    // boh sì :)
    
    hWnd=GetForegroundWindow();
    if(hWnd)
      SendMessage(hWnd,WM_CHAR,i,f);
    

		switch(i) {
			case '*':
      {
        hWnd=GetForegroundWindow();
        if(hWnd)
          SendMessage(hWnd,WM_INITMENUPOPUP,0,MAKELONG(0,1));
      }
				break;
				break;

			case '-':
        if(m_Wnd2->maximized) {
          ShowWindow(m_Wnd2,SW_SHOWNORMAL);
          }
        else
          ShowWindow(m_Wnd2,SW_SHOWMAXIMIZED);
				break;
        
			case '/':
        w++; w %= 6;
        switch(w) {
          case 0:
            ShowWindow(m_Wnd,SW_SHOWNORMAL);
            break;
          case 1:
            ShowWindow(m_Wnd2,SW_SHOWNORMAL);
            break;
          case 2:
            ShowWindow(m_Wnd3,SW_SHOWNORMAL);
            break;
          case 3:
            ShowWindow(m_Wnd4,SW_SHOWNORMAL);
            break;
          case 4:
            ShowWindow(m_Wnd5,SW_SHOWNORMAL);
            break;
          case 5:

            break;
          case 6:
            ShowWindow(m_Wnd6,SW_SHOWNORMAL);
            break;
          }
				break;
			case VK_SPACE:
				break;
      case VK_ESCAPE:
        break;
			case 'm':
        if(IsIconic(m_Wnd))
          ShowWindow(m_Wnd,SW_SHOWNORMAL);
        else
          ShowWindow(m_Wnd,SW_SHOWMINIMIZED);
				break;
			case 'b':
        if(m_Wnd2) {
          SendMessage(m_Wnd2,WM_CLOSE,0,0);
          m_Wnd2=NULL;
          }
        else {
          m_Wnd2=CreateWindow(MAKECLASS(MAKEFOURCC('M','B','A','S')),"MiniBASIC",
            WS_ACTIVE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            160,2+rand() % 8,159,170,
            NULL,NULL,(void*)1
            );
          }
				break;
			case 'c':
        if(m_Wnd3) {
          SendMessage(m_Wnd3,WM_CLOSE,0,0);
          m_Wnd3=NULL;
          }
        else {
          m_Wnd3=CreateWindow(MAKECLASS(WC_DEFAULTCLASS),NULL,WS_BORDER | WS_VISIBLE | WS_DISABLED,
            8+rand() % 8,210,20,18,
            NULL,(MENU *)NULL,(void*)3
            );
          }
				break;
			case 'i':
	      SendMessage(m_Wnd2,WM_NOTIFY,0,CDN_TYPECHANGE);
				break;
			case 'x':
				MessageBox(m_Wnd /* NULL*/,"attenzione!","PROVA",MB_OKCANCEL | MB_ICONEXCLAMATION);
				break;
      case 'w':
        printWindows(rootWindows);
        break;

			case 'a':
//        putcUSBUSART(m_Wnd3->font.font);
//        putcUSBUSART(m_Wnd3->foreColor);
//        putcUSBUSART(m_Wnd3->font.attributes);
				break;
			case VK_NUMPAD8:
			case '8':
				if(mousePosition.y>0)
          mousePosition.y--;
        goto drawcursor;
				break;
//			case VK_NUMPAD2:
			case '2':
				if(mousePosition.y<_height-1)
  				mousePosition.y++;
        goto drawcursor;
				break;
 			case '3':
		    InvalidateRect(m_Wnd3,NULL,TRUE);
				break;
 			case VK_NUMPAD4:
			case '4':
				if(mousePosition.x>0)
  				mousePosition.x--;
        goto drawcursor;
				break;
			case VK_NUMPAD6:
			case '6':
      {  CURSOR myCursor;
      
				if(mousePosition.x<_width-1)
  				mousePosition.x++;
      
drawcursor:        
				hWnd=WindowFromPoint(mousePosition);
        if(hWnd) {
          myCursor=(CURSOR)SendMessage(hWnd,WM_SETCURSOR,0,0);
          if(!myCursor)
            myCursor=standardCursor;
          }
        else
          myCursor=standardCursor;
        DrawCursor(mousePosition.x,mousePosition.y,myCursor,0);
        
        if(hWnd) {
      	  i=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
          if(i != HTNOWHERE && i != HTCLIENT)
            SendMessage(hWnd,WM_NCMOUSEMOVE,i,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
          }
      }
				break;

			case VK_RETURN:		//lbuttondown
      {

is_click:        
				hWnd=WindowFromPoint(mousePosition);
       
        dclick=(timeGetTime()-oldclick)<TIME_DCLICK;
        printf("Click: %X(%u)\r\n",hWnd,dclick);

        if(hWnd) {
				  nchit=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
        printf("..NCHIT: %d\r\n",nchit);
          if(nchit == HTCLIENT)
     				SendMessage(hWnd,WM_LBUTTONDOWN,0,0);
          else if(nchit != HTNOWHERE)
     				SendMessage(hWnd,WM_NCLBUTTONDOWN,nchit,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));

          switch(nchit) {
            case HTTOPLEFT:
              break;
            case HTTOPRIGHT:
              break;
            case HTTOP:
              break;
            case HTLEFT:
              break;
            case HTBORDER:
              break;
            case HTRIGHT:
              break;
            case HTSYSMENU:
              //nclmousebuttondown (ht_sysmenu) e poi syscommand e quindi entermenuloop
              SendMessage(hWnd,WM_INITMENUPOPUP,0,MAKELONG(0,1));
              break;
            case HTCAPTION:
              if(dclick) {
                if(hWnd->maximized)
                  ShowWindow(hWnd,SW_SHOWNORMAL);
                else
                  ShowWindow(hWnd,SW_SHOWMAXIMIZED);
                }
              break;
            case HTCLOSE:
              SendMessage(hWnd,WM_CLOSE,0,0);
              break;
            case HTMAXBUTTON:
              if(hWnd->maximized)
                ShowWindow(hWnd,SW_SHOWNORMAL);
              else
                ShowWindow(hWnd,SW_SHOWMAXIMIZED);
              break;
            case HTMINBUTTON:
              ShowWindow(hWnd,SW_MINIMIZE /*SW_SHOWMINIMIZED*/);
              break;
            case HTMENU:
              if(hWnd->menu) {   // direi ovvio dato nchit...
                MENU *myMenu;
/*                getMenuFromPoint(hWnd,mousePosition,&myMenu);
                if(myMenu)
                  SendMessage(hWnd,WM_INITMENU,(WPARAM)myMenu,0);
                else {*/
                i=getMenuFromPoint(hWnd,mousePosition,&myMenu);
                if(myMenu)
                  SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)myMenu->menuItems[0].menu,
                          MAKELONG(getMenuPosition(myMenu,hWnd->menu).x,0));
                else if(i)
                  SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),0);
//                  }
                }
              break;
            case HTHSCROLL:
              SetScrollPos(hWnd,SB_HORZ,mousePosition.x-hWnd->nonClientArea.left,TRUE);//finire!
              break;
            case HTCLIENT:
              if(dclick) {
                if(hWnd->minimized)
                  ShowWindow(hWnd,SW_SHOWNORMAL);
                }
              if(activeMenu) {    // per ora...
                SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)activeMenu,MAKELONG(0,0));
                }

              break;
            case HTSIZE:
              break;
            case HTVSCROLL:
              SetScrollPos(hWnd,SB_VERT,mousePosition.y-hWnd->nonClientArea.top,TRUE);//finire!
              break;
            case HTBOTTOMLEFT:
              break;
            case HTBOTTOM:
              break;
            case HTBOTTOMRIGHT:
              break;
            case HTNOWHERE:
              break;
            }
          
          if(!hWnd->active) {
//            ShowWindow(hWnd,SW_SHOWNORMAL);
            SendMessage(hWnd,WM_MOUSEACTIVATE,(WPARAM)hWnd->parent,MAKEWORD(nchit, 0 /*id mouse msg...*/));
            }
          
          if(dclick) {
            WNDCLASS *wc;
            GetClassInfo(0,hWnd->class,&wc);
            if(wc->style & CS_DBLCLKS)
              SendMessage(hWnd,WM_LBUTTONDBLCLK,0 /* FLAG tasti */,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
            }
          // 
          // o forse deve gestirsi le cose la windows??
          
/*          if(i == HTCLIENT)
     				SendMessage(hWnd,WM_LBUTTONUP,0,0);
          else if(i != HTNOWHERE)
     				SendMessage(hWnd,WM_NCLBUTTONUP,0,0);*/
            
          oldclick=timeGetTime();
          
          }

      }
				break;

			}

//    putcUSBUSART(i);
    oldch=i;
    }
  
  
#else


  if(mouseClick) {
    CURSOR myCursor;
    POINT pt;
    pt.x=mousePosition.x; pt.y=mousePosition.y;
    
    hWnd=WindowFromPoint(pt);
    if(hWnd) {
      myCursor=(CURSOR)SendMessage(hWnd,WM_SETCURSOR,0,0);
      if(!myCursor)
        myCursor=standardCursor;
      }
    else
      myCursor=standardCursor;
    DrawCursor(mousePosition.x,mousePosition.y,myCursor,0);

    if(hWnd) {
      i=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
      if(i != HTNOWHERE && i != HTCLIENT)
        SendMessage(hWnd,WM_NCMOUSEMOVE,i,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
      }

    dclick=(timeGetTime()-oldclick)<TIME_DCLICK;
//    printf("Click: %X(%u)\r\n",hWnd,dclick);

    if(hWnd) {
      nchit=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
//    printf("..NCHIT: %d\r\n",nchit);
      if(nchit == HTCLIENT)
        SendMessage(hWnd,WM_LBUTTONDOWN,0,0);
      else if(nchit != HTNOWHERE)
        SendMessage(hWnd,WM_NCLBUTTONDOWN,nchit,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));

      switch(nchit) {
        case HTTOPLEFT:
          break;
        case HTTOPRIGHT:
          break;
        case HTTOP:
          break;
        case HTLEFT:
          break;
        case HTBORDER:
          break;
        case HTRIGHT:
          break;
        case HTSYSMENU:
          //nclmousebuttondown (ht_sysmenu) e poi syscommand e quindi entermenuloop
          SendMessage(hWnd,WM_INITMENUPOPUP,0,MAKELONG(0,1));
          break;
        case HTCAPTION:
          if(dclick) {
            if(hWnd->maximized)
              ShowWindow(hWnd,SW_SHOWNORMAL);
            else
              ShowWindow(hWnd,SW_SHOWMAXIMIZED);
            }
          break;
        case HTCLOSE:
          SendMessage(hWnd,WM_CLOSE,0,0);
          break;
        case HTMAXBUTTON:
          if(hWnd->maximized)
            ShowWindow(hWnd,SW_SHOWNORMAL);
          else
            ShowWindow(hWnd,SW_SHOWMAXIMIZED);
          break;
        case HTMINBUTTON:
          ShowWindow(hWnd,SW_MINIMIZE /*SW_SHOWMINIMIZED*/);
          break;
        case HTMENU:
          if(hWnd->menu) {   // direi ovvio dato nchit...
            MENU *myMenu;
            POINT pt;
            pt.x=mousePosition.x; pt.y=mousePosition.y;
/*                getMenuFromPoint(hWnd,mousePosition,&myMenu);
            if(myMenu)
              SendMessage(hWnd,WM_INITMENU,(WPARAM)myMenu,0);
            else {*/
            i=getMenuFromPoint(hWnd,pt,&myMenu);
            if(myMenu)
              SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)myMenu->menuItems[0].menu,
                      MAKELONG(getMenuPosition(myMenu,hWnd->menu).x,0));
            else if(i)
              SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),0);
//                  }
            }
          break;
        case HTHSCROLL:
          SetScrollPos(hWnd,SB_HORZ,mousePosition.x-hWnd->nonClientArea.left,TRUE);//finire!
          break;
        case HTCLIENT:
          if(dclick) {
            if(hWnd->minimized)
              ShowWindow(hWnd,SW_SHOWNORMAL);
            }
          if(activeMenu) {    // per ora...
            SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)activeMenu,MAKELONG(0,0));
            }

          break;
        case HTSIZE:
          break;
        case HTVSCROLL:
          SetScrollPos(hWnd,SB_VERT,mousePosition.y-hWnd->nonClientArea.top,TRUE);//finire!
          break;
        case HTBOTTOMLEFT:
          break;
        case HTBOTTOM:
          break;
        case HTBOTTOMRIGHT:
          break;
        case HTNOWHERE:
          break;
        }

      if(!hWnd->active) {
//            ShowWindow(hWnd,SW_SHOWNORMAL);
        SendMessage(hWnd,WM_MOUSEACTIVATE,(WPARAM)hWnd->parent,MAKEWORD(nchit, 0 /*id mouse msg...*/));
        }

      if(dclick) {
        WNDCLASS *wc;
        GetClassInfo(0,hWnd->class,&wc);
        if(wc->style & CS_DBLCLKS)
          SendMessage(hWnd,WM_LBUTTONDBLCLK,0 /* FLAG tasti */,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
        }
      // 
      // o forse deve gestirsi le cose la windows??

/*          if(i == HTCLIENT)
        SendMessage(hWnd,WM_LBUTTONUP,0,0);
      else if(i != HTNOWHERE)
        SendMessage(hWnd,WM_NCLBUTTONUP,0,0);*/

      oldclick=timeGetTime();

      }
    }
  
  POINT pt;

  if(!(eXtra & 1) && mouse.new) {
    CURSOR myCursor;

    setPowerMode(1);
    
    mousePosition.x+=mouse.x;
    if((int16_t)mousePosition.x<0)
      mousePosition.x=0;
    if(mousePosition.x>=Screen.cx-8  )//o size??
      mousePosition.x=Screen.cx-8;
    mousePosition.y+=mouse.y;
    if((int16_t)mousePosition.y<0)
      mousePosition.y=0;
    if(mousePosition.y>=Screen.cy-8  )//o size??
      mousePosition.y=Screen.cy-8;
    mouse.new=0;
    
drawcursor:        
    pt.x=mousePosition.x; pt.y=mousePosition.y;
    hWnd=WindowFromPoint(pt);
    if(hWnd) {
      myCursor=(CURSOR)SendMessage(hWnd,WM_SETCURSOR,0,0);
      if(!myCursor)
        myCursor=standardCursor;
      }
    else
      myCursor=standardCursor;
    DrawCursor(mousePosition.x,mousePosition.y,myCursor,0);

    if(hWnd) {
      i=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
      if(i != HTNOWHERE && i != HTCLIENT)
        SendMessage(hWnd,WM_NCMOUSEMOVE,i,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
      }
    
    if(mouse.buttons & 7)
      goto is_click;
    else
      goto fine;
    }

  if(keypress.new) {
    DWORD f;
    static BYTE oldch;
    
    setPowerMode(1);
    
    i=keypress.key;
    f=1;          // flag vari, mettere.. https://docs.microsoft.com/en-us/windows/win32/inputdev/wm-char
    f |= i << 16;
    if(i == oldch)
      f |= 0x40000000;    // boh sì :)
    if(keypress.modifier & 0b00000100)    // e ALT-gr??
      f |= 0x20000000;    // 
    
    hWnd=GetForegroundWindow();
    if(hWnd) {
      WPARAM j;
      switch(i) {
        case 0x94: // up, 
          j=VK_UP;
          break;
        case 0x93: // down
          j=VK_DOWN;
          break;
        case 0x92: // left
          j=VK_LEFT;
          break;
        case 0x91: // right
          j=VK_RIGHT;
          break;
        case 0x96: // pgup
          j=VK_PRIOR;
          break;
        case 0x95: // pgdn
          j=VK_NEXT;
          break;
        case 0x97: // home
          j=VK_HOME;
          break;
        case 0x98: // end
          j=VK_END;
          break;
        case 0x99:
          j=VK_INSERT;
          break;
        case 0x9a:
          j=VK_DELETE;
          break;
        case 0xB8:
          j=VK_SNAPSHOT;
          break;
        case 0x9f:
          j=VK_PAUSE;
          break;
        case 0xa:		// serve... :)
          j=VK_RETURN;
          break;
        default:
          j=i;
          break;
        }
      SendMessage(hWnd,WM_CHAR,j,f);
      }

		switch(i) {
			case 0xa9:      // F9 ingrandisci / riduci a icona ..
        if(hWnd) {
          if(keypress.modifier & 0b00000100) {
            if(hWnd->minimized)
              ShowWindow(hWnd,SW_SHOWNORMAL);
            else
              ShowWindow(hWnd,SW_SHOWMINIMIZED);
            }
          else {
            if(hWnd->maximized)
              ShowWindow(hWnd,SW_SHOWNORMAL);
            else
              ShowWindow(hWnd,SW_SHOWMAXIMIZED);
            }
          }
				break;
			case 0x9:   //TAB
        if(keypress.modifier & 0b00000100) {    // finire con prima o succ. pressione di ALT!
          static BYTE whichWindow=0;     // non è il massimo ma ok...
          HWND w2=rootWindows,w3=NULL;
          i=0;
          while(w2->next) {
            if(i==whichWindow) {
              w3=w2;
              whichWindow++;
              break;
              }
            w2=w2->next;
            i++;
            }
          if(!w3) {
            w3=rootWindows;
            whichWindow=0;
            }
          if(w3)
            ShowWindow(w3,SW_SHOWNORMAL);
          }
        if(keypress.modifier & 0b00000001) {    // MDI; finire con prima o succ. pressione di ALT!
          }
				break;
			case 0xaa:      //F10 menu
        hWnd=GetForegroundWindow();
        if(hWnd && hWnd->menu)
          SendMessage(hWnd,WM_INITMENU,(WPARAM)hWnd->menu,0);
				break;
      case 0xa4:     // ALT F4, chiude baracca!
        if(keypress.modifier & 0b00000100) {
          /*if(! )*/ SendMessage(HWND_BROADCAST,WM_QUERYENDSESSION,0,0)
            /*skippa*/ ;   // in effetti va chiesto una per una...
          SendMessage(HWND_BROADCAST,WM_ENDSESSION,0,0);   // 
          return WM_QUIT;   // boh :D e anche mandarlo a tutte SendMessage(HWND_BROADCAST,WM_QUIT,0,0);
          }
				break;
			case VK_SPACE:
        if(keypress.modifier & 0b00000100) {
          hWnd=GetForegroundWindow();
          if(hWnd)
            SendMessage(hWnd,WM_INITMENUPOPUP,0,MAKELONG(0,1));
          }
				break;
//      case VK_ESCAPE:     // gestito cmq con WM_CHAR!
/*        if(activeMenu) {
          hWnd=GetForegroundWindow();
          if(hWnd)
            SendMessage(hWnd,WM_UNINITMENUPOPUP,0,MAKELONG(0,1));
        }*/
//        break;
#if 0
			case 'm':   // TOGLIERE beh anche le altre :D
        if(IsIconic(m_Wnd))
          ShowWindow(m_Wnd,SW_SHOWNORMAL);
        else
          ShowWindow(m_Wnd,SW_SHOWMINIMIZED);
				break;
			case 'b':
        if(m_Wnd2) {
//          SendMessage(m_Wnd2,WM_CLOSE,0,0);
          KillTimer(m_Wnd2,1);
          DestroyWindow(m_Wnd2);
          m_Wnd2=NULL;
          }
        else {
          m_Wnd2=CreateWindow(MAKECLASS(MAKEFOURCC('M','B','A','S')),"MiniBASIC",
            WS_ACTIVE | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
            160,2+rand() % 8,159,170,
            NULL,NULL,(void*)1
            );
          SetTimer(m_Wnd2,1,3000,NULL);   // cambio :)
          }
				break;
			case 'c':
        if(m_Wnd3) {
          SendMessage(m_Wnd3,WM_CLOSE,0,0);
          m_Wnd3=NULL;
          }
        else {
          m_Wnd3=CreateWindow(MAKECLASS(WC_DEFAULTCLASS),NULL,WS_BORDER | WS_VISIBLE | WS_DISABLED,
            8+rand() % 8,210,20,18,
            NULL,(MENU *)NULL,(void*)3
            );
          }
				break;
			case 'i':
	      SendMessage(m_Wnd,WM_NOTIFY,0,CDN_TYPECHANGE);
				break;
			case 'x':
				MessageBox(m_Wnd  /*NULL*/,"attenzione!","PROVA",MB_OKCANCEL | MB_ICONEXCLAMATION);
				break;
      case 'w':
        printWindows(rootWindows);
        printf("%u,%u\n",mousePosition.x,mousePosition.y);
        break;

			case 'a':
//        printf(m_Wnd3->font.font);
//        printf(m_Wnd3->foreColor);
//        printf(m_Wnd3->font.attributes);
				break;
      case '3':
        InvalidateRect(m_Wnd3,NULL,TRUE);
        break;
#endif
        
      case 0x94: // up, 
        if(eXtra & 1) {
          mouse.buttons=0;
          if(mousePosition.y>0)
            mousePosition.y--;
          goto drawcursor;
          }
        break;
      case 0x93: // down
        if(eXtra & 1) {
          mouse.buttons=0;
          if(mousePosition.y<Screen.cy-8  )//o size??
            mousePosition.y++;
          goto drawcursor;
          }
        break;
      case 0x92: // left
        if(eXtra & 1) {
          mouse.buttons=0;
          if(mousePosition.x>0)
            mousePosition.x--;
          goto drawcursor;
        }
        break;
      case 0x91: // right
        if(eXtra & 1) {
          mouse.buttons=0;
          if(mousePosition.x<Screen.cx-8  )//o size??
            mousePosition.x++;
          goto drawcursor;
          }
				break;

			case 0x0a:		// simulo lbuttondown con RETURN
        if(eXtra & 1) {

is_click:
        
        // GESTIRE tasto dx ecc!
        pt.x=mousePosition.x; pt.y=mousePosition.y;
				hWnd=WindowFromPoint(pt);
       
        dclick=(timeGetTime()-oldclick)<TIME_DCLICK;
//        printf("Click: %X(%u)\r\n",hWnd,dclick);

        if(hWnd) {
				  nchit=SendMessage(hWnd,WM_NCHITTEST,0,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
//        printf("..NCHIT: %d\r\n",nchit);
          if(nchit == HTCLIENT)
     				SendMessage(hWnd,WM_LBUTTONDOWN,0,0);
          else if(nchit != HTNOWHERE)
     				SendMessage(hWnd,WM_NCLBUTTONDOWN,nchit,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));

          switch(nchit) {
            case HTTOPLEFT:
              break;
            case HTTOPRIGHT:
              break;
            case HTTOP:
              break;
            case HTLEFT:
              break;
            case HTBORDER:
              break;
            case HTRIGHT:
              break;
            case HTSYSMENU:
              //nclmousebuttondown (ht_sysmenu) e poi syscommand e quindi entermenuloop
              SendMessage(hWnd,WM_INITMENUPOPUP,0,MAKELONG(0,1));
              break;
            case HTCAPTION:
              if(dclick) {
                if(hWnd->maximized)
                  ShowWindow(hWnd,SW_SHOWNORMAL);
                else
                  ShowWindow(hWnd,SW_SHOWMAXIMIZED);
                }
              break;
            case HTCLOSE:
              SendMessage(hWnd,WM_CLOSE,0,0);
              break;
            case HTMAXBUTTON:
              if(hWnd->maximized)
                ShowWindow(hWnd,SW_SHOWNORMAL);
              else
                ShowWindow(hWnd,SW_SHOWMAXIMIZED);
              break;
            case HTMINBUTTON:
              ShowWindow(hWnd,SW_MINIMIZE /*SW_SHOWMINIMIZED*/);
              break;
            case HTMENU:
              if(hWnd->menu) {   // direi ovvio dato nchit...
                MENU *myMenu;
                POINT pt;
                pt.x=mousePosition.x; pt.y=mousePosition.y;
/*                getMenuFromPoint(hWnd,mousePosition,&myMenu);
                if(myMenu)
                  SendMessage(hWnd,WM_INITMENU,(WPARAM)myMenu,0);
                else {*/
                i=getMenuFromPoint(hWnd,pt,&myMenu);
                if(myMenu)
                  SendMessage(hWnd,WM_INITMENUPOPUP,(WPARAM)myMenu->menuItems[0].menu,
                    MAKELONG(getMenuPosition(myMenu,hWnd->menu).x,0));
                else if(i)
                  SendMessage(hWnd,WM_COMMAND,MAKELONG(i,0),0);
//                  }
                }
              break;
            case HTHSCROLL:
              SetScrollPos(hWnd,SB_HORZ,mousePosition.x-hWnd->nonClientArea.left,TRUE);//finire!
              break;
            case HTCLIENT:
              if(dclick) {
                if(hWnd->minimized)
                  ShowWindow(hWnd,SW_SHOWNORMAL);
                }
              if(activeMenu) {    // per ora...
                SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)activeMenu,MAKELONG(0,0));
                }

              break;
            case HTSIZE:
              break;
            case HTVSCROLL:
              SetScrollPos(hWnd,SB_VERT,mousePosition.y-hWnd->nonClientArea.top,TRUE);//finire!
              break;
            case HTBOTTOMLEFT:
              break;
            case HTBOTTOM:
              break;
            case HTBOTTOMRIGHT:
              break;
            case HTNOWHERE:
              break;
            }
          
          if(!hWnd->active) {
//            ShowWindow(hWnd,SW_SHOWNORMAL);
            SendMessage(hWnd,WM_MOUSEACTIVATE,(WPARAM)hWnd->parent,MAKEWORD(nchit, 0 /*id mouse msg...*/));
            }
          
          if(dclick) {
            WNDCLASS *wc;
            GetClassInfo(0,hWnd->class,&wc);
            if(wc->style & CS_DBLCLKS)
              SendMessage(hWnd,WM_LBUTTONDBLCLK,0 /* FLAG tasti */,(LPARAM)MAKELPARAM(mousePosition.x,mousePosition.y));
            }
          // 
          // o forse deve gestirsi le cose la windows??
          
/*          if(i == HTCLIENT)
     				SendMessage(hWnd,WM_LBUTTONUP,0,0);
          else if(i != HTNOWHERE)
     				SendMessage(hWnd,WM_NCLBUTTONUP,0,0);*/
            
          oldclick=timeGetTime();
          
          }

        }
				break;

      case 0xb9: // Power ecc
      case 0xba:
      case 0xbb:
        setPowerMode(keypress.key==0xb9 || keypress.key==0xbb);    // per ora così
        break;

			}

fine:
    KBClear();
    oldch=i;
    }

  return 1;
#endif
  

  }

static void printWindows(HWND root) {
  HWND myWnd,myWnd2;
  static BYTE level,i;

  level++;
  myWnd=root;
  while(myWnd) {
    for(i=0; i<level*2; i++)
      putchar(' ');
    printf("Wnd: %X:[%u,%u,%u,%u],%u,%u; %u %u %u %u\r",myWnd,myWnd->nonClientArea.left,myWnd->nonClientArea.top,
			myWnd->nonClientArea.right,myWnd->nonClientArea.bottom,GetWindowLong(myWnd,0),myWnd->zOrder,
            myWnd->maximized,myWnd->minimized,myWnd->topmost,myWnd->focus);
    printWindows(myWnd->children);
    myWnd=myWnd->next;
    }
  level--;
  }

// ----------------------------------------------------------------------------
static THREAD *gRunningThread;
static BOOL addThread(THREAD *thread);
static BOOL removeThread(THREAD *thread);
THREAD *BeginThread(void *context) {
  THREAD *thread;
  
	ATOMIC_START();
  thread=malloc(sizeof(THREAD));
  thread->next=NULL;
  thread->context=context;
  thread->priority=THREAD_PTY_MIDDLE;
  thread->sleepCount=0;
  addThread(thread);
	ATOMIC_END();
  return thread;
  }
BOOL EndThread(THREAD *threadID) {
  
	ATOMIC_START();
  removeThread(threadID);
  free(threadID);
	ATOMIC_END();
  }

BOOL KillThread(THREAD *threadID) {
  
	ATOMIC_START();
	//DEBUG_PRINTF("Finishing Thread with id: %d", gRunningThread);
	threadID->state = THREAD_DONE;
	_yield();
	ATOMIC_END();
  }

void SuspendThread(THREAD *threadID) {
  
	threadID->state = THREAD_BLOCKED;
	_yield();
  }

void ResumeThread(THREAD *threadID) {
  
	threadID->state = THREAD_READY;
  }

static void updateThreads() {
  THREAD *myThread;
  
  myThread=rootThreads;
  while(myThread) {
		if(myThread->state == THREAD_SLEEPING) {
			uint32_t sleep = myThread->sleepCount;
			myThread->sleepCount = sleep >= TIMESLICE_MS ? sleep - TIMESLICE_MS : 0;
			if(myThread->sleepCount == 0) {
				myThread->state = THREAD_READY;
        }
      }
    myThread=myThread->next;
    }
  }

THREAD *_yield(void) {
  THREAD *myThread;
  
	updateThreads();

  myThread=rootThreads;
  while(myThread && myThread->state != THREAD_READY) {
		if(myThread == gRunningThread) {
			break;
      }
    myThread=myThread->next;
    }

	return myThread;
  
/*
  setjmp(a->env); 
  if(a->next)
    longjmp(a->next->env,0);    // salto a quello che segue...
  else {
    if(rootThreads->next)       //ma se ce n'è solo uno, NON salto!
      longjmp(rootThreads->env,0);
    }
  // http://www.cplusplus.com/reference/csetjmp/setjmp/
  */
  }

static void scheduler_runNextThread() {
  
	ATOMIC_START();
	THREAD *nextThread = _yield /*getNextThread*/();

	switch(nextThread->state) {
    case THREAD_RUNNING:
      break; // already running
    case THREAD_READY:
      if(setjmp(gRunningThread->context) == 0) { // jump out if longjmp called with 1
        //	saved current execution state
        if(gRunningThread->state == THREAD_RUNNING) {
          gRunningThread->state = THREAD_READY;
          }
        gRunningThread = nextThread;
        gRunningThread->state = THREAD_RUNNING;
        longjmp(gRunningThread->context, 1); // load context of next thread
        }
      break;
    default:
      break; // no thread to run --> sleep no atomic
  	}
	ATOMIC_END();
  }

void scheduler_sleep(uint32_t duration_ms) {
  
	ATOMIC_START();
	gRunningThread->sleepCount = duration_ms;
	gRunningThread->state = THREAD_SLEEPING;
	ATOMIC_END();
	_yield();
  }

DWORD __attribute__((weak)) timeGetTime(void) {
  
  ClrWdt();
  return TickGetMilliseconds();
//  return millis;    // con USB va a 1mS... 
  }

static BOOL addThread(THREAD *thread) {
  THREAD *myThread;
  
  if(!rootThreads)
    rootThreads=thread;
  else {
    myThread=rootThreads;
    while(myThread && myThread->next) {
      myThread=myThread->next;
      }
    myThread->next=thread;
    }
  return 1;
  }
static BOOL removeThread(THREAD *thread) {
  THREAD *myThread,*myThread2;

  if(thread==rootThreads) {
    rootThreads=thread->next;
	  return 1;
		}

  myThread=rootThreads;
  while(myThread != thread) {
    myThread2=myThread;
    myThread=myThread->next;
		}
	if(myThread2) {
	  myThread2->next=myThread->next;
		myThread=myThread2->next;
	  return 1;
		}

  return 0;
  }


void _swap(UGRAPH_COORD_T *a, UGRAPH_COORD_T *b) {
	UGRAPH_COORD_T t = *a; 
	*a = *b; 
	*b = t; 
	}

BOOL PtInRect(const RECT *lprc,POINT pt) {
  
  if(pt.x>=lprc->left && pt.x<lprc->right &&
          pt.y>=lprc->top && pt.y<lprc->bottom)
    return TRUE;
  else
    return FALSE;
  }

POINT MAKEPOINT(UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  POINT pt;
  
  pt.x=x; pt.y=y;
  return pt;
  }

BOOL IsRectEmpty(const RECT *lprc) {
  
  if(lprc->bottom<=lprc->top || lprc->right<=lprc->left)
    return 1;
  else
    return 0;
  }

void setTextSize(BYTE n) {
  
  textsize=n;
  }


static int getParmN(const char *buf,const char *parm,int *val) {
	char *p;

	if((p=strchr(buf,'='))) {
		*p=0;
		if(!stricmp(buf,parm)) {
			if(val)
				*val=atoi(p+1);
			*p='=';
			return 1;
			}
		else			
			*p='=';
		}
	return 0;
	}

static int getParmS(const char *buf,const char *parm,char *val) {
	char *p;

	if((p=strchr(buf,'='))) {
		*p=0;
		if(!stricmp(buf,parm)) {
			if(val)
				strcpy(val,p+1);
			*p='=';
			return 1;
			}
		else			
			*p='=';
		}
	return 0;
	}

static BYTE getProfileSection(SUPERFILE *f,const char *section) {
  char buffer[32],*p;
  
  while(SuperFileGets(f,buffer,31)) {
    if(buffer[0]='[') {
      if(p=strchr(buffer+1,']')) {
        *p=0;
        if(!stricmp(buffer+1,section))
          return 1;
        }
      }
    }
  return 0;
  }

BYTE GetProfileString(const char *file,const char *section,const char *key,char *val,char *defval) {
  SUPERFILE f;
  char buffer[64];
  BYTE n=0;
  VARIABLESTRING *vars;
  
  vars=findEnvVariable("TEMP");
  if(vars)
    f.drive=*vars->sval;   
  else
    f.drive=currDrive;
  if(SuperFileOpen(&f,file,'r')) {
    if(getProfileSection(&f,section)) {
      SuperFileSeek(&f,0,SEEK_SET);
      while(SuperFileGets(&f,buffer,63)) {
        if(getParmS(buffer,key,val)) {
          n=1;
          goto fine;
          }
        }
      }
    if(defval)
      strcpy(val,defval);
fine:  
    SuperFileClose(&f);
    }
  
  return n;
  }

int GetProfileInt(const char *file,const char *section,const char *key,int defval) {
  SUPERFILE f;
  char buffer[32];
  int n;
  VARIABLESTRING *vars;
  
  vars=findEnvVariable("TEMP");
  if(vars)
    f.drive=*vars->sval;   
  else
    f.drive=currDrive;
  
  if(SuperFileOpen(&f,file,'r')) {
    if(getProfileSection(&f,section)) {
      SuperFileSeek(&f,0,SEEK_SET);
      while(SuperFileGets(&f,buffer,31)) {
        if(getParmN(buffer,key,&n)) {
          defval=n; // :)
          goto fine;
          }
        }
      }
fine:    
    SuperFileClose(&f);
    }

  return defval;
  }

BYTE GetTempPath(BYTE nBufferLength,char *lpBuffer) {
  VARIABLESTRING *vars;
  
  vars=findEnvVariable("TEMP");
  if(vars)
    strncpy(lpBuffer,vars->sval,nBufferLength);
  else
    *lpBuffer=0;
  lpBuffer[nBufferLength-1]=0;
  return strlen(lpBuffer);
  }
UINT16 GetTempFileName(char *lpPathName,const char *lpPrefixString,
  UINT16 uUnique,char *lpTempFileName) {
  char buffer[32];
  UINT16 u=uUnique; 
  
  if(lpPathName) {      // mia variante!
    strncpy(buffer,lpPathName,31);
    buffer[31]=0;
    }
  else
    GetTempPath(31,buffer);
  if(!uUnique)
    uUnique=rand();
  sprintf(lpTempFileName,"%s\\%3s_%04u.tmp",buffer,lpPrefixString,uUnique);
  if(!u) //If uUnique is zero, GetTempFileName creates an empty file and closes it. If uUnique is not zero, you must create the file yourself
    ;
  return uUnique;
  }
DWORD GetLogicalDrives(void) {
  DWORD n=0;
  
  if(SDcardOK) {
    n |= 1 << 0;
    }
  if(FDOK) {
    n |= 1 << 1;
    }
  if(HDOK) {
    n |= 1 << 2;
    }
//  n |= 1 << 3;
#if defined(USA_USB_HOST_MSD)
  if(USBmemOK) {
    n |= 1 << 4;
#endif
#ifdef USA_RAM_DISK 
  if(RAMdiscArea) {
    n |= 1 << 17;
    }
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
  if(1   ) {
    n |= 1 << 25;
    }
#endif
    }
  return n;
  }
BOOL GetVolumeInformation(const char *lpRootPathName,char *lpVolumeNameBuffer,
  DWORD nVolumeNameSize, DWORD *lpVolumeSerialNumber,
  DWORD *lpMaximumComponentLength,DWORD *lpFileSystemFlags,
  char *lpFileSystemNameBuffer,BYTE nFileSystemNameSize) {
  
  
  switch(*lpRootPathName) {
    case 'A':
      if(SDcardOK) {
        }
      break;
    case 'B':
      if(FDOK) {
        }
      break;
    case 'C':
//#warning messo MOUNT in C: ecc
      if(HDOK) {
        }
      break;
    case 'D':
      break;
#if defined(USA_USB_HOST_MSD)
    case 'E':
      if(USBmemOK) {
        }
      break;
#endif
#ifdef USA_RAM_DISK 
    case DEVICE_RAMDISK:
      if(RAMdiscArea) {
        }
      break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
    case DEVICE_NETWORK:
      if(1   ) {
        }
      break;
#endif
    }
  
  }


void DrawWindow(UGRAPH_COORD_T x1,UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,UGRAPH_COORD_T y2,GFX_COLOR f,GFX_COLOR b) {
  BYTE parms[18];
  
  parms[0]=1;   // cmd
  parms[1]=LOBYTE(x1);
  parms[2]=HIBYTE(x1);
  parms[3]=LOBYTE(y1);
  parms[4]=HIBYTE(y1);
  parms[5]=LOBYTE(x2);
  parms[6]=HIBYTE(x2);
  parms[7]=LOBYTE(y2);
  parms[8]=HIBYTE(y2);
  parms[9]=LOBYTE(f);
  parms[10]=HIBYTE(f);
  parms[11]=LOBYTE(b);
  parms[12]=HIBYTE(b);
  
  CNNEFbits.CNNEF1=0;
  WritePMPs(VIDEO_CARD,BIOS_VIDEO_WINDOWS,parms,17);
  while(!m_VACK)
    ClrWdt();
  __delay_ns(VIDEO_WAIT);
  CNNEFbits.CNNEF1=1;
  
  }

void MovePointer(UGRAPH_COORD_T x,UGRAPH_COORD_T y) {
  BYTE parms[18];
  
  parms[0]=10;   // cmd
  parms[1]=LOBYTE(x);
  parms[2]=HIBYTE(x);
  parms[3]=LOBYTE(y);
  parms[4]=HIBYTE(y);
  
  CNNEFbits.CNNEF1=0;
  WritePMPs(VIDEO_CARD,BIOS_VIDEO_WINDOWS,parms,17);
  while(!m_VACK)
    ClrWdt();
  __delay_ns(VIDEO_WAIT);
  CNNEFbits.CNNEF1=1;
  
  }

