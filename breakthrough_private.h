/* 
 * File:   breakthrough_private.h
 * Author: dario
 *
 * Created on 1 marzo 2023, 22.09
 */

#ifndef BREAKTHROUGH_PRIVATE_H
#define	BREAKTHROUGH_PRIVATE_H

#ifdef	__cplusplus
extern "C" {
#endif

#define BREAKTHROUGH_COPYRIGHT_STRING "Breakthrough v2.1.1 - build "__DATE__
// provare, più compatta https://stackoverflow.com/questions/23032002/c-c-how-to-get-integer-unix-timestamp-of-build-time-not-string

extern const char *profileFile,*profileFile2;
extern const char *OPZIONI_STR,*DESKTOP_STR,*BOOT_STR,*WINDOWS_STR;

void drawCharWindow(HDC hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y, unsigned char c);
BOOL drawPixelWindow(HDC hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y);
BOOL drawPixelWindowColor(HDC hDC,UGRAPH_COORD_T x, UGRAPH_COORD_T y,GFX_COLOR c);
void drawLineWindow(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
void drawLineWindowColor(HDC hDC,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
void drawHorizLineWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T x2);
void drawHorizLineWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T x2,GFX_COLOR c);
void drawVertLineWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2);
void drawVertLineWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,UGRAPH_COORD_T y2,GFX_COLOR c);
void drawRectangleWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
#define drawRectangleWindowRect(hDC,rc) drawRectangleWindow(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom)
void drawRectangleWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
#define drawRectangleWindowColorRect(hDC,rc) drawRectangleWindowColor(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom)
void fillRectangleWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
#define fillRectangleWindowRect(hDC,rc) fillRectangleWindow(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom)
void fillRectangleWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
#define fillRectangleWindowColorRect(hDC,rc,c) fillRectangleWindowColor(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom,c)
void drawEllipseWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
void fillEllipseWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
void drawHorizArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE width,BYTE dir);
void drawVertArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE height,BYTE dir);
BOOL nonClientPaint(HWND hWnd,const RECT *rc,BYTE mode);
BOOL clientPaint(HWND hWnd,const RECT *rc);
uint16_t getMenuPopupFromPoint(HMENU menu,RECT *rc,POINT pt,HMENU*inMenu);
int drawCaret(HWND hWnd,UGRAPH_COORD_T x1, UGRAPH_COORD_T y1,const CURSOR caret,BYTE bShow);

int getFileInfo(const char *file,ICON *icon,BYTE size);

extern HWND m_WndClock,m_WndCalendar,m_WndCalc,m_WndFileManager[2],m_WndTaskManager,m_WndControlPanel,
        m_WndBasic /* in effetti se ne potrebbero aprire più d'una...*/,
        m_WndSurf,m_WndViewer,m_WndPlayer,m_WndNotepad;

extern DLGTEMPLATE fileChooserDlg;

extern GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;
extern const unsigned char font5x7[],font3x5[];
extern const GFXfont FreeSans9pt7b[],FreeSans12pt7b[],FreeSerif9pt7b[],FreeSerif12pt7b[],
	FreeSans18pt7b[],FreeSans24pt7b[],FreeSerif18pt7b[],FreeSerif24pt7b[];

extern HMENU activeMenu,activeMenuParent;
extern BYTE activeMenuCntX,activeMenuCntY;
extern HWND activeMenuWnd;
extern RECT activeMenuRect;

extern HWND rootWindows,taskbarWindow;
extern HWND desktopWindow;

extern BYTE inScreenSaver,quitSignal;
extern BYTE volumeAudio;
extern GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;
extern POINTS mousePosition,caretPosition;

extern int8_t caretTime;

extern const MENU menuStart,menuStart2;


extern const GFX_COLOR standardCaret[];

extern const GFX_COLOR standardIcon[];
extern const GFX_COLOR /*BYTE */standardCursor[],standardCursorSm[];
extern const GFX_COLOR hourglassCursorSm[];
extern const GFX_COLOR crossCursorSm[];
extern const GFX_COLOR handCursorSm[];
extern const GFX_COLOR halfSquareCursor[],halfSquareCursorSm[];
extern const GFX_COLOR standardCaret[];
extern const GFX_COLOR redBallIcon[];
extern const GFX_COLOR recyclerIcon[];
extern const GFX_COLOR folderIcon8[],fileIcon8[],folderIcon[],fileIcon[];
extern const GFX_COLOR windowIcon[],dosIcon[],zipIcon[];
extern const GFX_COLOR minibasicIcon[];
extern const GFX_COLOR diskIcon8[],diskIcon[],printerIcon[];
extern const GFX_COLOR deviceIcon[];
extern const GFX_COLOR mouseIcon[],keyboardIcon[];
extern const GFX_COLOR audioIcon[],audioIcon8[],videoIcon[];
extern const GFX_COLOR networkIcon[];
extern const GFX_COLOR surfIcon[],surfWaitIcon[],surfImage[];
extern const GFX_COLOR speakerIcon[],shieldIcon[],batteryIcon[];

extern const MENU systemMenu,systemMenu2;
extern const MENU explorerMenu;
extern const MENU menuStart;
extern const MENU cmdShellMenu;
extern const MENU menuControlPanel;

extern TRACKMOUSEEVENT trackMouseEvent;


#ifdef	__cplusplus
}
#endif

#endif	/* BREAKTHROUGH_PRIVATE_H */

