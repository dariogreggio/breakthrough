/*
 * BREAKTHROUGH 2.0
 * (C) G.Dar 1987-2023 :) !
 * (portions inspired by Windows, since 1990)
 * 
 * applicazioni: BASIC, OROLOGIO, CALCOLATRICE, FILE MANAGER, CALENDARIO, SURF!
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
extern DWORD extRAMtot;
extern struct KEYPRESS keypress;
extern struct MOUSE mouse;

const char *stristr(const char *,const char *);
uint8_t *memstr(const uint8_t *, const char *, int);

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
      
    case WM_CONTEXTMENU:
      // tipo memoria disponibile ecc o anche STOP
      MessageBox(hWnd,"info","BASIC",MB_OK | MB_ICONINFORMATION);
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
      
/* USARE ma occhio timezone  GCC di merda cancro ai nerd
      struct tm *oTime;
      time_t lTime;

      time(&lTime);
    	oTime=localtime(&lTime);
      */
      
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
        radius=min(ps.rcPaint.right,ps.rcPaint.bottom)/4.2;
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
      return 0;
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
		case WM_DISPLAYCHANGE: // Only comes through on plug'n'play systems
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      if(GetWindowByte(hWnd,GWL_USERDATA+0) & 2)        // gestire on top in menu!
        cs->style |= WS_EX_TOPMOST;
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
      {
      WINDOWPOS wpos;
      if(!(wpos.flags & SWP_NOSENDCHANGING))
        ;
      }
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
        sprintf(buffer,"%u",oTime->tm_mday);
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
        sprintf(buffer,"%u",oTime->tm_mday);
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
      fillRectangleWindowColor(hDC,hWnd->paintArea.left,hWnd->paintArea.top,
              hWnd->paintArea.right,hWnd->paintArea.bottom,GRAY242 /*GetSysColor(COLOR_WINDOWTEXT)*/);
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
      SetTimer(hWnd,1,65535 /*86400000*/,NULL);
      }
      return 0;
      break;
    case WM_WINDOWPOSCHANGED: 
      {
      WINDOWPOS wpos;
      if(!(wpos.flags & SWP_NOSENDCHANGING))
        ;
      }
      break;
    case WM_CLOSE:
      KillTimer(hWnd,1);
      DestroyWindow(hWnd);
      m_WndCalendar=NULL;
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


