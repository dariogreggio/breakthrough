/*
 * BREAKTHROUGH 2.2
 * (C) G.Dar 1987-2026 :) !
 * (portions inspired by Windows, since 1990)
 * 
 * applicazioni: BASIC, OROLOGIO, CALCOLATRICE, FILE MANAGER, CALENDARIO, SURF!, NOTEPAD
 */
 
#include <xc.h>
//#include "compiler.h"
#include "pc_pic_cpu.h"
#include <stdio.h>
#include <string.h>
#include <time.h>


#include "breakthrough.h"
#include "breakthrough_private.h"
#include "fat_sd/FSIO.h"
#if defined(USA_USB_HOST_MSD)
#include "../harmony_pic32mz/usb_host_msd.h"
#include "../harmony_pic32mz/usb_host_scsi.h"
#endif
#include "fat_ide/idefsio.h"
#include "superfile.h"

#include "picojpeg.h"
#include "minibasic/minibasic.h"
#include "kommissarRexx/kommissarRexx.h"

extern enum FILE_DEVICE m_stdout,m_stdin,m_stderr;
extern const char *ASTERISKS;
extern char prompt[16];
extern const char *profileFile;
extern signed char currDrive;

extern struct SOCKET_DATA socketData[11 /*v. minibasic */];
extern uint8_t internetBuffer[256];
extern BYTE rxBuffer[1536];
extern WORD internetBufferLen;

extern volatile unsigned long now;
extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern struct KEYPRESS keypress;
extern struct MOUSE mouse;
extern BYTE audio_played;
extern BYTE specialKeysStatus;
extern BYTE _bpp;

const char *stristr(const char *,const char *);
uint8_t *memstr(const uint8_t *, const char *, int);
char *skipSpaces(const char *);
#define skipSurfSpaces(s) skipSpaces(s)
extern const char demoscript[],demoscript2[],demoscript3[],demoscript4[];

extern LRESULT minibasicWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern const GFX_COLOR standardCursorSm[],hourglassCursorSm[];
extern const MENU minibasicMenu;


ATOM MyRegisterClassMinibasic(HINSTANCE hInstance) {
  WNDCLASS *miniBasicWC;
  miniBasicWC=malloc(sizeof(WNDCLASS));
  miniBasicWC->class=MAKECLASS(MAKEFOURCC('M','B','A','S'));
  miniBasicWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
  miniBasicWC->icon=minibasicIcon8;
  miniBasicWC->cursor=standardCursorSm;
  miniBasicWC->cbClsExtra=0;
  miniBasicWC->cbWndExtra=4+sizeof(MINIBASIC)+128+128/*commandLine*/;
  miniBasicWC->lpfnWndProc=(WINDOWPROC *)minibasicWndProc;
  miniBasicWC->hbrBackground=CreateSolidBrush(BLACK);
  miniBasicWC->menu=(MENU*)&minibasicMenu;
  if(!RegisterClass(miniBasicWC)) {
    free(miniBasicWC);
    return FALSE;
    }
  return (ATOM)miniBasicWC;
  }
BOOL InitInstanceMinibasic(HINSTANCE hInstance,int nCmdShow,LPSTR lpCmdLine) {
    
  m_WndBasic[0]=CreateWindow(MAKECLASS(MAKEFOURCC('M','B','A','S')),"MiniBASIC",WS_BORDER | WS_CAPTION | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
    CW_USEDEFAULT,CW_USEDEFAULT,199,159,
    NULL,(MENU *)NULL,(void*)lpCmdLine
    );
  if(!m_WndBasic)
    return FALSE;
  ShowWindow(m_WndBasic[0],nCmdShow);
//	UpdateWindow(m_WndBasic);
  return TRUE;
  }
THREADPROC int WinMainMinibasic(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)
    return FALSE;     // bah per ora!
  MyRegisterClassMinibasic(hInstance);
  if(!InitInstanceMinibasic(hInstance, nCmdShow,lpCmdLine))
		return FALSE;

  msg.hWnd=m_WndBasic[0];    // PER ORA COSI'! ma non viene sovrascritto sotto??? v. al posto di NULL in GetMessage
  MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(m_WndBasic[0],4);
  PostMessage(m_WndBasic[0],WM_USER,0,0);   // per messg copyright!
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);
// OCCHIO!! le variabili locali NON sono garantite nei thread (setjmp/longjmp)
//    GET_MESSAGE_LOOP(hInstance,msg);    QUA serve diverso :)
  for(;;) {
    Yield();
    minstance=(MINIBASIC*)GET_WINDOW_OFFSET(m_WndBasic[0],4);
  //       #warning POTREBBE NON ESSERE VALIDA dopo yield... o usare volatile
    if(minstance->run_mode) {   
      bRet=basicExecuteLine(minstance,m_WndBasic);
      if(bRet<=0)
        minstance->run_mode=0;
      }
    bRet=PeekMessage(&msg,m_WndBasic[0],0,0,PM_REMOVE);
    if(!bRet)
      continue;
    if(m_WndBasic[0]->internalState==MSGF_DIALOGBOX) {
      if(msg.message==WM_TIMER || msg.message==WM_PAINT)
        goto _get_message_loop_ok;
      if((msg.message>=WM_KEYFIRST && msg.message<=WM_KEYLAST) || (msg.message>=WM_LBUTTONDOWN/*WM_MOUSEFIRST*/ && msg.message<=WM_MOUSELAST))\
        MessageBeep(MB_ICONASTERISK);
      if(!(m_WndBasic[0]->style & DS_NOIDLEMSG))
        /*mettere divisore...PostMessage(hwnd,WM_ENTERIDLE,MSGF_DIALOGBOX,(LPARAM)hwnd)*/;
      continue;\
      }\
    if(msg.message==WM_QUIT) {
        m_WndBasic[0]=NULL;\
        ExitThread(msg.wParam);
      }
_get_message_loop_ok:
	  if(!TranslateAccelerator(msg.hWnd,hAccelTable,&msg)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
      }
    }

//	while(GetMessage(&msg, hInstance/*NULL*/, 0, 0)) {
    
  ExitThread(msg.wParam);
//	return msg.wParam;
  }


const BYTE slant=20;
static void drawSegmentA(HDC hDC,POINTS coords[],BYTE overlap) {

  MoveTo(hDC,coords[0].x+overlap,coords[0].y);
  LineTo(hDC,coords[1].x-overlap,coords[0].y);
	}
static void drawSegmentB(HDC hDC,POINTS coords[],int overlap) {

  MoveTo(hDC,coords[1].x,coords[1].y+overlap);
  LineTo(hDC,coords[3].x,coords[3].y-overlap);
	}
static void drawSegmentC(HDC hDC,POINTS coords[],int overlap) {

  MoveTo(hDC,coords[3].x,coords[3].y+overlap);
  LineTo(hDC,coords[5].x,coords[5].y-overlap);
	}
static void drawSegmentD(HDC hDC,POINTS coords[],int overlap) {

  MoveTo(hDC,coords[4].x+overlap,coords[4].y);
  LineTo(hDC,coords[5].x-overlap,coords[5].y);
	}
static void drawSegmentE(HDC hDC,POINTS coords[],int overlap) {

  MoveTo(hDC,coords[2].x,coords[2].y+overlap);
  LineTo(hDC,coords[4].x,coords[4].y-overlap);
	}
static void drawSegmentF(HDC hDC,POINTS coords[],int overlap) {

  MoveTo(hDC,coords[0].x,coords[0].y+overlap);
  LineTo(hDC,coords[2].x,coords[2].y-overlap);
	}
static void drawSegmentG(HDC hDC,POINTS coords[],int overlap) {

  MoveTo(hDC,coords[2].x+overlap,coords[2].y);
  LineTo(hDC,coords[3].x-overlap,coords[2].y);
	}
static void drawSegmentDot(HDC hDC,POINTS coords[],int overlap) {
	int sz=max(1,(coords[5].x-coords[4].x)/15);

	Ellipse(hDC,coords[4].x+3-sz,coords[4].y-sz,coords[4].x+3+sz,coords[4].y+sz);
	//uso la coordinata del bottom-left perché il puntino occupa un suo spazio, ridotto :)
	}
static void drawSegment2Dot(HDC hDC,POINTS coords[],int overlap) {
	int sz=max(1,(coords[3].x-coords[2].x)/8);

	Ellipse(hDC,coords[2].x+sz*3,coords[2].y-sz*5,coords[2].x+sz*4,coords[2].y-sz*4);
	Ellipse(hDC,coords[2].x+sz*3,coords[2].y+sz*3,coords[2].x+sz*4,coords[2].y+sz*4);
	}
int plotDigit(HDC hDC,BYTE n,UGRAPH_COORD_T xOrg,UGRAPH_COORD_T yOrg,BYTE xSize,BYTE ySize,BYTE style,BYTE thickness) {
	POINTS coords[7];		// 6 coordinate per 7 segmenti + punto
	int8_t noOverlap;
	GFX_COLOR myColor;

	switch(style) {
		case 0 /*LED_VERDI*/:
			myColor=Color565(0,255,0);
			break;
		case 1 /*LED_GIALLI*/:
			myColor=Color565(255,255,0);
			break;
		case 2 /*LED_ROSSI*/:
			myColor=Color565(255,0,0);
			break;
		default:		// tanto per...
			myColor=Color565(255,255,255);
			break;
		}
	PEN pen1=CreatePen(PS_SOLID,thickness,myColor),oldPen;
	BRUSH brush1=CreateSolidBrush(myColor /*Color565(0,0,0)*/),oldBrush;
	oldBrush=SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)brush1).brush;
	oldPen=SelectObject(hDC,OBJ_PEN,(GDIOBJ)pen1).pen;

	xSize-=thickness+2;
	ySize-=thickness+2;

	coords[0].x=xOrg+((ySize*slant)/100);			// top-left
	coords[0].y=yOrg;
	coords[1].x=xOrg+xSize+((ySize*slant)/100);		// top-right
	coords[1].y=yOrg;
	coords[2].x=xOrg+((ySize*slant)/200);			// mid-left
	coords[2].y=yOrg+ySize/2;
	coords[3].x=xOrg+xSize+((ySize*slant)/200);			// mid-right
	coords[3].y=yOrg+ySize/2;
	coords[4].x=xOrg;			// bottom-left
	coords[4].y=yOrg+ySize;
	coords[5].x=xOrg+xSize;			// bottom-right
	coords[5].y=yOrg+ySize;
	coords[6].x=xOrg+xSize;			// punto
	coords[6].y=yOrg+ySize;

	noOverlap=(coords[1].x-coords[0].x)/4;
  if(noOverlap==0)
    noOverlap=1;

	switch(n) {
		case 0:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			drawSegmentE(hDC,coords,noOverlap);
			drawSegmentF(hDC,coords,noOverlap);
			break;
		case 1:
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			break;
		case 2:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			drawSegmentE(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			break;
		case 3:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			break;
		case 4:
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			drawSegmentF(hDC,coords,noOverlap);
			break;
		case 5:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			drawSegmentF(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			break;
		case 6:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			drawSegmentE(hDC,coords,noOverlap);
			drawSegmentF(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			break;
		case 7:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			break;
		case 8:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			drawSegmentE(hDC,coords,noOverlap);
			drawSegmentF(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			break;
		case 9:
			drawSegmentA(hDC,coords,noOverlap);
			drawSegmentB(hDC,coords,noOverlap);
			drawSegmentC(hDC,coords,noOverlap);
			drawSegmentD(hDC,coords,noOverlap);
			drawSegmentF(hDC,coords,noOverlap);
			drawSegmentG(hDC,coords,noOverlap);
			break;
		case 10:		// punto, usare Point?
			drawSegmentDot(hDC,coords,noOverlap);
			break;
		case 11:		// -
			drawSegmentG(hDC,coords,noOverlap);
			break;
		case 12:		// +
			drawSegmentG(hDC,coords,noOverlap);
      MoveTo(hDC,(coords[3].x+coords[2].x)/2,(coords[2].y+coords[0].y)/2+noOverlap);
      LineTo(hDC,(coords[3].x+coords[2].x)/2,(coords[4].y+coords[2].y)/2-noOverlap);
			break;
		case 13:		// :
			drawSegment2Dot(hDC,coords,noOverlap);
			break;
		}

	SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)oldBrush);
	SelectObject(hDC,OBJ_PEN,(GDIOBJ)oldPen);
	return 1;
	}


LRESULT clockWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      char buf[32];
      PIC32_DATE date;
      PIC32_TIME time;
      int i;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      hDC->pen=CreatePen(1,1,LIGHTRED);
      
/* USARE ma occhio timezone  GCC di merda cancro ai nerd
      struct tm *oTime;
      time_t lTime;

      time(&lTime);
    	oTime=localtime(&lTime);
      */
      
      SetTimeFromNow(now,&date,&time);
      if(attrib & 0x80) {   // digitale/analogico
        int xSize,ySize;
        BYTE thickness;
        int cx,cy;
        if(!IsIconic(hWnd)) {
          cx=rc.right/(6); cy=(rc.bottom)/5;
          xSize=(rc.right-cx*2)/4;
          ySize=rc.bottom-(cy*2);
          thickness=cy/5;
          hDC->brush=CreateSolidBrush(BLUE);
          sprintf(buf,"%02u:%02u",time.hour, time.min);
					plotDigit(hDC,buf[0]-'0',rc.left+cx/2,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,buf[1]-'0',rc.left+(cx*3)/2,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,13,rc.left+(cx*5)/2,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,buf[3]-'0',rc.left+(cx*7)/2,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,buf[4]-'0',rc.left+(cx*9)/2,rc.top+cy,xSize,ySize,0,thickness);
          }
        else {
          cx=rc.right/10; cy=(rc.bottom)/7;
          xSize=3/*(rc.right-cx*2)/5*/;
          ySize=12/*rc.bottom-(cy)*/;
          thickness=max(xSize/3,1);
          hDC->brush=CreateSolidBrush(BLUE);
          sprintf(buf,"%02u:%02u",time.hour, time.min);
					plotDigit(hDC,buf[0]-'0',rc.left+cx,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,buf[1]-'0',rc.left+cx*3,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,13,rc.left+cx*5,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,buf[3]-'0',rc.left+cx*8,rc.top+cy,xSize,ySize,0,thickness);
					plotDigit(hDC,buf[4]-'0',rc.left+cx*12,rc.top+cy,xSize,ySize,0,thickness);
          
          
          }
        }   // digitale/analogico
      else {
        int radius;
        int cx,cy;
        float stp,rAngle,h,m,s;
        int xm,ym,xh,yh;
        if(!IsIconic(hWnd)) {
          hDC->font=CreateFont(8,6,0,0,FW_BOLD, FALSE,FALSE,FALSE, ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
          hDC->brush=CreateSolidBrush(RED);

          // Orologio 29/6/2022-4/7 .. since 1987 :)
          // variables to define the circle 
          cx=rc.right/2; cy=(rc.bottom-10)/2;
          stp=PI*2/12;
          radius=min(rc.right,rc.bottom-12)/2;
          if(radius<=0)
            radius=1;
          SetTextColor(hDC,WHITE); SetBkColor(hDC,BLACK);
          hDC->brush.size=0;
          //bah faccio prima :)  .. CreateBrushIndirct(&lbr)

          hDC->brush.size=1;       
          hDC->brush.color=SIENNA; //PROVA ELLISSE PIENO

          Ellipse(hDC,cx-radius,cy-radius,cx+radius,cy+radius);
          SetTextColor(hDC,BRIGHTGREEN);
          for(i=1; i<=12; i++) {
            rAngle=i*stp;
            itoa(buf,i,10);
            TextOut(hDC,cx-2+radius*sin(rAngle)*0.9,cy-2-radius*cos(rAngle)*0.9,buf,strlen(buf));
            }
          hDC->pen=CreatePen(1,1,BRIGHTCYAN);
    //      hDC->brush=CreateSolidBrush(BRIGHTCYAN);
          s=time.sec;
          rAngle=((s/5) * (PI*2))/12;
          radius=min(rc.right,rc.bottom)/3.0;
          int xs=cx+radius*sin(rAngle);
          int ys=cy-radius*cos(rAngle);
          m=time.min;
          rAngle=((m/5) * (PI*2))/12;
          radius=min(rc.right,rc.bottom)/3.5;
          xm=cx+radius*sin(rAngle);
          ym=cy-radius*cos(rAngle);
          h=time.hour % 12;
          m= m + (s/60); h= h + (m/60);
          rAngle=(h * (PI*2))/12;
          radius=min(rc.right,rc.bottom)/4.2;
          xh=cx+radius*sin(rAngle);
          yh=cy-radius*cos(rAngle);
          MoveTo(hDC,cx,cy);
          LineTo(hDC,xh,yh);
          MoveTo(hDC,cx,cy);
          LineTo(hDC,xm,ym);
          MoveTo(hDC,cx,cy);
          LineTo(hDC,xs,ys);

          hDC->font=CreateFont(8,6,0,0,FW_BOLD, FALSE,TRUE,FALSE, ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
          sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u %u",date.mday,date.mon,date.year+1980,
                  time.hour,time.min,time.sec);
          SetTextColor(hDC,LIGHTBLUE);
          TextOut(hDC,0,rc.bottom-8,buf,strlen(buf));
          DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
          }
        else {
          // Orologio 29/6/2022-4/7 .. since 1987 :)
          // variables to define the circle 
          cx=rc.right/2; cy=rc.bottom/2;
          stp=PI*2/12;
          radius=(min(rc.right,rc.bottom)/2) -1;
          if(radius<=0)
            radius=1;
          hDC->brush.size=0;
          //bah faccio prima :)  .. CreateBrushIndirct(&lbr)

      hDC->brush=CreateSolidBrush(RED);

          hDC->brush.size=1;       hDC->brush.color=SIENNA; //PROVA ELLISSE PIENO


          Ellipse(hDC,cx-radius,cy-radius,cx+radius,cy+radius);
          hDC->pen=CreatePen(1,1,BRIGHTCYAN);
    //      hDC->brush=CreateSolidBrush(BRIGHTCYAN);
          m=time.min;
          rAngle=((m/5) * (PI*2))/12;
          radius=min(rc.right,rc.bottom)/3.5;
          xm=cx+radius*sin(rAngle);
          ym=cy-radius*cos(rAngle);
          h=time.hour % 12;
          h= h + (m/60);
          rAngle=(h * (PI*2))/12;
          radius=min(rc.right,rc.bottom)/4.0;
          xh=cx+radius*sin(rAngle);
          yh=cy-radius*cos(rAngle);
          MoveTo(hDC,cx,cy);
          LineTo(hDC,xh,yh);
          MoveTo(hDC,cx,cy);
          LineTo(hDC,xm,ym);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          }
        }
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      EndPaint(hWnd,&ps);
      }
      return 0;
//      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,
              attrib & 0x80 ? BLUE: SIENNA);
      }
      return 1;
      break;
      
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case 1+0:
            SetWindowByte(hWnd,GWL_USERDATA+0,attrib ^ 0x80);
            break;
          case 2+0:
            SetWindowByte(hWnd,GWL_USERDATA+0,attrib ^ 0x40);
            break;
          case 3+0:
            goto sync_time;
            break;
          }
        }
      break;
   	case WM_INITMENU:
   	case WM_INITMENUPOPUP:    // in windows non lo mettevo, ma parrebbe utile cmq
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,1+0,MF_BYCOMMAND | (attrib & 0x80 ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,2+0,MF_BYCOMMAND | (attrib & 0x40 ? MF_CHECKED : MF_UNCHECKED));
      break;
      
    case WM_TIMECHANGE:
    case WM_TIMER:
		case WM_DISPLAYCHANGE: // Only comes through on plug'n'play systems
      if(attrib & 0x40) {   // chimes
        PIC32_DATE date;
        PIC32_TIME time;
        SetTimeFromNow(now,&date,&time);
        if(time.min==0 && time.sec==0) {
          MessageBeep(MB_ICONINFORMATION);    // beh mettere altro :)
          }
        }
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      attrib=GetProfileInt(OPZIONI_STR,"CLOCK",0);
      SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
      if(attrib & 2)        // gestire on top in menu!
        cs->style |= WS_EX_TOPMOST;
      SetTimer(hWnd,1,attrib & 0x80 ? 60000 : 1000,NULL);
      }
      return 0;
      break;
    case WM_SIZE:
