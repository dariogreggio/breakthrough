/*
 * BREAKTHROUGH 2.0
 * (C) G.Dar 1987-2023 :) !
 * (portions inspired by Windows, since 1990)
 * 
 * applicazioni: BASIC, OROLOGIO, CALCOLATRICE, FILE MANAGER
 */
 
#include <xc.h>
//#include "compiler.h"
#include "pc_pic_cpu.h"
#include <stdio.h>
#include <string.h>


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


extern volatile unsigned long now;
extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern DWORD extRAMtot;
extern struct KEYPRESS keypress;
extern struct MOUSE mouse;

extern const char demoscript[],demoscript2[],demoscript3[],demoscript4[];


LRESULT minibasicWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
#if 0
    case WM_ERASEBKGND:
      {RECT rc;
      HDC myDC,*hDC;
      hDC=GetDC(hWnd,&myDC);
      GetClientRect(hWnd,&rc);
//      rc.top+=2; rc.left+=2; rc.bottom-=2; rc.right-=2;
#ifndef USING_SIMULATOR
      FillRect(hDC /* ovvero (HDC *)wParam */,&rc,CreateSolidBrush(WHITE);
#endif
      ReleaseDC(hWnd,hDC);
      }
      return 1;
      break;
#endif
#if 0
    case WM_PAINT:
      {
      char buf[8];
      PAINTSTRUCT ps;
      GetClientRect(hWnd,&rc);
      HDC *myDC=BeginPaint(hWnd,&ps);
      EndPaint(hWnd,&ps);
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
#endif
    case WM_TIMER:
      {RECT rc;
      HDC myDC,*hDC;
      
  //TOGLIERE POI!
      MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(hWnd,4);
      DWORD script=GetWindowLong(hWnd,0);
      if(HIWORD(script)) {
        char *prg=(char*)script;
        SetWindowText(hWnd,prg);
        basic(minstance,prg,1,hWnd);
        }
      else {
        SetWindowText(hWnd,script==4 ? "MiniBasic - first prg" : 
              (script==3 ? "MiniBasic - trigircle" : (script==2 ? "MiniBasic - demoscrpt2" : "MiniBasic - demoscript")));
      
        basic(minstance,script==4 ? demoscript4 : 
          (script==3 ? demoscript3 : (script==2 ? demoscript2 : demoscript)),0,hWnd);
        }
      
//      minibasic();
//      KillTimer();
      
      // SetText(memoria libera...
      
//      ReleaseDC(hWnd,hDC);
      }
      return 1;
      break;
    case WM_CHAR:
      {
      MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(hWnd,4);
//      if(GetActiveMenu())   // 
//        SendMessage(hWnd,WM_UNINITMENUPOPUP,(WPARAM)GetActiveMenu(),
//          MAKELONG(0,GetActiveMenu()==GetSystemMenu(NULL,0) /*bah :D */ ? MF_SYSMENU : 0));
//        return DefWindowProc(hWnd,message,wParam,lParam);
//      else {
        minstance->incomingChar[0]=LOBYTE(wParam);
        minstance->incomingChar[1]=HIBYTE(wParam);    // modifier...? o GetAsyncKeyState ?

//        if(isdigit(LOBYTE(wParam)))
//          ; //SetWindowLong(hWnd,0,LOBYTE(wParam)-'0');     // per provare vari programmi! v. ACCEL
//        else
        
        
//          return DefWindowProc(hWnd,message,wParam,lParam);
//        }
      
      }
      return 0;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      // forzare font piccolo??

      MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(hWnd,4);
//      *minstance-> =0;
      //memset () ?? pare arrivi tutto a zero cmq da malloc...
      memset(minstance,0,sizeof(MINIBASIC));
      SetWindowLong(hWnd,0,(int)cs->lpCreateParams);

      minstance->hWnd=hWnd;
      minstance->threadID=BeginThread(0 /*env*/);

      }
      return 0;
      break;
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(hWnd,4);
        switch(LOWORD(wParam)) { 
          case 1:   // open
            {
              if(DialogBox((HINSTANCE)NULL,&fileChooserDlg,hWnd,(WINDOWPROC*)DefWindowProcFileDlgWC)) {
                
                }
            }
            break;
          case 2:   // run
            {
              doRun(minstance);
            }
            break;
            
          case 101:
            SetWindowLong(hWnd,0,1);
            break;
          case 102:
            SetWindowLong(hWnd,0,2);
            break;
          case 103:
            SetWindowLong(hWnd,0,3);
            break;
          case 104:
            SetWindowLong(hWnd,0,4);
            break;
          case SC_CLOSE:
            SendMessage(hWnd,WM_CLOSE,0,0);
            // goto WM_CLOSE??
            break;
          }
        minstance->menucommand=LOWORD(wParam);
        InvalidateRect(hWnd,NULL,TRUE);
        }
      break;
    case WM_CLOSE:
      {
      MINIBASIC *minstance=(MINIBASIC*)GET_WINDOW_OFFSET(hWnd,4);
      minstance->errorFlag= ERR_STOP;
      KillTimer(hWnd,1);
      EndThread(minstance->threadID);
//      cleanup(minstance,1);
      DestroyWindow(hWnd);
      }
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT clockWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      char buf[32];
      PIC32_DATE date;
      PIC32_TIME time;
      int i;
      
      hDC->pen=CreatePen(1,1,LIGHTRED);
      hDC->brush=CreateSolidBrush(RED);
      
      SetTimeFromNow(now,&date,&time);
      
      if(!IsIconic(hWnd)) {
        hDC->font=CreateFont(8,6,0,0,FW_BOLD, FALSE,FALSE,FALSE, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
        SetTimeFromNow(now,&date,&time);

        // Orologio 29/6/2022-4/7 .. since 1987 :)
        // variables to define the circle 
        int cx=ps.rcPaint.right/2,cy=(ps.rcPaint.bottom-10)/2;
        float stp=PI*2/12,rAngle,h,m,s;
        int radius=min(ps.rcPaint.right,ps.rcPaint.bottom-12)/2;
        if(radius<=0)
          radius=1;
        SetTextColor(hDC,WHITE); SetBkColor(hDC,BLACK);
        hDC->brush.size=0;
        //bah faccio prima :)  .. CreateBrushIndirct(&lbr)


        hDC->brush.size=1;       hDC->brush.color=SIENNA; //PROVA ELLISSE PIENO


        Ellipse(hDC,cx-radius,cy-radius,cx+radius,cy+radius);
        SetTextColor(hDC,BRIGHTGREEN);
        for(i=1; i<=12; i++) {
          rAngle=i*stp;
          sprintf(buf,"%u",i);
          TextOut(hDC,cx-2+radius*sin(rAngle)*0.9,cy-2-radius*cos(rAngle)*0.9,buf);
          }
        hDC->pen=CreatePen(1,1,BRIGHTCYAN);
  //      hDC->brush=CreateSolidBrush(BRIGHTCYAN);
        s=time.sec;
        rAngle=((s/5) * (PI*2))/12;
        radius=min(ps.rcPaint.right,ps.rcPaint.bottom)/3.0;
        int xs=cx+radius*sin(rAngle);
        int ys=cy-radius*cos(rAngle);
        m=time.min;
        rAngle=((m/5) * (PI*2))/12;
        radius=min(ps.rcPaint.right,ps.rcPaint.bottom)/3.5;
        int xm=cx+radius*sin(rAngle);
        int ym=cy-radius*cos(rAngle);
        h=time.hour % 12;
        m= m + (s/60); h= h + (m/60);
        rAngle=(h * (PI*2))/12;
        radius=min(ps.rcPaint.right,ps.rcPaint.bottom)/4.0;
        int xh=cx+radius*sin(rAngle);
        int yh=cy-radius*cos(rAngle);
        MoveTo(hDC,cx,cy);
        LineTo(hDC,xh,yh);
        MoveTo(hDC,cx,cy);
        LineTo(hDC,xm,ym);
        MoveTo(hDC,cx,cy);
        LineTo(hDC,xs,ys);

        hDC->font=CreateFont(8,6,0,0,FW_BOLD, FALSE,TRUE,FALSE, ANSI_CHARSET,
                OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_DONTCARE,NULL);
        sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u %u",date.mday,date.mon,date.year,
                time.hour,time.min,time.sec);
        SetTextColor(hDC,LIGHTBLUE);
        TextOut(hDC,0,ps.rcPaint.bottom-8,buf);
        DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
        }
      else {

        // Orologio 29/6/2022-4/7 .. since 1987 :)
        // variables to define the circle 
        int cx=ps.rcPaint.right/2,cy=ps.rcPaint.bottom/2;
        float stp=PI*2/12,rAngle,h,m;
        int radius=(min(ps.rcPaint.right,ps.rcPaint.bottom)/2) -1;
        if(radius<=0)
          radius=1;
        hDC->brush.size=0;
        //bah faccio prima :)  .. CreateBrushIndirct(&lbr)


        hDC->brush.size=1;       hDC->brush.color=SIENNA; //PROVA ELLISSE PIENO


        Ellipse(hDC,cx-radius,cy-radius,cx+radius,cy+radius);
        hDC->pen=CreatePen(1,1,BRIGHTCYAN);
  //      hDC->brush=CreateSolidBrush(BRIGHTCYAN);
        m=time.min;
        rAngle=((m/5) * (PI*2))/12;
        radius=min(ps.rcPaint.right,ps.rcPaint.bottom)/3.5;
        int xm=cx+radius*sin(rAngle);
        int ym=cy-radius*cos(rAngle);
        h=time.hour % 12;
        h= h + (m/60);
        rAngle=(h * (PI*2))/12;
        radius=min(ps.rcPaint.right,ps.rcPaint.bottom)/4.0;
        int xh=cx+radius*sin(rAngle);
        int yh=cy-radius*cos(rAngle);
        MoveTo(hDC,cx,cy);
        LineTo(hDC,xh,yh);
        MoveTo(hDC,cx,cy);
        LineTo(hDC,xm,ym);
        }
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
//      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      fillRectangleWindowColor(hDC,hWnd->paintArea.left,hWnd->paintArea.top,
              hWnd->paintArea.right,hWnd->paintArea.bottom,SIENNA);
      }
      return 1;
      break;
      
    case WM_TIMECHANGE:
    case WM_TIMER:
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetTimer(hWnd,1,1000,NULL);
      }
      return 0;
      break;
    case WM_SIZE:
      if(wParam==SIZE_MINIMIZED) {
        SetTimer(hWnd,1,60000,NULL);
        }
      else
        SetTimer(hWnd,1,1000,NULL);
      break;
    case WM_WINDOWPOSCHANGED:
      SendMessage(hWnd,WM_TIMER,1,0);   // forzo redraw icona al volo :) in SIZE non va bene
      // no non va cmq, arriva prima del Paint...
      break;
    case WM_CLOSE:
      KillTimer(hWnd,1);
      DestroyWindow(hWnd);
      m_WndClock=NULL;
      break;
      
    case WM_RBUTTONDOWN:
      // fare menu context...