#define SURF_TIMEOUT 2000
extern const char *tagContentLength;
extern const char *tagContentType;
extern const char *tagConnection;
char *parseSurfItems(const char *, struct HTMLinfo *);
static int surfNavigate(HWND hWnd,const char *url) {
  SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
  int n;
  char buf[64];
	char *pagePtr,*p;
  
#ifdef USA_WIFI
  if(url) {
    SetWindowText((HWND)GetWindowLong(hWnd,0),url);
    if(!stricmp(url,"http://"))
      url+=7;
    strcpy(buf,url);
    }
  else {
//    ricaricare pagina ultima...
    }
  
  if(s==INVALID_SOCKET) {   // gestire wifi/ethernet
    struct sockaddr_in strAddr;
    BYTE u8Flags=0;   // boh??
    IP_ADDR dstIP;
    HWND myWnd;
    DWORD tOut,bytes2Read;
    BYTE connectionClose;
    struct HTMLinfo *myHtml;
    s = socket(AF_INET,SOCK_STREAM,u8Flags);
    if(s != INVALID_SOCKET) {
      SetWindowByte(hWnd,GWL_USERDATA+1,s);

      socketData[0].s=s;

      strAddr.sin_family = AF_INET;
      strAddr.sin_port = _htons(80);

      StringToIPAddress(buf,&dstIP);
      tOut=0;
      *(unsigned long*)internetBuffer=0;
      if(!dstIP.Val) {
        gethostbyname((uint8_t*)buf);
        while(!*(unsigned long*)internetBuffer && tOut<DNS_TIMEOUT) {
          m2m_wifi_handle_events(NULL);
          tOut++;
          __delay_ms(1);
          }
        dstIP.Val=*(unsigned long*)internetBuffer;
        }

      
//      dstIP.v[0]=192; dstIP.v[1]=168; dstIP.v[2]=1; dstIP.v[3]=2; 

      strAddr.sin_addr.s_addr = dstIP.Val;
      myWnd=(HWND)GetWindowLong(hWnd,12);
      if(myWnd) {
        sprintf(buf,"Connessione a %u.%u.%u.%u...",dstIP.v[0],dstIP.v[1],dstIP.v[2],dstIP.v[3]);
        SetWindowText(myWnd,buf);
        }
			SetWindowByte(hWnd,GWL_USERDATA+3,1);
			myWnd=(HWND)GetWindowLong(hWnd,8);
			myWnd->icon=surfWaitIcon;
			InvalidateRect(myWnd,NULL,TRUE);

      connect(s,(struct sockaddr*)&strAddr,sizeof(struct sockaddr_in));
      M2M_INFO("Surf connect\r\n");
      tOut=0;
      *(unsigned long*)internetBuffer=0;
      while(!*(unsigned long*)internetBuffer && tOut<SURF_TIMEOUT) {
        m2m_wifi_handle_events(NULL);
        tOut++;
        __delay_ms(1);
        }

      strcpy(buf,"GET / HTTP/1.1\r\n\r\n");
      send(s,buf,strlen(buf),0);
      *(unsigned long*)internetBuffer=0;
      while(!*(unsigned long*)internetBuffer && tOut<SURF_TIMEOUT) {
        m2m_wifi_handle_events(NULL);
        tOut++;
        __delay_ms(1);
        }

					SetWindowByte(hWnd,GWL_USERDATA+3,2);
					myWnd=(HWND)GetWindowLong(hWnd,8);
					myWnd->icon=surfWaitIcon;
					InvalidateRect(myWnd,NULL,TRUE);


			recv(s,rxBuffer,1536,SURF_TIMEOUT);
			tOut=0;
			while(!socketData[0].dataAvail && tOut<SURF_TIMEOUT) {
				m2m_wifi_handle_events(NULL);
				tOut++;
				__delay_ms(1);
				}
      
			if(p=strstr(rxBuffer,tagConnection)) {
        connectionClose=!strnicmp(p+strlen(tagConnection),"close",5);
        }
			if(p=strstr(rxBuffer,tagContentLength)) {
				bytes2Read=atoi(p)+strlen(tagContentLength);
				pagePtr=GET_WINDOW_OFFSET(hWnd,4+4+4+4+sizeof(struct HTMLinfo));
				n=0;

				SetWindowByte(hWnd,GWL_USERDATA+3,3);
				myWnd=(HWND)GetWindowLong(hWnd,8);
				myWnd->icon=surfWaitIcon;
				InvalidateRect(myWnd,NULL,TRUE);

				while(bytes2Read>0 && n<0x4000 /**/) {

					SetWindowByte(hWnd,GWL_USERDATA+3,1);
					myWnd=(HWND)GetWindowLong(hWnd,8);
					myWnd->icon=surfWaitIcon;
					InvalidateRect(myWnd,NULL,TRUE);

					recv(s,rxBuffer,1536,SURF_TIMEOUT);
					myWnd=(HWND)GetWindowLong(hWnd,12);
					if(myWnd) {
						sprintf(buf,"Lettura dati [%u]...",socketData[0].dataAvail);
						SetWindowText(myWnd,buf);
						}
					tOut=0;
					while(!socketData[0].dataAvail && tOut<SURF_TIMEOUT) {
						m2m_wifi_handle_events(NULL);
						tOut++;
						__delay_ms(1);
						}
					if(socketData[0].dataAvail>0) {
						n+=socketData[0].dataAvail;
						bytes2Read-=socketData[0].dataAvail;
						memcpy(pagePtr,rxBuffer,socketData[0].dataAvail);
						pagePtr+=socketData[0].dataAvail;
						socketData[0].dataAvail=0;
						}
          else
            break;
					}
				}
      else
        goto close;

      myWnd=(HWND)GetWindowLong(hWnd,12);
      if(myWnd) {
        sprintf(buf,"Letti %u bytes",n);
        SetWindowText(myWnd,buf);
        }

			myHtml=(struct HTMLinfo *)GET_WINDOW_OFFSET(hWnd,4+4+4+4);
			myHtml->fLen=n;
			if(n>0) {
				pagePtr=GET_WINDOW_OFFSET(hWnd,4+4+4+4+sizeof(struct HTMLinfo));
    		parseSurfItems(pagePtr,myHtml);
				}
      
			SetWindowByte(hWnd,GWL_USERDATA+3,4 /**/);
			myWnd=(HWND)GetWindowLong(hWnd,8);
			myWnd->icon=surfIcon;
			InvalidateRect(myWnd,NULL,TRUE);

			if(connectionClose) {   // 
close:
			  close(s);
				SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);		// inutile qua ma ok :)
				}	

      }
    }