upd_timer:
      if((attrib & 0x80) || IsIconic(hWnd)/*(wParam==SIZE_MINIMIZED)*/)
        SetTimer(hWnd,1,60000,NULL);
      else
        SetTimer(hWnd,1,1000,NULL);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_WINDOWPOSCHANGED: 
      {
      WINDOWPOS *wpos=(WINDOWPOS*)lParam;
      if(!(wpos->flags & SWP_NOSENDCHANGING))
        ;
      }
      break;
    case WM_DESTROY:
      KillTimer(hWnd,1);
//      DestroyWindow(hWnd);
//      m_WndClock=NULL;
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_F6:   // mettere come acceleratori ora..
          SetWindowByte(hWnd,GWL_USERDATA+0,attrib ^ 0x80);
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_F5:
          {HDC hDC;
          DC myDC;
          RECT rc;
sync_time: 
#ifdef USA_WIFI
          hDC=GetDC(hWnd,&myDC);
          hDC->font=CreateFont(8,6,0,0,FW_BOLD, FALSE,TRUE,FALSE, ANSI_CHARSET,
                  OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
          SetTextColor(hDC,BRIGHTRED);
          GetClientRect(hWnd,&rc);
          TextOut(hDC,0,rc.bottom-8,"Syncing time...",15);
          DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
          
    			m2m_wifi_get_sytem_time();   // intanto :)
          ReleaseDC(hWnd,hDC);
#else
#endif
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
    case WM_RBUTTONDOWN:
      // fare menu context...
      
      if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
        goto sync_time;
      else
        SetWindowByte(hWnd,GWL_USERDATA+0,attrib ^ 0x80);
      InvalidateRect(hWnd,NULL,TRUE);
      goto upd_timer;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }
extern const char months[13][4];
#define Mesi(n) months[n+1]
extern const char wdays[7][4];
#define Giorni(n) wdays[n ? n-1 : 6]
extern const unsigned char days_month[2][12];
LRESULT calendarWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      char buffer[32];
//      PIC32_DATE date;
//      PIC32_TIME time;
      PEN hpenForegnd,hpenBackgnd;
      BRUSH hbrForegnd,hbrBackgnd;
      RECT CalendarRect,NumGiornoPos,GiornoPos,MesePos, rc;
      FONT hFont,hFont1,hFont2,hOldFont;
      int x,y,i,j,dotHeight,dotWidth;
      
//      SetTimeFromNow(now,&date,&time);
      struct tm *oTime;            /* the time currently displayed on the clock          */
      time_t lTime;

      time(&lTime);
    	oTime=localtime(&lTime);		// nota: fuso orario timezone=0 6/3/23
      GetClientRect(hWnd,&CalendarRect);
      
      // Calendar 3/3/2023 .. since 1992 :)
      hbrForegnd  = CreateSolidBrush(GetSysColor(COLOR_WINDOWTEXT));
      hbrBackgnd  = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
      hpenForegnd = CreatePen(0, 1, GetSysColor(COLOR_WINDOWTEXT));
      hpenBackgnd = CreatePen(0, 1, GetSysColor(COLOR_WINDOW));

      GetClientRect(hWnd,&rc);
      DWORD newHeight=rc.bottom-rc.top;
      DWORD newWidth=rc.right-rc.left;

      SetBkMode(hDC, TRANSPARENT);

      if(IsIconic(hWnd)) {
        NumGiornoPos.top=CalendarRect.top+newHeight/6;
        NumGiornoPos.left=CalendarRect.left;
        NumGiornoPos.bottom=CalendarRect.bottom-newHeight/8;
        NumGiornoPos.right=CalendarRect.right;

        GiornoPos.top=CalendarRect.top+1;
        GiornoPos.left=CalendarRect.left+newWidth/10;
        GiornoPos.bottom=CalendarRect.top;
        GiornoPos.right=CalendarRect.right;

        MesePos.top=CalendarRect.bottom-newHeight/3-1;
        MesePos.left=CalendarRect.left;
        MesePos.bottom=CalendarRect.bottom-newHeight/10;
        MesePos.right=CalendarRect.right;

        hFont=CreateFont((CalendarRect.bottom-CalendarRect.top)/2,
                     (CalendarRect.right-CalendarRect.left)/3,
                     0,0,400,0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,
                     CLIP_DEFAULT_PRECIS,PROOF_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE /*FF_ROMAN*/,
                     "Modern");
        hFont1=CreateFont((CalendarRect.bottom-CalendarRect.top)/4,
                     (CalendarRect.right-CalendarRect.left)/5,
                     0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE,
                     "Modern");
        hFont2=CreateFont((CalendarRect.bottom-CalendarRect.top)/4,
                     (CalendarRect.right-CalendarRect.left)/5,
                     0,0,400,0,0,0,ANSI_CHARSET,OUT_DEFAULT_PRECIS,
                     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_DONTCARE,
                     "Modern");
        SelectObject(hDC,OBJ_PEN,(GDIOBJ)hpenForegnd);
        Rectangle(hDC,CalendarRect.left,CalendarRect.top,
          CalendarRect.right,CalendarRect.bottom);
        SetTextAlign(hDC,TA_CENTER | TA_TOP | TA_NOUPDATECP);
        hOldFont=SelectObject(hDC,OBJ_FONT,(GDIOBJ)hFont).font;
        SetTextColor(hDC,BRIGHTRED);
        itoa(buffer,oTime->tm_mday,10);
//          ExtTextOut(hDC,(NumGiornoPos.right+NumGiornoPos.left)/2,NumGiornoPos.top,0,
//            &NumGiornoPos,buffer,strlen(buffer),0);
        DrawText(hDC,buffer,-1,&NumGiornoPos,DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)hFont1);
        SetTextColor(hDC,BLACK);
        sprintf(buffer,"%s",Giorni(oTime->tm_wday));
        buffer[3]=0;
//          ExtTextOut(hDC,(GiornoPos.right+GiornoPos.left)/2,GiornoPos.top,0,
//            &GiornoPos,buffer,strlen(buffer),0);
// troppo grande qua...        DrawText(hDC,buffer,-1,&GiornoPos,DT_CENTER | DT_TOP | DT_SINGLELINE);
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)hFont2);
        SetTextColor(hDC,BLACK);
        sprintf(buffer,"%s",Mesi(oTime->tm_mon));
        buffer[3]=0;
//          ExtTextOut(hDC,(MesePos.right+MesePos.left)/2,MesePos.top,0,&MesePos,
//            buffer,strlen(buffer),0);
// troppo grande qua...        DrawText(hDC,buffer,-1,&MesePos,DT_CENTER | DT_TOP | DT_SINGLELINE);
        }
      
      else {
        CalendarRect.top=CalendarRect.top+newHeight/9;
        CalendarRect.left=CalendarRect.left+newWidth/5  +1;
        CalendarRect.right=CalendarRect.right-newWidth/5  +1;
        CalendarRect.bottom=CalendarRect.bottom-newHeight/9;

        dotWidth=CalendarRect.right-CalendarRect.left;
        dotHeight=CalendarRect.bottom-CalendarRect.top;

        x=dotHeight/8;
        y=dotWidth/8;

        rc=CalendarRect;

        j=366/y;
        for(i=367; i>oTime->tm_yday; i-=j) {
          FillRect(hDC,&rc,hbrForegnd); //fare line o rect, non pieno
          rc.top+=1;
          rc.left-=1;
          rc.bottom+=1;
          rc.right-=1;
          }

        CalendarRect=rc;

        y=CalendarRect.bottom-CalendarRect.top;
        x=CalendarRect.right-CalendarRect.left;

        NumGiornoPos.top=CalendarRect.top+y/3;    // 4
        NumGiornoPos.left=CalendarRect.left+x/10;
        NumGiornoPos.bottom=CalendarRect.bottom-y/3;
        NumGiornoPos.right=CalendarRect.right-x/10;

        GiornoPos.top=CalendarRect.top+y/18;
        GiornoPos.left=CalendarRect.left;
        GiornoPos.bottom=CalendarRect.top+y/5;
        GiornoPos.right=CalendarRect.right;

        MesePos.top=CalendarRect.bottom-y/5;
        MesePos.left=CalendarRect.left;
        MesePos.bottom=CalendarRect.bottom-y/10;
        MesePos.right=CalendarRect.right;

        SelectObject(hDC,OBJ_PEN,(GDIOBJ)hpenForegnd);
        Rectangle(hDC,CalendarRect.left,CalendarRect.top,
          CalendarRect.right,CalendarRect.bottom);

        hFont=CreateFont((CalendarRect.bottom-CalendarRect.top)/2,
                     (CalendarRect.right-CalendarRect.left)/3,
                     0,0,600,0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,
                     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_ROMAN,
                     "Roman");
        hFont1=CreateFont((CalendarRect.bottom-CalendarRect.top)/7,
                     (CalendarRect.right-CalendarRect.left)/11,
                     0,0,400,0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,
                     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_ROMAN,
                     "Roman");
        hFont2=CreateFont((CalendarRect.bottom-CalendarRect.top)/7,
                     (CalendarRect.right-CalendarRect.left)/11,
                     0,0,400,0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,
                     CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
                     DEFAULT_PITCH | FF_ROMAN,
                     "Roman");
        SetTextAlign(hDC,TA_CENTER | TA_TOP | TA_NOUPDATECP);
        hOldFont=SelectObject(hDC,OBJ_FONT,(GDIOBJ)hFont).font;
        SetTextColor(hDC,BRIGHTRED);
        itoa(buffer,oTime->tm_mday,10);
        //uso drawtext xché TextAlign non ce l'ho ancora...
//          ExtTextOut(hDC,(NumGiornoPos.right+NumGiornoPos.left)/2,NumGiornoPos.top,0,
//            &NumGiornoPos,buffer,strlen(buffer),0);
        DrawText(hDC,buffer,-1,&NumGiornoPos,DT_CENTER | DT_TOP | DT_SINGLELINE);
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)hFont1);
        SetTextColor(hDC,BLACK);

//          ExtTextOut(hDC,(GiornoPos.right+GiornoPos.left)/2,GiornoPos.top,0,&GiornoPos,
//            Giorni(oTime->tm_wday),strlen(Giorni(oTime->tm_wday)),0);
        DrawText(hDC,Giorni(oTime->tm_wday),-1,&GiornoPos,DT_CENTER | DT_TOP | DT_SINGLELINE);
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)hFont2);
        SetTextColor(hDC,BLACK);

//          ExtTextOut(hDC,(MesePos.right+MesePos.left)/2,MesePos.top,0,&MesePos,
//            Mesi(oTime->tm_mon),strlen(Mesi(oTime->tm_mon)),0);
        DrawText(hDC,Mesi(oTime->tm_mon),-1,&MesePos,DT_CENTER | DT_TOP | DT_SINGLELINE);

        SelectObject(hDC,OBJ_FONT,(GDIOBJ)hOldFont);

        DeleteObject(OBJ_FONT,(GDIOBJ)hFont);
        DeleteObject(OBJ_FONT,(GDIOBJ)hFont1);
        DeleteObject(OBJ_FONT,(GDIOBJ)hFont2);
        }

      DeleteObject(OBJ_BRUSH,(GDIOBJ)hbrForegnd);
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hbrBackgnd);
      DeleteObject(OBJ_PEN,(GDIOBJ)hpenForegnd);
      DeleteObject(OBJ_PEN,(GDIOBJ)hpenBackgnd);
      EndPaint(hWnd,&ps);
      }
      return 0;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,GRAY242 /*GetSysColor(COLOR_WINDOWTEXT)*/);
      }
      return 1;
      break;
      
    case WM_TIMECHANGE:
    case WM_TIMER:
		case WM_DISPLAYCHANGE: // Only comes through on plug'n'play systems
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      if(GetWindowByte(hWnd,GWL_USERDATA+0) & 2)        // gestire on top in menu!
        cs->style |= WS_EX_TOPMOST;
      SetTimer(hWnd,1,86400000L,NULL);
      }
      return 0;
      break;
    case WM_WINDOWPOSCHANGED: 
      {
      WINDOWPOS *wpos=(WINDOWPOS*)lParam;
      if(!(wpos->flags & SWP_NOSENDCHANGING))
        ;
      }
      break;
    case WM_DESTROY:
      KillTimer(hWnd,1);
//      DestroyWindow(hWnd);
//      m_WndCalendar=NULL;
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_F5:
#ifdef USA_WIFI
    			m2m_wifi_get_sytem_time();   // intanto :)
#endif
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
    case WM_RBUTTONDOWN:
      // fare menu context...
#ifdef USA_WIFI
 			m2m_wifi_get_sytem_time();   // intanto :)
#endif
      InvalidateRect(hWnd,NULL,TRUE);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


LRESULT calcDlgproc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
/*const*/ char *calcSymbols[16]={
  "0","1","2","3","4","5","6","7","8","9",
  "+","-","*","/",".","="
  };
DLGITEMTEMPLATE calcDlgItems[16];
DLGITEMTEMPLATE calcDisplay={MAKECLASS(WC_STATIC),
  SS_RIGHT | WS_BORDER | WS_VISIBLE | WS_CHILD,
  21,9,80,14,
  "0.",
  32};
#define CALC_BUTTON_XSIZE 18
#define CALC_BUTTON_YSIZE 16
#define CALC_BUTTON_XSPACING 26
#define CALC_BUTTON_YSPACING 26
DLGTEMPLATE calcDlg={WS_BORDER | WS_VISIBLE | WS_DLGFRAME /*| WS_CAPTION*/ | WS_CHILD | WS_CLIPCHILDREN,
// mettere direttamente sul desktop e quindi cambiare!
//DLGTEMPLATE calcDlg={WS_POPUP | WS_BORDER | WS_VISIBLE | WS_DLGFRAME | WS_CAPTION | WS_CLIPCHILDREN,
  16,
  15,5,125,136   /*+TITLE_HEIGHT ev. CAPTION*/,
  {0},   // font
  "Calcolatrice",
  {&calcDisplay,&calcDlgItems[0],&calcDlgItems[1],&calcDlgItems[2],&calcDlgItems[3],
  &calcDlgItems[4],&calcDlgItems[5],&calcDlgItems[6],&calcDlgItems[7],
  &calcDlgItems[8],&calcDlgItems[9],&calcDlgItems[10],&calcDlgItems[11],
  &calcDlgItems[12],&calcDlgItems[13],&calcDlgItems[14],&calcDlgItems[15]}};
LRESULT calcWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  static char display[16];    // OVVIAMENTE andrebbero tutte in window extra byte!
  static BYTE displayptr=0;
  static double op1,op2;
  static char op,newop;
  
  switch(message) {
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        // menu o accelerator ev.
        }
      if(lParam) {   // da child
        switch(LOWORD(wParam)) { 
          case 0+1:
          case 1+1:
          case 2+1:
          case 3+1:
          case 4+1:
          case 5+1:
          case 6+1:
          case 7+1:
          case 8+1:
          case 9+1:
isnumber:           
            if(newop) {
              *display=0;
              displayptr=0;
              }
            displayptr=strlen(display);
            if(displayptr<15) {
              display[displayptr++]=LOWORD(wParam)-1+'0';
              display[displayptr]=0;
              }
            newop=0;
            break;
          case 10+1:
            op='+';
isop:
            op1=strtod(display,NULL);
            newop=1;
            break;
          case 11+1:
            op='-';
            goto isop;
            break;
          case 12+1:
            op='*';
            goto isop;
            break;
          case 13+1:
            op='/';
            goto isop;
            break;
          case 14+1:    // .
            break;
          case 15+1:    // =
calc:
            if(op) {
              op2=strtod(display,NULL);
              switch(op) {
                case '+':
                  op1=op1+op2;
                  break;
                case '-':
                  op1=op1-op2;
                  break;
                case '*':
                  op1=op1*op2;
                  break;
                case '/':
                  if(op1)
                    op1=op1/op2;
                  else
                    op1=0;
                  break;
                }
              sprintf(display,"%g",op1);
              displayptr=0;
              op=0;
              newop=1;
              }
            break;
          }
update:
        SetWindowText(GetDlgItem(hWnd,32),display);   // dovrebbe andare anche se non dialog... Enumerate mi viene lunga!
        return DefWindowProc(hWnd,message,wParam,lParam);
        }
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i,x,y;
      i=0;
      calcDlgItems[i].class=MAKECLASS(WC_BUTTON);
      calcDlgItems[i].x=10+0*CALC_BUTTON_XSPACING;
      calcDlgItems[i].y=30+3*CALC_BUTTON_YSPACING;
      calcDlgItems[i].cx=CALC_BUTTON_XSIZE;
      calcDlgItems[i].cy=CALC_BUTTON_YSIZE;   // evito 0 (v. dialogbox / createdialog
      calcDlgItems[i].id=i+1;
      calcDlgItems[i].style=BS_VCENTER | BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP | WS_GROUP,
      calcDlgItems[i].caption=calcSymbols[i];
      i++;
      for(y=2; y>=0; y--) {
        for(x=0; x<3; x++) {
          calcDlgItems[i].class=MAKECLASS(WC_BUTTON);
          calcDlgItems[i].x=10+x*CALC_BUTTON_XSPACING;
          calcDlgItems[i].y=30+y*CALC_BUTTON_YSPACING;
          calcDlgItems[i].cx=CALC_BUTTON_XSIZE;
          calcDlgItems[i].cy=CALC_BUTTON_YSIZE;
          calcDlgItems[i].id=i+1;
          calcDlgItems[i].style=BS_VCENTER | BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
          calcDlgItems[i].caption=calcSymbols[i];
          i++;
          }
        }
      for(y=0; y<4; y++) {
        calcDlgItems[i].class=MAKECLASS(WC_BUTTON);
        calcDlgItems[i].x=10+4+3*CALC_BUTTON_XSPACING;
        calcDlgItems[i].y=30+y*CALC_BUTTON_YSPACING;
        calcDlgItems[i].cx=CALC_BUTTON_XSIZE;
        calcDlgItems[i].cy=CALC_BUTTON_YSIZE;
        calcDlgItems[i].id=i+1;
        calcDlgItems[i].style=BS_VCENTER | BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
        calcDlgItems[i].caption=calcSymbols[i];
        i++;
        }
      calcDlgItems[i].class=MAKECLASS(WC_BUTTON);
      calcDlgItems[i].x=10+1*CALC_BUTTON_XSPACING;
      calcDlgItems[i].y=30+3*CALC_BUTTON_YSPACING;
      calcDlgItems[i].cx=CALC_BUTTON_XSIZE;
      calcDlgItems[i].cy=CALC_BUTTON_YSIZE;
      calcDlgItems[i].id=i+1;
      calcDlgItems[i].style=BS_VCENTER | BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
      calcDlgItems[i].caption=calcSymbols[i];
      i++;
      calcDlgItems[i].class=MAKECLASS(WC_BUTTON);
      calcDlgItems[i].x=10+4+2*CALC_BUTTON_XSPACING;
      calcDlgItems[i].y=30+3*CALC_BUTTON_YSPACING;
      calcDlgItems[i].cx=CALC_BUTTON_XSIZE;
      calcDlgItems[i].cy=CALC_BUTTON_YSIZE;
      calcDlgItems[i].id=i+1;
      calcDlgItems[i].style=BS_VCENTER | BS_CENTER | BS_PUSHBUTTON | WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP,
      calcDlgItems[i].caption=calcSymbols[i];
      i++;
      calcDlg.cdit=i+1 /*display!*/;
      
      for(i=0; i<16; i++)
        CreateWindow(calcDlgItems[i].class,calcDlgItems[i].caption,calcDlgItems[i].style,
          calcDlgItems[i].x,calcDlgItems[i].y,calcDlgItems[i].cx,calcDlgItems[i].cy,
          hWnd,(MENU *)(int)calcDlgItems[i].id,NULL);
      CreateWindow(calcDisplay.class,calcDisplay.caption,calcDisplay.style,
        calcDisplay.x,calcDisplay.y,calcDisplay.cx,calcDisplay.cy,
        hWnd,(MENU *)(void*)32,NULL);
      }
      return 0;
      break;
      
    case WM_CHAR:
      switch(LOWORD(wParam)) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          wParam-='0'-1;
          goto isnumber;
          break;
        case '.': //VK_DECIMAL:
//          goto isnumber;
          break;
        case '+': //VK_ADD:
          op='+';
          goto isop;
          break;
        case '-': //VK_SUBTRACT:
          op='-';
          goto isop;
          break;
        case '*': //VK_MULTIPLY:
          op='*';
          goto isop;
          break;
        case '/': //VK_DIVIDE:
          op='/';
          goto isop;
          break;
        case '\x1b':
          op=0;
          display[displayptr=0]='0';
          display[displayptr+1]=0;
          newop=1;
          goto update;
          break;
        case '\r':
          goto calc;
          break;
        default:
          MessageBeep(MB_ICONHAND);
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      return 0;
      break;
      
    case WM_DESTROY:
//      m_WndCalc=NULL;
//      DestroyWindow(hWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_CTLCOLOR:
      return BLUE;
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }
LRESULT calcDlgproc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  static char display[16];    // OVVIAMENTE andrebbero tutte in window extra byte!
  static BYTE displayptr=0;
  static double op1,op2;
  static char op,newop;
  
  switch(message) {
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        // verificare con CalcWnd!! 2026
        switch(LOWORD(wParam)) { 
          case 0+1:
          case 1+1:
          case 2+1:
          case 3+1:
          case 4+1:
          case 5+1:
          case 6+1:
          case 7+1:
          case 8+1:
          case 9+1:
isnumber:           
            if(newop) {
              *display=0;
              displayptr=0;
              }
            displayptr=strlen(display);
            if(displayptr<15) {
              display[displayptr++]=LOWORD(wParam)-1+'0';
              display[displayptr]=0;
              }
            newop=0;
            break;
          case 10+1:
            op='+';
isop:
            op1=strtod(display,NULL);
            newop=1;
            break;
          case 11+1:
            op='-';
            goto isop;
            break;
          case 12+1:
            op='*';
            goto isop;
            break;
          case 13+1:
            op='/';
            goto isop;
            break;
          case 14+1:    // .
            break;
          case 15+1:    // =
calc:
            if(op) {
              op2=strtod(display,NULL);
              switch(op) {
                case '+':
                  op1=op1+op2;
                  break;
                case '-':
                  op1=op1-op2;
                  break;
                case '*':
                  op1=op1*op2;
                  break;
                case '/':
                  if(op1)
                    op1=op1/op2;
                  else
                    op1=0;
                  break;
                }
              sprintf(display,"%g",op1);
              displayptr=0;
              op=0;
              newop=1;
              }
            break;
          }
update:
        SetWindowText(GetDlgItem(hWnd,32),display);
        return DefWindowProc(hWnd,message,wParam,lParam);
        }
      break;
    case WM_CHAR:
      switch(LOWORD(wParam)) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
          wParam-='0'-1;
          goto isnumber;
          break;
        case '.': //VK_DECIMAL:
//          goto isnumber;
          break;
        case '+': //VK_ADD:
          op='+';
          goto isop;
          break;
        case '-': //VK_SUBTRACT:
          op='-';
          goto isop;
          break;
        case '*': //VK_MULTIPLY:
          op='*';
          goto isop;
          break;
        case '/': //VK_DIVIDE:
          op='/';
          goto isop;
          break;
        case '\x1b':
          op=0;
          display[displayptr=0]='0';
          display[displayptr+1]=0;
          newop=1;
          goto update;
          break;
        case '\r':
          goto calc;
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          // forwardare per mettere focus? o no? o non così cmq... v. defDlgProc
          break;
        }
      return 0;
      break;
      
/*    case WM_DESTROY:
      m_WndCalc=NULL;
      break; no*/
      
    case WM_CTLCOLORDLG:
      return CYAN;
      break;
    case WM_CTLCOLOR:
      return BLUE;
      break;
    default:
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


static char *parseSurfItems(const char *, struct HTMLinfo *);
static int surfSetState(HWND hWnd,BYTE state,const char *text) {
  HWND myWnd;
  
  SetWindowByte(hWnd,GWL_USERDATA+3,state);
  
  myWnd=GetDlgItem(hWnd,204);
  if(myWnd) {
    switch(state) {
      case 0:   // IDLE
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),1);
        break;
      case 1:   // connecting
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),2);
        break;
      case 2:   // connesso
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),2);
        break;
      case 3:   // download
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),2);
        break;
      case 4:   // finito
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),2);
        break;
      case 5:   // socket chiuso
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),1);
        break;
      default:   // errore ecc
        SendMessage(myWnd,SB_SETICON,MAKELONG(1,1),3);
        break;
      }
    if(text)
      SetWindowText(myWnd,text);
    }
  myWnd=GetDlgItem(hWnd,203);    // icona
  switch(state) {
    case 0:   // IDLE
      myWnd->icon=surfIcon;
      break;
    case 1:   // connecting
      myWnd->icon=surfWaitIcon;
      break;
    case 2:   // connesso
      myWnd->icon=surfWaitIcon;
      break;
    case 3:   // download
      myWnd->icon=surfWaitIcon;
      break;
    case 4:   // finito
      myWnd->icon=surfIcon;
      break;
    case 5:   // socket chiuso
      myWnd->icon=surfIcon;
      break;
    default:   // errore ecc
      myWnd->icon=surfIcon;
      break;
    }
  InvalidateRect(myWnd,NULL,TRUE);
  
  }
char *surfGetState(HWND hWnd) {
  switch(GetWindowByte(hWnd,GWL_USERDATA+3)) {
    case 0:   // IDLE
      break;
    case 1:   // connecting
      break;
    case 2:   // connesso
      break;
    case 3:   // download
      break;
    case 4:   // finito
      break;
    case 5:   // socket chiuso
      break;
    default:   // errore ecc
      break;
    }
  }
const char *surfApp="Surf!";
static int surfNavigate(HWND hWnd,const char *url,const char *file,BYTE tipo) {
  SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
  int n;
  char buf[128];
	char *pagePtr=GET_WINDOW_OFFSET(hWnd,sizeof(struct HTMLinfo)),*p;
  IP_ADDR dstIP;
  
#ifdef USA_WIFI
  if(url) {
    SetWindowText(GetDlgItem(hWnd,201),url);
    strcpy(buf,url);
    }
  else {
//    ricaricare pagina ultima...
    GetWindowText(GetDlgItem(hWnd,201),buf,63);
    }
  url=buf;
  if(!strnicmp(url,"http:",5))
    url+=5;
  if(*url=='\\' || *url=='/')
    url++;
  if(*url=='\\' || *url=='/')
    url++;
  if((p=strchr(url,'\\')) || (p=strchr(url,'/'))) {
    *p=0;   // e puntare file dove segue??
    if(!file)
      file=p+1;
    }
  if(!strnicmp(url,"file:",5)) {
    SUPERFILE f;
    url+=5;
    getDrive(url,&f,NULL);
    if(SuperFileOpen(&f,url,OPEN_READ,TYPE_TEXT | SHARE_READ)) {
      n=SuperFileSize(&f,url);
      memset(pagePtr,0,HTML_SIZE);
      n=min(n,HTML_SIZE);
      SuperFileGets(&f,pagePtr,n);
      SuperFileClose(&f);
      return 1;
      }
    return 0;
    }
    
  if(s==INVALID_SOCKET) {   // gestire wifi/ethernet
    struct sockaddr_in strAddr;
    BYTE u8Flags=0;   // boh??
//    HWND myWnd;
    DWORD tOut,bytes2Read;
    BYTE connectionClose=0;
    struct HTMLinfo *myHtml=(struct HTMLinfo *)GET_WINDOW_OFFSET(hWnd,0);

    s = socket(AF_INET,SOCK_STREAM,u8Flags);
    if(s != INVALID_SOCKET) {
      SetWindowByte(hWnd,GWL_USERDATA+1,s);

      socketData[0].s=s;

      strAddr.sin_family = AF_INET;
      strAddr.sin_port = _htons(80);

			strncpy(myHtml->baseAddr,url,sizeof(myHtml->baseAddr)-1);
			myHtml->baseAddr[sizeof(myHtml->baseAddr)-1]=0;

      StringToIPAddress(url,&strAddr.sin_addr.s_addr);
      if(!strAddr.sin_addr.s_addr) {
        tOut=0;
        *(unsigned long*)internetBuffer=0;
        gethostbyname((uint8_t*)url);
        while(!*(unsigned long*)internetBuffer && tOut<DNS_TIMEOUT) {
          m2m_wifi_handle_events(NULL);
          tOut++;
          __delay_ms(1);
          }
        strAddr.sin_addr.s_addr=*(unsigned long*)internetBuffer;
        }
      dstIP.Val=strAddr.sin_addr.s_addr;
      if(dstIP.Val)
        sprintf(buf,"connessione a %u.%u.%u.%u...",dstIP.v[0],dstIP.v[1],dstIP.v[2],dstIP.v[3]);
      else
        sprintf(buf,"host non trovato");
      surfSetState(hWnd,1,buf);

      connect(s,(struct sockaddr*)&strAddr,sizeof(struct sockaddr_in));
      tOut=0;
      *(unsigned long*)internetBuffer=0;
      while(!*(unsigned long*)internetBuffer && tOut<SURF_TIMEOUT) {
        m2m_wifi_handle_events(NULL);
        tOut++;
        __delay_ms(1);
        }

      if(!*(unsigned long*)internetBuffer)
        goto close;
      
      sprintf(buf,"GET %s HTTP/1.1\r\nUser-Agent: Surf/2.0\r\nAccept: %s\r\n\r\n",
        file ? file : "/",tipo ? "*.*" : "*.*" /*fare!*/);   // Host, Accept
      send(s,buf,strlen(buf),0);
      tOut=0;
      *(unsigned long*)internetBuffer=0;
      while(!*(unsigned long*)internetBuffer && tOut<SURF_TIMEOUT) {
        m2m_wifi_handle_events(NULL);
        tOut++;
        __delay_ms(1);
        }

      surfSetState(hWnd,2,NULL);
      
			recv(s,rxBuffer,1536,SURF_TIMEOUT);
			tOut=0;
			while(!socketData[0].dataAvail && tOut<SURF_TIMEOUT) {
				m2m_wifi_handle_events(NULL);
				tOut++;
				__delay_ms(1);
				}
      if(!socketData[0].dataAvail) 
        goto close;
      
      sprintf(buf,"lettura header [%u]...",socketData[0].dataAvail);
      surfSetState(hWnd,3,buf);
          
			if(p=(char*)stristr(rxBuffer,tagConnection)) {
        connectionClose=!strnicmp(p+strlen(tagConnection)+1,"close",5);
        }
			if(p=(char*)stristr(rxBuffer,tagContentLength)) {
				bytes2Read=atoi(p+strlen(tagContentLength)+1);
        p=(char*)strstr(rxBuffer,"\r\n\r\n");   // https://serverfault.com/questions/862383/if-i-send-a-http-get-request-do-i-receive-the-response-in-get
        if(!p)
          goto error_close;
        p+=4;        
				pagePtr=GET_WINDOW_OFFSET(hWnd,sizeof(struct HTMLinfo));
        memset(pagePtr,0,HTML_SIZE);
				n=0;

        surfSetState(hWnd,3,NULL);
        
        socketData[0].dataAvail -= p-(char*)rxBuffer;
        n+=socketData[0].dataAvail;
        bytes2Read-=socketData[0].dataAvail;
        memcpy(pagePtr,p,socketData[0].dataAvail);
        pagePtr+=socketData[0].dataAvail;  
        *pagePtr=0;
        socketData[0].dataAvail=0;
        
				while(bytes2Read>0 && n<HTML_SIZE) {
					recv(s,rxBuffer,1536,SURF_TIMEOUT);

					tOut=0;
					while(!socketData[0].dataAvail && tOut<SURF_TIMEOUT) {
						m2m_wifi_handle_events(NULL);
						tOut++;
						__delay_ms(1);
						}
					sprintf(buf,"lettura dati [%u]...",socketData[0].dataAvail);
          surfSetState(hWnd,3,buf);
          
					if(socketData[0].dataAvail>0) {
						n+=socketData[0].dataAvail;
						bytes2Read-=socketData[0].dataAvail;
						memcpy(pagePtr,rxBuffer,socketData[0].dataAvail);
						pagePtr+=socketData[0].dataAvail; 
            *pagePtr=0;
						socketData[0].dataAvail=0;
						}
          else
            break;
					}
				}
      else
        goto close;
      
      sprintf(buf,"letti %u bytes",n);
      surfSetState(hWnd,3,buf);

			myHtml->fLen=n;
			if(n>0) {
				pagePtr=GET_WINDOW_OFFSET(hWnd,sizeof(struct HTMLinfo));
    		parseSurfItems(pagePtr,myHtml);
        if(*myHtml->sTitle) {
          strcpy(buf,surfApp);
          strcat(buf,"-");
          strncat(buf,myHtml->sTitle,50);
          SetWindowText(hWnd,buf);
          }
				}
      
      surfSetState(hWnd,4,NULL);

			if(connectionClose) {   // 
error_close:        
close:
			  close(s);
				SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);		// inutile qua ma ok :)
        surfSetState(hWnd,5,NULL);
				}	

      }
    }
#endif
  InvalidateRect(hWnd,NULL,TRUE);
  return n;
  }
static int surfNavigateDownload(HWND hWnd,SUPERFILE *f,const char *url,const char *file,BYTE tipo) {
  SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
  int n;
  char buf[128];
	char *p;
  IP_ADDR dstIP;
  
#ifdef USA_WIFI
  strcpy(buf,url);
  url=buf;
  if(!strnicmp(url,"http:",5))
    url+=5;
  if(*url=='\\' || *url=='/')
    url++;
  if(*url=='\\' || *url=='/')
    url++;
  if((p=strchr(url,'\\')) || (p=strchr(url,'/'))) {
    *p=0;   // e puntare file dove segue??
    if(!file)
      file=p+1;
    }
  
  if(s==INVALID_SOCKET) {   // gestire wifi/ethernet
    struct sockaddr_in strAddr;
    BYTE u8Flags=0;   // boh??
    DWORD tOut,bytes2Read;
    BYTE connectionClose=0;
    struct HTMLinfo *myHtml;
    s = socket(AF_INET,SOCK_STREAM,u8Flags);
    if(s != INVALID_SOCKET) {
      SetWindowByte(hWnd,GWL_USERDATA+1,s);

      socketData[0].s=s;

      strAddr.sin_family = AF_INET;
      strAddr.sin_port = _htons(80);

      StringToIPAddress(url,&strAddr.sin_addr.s_addr);
      if(!strAddr.sin_addr.s_addr) {
        tOut=0;
        *(unsigned long*)internetBuffer=0;
        gethostbyname((uint8_t*)url);
        while(!*(unsigned long*)internetBuffer && tOut<DNS_TIMEOUT) {
          m2m_wifi_handle_events(NULL);
          tOut++;
          __delay_ms(1);
          }
        strAddr.sin_addr.s_addr=*(unsigned long*)internetBuffer;
        }
      dstIP.Val=strAddr.sin_addr.s_addr;

      connect(s,(struct sockaddr*)&strAddr,sizeof(struct sockaddr_in));
      tOut=0;
      *(unsigned long*)internetBuffer=0;
      while(!*(unsigned long*)internetBuffer && tOut<SURF_TIMEOUT) {
        m2m_wifi_handle_events(NULL);
        tOut++;
        __delay_ms(1);
        }

      if(!*(unsigned long*)internetBuffer)
        goto close;
      
      sprintf(buf,"GET %s HTTP/1.1\r\nUser-Agent: Surf/2.0\r\nAccept: %s\r\n\r\n",
        file ? file : "/",tipo ? "*.*" : "*.*" /*fare!*/);   // Host, Accept
      send(s,buf,strlen(buf),0);
      tOut=0;
      *(unsigned long*)internetBuffer=0;
      while(!*(unsigned long*)internetBuffer && tOut<SURF_TIMEOUT) {
        m2m_wifi_handle_events(NULL);
        tOut++;
        __delay_ms(1);
        }

			recv(s,rxBuffer,1536,SURF_TIMEOUT);
			tOut=0;
			while(!socketData[0].dataAvail && tOut<SURF_TIMEOUT) {
				m2m_wifi_handle_events(NULL);
				tOut++;
				__delay_ms(1);
				}
      if(!socketData[0].dataAvail) 
        goto error_close;
      
			if(p=(char*)stristr(rxBuffer,tagConnection)) {
        connectionClose=!strnicmp(p+strlen(tagConnection)+1,"close",5);
        }
			if(p=(char*)stristr(rxBuffer,tagContentLength)) {
				bytes2Read=atoi(p+strlen(tagContentLength)+1);
        p=(char*)strstr(rxBuffer,"\r\n\r\n");   // https://serverfault.com/questions/862383/if-i-send-a-http-get-request-do-i-receive-the-response-in-get
        if(!p)
          goto error_close;
        
				n=0;

        socketData[0].dataAvail -= p-(char*)rxBuffer-4;
        n+=socketData[0].dataAvail;
        bytes2Read-=socketData[0].dataAvail;
				if(!SuperFileWrite(f,rxBuffer,socketData[0].dataAvail))
          goto error_close;
        socketData[0].dataAvail=0;
        
				while(bytes2Read>0) {

					recv(s,rxBuffer,1536,SURF_TIMEOUT);

					tOut=0;
					while(!socketData[0].dataAvail && tOut<SURF_TIMEOUT) {
						m2m_wifi_handle_events(NULL);
						tOut++;
						__delay_ms(1);
						}
					sprintf(buf,"download dati [%u]...",socketData[0].dataAvail);
          surfSetState(hWnd,3,buf);
          
					if(socketData[0].dataAvail>0) {
						n+=socketData[0].dataAvail;
						bytes2Read-=socketData[0].dataAvail;
						if(!SuperFileWrite(f,rxBuffer,socketData[0].dataAvail))
              break;
						socketData[0].dataAvail=0;
						}
          else
            break;
					}
				}
      else
        goto close;
      
			if(n>0) {
				}
      
			if(connectionClose) {   // 
error_close:        
close:
			  close(s);
				SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);		// inutile qua ma ok :)
				}	

      }
    }