#ifdef USA_WIFI
  			m2m_wifi_get_sytem_time();   // intanto :)
#endif
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
  static BYTE state;
  
  switch(message) {
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case 1:
            DialogBox((HINSTANCE)NULL,&calcDlg,hWnd,(WINDOWPROC*)calcDlgproc);
            break;
          }
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
      // provare a far dialog già qua...
      }
      return 0;
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
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
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
        return DefDlgProc(hWnd,message,wParam,lParam);
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
        case '+':
          op='+';
          goto isop;
          break;
        case '-':
          op='-';
          goto isop;
          break;
        case '*':
          op='*';
          goto isop;
          break;
        case '/':
          op='/';
          goto isop;
          break;
        case VK_ESCAPE:
          op=0;
          display[displayptr=0]='0';
          display[displayptr+1]=0;
          newop=1;
          goto update;
          break;
        case VK_RETURN:
          goto calc;
          break;
        default:
          return DefDlgProc(hWnd,message,wParam,lParam);
          // forwardare per mettere focus? o no? o non così cmq... v. defDlgProc
          break;
        }
      return 0;
      break;
      
    case WM_CLOSE:
      m_WndCalc=NULL;
      break;
      
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


LRESULT surfWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      char buf[32];
      int i;
      BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
      
      hDC->pen=CreatePen(1,1,LIGHTGREEN);
      hDC->brush=CreateSolidBrush(GREEN);
      SetTextColor(hDC,LIGHTGREEN);
      
      TextOut(hDC,2,2,"Indirizzo:");
      drawHorizLineWindow(hDC,0,12,ps.rcPaint.right-1);
      
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
        fillRectangleWindow(hDC,hWnd->paintArea.left,hWnd->paintArea.top,
              hWnd->paintArea.right,hWnd->paintArea.bottom);
      }
      return 1;
      break;
    case WM_CTLCOLOR:
      {HDC hDC;
      DC myDC;
      hDC=GetDC(hWnd,&myDC);
      ReleaseDC(hWnd,hDC);
      return myDC.brush.color;
      }
      break;
      
    case WM_TIMER:
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
			memset(GET_WINDOW_OFFSET(hWnd,0),4+4+4+4,0);
      SetTimer(hWnd,1,1000,NULL); //per animazione icona, timeouttimeout?
      char *url=(char *)cs->lpCreateParams;   // usare, salvare...
      BYTE attrib=GetProfileInt(profileFile,"SURF","attributi",1);
      SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
      SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // tipo socket
      SetWindowByte(hWnd,GWL_USERDATA+3,0);   //stato
      HWND myWnd=CreateWindow(MAKECLASS(WC_EDIT),"http://",WS_BORDER | WS_VISIBLE | WS_CHILD | 
        0,
        64,1,cs->cx-64-36-12,10,
        hWnd,(HMENU)1,NULL
        );
      SetWindowLong(hWnd,0,(DWORD)myWnd);
      myWnd=CreateWindow(MAKECLASS(WC_BUTTON),"Vai",WS_BORDER | WS_VISIBLE | WS_TABSTOP |
        WS_CHILD | BS_CENTER | BS_PUSHBUTTON,
        cs->cx-34-12,1,30,10,
        hWnd,(HMENU)2,NULL
        );
      SetWindowLong(hWnd,4,(DWORD)myWnd);
      myWnd=CreateWindow(MAKECLASS(WC_STATIC),"",WS_BORDER | /*WS_VISIBLE | per icon dopo! */WS_CHILD | 
        /*WS_DISABLED | */ SS_ICON,
        cs->cx-10,2,8,8,
        hWnd,(HMENU)3,NULL
        );
      myWnd->icon=redBallIcon;
      SetWindowLong(hWnd,8,(DWORD)myWnd);
			if(attrib & 1) {   // statusbar
        myWnd=CreateWindow(MAKECLASS(WC_STATIC),"idle",WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED | SS_LEFT,
          0,cs->cy-11,cs->cx-1,cs->cy-1,
          hWnd,(HMENU)4,NULL
          );
   			SetWindowLong(hWnd,12,(DWORD)myWnd);
				}
      }
      return 0;
      break;
    case WM_CLOSE:
      KillTimer(hWnd,1);
      m_WndSurf=NULL;
      DestroyWindow(hWnd);
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_RETURN:     // mmm credo che arrivi da notify del control EDIT..
          {HWND myWnd;
          myWnd=(HWND)GetWindowLong(hWnd,0);
          if(SendMessage(myWnd,WM_GETTEXTLENGTH,0,0) > 0)
            SendMessage(myWnd,WM_COMMAND,MAKEWORD(2,BN_CLICKED),0);
          } 
          break;
        case VK_ESCAPE:     // o accelerator??
          break;
        }
      break;
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
        switch(LOWORD(wParam)) {
          case 32+1:    // reload
            {SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
            if(s!=INVALID_SOCKET) {   // gestire wifi/ethernet
              char buf[32];
              strcpy(buf,"GET / HTTP/1.1\r\n");
              send(s,buf,strlen(buf),0);
              }
            }
            break;
          case 48+4:    // stop
            break;
          }
        }
      if(HIWORD(wParam) == BN_CLICKED) {   // è 0!! coglioni :D
        switch(LOWORD(wParam)) { 
          case 2:   // vai!
            {SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
            if(s==INVALID_SOCKET) {   // gestire wifi/ethernet
              struct sockaddr_in strAddr;
              BYTE u8Flags=0;   // boh??
              IP_ADDR dstIP;
              HWND myWnd;
              char buf[32];
              DWORD tOut;
              s = socket(AF_INET,SOCK_STREAM,u8Flags);
              if(s != INVALID_SOCKET) {
                SetWindowByte(hWnd,GWL_USERDATA+1,s);
                
                strAddr.sin_family = AF_INET;
                strAddr.sin_port = _htons(80);

                dstIP.v[0]=192; dstIP.v[1]=168; dstIP.v[2]=1; dstIP.v[3]=2; 
//      gethostbyname((uint8_t*)parmsPointer); finire ecc DNS
                myWnd=(HWND)GetWindowLong(hWnd,0);
                GetWindowText(myWnd,buf,31);

                strAddr.sin_addr.s_addr = dstIP.Val;
                connect(s, (struct sockaddr*)&strAddr, sizeof(struct sockaddr_in));
                M2M_INFO("Surf connect\r\n");
//                while(!*(unsigned long*)internetBuffer && tOut<DNS_TIMEOUT) {
                tOut=0;
                while(tOut<1000) {    // finire!
                  m2m_wifi_handle_events(NULL);
                  tOut++;
                  __delay_ms(1);
                  }
                char buf[32];
                strcpy(buf,"GET / HTTP/1.1\r\n\r\n");
                send(s,buf,strlen(buf),0);
                tOut=0;
                while(tOut<1000) {    // finire!
                  m2m_wifi_handle_events(NULL);
                  tOut++;
                  __delay_ms(1);
                  }
                myWnd=(HWND)GetWindowLong(hWnd,12);
                if(myWnd) {
                  sprintf(buf,"Connessione a %u.%u.%u.%u...",dstIP.v[0],dstIP.v[1],dstIP.v[2],dstIP.v[3]);
                  SetWindowText(myWnd,buf);
                  }
                myWnd=(HWND)GetWindowLong(hWnd,8);
                myWnd->icon=surfWaitIcon;
                InvalidateRect(myWnd,NULL,TRUE);
                }
              }
            }
            break;
          }
        }
      return 1;
      break;
      
    case WM_SIZE:
      {HWND myWnd;
      myWnd=(HWND)GetWindowLong(hWnd,0);
      MoveWindow(myWnd,64,1,LOWORD(lParam)-64-36-12,10,TRUE);
      myWnd=(HWND)GetWindowLong(hWnd,4);
      MoveWindow(myWnd,LOWORD(lParam)-34-12,1,30,10,TRUE);
      myWnd=(HWND)GetWindowLong(hWnd,8);
      MoveWindow(myWnd,LOWORD(lParam)-10,2,8,8,TRUE);
      myWnd=(HWND)GetWindowLong(hWnd,12);
      if(myWnd)
        MoveWindow(myWnd,0,HIWORD(lParam)-10,LOWORD(lParam),10,TRUE);
      }
      break;
      