#endif
  InvalidateRect(hWnd,NULL,TRUE);
  return n;
  }
int8_t surfNavigateStop(HWND hWnd) {
  SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
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
struct HYPER_LINK *link1;
char *skipSurfSpaces(char *p) {

	while(*p == ' ')
		p++;
	return p;
	}
int getSurfNParm(char *s,char *t,int n) {
	char *p;
	int i;

	p=(char*)stristr(s,t);
	if(p) {
		while(*p != '=')
			p++;
		p++;
		p=(char*)skipSpaces(p);
		if(isdigit(*p)) {
			return atoi(p);
			}
		else if(*p=='+') {
			skipSpaces(p);
			return atoi(p)+n;
			}
		else if(*p=='-') {
			skipSpaces(p);
			return n-atoi(p);
			}

		}
	return 0;
  }
char *getSurfSParm(char *s,char *t,char *buf) {
	char *p;
	int i;

	p=(char*)stristr(s,t);
	if(p) {
		while(*p != '=')
			p++;
		p++;
		p=(char*)skipSpaces(p);
		if(isalnum(*p)) {
			do {
				*buf++=*p++;
				} while(*p != ' ');
			*buf=0;
			return buf;
			}
		else if(*p=='\"') {
			p++;
			do {
				*buf++=*p++;
				} while(*p != '\"');
			*buf=0;
			return buf;
			}
		}
	return 0;
  }
FONT Font1[4];
char *parseSurfItems(const char *p, struct HTMLinfo *h) {
	char myBuf[256],myBuf1[256];
	char ch;
	char *pCmd;
	BOOL bFlag,bExit,bExitW;
	BOOL bInTitle,bInHead,bNegate;
	register int i,n;

	h->bkColor=GRAY204;
	h->textColor=BLACK;
	h->vlinkColor=h->hlinkColor=LIGHTMAGENTA;
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
			  if(bInHead) {
					if(!strnicmp(pCmd,"META",4)) {
						if(getSurfSParm(pCmd,"NAME",myBuf1)) {
							if(!stricmp(myBuf1,"generator")) {
								getSurfSParm(pCmd,"CONTENT",myBuf1);
								strcpy(h->sGenerator,myBuf1);
								}
							else if(!stricmp(myBuf1,"description")) {
								getSurfSParm(pCmd,"CONTENT",myBuf1);
								strcpy(h->sDescription,myBuf1);
								}
							}
						}
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
struct HYPER_LINK *addToSurfLinks(int x, int y, int cx, int cy, char *s) {
	struct HYPER_LINK *hl,*hl1;

	hl=(struct HYPER_LINK *)malloc(sizeof(struct HYPER_LINK));
	hl->rc.top=y;
	hl->rc.left=x;
	hl->rc.bottom=y+cy;
	hl->rc.right=x+cx;
	strcpy(hl->text,s);
	hl->visto=0;
	hl->next=0;
	if(link1) {
		hl1=link1;
		while(hl1->next)
			hl1=hl1->next;
		hl1->next=hl;
		}
	else {
		link1=hl;
		}
	return link1;
  }
struct HYPER_LINK *deleteSurfLinks(struct HYPER_LINK *l) {
	struct HYPER_LINK *hl,*hl1;

	hl1=link1;
	while(hl1) {
		hl=hl1;
		hl1=hl1->next;
		free(hl);
		}

	link1=NULL;
	return link1;
	}
struct HYPER_LINK *getSurfLink(POINT pt) {
	struct HYPER_LINK *hl=link1;

	while(hl) {
		if(PtInRect(&hl->rc,pt)) {
			return hl;
			}
		hl=hl->next;
		}
	return NULL;
	}
static void surfRender(HWND hWnd,HDC hDC,const char *pDoc,struct HTMLinfo *myHtml) {
	LOGBRUSH lb;
	char *p,*p1,myBuf[512],*pCmd;
	int i,j,n,x,y,ch;
	int cFont=2,cOldFont=2;
	GFX_COLOR cColor,cOldColor;
	BOOL bExit=FALSE,bExitW=FALSE,bOK,bNegate=FALSE;
	BOOL bInLink=FALSE,bInFont=FALSE,bInP=FALSE,bInTitle=FALSE;
	char tLink[64];
	FONT oldFont;
	BRUSH oldBrush;
	PEN oldPen;
	RECT rc;
	SIZE sz;

	link1=deleteSurfLinks(link1);

	lb.lbStyle=BS_SOLID;
	lb.lbColor=myHtml->bkColor;
	lb.lbHatch=0;
	BRUSH Brush1=CreateBrushIndirect(&lb);
	PEN Pen1=CreatePen(PS_SOLID,1,myHtml->bkColor);
	GetClientRect(hWnd,&rc);
	oldPen=SelectObject(hDC,OBJ_PEN,(GDIOBJ)Pen1).pen;
	oldBrush=SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)Brush1).brush;
	Rectangle(hDC,rc.left,rc.top,rc.right,rc.bottom);
	oldFont=SelectObject(hDC,OBJ_FONT,(GDIOBJ)Font1[cFont]).font;
	cColor=myHtml->textColor;
	SetTextColor(hDC,cColor);
	SetBkColor(hDC,myHtml->bkColor);
	p=(char*)pDoc;
	ySurfMax=xSurfMax=0;
	bOK=TRUE;
	x=1;
	y=1;
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
					break;
				case 10:
				  if(n)
					  myBuf[n++]=' ';
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
					if(bNegate)
						cFont=cOldFont;
					else {
						cOldFont=cFont;
						cFont=getSurfNParm(pCmd,"size",cOldFont);
						if(cFont < 0)
							cFont=0;
						if(cFont > 6)
							cFont=6;
						cOldColor=cColor;
						cColor=getSurfNParm(pCmd,"color",cOldColor);
						}
					}
				else if(!strnicmp(pCmd,"PRE",3)) {
					}
				else if(!strnicmp(pCmd,"IMG",3)) {
					}
				else if(!strnicmp(pCmd,"LI",2)) {
					}
				else if(!strnicmp(pCmd,"A",1)) {
					bInLink=!bNegate;
					if(bInLink)
						getSurfSParm(pCmd,"HREF",tLink);
					}
				else if(!strnicmp(pCmd,"P",1)) {
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
			if(bInLink) {
				if(1)
					SetTextColor(hDC,myHtml->hlinkColor);
				}
			else
				SetTextColor(hDC,cColor);
			SelectObject(hDC,OBJ_FONT,(GDIOBJ)Font1[cFont]);
			j=strlen(myBuf);
			GetTextExtentPoint(hDC,myBuf,j,&sz);
			if((x+sz.cx) > (rc.right-rc.left)) {
				ySurfMax+=sz.cy;
				x=1;
				if(ySurfMax>=ySurfPos) 
					y+=sz.cy;
				}
			if(ySurfMax>=ySurfPos) {
 				TextOut(hDC,x,y,myBuf);
				if(bInLink) {
					link1=addToSurfLinks(x,y,sz.cx,sz.cy,tLink);
				  }
				}
			x+=sz.cx;
			}
//		if(y > rc.bottom-rc.top)			// questo incasina il conto di yMax...
//			bExit=1;
		} while(!bExit);