#endif
  return n;
  }
int8_t surfNavigateStop(HWND hWnd) {
  SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
  surfSetState(hWnd,5,NULL);
  if(s != INVALID_SOCKET) {   // gestire wifi/ethernet
    close(s);
    SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);
    return 1;
    }
  return 0;
  }
static UGRAPH_COORD_T ySurfPos,xSurfPos,ySurfMax,xSurfMax;
struct HYPER_LINK {
	RECT rc;
	char text[64];
	BOOL visto;
	struct HYPER_LINK *next;
  };
int getSurfNParm(const char *s,const char *t,int n) {
	char *p;
	int i;

  if(!t) {
    p=(char*)s;
    goto noparm;
    }
	p=(char*)stristr(s,t);
	if(p) {
noparm:    
		while(*p != '=')
			p++;
		p++;
		p=(char*)skipSurfSpaces(p);
		if(isdigit(*p)) {
			return atoi(p);
			}
		else if(*p=='+') {
			p=skipSurfSpaces(p);
			return atoi(p)+n;
			}
		else if(*p=='-') {
			p=skipSurfSpaces(p);
			return n-atoi(p);
			}

		}
	return n;
  }
char *getSurfSParm(const char *s,const char *t,char *buf) {
	char *p;
	int i;

  if(!t) {
    p=(char*)s;
    goto noparm;
    }
	p=(char*)stristr(s,t);
	if(p) {
noparm:    
		while(*p != '=')
			p++;
		p++;
		p=(char*)skipSurfSpaces(p);
		if(isalnum(*p)) {
			do {
				*buf++=*p++;
				} while(*p != ' ');
			*buf=0;
			return buf;
			}
		else if(*p=='\"' || *p=='\'') {
      char ch=*p;
			p++;
			do {
				*buf++=*p++;
				} while(*p != ch);
			*buf=0;
			return buf;
			} 
    else 
      *buf=0; // safety diciamo
		}
	return 0;
  }
static GFX_COLOR getSurfColor(const char *s,const char *parm,GFX_COLOR def) {
  uint32_t c;
  char buf[32];

  if(getSurfSParm(s,parm,buf)) {
    if(*buf=='#')
      c=myhextoi(((char*)buf)+1);
    else
      c=myhextoi(buf);
    // e aggiungere altri modi, v. https://developer.mozilla.org/en-US/docs/Web/CSS/color
    return Color24_565(c);
    }
	else
		return def;
  }
static char *parseSurfItems(const char *p, struct HTMLinfo *h) {
	char myBuf[256],myBuf1[256];
	char ch;
	char *pCmd;
	BOOL bFlag=0,bExit=0,bExitW=0;
	BOOL bInTitle=0,bInHead=0,bInBody=0,bNegate=0;
	register int i,n;

	h->bkColor=BLACK;
	h->textColor=GRAY204;
	h->vlinkColor=LIGHTMAGENTA;
	h->hlinkColor=LIGHTCYAN;
	h->ulinkColor=h->textColor;
	*h->bkImage=0;
	*h->bkSound=0;
	*h->sDescription=0;
	*h->sGenerator=0;
	*h->sKeywords=0;
	*h->sTitle=0 /*GetPathName()*/;
	bExit=FALSE;
	do {
		bExitW=FALSE;
		n=0;
		do {
			ch=*p++;
			switch(ch) {
				case '<':
					if(n) {
						p--;
					  bExitW=TRUE;
					  }
				  else
						myBuf[n++]=ch;
					break;
				case '>':
				  myBuf[n++]=ch;
					if(myBuf[0] == '<')
						bExitW=TRUE;
					break;
				case 10:
					break;
				case 13:
					break;
				case 0:
					bExit=bExitW=TRUE;
					break;
				default:
				  myBuf[n++]=ch;
					break;
				}
		  } while(n<sizeof(myBuf) && !bExitW);
		myBuf[n]=0;

		if(myBuf[0] == '<') {
			if(myBuf[1] != '!') {
				if(myBuf[1] == '/') {
					bNegate=TRUE;
					pCmd=myBuf+2;
					}
				else {
					bNegate=FALSE;
					pCmd=myBuf+1;
					}
				if(!strnicmp(pCmd,"TITLE",5)) {
					bInTitle=!bNegate;
					}
				else if(!strnicmp(pCmd,"HEAD",4)) {
					bInHead=!bNegate;
					}
				else if(!strnicmp(pCmd,"BODY",4)) {
					bInBody=!bNegate;
					}
			  if(bInHead) {
					if(!strnicmp(pCmd,"META",4)) {
						if(getSurfSParm(pCmd,"NAME",myBuf1)) {
							if(!stricmp(myBuf1,"generator")) {
								getSurfSParm(pCmd,"CONTENT",myBuf1);
								strncpy(h->sGenerator,myBuf1,sizeof(h->sGenerator)-1);
                h->sGenerator[sizeof(h->sGenerator)-1]=0;
								}
							else if(!stricmp(myBuf1,"description")) {
								getSurfSParm(pCmd,"CONTENT",myBuf1);
								strncpy(h->sDescription,myBuf1,sizeof(h->sDescription));
								h->sDescription[sizeof(h->sDescription)-1]=0;
								}
							}
						}
          }
			  if(bInBody) {
          h->textColor=getSurfColor(pCmd,"TEXT",h->textColor);
          h->hlinkColor=getSurfColor(pCmd,"LINK",h->hlinkColor);
          h->vlinkColor=getSurfColor(pCmd,"VLINK",h->vlinkColor);
          h->ulinkColor=getSurfColor(pCmd,"ALINK",h->ulinkColor);   // VERIFICARE! dopo 27 anni non è chiaro come fosse :D
          h->bkColor=getSurfColor(pCmd,"BGCOLOR",h->bkColor);
          getSurfSParm(pCmd,"BACKGROUND",myBuf1);
          strncpy(h->bkImage,myBuf1,sizeof(h->bkImage)-1);
          h->bkImage[sizeof(h->bkImage)-1]=0;
					bInBody=0;
					}
				}
			}
		else {
			if(bInTitle)
				strcpy(h->sTitle,myBuf);

			}
		} while(!bExit);

	return (char*)p;
  }
struct HYPER_LINK *addToSurfLinks(struct HTMLinfo *myHtml,UGRAPH_COORD_T x, UGRAPH_COORD_T y, int cx, int cy, const char *s) {
	struct HYPER_LINK *hl,*hl1;

	hl=(struct HYPER_LINK *)malloc(sizeof(struct HYPER_LINK));
	hl->rc.top=y;
	hl->rc.left=x;
	hl->rc.bottom=y+cy;
	hl->rc.right=x+cx;
	strncpy(hl->text,s,sizeof(hl->text)-1);
	hl->text[sizeof(hl->text)-1]=0;
	hl->visto=0;
	hl->next=NULL;
	if(myHtml->link1) {
		hl1=myHtml->link1;
		while(hl1->next)
			hl1=hl1->next;
		hl1->next=hl;
		}
	else {
		myHtml->link1=hl;
		}
	return myHtml->link1;
  }
struct HYPER_LINK *deleteSurfLinks(struct HTMLinfo *myHtml,struct HYPER_LINK *l) {
	struct HYPER_LINK *hl,*hl1;

	hl1=myHtml->link1;
	while(hl1) {
		hl=hl1;
		hl1=hl1->next;
		free(hl);
		}

	myHtml->link1=NULL;
	return myHtml->link1;
	}
struct HYPER_LINK *getSurfLink(struct HTMLinfo *myHtml,POINT pt) {
	struct HYPER_LINK *hl=myHtml->link1;

	while(hl) {
		if(PtInRect(&hl->rc,pt)) {
			return hl;
			}
		hl=hl->next;
		}
	return NULL;
	}
static FONT setSurfFont(HDC hDC,BYTE family,BYTE size,int8_t bold,int8_t italic,int8_t underline,int8_t strikethrough) {
  FONT f;
  
  if(bold<0)
    bold=hDC->font.bold;
  if(italic<0)
    italic=hDC->font.italic;
  if(underline<0)
    underline=hDC->font.underline;
  if(strikethrough<0)
    strikethrough=hDC->font.strikethrough;
  
  switch(size) {
    case 1:
      f=CreateFont(6,4,0,0,bold ? FW_BOLD : FW_NORMAL, italic,underline,strikethrough, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      break;
    case 2:
    case 3:
      f=CreateFont(8,6,0,0,bold ? FW_BOLD : FW_NORMAL, italic,underline,strikethrough, ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
      break;
    case 4:
    case 5:
    case 6:
			size--;
      f=CreateFont(size*3,(size*3*3)/4,0,0,bold ? FW_BOLD : FW_NORMAL,italic,underline,strikethrough,ANSI_CHARSET,
        OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,VARIABLE_PITCH | FF_ROMAN,NULL);
      break;
    }
  return SelectObject(hDC,OBJ_FONT,(GDIOBJ)f).font;
  }
static void surfRender(HWND hWnd,HDC hDC,const char *pDoc,struct HTMLinfo *myHtml) {
	LOGBRUSH lb;
	char *p,*p1,myBuf[512],*pCmd;
	int i,j,n,x,y,ch;
	BYTE cFont=3,cOldFont=3;
  BYTE align=0;
	GFX_COLOR cColor,cOldColor;
	BOOL bExit=FALSE,bExitW=FALSE,bOK,bNegate=FALSE;
	BOOL bInLink=FALSE,bInFont=FALSE,bInP=FALSE,bInTitle=FALSE;
	char tLink[64]={0},lastch=0;
	FONT oldFont;
	BRUSH oldBrush;
	PEN oldPen;
	RECT rc;
	SIZE sz;

	myHtml->link1=deleteSurfLinks(myHtml,myHtml->link1);

	lb.lbStyle=BS_SOLID;
	lb.lbColor=myHtml->bkColor;
	lb.lbHatch=0;
	BRUSH Brush1=CreateBrushIndirect(&lb);
	PEN Pen1=CreatePen(PS_SOLID,1,myHtml->bkColor);
	GetClientRect(hWnd,&rc);
	oldPen=SelectObject(hDC,OBJ_PEN,(GDIOBJ)Pen1).pen;
	oldBrush=SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)Brush1).brush;
//	Rectangle(hDC,rc.left,rc.top,rc.right,rc.bottom);
	oldFont=setSurfFont(hDC,0,3,0,0,0,0);
	cColor=myHtml->textColor;
	SetTextColor(hDC,cColor);
	SetBkColor(hDC,myHtml->bkColor);
	p=(char*)pDoc;
	ySurfMax=xSurfMax=0;
	bOK=TRUE;
	x=0;
	y=0;
	do {
		bExitW=FALSE;
		n=0;
		do {
			ch=*p++;
			switch(ch) {
				case '<':
					if(n) {
						p--;
					  bExitW=TRUE;
					  }
				  else
						myBuf[n++]=ch;
					break;
				case '>':
				  myBuf[n++]=ch;
					if(myBuf[0] == '<')
						bExitW=TRUE;
					break;
				case '&':
					{char *p2=p++;
					while(isalnum(*p))
						p++;
					if(*p==';') {
						if(!strnicmp(p2,"nbsp",4)) {
							ch=' ';
							}
						else if(!strnicmp(p2,"copy",4)) {
							ch='@';		// mettere...
							}
						else if(!strnicmp(p2,"agrave",5)) {
							ch='à';
							}
						else if(!strnicmp(p2,"egrave",5)) {
							ch='è';
							}
						else if(!strnicmp(p2,"eacute",5)) {
							ch='é';
							}
						else if(!strnicmp(p2,"igrave",5)) {
							ch='ì';
							}
						else if(!strnicmp(p2,"iacute",5)) {
							ch='ì';
							}
						else if(!strnicmp(p2,"ccedil",5)) {
							ch='ç';
							}
						
						myBuf[n++]=ch;
						p++;
						}
					}
					break;
				case 10:
				  if(lastch == 13)
  					break;
				case 13:
				  if(n)
					  myBuf[n++]=' ';
					break;
				case 0:
					bExit=bExitW=TRUE;
					break;
				default:
				  myBuf[n++]=ch;
					break;
				}
			lastch=ch;
		  } while(n<sizeof(myBuf) && !bExitW);
		myBuf[n]=0;
		if(myBuf[0] == '<') {
			if(myBuf[1] != '!') {
				if(myBuf[1] == '/') {
					bNegate=TRUE;
					pCmd=myBuf+2;
					}
				else {
					bNegate=FALSE;
					pCmd=myBuf+1;
					}
				if(!strnicmp(pCmd,"BR",2)) {
					ySurfMax+=getFontHeight(&hDC->font);
					if(ySurfMax>=ySurfPos)
						y+=getFontHeight(&hDC->font);
					x=1;
					}
				else if(!strnicmp(pCmd,"TITLE",5)) {
					bInTitle=!bNegate;
					}
				else if(!strnicmp(pCmd,"TABLE",5)) {
					}
				else if(!strnicmp(pCmd,"HTML",4)) {
					}
				else if(!strnicmp(pCmd,"HEAD",4)) {
					}
				else if(!strnicmp(pCmd,"FONT",4)) {
					if(bNegate) {
						cFont=cOldFont;
						cColor=cOldColor;
            }
					else {
						cOldFont=cFont;
						cFont=getSurfNParm(pCmd,"size",cOldFont);
						if(cFont < 0)
							cFont=0;
						if(cFont > 6)
							cFont=3;
          	oldFont=setSurfFont(hDC,0,cFont,-1,-1,-1,-1);
						cOldColor=cColor;
						cColor=getSurfColor(pCmd,"color",cColor);
						}
					}
				else if(!strnicmp(pCmd,"BLINK",5)) {
					if(bNegate)
            ;
          else
            ;
					}
				else if(!strnicmp(pCmd,"CENTER",6)) {
					if(bNegate)
            align=0;
          else
#warning RIMETTERE!            align=1;
            ;
					}
				else if(toupper(*pCmd)=='H' && isdigit(*(pCmd+1))) {
					if(bNegate)
						cFont=cOldFont;
					else {
						cOldFont=cFont;
						cFont=7-atoi(pCmd+1);
						if(cFont < 0)
							cFont=0;
						if(cFont > 6)
							cFont=6;
          	oldFont=setSurfFont(hDC,0,cFont,-1,-1,-1,-1);
						}
					}
				else if(toupper(*pCmd)=='I' && !isalnum(*(pCmd+1))) {
         	oldFont=setSurfFont(hDC,0,cFont,-1,!bNegate,-1,-1);
					}
				else if(toupper(*pCmd)=='B' && !isalnum(*(pCmd+1))) {
         	oldFont=setSurfFont(hDC,0,cFont,!bNegate,-1,-1,-1);
					}
				else if(!strnicmp(pCmd,"PRE",3)) {
					}
				else if(!strnicmp(pCmd,"IMG",3)) {
					if(1 /*broken image*/) {
						drawIcon8(hDC,x,y+12+2,surfImage);
						x+=10;
						}
					}
				else if(!strnicmp(pCmd,"LI",2)) {
          myBuf[0]='*'; myBuf[1]=0;   // finire...
          goto putchar;
					}
				else if(!strnicmp(pCmd,"A",1)) {
					bInLink=!bNegate;
					if(bInLink)
						getSurfSParm(pCmd,"HREF",tLink);
         	oldFont=setSurfFont(hDC,0,cFont,-1,-1,bInLink,-1);
					}
				else if(!strnicmp(pCmd,"P",1)) {
          char buf[16];
					if(getSurfSParm(pCmd,"align",buf))
            align=0;      // finire
          
					ySurfMax+=getFontHeight(&hDC->font);
					if(ySurfMax>=ySurfPos)
						y+=getFontHeight(&hDC->font);
					x=1;
					}
				}
		  }
		else if(bInTitle) {
			}
		else {
putchar:      
			j=strlen(myBuf);
			GetTextExtentPoint(hDC,myBuf,j,&sz);
			if((x+sz.cx) > (rc.right-rc.left)) {
				ySurfMax+=sz.cy;
				x=1;
				if(ySurfMax>=ySurfPos) 
					y+=sz.cy;
				}
			if(ySurfMax>=ySurfPos) {
//        RECT rc2;
//        rc2.left=x; rc2.top=y+12;
//        rc2.right=rc.right; rc2.bottom=y+50 /*non importa*/;
 /*usare return come sz.cy?boh*/				
//        DrawText(hDC,myBuf,-1,&rc2,
//          DT_SINGLELINE | DT_NOPREFIX | /*(align ? (align==1 ? DT_CENTER : DT_RIGHT) : mi fotte gli hyperlink... )*/DT_LEFT);
        if(bInLink) {
          SetTextColor(hDC,0 /*hl->visto */ ? myHtml->vlinkColor : myHtml->hlinkColor);
          }
        else
          SetTextColor(hDC,cColor);
        switch(align) {
          case 0:
            TextOut(hDC,x,y+12+2,myBuf,j);
            if(bInLink) {
              myHtml->link1=addToSurfLinks(myHtml,x,y+12+2,sz.cx,sz.cy,tLink);
              }
            break;
          case 1:
            TextOut(hDC,(rc.right-x)/2,+12+2,myBuf,j);
            if(bInLink) {
              myHtml->link1=addToSurfLinks(myHtml,(rc.right-x)/2,y+12+2,sz.cx,sz.cy,tLink);
              }
            break;
          case 2:
            TextOut(hDC,rc.right-x,y+12+2,myBuf,j);
            if(bInLink) {
              myHtml->link1=addToSurfLinks(myHtml,rc.right-x,y+12+2,sz.cx,sz.cy,tLink);
              }
            break;
          }
				}
			x+=sz.cx;
      if(x>xSurfMax)
        xSurfMax=x;
			}
//		if(y > rc.bottom-rc.top)			// questo incasina il conto di yMax...
//			bExit=1;
		} while(!bExit);

//	wsprintf(myBuf,"ypos=%d, ymax=%d",yPos,yMax);
//	AfxMessageBox(myBuf);

  y=(rc.bottom-rc.top);
  if(ySurfMax>=y) {
    EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
    SetScrollRange(hWnd,SB_VERT,0,1+ySurfMax-y,FALSE);
    SetScrollPos(hWnd,SB_VERT,ySurfPos,TRUE);
    }
  else {
    EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
    SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
    SetScrollPos(hWnd,SB_VERT,0,TRUE);
    }
  x=(rc.right-rc.left);
  if(xSurfMax>=x) {
    EnableScrollBar(hWnd,SB_HORZ,ESB_ENABLE_BOTH);   //
    SetScrollRange(hWnd,SB_HORZ,0,1+xSurfMax-x,FALSE);
    SetScrollPos(hWnd,SB_HORZ,xSurfPos,TRUE);
    }
  else {
    EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //
    SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
    SetScrollPos(hWnd,SB_HORZ,0,TRUE);
    }
	SelectObject(hDC,OBJ_FONT,(GDIOBJ)oldFont);
  SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)oldBrush);
  SelectObject(hDC,OBJ_PEN,(GDIOBJ)oldPen);
  DeleteObject(OBJ_BRUSH,(GDIOBJ)Brush1);
	DeleteObject(OBJ_PEN,(GDIOBJ)Pen1);
  }