//    case WM_RBUTTONDOWN:
      // fare menu context...
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

extern unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, 
  unsigned char *pBytes_actually_read, void *pCallback_data);
        
LRESULT viewerWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int i=0;
      SUPERFILE jpegFile;
      char *nomefile=(char *)GET_WINDOW_OFFSET(hWnd,0);
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      
      jpegFile.drive=currDrive;
      if(stristr(nomefile,".jpg")) {
        if(SuperFileOpen(&jpegFile,nomefile,'r')) {  
          i=1;
          
          int x,x1,mcu_x=0;
          int y,y1,mcu_y=0;
          pjpeg_image_info_t JPG_Info;
          bool status;
          DWORD cback_data[2];
          cback_data[0]=(DWORD)&jpegFile;      // riciclo la struttura del desktop!
          cback_data[1]=0xffffffff;
          int8_t z=GetWindowByte(hWnd,GWL_USERDATA);
          
          if(!(status=pjpeg_decode_init(&JPG_Info,pjpeg_need_bytes_callback,
            &cback_data,0))) {
            img_ofs_y=JPG_Info.m_height;
            img_ofs_x=JPG_Info.m_width;
            if(z<0) for(x=0; x>z; x--) {
              img_ofs_y /= 2;
              img_ofs_x /= 2;
              }
            else for(x=0; x<z; x++) {
              img_ofs_y *= 2;
              img_ofs_x *= 2;
              }
            SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / JPG_Info.m_height,FALSE);
            SetScrollPos(hWnd,SB_VERT,img_ofs_y/2,FALSE);
            SetScrollRange(hWnd,SB_HORZ,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / JPG_Info.m_width,FALSE);
            SetScrollPos(hWnd,SB_HORZ,img_ofs_x/2,FALSE);
            img_ofs_y=((int)(ps.rcPaint.bottom-ps.rcPaint.top)-(int)img_ofs_y)/2;
            img_ofs_x=((int)(ps.rcPaint.right-ps.rcPaint.left)-(int)img_ofs_x)/2;
            if(img_ofs_y<0) img_ofs_y=0;
            if(img_ofs_x<0) img_ofs_x=0;
            
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

                  uint8_t bx,by;
                  for(by=0; by<8; by++) {
                    for(bx=0; bx<8; bx++) {
                      if(z==0) {
                        drawPixelWindowColor(hDC,img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                        }
                      else if(z>0) {
                        fillRectangleWindowColor(hDC,img_ofs_x+(x1+bx)*z,img_ofs_y+(y1+by)*z,
                          img_ofs_x+(x1+bx)*z+bx*z,img_ofs_y+(y1+by)*z+by*z,Color565(*pSrcR,*pSrcG,*pSrcB));
                        }
                      else if(z<0) {
                        drawPixelWindowColor(hDC,img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                        bx+=-z; by+=-z;   // finire...
                        }
                      pSrcR++; pSrcG++; pSrcB++;
                      }
                    }
                  }
                }

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
error_compressed:
          
          SuperFileClose(&jpegFile);
          }
        }
      else if(stristr(nomefile,".bmp")) {
        }
      else if(stristr(nomefile,".txt")) {
        if(SuperFileOpen(&jpegFile,nomefile,'r')) {  
          char buf[128];
          int y=0;
          i=1;
          while(SuperFileGets(&jpegFile,buf,127)>0) {
            TextOut(hDC,0,y,buf);
            y+=8;
            SetScrollRange(hWnd,SB_VERT,0,y,FALSE);
            SetScrollPos(hWnd,SB_VERT,0,FALSE);
            }
          SuperFileClose(&jpegFile);
          }
        }
      if(!i)
        DrawText(hDC,"impossibile visualizzare il file",-1,&ps.rcPaint,
                DT_VCENTER | DT_CENTER);
        
      EndPaint(hWnd,&ps);
      }
      return 0;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
        fillRectangleWindowColor(hDC,hWnd->paintArea.left,hWnd->paintArea.top,
              hWnd->paintArea.right,hWnd->paintArea.bottom,WHITE);
      }
      return 1;
      break;
      
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA,0);    // pulisco
			memset(GET_WINDOW_OFFSET(hWnd,0),0,32);
      char *nomefile=(char *)cs->lpCreateParams;   // 
      if(nomefile) {
        strncpy(GET_WINDOW_OFFSET(hWnd,0),nomefile,31);
        SetWindowText(hWnd,nomefile); //"photoviewer"
        }
      return 0;
      }
      break;
    case WM_CLOSE:
      m_WndViewer=NULL;
      DestroyWindow(hWnd);
      break;
      
    case WM_CHAR:
      switch(wParam) {     //fare zoom
        int8_t t;
        case '+':
zoomin:
          t=GetWindowByte(hWnd,GWL_USERDATA);
          if(t<8) {
            t++;
            SetWindowByte(hWnd,GWL_USERDATA,t);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case '-':
          t=GetWindowByte(hWnd,GWL_USERDATA);
          if(t>-7) {
            t--;
            SetWindowByte(hWnd,GWL_USERDATA,t);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        }
      break;
    case WM_LBUTTONDBLCLK:
      goto zoomin;    // centrare! in culo a bechis sempre :D ;)
      break;
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
        switch(LOWORD(wParam)) {
          }
        }
      return 1;
      break;
      
      
//    case WM_RBUTTONDOWN:
      // fare menu context... info
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