//	wsprintf(myBuf,"ypos=%d, ymax=%d",yPos,yMax);
//	AfxMessageBox(myBuf);

  SetScrollRange(hWnd,SB_VERT,0,ySurfMax,FALSE);
  SetScrollRange(hWnd,SB_HORZ,0,xSurfMax,FALSE);
  SetScrollPos(hWnd,SB_VERT,ySurfPos,FALSE);
  SetScrollPos(hWnd,SB_HORZ,xSurfPos,FALSE);
	SelectObject(hDC,OBJ_FONT,(GDIOBJ)oldFont);
  SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)oldBrush);
  SelectObject(hDC,OBJ_PEN,(GDIOBJ)oldPen);
  DeleteObject(OBJ_BRUSH,(GDIOBJ)Brush1);
	DeleteObject(OBJ_PEN,(GDIOBJ)Pen1);
  }
static void surfRenderSource(HWND hWnd,HDC hDC,const char *pDoc) {
  
  SetTextColor(hDC,WHITE);
  if(socketData[0].dataAvail) {
    char *p=(char*)pDoc,buf[2];
    int n=socketData[0].dataAvail;
    int x=0,y=0;
    while(n--) {
      switch(*p) {
        case 13:
          break;
        case 10:
          x=0;
          y+=getFontHeight(&hDC->font);
          break;
        default:
          buf[0]=*p; buf[1]=0;
          TextOut(hDC,x,y+12+2,buf);
          x+=getFontWidth(&hDC->font);
          break;
        }
      p++;
      }
    }
  }