static void surfRenderSource(HWND hWnd,HDC hDC,const char *pDoc) {
  char *p=(char*)pDoc,buf[2];
  struct HTMLinfo *myHtml=(struct HTMLinfo *)GET_WINDOW_OFFSET(hWnd,0);
  int n=myHtml->fLen;
  UGRAPH_COORD_T x=0,y=0;
  BYTE inTag=0;
	RECT rc;
  
	GetClientRect(hWnd,&rc);
  
	ySurfMax=xSurfMax=0;
  SetTextColor(hDC,WHITE);
  while(n--) {
    switch(*p) {
      case 13:
        break;
      case 10:
        x=0;
        y+=getFontHeight(&hDC->font);
        break;
      case 0:
        goto fine;
        break;
      case '<':   // vabbe' :) per cominciare
        inTag=1;
      case '>':
      default:
        SetTextColor(hDC,inTag ? LIGHTRED : WHITE);
        buf[0]=*p; buf[1]=0;
        TextOut(hDC,x,y+12+2,buf,strlen(buf));
        x+=getFontWidth(&hDC->font);
        if(x>xSurfMax)
          xSurfMax=x;
        break;
      }
    if(*p=='>')
      inTag=0;
    p++;
    }
fine:
    ;

  ySurfMax=y;
  y=(rc.bottom-rc.top);
  if(ySurfMax>=y) {
    EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
    SetScrollRange(hWnd,SB_VERT,0,1+ySurfMax-y,FALSE);
    SetScrollPos(hWnd,SB_VERT,ySurfPos,TRUE);
    }
  else {
    EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
    SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
    SetScrollPos(hWnd,SB_VERT,0,TRUE);
    }
  x=(rc.right-rc.left);
  if(xSurfMax>=x) {
    EnableScrollBar(hWnd,SB_HORZ,ESB_ENABLE_BOTH);   //
    SetScrollRange(hWnd,SB_HORZ,0,1+xSurfMax-x,FALSE);
    SetScrollPos(hWnd,SB_HORZ,xSurfPos,TRUE);
    }
  else {
    EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //
    SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
    SetScrollPos(hWnd,SB_HORZ,0,TRUE);
    }
  }
LRESULT surfWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  struct HTMLinfo *myHtml=(struct HTMLinfo *)GET_WINDOW_OFFSET(hWnd,0);
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int i;
      BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
      RECT rc;
      GetClientRect(hWnd,&rc);

      hDC->pen=CreatePen(1,1,LIGHTGREEN);
      hDC->brush=CreateSolidBrush(GREEN);
      SetTextColor(hDC,LIGHTGREEN);
      
      TextOut(hDC,2,2,"Indirizzo:",10);
      drawHorizLineWindow(hDC,0,12,rc.right);

      rc.top+=14; rc.bottom-=24;
      
      if(myHtml->fLen>0) {
        
#ifdef USA_WIFI
				char *pagePtr=GET_WINDOW_OFFSET(hWnd,sizeof(struct HTMLinfo));
        if(GetWindowByte(hWnd,GWL_USERDATA+0) & 2)
          surfRenderSource(hWnd,hDC,pagePtr);
        else
          surfRender(hWnd,hDC,pagePtr,myHtml);
#endif
// finire        internetBufferLen=0;
      
        }
      
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
        fillRectangleWindowColorRect(hDC,&hWnd->paintArea,myHtml->bkColor);
      }
      return 1;
      break;
    case WM_CTLCOLOR:
      {HDC hDC;
      DC myDC;
      hDC=GetDC(hWnd,&myDC);
      if((void*)lParam==GetDlgItem(hWnd,204))   // la status bar, meglio
        return GRAY192;
      else
        return myDC.brush.color;
      ReleaseDC(hWnd,hDC);
      }
      break;
      
    case WM_TIMER:
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      char *url=(char *)cs->lpCreateParams;   // usare, salvare...
      BYTE attrib=GetProfileInt(surfApp,"attributi",1);
      SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
      SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // tipo socket
      SetWindowByte(hWnd,GWL_USERDATA+3,0);   //stato
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      CreateWindow(MAKECLASS(WC_EDIT),"http://",WS_BORDER | WS_VISIBLE | WS_CHILD | 
        0,
        64,1,cs->cx-64-36-12-SCROLL_SIZE-1,9,
        hWnd,(HMENU)201,NULL
        );
      CreateWindow(MAKECLASS(WC_BUTTON),"Vai",WS_BORDER | WS_VISIBLE | WS_TABSTOP |
        WS_CHILD | BS_CENTER | BS_PUSHBUTTON,
        cs->cx-32-12-SCROLL_SIZE-1,1,30,9,
        hWnd,(HMENU)202,NULL
        );
      CreateWindow(MAKECLASS(WC_STATIC),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD | 
        /*WS_DISABLED | */ SS_ICON,
        cs->cx-12-SCROLL_SIZE-1,1,8,10,
        hWnd,(HMENU)203,NULL
        );
      SendMessage(GetDlgItem(hWnd,203),STM_SETICON,(WPARAM)surfIcon,0);
			if(attrib & 1) {   // statusbar
        CreateWindow(MAKECLASS(WC_STATUSBAR),"idle",WS_VISIBLE | WS_CHILD
           /*| SS_ICONs */,
          0,
          cs->cy-8,
          cs->cx,cs->cy,    // se thickborder deve andare più in giù e + larga, pare CMQ SI AUTOGESTISCE!
          hWnd,(HMENU)204,NULL
          );
				}
      memset(myHtml,0,sizeof(struct HTMLinfo));
      myHtml->bkColor=windowBackColor;
      if(url) {   // non so se è bene farlo da qua...
        SetWindowText(GetDlgItem(hWnd,201),url);   // appunto!
        surfNavigate(hWnd,url,NULL,0);
        }
      // o andare in homepage, da WIN.INI?
      SetTimer(hWnd,1,1000,NULL); //per animazione icona, timeouttimeout?
      }
      return 0;
      break;
    case WM_DESTROY:
			{SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
		  if(s != INVALID_SOCKET) {   // gestire wifi/ethernet
			  close(s);
				SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);		// inutile qua ma ok :)
				}	
      KillTimer(hWnd,1);
//      m_WndSurf=NULL;
//      DestroyWindow(hWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
			}
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_RETURN:     // mmm credo che arrivi da notify del control EDIT..
          if(SendMessage(GetDlgItem(hWnd,201),WM_GETTEXTLENGTH,0,0) > 0)
            PostMessage(hWnd,WM_COMMAND,MAKELONG(202,BN_CLICKED),0);
          break;
        case VK_BROWSER_BACK:
          break;
        case VK_BROWSER_FORWARD:
          break;
        case VK_BROWSER_REFRESH:
        case VK_F5:
keyboard_reload:
           {SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
            if(s != INVALID_SOCKET) {   // gestire wifi/ethernet
              close(s);
              SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);
              }
            surfNavigate(hWnd,NULL,NULL,0);
            }
          break;
        case VK_BROWSER_STOP:
keyboard_stop:
          surfNavigateStop(hWnd);
          break;
        case VK_BROWSER_SEARCH:
          PostMessage(hWnd,WM_COMMAND,MAKELONG(96+3,0),0);
          break;
        case VK_BROWSER_FAVORITES:
          break;
        case VK_BROWSER_HOME:
          {
          char buf[32],*homepage=buf;
keyboard_home:
          GetProfileString(surfApp,"homepage","192.168.1.2",buf,31);
          PostMessage(hWnd,WM_SURF_NAVIGATE,0,(LPARAM)homepage);
          }
          break;
        case VK_ESCAPE:     // o accelerator??
          goto keyboard_stop;
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) {
          case 2:    // open url
            {
            if(DialogBox((HINSTANCE)NULL,&fileChooserDlg,hWnd,(WINDOWPROC*)DefWindowProcFileDlgWC)) {
//            myWnd=(HWND)GetWindowLong(hWnd,0);
//            GetWindowText(myWnd,buf,31);
              }
            }
            break;
          case 3:    // open file
            if(DialogBox((HINSTANCE)NULL,&fileChooserDlg,hWnd,(WINDOWPROC*)DefWindowProcFileDlgWC)) {
//      myWnd=(HWND)GetWindowLong(hWnd,0);
//      GetWindowText(myWnd,buf,31);
                
              }
            break;
          case 4:    // save file FINIRE
            {SUPERFILE f;
            f.drive=currDrive;
            if(myHtml->fLen>0) {
              if(SuperFileOpen(&f,"index.htm",OPEN_WRITE,TYPE_TEXT | SHARE_READ)) {
                SuperFileWrite(&f,(char*)GET_WINDOW_OFFSET(hWnd,sizeof(struct HTMLinfo)),myHtml->fLen);
                SuperFileClose(&f);
                MessageBox(hWnd,"file index salvato",surfApp,MB_OK | MB_ICONINFORMATION);
                }
              }
//            if(DialogBox((HINSTANCE)NULL,&fileChooserDlg,hWnd,(WINDOWPROC*)DefWindowProcFileDlgWC)) {
//      myWnd=(HWND)GetWindowLong(hWnd,0);
//      GetWindowText(myWnd,buf,31);
                
//              }
            }
          case 7:    // esci
            PostMessage(hWnd,WM_CLOSE,0,0);
            break;
          case 32+1:    // reload
            goto keyboard_reload;
            break;
          case 32+5:    // mostra html
            {BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0) ^ 2;
            SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
            InvalidateRect(hWnd,NULL,TRUE);
            }
            break;
          case 48+3:    // home
            goto keyboard_home;
            break;
          case 48+4:    // stop
            {SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
            if(s != INVALID_SOCKET) {
              close(s);
              SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);
              SetWindowByte(hWnd,GWL_USERDATA+3,4);
              InvalidateRect(hWnd,NULL,TRUE);
              }
            }
            break;
          case 80+6:    // statusbar
            {BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0) ^ 1;
            SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
            WriteProfileInt(surfApp,"attributi",attrib);
            // creare/distruggere finestra status..
            InvalidateRect(hWnd,NULL,TRUE);
            }
            break;
          case 96+2:    // home adpm
            SendMessage(hWnd,WM_SURF_NAVIGATE,0,(LPARAM)"http://cyberdyne.biz.ly");
            break;
          case 96+3:    // google
            SendMessage(hWnd,WM_SURF_NAVIGATE,0,(LPARAM)"http://www.google.com");
            break;
          case 128+2:    // info
            MessageBox(hWnd,"Versione 2.0 per Breakthrough",
              "Informazioni su Surf!",MB_OK | MB_ICONINFORMATION);
            break;
          }
        }
      if(HIWORD(wParam) == BN_CLICKED) {   // è 0!! coglioni :D
        switch(LOWORD(wParam)) { 
          case 202:   // vai!
            {
            char url[64];
            GetWindowText(GetDlgItem(hWnd,201),url,31);
            surfNavigate(hWnd,url,NULL,0);
            
//            surfNavigate(hWnd,"192.168.1.2",NULL,0);
            
            }
            break;
          }
        }
      return 1;
      break;
      
    case WM_SIZE:
      {HWND myWnd;
      MoveWindow(GetDlgItem(hWnd,201),64,1,LOWORD(lParam)-64-36-12,9,TRUE);  // cmq EDIT si autogestisce altezza
      SetWindowPos(GetDlgItem(hWnd,202),NULL,LOWORD(lParam)-32-12,1,0,0,SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
      SetWindowPos(GetDlgItem(hWnd,203),NULL,LOWORD(lParam)-12,1,0,0,SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);   //idem icona
      myWnd=GetDlgItem(hWnd,204);
      if(myWnd)
      //  MoveWindow(myWnd,0,HIWORD(lParam)-8-1,LOWORD(lParam),8+1,TRUE);// cmq si autogestisce! 
        {
        WINDOWPOS wpos;
        wpos.hwnd=hWnd;
        wpos.hwndInsertAfter=NULL;
        wpos.x=hWnd->nonClientArea.left;
        wpos.y=hWnd->nonClientArea.top;
        wpos.cx=LOWORD(lParam);
        wpos.cy=HIWORD(lParam);
        wpos.flags=SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW;
				SendMessage(myWnd,WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
        }
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_MOUSEMOVE:
      {POINT pt={LOWORD(lParam),HIWORD(lParam)};
      struct HYPER_LINK *l;
      HWND myWnd=GetDlgItem(hWnd,204);
      
//      pt.x-=hWnd->clientArea.left; pt.y-=hWnd->clientArea.top;
      //ScreenToClient(hWnd,&pt);
      if(/* !hWnd->internalState anche && */ SendMessage(hWnd,WM_NCHITTEST,0,lParam) == HTCLIENT) {
        l=getSurfLink(myHtml,pt);
        if(l) {
          if(myWnd)
            SetWindowText(myWnd,l->text);
          SetCursor((CURSOR)&handCursorSm);
          }
        else {
          surfGetState(hWnd);
          switch(GetWindowByte(hWnd,GWL_USERDATA+3)) {
            case 0:   // IDLE
              SetCursor((CURSOR)&standardCursorSm);
              if(myWnd)
                SetWindowText(myWnd,"fatto.");		// rimettere la stringa di stato... salvarla, calcolarla
              break;
            case 1:   // connecting
              SetCursor((CURSOR)&hourglassCursorSm);
              break;
            case 2:   // connesso
              SetCursor((CURSOR)&standardCursorSm);
              break;
            case 3:   // download
              SetCursor((CURSOR)&hourglassCursorSm);
              break;
            case 4:   // finito
            case 5:   // 
              SetCursor((CURSOR)&standardCursorSm);
              if(myWnd)
                SetWindowText(myWnd,"idle");		// rimettere la stringa di stato... salvarla, calcolarla
              break;
            }
          }
        }   // HITTEST
      else
        SetCursor((CURSOR)&standardCursorSm);
      }
      return 0;
      break;
    case WM_SETCURSOR:
      return 1;   // non è perfetto, mancano i cursori "di sistema" tipo cross ecc...
      break;
      
    case WM_LBUTTONDOWN:
      {POINT pt={LOWORD(lParam),HIWORD(lParam)};
      struct HYPER_LINK *l;

      l=getSurfLink(myHtml,pt);
      if(l) {
        surfNavigate(hWnd,l->text,NULL,0);
        l->visto=TRUE;
        }
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
  
    case WM_RBUTTONDOWN:    // 
      {POINT pt={LOWORD(lParam),HIWORD(lParam)};
      struct HYPER_LINK *l;
      SUPERFILE f;
      char buf[64];
      
      l=getSurfLink(myHtml,pt);
      if(l) {
        char *p,*p2,*p3;
        p=p3=l->text;
        do {    // cerco il nome del file al fondo del link, o uso il link se non c'è altro
          p2=p3;
          p3=strchr(p,'/');
          if(!p3)
            p3=strchr(p,'\\');
          if(p3) {
            p3++;
            p=p3;
            }
          } while(p3);
        if(p2) {
//          p2++;
          f.drive=currDrive;
          if(SuperFileOpen(&f,p2,OPEN_WRITE,TYPE_BINARY | SHARE_READ)) {
            surfNavigateDownload(hWnd,&f,l->text,p2,0); // credo si autogestisca il "file"
            SuperFileClose(&f);
            sprintf(buf,"file %s scaricato",p2);
            MessageBox(hWnd,buf,surfApp,MB_OK | MB_ICONINFORMATION);
            }
          }
// qua no        l->visto=TRUE;
//        break;
        }
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_CONTEXTMENU:
      {char buf[32];
#ifdef USA_WIFI
      sprintf(buf,"pagina: %u bytes",myHtml->fLen);
#endif
      MessageBox(hWnd,buf,surfApp,MB_OK | MB_ICONINFORMATION);
      }
      break;
    case WM_VSCROLL:
      {RECT rc;
      int i;
      BYTE fOK=0;
      GetClientRect(hWnd,&rc);
      i=rc.bottom-rc.top;
      switch(LOWORD(wParam)) {
        case SB_LINEDOWN:
          if(ySurfPos<ySurfMax) {
            ySurfPos+=16;
            fOK=TRUE;
            }
          break;
        case SB_LINEUP:
          if(ySurfPos>16) {
            ySurfPos-=16;
            fOK=TRUE;
            }
          break;
        case SB_PAGEDOWN:
          if(ySurfPos<(ySurfMax-i-i)) {
            ySurfPos+=i;
            fOK=TRUE;
            }
          else if(ySurfPos<(ySurfMax-i)) {
            ySurfPos=ySurfMax-i;
            fOK=TRUE;
            }
          break;
        case SB_PAGEUP:
          if(ySurfPos>i) {
            ySurfPos-=i;
            fOK=TRUE;
            }
          else if(ySurfPos>0) {
            ySurfPos=0;
            fOK=TRUE;
            }
          break;
        case SB_THUMBPOSITION:
          ySurfPos=HIWORD(wParam);
          fOK=TRUE;
          break;
        }
      if(fOK)
        InvalidateRect(hWnd,NULL,TRUE);
      }
			break;
    case WM_HSCROLL:
      break;
      
    case WM_SURF_NAVIGATE:
      {
      char *url=(char *)lParam;   // 
      if(url) {
        SetWindowText(GetDlgItem(hWnd,201),url);
        surfNavigate(hWnd,url,NULL,0);
        ShowWindow(hWnd,SW_SHOW);
        }
      }
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, 
  unsigned char *pBytes_actually_read, void *pCallback_data) {
  int32_t n;
  DWORD *myjpegLen=(DWORD*)(pCallback_data+4);

  if(*myjpegLen == 0xffffffff) {
    SUPERFILE *myJpegFile=(*(SUPERFILE**)pCallback_data);
    n=SuperFileRead(myJpegFile,pBuf,buf_size);
    if(n<=0)
      return PJPG_STREAM_READ_ERROR;
    *pBytes_actually_read = (unsigned char)n;
    }
  else {
    BYTE **myjpegPtr=((BYTE **)pCallback_data);
    n = min(*myjpegLen, buf_size);
    if(n != buf_size)
      return PJPG_STREAM_READ_ERROR;
    memcpy(pBuf,*myjpegPtr,n);
    *pBytes_actually_read = (unsigned char)n;
    *myjpegPtr += n;
    *myjpegLen -= n;
    }
  return 0;
  }

int drawJPGfile(HDC hDC,const char *filename,GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T *xs, GRAPH_COORD_T *ys,int8_t zoom) {
  GRAPH_COORD_T img_ofs_x,img_ofs_y;
  SUPERFILE f;
  
  if(SuperFileOpen(&f,filename,OPEN_READ,TYPE_BINARY | SHARE_READ)) {  
    int x,x1,mcu_x;
    int y,y1,mcu_y;
    pjpeg_image_info_t JPG_Info;
    BOOL status;
    DWORD cback_data[2];
    cback_data[0]=(DWORD)&f;      // riciclo la struttura del desktop!
    cback_data[1]=0xffffffff;

    if(!(status=pjpeg_decode_init(&JPG_Info,pjpeg_need_bytes_callback,
      &cback_data,0))) {
      img_ofs_y=JPG_Info.m_height;
      img_ofs_x=JPG_Info.m_width;
      if(zoom<0) for(x=0; x>zoom; x--) {
        img_ofs_y /= 2;
        img_ofs_x /= 2;
        }
      else if(zoom>0) for(x=0; x<zoom; x++) {
        img_ofs_y *= 2;
        img_ofs_x *= 2;
        }

      img_ofs_y=((int)(ys)-(int)img_ofs_y)/2;
      img_ofs_x=((int)(xs)-(int)img_ofs_x)/2;
      if(img_ofs_y<0) img_ofs_y=0;
      if(img_ofs_x<0) img_ofs_x=0;

      mcu_x=0; mcu_y=0;
      for(;;) {
        if(status = pjpeg_decode_mcu())
          goto error_compressed;

        for(y=0; y < JPG_Info.m_MCUHeight; y += 8) {
          y1=(mcu_y*JPG_Info.m_MCUHeight) + y;
          for(x=0; x < JPG_Info.m_MCUWidth; x += 8) {
            x1=(mcu_x*JPG_Info.m_MCUWidth) + x  /* * JPG_Info.m_comps*/;

            // Compute source byte offset of the block in the decoder's MCU buffer.
            uint32_t src_ofs = (x*8) + (y*16);
            const uint8_t *pSrcR = JPG_Info.m_pMCUBufR + src_ofs;
            const uint8_t *pSrcG = JPG_Info.m_pMCUBufG + src_ofs;
            const uint8_t *pSrcB = JPG_Info.m_pMCUBufB + src_ofs;

            uint8_t bx,by,zcntx,zcnty;
            zcnty=(-zoom)+1;
            for(by=0; by<8; by++) {
              zcntx=(-zoom)+1;
              for(bx=0; bx<8; bx++) {
                if(zoom==0) {
                  if(hDC)
                    drawPixelWindowColor(hDC,img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                  else
                    DrawPixel(img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                  }
                else if(zoom>0) {
                  if(hDC)
                    fillRectangleWindowColor(hDC,img_ofs_x+(x1+bx)*(zoom+1),img_ofs_y+(y1+by)*(zoom+1),
                      img_ofs_x+(x1+bx)*(zoom+1)+bx*(zoom+1),img_ofs_y+(y1+by)*(zoom+1)+by*(zoom+1),Color565(*pSrcR,*pSrcG,*pSrcB));
                  else
                    DrawRectangle(img_ofs_x+(x1+bx)*(zoom+1),img_ofs_y+(y1+by)*(zoom+1),
                      img_ofs_x+(x1+bx)*(zoom+1)+bx*(zoom+1),img_ofs_y+(y1+by)*(zoom+1)+by*(zoom+1),Color565(*pSrcR,*pSrcG,*pSrcB));
                  }
                else if(zoom<0) {
                  zcntx--;
                  if(!zcntx) {
                    if(hDC)
                      drawPixelWindowColor(hDC,img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                    else
                      DrawPixel(img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                    zcntx=(-zoom)+1;
                    // e plotto.. FINIRE! bisogna tener traccia dei pixel plottati xPixels ecc
                    // fare su y
                    }
                  }
                pSrcR++; pSrcG++; pSrcB++;
                }
              }
            }
          }

        mcu_x++;      // in x ogni blocco è già 16 pixel (con YUV, pare)
        if(mcu_x >= JPG_Info.m_MCUSPerRow  /* e anche ps.rcPaint.right ecc*/) {
          mcu_x=0;
          mcu_y++;
          if(mcu_y>*ys)
            break;
          }
        ClrWdt();
        }
      }
error_compressed:

    *xs=JPG_Info.m_width;
    *ys=JPG_Info.m_height;
    SuperFileClose(&f);
    return 1;
    }
  return 0;
  }
          
int drawBMPfile(HDC hDC,const char *filename,GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T *xs, GRAPH_COORD_T *ys,int8_t zoom) {
  DWORD palette[256];
  struct BITMAPFILEHEADER bfi;
  struct BITMAPINFOHEADER bi;
  SUPERFILE f;
  UGRAPH_COORD_T x,x2;

  if(SuperFileOpen(&f,filename,OPEN_READ,TYPE_BINARY | SHARE_READ)) {  
    if(SuperFileRead(&f,(char*)&bfi,sizeof(struct BITMAPFILEHEADER))==sizeof(struct BITMAPFILEHEADER) &&
      SuperFileRead(&f,(char*)&bi,sizeof(struct BITMAPINFOHEADER))==sizeof(struct BITMAPINFOHEADER)) {
      if(bi.biBitCount==4 || bi.biBitCount==8)
        SuperFileRead(&f,(char*)&palette,4*(1 << bi.biBitCount));
      while(--bi.biHeight) {
        x2=bi.biWidth;
        x=0;
        while(x2--) {
          switch(bi.biBitCount) {
            case 1:
              {BYTE c,j;
              SuperFileRead(&f,&c,1);
              for(j=0x80; j; j>>1) {
                if(zoom==0) {
                  if(hDC)
                    drawPixelWindowColor(hDC,x++,bi.biHeight,c & j ? WHITE : BLACK);
                  else
                    DrawPixel(x++,bi.biHeight,c & j ? WHITE : BLACK);
                  }
                else if(zoom>0) {
//finire con Y                      fillRectangleWindowColor(hDC,x*(z+1),bi.biHeight- y*(z+1),
//                        x*(z+1)+bx*(z+1),bi.biHeight- y*(z+1)  +by*(z+1),Color24To565(c));
                  if(hDC)
                    ;
                  else
                    ;
                  x+=zoom+1;
                  }
                else if(zoom<0) {
                  }
                }
              x2-=7;
              }
              break;
            case 4:
              {BYTE c,j,c2;
              SuperFileRead(&f,&c2,1);
              for(j=0; j<2; j++) {
                c= j ? c2 >> 4 : c2 & 0xf;
                if(zoom==0) {
                  if(hDC)
                    drawPixelWindowColor(hDC,x++,bi.biHeight,Color24To565(palette[c]));
                  else
                    DrawPixel(x++,bi.biHeight,Color24To565(palette[c]));
                  }
                else if(zoom>0) {
                  if(hDC)
//finire con Y                      fillRectangleWindowColor(hDC,x*(z+1),bi.biHeight- y*(z+1),
//                        x*(z+1)+bx*(z+1),bi.biHeight- y*(z+1)  +by*(z+1),Color24To565(c));
                    ;
                  else
                    ;
                  x+=zoom+1;
                  }
                else if(zoom<0) {
                  }
                }
              x2--;
              }
              break;
            case 8:
              {BYTE c;
              SuperFileRead(&f,&c,1);
              if(zoom==0) {
                if(hDC)
                  drawPixelWindowColor(hDC,x++,bi.biHeight,Color24To565(palette[c]));
                else
                  DrawPixel(x++,bi.biHeight,Color24To565(palette[c]));
                }
              else if(zoom>0) {
//finire con Y                      fillRectangleWindowColor(hDC,x*(z+1),bi.biHeight- y*(z+1),
//                        x*(z+1)+bx*(z+1),bi.biHeight- y*(z+1)  +by*(z+1),Color24To565(c));
                if(hDC)
                  ;
                else
                  ;

                x+=zoom+1;
                }
              else if(zoom<0) {
                }
              }
              break;
            case 16:
              {GFX_COLOR c;
              SuperFileRead(&f,(BYTE*)&c,2);
              if(zoom==0) {
                if(hDC)
                  drawPixelWindowColor(hDC,x++,bi.biHeight,c);
                else
                  DrawPixel(x++,bi.biHeight,c);
                }
              else if(zoom>0) {
//finire con Y                      fillRectangleWindowColor(hDC,x*(z+1),bi.biHeight- y*(z+1),
//                        x*(z+1)+bx*(z+1),bi.biHeight- y*(z+1)  +by*(z+1),Color24To565(c));
                if(hDC)
                  ;
                else
                  ;
                x+=zoom+1;
                }
              else if(zoom<0) {
                }
              }
              break;
            case 24:
              {COLORREF c;
              SuperFileRead(&f,(BYTE*)&c,3);
              if(zoom==0) {
                if(hDC)
                  drawPixelWindowColor(hDC,x++,bi.biHeight,Color24To565(c));
                else
                  DrawPixel(x++,bi.biHeight,Color24To565(c));
                }
              else if(zoom>0) {
//finire con Y                      fillRectangleWindowColor(hDC,x*(z+1),bi.biHeight- y*(z+1),
//                        x*(z+1)+bx*(z+1),bi.biHeight- y*(z+1)  +by*(z+1),Color24To565(c));
                if(hDC)
                  ;
                else
                  ;
                x+=zoom+1;
                }
              else if(zoom<0) {
                }
              }
              break;
            case 32:
              {COLORREF c;
              SuperFileRead(&f,(BYTE*)&c,4);
              c &= 0x00ffffff;      // alpha??
              if(zoom==0) {
                if(hDC)
                  drawPixelWindowColor(hDC,x++,bi.biHeight,Color24To565(c));
                else
                  DrawPixel(x++,bi.biHeight,Color24To565(c));
                }
              else if(zoom>0) {
//finire con Y                      fillRectangleWindowColor(hDC,x*(z+1),bi.biHeight- y*(z+1),
//                        x*(z+1)+bx*(z+1),bi.biHeight- y*(z+1)  +by*(z+1),Color24To565(c));
                if(hDC)
                  ;
                else
                  ;
                x+=zoom+1;
                }
              else if(zoom<0) {
                }
              }
              break;
            }
          }
        switch(bi.biBitCount) {
          BYTE xn;
          case 1:
            xn=(bi.biWidth+7)/8 & 3;
            if(xn)
              SuperFileSeek(&f,4-xn,SEEK_CUR);
            break;
          case 4:
            xn=(bi.biWidth+1)/2 & 3;
            if(xn)
              SuperFileSeek(&f,4-xn,SEEK_CUR);
            break;
          case 8:
            xn=(bi.biWidth & 3);
            if(xn)
              SuperFileSeek(&f,4-xn,SEEK_CUR);
            break;
          case 16:
            xn=((bi.biWidth*2) & 3);
            if(xn)
              SuperFileSeek(&f,4-xn,SEEK_CUR);
            break;
          case 24:
            xn=((bi.biWidth*3) & 3);
            if(xn)
              SuperFileSeek(&f,4-xn,SEEK_CUR);
            break;
          case 32:
            break;
          }
        }
      }
    *xs=bi.biWidth;
    *ys=bi.biHeight;
    SuperFileClose(&f);
    return 1;
    }
  return 0;
  }

int drawPNGfile(HDC hDC,const char *filename,GRAPH_COORD_T x1, GRAPH_COORD_T y1,GRAPH_COORD_T *xs, GRAPH_COORD_T *ys,int8_t zoom) {
  SUPERFILE f;
  
  if(SuperFileOpen(&f,filename,OPEN_READ,TYPE_BINARY | SHARE_READ)) {  
//    *xs=JPG_Info.m_width;
//    *ys=JPG_Info.m_height;
    SuperFileClose(&f);
    }
  return 0;
  }

LRESULT viewerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int i=0;
      SUPERFILE jpegFile;
      char *nomefile=(char *)GET_WINDOW_OFFSET(hWnd,0);
      int8_t z=GetWindowByte(hWnd,GWL_USERDATA);
      RECT rc;
      GetClientRect(hWnd,&rc);
      GRAPH_COORD_T xs,ys;
      
      getDrive(nomefile,&jpegFile,NULL);
      if(stristr(nomefile,".jpg")) {
        xs=rc.right;
        ys=rc.bottom;
        i=drawJPGfile(hDC,nomefile,rc.left,rc.top,&xs,&ys,z);
        
        if(ys>=rc.bottom) {    // tenere conto di Zoom z
          EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
          SetScrollRange(hWnd,SB_VERT,0,1+ys-rc.bottom,FALSE);
          SetScrollPos(hWnd,SB_VERT,0,TRUE);
          }
        else {
          EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
          SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
          SetScrollPos(hWnd,SB_VERT,0,TRUE);
          }

        if(xs>=rc.right) {
          EnableScrollBar(hWnd,SB_HORZ,ESB_ENABLE_BOTH);   //
          SetScrollRange(hWnd,SB_HORZ,0,1+xs-rc.right,FALSE);
          SetScrollPos(hWnd,SB_HORZ,0,TRUE);
          }
        else {
          EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //
          SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
          SetScrollPos(hWnd,SB_HORZ,0,TRUE);
          }
        
        }
      else if(stristr(nomefile,".bmp")) {
        xs=rc.right;
        ys=rc.bottom;
        
        i=drawBMPfile(hDC,nomefile,rc.left,rc.top,&xs,&ys,z);
        }
      else if(stristr(nomefile,".png")) {
        xs=rc.right;
        ys=rc.bottom;

        i=drawPNGfile(hDC,nomefile,rc.left,rc.top,&xs,&ys,z);
        }
      else if(stristr(nomefile,".txt") || stristr(nomefile,".ini")) {
        int y=0;
        SetTextColor(hDC,BLACK);
        SetBkColor(hDC,WHITE);
        if(SuperFileOpen(&jpegFile,nomefile,OPEN_READ,TYPE_TEXT | SHARE_READ)) {  
          char buf[128];
          i=1;
          while(SuperFileGets(&jpegFile,buf,127)>=0) {
            if(y<ps.rcPaint.bottom)   // lascio cmq scorrere per scrollbar!
              TextOut(hDC,0,y,buf,strlen(buf));
            y+=getFontHeight(&hDC->font);
            }
          SuperFileClose(&jpegFile);
          }
        i=(rc.bottom-rc.top);
        if(y>=i) {
          EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
          SetScrollRange(hWnd,SB_VERT,0,1+y-i,FALSE);
          SetScrollPos(hWnd,SB_VERT,0,TRUE);
          }
        else {
          EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
          SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
          SetScrollPos(hWnd,SB_VERT,0,TRUE);
          }
        }
      if(!i)
        DrawText(hDC,"impossibile visualizzare il file",-1,&rc,
                DT_VCENTER | DT_CENTER);
        
      EndPaint(hWnd,&ps);
      }
      return 0;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
        fillRectangleWindowColorRect(hDC,&hWnd->paintArea,WHITE);
      }
      return 1;
      break;
      
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA,0);    // pulisco
			memset(GET_WINDOW_OFFSET(hWnd,0),0,32);
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      char *nomefile=(char *)cs->lpCreateParams;   // 
      if(nomefile) {
        strncpy(GET_WINDOW_OFFSET(hWnd,0),nomefile,31);
        SetWindowText(hWnd,nomefile); //"photoviewer"
        }
      return 0;
      }
      break;
    case WM_DESTROY:
//      m_WndViewer=NULL;
//      DestroyWindow(hWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {     //fare zoom
        int8_t t;
        case VK_ADD:
zoomin:
          t=GetWindowByte(hWnd,GWL_USERDATA);
          if(t<8) {
            t++;
            SetWindowByte(hWnd,GWL_USERDATA,t);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_SUBTRACT:
          t=GetWindowByte(hWnd,GWL_USERDATA);
          if(t>-7) {
            t--;
            SetWindowByte(hWnd,GWL_USERDATA,t);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_SPACE:
        case VK_0:
          SetWindowByte(hWnd,GWL_USERDATA,0);

        case VK_F5:   // vabbe' :)
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_ESCAPE:   // bah no
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
    case WM_LBUTTONDBLCLK:
      goto zoomin;    // centrare! in culo a bechis sempre :D ;)
      break;
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) {
          }
        }
      return 1;
      break;
      
    case WM_VIEWER_OPEN:
      {
      char *nomefile=(char *)lParam;   // 
      if(nomefile) {
        strncpy(GET_WINDOW_OFFSET(hWnd,0),nomefile,31);
        SetWindowText(hWnd,nomefile); //"photoviewer"
        ShowWindow(hWnd,SW_SHOW);
        InvalidateRect(hWnd,NULL,TRUE);
        }
      }
      break;
      
//    case WM_RBUTTONDOWN:
      // fare menu context... info
//      return DefWindowProc(hWnd,message,wParam,lParam);
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT playerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA,0);    // pulisco
      CreateWindow(MAKECLASS(WC_SLIDER),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD
        | TBS_HORZ | TBS_TOP | TBS_AUTOTICKS,
        0,cs->cy/2-4,cs->cx,14,
        hWnd,(MENU *)201,NULL
        );
      SendMessage(GetDlgItem(hWnd,201),TBM_SETRANGE,0,MAKELONG(1,10));  // calcolare su secondi musica
      SendMessage(GetDlgItem(hWnd,201),TBM_SETLINESIZE,0,1);
      SendMessage(GetDlgItem(hWnd,201),TBM_SETPOS,1,0);
      CreateWindow(MAKECLASS(WC_BUTTON),">",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
        20,cs->cy/2+18,40,12,
        hWnd,(MENU *)202,NULL
        );
      CreateWindow(MAKECLASS(WC_BUTTON),"II",WS_BORDER | WS_VISIBLE | WS_CHILD | WS_TABSTOP |
              BS_PUSHBUTTON | BS_CENTER | BS_TOP | BS_BOTTOM,
        80,cs->cy/2+18,40,12,
        hWnd,(MENU *)203,NULL
        );
			memset(GET_WINDOW_OFFSET(hWnd,0),0,32);
      char *nomefile=(char *)cs->lpCreateParams;   // 
      if(nomefile) {
        strncpy(GET_WINDOW_OFFSET(hWnd,0),nomefile,31);
        SetWindowText(hWnd,nomefile); //"player"
        }
      SetTimer(hWnd,1,1000,NULL);     // da usare poi, per suonare o boh
      return 0;
      }
      break;
    case WM_DESTROY:
      KillTimer(hWnd,1);
//      m_WndPlayer=NULL;
//      DestroyWindow(hWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_TIMER:
suona:
      if(SendMessage(GetDlgItem(hWnd,202),BM_GETSTATE,0,0)==BST_PUSHED) {      // play/pause... GetText?
        }
  /*      uint32_t offset=0;
        int readBytes;
        uint8_t buffer[1152*4];   // VERIFICARE!
        SUPERFILE f;
        if(lpDirectory)
          getDrive(lpDirectory,&f,NULL);
        else
          f.drive=currDrive;
        mp3_init();
        if(SuperFileOpen(&f,lpFile,OPEN_READ,TYPE_BINARY)) {
  //        uint16_t sampling_rate = mp3_get_sampling_rate();
  //        BYTE channels = mp3_get_channel_mode() == mp3_Mono ? 1 : 2;
          
          if(stristr(lpFile,".MP3")) {
            SetAudioMode(SAMPLES);		// in teoria non serve...
            WAVE_SAMPLES_FORMAT wsf;
            wsf.channels=2;
            wsf.bytesPerSample=2;
            wsf.samplesPerSec=44100;
            wsf.bufferSize=2* 576*2*  2;
            wsf.buffers=1;
            wsf.loop=0; //1
            wsf.notify=1;
            wsf.active=1;
            wsf.busy=wsf.finished=0;    // gestiti dall'audio cmq
            SetAudioSamplesFormat(&wsf);

          readBytes=SuperFileRead(&f,((uint8_t*)buffer),1152);
          if(readBytes < 1152)
            goto mp3_fine;
  mp3_read:
          KBClear();
          offset=0;
          mp3_mp3(&buffer[offset]);
          while(readBytes > (offset + mp3_get_header_size() + mp3_get_frame_size())) {
  mp3_resync:              
            mp3_init_header_params(&buffer[offset]);
            if(mp3_is_valid()) {
              mp3_init_frame_params(&buffer[offset]);
              offset += mp3_get_frame_size();
              }
            else {
              offset++;
              mp3_mp3(&buffer[offset]);
              continue;
              }

            audio_played=FALSE;
            if(mp3_is_valid()) { //all'ultimo si pianta... forse mp3_get_channels=0? NO boh
              SetAudioSamples(0, 0, (BYTE*)mp3_get_samples_as_int(),2* 576*2*mp3_get_channels());
              while(!audio_played)
                handle_events();
              }
// ci mette 200mS circa invec di 20-25 come dovrebbe (4500bytes @44100*2*2/sec
            printf("MP3 frame %u(%u):%u  ofs %u\n",++n,readBytes,timeGetTime(),offset);
            }

  // decimare, lampeggiare, fare finestra avanzamento        
      KBClear();
      handle_events();

      handle_more_events();

          if(keypress.key == 0x1b)
            goto mp3_fine;

          goto mp3_read;

  mp3_fine:        
        ;
            }
          else if(stristr(lpFile,".WAV")) {
            struct WAV_HEADER wh;
            struct WAV_HEADER_FACT whf;
            struct WAV_HEADER_DATA whd;
              //        leggere header RIFF ecc!!
            readBytes=SuperFileRead(&f,&wh,sizeof(struct WAV_HEADER));
            readBytes=SuperFileRead(&f,&whf,sizeof(struct WAV_HEADER_FACT));
            readBytes=SuperFileRead(&f,&whd,sizeof(struct WAV_HEADER_DATA));
   * 
            SetAudioMode(SAMPLES);
            WAVE_SAMPLES_FORMAT wsf;
            wsf.channels=1;
            wsf.bytesPerSample=1;
            wsf.samplesPerSec=22050;
            wsf.bufferSize=512;
            wsf.buffers=1;
            wsf.active=1; wsf.loop=1;
            wsf.notify=1;
            SetAudioSamplesFormat(&wsf);
           for(;;) {
              readBytes=SuperFileRead(&f,buffer,1024);    // usare buffer diverso!
              if(readBytes <=0)
                break;
              SetAudioSamples(0, 0, (BYTE*)mp3_get_samples_as_int(),  1024);
   audio_played=FALSE;
 
   while(!audio_played)
    handle_events();

  // decimare, lampeggiare, fare finestra avanzamento        
      KBClear();
      handle_events();

      handle_more_events();

              if(keypress.key == 0x1b)
                break;
              }
    
            }
          SuperFileClose(&f);
    mp3_end();
          }
        }
*/      
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {     //
        int8_t t;
        case VK_RIGHT:
          break;
        case VK_LEFT:
          break;
        case VK_SPACE:
          break;
        case VK_ESCAPE:
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) {
          }
        }
      if(HIWORD(wParam) == BN_CLICKED) {   // è 0!! coglioni :D
        switch(LOWORD(wParam)) { 
          case 202:   // play/pause
            break;
          case 203:   // stop
            break;
          }
        }
      return 1;
      break;
      
    case WM_PLAYER_OPEN:
      {
      char *nomefile=(char *)lParam;   // 
      if(nomefile) {
        strncpy(GET_WINDOW_OFFSET(hWnd,0),nomefile,31);
        SetWindowText(hWnd,nomefile); //"player"
        ShowWindow(hWnd,SW_SHOW);
        InvalidateRect(hWnd,NULL,TRUE);
        }
      }
      break;
      
//    case WM_RBUTTONDOWN:
      // fare menu context... info
//      return DefWindowProc(hWnd,message,wParam,lParam);
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

const char *notepadApp="Notepad";
LRESULT notepadWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  char *nomefile;   // 
  
  switch(message) {
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      nomefile=(char *)cs->lpCreateParams;   // 
      char *p=malloc(HTML_SIZE);      // riciclo :)
      *p=0;
      CreateWindow(MAKECLASS(WC_EDIT),NULL,WS_VISIBLE | WS_CHILD | ES_MULTILINE,
        0,0,cs->cx-1,cs->cy-1,
        hWnd,(HMENU)201,NULL
        );
      // FORZARE DLGC_WANTTAB qua :) per editor, o unire a MULTILINE
      SetWindowLong(hWnd,0,(DWORD)p);
      SendMessage(GetDlgItem(hWnd,201),EM_SETHANDLE,(WPARAM)p,0);
			memset(GET_WINDOW_OFFSET(hWnd,4),0,32);
      if(nomefile) {
        strncpy(GET_WINDOW_OFFSET(hWnd,4),nomefile,31);
        SetWindowText(hWnd,nomefile); //"notepad"
updatedafile:
        {
        SUPERFILE f;
        getDrive(nomefile,&f,NULL);
        if(SuperFileOpen(&f,nomefile,OPEN_READ,TYPE_TEXT | SHARE_READ)) {
          SuperFileRead(&f,p,HTML_SIZE-1);
          SuperFileClose(&f);
          SendMessage(GetDlgItem(hWnd,201),WM_SETTEXT /*EM_SETTEXTEX */,0,(LPARAM)p);
          }
        else
          MessageBox(hWnd,"File non trovato",notepadApp,MB_OK | MB_ICONINFORMATION);
        }
        }
      }
      return 0;
      break;
    case WM_CLOSE:
      if(SendMessage(GetDlgItem(hWnd,201),EM_GETMODIFY,0,0))
        if(MessageBox(hWnd,"Procedere?","File non salvato",MB_YESNO) != IDYES)
          return 0;
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_DESTROY:
      {
      void *p=(void*)GetWindowLong(hWnd,0);
      free(p);
//      m_WndNotepad=NULL;
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_CTLCOLOR:
      return GRAY064;
      break;
      
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) {
          case 1:    // nuovo
            if(SendMessage(GetDlgItem(hWnd,201),EM_GETMODIFY,0,0))
              MessageBox(hWnd,"Procedere?","File non salvato",MB_YESNO);
            SendMessage(GetDlgItem(hWnd,201),EM_SETSEL,0,-1);
            SendMessage(GetDlgItem(hWnd,201),WM_SETTEXT,0,(LPARAM)STR_EMPTY);   //boh :) vabbe'
            break;
          case 2:    // open file
            if(DialogBox((HINSTANCE)NULL,&fileChooserDlg,hWnd,(WINDOWPROC*)DefWindowProcFileDlgWC)) {
//      myWnd=(HWND)GetWindowLong(hWnd,0);
//      GetWindowText(myWnd,buf,31);
                
              nomefile=fileChooserDlg.caption;   // 
              if(SendMessage(GetDlgItem(hWnd,201),EM_GETMODIFY,0,0))
                MessageBox(hWnd,"Procedere?","File non salvato",MB_YESNO);
              goto updatedafile;
              }
            break;
          case 4:    // save file con nome FINIRE
//            if(DialogBox((HINSTANCE)NULL,&fileChooserDlg,hWnd,(WINDOWPROC*)DefWindowProcFileDlgWC)) {
//      myWnd=(HWND)GetWindowLong(hWnd,0);
//      GetWindowText(myWnd,buf,31);
                
//              }
          case 3:    // save file 
            {
            SUPERFILE f;
            f.drive=currDrive;
            int i=SendMessage(GetDlgItem(hWnd,201),WM_GETTEXTLENGTH,0,0);
            //p=SendMessage(myWnd,WM_GETTEXT,0,0); salto un passaggio e uso il buffer direttamente!
            if(i>=0) {    // bah anche 0!
              if(SuperFileOpen(&f,GET_WINDOW_OFFSET(hWnd,4),OPEN_WRITE,TYPE_TEXT | SHARE_READ)) {
                SuperFileWrite(&f,(char*)GetWindowLong(hWnd,0),i);
                SuperFileClose(&f);
                }
              }
            }
          case 7:    // esci
            if(SendMessage(GetDlgItem(hWnd,201),EM_GETMODIFY,0,0))
              if(MessageBox(hWnd,"Procedere?","File non salvato",MB_YESNO) != IDYES)
                return 1;
            PostMessage(hWnd,WM_CLOSE,0,0);
            break;
          case 128+2:    // info
            MessageBox(hWnd,"Notepad editor 1.0\nper Breakthrough", // troppo lungo! gestire di là :)
              "Informazioni su Notepad",MB_OK | MB_ICONINFORMATION);
            break;
          }
        }
      return 1;
      break;
      
    case WM_SIZE:
      MoveWindow(GetDlgItem(hWnd,201),0,0,LOWORD(lParam),HIWORD(lParam),TRUE);  // 
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_NOTEPAD_OPEN:
      {
      nomefile=(char *)lParam;   // 
      if(nomefile) {
        goto updatedafile;
        }
      }
      break;
      
    case WM_PRINTCLIENT:
      // fare :)
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }



ATOM MyRegisterClass(HINSTANCE hInstance) {
  }
BOOL InitInstance(HINSTANCE hInstance,int nCmdShow) {
//  ShowWindow( ,nCmdShow);
  }
THREADPROC int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;
  HWND myWnd;

  if(hPrevInstance)
    ;
  MyRegisterClass(hInstance);
  if(!InitInstance(hInstance, nCmdShow))
		return FALSE;

  myWnd=(HWND)hInstance;    // PER ORA COSI'! 
  msg.hWnd=myWnd;    // ma non viene sovrascritto sotto??? v. al posto di NULL in GetMessage
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);
// OCCHIO!! le variabili locali NON sono garantite nei thread (setjmp/longjmp)
    GET_MESSAGE_LOOP(myWnd,msg);
//	while(GetMessage(&msg, myWnd/*NULL*/, 0, 0)) {
    
  ExitThread(msg.wParam);
//	return msg.wParam;
  }
extern LRESULT wnd1proc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
extern LRESULT wnd3proc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam);
const MENU testMenu1={{
  {"Prova &dialog", 1, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Prova MsgBox", 2, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Prova MsgBox G", 3, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"Prova popup", 4 /*0*/, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&explorerMenu},
  {"Esci", 5, MF_ENABLED | MF_STRING | MF_UNCHECKED, NULL},
  {"\0\0\0\0", 0, 0, NULL},
  {0},
  },
  {
  {'D',0b00000001},{'M',0b00000001},{'G',0b00000001},{'P',0b00000001},{'E',0b00000001},
  }
  };
const MENU testMenu={{    // il titolo :)
  {"&File", 1, MF_ENABLED | MF_STRING | MF_UNCHECKED | MF_POPUP, (MENU*)&testMenu1},
  {"\0\0\0\0", 0, 0, NULL},
  },
  {
  {0}
  }
  };
HWND m_Wnd1;
ATOM MyRegisterClassDemo(HINSTANCE hInstance) {
  }
BOOL InitInstanceDemo(HINSTANCE hInstance,int nCmdShow) {
  m_Wnd1=CreateWindow(MAKECLASS(WC_DEFAULTCLASS),"finestra1",WS_BORDER | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE | WS_CLIPCHILDREN /*per prova messagebox*/,
    CW_USEDEFAULT,CW_USEDEFAULT,160,153,
    NULL,(MENU *)&testMenu,(void*)1/*lpCmdLine*/
    );
  if(!m_Wnd1)
    return FALSE;
  m_Wnd1->windowProc=(WINDOWPROC*)wnd1proc;    // 
  ShowWindow(m_Wnd1,nCmdShow);
  UpdateWindow(m_Wnd1);
  return TRUE;
  }
THREADPROC int WinMainDemo(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
  signed char bRet;
	HACCEL hAccelTable;

  if(hPrevInstance)
    ;
  MyRegisterClassDemo(hInstance);
  if(!InitInstanceDemo(hInstance, nCmdShow))
		return FALSE;

  SetTimer(m_Wnd1,1,976,NULL); // se non registri classe ad hoc, la CREATE non verrà eseguita anche forzandone la wndproc
  msg.hWnd=m_Wnd1;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_Wnd1,msg);
//    do {
//        bRet=GetMessage(&msg, m_Wnd/*NULL*/, 0, 0);
//		if(!TranslateAccelerator(msg.hWnd, hAccelTable, &msg)) {
//			TranslateMessage(&msg);
//			DispatchMessage(&msg);
//      }
//    } while(bRet!=-1);
//	while(GetMessage(&msg, m_Wnd/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }
extern HWND m_Wnd3;
ATOM MyRegisterClassDemo2(HINSTANCE hInstance) {
  }
BOOL InitInstanceDemo2(HINSTANCE hInstance,int nCmdShow) {
  m_Wnd3=CreateWindow(MAKECLASS(WC_DEFAULTCLASS),"test controls",WS_BORDER | WS_CAPTION | 
    WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE | WS_CLIPCHILDREN,
    210,200,256,102,
    NULL,(MENU *)&explorerMenu,(void*)3
    );
  if(!m_Wnd3)
    return FALSE;
  m_Wnd3->windowProc=(WINDOWPROC*)wnd3proc;    // 
  ShowWindow(m_Wnd3,nCmdShow);
//  UpdateWindow(m_Wnd3);
  m_Wnd3->children=CreateWindow(MAKECLASS(WC_PROGRESS),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD
    | PBS_CAPTION,
    26,12,150,18,
    m_Wnd3,(MENU *)1,NULL
    );
  SendMessage(m_Wnd3->children,PBM_SETRANGE,0,MAKELONG(0,100));
  SendMessage(m_Wnd3->children,PBM_SETSTATE,1,0);    // testo
  SendMessage(m_Wnd3->children,PBM_SETBARCOLOR,0,BRIGHTYELLOW);
  SendMessage(m_Wnd3->children,PBM_SETBKCOLOR,0,BRIGHTBLUE);
  SendMessage(m_Wnd3->children,PBM_SETPOS,45,0);
  m_Wnd3->children->next=CreateWindow(MAKECLASS(WC_LISTBOX),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD | WS_VSCROLL,
    8,34,100,40,
    m_Wnd3,(MENU *)2,NULL
    );
  SendMessage(m_Wnd3->children->next,LB_ADDSTRING,0,(LPARAM)"piciu");
  SendMessage(m_Wnd3->children->next,LB_ADDSTRING,0,(LPARAM)"stronzo");
  SendMessage(m_Wnd3->children->next,LB_INSERTSTRING,0 /*-1*/,(LPARAM)"minchia");
  SendMessage(m_Wnd3->children->next,LB_ADDSTRING,0,(LPARAM)"frocio");
  SendMessage(m_Wnd3->children->next,LB_ADDSTRING,0,(LPARAM)"merda");
  SendMessage(m_Wnd3->children->next,LB_ADDSTRING,0,(LPARAM)"cazzo");
  SendMessage(m_Wnd3->children->next,LB_SETSEL,1,1);
  m_Wnd3->children->next->next=CreateWindow(MAKECLASS(WC_EDIT),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD
    | 0 /*ES_MULTILINE*/,
    110,40,80,11,
    m_Wnd3,(MENU *)3,NULL
    );
  SendMessage(m_Wnd3->children->next->next,WM_SETTEXT,0,(LPARAM)"collino");
  SendMessage(m_Wnd3->children->next->next,EM_SETSEL,1,2);
  SendMessage(m_Wnd3->children->next->next,WM_SETFOCUS,0,1);

  m_Wnd3->children->next->next->next=CreateWindow(MAKECLASS(WC_UPDOWN),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD
    | UDS_ALIGNRIGHT     | UDS_ARROWKEYS,
    120,52,34,20,
    m_Wnd3,(MENU *)4,NULL
    );
  SendMessage(m_Wnd3->children->next->next->next,UDM_SETRANGE,0,MAKELONG(100,0));

  m_Wnd3->children->next->next->next->next=CreateWindow(MAKECLASS(WC_SLIDER),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD
    | TBS_HORZ | TBS_TOP | TBS_AUTOTICKS,
    164,56,50,14,
    m_Wnd3,(MENU *)5,NULL
    );
  SendMessage(m_Wnd3->children->next->next->next->next,TBM_SETRANGE,0,MAKELONG(1,5));
  SendMessage(m_Wnd3->children->next->next->next->next,TBM_SETLINESIZE,0,1);
  SendMessage(m_Wnd3->children->next->next->next->next,TBM_SETPOS,1 /*aggiorna*/,3);

  m_Wnd3->children->next->next->next->next->next=CreateWindow(MAKECLASS(WC_COMBOBOX),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD 
    | CBS_DROPDOWNLIST,
    184,10,70,44,
    m_Wnd3,(MENU *)6,NULL
    );
  SendMessage(m_Wnd3->children->next->next->next->next->next,CB_ADDSTRING,0,(LPARAM)"bipiciu");
  SendMessage(m_Wnd3->children->next->next->next->next->next,CB_ADDSTRING,0,(LPARAM)"dario");
  SendMessage(m_Wnd3->children->next->next->next->next->next,CB_ADDSTRING,0,(LPARAM)"dario2");
  SendMessage(m_Wnd3->children->next->next->next->next->next,CB_ADDSTRING,0,(LPARAM)"dario3");

  return TRUE;
  }
