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

#define BREAKTHROUGH_COPYRIGHT_STRING "Breakthrough v2.2.4 - build "__DATE__
// provare, più compatta https://stackoverflow.com/questions/23032002/c-c-how-to-get-integer-unix-timestamp-of-build-time-not-string
// unire con BREAKTHROUGH_VERSION_H,BREAKTHROUGH_VERSION_L    

extern const char *profileFile,*profileFile2;
extern const char *OPZIONI_STR,*DESKTOP_STR,*BOOT_STR,*WINDOWS_STR;

void drawCharWindow(HDC hDC,GRAPH_COORD_T x, GRAPH_COORD_T y, unsigned char c);
void drawPixelWindow(HDC hDC,GRAPH_COORD_T x, GRAPH_COORD_T y);
void drawPixelWindowColor(HDC hDC,GRAPH_COORD_T x, GRAPH_COORD_T y,GFX_COLOR c);
void drawLineWindow(HDC hDC,GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2);
void drawLineWindowColor(HDC hDC,GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T x2, GRAPH_COORD_T y2,GFX_COLOR c);
void drawHorizLineWindow(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T x2);
void drawHorizLineWindowColor(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T x2,GFX_COLOR c);
void drawVertLineWindow(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T y2);
void drawVertLineWindowColor(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T y2,GFX_COLOR c);
void drawRectangleWindow(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2);
#define drawRectangleWindowRect(hDC,rc) drawRectangleWindow(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom)
void drawRectangleWindowColor(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2,GFX_COLOR c);
#define drawRectangleWindowColorRect(hDC,rc) drawRectangleWindowColor(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom)
void fillRectangleWindow(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2);
#define fillRectangleWindowRect(hDC,rc) fillRectangleWindow(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom)
void fillRectangleWindowColor(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2,GFX_COLOR c);
#define fillRectangleWindowColorRect(hDC,rc,c) fillRectangleWindowColor(hDC,(rc)->left,(rc)->top,(rc)->right,(rc)->bottom,c)
void drawEllipseWindow(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2);
void fillEllipseWindow(HDC hDC, GRAPH_COORD_T x1, GRAPH_COORD_T y1, GRAPH_COORD_T x2, GRAPH_COORD_T y2);
void drawHorizArrowWindow(HDC hDC,GRAPH_COORD_T x,GRAPH_COORD_T y,BYTE size,BYTE width,BYTE dir);
void drawVertArrowWindow(HDC hDC,GRAPH_COORD_T x,GRAPH_COORD_T y,BYTE size,BYTE height,BYTE dir);
void nonClientPaint(HWND hWnd,const RECT *rc,BYTE mode,BYTE bk);
void clientPaint(HWND hWnd,const RECT *rc);
uint16_t getMenuPopupFromPoint(HMENU menu,RECT *rc,POINT pt,HMENU*inMenu);
int drawCaret(HWND hWnd,GRAPH_COORD_T x1, GRAPH_COORD_T y1,const CURSOR caret,BYTE bShow);

int getFileInfo(const char *file,ICON *icon,BYTE size);

extern HWND m_WndClock,m_WndCalendar,m_WndCalc,m_WndFileManager[2],m_WndTaskManager,m_WndControlPanel,
        m_WndBasic[2],m_WndDOS,
        m_WndSurf,m_WndViewer,m_WndPlayer,m_WndNotepad;

extern int WinMainDesktop(HINSTANCE,HINSTANCE,LPSTR,int);
extern int WinMainTaskbar(HINSTANCE,HINSTANCE,LPSTR,int);
extern int WinMainFilemanager(HINSTANCE,HINSTANCE,LPSTR,int);
extern int WinMainTaskman(HINSTANCE,HINSTANCE,LPSTR,int);
extern int WinMainControl(HINSTANCE,HINSTANCE,LPSTR,int);
extern int WinMainDOS(HINSTANCE,HINSTANCE,LPSTR,int);

extern LRESULT clockWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT calendarWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT calcWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT surfWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT viewerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT notepadWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT playerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);

extern DLGTEMPLATE fileChooserDlg;

extern GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;
extern const unsigned char font5x7[],font3x5[];
extern const GFXfont FreeSans9pt7b[],FreeSans12pt7b[],FreeSerif9pt7b[],FreeSerif12pt7b[],
	FreeSans18pt7b[],FreeSans24pt7b[],FreeSerif18pt7b[],FreeSerif24pt7b[];

extern HMENU activeMenu,activeMenuParent;
extern uint8_t activeMenuCntX,activeMenuCntY, inTrackMenu;
extern HWND activeMenuWnd;
extern RECT activeMenuRect;

extern HWND rootWindows,taskbarWindow;
extern HWND desktopWindow;
extern CURSOR mouseCursor;

extern uint8_t inScreenSaver,quitSignal;
extern uint8_t screenSaverRefresh;
extern uint8_t volumeAudio;
extern GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;
extern POINTS mousePosition,caretPosition;

extern int8_t caretTime;

extern const MENU menuStart,menuStart2;


extern const GFX_COLOR standardCaret[];

extern const GFX_COLOR standardIcon[];
extern const GFX_COLOR /*BYTE */standardCursor[],standardCursorSm[];
extern const GFX_COLOR hourglassCursorSm[],watchCursorSm[];
extern const GFX_COLOR crossCursorSm[];
extern const GFX_COLOR handCursorSm[];
extern const GFX_COLOR halfSquareCursor[],halfSquareCursorSm[];
extern const GFX_COLOR standardCaret[];
extern const GFX_COLOR redBallIcon8[],redBallIcon[];
extern const GFX_COLOR recyclerIcon[];
extern const GFX_COLOR folderIcon8[],fileIcon8[],folderIcon[],fileIcon[];
extern const GFX_COLOR windowIcon[],windowIcon8[],dosIcon[],dosIcon8[],zipIcon[];
extern const GFX_COLOR minibasicIcon[],minibasicIcon8[];
extern const GFX_COLOR diskIcon8[],diskIcon[],printerIcon[];
extern const GFX_COLOR deviceIcon[];
extern const GFX_COLOR mouseIcon[],keyboardIcon[];
extern const GFX_COLOR audioIcon[],audioIcon8[],videoIcon[];
extern const GFX_COLOR networkIcon[],bombIcon[];
extern const GFX_COLOR surfIcon[],surfWaitIcon[],surfImage[];
extern const GFX_COLOR speakerIcon[],shieldIcon[],batteryIcon[];

extern const MENU systemMenu,systemMenu2;
extern const MENU explorerMenu;
extern const MENU menuStart;
extern const MENU cmdShellMenu;
extern const MENU menuControlPanel;
extern const MENU clockMenu;

extern TRACKMOUSEEVENT trackMouseEvent;


#ifdef	__cplusplus
}
#endif

#endif	/* BREAKTHROUGH_PRIVATE_H */