LRESULT surfWndProc(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int i;
      BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
			struct HTMLinfo *myHtml=(struct HTMLinfo *)GET_WINDOW_OFFSET(hWnd,4+4+4+4);

      hDC->pen=CreatePen(1,1,LIGHTGREEN);
      hDC->brush=CreateSolidBrush(GREEN);
      SetTextColor(hDC,LIGHTGREEN);
      
      TextOut(hDC,2,2,"Indirizzo:");
      drawHorizLineWindow(hDC,0,12,ps.rcPaint.right-1);

      ps.rcPaint.top+=14; ps.rcPaint.bottom-=24;
      
      SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
      if(s != INVALID_SOCKET) {
//    		parseSurfItems(rxBuffer,&myHtml);
//        surfRender(hWnd,hDC,rxBuffer,&myHtml);
        surfRenderSource(hWnd,hDC,rxBuffer);
// finire        internetBufferLen=0;
      
        }
      
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
        hWnd,(HMENU)201,NULL
        );
      SetWindowLong(hWnd,0,(DWORD)myWnd);
      myWnd=CreateWindow(MAKECLASS(WC_BUTTON),"Vai",WS_BORDER | WS_VISIBLE | WS_TABSTOP |
        WS_CHILD | BS_CENTER | BS_PUSHBUTTON,
        cs->cx-34-12,1,30,10,
        hWnd,(HMENU)202,NULL
        );
      SetWindowLong(hWnd,4,(DWORD)myWnd);
      myWnd=CreateWindow(MAKECLASS(WC_STATIC),"",WS_BORDER | /*WS_VISIBLE | per icon dopo! */WS_CHILD | 
        /*WS_DISABLED | */ SS_ICON,
        cs->cx-11,1,8,8,
        hWnd,(HMENU)203,NULL
        );
      myWnd->icon=redBallIcon;
      SetWindowLong(hWnd,8,(DWORD)myWnd);
			if(attrib & 1) {   // statusbar
        myWnd=CreateWindow(MAKECLASS(WC_STATUS),"idle",WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED /*| SS_ICONs */,
          0,cs->cy-10,cs->cx,cs->cy,
          hWnd,(HMENU)204,NULL
          );
   			SetWindowLong(hWnd,12,(DWORD)myWnd);
				}
      if(url) {   // non so se è bene farlo da qua...
        SetWindowText((HWND)GetWindowLong(hWnd,0),url);   // appunto!
        surfNavigate(hWnd,url);
        }
      }
      return 0;
      break;
    case WM_CLOSE:
			{SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
		  if(s != INVALID_SOCKET) {   // gestire wifi/ethernet
			  close(s);
				SetWindowByte(hWnd,GWL_USERDATA+1,INVALID_SOCKET);		// inutile qua ma ok :)
				}	
      KillTimer(hWnd,1);
      m_WndSurf=NULL;
      DestroyWindow(hWnd);
			}
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_RETURN:     // mmm credo che arrivi da notify del control EDIT..
          {HWND myWnd;
          myWnd=(HWND)GetWindowLong(hWnd,0);
          if(SendMessage(myWnd,WM_GETTEXTLENGTH,0,0) > 0)
            SendMessage(myWnd,WM_COMMAND,MAKEWORD(202,BN_CLICKED),0);
          } 
          break;
        case VK_BROWSER_BACK:
          break;
        case VK_BROWSER_FORWARD:
          break;
        case VK_BROWSER_REFRESH:
keyboard_reload:
           {SOCKET s=GetWindowByte(hWnd,GWL_USERDATA+1);
            if(s != INVALID_SOCKET) {   // gestire wifi/ethernet
              close(s);
              }
            surfNavigate(hWnd,NULL);
            }
          break;
        case VK_BROWSER_STOP:
keyboard_stop:
          surfNavigateStop(hWnd);
          break;
        case VK_BROWSER_SEARCH:
          SendMessage(hWnd,WM_COMMAND,MAKELONG(96+3,0),0);
          break;
        case VK_BROWSER_FAVORITES:
          break;
        case VK_BROWSER_HOME:
          {
          char buf[32],*homepage=buf;
          GetProfileString(profileFile,"SURF","homepage",buf,"192.168.1.2");
          SendMessage(hWnd,WM_SURF_NAVIGATE,0,(LPARAM)homepage);
          }
          break;
        case VK_ESCAPE:     // o accelerator??
          goto keyboard_stop;
          break;
        }
      break;
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
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
          case 32+1:    // reload
            goto keyboard_reload;
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
            WriteProfileInt(profileFile,"SURF","attributi",attrib);
            // creare/distruggere finestra static..
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
            char *url;   // 
            HWND myWnd=(HWND)GetWindowLong(hWnd,0);
//      myWnd=(HWND)GetWindowLong(hWnd,0);
//      GetWindowText(myWnd,buf,31);
            url=(char*)SendMessage(myWnd,WM_GETTEXT,0,0);