THREADPROC int WinMainDemo2(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)
    ;
  MyRegisterClassDemo2(hInstance);
  if(!InitInstanceDemo2(hInstance, nCmdShow))
		return FALSE;

  SetTimer(m_Wnd3,1,3000,NULL);    // diverso da 2 * 500 :)  // se non registri classe ad hoc, la CREATE non verrà eseguita anche forzandore la wndproc
  msg.hWnd=m_Wnd3;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

  GET_MESSAGE_LOOP(m_Wnd3,msg);

//	while(GetMessage(&msg, m_Wnd3/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassClock(HINSTANCE hInstance) {
  WNDCLASS *clockWC;
  clockWC=malloc(sizeof(WNDCLASS));
  clockWC->class=MAKECLASS(MAKEFOURCC('C','L','O','K'));
  clockWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS  | CS_DROPSHADOW;
  clockWC->icon= NULL /*standardIcon*/;
  clockWC->cursor=hourglassCursorSm;
  clockWC->cbClsExtra=0;
  clockWC->cbWndExtra=0;
  clockWC->lpfnWndProc=(WINDOWPROC *)clockWndProc;
  clockWC->hbrBackground=CreateSolidBrush(RED);
  clockWC->menu=(MENU*)&clockMenu;
  if(!RegisterClass(clockWC)) {
    free(clockWC);
    return FALSE;
    }
  return (ATOM)clockWC;
  }
BOOL InitInstanceClock(HINSTANCE hInstance, int nCmdShow) {
  // metterla in basso a dx!
  m_WndClock=CreateWindow(MAKECLASS(MAKEFOURCC('C','L','O','K')),"Orologio",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
    CW_USEDEFAULT,CW_USEDEFAULT,99,109+10/*menu*/,
    NULL,NULL,(void*)hInstance
    );
  if(!m_WndClock)
    return FALSE;
  ShowWindow(m_WndClock,nCmdShow);
//	UpdateWindow(m_WndClock);
  return m_WndClock ? TRUE : FALSE;
  }
THREADPROC int WinMainClock(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;
  WNDCLASS *clockWC;

  if(hPrevInstance)
    return FALSE;
  
  clockWC=MAKEINTATOM(MyRegisterClassClock(hInstance));
  if(!InitInstanceClock(hInstance, nCmdShow))
		return FALSE;

  msg.hWnd=m_WndClock;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndClock,msg);
//	while(GetMessage(&msg, m_WndClock/*NULL*/, 0, 0)) {
    
  UnregisterClass(clockWC->class,hInstance);
  free(clockWC);
  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassCalendar(HINSTANCE hInstance) {
  WNDCLASS *calendarWC;
  calendarWC=malloc(sizeof(WNDCLASS));
  calendarWC->class=MAKECLASS(MAKEFOURCC('C','L','D','R'));
  calendarWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS  | CS_DROPSHADOW;
  calendarWC->icon= NULL /*standardIcon*/;
  calendarWC->cursor=hourglassCursorSm;
  calendarWC->cbClsExtra=0;
  calendarWC->cbWndExtra=0;
  calendarWC->lpfnWndProc=(WINDOWPROC *)calendarWndProc;
  calendarWC->hbrBackground=CreateSolidBrush(WHITE);
  calendarWC->menu=NULL  /* ev menu per internet time ecc*/;
  if(!RegisterClass(calendarWC)) {
    free(calendarWC);
    return FALSE;
    }
  return (ATOM)calendarWC;
  }
BOOL InitInstanceCalendar(HINSTANCE hInstance, int nCmdShow) {
  // metterla in basso a dx!
  m_WndCalendar=CreateWindow(MAKECLASS(MAKEFOURCC('C','L','D','R')),"Calendario",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE | 
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
    CW_USEDEFAULT,CW_USEDEFAULT,99,109,
    NULL,NULL,(void*)hInstance
    );
  ShowWindow(m_WndCalendar,nCmdShow);
//	UpdateWindow(m_WndCalendar);
  return m_WndCalendar ? TRUE : FALSE;
  }
THREADPROC int WinMainCalendar(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  WNDCLASS *calendarWC;
  signed char bRet;

  if(hPrevInstance)
    return FALSE;

  calendarWC=MAKEINTATOM(MyRegisterClassCalendar(hInstance));
  if(!InitInstanceCalendar(hInstance, nCmdShow))
		return FALSE;

  msg.hWnd=m_WndCalendar;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndCalendar,msg);
//	while(GetMessage(&msg, m_WndCalendar/*NULL*/, 0, 0)) {
  UnregisterClass(calendarWC->class,hInstance);
  free(calendarWC);
  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassCalc(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstanceCalc(HINSTANCE hInstance, int nCmdShow) {
  m_WndCalc=CreateWindow(MAKECLASS(WC_DEFAULTCLASS),"Calcolatrice",WS_BORDER | WS_CAPTION | WS_MINIMIZEBOX |
    WS_SYSMENU | WS_VISIBLE | WS_CLIPCHILDREN,
    CW_USEDEFAULT,CW_USEDEFAULT,122,144,
    NULL,(MENU *)NULL,(void*)hInstance
    );
  if(!m_WndCalc)
    return FALSE;
  m_WndCalc->windowProc=(WINDOWPROC*)calcWndProc;    // 
  SendMessage(m_WndCalc,WM_CREATE,0,0);   // per wndproc; cfr internalCreateWindow
  ShowWindow(m_WndCalc,nCmdShow);
//	UpdateWindow(m_WndClock);
  return m_WndCalc ? TRUE : FALSE;
  }
THREADPROC int WinMainCalc(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)
    return FALSE;

  MyRegisterClassCalc(hInstance);
  if(!InitInstanceCalc(hInstance, nCmdShow))
		return FALSE;

  msg.hWnd=m_WndCalc;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndCalc,msg);
//	while(GetMessage(&msg, m_WndCalc/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassViewer(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstanceViewer(HINSTANCE hInstance, int nCmdShow,LPSTR lpCmdLine) {
  m_WndViewer=CreateWindow(MAKECLASS(WC_VIEWERCLASS),"Viewer",WS_BORDER | WS_CAPTION | 
    WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE,
    CW_USEDEFAULT,CW_USEDEFAULT,159,169 /*CYMENU*/,
    NULL,(MENU *)NULL,(void*)lpCmdLine
    ); 
  if(!m_WndViewer)
    return FALSE;
  ShowWindow(m_WndViewer,nCmdShow);
//	UpdateWindow(m_WndViewer);
  return TRUE;
  }
THREADPROC int WinMainViewer(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)
    ;
  MyRegisterClassViewer(hInstance);
  if(!InitInstanceViewer(hInstance,nCmdShow,lpCmdLine))
    return FALSE;

  msg.hWnd=m_WndViewer;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndViewer,msg);
//	while(GetMessage(&msg, m_WndViewer/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassTaskman(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstanceTaskman(HINSTANCE hInstance, int nCmdShow) {
  int xPos,AppXSize,yPos,AppYSize;
  
  if(GetSystemMetrics(SM_CXSCREEN)>500)
    AppXSize=339;
  else
    AppXSize=229;
  if(GetSystemMetrics(SM_CYSCREEN)>320)
    AppYSize=179;
  else
    AppYSize=119;
	xPos=(GetSystemMetrics(SM_CXSCREEN)-AppXSize)/2;
	yPos=(GetSystemMetrics(SM_CYSCREEN)-AppYSize)/2;
  m_WndTaskManager=CreateWindow(MAKECLASS(WC_TASKMANCLASS),"Task Manager",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_VISIBLE | 
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VSCROLL | WS_HSCROLL */  | WS_EX_TOPMOST /*opzionale :)*/,
    xPos,yPos,AppXSize,AppYSize,   // 
    NULL,(MENU *)NULL,(void*)hInstance
    );
  if(!m_WndTaskManager)
    return FALSE;
  ShowWindow(m_WndTaskManager,nCmdShow);
//	UpdateWindow(m_WndClock);
  return TRUE;
  }
THREADPROC int WinMainTaskman(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)
    return FALSE;
  
  MyRegisterClassTaskman(hInstance);
  if(!InitInstanceTaskman(hInstance, nCmdShow))
		return FALSE;

  msg.hWnd=m_WndTaskManager;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndTaskManager,msg);
//	while(GetMessage(&msg, m_WndTaskManager/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassFileManager(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstanceFilemanager(HINSTANCE hInstance,HINSTANCE hPrevInstance,int nCmdShow,LPSTR lpCmdLine) {
  HWND hWnd;
  hWnd=CreateWindow(MAKECLASS(WC_DIRCLASS),"Disco",WS_BORDER | WS_CAPTION | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
    CW_USEDEFAULT,CW_USEDEFAULT,239,159,
    NULL,(MENU *)&explorerMenu,(void*)lpCmdLine
    );    // 
  if(!hWnd)
    return FALSE;
  ShowWindow(hWnd,nCmdShow);
  if(hPrevInstance)   // gestire meglio...
    m_WndFileManager[1]=hWnd;
  else
    m_WndFileManager[0]=hWnd;
//	UpdateWindow(hWnd);
  return TRUE;
  }
THREADPROC int WinMainFilemanager(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;
  volatile HWND *volatile myWnd;

  if(hPrevInstance)   // (gestire qua!
    ;
  MyRegisterClassFileManager(hInstance);
  if(!InitInstanceFilemanager(hInstance,hPrevInstance,nCmdShow,lpCmdLine))
		return FALSE;
  // strano, aprendo la seconda la prima diventa trasparente e tutto è "non responding" ma mouse e screensaver vanno, come con le dialog box
//si è anche schiantato in MPFS... forse non rientrante?
  
  myWnd=hPrevInstance ? &m_WndFileManager[1] : &m_WndFileManager[0];    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  msg.hWnd=*myWnd;
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndFileManager[0]/*(*myWnd)*/,msg);    // non va con volatile, schianti casuali - SERVE STATICA
//		si potrebbe provare e ricaricare myWnd DENTRO al loop

//	while(GetMessage(&msg, m_WndFileManager[0]/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassControl(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstanceControl(HINSTANCE hInstance,int nCmdShow,LPSTR lpCmdLine) {
  m_WndControlPanel=CreateWindow(MAKECLASS(WC_CONTROLPANELCLASS),"Pannello di controllo",WS_BORDER | WS_THICKFRAME | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
    CW_USEDEFAULT,CW_USEDEFAULT,219,159,
    NULL,(HMENU)&menuControlPanel,(void*)lpCmdLine
    );
  if(!m_WndControlPanel)
    return FALSE;
  ShowWindow(m_WndControlPanel,nCmdShow);
//	UpdateWindow(m_WndControlPanel);
  return TRUE;
  }
THREADPROC int WinMainControl(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)   // 
    return FALSE;
  
  MyRegisterClassControl(hInstance);
  if(!InitInstanceControl(hInstance, nCmdShow,lpCmdLine))
		return FALSE;

  msg.hWnd=m_WndControlPanel;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndControlPanel,msg);
//	while(GetMessage(&msg, m_WndControlPanel/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassDOS(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstanceDOS(HINSTANCE hInstance,int nCmdShow,LPSTR lpCmdLine) {
  m_WndDOS = CreateWindow(MAKECLASS(WC_CMDSHELL),"DOS shell",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU /*| WS_VISIBLE*/,
    /*CW_USEDEFAULT*/240,/*CW_USEDEFAULT*/30,219,149,
    NULL,(MENU *)NULL,(void*)lpCmdLine
    );
  if(!m_WndDOS)
    return FALSE;
  ShowWindow(m_WndDOS,nCmdShow);
//	UpdateWindow(m_WndDOS);
  return TRUE;
  }
THREADPROC int WinMainDOS(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  if(hPrevInstance)
    ;     // gestire qua
  
  MyRegisterClassDOS(hInstance);
  if(!InitInstanceDOS(hInstance,nCmdShow,lpCmdLine))
		return FALSE;

  msg.hWnd=m_WndDOS;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndDOS,msg);
//	while(GetMessage(&msg, m_WndDOS/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassSurf(HINSTANCE hInstance) {
  WNDCLASS *surfWC;
  surfWC=malloc(sizeof(WNDCLASS));
  surfWC->class=MAKECLASS(MAKEFOURCC('S','U','R','F'));
  surfWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
extern const MENU surfMenu;
extern const GFX_COLOR surfIcon[],surfWaitIcon[];
extern const char *surfApp;
  surfWC->icon=surfIcon;
  surfWC->cursor=standardCursorSm;
  surfWC->cbClsExtra=0;
  surfWC->cbWndExtra=sizeof(struct HTMLinfo) + HTML_SIZE;   // dati pagina web, pagina web (in USERDATA attrib, SOCKET, socket-tipo,stato
  surfWC->lpfnWndProc=(WINDOWPROC *)surfWndProc;
  surfWC->hbrBackground=CreateSolidBrush(GRAY096);
  surfWC->menu=(MENU*)&surfMenu;
  if(!RegisterClass(surfWC)) {
    free(surfWC);
    return FALSE;
    }
  return (ATOM)surfWC;
  }
BOOL InitInstanceSurf(HINSTANCE hInstance,int nCmdShow,LPSTR lpCmdLine) {
  m_WndSurf=CreateWindow(MAKECLASS(MAKEFOURCC('S','U','R','F')),surfApp,WS_BORDER | WS_CAPTION | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_THICKFRAME | WS_VSCROLL | WS_HSCROLL,
    CW_USEDEFAULT,CW_USEDEFAULT,299,239,
    NULL,(MENU *)NULL,(void*)lpCmdLine
    );
  if(!m_WndSurf)
    return FALSE;
  SetWindowByte(GetDlgItem(m_WndSurf,204),GWL_USERDATA,0b00000101);   // prova! status led OCCHIO se togli status!
  ShowWindow(m_WndSurf,nCmdShow);
//	UpdateWindow(m_WndSurf);
  return TRUE;
  }
THREADPROC int WinMainSurf(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  WNDCLASS *surfWC;
  signed char bRet;

  if(hPrevInstance)
    ;   // gestire
  
  surfWC=MAKEINTATOM(MyRegisterClassSurf(hInstance));
  if(!InitInstanceSurf(hInstance,nCmdShow,lpCmdLine))
		return FALSE;

  msg.hWnd=m_WndSurf;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndSurf,msg);
//	while(GetMessage(&msg, m_WndSurf/*NULL*/, 0, 0)) {
  UnregisterClass(surfWC->class,hInstance);
  free(surfWC);
  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassNotepad(HINSTANCE hInstance) {
  WNDCLASS *notepadWC;
  notepadWC=malloc(sizeof(WNDCLASS));
  notepadWC->class=MAKECLASS(MAKEFOURCC('N','O','T','E'));
  notepadWC->style= CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
extern const MENU notepadMenu;
extern const GFX_COLOR notepadIcon[];
extern const char *notepadApp;
  notepadWC->icon=notepadIcon;
  notepadWC->cursor=standardCursorSm;
  notepadWC->cbClsExtra=0;
  notepadWC->cbWndExtra=4+32;   // handle memoria testo per Edit; nomefile
  notepadWC->lpfnWndProc=(WINDOWPROC *)notepadWndProc;
  notepadWC->hbrBackground=CreateSolidBrush(GRAY096);
  notepadWC->menu=(MENU*)&notepadMenu;
  if(!RegisterClass(notepadWC)) {
    free(notepadWC);
    return FALSE;
    }
  return (ATOM)notepadWC;
  }
BOOL InitInstanceNotepad(HINSTANCE hInstance,int nCmdShow,LPSTR lpCmdLine) {
  m_WndNotepad=CreateWindow(MAKECLASS(MAKEFOURCC('N','O','T','E')),notepadApp,WS_BORDER | WS_CAPTION | WS_VISIBLE |
    WS_OVERLAPPEDWINDOW | WS_SYSMENU,
    CW_USEDEFAULT,CW_USEDEFAULT,219,199,
    NULL,(MENU *)NULL,(void*)lpCmdLine
    );
  if(!m_WndNotepad)
    return FALSE;
  ShowWindow(m_WndNotepad,nCmdShow);
//	UpdateWindow(m_WndNotepad);
  return TRUE;
  }
THREADPROC int WinMainNotepad(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  WNDCLASS *notepadWC;
  signed char bRet;

  if(hPrevInstance)
    ;     // gestire qua
  
  notepadWC=MAKEINTATOM(MyRegisterClassNotepad(hInstance));
  if(!InitInstanceNotepad(hInstance,nCmdShow,lpCmdLine))
		return FALSE;

  msg.hWnd=m_WndNotepad;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndNotepad,msg);
//	while(GetMessage(&msg, m_WndNotepad/*NULL*/, 0, 0)) {
  UnregisterClass(notepadWC->class,hInstance);
  free(notepadWC);
  ExitThread(msg.wParam);
//	return msg.wParam;
  }

ATOM MyRegisterClassPlayer(HINSTANCE hInstance) {
  return (ATOM)TRUE;
  }
BOOL InitInstancePlayer(HINSTANCE hInstance,int nCmdShow,LPSTR lpCmdLine) {
  m_WndPlayer=CreateWindow(MAKECLASS(WC_PLAYERCLASS),"Player",WS_BORDER | WS_CAPTION | 
    WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE,
    CW_USEDEFAULT,CW_USEDEFAULT,159,99 /*CYMENU*/,
    NULL,(MENU *)NULL,(void*)lpCmdLine
    ); 
  if(!m_WndPlayer)
    return FALSE;
  ShowWindow(m_WndPlayer,nCmdShow);
//	UpdateWindow(m_WndPlayer);
  return TRUE;
  }
THREADPROC int WinMainPlayer(HINSTANCE hInstance,HINSTANCE hPrevInstance,
  LPSTR lpCmdLine,int nCmdShow) {
	MSG msg;
	HACCEL hAccelTable;
  signed char bRet;

  MyRegisterClassPlayer(hInstance);
  if(!InitInstancePlayer(hInstance,nCmdShow,lpCmdLine))
		return FALSE;

  msg.hWnd=m_WndPlayer;    // PER ORA COSI'! dopo viene cmq sovrascritto... metto hWnd al posto di NULL
  
	hAccelTable = LoadAccelerators(hInstance, (LPCSTR)1 /*IDC_TESTWINDOWS*/);

    GET_MESSAGE_LOOP(m_WndPlayer,msg);
//	while(GetMessage(&msg, m_WndPlayer/*NULL*/, 0, 0)) {

  ExitThread(msg.wParam);
//	return msg.wParam;
  }

