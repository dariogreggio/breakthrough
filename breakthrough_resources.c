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


extern volatile unsigned long now;
extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern DWORD extRAMtot;
extern struct KEYPRESS keypress;
extern struct MOUSE mouse;


const GFX_COLOR standardIcon[]={  // una specie di mela
  BLACK,BLACK,BLACK,GRAY064,LIGHTGREEN,BRIGHTGREEN,GRAY064,BLACK,
  BLACK,BLACK,GRAY064,LIGHTGREEN,GRAY064,BLACK,BLACK,BLACK,
  BLACK,GRAY064,LIGHTRED,LIGHTRED,LIGHTRED,LIGHTRED,GRAY064,BLACK,
  BLACK,GRAY064,LIGHTRED,LIGHTRED,LIGHTRED,LIGHTRED,GRAY064,BLACK,
  BLACK,LIGHTRED,LIGHTRED,BRIGHTRED,BRIGHTRED,LIGHTRED,LIGHTRED,BLACK,
  BLACK,LIGHTRED,LIGHTRED,LIGHTRED,LIGHTRED,LIGHTRED,LIGHTRED,BLACK,
  BLACK,GRAY064,LIGHTRED,LIGHTRED,LIGHTRED,LIGHTRED,GRAY064,BLACK,
  BLACK,BLACK,GRAY064,LIGHTRED,LIGHTRED,GRAY064,BLACK,BLACK,
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
const GFX_COLOR hourglassCursorSm[]={
  CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP,
  CUR_TRNSP,CUR_TRNSP,CUR_WHT,CUR_WHT,CUR_WHT,CUR_WHT,CUR_TRNSP,CUR_TRNSP
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
  {"&Ripristina", SC_RESTORE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"&Sposta", SC_MOVE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"R&idimensiona", SC_SIZE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Ri&duci a icona", SC_MINIMIZE, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
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
  {"&Visualizza", 0, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&explorerMenu2},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
const MENU menuStart={{    // questo � un popup!
  {"File manager", 1, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Pannello ctrl", 2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Task list", 3, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Calcolatrice", 4, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Orologio", 5, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Esegui...", 6, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Fine sessione", 7, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
const MENU cmdShellMenu1={{
  {"Esegui...", 1, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, MF_SEPARATOR, NULL},
  {"Esci", 2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},{0}
  },
  {
  {'E',0b00000001}
  }
  };
const MENU cmdShellMenu2={{
  {"Cut", 16+1, MF_GRAYED | MF_STRING | MF_CHECKED, NULL},
  {"Copy", 16+2, MF_GRAYED | MF_STRING | MF_CHECKED, NULL},
  {"Paste", 16+3, MF_GRAYED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {'X',0b00000001},{'C',0b00000001},{'V',0b00000001}
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