//            surfNavigate(hWnd,url);
            surfNavigate(hWnd,"192.168.1.2");
            
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
    case WM_SETCURSOR:
      switch(GetWindowByte(hWnd,GWL_USERDATA+3)) {
        case 0:   // IDLE
          return (DWORD)&standardCursorSm;
          break;
        case 1:   // connecting
          return (DWORD)&hourglassCursorSm;
          break;
        case 2:   // connesso
          return (DWORD)&standardCursorSm;
          break;
        case 3:   // download
          return (DWORD)&hourglassCursorSm;
          break;
        case 4:   // finito
          return (DWORD)&standardCursorSm;
          break;
        }
      break;
    case WM_RBUTTONDOWN:    // non dovrebbe servire...
    case WM_CONTEXTMENU:
      {char buf[32];
			struct HTMLinfo *myHtml=(struct HTMLinfo *)GET_WINDOW_OFFSET(hWnd,4+4+4+4);
#ifdef USA_WIFI
      sprintf(buf,"%u bytes",myHtml->fLen);
#endif
      MessageBox(hWnd,buf,"Pagina",MB_OK | MB_ICONINFORMATION);
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
      
    case WM_SURF_NAVIGATE:
      {
      char *url=(char *)lParam;   // 
      if(url) {
        HWND myWnd=(HWND)GetWindowLong(hWnd,0);
        SetWindowText(myWnd,url);
        surfNavigate(hWnd,url); 
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
          
  				int x,x1,mcu_x;
          int y,y1,mcu_y;
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
            else if(z>0) for(x=0; x<z; x++) {
              img_ofs_y *= 2;
              img_ofs_x *= 2;
              }
            SetScrollRange(hWnd,SB_VERT,0,((ps.rcPaint.bottom-ps.rcPaint.top)*JPG_Info.m_height) / (ps.rcPaint.bottom-ps.rcPaint.top),FALSE);
            SetScrollPos(hWnd,SB_VERT,img_ofs_y/2,FALSE);
            SetScrollRange(hWnd,SB_HORZ,0,((ps.rcPaint.right-ps.rcPaint.left)*JPG_Info.m_width) / (ps.rcPaint.left-ps.rcPaint.right),FALSE);
            SetScrollPos(hWnd,SB_HORZ,img_ofs_x/2,FALSE);
            img_ofs_y=((int)(ps.rcPaint.bottom-ps.rcPaint.top)-(int)img_ofs_y)/2;
            img_ofs_x=((int)(ps.rcPaint.right-ps.rcPaint.left)-(int)img_ofs_x)/2;
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
                  zcnty=(-z)+1;
                  for(by=0; by<8; by++) {
                    zcntx=(-z)+1;
                    for(bx=0; bx<8; bx++) {
                      if(z==0) {
                        drawPixelWindowColor(hDC,img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                        }
                      else if(z>0) {
                        fillRectangleWindowColor(hDC,img_ofs_x+(x1+bx)*(z+1),img_ofs_y+(y1+by)*(z+1),
                          img_ofs_x+(x1+bx)*(z+1)+bx*(z+1),img_ofs_y+(y1+by)*(z+1)+by*(z+1),Color565(*pSrcR,*pSrcG,*pSrcB));
                        }
                      else if(z<0) {
                        zcntx--;
                        if(!zcntx) {
                          drawPixelWindowColor(hDC,img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                          zcntx=(-z)+1;
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
              if(mcu_x >= JPG_Info.m_MCUSPerRow) {
                mcu_x=0;
                mcu_y++;
                if(mcu_y>ps.rcPaint.bottom)
                  break;
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
      else if(stristr(nomefile,".txt") || stristr(nomefile,".ini")) {
        SetTextColor(hDC,BLACK);
        SetBkColor(hDC,WHITE);
        if(SuperFileOpen(&jpegFile,nomefile,'r')) {  
          char buf[128];
          int y=0;
          i=1;
          while(SuperFileGets(&jpegFile,buf,127)>=0) {
            if(y<ps.rcPaint.bottom)   // lascio cmq scorrere per scrollbar!
              TextOut(hDC,0,y,buf);
            y+=getFontHeight(&hDC->font);
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
        case VK_SPACE:
        case '0':
          SetWindowByte(hWnd,GWL_USERDATA,0);
          InvalidateRect(hWnd,NULL,TRUE);
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
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

