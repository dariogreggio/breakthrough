/*
 * BREAKTHROUGH 2.2
 * (C) G.Dar 1987-2026 :) !
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

#include "harmony_app.h"


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
#include "breakthrough_private.h"
#include "fat_sd/FSIO.h"
#if defined(USA_USB_HOST_MSD)
#include "../harmony_pic32mz/usb_host_msd.h"
#include "../harmony_pic32mz/usb_host_scsi.h"
#endif
#include "fat_ide/idefsio.h"
#include "superfile.h"

#ifdef USA_USB_HOST_UVC   // usare BBitBlt per immagine a video ? ma in finestra...
#include "breakthrough_uvc.h"
#endif

#include "picojpeg.h"
#include "kommissarRexx/kommissarRexx.h"

extern enum FILE_DEVICE m_stdout,m_stdin,m_stderr;
extern const char *ASTERISKS;
extern char prompt[16];
extern signed char currDrive;
extern APP_DATA appData;
extern Ipv4Addr myIp;
#if defined(USA_WIFI) || defined(USA_ETHERNET)
extern NETWORKFILE networkFile;
extern BYTE myRSSI;
#endif
#ifdef USA_USB_HOST_UVC		// occhio la usiamo pure in breakthrough
#include "usb_host_uvc.h"
extern BYTE *VideoBufferPtr2;
extern DWORD VideoBufferLen2;
extern enum CAPTURING_VIDEO capturingVideo;
#endif

extern volatile unsigned long now;
extern MEDIA_INFORMATION SDMediaInfo,HDMediaInfo,RAMMediaInfo,FDCMediaInfo,USBMediaInfo;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern BYTE volumeAudio;
extern BYTE eXtra;
extern uint16_t screenSaverTime;

extern THREAD *rootThreads;


LRESULT DefWindowProcStaticWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i,x,y;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORSTATIC*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }

      if(hWnd->style & SS_ENDELLIPSIS) {  //SS_PATHELLIPSIS SS_WORDELLIPSIS
        }
      switch(hWnd->style & SS_TYPEMASK) {
        case SS_SIMPLE:   // if the control is disabled, the control does not gray its text.
        case SS_LEFTNOWORDWRAP:
        case SS_LEFT:
          SetTextColor(hDC,(hWnd->enabled || (hWnd->style & SS_SIMPLE)) ? WHITE : GRAY192);
          SetBkColor(hDC,c);
          DrawText(hDC,hWnd->caption,-1,&rc,DT_VCENTER | DT_LEFT |
            (hWnd->style & SS_NOPREFIX ? DT_NOPREFIX : 0) | 
            ((hWnd->style & SS_TYPEMASK == SS_LEFTNOWORDWRAP) ? DT_SINGLELINE : 0));
          break;
        case SS_CENTER:
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY192);
          SetBkColor(hDC,c);
          DrawText(hDC,hWnd->caption,-1,&rc,DT_VCENTER | DT_CENTER |
            (hWnd->style & SS_NOPREFIX ? DT_NOPREFIX : 0));
          break;
        case SS_RIGHT:
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY192);
          SetBkColor(hDC,c);
          DrawText(hDC,hWnd->caption,-1,&rc,DT_VCENTER | DT_RIGHT |
            (hWnd->style & SS_NOPREFIX ? DT_NOPREFIX : 0));
          break;
        case SS_ICON:
          if(hWnd->icon)
            drawIcon8(hDC,rc.left,rc.top,hWnd->icon);
          break;
        case SS_BITMAP:
//          DrawIcon(hDC,0,0,hWnd->icon);
          break;
        case SS_BLACKFRAME:
          SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BLACK));
          drawRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          break;
        case SS_GRAYFRAME:
          SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,LIGHTGRAY));
          drawRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          break;
        case SS_WHITEFRAME:
          SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,WHITE));
          drawRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          break;
        case SS_BLACKRECT:
          fillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,BLACK);
          break;
        case SS_GRAYRECT:
          fillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,GRAY192);
          break;
        case SS_WHITERECT:
          fillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,WHITE);
          break;
        case SS_SUNKEN:
          break;
        case SS_OWNERDRAW:
          {
          DRAWITEMSTRUCT dis;
          dis.CtlID=(DWORD)hWnd->menu;
          dis.itemID=0;
          dis.itemState=GetWindowByte(hWnd,GWL_USERDATA+1);   // TRADURRE! ODS_CHECKED ecc
          dis.hwndItem=hWnd;
          dis.hDC=hDC;
          dis.rcItem=rc;
          dis.CtlType=ODT_STATIC;
          SendMessage(hWnd->parent,WM_DRAWITEM,(WPARAM)(DWORD)hWnd->menu,(LPARAM)&dis);
          }
          break;
        }
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      int c=hDC->brush.color;
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORSTATIC*/,
          wParam,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      i=cs->style & WS_BORDER ? 2 : 0;
      i+=cs->style & WS_THICKFRAME ? 2 : 0;
// qua non uso      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      if(!(cs->style & SS_NOTIFY))    // bah..
        cs->style |= WS_DISABLED;   // 
      if((cs->style & 0xff) == SS_ICON)     // e poi SS_REALSIZEIMAGE
        cs->cx=cs->cy=8 + i-1; // type ecc..
//NO! multiline ecc      else      //andrebbe quindi poi gestito in SIZE setfont ecc!
//        cs->cy=getFontHeight(&hWnd->font) + i-1; // font è già valido qua??
      }
      return 0;
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char*)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case STM_SETICON:
      hWnd->icon=(ICON)wParam; 
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case STM_SETIMAGE:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case STM_GETICON:
      return (LRESULT)hWnd->icon;
      break;
    case STM_GETIMAGE:
      break;
    case WM_SETFOCUS:
      if(hWnd->style & SS_NOTIFY) {   // bah..
        hWnd->focus=1;
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_SETFOCUS),(LPARAM)hWnd);
        }
      return 0;
      break;
    case WM_KILLFOCUS:
      hWnd->focus=0;
      return 0;
      break;

    case WM_NCHITTEST:
      if(!(hWnd->style & SS_NOTIFY))
        return HTTRANSPARENT;
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_LBUTTONDOWN:
      if(hWnd->style & SS_NOTIFY && hWnd->parent /*&& hWnd->enabled*/)
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((BYTE)(DWORD)hWnd->menu,STN_CLICKED),(LPARAM)hWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_LBUTTONDBLCLK:
      if(hWnd->style & SS_NOTIFY && hWnd->parent /*&& hWnd->enabled*/)
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((BYTE)(DWORD)hWnd->menu,STN_DBLCLK),(LPARAM)hWnd);
      break;
/* verificare che fa, trasforma static in edit??    case STN_ENABLE:
      break;
    case STN_DISABLE:
 */
    case WM_WINDOWPOSCHANGING:
      {WINDOWPOS *wpos=(WINDOWPOS*)lParam;
        int i;
        i=hWnd->style & WS_BORDER ? 2 : 0;
        i+=hWnd->style & WS_THICKFRAME ? 2 : 0;
        BYTE height=getFontHeight(&hWnd->font);
        if(hWnd->style & SS_ICON)
          wpos->cy=8+i-1;
        else
          wpos->cy=height+i-1;
      }
      break;

    case WM_GETDLGCODE: 
      return DLGC_STATIC;
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcToolbarWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_NCPAINT:  // fondamentalmente, per font piccolo
      {BYTE flags=DC_SMALLCAP;
      RECT rc;
      HDC hDC;
      DC myDC;
      
      hDC=GetWindowDC(hWnd,&myDC);
      if(hWnd->style & WS_SYSMENU)
        flags |= DC_BUTTONS;
      if((hWnd->style & (WS_CAPTION | WS_CHILD)) == WS_CAPTION) {
        flags |= DC_BUTTONS;
        flags |= DC_TEXT;
        }
      if(hWnd->active)
        flags |= DC_ACTIVE;
      SendMessage(hWnd,WM_NCCALCSIZE,FALSE,(LPARAM)&hWnd->clientArea);
      rc=hWnd->nonClientArea;
      if(wParam) {
        IntersectRect(&rc,&rc,(RECT*)wParam);
        }
      fillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,windowBackColor);
      DrawEdge(hDC,&rc,0,BF_TOPLEFT | BF_BOTTOMRIGHT);
      DrawCaption(hWnd,hDC,&rc,flags);
      ReleaseDC(hWnd,&myDC);
      }
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      cs->style &= ~(WS_SYSMENU | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME);
      
      }
      return 0;
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcEditWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  char *s;
  POINT pt;
  BYTE startSel,endSel;
  if(GetWindowLong(hWnd,0) != 0xffffffff)    // se buffer allocato apposta..
    s=(char*)GET_WINDOW_OFFSET(hWnd,0);
  else
    s=(char*)GetWindowLong(hWnd,4);
  startSel=GetWindowByte(hWnd,GWL_USERDATA+2);
  endSel=GetWindowByte(hWnd,GWL_USERDATA+3);
  pt.x=GetWindowByte(hWnd,GWL_USERDATA+0);   // curpos
  
  switch(message) {
    case WM_PAINT:
      {
      int i,x,y;
      PAINTSTRUCT ps;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR  /*WM_CTLCOLOREDIT*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      
      if(hWnd->parent)
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_UPDATE),(LPARAM)hWnd);
      
      i=getFontWidth(&hDC->font);
      x=rc /*ps.rcPaint gestire*/.left/i; y=rc.top /* /getFontHeight(&hDC->font)*/;
      SetTextColor(hDC,GetWindowByte(hWnd,GWL_USERDATA+1) & 8 ? GRAY192 : WHITE); // READONLY
      if(hWnd->style & ES_AUTOHSCROLL) {
        }
      if(hWnd->style & ES_RIGHT) {
        }
      while(*s && x<ps.rcPaint.right/i && y<ps.rcPaint.bottom) {
        if(startSel || endSel) {
          if(hWnd->style & ES_NOHIDESEL) {
            }
          if(x>=startSel && x<=endSel)
            SetBkColor(hDC,BLUE);
          else
            SetBkColor(hDC,c);
          }
        else
          SetBkColor(hDC,c);
        if(hWnd->style & ES_MULTILINE) {
          if(hWnd->style & ES_AUTOVSCROLL) {
            }
          switch(*s) {
            case '\r':
              x=-1;   // incr sotto!
              break;
            case '\n':
              x=-1; y+=getFontHeight(&hWnd->font);
              break;
            default:
              goto normalchar;
              break;
            }
          }
        else {
normalchar:
          if(hWnd->style & ES_PASSWORD)
            drawCharWindow(hDC,x*i,y,'*');
          else
            drawCharWindow(hDC,x*i,y,*s);
          }
        x++;
        s++;
        }
      EndPaint(hWnd,&ps);
      
//      SetScrollRange(hWnd,SB_VERT,  
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      int c=hDC->brush.color;
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLOREDIT*/,
          wParam,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      if(!(cs->style & ES_MULTILINE)) {
        BYTE height=getFontHeight(&hWnd->font);
        i=cs->style & WS_BORDER ? 2 : 0;
        i+=cs->style & WS_THICKFRAME ? 2 : 0;
        cs->cy=height+i-1;
        }
      else {
        cs->style |= WS_VSCROLL;
        hWnd->scrollSizeY=0;
        }
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      *(DWORD*)s=0;    // azzero content
      strcpy(s,cs->lpszName);    // gestire lunghezza, in caso...
      if(hWnd->style & ES_LOWERCASE)
        strlwr(s);
      if(hWnd->style & ES_UPPERCASE)
        strupr(s);
      if(!(hWnd->style & WS_DISABLED))		// boh sì :) per pulizia
				caretPosition.x=caretPosition.y=0;
      }
      return 0;
      break;
    case WM_WINDOWPOSCHANGING:
      {WINDOWPOS *wpos=(WINDOWPOS*)lParam;
        int i;
        i=hWnd->style & WS_BORDER ? 2 : 0;
        i+=hWnd->style & WS_THICKFRAME ? 2 : 0;
        BYTE height=getFontHeight(&hWnd->font);
        if(!(hWnd->style & ES_MULTILINE)) {
          wpos->cy=height+i-1;
          }
      }
      break;

    case WM_DESTROY:
      KillTimer(hWnd,31);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_LBUTTONDOWN:
      {
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // per prova!
      SetWindowByte(hWnd,GWL_USERDATA+3,0);
      //startsel ecc
      
      BYTE x=GET_X_LPARAM(lParam) / getFontWidth(&hWnd->font);
      x=min(x,63); 
      x=min(x,GetWindowByte(hWnd,GWL_USERDATA+0));
      SetWindowByte(hWnd,GWL_USERDATA+0,x);
      caretPosition.x=x *getFontWidth(&hWnd->font);
			caretPosition.y= 0* getFontHeight(&hWnd->font);
      InvalidateRect(hWnd,NULL,TRUE);
      if(hWnd->enabled)
        SendMessage(hWnd,WM_SETFOCUS,0,0);
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_LBUTTONDBLCLK:    // FINIRE ! selezionare solo parola
      SetWindowByte(hWnd,GWL_USERDATA+2,0);
      SetWindowByte(hWnd,GWL_USERDATA+3,strlen(s));
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_DELETE:
          goto do_del;
          break;
        case VK_LEFT:
left_arrow:
          if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
          // gestire estensione select se shift premuto con frecce..
            }
          else {
            if(pt.x > 0)
              SetWindowByte(hWnd,GWL_USERDATA+0,--pt.x);
            if(hWnd->style & ES_MULTILINE) {
              }
            goto updatecaret;
            }
          break;
        case VK_RIGHT:
right_arrow:
          if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
          // gestire estensione select se shift premuto con frecce..
            }
          else {
            if(pt.x < strlen(s))
              SetWindowByte(hWnd,GWL_USERDATA+0,++pt.x);
            if(hWnd->style & ES_MULTILINE) {
              }
            goto updatecaret;
            }
          break;
        case VK_UP:
          if(hWnd->style & ES_MULTILINE) {
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
          // gestire estensione select se shift premuto con frecce..
              }
            else {
              }
            }
					else
						goto left_arrow;		// windows fa così (almeno in rename file, verificare!
          break;
        case VK_DOWN:
          if(hWnd->style & ES_MULTILINE) {
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {
          // gestire estensione select se shift premuto con frecce..
              }
            else {
              }
            }
					else
						goto right_arrow;		// windows fa così
          break;
        case VK_HOME:
          pt.x=0;
          SetWindowByte(hWnd,GWL_USERDATA+0,pt.x);
          if(hWnd->style & ES_MULTILINE) {
            if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {
              pt.y=0;
//              SetWindowByte(hWnd,     ,pt.y);
              }
            }
          goto updatecaret;
          break;
        case VK_END:
          pt.x=strlen(s); 
          SetWindowByte(hWnd,GWL_USERDATA+0,pt.x);
          if(hWnd->style & ES_MULTILINE) {
            if(GetAsyncKeyState(VK_CONTROL) & 0x8000)
              ; // finire pt.y=0;
            }
          goto updatecaret;
          break;
        }
      break;
    case WM_CHAR:
      { char buf[8];
      int i;
      DC myDC;
      HDC hDC;
      
      if(GetWindowByte(hWnd,GWL_USERDATA+1) & 8)    // READONLY
        return 0;
      
//      if(hWnd->enabled) {
        if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {   // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
          // non so se van gestiti qua o da fuori... 
          switch(wParam) {
            case 'C':
              goto do_copy;
              break;
            case 'X':
              goto do_cut;
              break;
            case 'V':
              goto do_paste;
              break;
            case 'A':
              SetWindowByte(hWnd,GWL_USERDATA+2,0);
              SetWindowByte(hWnd,GWL_USERDATA+3,strlen(s));
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }
          }
        else {
          switch(wParam) {
            case '\x8':
              if(pt.x > 0) {
                SetWindowByte(hWnd,GWL_USERDATA+0,--pt.x); 
                s[pt.x]=0;
                }
              InvalidateRect(hWnd,NULL,TRUE); // per velocità!
updatecaret:
              hDC=GetDC(hWnd,&myDC);
              drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,FALSE);
              caretPosition.x=pt.x *getFontWidth(&hDC->font);
              caretPosition.y=pt.y *getFontHeight(&hDC->font); 
              ReleaseDC(hWnd,hDC);    
              break;
            case '\r':   // gestire tasti strani..
              if(hWnd->style & ES_MULTILINE) {
                if(hWnd->style & ES_WANTRETURN) {
// tipo...        SendMessage(hWnd,WM_COMMAND,MAKELONG((WORD)myWnd->tag,BN_CLICKED),myWnd);
                  }
                caretPosition.x=0; caretPosition.y++;
                // finire
                SetWindowByte(hWnd,GWL_USERDATA+0,0);   // 
                }
              else {
                }
              break;
            default:
              if(isprint(LOBYTE(wParam))) {
                buf[0]=LOBYTE(wParam); buf[1]=0;
                if((hWnd->style & ES_NUMBER) && !isdigit(buf[0]))
                  goto no_char;
                if(hWnd->style & ES_UPPERCASE)
                  buf[0]=toupper(buf[0]);
                if(hWnd->style & ES_LOWERCASE)
                  buf[0]=tolower(buf[0]);
                hDC=GetDC(hWnd,&myDC);
                pt.y=0;   // curpos
                s[pt.x]=buf[0]; s[pt.x+1]=0;
    //              TextOut(hDC,pt.x *6 * hDC->font.size,pt.y  *8*hDC->font.size,buf); PER ORA FACCIO PRIMA a Invalidate!
                if(pt.x<64   )       // curpos
                  pt.x++;
                if(hWnd->style & ES_MULTILINE) {
                  }
                ReleaseDC(hWnd,hDC);
changed:
                SetWindowByte(hWnd,GWL_USERDATA+0,pt.x);   // 
                InvalidateRect(hWnd,NULL,TRUE);

clearSel:
                SetWindowByte(hWnd,GWL_USERDATA+2,0);   // elimino selezione sempre
                SetWindowByte(hWnd,GWL_USERDATA+3,0);   // 
                
                SetWindowByte(hWnd,GWL_USERDATA+1,GetWindowByte(hWnd,GWL_USERDATA+1) | 2);
                if(1)//Supported in Microsoft Rich Edit 1.0 and later. To receive EN_CHANGE notification codes, specify ENM_CHANGE in the mask sent with the EM_SETEVENTMASK message
                  PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd); 
                goto updatecaret;
no_char:
                break;
              }
            }
          }
//        }
//      else
//        return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_SETFOCUS:
      if(hWnd->enabled) {
      if(!hWnd->focus) {
        ShowCaret(hWnd);
//        drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,TRUE);
        SetTimer(hWnd,31,caretTime*10,NULL);
        }
      
//                InvalidateRect(hWnd,NULL,TRUE);
      
      PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_SETFOCUS),(LPARAM)hWnd);
      hWnd->focus=1;
      }
      return 0;
      break;
    case WM_KILLFOCUS:
      if(hWnd->enabled) {
      HideCaret(hWnd);
      //drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,FALSE);
      KillTimer(hWnd,31);
      
      PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_KILLFOCUS),(LPARAM)hWnd);
      hWnd->focus=0;
      }
      return 0;
      break;
    case WM_SETTEXT:
      {
      strncpy(s,(const char*)lParam,   64-1);
      if(!(hWnd->style & ES_MULTILINE)) {
        s[   64-1]=0;
        } // se no finire
      SetWindowByte(hWnd,GWL_USERDATA+0,strlen(s));   // curpos ecc
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // 
      SetWindowByte(hWnd,GWL_USERDATA+3,0);   // 
      InvalidateRect(hWnd,NULL,TRUE);
      caretPosition.x=(DWORD)GetWindowByte(hWnd,GWL_USERDATA+0)*getFontWidth(&hWnd->font); 
      if(!(hWnd->style & ES_MULTILINE))
        caretPosition.y= 0;
      else
        caretPosition.y= 0 /*getFontHeight(&hWnd->font) gestire*/;
      if(!(hWnd->style & ES_MULTILINE)) //The EN_CHANGE notification code is not sent when the ES_MULTILINE style is used and the text is sent through WM_SETTEXT.
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd);
      }
      return 1;
      break;
    case WM_GETTEXT:
      {
      int i=min(strlen(s)+1,wParam);
      strncpy((char*)lParam,s,i);
      ((char*)lParam)[i]=0;
      return i;
      }
      break;
    case WM_GETTEXTLENGTH:
      return strlen(s);
      break;

    case EM_CANUNDO:
      return GetWindowByte(hWnd,GWL_USERDATA+1) & 4;
      break;
    case EM_CHARFROMPOS:
      {
      if(!(hWnd->style & ES_MULTILINE)) {
        BYTE x=GET_X_LPARAM(lParam) / getFontWidth(&hWnd->font);
        if(x<63)
          return s[x];
        }      
      }      
      break;
    case EM_GETLINE:
      if(!(hWnd->style & ES_MULTILINE)) {
        strcpy((char*)lParam,s);
        return strlen((char*)lParam);
        }
      else {
// fare...        return strcpy((char*)lParam,s);
        }
      break;
    case EM_GETMARGINS:
      break;
    case EM_GETMODIFY:
      return GetWindowByte(hWnd,GWL_USERDATA+1) & 2;
      break;
    case EM_SETMODIFY:
      SetWindowByte(hWnd,GWL_USERDATA+1,
        wParam ? (GetWindowByte(hWnd,GWL_USERDATA+1) | 2) : (GetWindowByte(hWnd,GWL_USERDATA+1) & ~2));
      return 0;
      break;
    case EM_GETHANDLE:
      if(GetWindowLong(hWnd,0) != 0xffffffff && hWnd->style & ES_MULTILINE)    // se buffer allocato apposta..
        return GetWindowLong(hWnd,4);
      else
        return (DWORD)NULL;
      break;
    case EM_LIMITTEXT:
//    case EM_SETLIMITTEXT:
      break;
    case EM_LINEINDEX:
      if(!(hWnd->style & ES_MULTILINE)) {
        return 0;   // direi!
        }
      else {
        return 0;   // fare...
        }
      break;
    case EM_LINEFROMCHAR:
      if(!(hWnd->style & ES_MULTILINE)) {
        return 0;
        }
      else {
        if(wParam == 0xffffffff)
          return 0;   // poi finire :)
        else
          return 0;
        }      
      break;
    case EM_LINELENGTH:
      break;
    case EM_POSFROMCHAR:
      {
      DC myDC;
      HDC hDC;
      int n;
      n=(DWORD)GetWindowByte(hWnd,GWL_USERDATA+0)* getFontWidth(&hWnd->font);
      return n;
      }      
      break;
    case EM_SCROLL:
    case WM_VSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_HSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case EM_SETHANDLE:
      if(hWnd->style & ES_MULTILINE) {
        SetWindowLong(hWnd,0,0xffffffff);    // indico buffer a parte
        SetWindowLong(hWnd,4,wParam);
        SetWindowByte(hWnd,GWL_USERDATA+1,GetWindowByte(hWnd,GWL_USERDATA+1) & ~(2 | 4));   // MODIFY, CANUNDO
        InvalidateRect(hWnd,NULL,TRUE);
        }
      break;
    case EM_SETPASSWORDCHAR:
      break;
    case EM_SETREADONLY:
      SetWindowByte(hWnd,GWL_USERDATA+1,GetWindowByte(hWnd,GWL_USERDATA+1) | 8);   // 
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case EM_SETEVENTMASK:
      break;
    case EM_SETSEL:
      {
      int i;
      i=strlen(s);
      wParam=min(wParam,i);
      lParam=min(lParam,i);
      if(wParam>=lParam) 
        wParam=lParam=0;    // 
      SetWindowByte(hWnd,GWL_USERDATA+2,wParam);
      SetWindowByte(hWnd,GWL_USERDATA+3,lParam);
      // muovere cursore qua? caretposition ecc
      InvalidateRect(hWnd,NULL,TRUE);
      }
      return 1;
      break;
    case EM_SETTABSTOPS:
      break;
//    case EM_TAKEFOCUS:
//      break;
//    case EM_NOSETFOCUS:
//      break;
    case WM_UNDO:
    case EM_UNDO:
      break;
      
    case WM_COPY:
do_copy:
      if(OpenClipboard(hWnd)) {
        if(startSel && endSel) {
          BYTE ch=s[endSel];
          s[endSel]=0;
          SetClipboardData(CF_TEXT,s+startSel);    // 
          s[endSel]=ch;
          }
        else
          SetClipboardData(CF_TEXT,s);    // 
        CloseClipboard();
        }
      break;
    case WM_CUT:
do_cut:
      if(OpenClipboard(hWnd)) {
        if(startSel && endSel) {
          s[endSel]=0;
          SetClipboardData(CF_TEXT,s+startSel);    // 
          strcpy(s+startSel,s+endSel+1);
          pt.x=strlen(s); 
          }
        else {
          SetClipboardData(CF_TEXT,s);    // 
          s[pt.x=0]=0;
          }
        CloseClipboard();
        goto changed;
        }
      break;
    case WM_PASTE:
do_paste:
      if(OpenClipboard(hWnd)) {
        if(GetClipboardData(CF_TEXT))    // 
          if(startSel && endSel) {
            //fare!
            }

        CloseClipboard();
        pt.x=strlen(s); 
        goto changed;   // in teoria solo cf_text ma cmq ok
        }
      break;
    case WM_CLEAR:
do_del:
      if(startSel && endSel) {
        strcpy(s+startSel,s+endSel+1); 
        pt.x=strlen(s); 
        }
      else
        s[pt.x=0]=0;
      goto changed;
      break;
      
    case WM_GETDLGCODE:
      return DLGC_HASSETSEL | DLGC_WANTCHARS | DLGC_WANTARROWS;
      break;

    case WM_TIMER:
      {
      if(wParam==31) {
        SetWindowByte(hWnd,GWL_USERDATA+1,GetWindowByte(hWnd,GWL_USERDATA+1) ^ 1);
        drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,GetWindowByte(hWnd,GWL_USERDATA+1) & 1);
        }
      }
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

static void listbox_bubble_sort(struct _LISTITEM **head) {    //https://stackoverflow.com/questions/21388916/bubble-sort-singly-linked-list-in-c-with-pointers
  int done = 0;         // True if no swaps were made in a pass

  // Don't try to sort empty or single-node lists
  if(!*head || !((*head)->next))
    return;

  while(!done) {
    struct _LISTITEM **pv = head;         // "source" of the pointer to the current node in the list struct
    struct _LISTITEM *nd = *head;            // local iterator pointer
    struct _LISTITEM *nx = (*head)->next;  // local next pointer

    done = 1;

    while(nx) {
      if(nd->id > nx->id) {   // ordino da più basso a più alto 
				// if(strcmp(nd->data,nx->next)>0) {
        COMPAREITEMSTRUCT cis;
        // SendMessage(hWnd->parent,WM_COMPAREITEM,idCtl,cis);
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
static LISTITEM *getListItemFromCnt(LISTITEM *item,int n) {
  while(item && n>0) {
    item=item->next;
    n--;
    }
  return item;
  }


void subDrawList(HWND hWnd,HDC hDC,RECT *rc,LISTITEM *item,BYTE type) {
  int x,y;
  int c=hDC->brush.color;
  GFX_COLOR c2=hWnd->active ? windowForeColor : windowInactiveForeColor;
  
  if(hWnd->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) {    // i CBS_ sono identici qua!
    DRAWITEMSTRUCT dis;
    dis.CtlID=(DWORD)hWnd->menu;
    dis.itemID=0;
    dis.itemState=GetWindowByte(hWnd,GWL_USERDATA+1);   // TRADURRE! ODS_CHECKED ecc
    dis.hwndItem=hWnd;
    dis.hDC=hDC;
    dis.rcItem=*rc;
    dis.CtlType=type ? ODT_COMBOBOX : ODT_LISTBOX;
    SendMessage(hWnd->parent,WM_DRAWITEM,(WPARAM)(DWORD)hWnd->menu,(LPARAM)&dis);
    }
  else {
    if(hWnd->style & WS_CHILD) {
      if((c=SendMessage(hWnd->parent,WM_CTLCOLOR  /*WM_CTLCOLORLISTBOX*/,
        (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
        c=hDC->brush.color;
        }
      }

    x=0; y=0;
    while(item && y<hWnd->scrollPosY) {   // finire...
      item=item->next;
      y++;
      }
    y=rc->top  /*/getFontHeight(&hWnd->font)*/;
    while(item) {
      if(item->state & 1) {
        SetTextColor(hDC,BLACK /*c*/);
        SetBkColor(hDC,c2);
        fillRectangleWindowColor(hDC,rc->left,y,rc->right,y+getFontHeight(&hWnd->font),
          c2);
        }
      // in effetti Windows non sembra usare colore attivo/inattivo, ma sempre attivo; e quando la finestra si disattiva la riga pare deselezionarsi.
      else {
        SetBkColor(hDC,c);
        SetTextColor(hDC,c2);
        fillRectangleWindowColor(hDC,rc->left,y,rc->right,y+getFontHeight(&hWnd->font),c);
        }
      if(!type) {
        if(hWnd->style & LBS_USETABSTOPS)
          ;
        }
      TextOut(hDC,x+1,y,item->data,strlen(item->data));    // finire :)
      y+=getFontHeight(&hWnd->font);
      drawHorizLineWindowColor(hDC,rc->left,y++,rc->right,c2);
      if(y>=rc->bottom)
        break;
      item=item->next;
      }
    // gestire scrollbar, CBS_DISABLENOSCROLL LBS_DISABLENOSCROLL
    }
  }

LRESULT DefWindowProcListboxWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int i;
  LISTITEM *item=(LISTITEM*)GetWindowLong(hWnd,0);    // listitem *
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      subDrawList(hWnd,hDC,&rc,item,0);

      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      int c=hDC->brush.color;
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORLISTBOX*/,
          wParam,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero OCCHIO SAREBBE da usare 0xffff 0xffff
      SetWindowLong(hWnd,0,0);    // azzero root
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      }
      return 0;
      break;
    case WM_DESTROY:
      {LISTITEM *item2;
      while(item) {
        item2=item->next;
        free(item);
        item=item2;
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_SETREDRAW:
      if(wParam)
        hWnd->style &= ~LBS_NOREDRAW;
      else
        hWnd->style |= LBS_NOREDRAW;
      break;
    case WM_KEYDOWN:
      {int j;
//        if(hWnd->enabled) {   // ma se è disabled arrivano o no? penso di no..
        switch(wParam) {
          case VK_HOME:
            j=GetWindowWord(hWnd,GWL_USERDATA+0);
            if(j>=0 && item)
              SendMessage(hWnd,LB_SETSEL,0,j);
            SendMessage(hWnd,LB_SETSEL,1,0);
//              SetWindowWord(hWnd,GWL_USERDATA+0,0);
            hWnd->scrollPosY=0;
            break;
          case VK_END:
            j=GetWindowWord(hWnd,GWL_USERDATA+0);
            if(j>=0 && item)
              SendMessage(hWnd,LB_SETSEL,0,j);
            j=GetWindowWord(hWnd,GWL_USERDATA+2)-1;
            SendMessage(hWnd,LB_SETSEL,1,j);
//              SetWindowWord(hWnd,GWL_USERDATA+0,j);
//              hWnd->scrollPosY=0;     // finire...
            break;
          case VK_UP:
            j=GetWindowWord(hWnd,GWL_USERDATA+0);
            if(j>0) {
              SendMessage(hWnd,LB_SETSEL,0,j);
              j--;
              SendMessage(hWnd,LB_SETSEL,1,j);
//                SetWindowWord(hWnd,GWL_USERDATA+0,j);
              }
            break;
          case VK_DOWN:
            j=GetWindowWord(hWnd,GWL_USERDATA+0);
            if(j<GetWindowWord(hWnd,GWL_USERDATA+2)-1) {
              SendMessage(hWnd,LB_SETSEL,0,j);
              j++;
              SendMessage(hWnd,LB_SETSEL,1,j);
// inutile                SetWindowWord(hWnd,GWL_USERDATA+0,j);
              }
            break;
          case VK_SPACE:
            if(item)
              item->state ^= 1;
            if(!(hWnd->style & LBS_NOREDRAW))    // solo se cambiata?
              InvalidateRect(hWnd,NULL,TRUE);
            break;
          case VK_RETURN:
            if(SendMessage(hWnd,LB_GETCURSEL,0,0) != LB_ERR)
              goto select_list;
            break;
          default:
            break;
          }
//        }
  //      else
  //        return DefWindowProc(hWnd,message,wParam,lParam);
//    if(style & LBS_WANTKEYBOARDINPUT)
//    SendMessage(GetParent(hWnd),WM_CHARTOITEM,,);    // fare, dice
      }
      return 0;
      break;
    case WM_CHAR:
      { char buf[8];
        int j;
//        if(hWnd->enabled) {   // ma se è disabled arrivano o no? penso di no..
          if(isalnum(wParam)) {
            buf[0]=wParam; buf[1]=0;
            SendMessage(hWnd,LB_SELECTSTRING,0xffffffff,(LPARAM)buf);    // prova!!
            }
  //        }
  //      else
  //        return DefWindowProc(hWnd,message,wParam,lParam);
//    if(style & LBS_WANTKEYBOARDINPUT)
//    SendMessage(GetParent(hWnd),WM_CHARTOITEM,,);    // fare, dice
      }
      return 0;
      break;
    case WM_GETDLGCODE: 
      return DLGC_WANTCHARS | DLGC_WANTARROWS;
      break;
      
    case WM_LBUTTONDOWN:
      {
      i=SendMessage(hWnd,LB_ITEMFROMPOINT,0,lParam);
      if(i != LB_ERR) {
        int j;
        j=SendMessage(hWnd,LB_GETCURSEL,0,0);
        if(j!=LB_ERR) 
          SendMessage(hWnd,LB_SETSEL,0,j);
        SendMessage(hWnd,LB_SETSEL,1,i);
//inutile        SetWindowWord(hWnd,GWL_USERDATA+0,i);
        }
      if(hWnd->style & LBS_NOSEL)
        ;
      if(hWnd->style & LBS_EXTENDEDSEL)
        ;
      if(hWnd->style & LBS_NOTIFY) {
        if(hWnd->parent) {
          NMHDR nmh;
          PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,LBN_SELCHANGE),(LPARAM)hWnd);
          nmh.code=LBN_SELCHANGE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // volendo NMHDR ecc
          }
        }
      if(!(hWnd->style & LBS_NOREDRAW))    // solo se cambiata?
        InvalidateRect(hWnd,NULL,TRUE);
      } 
//      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_LBUTTONDBLCLK:
select_list:
      if(hWnd->style & LBS_NOSEL)
        ;
      if(hWnd->style & LBS_EXTENDEDSEL)
        ;
      if(hWnd->style & LBS_NOTIFY) {
        if(hWnd->parent) {
          NMHDR nmh;
          nmh.code=LBN_DBLCLK;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // finire
          }
        }
      if(!(hWnd->style & LBS_NOREDRAW))    // solo se cambiata?
        InvalidateRect(hWnd,NULL,TRUE);
      break;

    case LB_ADDSTRING:
      {LISTITEM *item2;
      i=0;
      item2=item;
      if(item) {
        while(item2 && item2->next) {
          i++;
          item2=item2->next;
          }
        item2->next=malloc(sizeof(LISTITEM));
        item2=item2->next;
        }
      else
        item=item2=malloc(sizeof(LISTITEM));
      i++;
      item2->next=NULL;
      strncpy(item2->data,(char*)lParam,sizeof(item2->data)-1);
      item2->data[sizeof(item2->data)-1]=0;     
      item2->state=0; item2->id=0;
      if(hWnd->style & LBS_SORT)
        listbox_bubble_sort(&item);
      SetWindowLong(hWnd,0,(DWORD)item);
//      SetWindowWord(hWnd,GWL_USERDATA+0,i);
      SetWindowWord(hWnd,GWL_USERDATA+2,i);
      
updatelista:
      {
      RECT rc;
      int j;
      GetClientRect(hWnd,&rc);
      i=SendMessage(hWnd,LB_GETCOUNT,0,0);
      rc.bottom=(rc.bottom-rc.top)/getFontHeight(&hWnd->font);
      if(i>=rc.bottom) {
//          EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
        j=SendMessage(hWnd,LB_GETCURSEL,0,0);
        if(j != LB_ERR) {
          SetScrollRange(hWnd,SB_VERT,0,1+i-rc.bottom,FALSE);
          SetScrollPos(hWnd,SB_VERT,j>rc.bottom ? j-rc.bottom : 0,FALSE);
          }
        else
          SetScrollRange(hWnd,SB_VERT,0,1+i-rc.bottom,FALSE);
        ShowScrollBar(hWnd,SB_VERT,TRUE); //
        }
      else {
        if(hWnd->style & LBS_DISABLENOSCROLL) {
          }
        else {
//            EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
          SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
          SetScrollPos(hWnd,SB_VERT,0,FALSE);
          ShowScrollBar(hWnd,SB_VERT,FALSE); //
          }
        }
      }
      if(!(hWnd->style & LBS_NOREDRAW))
        InvalidateRect(hWnd,NULL,TRUE);
      return i;
      }
      break;
    // unire con dlgdir dirdlg??
    case LB_ADDFILE:  // serve qualche campo in più per data, dim, tipo file..
      {LISTITEM *item2;
      i=0;
      item2=item;
      if(item) {
        while(item2 && item2->next) {
          i++;
          item2=item2->next;
          }
        item2->next=malloc(sizeof(LISTITEM));
        item2=item2->next;
        }
      else
        item=item2=malloc(sizeof(LISTITEM));
      i++;
      item2->next=NULL;
      strncpy(item2->data,(char*)lParam,sizeof(item2->data)-1);
      item2->data[sizeof(item2->data)-1]=0;
      item2->state=0; item2->id=0;
      if(hWnd->style & LBS_SORT)
        listbox_bubble_sort(&item);
      SetWindowLong(hWnd,0,(DWORD)item);
      SetWindowWord(hWnd,GWL_USERDATA+0,i);
      goto updatelista;
      }
      break;
    case LB_DIR:  // serve qualche campo in più per data, dim, tipo file..
      {
      SearchRec rec;
      char *lpPathSpec=(char*)lParam;
      char drive;
      BYTE attr;
      MEDIA_INFORMATION *mi;

      if(lpPathSpec[1]==':')
        drive=lpPathSpec[0];
      else
        drive=currDrive;
      
      SendMessage(hWnd,LB_RESETCONTENT,0,0);

      attr=ATTR_NORMAL;   // DDL_READWRITE
      attr |= wParam & DDL_DIRECTORY ? ATTR_DIRECTORY : 0;
      attr |= wParam & DDL_HIDDEN ? ATTR_HIDDEN : 0;
      attr |= wParam & DDL_SYSTEM ? ATTR_SYSTEM : 0;
      attr |= wParam & DDL_READONLY ? ATTR_READ_ONLY : 0;
      attr &= ~ATTR_VOLUME;   // cmq

      mi=getMediaInfo(drive);
      if(!mi || mi->errorCode != MEDIA_NO_ERROR)
no_disc:
        return 0;
      switch(drive) {   //
        case 'A':
        case 'B':
        case 'C':
        case 'D':
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
#endif
#ifdef USA_USB_HOST_MSD
        case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
        case DEVICE_NETWORK:
#endif
          i=SuperFileFindFirst(drive,lpPathSpec, attr, &rec);
          break;
        default:
          goto no_disc;
          break;
        }

      if(!i) {
loop:
        SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM)rec.filename);

        switch(drive) {
          case 'A':
          case 'B':
          case 'C':
          case 'D':
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
#endif
#ifdef USA_USB_HOST_MSD
          case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case DEVICE_NETWORK:
#endif
            if(!SuperFileFindNext(drive,&rec))
              goto loop;
          }
        }
      SetWindowWord(hWnd,GWL_USERDATA+0,0);
      SetWindowWord(hWnd,GWL_USERDATA+2,i);
      return 1;
      }
      break;
    case LB_INSERTSTRING:   // no SORT qua!
      {LISTITEM *item2;
      i=0;
      item2=item;
      if(item) {
        if(wParam == 0xffffffff) {
fine_lista:
          while(item2 && item2->next) {
            i++;
            item2=item2->next;
            }
	        item2->next=malloc(sizeof(LISTITEM));
		      item2=item2->next;
		      item2->next=NULL;
          }
        else {
          if(wParam==0) {   // se 0, lo gestisco a parte!
  	        item2=malloc(sizeof(LISTITEM));
  		      item2->next=item;
            item=item2;
            }
          else {
            item2=getListItemFromCnt(item,wParam);
            if(!item2) {
              item2=item;
              goto fine_lista;
              }
            else {
              LISTITEM *item3=item2->next;
              item2->next=malloc(sizeof(LISTITEM));
              item2=item2->next;
              item2->next=item3;
              }
            }
          }
        }
      else {
        item=item2=malloc(sizeof(LISTITEM));
	      item2->next=NULL;
				}
      i++;
      strncpy(item2->data,(char*)lParam,sizeof(item2->data)-1);
      item2->data[sizeof(item2->data)-1]=0;
      item2->state=0; item2->id=0;
      SetWindowLong(hWnd,0,(DWORD)item);
      SetWindowWord(hWnd,GWL_USERDATA+2,i);
      goto updatelista;
      }
      break;
    case LB_DELETESTRING:
      {LISTITEM *item2;
      item2=item;
      while(wParam-- && item) {
        item2=item;
        item=item->next;
        }
      if(item2==(LISTITEM*)GetWindowLong(hWnd,0)) {
        SetWindowLong(hWnd,0,(DWORD)item->next);
        SetWindowWord(hWnd,GWL_USERDATA+0,0);
        SetWindowWord(hWnd,GWL_USERDATA+2,0);
        }
      else {
        item2->next=item->next;
//        SetWindowWord(hWnd,GWL_USERDATA+0,i);
        SetWindowWord(hWnd,GWL_USERDATA+2,GetWindowWord(hWnd,GWL_USERDATA+2)-1);
        }
      free(item);
      goto updatelista;
      }
      break;
    case LB_RESETCONTENT:
      {LISTITEM *item2;
      while(item) {
        item2=item->next;
        free(item);
        item=item2;
       }
      SetWindowLong(hWnd,0,(DWORD)NULL);

      SetWindowLong(hWnd,GWL_USERDATA+0,0);   // + rapido!

      goto updatelista;
      }
      break;
    case LB_SETSEL:
      {LISTITEM *item2;
      if(lParam==0xffffffff) {
        while(item) {
          if(wParam)
            item->state |= 1;
          else
            item->state &= ~1;
          item=item->next;
          }
        lParam=0;
        if(wParam)
          SetWindowWord(hWnd,GWL_USERDATA+0,0);
        }
      else {
        item=getListItemFromCnt(item,lParam);
        if(item) {
          if(wParam)
            item->state |= 1;
          else
            item->state &= ~1;
          if(hWnd->style & LBS_MULTIPLESEL)   // gestire..
            ;
          if(wParam)
            SetWindowWord(hWnd,GWL_USERDATA+0,lParam);
          }
        else
          return LB_ERR;
        }
      }
      goto updatelista;
      break;
    case LB_GETSEL:
      {
      item=getListItemFromCnt(item,lParam);
      if(item)
        return item->state & 1 ? (LRESULT)item : 0;
      else
        return LB_ERR;
      }
      break;
    case LB_GETTEXT:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)item->data;
      else
        return LB_ERR;
      }
      break;
    case LB_GETTEXTLEN:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)strlen(item->data);
      else
        return LB_ERR;
      }
      break;
    case LB_GETCOUNT:   //The index is zero-based, so the returned count is one greater than the index value of the last item.
      {
      i=1;
      while(item) {
        i++;
        item=item->next;
        }
      SetWindowWord(hWnd,GWL_USERDATA+2,i-1);
      return i;
      }
      break;
    case LB_GETCURSEL:
      {
      i=0;
      while(item) {
        if(item->state & 1) {
          SetWindowWord(hWnd,GWL_USERDATA+0,i);
          return i;
          }
        i++;
        item=item->next;
        }
      return LB_ERR;
      }
      break;
    case LB_GETSELITEMS:
      if(hWnd->style & LBS_MULTIPLESEL) {
        DWORD *ptr=(DWORD*)lParam;
        int i=0;
        while(item && wParam) {
          if(item->state & 1) {
            *ptr++=i;
            wParam--;
            }
          i++;
          item=item->next;
          }
        return i;
        }
      else
        return LB_ERR;
      break;
    case LB_GETSELCOUNT:
      {
      i=0;
      while(item) {
        if(item->state & 1)
          i++;
        item=item->next;
        }
      return i;
      }
      break;
    case LB_GETITEMDATA:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)item->data;
      else
        return LB_ERR;
      }
      break;
    case LB_SETITEMDATA:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        strcpy(item->data,(char*)lParam);
      else
        return LB_ERR;
      }
      goto updatelista;
      break;
    case LB_FINDSTRING:
    case LB_SELECTSTRING:
      {
      i=0;
      if(wParam != 0xffffffff)
        item=getListItemFromCnt(item,wParam);
      while(item) {
        if(!strnicmp(item->data,(char*)lParam,strlen((char*)lParam))) {
          int j;
          if(message==LB_SELECTSTRING) {
            j=SendMessage(hWnd,LB_GETCURSEL,0,0);
            if(j!=LB_ERR)
              SendMessage(hWnd,LB_SETSEL,0,j);
            SendMessage(hWnd,LB_SETSEL,1,i);
//            SetWindowWord(hWnd,GWL_USERDATA+0,i);
            goto updatelista;
            }
          return i;
          break;
          }
        i++;
        item=item->next;
        }
      }
      return 0;
      break;
    case LB_INITSTORAGE:
      //LB_ERRSPACE
      return 0;
      break;
    case LB_SETCOUNT:
      break;
    case LB_SETITEMHEIGHT:
      // USARE CREATEFONT!
      hWnd->font.base16=max(lParam,6);   // no, è per ownerdraw, singola riga :) ma ok
      goto updatelista;
      break;
    case LB_GETITEMHEIGHT:
      return hWnd->font.base16;
      break;
    case LB_ITEMFROMPOINT:
      {int y;
      y=GET_Y_LPARAM(lParam);
      y /= hWnd->font.base16+1;   // così al volo!
      return MAKELONG(y,  0 /*1 se fuori client area...?! forse scrollbar */);
      }
      break;
    case LB_GETITEMRECT:
      {RECT *rc;
      rc=(RECT*)lParam;
      GetClientRect(hWnd,rc);
      rc->top=wParam*(getFontHeight(&hWnd->font)+1);
      // ovviamente finire... trovare chi è visualizzato e la posizione...
      rc->bottom=rc->top+(getFontHeight(&hWnd->font)+1);
      return 0;
      }
      break;
    case LB_GETTOPINDEX:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return 0;     // finire!
      else
        return LB_ERR;
      }
      break;
    case LB_GETLISTBOXINFO:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return 0;     // finire! items per column
      else
        return LB_ERR;
      }
      break;
    case LB_SETHORIZONTALEXTENT:
      // finire... https://learn.microsoft.com/en-us/windows/win32/controls/lb-sethorizontalextent
      hWnd->scrollSizeX=wParam;
      if(wParam) {
//        EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH); 
        ShowScrollBar(hWnd,SB_HORZ,TRUE); //
        }
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    case WM_HSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_VSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    case WM_SETFOCUS:
      if(hWnd->enabled) {
        if(!hWnd->focus) {
          hWnd->focus=1;
          InvalidateRect(hWnd,NULL,TRUE);
          }
        }
      break;
    case WM_KILLFOCUS:
      if(hWnd->enabled) {
        if(hWnd->focus) {
          hWnd->focus=0;
          InvalidateRect(hWnd,NULL,TRUE);
          }
        }
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

#define COMBO_ARROW_SIZE 9   // legare a fontheight
//secondo lui, è in effetti composto da un Edit+freccia e un List ... https://forums.codeguru.com/showthread.php?412242-ComboBox-drawing-amp-painting
//  p.es. scrollbar sarebbero più semplici... v. ncpaint
LRESULT DefWindowProcComboboxWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int i;
  char *s;
  POINT pt;
  BYTE startSel,endSel;
  BYTE itemsNumber;
  s=hWnd->caption;
  startSel=GetWindowByte(hWnd,4+2);
  endSel=GetWindowByte(hWnd,4+3);
  pt.x=GetWindowByte(hWnd,4+0);   // curpos
  itemsNumber=GetWindowByte(hWnd,4+4);   // numero di righe
  LISTITEM *item=(LISTITEM*)GetWindowLong(hWnd,GWL_USERDATA);    // listitem *
  // qua si potrebbe spostare nei singoli case dove serve
  
  switch(message) {
    case WM_NCPAINT:
      // GESTIRE le scrollbar in modo speciale! horiz solo de dropped e vert idem e solo per la lista
      if(GetWindowByte(hWnd,4+1) & 0x80) {    // se dropped
//        DWORD oldStyle=hWnd->style;
//        hWnd->style &= ~(WS_HSCROLL | WS_VSCROLL);
        // non va benissimo cmq, verificare
        // e BISOGNA abbassare la posizione della scrollbar verticale! o fare sotto in paint
        nonClientPaint(hWnd,(RECT*)wParam,hWnd->active,FALSE/*TRUE*/);   // 
//        hWnd->style = oldStyle;
        }
      else {
        nonClientPaint(hWnd,(RECT*)wParam,hWnd->active,FALSE/*TRUE*/);   // 
        }
      return 0;
      break;
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int x,y;
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      RECT rc;
      GetClientRect(hWnd,&rc);

      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR  /*WM_CTLCOLORLISTBOX*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      
      SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,hWnd->active ? windowForeColor : windowInactiveForeColor));
      drawVertArrowWindow(hDC,rc.right-(COMBO_ARROW_SIZE-2),rc.top+1,
        COMBO_ARROW_SIZE-3,COMBO_ARROW_SIZE-4,FALSE);
      drawVertLineWindow(hDC,rc.right-COMBO_ARROW_SIZE,rc.top,rc.top+COMBO_ARROW_SIZE);
      drawVertLineWindow(hDC,rc.right-SCROLL_SIZE+1,rc.top+COMBO_ARROW_SIZE,rc.bottom);
      
      rc.right-=COMBO_ARROW_SIZE;
      
// qua??      if(hWnd->parent)
//        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_UPDATE),(LPARAM)hWnd);
      
      
      i=getFontWidth(&hDC->font);
      x=rc /*ps.rcPaint gestire*/.left/i; y=rc.top /* /getFontHeight(&hDC->font)*/;
      if((hWnd->style & 0xf) == CBS_DROPDOWN) {
        }
// dubito che serva qua      SetTextColor(hDC,GetWindowByte(hWnd,0+1) & 8 ? GRAY192 : WHITE); // READONLY
      SetTextColor(hDC,WHITE);
      
      if(hWnd->style & CBS_AUTOHSCROLL) {
        }
      while(*s && x<rc.right/i) {
        if((hWnd->style & 0xf) == CBS_DROPDOWNLIST) {
          SetBkColor(hDC,c);
          }
        else {
          if(startSel || endSel) {
            if(x>=startSel && x<=endSel)
              SetBkColor(hDC,BLUE);
            else
              SetBkColor(hDC,c);
            }
          else
            SetBkColor(hDC,c);
          }
        drawCharWindow(hDC,x*i,y,*s++);
        x++;
        }
      
      rc.right+=COMBO_ARROW_SIZE;
      if(GetWindowByte(hWnd,4+1) & 0x80) {    // se dropped
        rc.top+= /*getFontHeight(&hDC->font)+2*/   COMBO_ARROW_SIZE-1;
        drawHorizLineWindow(hDC,rc.left,rc.top,rc.right);
        rc.right-=COMBO_ARROW_SIZE-1;
        rc.top+=1;
        
// da nonclientpaint
        if(hWnd->scrollSizeY) {
          RECT rc2=rc;
          int y2=rc.bottom-rc.top-2;
          y2=y2/hWnd->scrollSizeY; 
        y=hWnd->scrollPosY*y2;
          fillRectangleWindowColor(hDC,rc2.right-SCROLL_SIZE+2,rc2.top+2+y,
            rc2.right-2,rc2.top+2+y+y2,hDC->pen.color);
  //      drawVertArrowWindow(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE size,BYTE height,BYTE dir);
        //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLUP,DFCS_FLAT);
        //DrawFrameControl(hDC,&rc2,DFC_SCROLL | DFCS_SCROLLDOWN,DFCS_FLAT);
  
//          DFCS_SCROLLCOMBOBOX
          }
        if((hWnd->style & 0xf) == CBS_DROPDOWN)
          rc.left++;    // bah finezza che distingue dropdown da dropdownlist
        subDrawList(hWnd,hDC,&rc,item,BORDER_SIZE);
        }
      else {
        drawHorizLineWindow(hDC,rc.left,rc.bottom  ,rc.right); //ma ALLUNGARE un pelino :)
        }
      
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      int c=hDC->brush.color;
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORLISTBOX*/,
          wParam,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero list root
      SetWindowLong(hWnd,0,0);    // azzero varie variabili + text
      SetWindowLong(hWnd,4,0);
      SetWindowLong(hWnd,8,0);
      i=(cs->cy-COMBO_ARROW_SIZE)/COMBO_ARROW_SIZE;
      i=max(i,1);
      SetWindowByte(hWnd,4+4,i); // definisco il num di righe nella lista
//      SetWindowByte(hWnd,4+4,GetSystemMetrics(SM_CYSCREEN) >= 400 ? 5 : 3);    // itemsNumber (num righe), finezza :)
      cs->cy=COMBO_ARROW_SIZE +1;    // imposto a "solo riga testo" +1 per indicare list chiusa (v.paint
      if((hWnd->style & 0xf) == CBS_DROPDOWN) {
        }
      if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
        caretPosition.x=caretPosition.y=0;
        }
      if((hWnd->style & 0xf) == CBS_SIMPLE) {
        DC myDC;
        HDC hDC;
        SetWindowByte(hWnd,4+1,0x80);   // forzo dropped, ergo
        hDC=GetDC(hWnd,&myDC);
        cs->cy=COMBO_ARROW_SIZE+itemsNumber*(getFontHeight(&hDC->font)+1);
        ReleaseDC(hWnd,hDC);
        }
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      }
      return 0;
      break;
    case WM_DESTROY:
      {LISTITEM *item2;
      while(item) {
        item2=item->next;
        free(item);
        item=item2;
        }
      KillTimer(hWnd,31);
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_TIMER:
      if(wParam==31) {
        SetWindowByte(hWnd,4+1,GetWindowByte(hWnd,4+1) ^ 1);
        drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,GetWindowByte(hWnd,4+1) & 1);
        }
      break;
      
    case WM_KEYDOWN:
      {int j;
//        if(hWnd->enabled) {   // ma se è disabled arrivano o no? penso di no..
        switch(wParam) {
          case VK_HOME:
            j=GetWindowWord(hWnd,0+0);
            SendMessage(hWnd,CB_SETCURSEL,0,0);
//              SetWindowWord(hWnd,0+0,0);
            hWnd->scrollPosY=0;
            break;
          case VK_END:
            j=GetWindowWord(hWnd,0+2);
            SendMessage(hWnd,CB_SETCURSEL,j,0);
//              SetWindowWord(hWnd,0+0,j);
//              hWnd->scrollPosY=0;     // finire...
            break;
          case VK_UP:
            j=GetWindowWord(hWnd,0+0);
            if(j>0) {
              SendMessage(hWnd,CB_SETCURSEL,j-1,0);
//                SetWindowWord(hWnd,0+0,j);
              }
            break;
          case VK_DOWN:
            if(GetWindowByte(hWnd,4+1) & 0x80) {    // se era chiuso, lo apre
              j=GetWindowWord(hWnd,0+0);
              if(j<GetWindowWord(hWnd,0+2)) {
                SendMessage(hWnd,CB_SETCURSEL,j+1,0);
  // inutile                SetWindowWord(hWnd,0+0,j);
                }
              }
            else
              goto change_dropped;
            break;
          case VK_SPACE:    // in effetti ora su Win7 non sembra fare, ma io ricordo che sì :)
            if((hWnd->style & 0xf) == CBS_DROPDOWNLIST) {
              goto change_dropped;
              }
            break;
          case VK_RETURN:
            if((GetWindowByte(hWnd,4+1) & 0x80) &&
              SendMessage(hWnd,CB_GETCURSEL,0,0) != CB_ERR)
              goto select_combo;
            break;
          case VK_ESCAPE:
            if((hWnd->style & 0xf) != CBS_SIMPLE) {
              if(GetWindowByte(hWnd,4+1) & 0x80)
                goto change_dropped;
              }
            break;
          default:
            break;
          }
  //      }
  //      else
  //        return DefWindowProc(hWnd,message,wParam,lParam);
//    if(style & CB_WANTKEYBOARDINPUT)
//    SendMessage(GetParent(hWnd),WM_CHARTOITEM,,);    // fare, dice
      }
      return 0;
      break;
    case WM_CHAR:
      { char buf[8];
        int j;
        DC myDC;
        HDC hDC;
        
//        if(hWnd->enabled) {   // ma se è disabled arrivano o no? penso di no..
        if(GetAsyncKeyState(VK_CONTROL) & 0x8000) {   // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
          // non so se van gestiti qua o da fuori... 
/*          mettere ?!  switch(wParam) {
            case 'C':
              goto do_copy;
              break;
            case 'X':
              goto do_cut;
              break;
            case 'V':
              goto do_paste;
              break;
            case 'A':
              SetWindowByte(hWnd,4+2,0);
              SetWindowByte(hWnd,4+3,strlen(s));
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }*/
          }
        else {
          if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
            switch(wParam) {
              case '\x8':
                if(pt.x > 0) {
                  SetWindowByte(hWnd,4,--pt.x); 
                  s[pt.x]=0;
                  }
                InvalidateRect(hWnd,NULL,TRUE); // per velocità!
updatecaret:
                hDC=GetDC(hWnd,&myDC);
                drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,FALSE);
                caretPosition.x=pt.x *getFontWidth(&hDC->font);
                caretPosition.y=0;
                ReleaseDC(hWnd,hDC);    
                break;
              default:
                if(isprint(LOBYTE(wParam))) {
                  buf[0]=LOBYTE(wParam); buf[1]=0;
                  if(hWnd->style & CBS_UPPERCASE)
                    buf[0]=toupper(buf[0]);
                  if(hWnd->style & CBS_LOWERCASE)
                    buf[0]=tolower(buf[0]);
                  hDC=GetDC(hWnd,&myDC);
                  pt.y=0;   // curpos
                  s[pt.x]=buf[0]; s[pt.x+1]=0;
      //              TextOut(hDC,pt.x *6 * hDC->font.size,pt.y  *8*hDC->font.size,buf); PER ORA FACCIO PRIMA a Invalidate!
                  if(pt.x < (sizeof(hWnd->caption)-1))       // curpos
                    pt.x++;
                  ReleaseDC(hWnd,hDC);
changed:
                  SetWindowByte(hWnd,4,pt.x);   // 
                  InvalidateRect(hWnd,NULL,TRUE);

clearSel:
                  SetWindowByte(hWnd,4+2,0);   // elimino selezione sempre
                  SetWindowByte(hWnd,4+3,0);   // 

                  SetWindowByte(hWnd,4+1,GetWindowByte(hWnd,4+1) | 2);
                  if(1)//Supported in Microsoft Rich Edit 1.0 and later. To receive EN_CHANGE notification codes, specify ENM_CHANGE in the mask sent with the EM_SETEVENTMASK message
                    PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd); 
                  goto updatecaret;
                  break;
                }
              }
            }
        
          else {
// solo se dropped qua?      if(GetWindowByte(hWnd,4+1) & 0x80) {    // se dropped
            if(isalnum(wParam)) {
              buf[0]=wParam; buf[1]=0;
              SendMessage(hWnd,CB_SELECTSTRING,0xffffffff,(LPARAM)buf);    // prova!!
              }
            }
          }
  //        }
  //      else
  //        return DefWindowProc(hWnd,message,wParam,lParam);
//    if(style & CB_WANTKEYBOARDINPUT)
//    SendMessage(GetParent(hWnd),WM_CHARTOITEM,,);    // fare, dice
      }
      return 0;
      break;
    case WM_SETFOCUS:
      if(hWnd->enabled) {
        if(!hWnd->focus) {
          if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
            ShowCaret(hWnd);
      //        drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,TRUE);
            SetTimer(hWnd,31,caretTime*10,NULL);
            }
      
//                InvalidateRect(hWnd,NULL,TRUE);
      
          PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_SETFOCUS),(LPARAM)hWnd);
          hWnd->focus=1;
          }
        }
      return 0;
      break;
    case WM_KILLFOCUS:
      {
      if(hWnd->focus) {
        if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
          HideCaret(hWnd);
          //drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,FALSE);
          KillTimer(hWnd,31);
          }
      
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_KILLFOCUS),(LPARAM)hWnd);
        hWnd->focus=0;
        
        if((hWnd->style & 0xf) != CBS_SIMPLE) {
          if(GetWindowByte(hWnd,4+1) & 0x80)    // mah mi pare sia così
            goto change_dropped;
          }
        }
      }
      return 0;
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char*)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      if(hWnd->style & CBS_LOWERCASE)
        strlwr(hWnd->caption);
      else if(hWnd->style & CBS_UPPERCASE)
        strupr(hWnd->caption);
      BYTE x=strlen(hWnd->caption);
      SetWindowByte(hWnd,4,x);
      SetWindowByte(hWnd,4+2,0);   // elimino selezione sempre
      SetWindowByte(hWnd,4+3,0);   // 
      caretPosition.x=x *getFontWidth(&hWnd->font);
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_GETDLGCODE: 
      return DLGC_WANTCHARS | DLGC_WANTARROWS;
      break;
      
    case WM_LBUTTONDOWN:
      {RECT rc;
      POINT pt={LOWORD(lParam),HIWORD(lParam)};
      DC myDC;
      HDC hDC;
      GetClientRect(hWnd,&rc);
      if(pt.y>COMBO_ARROW_SIZE) {
        if(pt.x > rc.right-SCROLL_SIZE) {    // la scrollbar arriva qua o con WM_VSCROLL??
          
          }
        else {
          int j;
          j=GET_Y_LPARAM(lParam)-COMBO_ARROW_SIZE;
          j /= hWnd->font.base16+1;   // così al volo!
          SendMessage(hWnd,CB_SETCURSEL,j,0);

select_combo:
          j=SendMessage(hWnd,CB_GETCURSEL,0,0);   // qua sarebbe inutile o ottimizzabile, essendoci UN SOLO elem. selezionato!
          if(j!=CB_ERR) {
//inutile        SetWindowWord(hWnd,0+0,i);
            if(hWnd->parent) {
              NMHDR nmh;
              if((hWnd->style & 0xf) == CBS_SIMPLE) {
                //In a combo box with the CBS_SIMPLE style, the CBN_SELENDOK notification code is sent immediately before every CBN_SELCHANGE notification code.
                PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,CBN_SELENDOK),(LPARAM)hWnd);
                nmh.code=CBN_SELENDOK;
                nmh.hwndFrom=hWnd;
                nmh.idFrom=(DWORD)hWnd->menu;
                PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // volendo NMHDR ecc
              // anche CBN_SELENDOK e CBN_SELENDCANCEL
                }
              PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,CBN_SELCHANGE),(LPARAM)hWnd);
              nmh.code=CBN_SELCHANGE;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // volendo NMHDR ecc
              if((hWnd->style & 0xf) == CBS_DROPDOWN || (hWnd->style & 0xf) == CBS_DROPDOWNLIST) {
              // anche CBN_SELENDOK e CBN_SELENDCANCEL
                }
              }
            SendMessage(hWnd,WM_SETTEXT,0,SendMessage(hWnd,CB_GETLBTEXT,j,0));  // questo fa Invalidate...
            if((hWnd->style & 0xf) != CBS_SIMPLE)
              SetWindowByte(hWnd,4+1,GetWindowByte(hWnd,4+1) & 0x7f);    // chiudo drop
            goto change_dropped2;
            }
          }
        // solo se sel cambiata? e/o scroll
        InvalidateRect(hWnd,NULL,TRUE);
        }
      else {
        if(pt.x > rc.right-COMBO_ARROW_SIZE) {
          RECT rc;
change_dropped:
          if((hWnd->style & 0xf) != CBS_SIMPLE)
            SetWindowByte(hWnd,4+1,GetWindowByte(hWnd,4+1) ^ 0x80);    // droppo/no
// aprire sempre su prima riga? o nessuna?? boh
          if(((hWnd->style & 0xf) == CBS_DROPDOWN || (hWnd->style & 0xf) == CBS_DROPDOWNLIST) && hWnd->parent) {// PRIMA del cambiamento, dice; inoltre forse Send?
            NMHDR nmh;
            PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,
              GetWindowByte(hWnd,4+1) & 0x80 ? CBN_DROPDOWN : CBN_CLOSEUP),(LPARAM)hWnd);
            nmh.code=GetWindowByte(hWnd,4+1) & 0x80 ? CBN_DROPDOWN : CBN_CLOSEUP;
            nmh.hwndFrom=hWnd;
            nmh.idFrom=(DWORD)hWnd->menu;
            PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // volendo NMHDR ecc
            }
change_dropped2:
          GetWindowRect(hWnd,&rc);
          hDC=GetDC(hWnd,&myDC);
          SetWindowPos(hWnd,NULL,0,0,rc.right-rc.left,
            (GetWindowByte(hWnd,4+1) & 0x80 ? 
              COMBO_ARROW_SIZE+itemsNumber*(getFontHeight(&hDC->font)+1) : 
              COMBO_ARROW_SIZE  +1),
            SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);// redraw incorporato
          ReleaseDC(hWnd,hDC);
//          SendMessage(hWnd,WM_NCPAINT,0,0);
//          InvalidateRect(hWnd,NULL,TRUE);
          }
        else {
          if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
            SetWindowByte(hWnd,4+2,0);   // per prova!
            SetWindowByte(hWnd,4+3,0);
            //startsel ecc

            BYTE x=GET_X_LPARAM(lParam) / getFontWidth(&hWnd->font);
            x=min(x,31); 
            x=min(x,GetWindowByte(hWnd,4+0));
            SetWindowByte(hWnd,4+0,x);
            caretPosition.x=x *getFontWidth(&hWnd->font);
            InvalidateRect(hWnd,NULL,TRUE);   // solo la parte alta...
            }
          else
            goto change_dropped;
          }
        }
      if(hWnd->enabled)
        SendMessage(hWnd,WM_SETFOCUS,0,0);
      } 
//      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_LBUTTONDBLCLK:
      if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
        SetWindowByte(hWnd,4+2,0);   // fare solo parola, come in Edit
        SetWindowByte(hWnd,4+3,strlen(s));
        if(1 /*hWnd->style & CBS_NOTIFY non c'è??*/) {
          if(hWnd->parent) {
            NMHDR nmh;
            nmh.code=CBN_DBLCLK;
            nmh.hwndFrom=hWnd;
            nmh.idFrom=(DWORD)hWnd->menu;
            PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // finire
            }
          }
        InvalidateRect(hWnd,NULL,TRUE);
        }
      break;

    case CB_ADDSTRING:
      {LISTITEM *item2;
      i=0;
      item2=item;
      if(item) {
        while(item2 && item2->next) {
          i++;
          item2=item2->next;
          }
        item2->next=malloc(sizeof(LISTITEM));
        item2=item2->next;
        }
      else
        item=item2=malloc(sizeof(LISTITEM));
      i++;
      item2->next=NULL;
      strncpy(item2->data,(char*)lParam,sizeof(item2->data)-1);
      if(hWnd->style & CBS_LOWERCASE)
        strlwr(item2->data);
      else if(hWnd->style & CBS_UPPERCASE)
        strupr(item2->data);
      item2->data[sizeof(item2->data)-1]=0;     
      item2->state=0; item2->id=0;
      if(hWnd->style & CBS_SORT)
        listbox_bubble_sort(&item);
      SetWindowLong(hWnd,GWL_USERDATA,(DWORD)item);
//      SetWindowWord(hWnd,0+0,i);
      SetWindowWord(hWnd,0+2,i);
      
updatecombo:
      if(GetWindowByte(hWnd,4+1) & 0x80) {    // se dropped
        RECT rc;
        int j;
        GetClientRect(hWnd,&rc);
        i=SendMessage(hWnd,CB_GETCOUNT,0,0);
        rc.bottom=(rc.bottom-rc.top)/getFontHeight(&hWnd->font);
        if(i>=rc.bottom) {
  //          EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
          j=SendMessage(hWnd,CB_GETCURSEL,0,0);
          if(j != CB_ERR) {
            SetScrollRange(hWnd,SB_VERT,0,1+i-rc.bottom,FALSE);
            SetScrollPos(hWnd,SB_VERT,j>rc.bottom ? j-rc.bottom : 0,FALSE);
            }
          else
            SetScrollRange(hWnd,SB_VERT,0,1+i-rc.bottom,FALSE);
          ShowScrollBar(hWnd,SB_VERT,TRUE); //
          }
        else {
          if(hWnd->style & CBS_DISABLENOSCROLL) {
            }
          else {
  //            EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
  //            SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
            SetScrollPos(hWnd,SB_VERT,0,FALSE);
            ShowScrollBar(hWnd,SB_VERT,FALSE); //
            }
          }
        }
      InvalidateRect(hWnd,NULL,TRUE);
      return i;
      }
      break;
    case CB_DIR:  // serve qualche campo in più per data, dim, tipo file..
      {
      SearchRec rec;
      char *lpPathSpec=(char*)lParam;
      char drive;
      BYTE attr;
      MEDIA_INFORMATION *mi;

      if(lpPathSpec[1]==':')
        drive=lpPathSpec[0];
      else
        drive=currDrive;
      
      SendMessage(hWnd,CB_RESETCONTENT,0,0);

      attr=ATTR_NORMAL;   // DDL_READWRITE
      attr |= wParam & DDL_DIRECTORY ? ATTR_DIRECTORY : 0;
      attr |= wParam & DDL_HIDDEN ? ATTR_HIDDEN : 0;
      attr |= wParam & DDL_SYSTEM ? ATTR_SYSTEM : 0;
      attr |= wParam & DDL_READONLY ? ATTR_READ_ONLY : 0;
      attr &= ~ATTR_VOLUME;   // cmq

      mi=getMediaInfo(drive);
      if(!mi || mi->errorCode != MEDIA_NO_ERROR)
no_disc:
        return 0;
      switch(drive) {   //
        case 'A':
        case 'B':
        case 'C':
        case 'D':
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
#endif
#ifdef USA_USB_HOST_MSD
        case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
        case DEVICE_NETWORK:
#endif
          i=SuperFileFindFirst(drive,lpPathSpec, attr, &rec);
          break;
        default:
          goto no_disc;
          break;
        }

      if(!i) {
loop:
        SendMessage(hWnd,CB_ADDSTRING,0,(LPARAM)rec.filename);

        switch(drive) {
          case 'A':
          case 'B':
          case 'C':
          case 'D':
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
#endif
#ifdef USA_USB_HOST_MSD
          case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case DEVICE_NETWORK:
#endif
            if(!SuperFileFindNext(drive,&rec))
              goto loop;
            break;
          }
        }
      SetWindowWord(hWnd,0+0,0);
      SetWindowWord(hWnd,0+2,i);
      return 1;
      }
      break;
    case CB_INSERTSTRING:   // no SORT qua!
      {LISTITEM *item2;
      i=0;
      item2=item;
      if(item) {
        if(wParam == 0xffffffff) {
fine_lista:
          while(item2 && item2->next) {
            i++;
            item2=item2->next;
            }
	        item2->next=malloc(sizeof(LISTITEM));
		      item2=item2->next;
		      item2->next=NULL;
          }
        else {
          if(wParam==0) {   // se 0, lo gestisco a parte!
  	        item2=malloc(sizeof(LISTITEM));
  		      item2->next=item;
            item=item2;
            }
          else {
            item2=getListItemFromCnt(item,wParam);
            if(!item2) {
              item2=item;
              goto fine_lista;
              }
            else {
              LISTITEM *item3=item2->next;
              item2->next=malloc(sizeof(LISTITEM));
              item2=item2->next;
              item2->next=item3;
              }
            }
          }
        }
      else {
        item=item2=malloc(sizeof(LISTITEM));
	      item2->next=NULL;
				}
      i++;
      strncpy(item2->data,(char*)lParam,sizeof(item2->data)-1);
      item2->data[sizeof(item2->data)-1]=0;
      item2->state=0; item2->id=0;
      SetWindowLong(hWnd,GWL_USERDATA,(DWORD)item);
      SetWindowWord(hWnd,0+2,i);
      goto updatecombo;
      }
      break;
    case CB_DELETESTRING:
      {LISTITEM *item2;
      item2=item;
      while(wParam-- && item) {
        item2=item;
        item=item->next;
        }
      if(item2==(LISTITEM*)GetWindowLong(hWnd,GWL_USERDATA)) {
        SetWindowLong(hWnd,GWL_USERDATA,(DWORD)item->next);
        SetWindowWord(hWnd,0+0,0);
        SetWindowWord(hWnd,0+2,0);// non dovrebbe essere prec-1?? anche in listbox??
        }
      else {
        item2->next=item->next;
//        SetWindowWord(hWnd,GWL_USERDATA+0,i);
        SetWindowWord(hWnd,0+2,GetWindowWord(hWnd,0+2)-1);
        }
      free(item);
      goto updatecombo;
      }
      break;
    case CB_RESETCONTENT:
      {LISTITEM *item2;
      while(item) {
        item2=item->next;
        free(item);
        item=item2;
       }
      SetWindowLong(hWnd,GWL_USERDATA,(DWORD)NULL);

      SetWindowLong(hWnd,0,0);    // + rapido!

      goto updatecombo;
      }
      break;
    case CB_SETCURSEL:
      {LISTITEM *item2=item;
      while(item2) {
        item2->state &= ~1;
        item2=item2->next;
        }
      SetWindowWord(hWnd,0+0,0);
      if(wParam==0xffffffff) {
        *s=0;
        SendMessage(hWnd,WM_SETTEXT,0,(LPARAM)s);  // questo fa Invalidate...
        }
      else {
        item=getListItemFromCnt(item,wParam);
        if(item) {
          item->state |= 1;
          SetWindowWord(hWnd,0+0,wParam);
          }
        else
          return CB_ERR;
        }
      }
      goto updatecombo;
      break;
    case CB_GETCURSEL:
      {
      i=0;
      while(item) {
        if(item->state & 1) {
          SetWindowWord(hWnd,0+0,i);
          return i;
          }
        i++;
        item=item->next;
        }
      return CB_ERR;
      }
      break;
    case CB_GETLBTEXT:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)item->data;
      else
        return CB_ERR;
      }
      break;
    case CB_GETLBTEXTLEN:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)strlen(item->data);
      else
        return CB_ERR;
      }
      break;
    case CB_GETCOUNT:   // The index is zero-based, so the returned count is one greater than the index value of the last item.
      {
      i=1;
      while(item) {
        i++;
        item=item->next;
        }
      SetWindowWord(hWnd,0+2,i-1);
      return i;
      }
      break;
    case CB_GETITEMDATA:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)item->data;
      else
        return CB_ERR;
      }
      break;
    case CB_SETITEMDATA:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        strcpy(item->data,(char*)lParam);
      else
        return CB_ERR;
      }
      goto updatecombo;
      break;
    case CB_FINDSTRING:
    case CB_FINDSTRINGEXACT:
    case CB_SELECTSTRING:
      {
      i=0;
      if(wParam != 0xffffffff)
        item=getListItemFromCnt(item,wParam);
      while(item) {
        if((message==CB_FINDSTRINGEXACT && !strcmp(item->data,(char*)lParam)) 
          || (message!=CB_FINDSTRINGEXACT && !strnicmp(item->data,(char*)lParam,strlen((char*)lParam)))) {
          int j;
          if(message==CB_SELECTSTRING) {
            j=SendMessage(hWnd,CB_GETCURSEL,0,0);
            if(j!=CB_ERR)
              SendMessage(hWnd,CB_SETCURSEL,-1,0);
            SendMessage(hWnd,CB_SETCURSEL,i,0);
//            SetWindowWord(hWnd,0+0,i);
            goto updatecombo;
            }
          return i;
          break;
          }
        i++;
        item=item->next;
        }
      }
      return 0;
      break;
    case CB_INITSTORAGE:
      //CB_ERRSPACE
      return 0;
      break;
    case CB_SETITEMHEIGHT:
      // USARE CREATEFONT!
      hWnd->font.base16=max(lParam,6);   // no, è per ownerdraw, singola riga :) ma ok
      goto updatecombo;
      break;
    case CB_GETITEMHEIGHT:
      return hWnd->font.base16;
      break;
    case CB_GETTOPINDEX:
      {
      item=getListItemFromCnt(item,wParam);
      if(item)
        return 0;     // finire!
      else
        return CB_ERR;
      }
      break;
    case CB_GETEDITSEL:
      if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
        if(wParam)
          *(DWORD*)wParam=GetWindowByte(hWnd,4+2);
        if(lParam)
          *(DWORD*)lParam=GetWindowByte(hWnd,4+3);
        return MAKELONG(GetWindowByte(hWnd,4+2),GetWindowByte(hWnd,4+3)+1);
        }
      else
        return CB_ERR;
      break;
    case CB_SETEDITSEL:
      if((hWnd->style & 0xf) != CBS_DROPDOWNLIST) {
        if(LOWORD(lParam)==0xffff || HIWORD(lParam)==0xffff) {
          SetWindowByte(hWnd,4+2,0);
          SetWindowByte(hWnd,4+3,0);
          }
        else {
          SetWindowByte(hWnd,4+2,LOWORD(lParam));
          SetWindowByte(hWnd,4+3,HIWORD(lParam));
          }
        return TRUE;
        }
      else
        return CB_ERR;
      break;
    case CB_GETCUEBANNER:
      return 0; //boh
      break;
    case CB_SETCUEBANNER:
      return 0; //boh
      break;
    case CB_SETEXTENDEDUI:
      return 0; //boh
      break;
    case CB_SETHORIZONTALEXTENT:
      // finire... https://learn.microsoft.com/en-us/windows/win32/controls/lb-sethorizontalextent
      hWnd->scrollSizeX=wParam;
      if(wParam) {
        ShowScrollBar(hWnd,SB_HORZ,TRUE); //
// errore        EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH); 
        }
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case CB_SETDROPPEDWIDTH:
      //By default, the minimum allowable width of the drop-down list box is zero. The width of the list box is either the minimum allowable width or the combo box width, whichever is larger.
      break;
    case CB_GETDROPPEDWIDTH:
      {RECT rc;
      GetClientRect(hWnd,&rc);
      
      // per ora :)
      return rc.right-rc.left;
      }
      break;
    case CB_SHOWDROPDOWN:
      if((hWnd->style & 0xf) != CBS_SIMPLE) {
        SetWindowByte(hWnd,4+1,(GetWindowByte(hWnd,4+1) & 0x7f) | (wParam ? 0x80 : 0));
        goto change_dropped2;
        }
      break;
    case CB_GETDROPPEDSTATE:
      return GetWindowByte(hWnd,4+1) & 0x80 ? TRUE : FALSE;
      break;
    case CB_GETDROPPEDCONTROLRECT:
      {RECT rc,*rc2=(RECT*)wParam;
      DC myDC;
      HDC hDC;
      GetWindowRect(hWnd,&rc);
      hDC=GetDC(hWnd,&myDC);
      rc.bottom += COMBO_ARROW_SIZE+itemsNumber*(getFontHeight(&hDC->font)+1);
      *rc2=rc;
      ReleaseDC(hWnd,hDC);
      return 1;
      }
      break;
    case CB_GETMINVISIBLE:
      {DC myDC;
      HDC hDC;
      int i;
      hDC=GetDC(hWnd,&myDC);
      i=itemsNumber*(getFontHeight(&hDC->font)+1);
      ReleaseDC(hWnd,hDC);
      return i;
      }
      break;
    case CB_SETMINVISIBLE:
      SetWindowByte(hWnd,4+4,wParam);   // limitare??
      break;
    case CB_GETCOMBOBOXINFO:
      {COMBOBOXINFO *cbi=(COMBOBOXINFO*)lParam;
      RECT rc;
      GetWindowRect(hWnd,&rc);
      cbi->hwndCombo=hWnd;
      cbi->hwndItem=hWnd;   // sarebbero separati in windows...
      cbi->hwndList=hWnd;
      cbi->rcItem=rc;
      rc.left=rc.right-COMBO_ARROW_SIZE;
      cbi->rcButton=rc;
      cbi->stateButton=GetWindowByte(hWnd,4+1) & 0x80 ? STATE_SYSTEM_PRESSED : 0;  // dice anche STATE_SYSTEM_INVISIBLE...
      }
      break;
      
    case WM_HSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_VSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

int DlgDirList(HWND hDlg, char *lpPathSpec, int nIDListBox, int nIDStaticPath,
  UINT uFileType) {
  int i; 

  if(nIDStaticPath)
    SetDlgItemText(hDlg,nIDStaticPath,lpPathSpec);

  if(uFileType & DDL_POSTMSGS)
    PostMessage(GetDlgItem(hDlg,nIDListBox),LB_DIR,uFileType,(LPARAM)lpPathSpec);
  else
    SendMessage(GetDlgItem(hDlg,nIDListBox),LB_DIR,uFileType,(LPARAM)lpPathSpec);

  return 1;
  }
    
BOOL DlgDirSelectEx(HWND hwndDlg,char *lpString,int chCount,int idListBox) {
  int i=0;
  
  i=SendMessage(GetDlgItem(hwndDlg,idListBox),LB_GETCURSEL,0,0);
  if(SendMessage(GetDlgItem(hwndDlg,idListBox),LB_GETTEXTLEN,i,0) >= chCount)
    return 0;
  // TROVARE DIRECTORY!
  SendMessage(GetDlgItem(hwndDlg,idListBox),LB_GETTEXT,i,(LPARAM)lpString);   // controllare lunghezza...
  return i;
  }

LRESULT DefWindowProcButtonWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      int i,x,y;
      PAINTSTRUCT ps;
      int align=0;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;

      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORBTN*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }

      switch(hWnd->style & (BS_LEFT | BS_RIGHT)) {
        case BS_CENTER:
          align=DT_CENTER;
          break;
        case BS_RIGHT:
          align=DT_RIGHT;
          break;
        case BS_LEFTTEXT:
    //	Places text on the left side of the radio button or check box when combined with a radio button or check box style. Same as the BS_RIGHTBUTTON style.
        case BS_LEFT:
    //Left-justifies the text in the button rectangle. However, if the button is a check box or radio button that does not have the BS_RIGHTBUTTON style, the text is left justified on the right side of the check box or radio button.
        case BS_TEXT:
        default:
          align=DT_LEFT;
          break;
        }
      switch(hWnd->style & (BS_TOP | BS_BOTTOM)) {
        case BS_TOP:
          align |= DT_TOP;
          break;
        case BS_TOP | BS_BOTTOM:
          align |= DT_VCENTER;
          break;
        case BS_BOTTOM:
          align |= DT_BOTTOM;
          break;
        }
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          if(hWnd->enabled)
            SetTextColor(hDC,GetWindowByte(hWnd,GWL_USERDATA+1) ? WHITE : GRAY204);   // il secondo byte è lo stato premuto/non premuto
          else
            SetTextColor(hDC,GetWindowByte(hWnd,GWL_USERDATA+1) ? GRAY192 : GRAY128);   // il secondo byte è lo stato premuto/non premuto
          SetBkColor(hDC,c);
          switch(hWnd->style & 0x0ff0) {
            case BS_ICON:
              x=0; y=0;
              drawIcon8(hDC,x,y,hWnd->icon);
              break;
            default:
              DrawText(hDC,hWnd->caption,-1,&rc,align |
                (hWnd->style & 0 /*BS_NOPREFIX non ce..*/ ? DT_NOPREFIX : 0));
              break;
            }
          if(hWnd->focus)
            drawHorizLineWindowColor(hDC,rc.left+1,rc.bottom,rc.right-1,CYAN);
            //v. DrawFocusRect(HDC hDC,const RECT *lprc)
          break;
          
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          i=rc.bottom-rc.top-2;
          switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
            case 0:
              SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY204);
              break;
            case 1:
              SetTextColor(hDC,hWnd->enabled ? GRAY192 : GRAY128);
              break;
            case 2:     // se 3 state...
              SetTextColor(hDC,hWnd->enabled ? GRAY128 : GRAY064);
              break;
            }
          hDC->pen.color=hDC->foreColor;  //           SetDCPenColor(hDC,hDC->foreColor);
          drawRectangleWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
          // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+  ));
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                drawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                drawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                drawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              break;
            case BS_TOP:
//              break;
            case BS_BOTTOM:
// x ora              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
                drawLineWindow(hDC,rc.left+i,rc.top,rc.left,rc.top+i);
                }
              break;
            case BS_ICON:
              break;
            }
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY160);
          SetBkColor(hDC,c);
          switch(hWnd->style & 0x0ff0) {
            case BS_ICON:
              x=0; y=0;
              drawIcon8(hDC,x,y,hWnd->icon);
              break;
            default:
              DrawText(hDC,hWnd->caption,-1,&rc,align |
                (hWnd->style & 0 /*BS_NOPREFIX non ce..*/ ? DT_NOPREFIX : 0));
              break;
            }
          if(hWnd->focus)
            ;
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          hDC->pen.color=hWnd->active ? WHITE : GRAY192;    //           SetDCPenColor(hDC, );
          drawEllipseWindow(hDC,rc.left,rc.top,rc.left+i,rc.top+i);
          // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+ ));
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              else
                drawEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              else
                drawEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              else
                drawEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              break;
            case BS_TOP:
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*getFontWidth(&hDC->font))/2,0,hWnd->caption,strlen(hWnd->caption));
              break;
            case BS_BOTTOM:
              TextOut(hDC,(rc.right-strlen(hWnd->caption)*getFontWidth(&hDC->font))/2,rc.bottom-getFontHeight(&hDC->font),
                      hWnd->caption,strlen(hWnd->caption));
              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              else
                drawEllipseWindow(hDC,rc.left+1,rc.top+1,rc.left+i-1,rc.top+i-1);
              break;
            case BS_ICON:
            case BS_OWNERDRAW:
            case BS_USERBUTTON:
              break;
            }
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY160);
          SetBkColor(hDC,c);
          switch(hWnd->style & 0x0ff0) {
            case BS_ICON:
              x=0; y=0;
              drawIcon8(hDC,x,y,hWnd->icon);
              break;
            case BS_OWNERDRAW:
              {
              DRAWITEMSTRUCT dis;
              dis.CtlID=(DWORD)hWnd->menu;
              dis.itemID=0;
              dis.itemState=GetWindowByte(hWnd,GWL_USERDATA+1);   // TRADURRE! ODS_CHECKED ecc
              dis.hwndItem=hWnd;
              dis.hDC=hDC;
              dis.rcItem=rc;
              dis.CtlType=ODT_BUTTON;
              SendMessage(hWnd->parent,WM_DRAWITEM,(WPARAM)(DWORD)hWnd->menu,(LPARAM)&dis);
              }
              break;
            default:
              DrawText(hDC,hWnd->caption,-1,&rc,align |
                (hWnd->style & 0 /*BS_NOPREFIX non ce..*/ ? DT_NOPREFIX : 0));
              break;
            }
          if(hWnd->focus)
            ;
          break;
        }
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      int i;
      int c=hDC->brush.color;
      
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR/*WM_CTLCOLORBTN*/,
          wParam,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
          break;
        }
      }
      return 1;
      break;
    case WM_LBUTTONDOWN:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
          SetWindowByte(hWnd,GWL_USERDATA+1,0);
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:
          SetWindowByte(hWnd,GWL_USERDATA+1,!GetWindowByte(hWnd,GWL_USERDATA+1));// BUTTON FINIRE ECC
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          break;
        }
      if(hWnd->parent)
        PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_LBUTTONUP:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
          SetWindowByte(hWnd,GWL_USERDATA+1,1);
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          break;
        }


      return 0;
      break;
    case WM_LBUTTONDBLCLK:
  //      if(hWnd->enabled && hWnd->style & BS_NOTIFY RADIOBUTTON) { v. casi particolari in doc
      if(hWnd->enabled /*&& hWnd->style & BS_NOTIFY*/) {
        if(hWnd->parent)
          PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_DOUBLECLICKED),(LPARAM)hWnd);
        }
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_KEYDOWN:
    { 
//      if(hWnd->enabled) {
        switch(hWnd->style & 0x0f) {
              // mandarsi BM_SETSTATE?
          case BS_PUSHBUTTON:
          case BS_DEFPUSHBUTTON:
            switch(wParam) {
              case VK_SPACE:
                SetWindowByte(hWnd,GWL_USERDATA+1,0);

char_clicked:                
                InvalidateRect(hWnd,NULL,TRUE);
                if(hWnd->parent /*SEMPRE!&& hWnd->style & BS_NOTIFY*/)
                  PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
// lo spazio viene inviato anch'esso al parent.. non dovrebbe 2026 verificare a monte
                break;
              case VK_RETURN:
                break;
              default:  // hot key??
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
                  if(GetAsyncKeyState(VK_MENU) & 0x8000) {
                    if(toupper(LOBYTE(wParam)) == toupper(GetWindowByte(hWnd,GWL_USERDATA+2)))
                      goto char_clicked;
                    }
                  }
                break;
              }
            break;
          case BS_CHECKBOX:   // BUTTON FINIRE ECC
          case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
            switch(wParam) {
              default:  // hot key??
                
                SetWindowByte(hWnd,GWL_USERDATA+1,!GetWindowByte(hWnd,GWL_USERDATA+1));
                //prova!
                
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
                  }
                
                  InvalidateRect(hWnd,NULL,TRUE);

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
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
                  }
                
                    SetWindowByte(hWnd,GWL_USERDATA+1,!GetWindowByte(hWnd,GWL_USERDATA+1));
                    //prova!

                  InvalidateRect(hWnd,NULL,TRUE);
                
                break;
              }
            break;
          }
//        }
//      else
//        return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_KEYUP:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:
        case BS_DEFPUSHBUTTON:
          switch(wParam) {
            case VK_SPACE:
              SetWindowByte(hWnd,GWL_USERDATA+1,1);   
              // mandarsi BM_SETSTATE?

char_released:
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          break;
        }
      return 0;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
//      SetWindowByte(hWnd,GWL_USERDATA+0 ,cs->style & 0x0f);   // salvo il tipo, anche se in effetti sarebbe anche in style..
      int i;
      char *p;
      if(!(cs->style & BS_MULTILINE)) {// bah no qua..
//        BYTE height=getFontHeight(&hWnd->font);
//        i=cs->style & WS_BORDER ? 2 : 0;
//        i+=cs->style & WS_THICKFRAME ? 2 : 0;
//        cs->cy=height+i-1;
        }
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      SetWindowByte(hWnd,GWL_USERDATA+1,!(hWnd->style & WS_DISABLED));    // stato
      if(p=strchr(hWnd->caption,'&'))
        SetWindowByte(hWnd,GWL_USERDATA+2,p[1]);    // hotkey

      }
      return 0;
      break;
    case WM_WINDOWPOSCHANGING:
      {WINDOWPOS *wpos=(WINDOWPOS*)lParam;
        int i;
        i=hWnd->style & WS_BORDER ? 2 : 0;
        i+=hWnd->style & WS_THICKFRAME ? 2 : 0;
        if(!(hWnd->style & BS_MULTILINE)) {// bah no qua..
//        BYTE height=getFontHeight(&hWnd->font);
//          wpos->cy=height+i-1;
          }
      }
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char*)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_GETDLGCODE: 
      return DLGC_BUTTON | DLGC_WANTCHARS | 
        ((hWnd->style & 0x0f == BS_DEFPUSHBUTTON) ? DLGC_DEFPUSHBUTTON : DLGC_UNDEFPUSHBUTTON);
      break;

    case BM_SETSTATE:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          SetWindowByte(hWnd,GWL_USERDATA+1,wParam);
          InvalidateRect(hWnd,NULL,TRUE);
          if(hWnd->enabled) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
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
          return GetWindowByte(hWnd,GWL_USERDATA+1) ? BST_PUSHED : 0;
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
    case BM_SETSTYLE:
      hWnd->style = (hWnd->style & ~0xf) | (wParam & 0xf);
      if(lParam)
        InvalidateRect(hWnd,NULL,TRUE);
      break;

    case WM_SETFOCUS:
      if(hWnd->enabled) {
        if(!hWnd->focus) {
          hWnd->focus=1;
          if(hWnd->parent && hWnd->style & BS_NOTIFY)
            PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_SETFOCUS),(LPARAM)hWnd);
          InvalidateRect(hWnd,NULL,TRUE);
          }
        }
      return 0;
      break;
    case WM_KILLFOCUS:
      if(hWnd->enabled) {
        if(hWnd->focus) {
          hWnd->focus=0;
          if(hWnd->parent && hWnd->style & BS_NOTIFY)
            PostMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_KILLFOCUS),(LPARAM)hWnd);
          InvalidateRect(hWnd,NULL,TRUE);
          }
        }
      return 0;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcStatusBar(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_NCPAINT:    // solo tratto superiore, v.ERASEBKGND
//      calculateClientArea(hWnd,&hWnd->clientArea);    // 
      SendMessage(hWnd,WM_NCCALCSIZE,FALSE,(LPARAM)&hWnd->clientArea);
      return 1;
      break;
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORSTATIC*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }

      rc.top++;   // perché la riga "bordo" sopra la disegno!
      rc.left++; //volendo! per testo specialmente
			i=GetWindowByte(hWnd,GWL_USERDATA+0);		// 2bit tipo 2bit stato, x 2
			switch(i & 0b00000011) {
        case 1:     // icona verde-rosso-giallo
          rc.right-=8;
    			switch(i & 0b00001100) {
            case 0<<2:
              break;
            case 1<<2:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTGREEN));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTGREEN));
              goto led1;
            case 2<<2:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTYELLOW));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTYELLOW));
              goto led1;
            case 3<<2:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTRED));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTRED));
led1:
              drawVertLineWindowColor(hDC,rc.right-1,rc.top,rc.bottom,windowInactiveForeColor);
              Ellipse(hDC,rc.right+1,rc.top+0,rc.right+7,rc.top+6);
              break;
            }
          break;
        case 2:     // ? / !
          rc.right-=getFontWidth(&hDC->font)+2;
    			switch(i & 0b00001100) {
            char *p;
            case 0<<2:
              break;
            case 1<<2:
              p="?";
              goto text1;
              break;
            case 2<<2:
              p="-";
              goto text1;
              break;
            case 3<<2:
              p="!";
text1:
              drawVertLineWindowColor(hDC,rc.right-1,rc.top,rc.bottom,windowInactiveForeColor);
              DrawText(hDC,p,1,&rc,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        case 3:     // OK / NO
          rc.right-=getFontWidth(&hDC->font)*2+2;
    			switch(i & 0b00001100) {
            char *p;
            case 0<<2:
              break;
            case 1<<2:
              p="NO";
              goto text11;
              break;
            case 2<<2:
              p="?";
              goto text11;
              break;
            case 3<<2:
              p="OK";
text11:
              drawVertLineWindowColor(hDC,rc.right-1,rc.top,rc.bottom,windowInactiveForeColor);
              DrawText(hDC,p,-1,&rc,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        default:
          goto skippa_seconda;
          break;
				}
			switch(i & 0b00110000) {
        case 1<<4:
          rc.right-=8;
    			switch(i & 0b11000000) {
            case 0<<6:
              break;
            case 1<<6:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTGREEN));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTGREEN));
              goto led2;
            case 2<<6:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTYELLOW));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTYELLOW));
              goto led2;
            case 3<<6:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTRED));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTRED));
led2:
              drawVertLineWindowColor(hDC,rc.right-1,rc.top,rc.bottom,windowInactiveForeColor);
              Ellipse(hDC,rc.right+1,rc.top+0,rc.right+7,rc.top+6);
            }
          break;
        case 2<<4:
          rc.right-=getFontWidth(&hDC->font)+2;
    			switch(i & 0b00001100) {
            char *p;
            case 0<<6:
              break;
            case 1<<6:
              p="?";
              goto text2;
              break;
            case 2<<6:
              p="-";
              goto text2;
              break;
            case 3<<6:
              p="!";
text22: 
              drawVertLineWindowColor(hDC,rc.right-1,rc.top,rc.bottom,windowInactiveForeColor);
              DrawText(hDC,p,1,&rc,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        case 3<<4:
          rc.right-=getFontWidth(&hDC->font)*2+2;
    			switch(i & 0b11000000) {
            char *p;
            case 0<<6:
              break;
            case 1<<6:
              p="NO";
              goto text22;
              break;
            case 2<<6:
              p="?";
              goto text22;
              break;
            case 3<<6:
              p="OK";
text2:
              drawVertLineWindowColor(hDC,rc.right-1,rc.top,rc.bottom,windowInactiveForeColor);
              DrawText(hDC,p,-1,&rc,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        default:
          break;
				}
skippa_seconda:

//      SetTextColor(hDC,WHITE /*GRAY224*/); v. class
      SetBkColor(hDC,c);

      i=DT_VCENTER | DT_LEFT | DT_SINGLELINE;
      if(hWnd->style & SS_WORDELLIPSIS)
        i |= DT_WORD_ELLIPSIS;
      if(hWnd->style & SS_ENDELLIPSIS)
        i |= DT_END_ELLIPSIS;
// bah questoi direi di no      if(hWnd->style & SS_PATHELLIPSIS)
//        i |= DT_PATH_ELLIPSIS;

      switch(hWnd->style & SS_TYPEMASK) {
        case SS_ICON:
          if(hWnd->icon)
            drawIcon8(hDC,rc.left,rc.top,hWnd->icon);
          break;
        default:
          DrawText(hDC,hWnd->caption,-1,&rc,i);
          break;
        }

      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      int c=hDC->brush.color;
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORSTATIC*/,
          wParam,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,c);
      // no NCPAINT:
      drawHorizLineWindowColor(hDC,0,0,hWnd->paintArea.right, 
              hWnd->parent ? (GetParent(hWnd)->active ? windowForeColor : windowInactiveForeColor) : BLACK /* :) */);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      cs->style |= WS_DISABLED; 
      cs->style &= ~(/*incluso WS_CAPTION |*/ WS_BORDER /* v. NCPAINT*/);
/*      if((cs->style & 0xff) == SS_ICON)
        cs->cx=cs->cy=8 + (hWnd->style & WS_BORDER ? 2 : 0); // type ecc..
      else
        cs->cy=getFontHeight(&hWnd->font) + (cs->style & WS_BORDER ? 2 : 0); // */
      strncpy(hWnd->caption,cs->lpszName,sizeof(hWnd->caption)-1);  // perché non metto WS_CAPTION
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      if(hWnd->parent) {
        RECT rc;
        BYTE height=getFontHeight(&hWnd->font);
        GetClientRect(hWnd->parent,&rc);
        cs->x=rc.left;
        cs->y=rc.bottom-height;   // occhio cmq a come viene creata, al primo giro
        cs->cx=rc.right-rc.left;
        cs->cy=height;
        }
			SetWindowLong(hWnd,GWL_USERDATA,0xffff0000);		// pulisco, colore default
      }
      return 0;
      break;
    case WM_SETTEXT:    // gestisco, per gestire ev. 2° pane/icons
    case SB_SETTEXT:    // LOBYTE(LOWORD(wParam == quale, qua 0 per ora
      strncpy(hWnd->caption,(const char*)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_GETTEXT:    // gestisco, per gestire ev. 2° pane/icons
    case SB_GETTEXT:
      {DWORD n;
      n=DefWindowProc(hWnd,WM_GETTEXT,wParam,lParam);
      return MAKELONG(n,0 /*SBT_NOBORDERS*/);
      }
      break;
    case WM_GETTEXTLENGTH:    // gestisco, per gestire ev. 2° pane/icons
    case SB_GETTEXTLENGTH:
      {DWORD n;
      n=DefWindowProc(hWnd,WM_GETTEXTLENGTH,wParam,lParam);
      return MAKELONG(n,0 /*SBT_NOBORDERS*/);
      }
      break;
    case WM_SETFOCUS:
      return 0;
      break;
    case WM_KILLFOCUS:
      return 0;
      break;

    case SB_SETPARTS:   // wparam=quanti, lparam=dim
      {BYTE n;
      n=GetWindowByte(hWnd,GWL_USERDATA+0);
      switch(wParam) {
        case 0:
          n=0;
          break;
        case 1:
          n &= ~0b00001111;
          n |= 0b00000001;    // beh tanto per
          break;
        case 2:
          n &= ~0b11110000;
          n |= 0b00010000;    // beh tanto per
          break;
        }
			SetWindowByte(hWnd,GWL_USERDATA+0,n);
      }
      break;
    case SB_SETICON:   // wparam=quale | modo; lparam=stato
      {BYTE n;
      n=GetWindowByte(hWnd,GWL_USERDATA+0);
      switch(LOWORD(wParam)) {
        case 1:
          n &= ~0b00001111;
          n |= (HIWORD(wParam) & 3) << 0;
          n |= (lParam & 3) << 2;
          break;
        case 2:
          n &= ~0b11110000;
          n |= (HIWORD(wParam) & 3) << 4;
          n |= (lParam & 3) << 6;
          break;
        }
			SetWindowByte(hWnd,GWL_USERDATA+0,n);
      InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case SB_GETICON:
			return GetWindowByte(hWnd,GWL_USERDATA+0);    // non è proprio così ma ok!
      break;

    case SB_GETRECT:
      {BYTE n;
      RECT *rc=(RECT*)lParam;
      n=GetWindowByte(hWnd,GWL_USERDATA+0);
      switch(wParam) {
        case 0:
          if(n) {
            }
          else {
            }
          break;
        case 1:
          if(n & 0b00000011) {
            }
          else {
            }
          break;
        case 2:
          if(n & 0b00110000) {
            }
          else {
            }
          break;
        }
      }
      break;
    case SB_GETPARTS:
      {BYTE n;
      // in lparam array di coords...
      n=GetWindowByte(hWnd,GWL_USERDATA+0);
      return (n & 0b00110000) ? 3 : ((n & 0b00000011) ? 2 : 1);
      }
      break;
    case SB_SETTIPTEXT:
      break;
    case SB_GETTIPTEXT:
      break;
    case SB_SIMPLE:
      if(wParam)
        SetWindowByte(hWnd,GWL_USERDATA+0,0);   // vabbe' :)
      break;
    case SB_ISSIMPLE:
      return !GetWindowByte(hWnd,GWL_USERDATA+0);
      break;
    case SB_SETBKCOLOR:
      // lParam
      break;
      
    case WM_LBUTTONDOWN:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        switch(message) {
          case WM_LBUTTONDOWN:
            nmh.code=NM_CLICK;
            break;
          case WM_LBUTTONDBLCLK:
            nmh.code=NM_DBLCLK;
            break;
          case WM_RBUTTONDOWN:
            nmh.code=NM_RCLICK;
            break;
          case WM_RBUTTONDBLCLK:
            nmh.code=NM_DBLCLK;
            break;
          }
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); 
        }
      if(message==WM_LBUTTONDOWN || message==WM_RBUTTONDOWN)
        return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_WINDOWPOSCHANGED:
      {
        WINDOWPOS *wpos=(WINDOWPOS*)lParam;
reposition:
      if(hWnd->parent) {
        RECT rc;
        WINDOWPLACEMENT wp;
        BYTE height=getFontHeight(&hWnd->font);
        
        GetClientRect(hWnd->parent,&rc);
        rc.top=rc.bottom-height  /*-1 boh strano..*/;
//        rc.bottom=height+1;
//        SetWindowPos(hWnd,NULL,rc.left,rc.top,rc.right,rc.bottom,
//                SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOACTIVATE);
        // setwindowpos mi rimanda windowposchanged ricorsivamente! sarebbe logico.. ma in windows non lo fa... boh?
        // per ora uso questa:
//        rc.bottom=rc.top+height+1;
        wp.rcNormalPosition=rc;
        SetWindowPlacement(hWnd,&wp);
        }
      }
      break;
//    case WM_CHILDACTIVATE:
//      goto reposition;
//      return 0;
//      break;
//    case WM_SHOWWINDOW:
//      if(lParam==SW_PARENTOPENING)
//        goto reposition;
//      return 0;
//      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcProgressWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i,x,y;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      GFX_COLOR f=GetWindowWord(hWnd,GWL_USERDATA+0),b=GetWindowWord(hWnd,GWL_USERDATA+2);
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      if(hWnd->style & PBS_MARQUEE) {
        fillRectangleWindowColor(hDC,rc.left,rc.top,rc.left+rand() % i,rc.bottom,    // finire con marquee vero :D
          f);
        }
      else {
        i=GetWindowByte(hWnd,3);
        if(hWnd->style & PBS_CAPTION)
          sprintf(hWnd->caption,"%u%%",i);
        if(hWnd->style & PBS_VERTICAL) {
          i=((rc.bottom-rc.top)*(i-GetWindowByte(hWnd,1))) /
            (GetWindowByte(hWnd,2)-GetWindowByte(hWnd,1));
          fillRectangleWindowColor(hDC,rc.left,rc.top+i,rc.left,rc.bottom,
            f);
          }
        else {
          i=((rc.right-rc.left)*(i-GetWindowByte(hWnd,1))) /
            (GetWindowByte(hWnd,2)-GetWindowByte(hWnd,1));
          fillRectangleWindowColor(hDC,rc.left,rc.top,rc.left+i,rc.bottom,
            f);
          }
        if(hWnd->style & PBS_CAPTION) {
          SetTextColor(hDC,WHITE /*textColors[GetWindowByte(hWnd,4) & 15]*/);
          SetBkColor(hDC,b);
          DrawText(hDC,hWnd->caption,-1,&rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
          }
        }

      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      GFX_COLOR b=GetWindowWord(hWnd,GWL_USERDATA+2);
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,b);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      cs->style |= WS_DISABLED;   // direi
      if((cs->style & WS_CAPTION) == WS_CAPTION)
        cs->style |= PBS_CAPTION;   //:)  sfizio mio ma occhio a flag WS_CHILD cmq
      cs->style &= ~WS_DLGFRAME /*ossia WS_CAPTION*/;
      SetWindowByte(hWnd,0,0);    // attrib
      SetWindowByte(hWnd,1,0); SetWindowByte(hWnd,2,100);   // default
      SetWindowByte(hWnd,3,0);    // valore
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(windowForeColor,windowForeColor/2));   // colori f/b
      }
      return 0;
      break;
    case PBM_SETRANGE:
      {int8_t i=GetWindowByte(hWnd,3);
      i=max(i,LOWORD(lParam));
      i=min(i,HIWORD(lParam));
      SetWindowByte(hWnd,1,LOWORD(lParam));    // 
      SetWindowByte(hWnd,2,HIWORD(lParam));    // 
      SetWindowByte(hWnd,3,i);    // 
      InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case PBM_GETRANGE:
      if(wParam)
        return GetWindowByte(hWnd,1);   // low range
      else
        return GetWindowByte(hWnd,2);
      if(lParam) {    // FARE PBRANGE struct
        }
      break;
    case PBM_SETPOS:
      {
      int i;
      i=min(wParam,GetWindowByte(hWnd,2));
      i=max(i,GetWindowByte(hWnd,1));
      SetWindowByte(hWnd,3,i);
      }
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case PBM_GETPOS:
      return GetWindowByte(hWnd,3);
      break;
    case PBM_STEPIT:
      {
      int i;
      i=GetWindowByte(hWnd,3)+1;    // step...
      if(i>GetWindowByte(hWnd,2))
        i=GetWindowByte(hWnd,1);
      SetWindowByte(hWnd,3,i);
      }
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case PBM_DELTAPOS:
      break;
    case PBM_SETMARQUEE:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case PBM_SETSTATE:
      SetWindowByte(hWnd,0,wParam);    // lo uso come Set Attributes!
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case PBM_GETBARCOLOR:
      return GetWindowWord(hWnd,GWL_USERDATA+0);
      break;
    case PBM_SETBARCOLOR:
      if(lParam==CLR_DEFAULT)
        SetWindowWord(hWnd,GWL_USERDATA+0,windowForeColor);    // v. create
      else
        SetWindowWord(hWnd,GWL_USERDATA+0,lParam);    //
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case PBM_GETBKCOLOR:
      return GetWindowWord(hWnd,GWL_USERDATA+2);
      break;
    case PBM_SETBKCOLOR:
      if(lParam==CLR_DEFAULT)
        SetWindowWord(hWnd,GWL_USERDATA+2,windowForeColor/2);    // v. create
      else
        SetWindowWord(hWnd,GWL_USERDATA+2,lParam);    //
      InvalidateRect(hWnd,NULL,TRUE);
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

#define THUMBLENGTH (size/2)
#define THUMBWIDTH (3)    // meglio dispari :) per centratura

LRESULT DefWindowProcScrollBarWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  RECT rc;
  GetClientRect(hWnd,&rc);
  int spaceBeforeBar,spaceAfterBar;
  BYTE arrowSize;
  if(hWnd->style & WS_VSCROLL) {
    arrowSize=(rc.bottom-rc.top)/10;
    spaceBeforeBar=rc.bottom-rc.top-arrowSize-arrowSize+hWnd->scrollPosY;
    //usare GetWindowWord(hWnd,GWL_USERDATA+0)
    spaceAfterBar=rc.bottom-rc.top-spaceBeforeBar-GetWindowWord(hWnd,GWL_USERDATA+2);
    }
  else if(hWnd->style & WS_HSCROLL) {
    arrowSize=(rc.right-rc.left)/10;
    spaceBeforeBar=rc.right-rc.left-arrowSize-arrowSize+hWnd->scrollPosX;
    //usare GetWindowWord(hWnd,GWL_USERDATA+0)
    spaceAfterBar=rc.right-rc.left-spaceBeforeBar-GetWindowWord(hWnd,GWL_USERDATA+2);
    }
  
  switch(message) {
    case WM_NCPAINT:    // solo bordo
//      calculateClientArea(hWnd,&hWnd->clientArea);    // 
      SendMessage(hWnd,WM_NCCALCSIZE,FALSE,(LPARAM)&hWnd->clientArea);
      return 1;
      break;
    case WM_PAINT:	// v. nonClientPaint, la parte relativa alle scrollbar
    { 
      int i,x,y;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      // e usare DrawFrameControl con DFCS_SCROLLLEFT ecc per frecce
      if(hWnd->style & WS_HSCROLL) {
        // credo che vadano scalati su dim control...
        drawHorizArrowWindow(hDC,rc.left+1,rc.top+1,
          arrowSize,(rc.bottom-rc.top)-2,TRUE);
        drawHorizArrowWindow(hDC,rc.right-arrowSize-1,rc.top+1,
          arrowSize,(rc.bottom-rc.top)-2,FALSE);
        
        // usare spaceAfterBar ecc
        fillRectangleWindowColor(hDC,rc.left+arrowSize+1+hWnd->scrollPosX+GetWindowWord(hWnd,GWL_USERDATA+0),
          rc.top+1,
          rc.left+hWnd->scrollPosX+GetWindowWord(hWnd,GWL_USERDATA+0)+GetWindowWord(hWnd,GWL_USERDATA+2)-1,
          rc.bottom-1,hWnd->focus ? hDC->pen.color : hDC->pen.color);   // bah gestire thimb
        }
      else if(hWnd->style & WS_VSCROLL) {
        drawVertArrowWindow(hDC,rc.left+1,rc.top+1,
          (rc.right-rc.left)-2,arrowSize,TRUE);
        drawVertArrowWindow(hDC,rc.left+1,rc.bottom-arrowSize-1,
          (rc.right-rc.left)-2,arrowSize,FALSE);
        
        
        // usare spaceAfterBar ecc
        fillRectangleWindowColor(hDC,rc.left+1,
          rc.top+hWnd->scrollPosY+1+GetWindowWord(hWnd,GWL_USERDATA+0),
          rc.right-1,
          rc.top+hWnd->scrollPosY+GetWindowWord(hWnd,GWL_USERDATA+0)+GetWindowWord(hWnd,GWL_USERDATA+2)-1,
          hWnd->focus ? hDC->pen.color : hDC->pen.color);   // bah gestire thimb
        }
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,hDC->brush.color);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      cs->style &= ~WS_DLGFRAME /* ossia WS_CAPTION*/;
      SetWindowLong(hWnd,GWL_USERDATA,0);   // azzero
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      }
      return 0;
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_LEFT:     // 
          if(hWnd->style & WS_HSCROLL) {

            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_LINELEFT,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_UP:
          if(hWnd->style & WS_VSCROLL) {

            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_LINEUP,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_RIGHT:     // 
          if(hWnd->style & WS_HSCROLL) {

            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_LINERIGHT,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_DOWN:
          if(hWnd->style & WS_VSCROLL) {
            
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_LINEDOWN,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_HOME:    // 
          if(hWnd->style & WS_VSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_TOP,0),(DWORD)hWnd);
            }
          else if(hWnd->style & WS_HSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_LEFT,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_END:    // 
          if(hWnd->style & WS_VSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_BOTTOM,0),(DWORD)hWnd);
            }
          else if(hWnd->style & WS_HSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_TOP,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_NEXT:    // 
          if(hWnd->style & WS_VSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEDOWN,0),(DWORD)hWnd);
            }
          else if(hWnd->style & WS_HSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGEDOWN,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_PRIOR:    // 
          if(hWnd->style & WS_VSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEUP,0),(DWORD)hWnd);
            }
          else if(hWnd->style & WS_HSCROLL) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGEUP,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      break;
    case WM_LBUTTONDOWN:
      {RECT rc;
      POINT pt={LOWORD(lParam),HIWORD(lParam)};
      GetClientRect(hWnd,&rc);
      if(hWnd->style & WS_VSCROLL) {
        if(pt.y>spaceAfterBar) {   // finire
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEDOWN,0),(DWORD)hWnd);
          }
        else if(pt.y<spaceBeforeBar) {   // finire
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEUP,0),(DWORD)hWnd);
          }
        else {    // thumb
          // gestire colore mentre premuto... per ora uso focus (v.sopra)
          }
        }
      else if(hWnd->style & WS_HSCROLL) {
        if(pt.y>spaceAfterBar) {   // finire
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGERIGHT,0),(DWORD)hWnd);
          }
        else if(pt.y<spaceBeforeBar) {   // finire
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGELEFT,0),(DWORD)hWnd);
          }
        else {    // thumb
          // gestire colore mentre premuto... per ora uso focus (v.sopra)
          }
        }
      InvalidateRect(hWnd,NULL,TRUE);
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;

    case SBM_SETRANGE:
      SetWindowWord(hWnd,GWL_USERDATA+0,wParam);
      SetWindowWord(hWnd,GWL_USERDATA+2,lParam);
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case SBM_GETRANGE:
      *(DWORD*)wParam=GetWindowWord(hWnd,GWL_USERDATA+0);   // low range
      *(DWORD*)lParam=GetWindowWord(hWnd,GWL_USERDATA+2);   // high range
      break;
    case SBM_SETPOS:
      if(hWnd->style & WS_VSCROLL)
        hWnd->scrollPosY=wParam;
      else if(hWnd->style & WS_HSCROLL)
        hWnd->scrollPosX=wParam;
      if(lParam)
        InvalidateRect(hWnd,NULL,TRUE);
      break;
    case SBM_GETPOS:
      return hWnd->style & WS_VSCROLL ? hWnd->scrollPosY : hWnd->scrollPosX;
      break;
    case SBM_GETSCROLLBARINFO:
      {SCROLLBARINFO *si=(SCROLLBARINFO*)lParam;
      si->rcScrollBar=hWnd->nonClientArea;
      si->xyThumbTop=0;
      si->xyThumbBottom=THUMBWIDTH;   // riciclo!
      }
      break;
    case SBM_SETSCROLLINFO:
      {SCROLLINFO *si=(SCROLLINFO*)lParam;
      if(si->fMask & SIF_POS) {
        if(hWnd->style & WS_VSCROLL)
          hWnd->scrollPosY=si->nPos;
        else if(hWnd->style & WS_HSCROLL)
          hWnd->scrollPosX=si->nPos;
        }
      if(si->fMask & SIF_PAGE) {
        if(hWnd->style & WS_VSCROLL)
          hWnd->scrollSizeY=si->nPage;
        else if(hWnd->style & WS_HSCROLL)
          hWnd->scrollSizeX=si->nPage;
        }
      if(si->fMask & SIF_RANGE) {
        SetWindowWord(hWnd,GWL_USERDATA+0,si->nMin);
        SetWindowWord(hWnd,GWL_USERDATA+2,si->nMax);
        }
      if(si->fMask & SIF_DISABLENOSCROLL)
        ;
      }
      break;
    case SBM_GETSCROLLINFO:
      {SCROLLINFO *si=(SCROLLINFO*)lParam;
      si->nPos=hWnd->style & WS_VSCROLL ? hWnd->scrollPosY : hWnd->scrollPosX;
      si->nPage=hWnd->style & WS_VSCROLL ? hWnd->scrollSizeY : hWnd->scrollSizeX;
//        =hWnd->style & WS_VSCROLL ? hWnd->scrollRangeY : hWnd->scrollRangeX;   // high range
      si->nMin=GetWindowWord(hWnd,GWL_USERDATA+0);
      si->nMax=GetWindowWord(hWnd,GWL_USERDATA+2);
      si->fMask=SIF_POS | SIF_PAGE | SIF_RANGE;   // bah sì
      }
      break;
    case SBM_ENABLE_ARROWS:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcSpinWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int8_t i;
  int8_t m=GetWindowByte(hWnd,GWL_USERDATA+1),M=GetWindowByte(hWnd,GWL_USERDATA+2);
#define ARROW_SIZE 5
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      RECT rc;
      BYTE size=getFontWidth(&hDC->font) +2;
      
      GetClientRect(hWnd,&rc);
      SetTextColor(hDC,hWnd->active ? windowForeColor : windowInactiveForeColor);
      SetBkColor(hDC,windowBackColor);
      SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,hWnd->active ? windowForeColor : windowInactiveForeColor));
          
      if(hWnd->style & UDS_HORZ) {
        rc.top+=1; 
        rc.bottom-=getFontHeight(&hDC->font);   // uso dim testo per largh frecce :) provare
        drawRectangleWindow(hDC,rc.left,rc.bottom,rc.right,rc.bottom+getFontHeight(&hDC->font));
        drawHorizArrowWindow(hDC,rc.left,rc.bottom+1,(rc.right-rc.left)/2-1,
          ARROW_SIZE,TRUE);
        drawVertLineWindow(hDC,(rc.right-rc.left)/2,rc.bottom,rc.bottom+getFontHeight(&hDC->font));
        drawHorizArrowWindow(hDC,(rc.right-rc.left)/2+1,rc.top+1,(rc.right-rc.left)-1,
          ARROW_SIZE,FALSE);
        }
      else {
        if(hWnd->style & UDS_ALIGNRIGHT) {    // frecce a dx, testo a sx
          rc.left += 1;
          rc.right -= (size+2);   // uso dim testo per largh frecce :)
          drawRectangleWindow(hDC,rc.right+1,rc.top,rc.right+size+2,rc.bottom);
          drawVertArrowWindow(hDC,rc.right+2,rc.top+2,size-2,
// meglio size arrow dispari ossia size-2...
            ARROW_SIZE,TRUE);
/*          drawLineWindow(hDC,rc.right-1,rc.top+ARROW_SIZE,
                rc.right-(size)/2,rc.top+1);
          drawLineWindow(hDC,rc.right-(size)/2-1,rc.top+1,
                rc.right-(size)+1,rc.top+ARROW_SIZE);*/
          drawHorizLineWindow(hDC,rc.right+1,(rc.bottom-rc.top)/2,
                rc.right+size+1);
          drawVertArrowWindow(hDC,rc.right+2,(rc.bottom-rc.top)/2+2,size-2,
            ARROW_SIZE,FALSE);
/*          drawLineWindow(hDC,rc.right-1,rc.bottom-ARROW_SIZE,
                rc.right-(size)/2,rc.bottom-1);
          drawLineWindow(hDC,rc.right-(size)/2-1,rc.bottom-1,
                rc.right-(size)+1,rc.bottom-ARROW_SIZE);*/
          }
        else if(hWnd->style & UDS_ALIGNLEFT) {    // frecce a sx, testo a dx
          rc.right -= 1;
          rc.left += (size+1);    // uso dim testo per largh frecce :)
          drawRectangleWindow(hDC,rc.left-size,rc.top,rc.left,rc.bottom);
          drawVertArrowWindow(hDC,rc.left+1,rc.top+2,size-2,
            ARROW_SIZE,TRUE);
/*          drawLineWindow(hDC,rc.left+1,rc.top+ARROW_SIZE,
                rc.left+(size)/2,rc.top+1);
          drawLineWindow(hDC,rc.left+(size)/2-1,rc.top+1,
                rc.left+(size)-1,rc.top+ARROW_SIZE);*/
          drawHorizLineWindow(hDC,rc.left,(rc.bottom-rc.top)/2,
                rc.left+(size)-1);
          drawVertArrowWindow(hDC,rc.left+1,(rc.bottom-rc.top)/2+2,size-2,
            ARROW_SIZE,FALSE);
/*          drawLineWindow(hDC,rc.left+1,rc.bottom-ARROW_SIZE,
                rc.left+(size)/2,rc.bottom-1);
          drawLineWindow(hDC,rc.left+(size)/2-1,rc.bottom-1,
                rc.left+(size)-1,rc.bottom-ARROW_SIZE);*/
          }
        }
      if(hWnd->style & UDS_NOTHOUSANDS) {
        }
      itoa(hWnd->caption,GetWindowByte(hWnd,GWL_USERDATA+3),10);
      DrawText(hDC,hWnd->caption,-1,&rc,
              DT_VCENTER | (hWnd->style & UDS_ALIGNRIGHT ? DT_RIGHT : DT_LEFT) | DT_SINGLELINE);
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      // COLORE CTLCOLOR qua o no?? bah sì, direi..
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,windowBackColor);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      SetWindowLong(hWnd,GWL_USERDATA,0);
      SetWindowByte(hWnd,GWL_USERDATA+0,1);   // step
      }
      return 0;
      break;
    case WM_KEYDOWN:
      {i=GetWindowByte(hWnd,GWL_USERDATA+3);
      int8_t step=GetWindowByte(hWnd,GWL_USERDATA+0);
      switch(wParam) {
        case VK_HOME:
          i=m;
          //qua??
          goto update;
          break;
        case VK_END:
          i=M;
          //qua??
          goto update;
          break;
        case VK_UP:
          if(hWnd->style & UDS_ARROWKEYS) {
            if(hWnd->style & UDS_WRAP) {
              i += step;
              }
            else {
              if(i < M-step)
                i += step;
              else
                i=M;
              }
            
update:            
            SetWindowByte(hWnd,GWL_USERDATA+3,i);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent) {
              NMHDR nmh;
              NMUPDOWN nmu;
//              nmh.code=UDN_CLK;
              nmu.iDelta=step;
              nmu.iPos=i;   // dice che dovrebbe essere il valore precedente...
              nmu.hdr=nmh;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
              }
            }
          break;
        case VK_DOWN:
          if(hWnd->style & UDS_ARROWKEYS) {
            if(hWnd->style & UDS_WRAP) {
              i -= step;
              }
            else {
              if(i > m+step)   // 
                i -= step;
              else
                i=m;
              }
            goto update;
            }
          break;
        case VK_ESCAPE:   // boh?
          SetWindowByte(hWnd,GWL_USERDATA+3,m);
          goto update;
          break;
        }
      }
      break;
    case WM_LBUTTONDOWN:
      {i=GetWindowByte(hWnd,GWL_USERDATA+3);
      int8_t step=GetWindowByte(hWnd,GWL_USERDATA+0);
      int8_t t=0;
      RECT rc;
      POINT pt={LOWORD(lParam),HIWORD(lParam)};
      GetClientRect(hWnd,&rc);
      if(hWnd->style & UDS_HORZ) {
        rc.top = rc.bottom-getFontHeight(&hWnd->font);
        rc.right /= 2;
        if(wndPtInRect(&rc,pt))
          t=-1;
        rc.left=rc.right; rc.right *= 2;
        if(wndPtInRect(&rc,pt))
          t=1;
        }
      else {
        if(hWnd->style & UDS_ALIGNRIGHT) {    // frecce a dx, testo a sx
          rc.left = rc.right-getFontWidth(&hWnd->font);
          rc.bottom /= 2;
          if(wndPtInRect(&rc,pt))
            t=1;
          rc.top=rc.bottom; rc.bottom *= 2;
          if(wndPtInRect(&rc,pt))
            t=-1;
          }
        else if(hWnd->style & UDS_ALIGNLEFT) {    // frecce a sx, testo a dx
          rc.right = rc.left-getFontWidth(&hWnd->font);
          rc.bottom /= 2;
          if(wndPtInRect(&rc,pt))
            t=1;
          rc.top=rc.bottom; rc.bottom *= 2;
          if(wndPtInRect(&rc,pt))
            t=-1;
          }
        }
      if(t>0) {
        if(hWnd->style & UDS_WRAP) {
          i += step;
          }
        else {
          if(i < M-step)
            i += step;
          else
            i=M;
          }
        }
      else if(t<0) {
        if(hWnd->style & UDS_WRAP) {
          i -= step;
          }
        else {
          if(i > m+step)   // 
            i -= step;
          else
            i=m;
          }
        }
      goto update;
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case UDM_SETRANGE:    // ma QUANTO siete froci?? è il contrario della progress!
      {int8_t i=GetWindowByte(hWnd,GWL_USERDATA+3);
      i=max(i,HIWORD(lParam));
      i=min(i,LOWORD(lParam));
      SetWindowByte(hWnd,GWL_USERDATA+1,HIWORD(lParam));    // 
      SetWindowByte(hWnd,GWL_USERDATA+2,LOWORD(lParam));    // 
      SetWindowByte(hWnd,GWL_USERDATA+3,i);    // 
      InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case UDM_GETRANGE:
      return MAKELONG(GetWindowByte(hWnd,GWL_USERDATA+2),GetWindowByte(hWnd,GWL_USERDATA+1));
      if(lParam) {    // FARE PBRANGE struct
        }
      break;
    case UDM_SETPOS:
      {
      int i;
      i=min((int)wParam,UD_MAXVAL);
      i=min(i,(int)GetWindowByte(hWnd,GWL_USERDATA+2));
      i=max((int)wParam,UD_MINVAL);
      i=max(i,(int)GetWindowByte(hWnd,GWL_USERDATA+1));
      SetWindowByte(hWnd,GWL_USERDATA+3,i);
      }
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case UDM_GETPOS:
      return GetWindowByte(hWnd,GWL_USERDATA+3);
      break;
    case UDM_SETBUDDY:    // sarebbe "il testo amico" che qua incorporo!
      return 0;
      break;
    case UDM_GETBUDDY:
      return (DWORD)hWnd;
      break;
    case UDM_SETACCEL:
      break;
    case UDM_GETACCEL:
      break;
    case UDM_SETBASE:   // base 16 o 10
      break;
    case UDM_GETBASE:
      break;

    case WM_GETDLGCODE:
      return DLGC_WANTARROWS;
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcSliderWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int8_t m=GetWindowByte(hWnd,GWL_USERDATA+1);
  int8_t M=GetWindowByte(hWnd,GWL_USERDATA+2);
  int8_t val=GetWindowByte(hWnd,GWL_USERDATA+3);
  int8_t step=GetWindowByte(hWnd,GWL_USERDATA+0);
  int8_t ticksPerStep=step ? (M-m)/step : 0;
  BYTE size;
      
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      uint8_t pxPerTicks;
      RECT rc;
      GetClientRect(hWnd,&rc);

      SetTextColor(hDC,hWnd->active ? windowForeColor : windowInactiveForeColor);
      SetBkColor(hDC,windowBackColor);
      SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,hWnd->active ? windowForeColor : windowInactiveForeColor));
      SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(hWnd->active ? windowForeColor : windowInactiveForeColor));
      if(hWnd->style & TBS_VERT) {
        rc.left++;    // -(THUMBWIDTH) ... a seconda di horz o vert..
        rc.right--;
        size=rc.right-rc.left;
        if(step && M>m)
          pxPerTicks=(rc.bottom-rc.top-(1  /*BORDER*/))/((M-m)/step);
        fillRectangleWindow(hDC,rc.left+(size/2)-1,rc.top,    // lo slider
          rc.left+(size/2)+1,rc.bottom);
        if(!(hWnd->style & TBS_LEFT) || hWnd->style & TBS_BOTH) {    // 
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=M; i>=m; i-=step)
                drawHorizLineWindow(hDC,(rc.right-rc.left)/2,
                  rc.bottom-(i-m)*pxPerTicks-THUMBWIDTH/2-1,
                  rc.right-1);
              }
            else {  // fare...
              }
            }
          }
        if(hWnd->style & TBS_LEFT || hWnd->style & TBS_BOTH) {    // 
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=M; i>=m; i-=step)
                drawHorizLineWindow(hDC,rc.left+1,
                  rc.bottom-(i-m)*pxPerTicks-THUMBWIDTH/2-1,
                  (rc.right-rc.left)/2);
              }
            else {  // fare...
              }
            }
          }
        if(!(hWnd->style & TBS_NOTHUMB)) {
          fillRectangleWindow(hDC,
            rc.left+THUMBLENGTH/2,
            rc.bottom-(val-m)*pxPerTicks-THUMBWIDTH,
            rc.right-THUMBLENGTH/2,
            rc.bottom-1-(val-m)*pxPerTicks);
          }
        }
      else {
        rc.top++;
        rc.bottom--;
        size=rc.bottom-rc.top;
        if(step && M>m)
          pxPerTicks=(rc.right-rc.left-(1 /*BORDER*/))/((M-m)/step);
        else
          pxPerTicks=0;
        fillRectangleWindow(hDC,rc.left,rc.top+(size/2)-1,  // lo slider
          rc.right,rc.top+(size/2)+1);
        if(!(hWnd->style & TBS_TOP) || hWnd->style & TBS_BOTH) {    // COGLIONI! BOTTOM è 0
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=m; i<=M; i+=step)
                drawVertLineWindow(hDC,rc.left+(i-m)*pxPerTicks+THUMBWIDTH/2,
                  (rc.bottom-rc.top)/2,
                  rc.bottom-1);
              }
            else {  // fare...
              }
            }
          }
        if(hWnd->style & TBS_TOP || hWnd->style & TBS_BOTH) {    // 
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=m; i<=M; i+=step) 
                drawVertLineWindow(hDC,rc.left+(i-m)*pxPerTicks+THUMBWIDTH/2,
                  rc.top+1,
                  (rc.bottom-rc.top)/2);
              }
            else {  // fare...
              }
            }
          }
        if(!(hWnd->style & TBS_NOTHUMB)) {
          fillRectangleWindow(hDC,
            rc.left+1+2+(val-m)*pxPerTicks-THUMBWIDTH,
            rc.top+THUMBLENGTH/2,
            rc.left+1+2-1+(val-m)*pxPerTicks,
            rc.bottom-THUMBLENGTH/2);
          }
        }
      
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      // COLORE CTLCOLOR qua o no?? bah sì, direi..
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,windowBackColor);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      if(hWnd->style & TBS_HORZ && hWnd->style & TBS_VERT)    // COGLIONI horz è 0..
        return -1;    // eh beh
      if(hWnd->style & TBS_NOTICKS && hWnd->style & TBS_AUTOTICKS)
        return -1;    // eh beh
      SetWindowLong(hWnd,GWL_USERDATA,0);
      SetWindowByte(hWnd,GWL_USERDATA+0,1);   // step
      }
      return 0;
      break;
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_LEFT:     // 
          if(!(hWnd->style & TBS_VERT)) {
            if(val >= m+step)
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_LINELEFT,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_UP:
          if(hWnd->style & TBS_VERT) {
            if(val <= M-step)   // 
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_LINEUP,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_RIGHT:     // TB_LINEDOWN
          if(!(hWnd->style & TBS_VERT)) {
            if(val <= M-step)
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_LINERIGHT,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_DOWN:
          if(hWnd->style & TBS_VERT) {
            if(val >= m+step)   // 
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_LINEDOWN,0),(DWORD)hWnd);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
        case VK_HOME:    // TB_TOP
          if(hWnd->style & TBS_VERT)    // beh sì :)
            SetWindowByte(hWnd,GWL_USERDATA+3,M);
          else
            SetWindowByte(hWnd,GWL_USERDATA+3,m);
          if(hWnd->style & TBS_VERT) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_TOP,0),(DWORD)hWnd);
            }
          else {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_LEFT,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_END:    // TB_BOTTOM
          if(hWnd->style & TBS_VERT)    // beh sì :)
            SetWindowByte(hWnd,GWL_USERDATA+3,m);
          else
            SetWindowByte(hWnd,GWL_USERDATA+3,M);
          if(hWnd->style & TBS_VERT) {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_BOTTOM,0),(DWORD)hWnd);
            }
          else {
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_TOP,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_NEXT:    // TB_PAGEDOWN
          step *= 2;
          if(hWnd->style & TBS_VERT) {
            if(val >= m+step)   // 
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEDOWN,0),(DWORD)hWnd);
            }
          else {
            if(val <= M-step)
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGEDOWN,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_PRIOR:    // TB_PAGEUP
          step *= 2;
          if(hWnd->style & TBS_VERT) {
            if(val <= M-step)
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEUP,0),(DWORD)hWnd);
            }
          else {
            if(val >= m+step)   // 
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGEUP,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      if(hWnd->parent) {
        NMHDR nmh;
        nmh.code=WM_VSCROLL; // finire, sistemare
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // e verificare con WM_scroll
        }
      break;
    case WM_LBUTTONDOWN:
      {RECT rc;
      POINT pt={LOWORD(lParam),HIWORD(lParam)};
      GetClientRect(hWnd,&rc);
      uint8_t pxPerTicks;
      if((M-m)>10) 
        step *= 2;   // ideuzza :)
      if(hWnd->style & TBS_VERT) {
        if(step && M>m)
          pxPerTicks=(rc.bottom-rc.top-THUMBWIDTH)/((M-m)/step);
//err_printf("pxpt=%d,val=%d,m=%d,pt.y=%d; %d\n",pxPerTicks,val,m,pt.y,(DWORD)(val-m)*pxPerTicks);
        if(pt.y > rc.bottom-(DWORD)(val-m)*pxPerTicks) {
          if(val >= m+step)   // 
            val -= step;
          else
            val=m;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEDOWN,0),(DWORD)hWnd);
          //NMHDR sotto...
          }
        else {
          if(val <= M-step)   // 
            val += step;
          else
            val=M;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_VSCROLL,MAKELONG(SB_PAGEUP,0),(DWORD)hWnd);
          }
        }
      else {
        if(step && M>m)
          pxPerTicks=(rc.right-rc.left-THUMBWIDTH)/((M-m)/step);

        if(pt.x > (DWORD)(val-m)*pxPerTicks) {
          if(val <= M-step)   // 
            val += step;
          else
            val=M;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGERIGHT,0),(DWORD)hWnd);
          }
        else {
          if(val >= m+step)   // 
            val -= step;
          else
            val=m;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            PostMessage(hWnd->parent,WM_HSCROLL,MAKELONG(SB_PAGELEFT,0),(DWORD)hWnd);
          }
        }
      InvalidateRect(hWnd,NULL,TRUE);
      if(hWnd->parent) {
        NMHDR nmh;
        nmh.code=WM_VSCROLL;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
        }
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case TBM_SETRANGEMIN:
      {int8_t i=GetWindowByte(hWnd,3);
      i=max(i,HIWORD(lParam));
      SetWindowByte(hWnd,GWL_USERDATA+1,HIWORD(lParam));
      SetWindowByte(hWnd,GWL_USERDATA+3,i);    // 
      if(wParam)
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case TBM_SETRANGEMAX:
      {int8_t i=GetWindowByte(hWnd,3);
      i=min(i,HIWORD(lParam));
      SetWindowByte(hWnd,GWL_USERDATA+2,HIWORD(lParam));
      SetWindowByte(hWnd,GWL_USERDATA+3,i);    // 
      if(wParam)
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case TBM_SETRANGE:    // 
      {int8_t i=GetWindowByte(hWnd,3);
      i=min(i,HIWORD(lParam));
      i=max(i,LOWORD(lParam));
      SetWindowByte(hWnd,GWL_USERDATA+1,LOWORD(lParam));    // 
      SetWindowByte(hWnd,GWL_USERDATA+2,HIWORD(lParam));    // 
      SetWindowByte(hWnd,GWL_USERDATA+3,i);    // 
      if(wParam)
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case TBM_GETRANGEMAX:
      return GetWindowByte(hWnd,GWL_USERDATA+2);
      break;
    case TBM_GETRANGEMIN:
      return GetWindowByte(hWnd,GWL_USERDATA+1);
      break;
    case TBM_SETPOS:
      {
      int i;
      i=min(lParam,M);
      i=max(i,m);
      SetWindowByte(hWnd,GWL_USERDATA+3,i);
      }
      if(wParam)
        InvalidateRect(hWnd,NULL,TRUE);
      break;
    case TBM_GETPOS:
      return GetWindowByte(hWnd,GWL_USERDATA+3);
      break;
    case TBM_GETTIC:
      return GetWindowByte(hWnd,GWL_USERDATA+3)   ;   // beh finire
      break;
    case TBM_GETTICPOS:
      return GetWindowByte(hWnd,GWL_USERDATA+3)   *GetWindowByte(hWnd,GWL_USERDATA+0);    // finire..
      break;
    case TBM_SETBUDDY:    // sarebbe "il testo amico" ... anche qua??
      return 0;
      break;
    case TBM_GETBUDDY:
      return (DWORD)hWnd;
      break;
    case TBM_GETTHUMBRECT:
      break;
    case TBM_SETTHUMBLENGTH:
      // qua fisso 4, per ora almeno
      break;
    case TBM_GETTHUMBLENGTH:
      return THUMBLENGTH;
      break;
    case TBM_SETTIC:    // fare... con array
      break;
    case TBM_SETTICFREQ:
      SetWindowByte(hWnd,GWL_USERDATA+0,lParam);    // vale come "step" idem
      break;
    case TBM_SETLINESIZE:
      SetWindowByte(hWnd,GWL_USERDATA+0,lParam);    // vale come "step"
      break;
    case TBM_SETPAGESIZE:
      // fisso a 2x step per ora
      break;
    case TBM_SETTIPSIDE:
      break;
    case TBM_GETLINESIZE:
      return GetWindowByte(hWnd,GWL_USERDATA+0);
      break;
    case TBM_GETPAGESIZE:
      return GetWindowByte(hWnd,GWL_USERDATA+0) *2;   // qua così
      break;
    case TBM_GETNUMTICS:
      if(!(hWnd->style & TBS_NOTICKS)) {
        if(hWnd->style & TBS_AUTOTICKS) {
          return step ? ((M-m)/step)+2 : 0;   // safety :)
          }
        else
          return 2;   // v.doc
        }
      else
        return 0;
      break;
    case TBM_GETPTICS:
      break;
    case TBM_SETSELSTART:
      break;
    case TBM_SETSELEND:
      break;
    case TBM_GETSELSTART:
      break;
    case TBM_GETSELEND:
      break;

    case WM_GETDLGCODE:
      return /*DLGC_HASSETSEL |*/ DLGC_WANTARROWS;
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }



#define X_SPACING_LARGEICONS 60
#define X_SPACING_LARGEICONS_DISK 80
#define Y_SPACING_LARGEICONS 40
#define Y_SPACING_LARGEICONS_DISK 44
#define X_SPACING_SMALLICONS 40
#define X_SPACING_SMALLICONS_DISK 40
#define Y_SPACING_SMALLICONS 32
#define Y_SPACING_SMALLICONS_DISK 32
#define Y_SPACING_DETAILS 8  //getFontHeight(&hDC->font)

LRESULT DefWindowProcFileDlgWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  OPENFILENAME *ofn=(OPENFILENAME*)GET_WINDOW_DLG_OFFSET(hWnd,0);
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      SearchRec rec;
      int i;
      uint32_t sn;
      UGRAPH_COORD_T x,y;
      char label[16 /*TOTAL_FILE_SIZE+1*/];
      WORD totFiles;
      RECT rc;
      GetClientRect(hWnd,&rc);
      MEDIA_INFORMATION *mi;
      
      HDC hDC=BeginPaint(hWnd,&ps);

      if(!GetWindowByte(hWnd,GWL_USERDATA+2))
        fillRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
      totFiles=0;

      mi=getMediaInfo(ofn->disk);
      switch(ofn->disk) {
        case 'A':
        case 'B':
        case 'C':
        case 'D':
#ifdef USA_RAM_DISK 
        case DEVICE_RAMDISK:
#endif
#if defined(USA_USB_HOST_MSD)
        case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
        case DEVICE_NETWORK:
#endif
          if(!mi || mi->errorCode != MEDIA_NO_ERROR) {    // diversificare qua con errore :)
no_disc:
            SetWindowByte(hWnd,GWL_USERDATA+2,1);
            SetWindowText(hWnd,"Disco");
            char buf[32];
            sprintf(buf,"Inserire disco nell'unità %c:",ofn->disk);
            SetWindowText(GetDlgItem(hWnd,201),buf);
            goto fine;
            }
          SuperFileGetVolume(ofn->disk,label,&sn);
          if(*ofn->path)
            i=SuperFileFindFirst(ofn->disk,ofn->path, ATTR_MASK ^ ATTR_VOLUME, &rec);
          else
            i=SuperFileFindFirst(ofn->disk,ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
          break;
        default:
          goto no_disc;
          break;
        }
      
      if(!i) {
        x=0; y=0;
        if(GetWindowByte(hWnd,GWL_USERDATA+2)<2)
          SetWindowByte(hWnd,GWL_USERDATA+2,2);
        
loop:
					totFiles++;
          if(y<ps.rcPaint.bottom) {
            switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
              case 0:     // icone piccole
                hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font;
                rc.left=rc.left+4; rc.top=y+4+8+2; rc.right=rc.right-4; rc.bottom=rc.bottom-1;
                SetTextColor(hDC,rec.attributes & ATTR_HIDDEN ? GRAY192 : WHITE);
                DrawText(hDC,rec.filename,-1,&rc,DT_TOP | DT_CENTER);
                if(rec.attributes & ATTR_DIRECTORY) {
                  drawIcon8(hDC,x+18,y+4,folderIcon8);
                  }
                else {
                  drawIcon8(hDC,x+18,y+4,fileIcon8);
                  }
                x+=X_SPACING_SMALLICONS;
                if(x>=rc.right) {
                  x=0;
                  y+=Y_SPACING_SMALLICONS;
                  }
                break;
              case 1:     // icone grandi
                rc.left=rc.left+4; rc.top=y+4+16+2; rc.right=rc.right-4; rc.bottom=rc.bottom-1;
                SetTextColor(hDC,rec.attributes & ATTR_HIDDEN ? GRAY192 : WHITE);
                DrawText(hDC,rec.filename,-1,&rc,DT_TOP | DT_CENTER);
                if(rec.attributes & ATTR_DIRECTORY) {
                  DrawIcon(hDC,x+25,y+4,folderIcon);
                  }
                else {
                  DrawIcon(hDC,x+25,y+4,fileIcon);
                  }
                x+=X_SPACING_LARGEICONS;
                if(x>=rc.right) {
                  x=0;
                  y+=Y_SPACING_LARGEICONS;
                  }
                break;
              case 2:     // dettagli
//                rec.filename[16]=0; INUTILE nomi file corti :)
                SetTextColor(hDC,rec.attributes & ATTR_HIDDEN ? GRAY192 : WHITE);
                TextOut(hDC,x+2,y+2,rec.filename,strlen(rec.filename)); 
                if(rec.attributes & ATTR_DIRECTORY) {
                  TextOut(hDC,x+13*6+2,y+2,"DIR",3);
                  }
                else {
                  char buf[32];
                  itoa(buf,rec.filesize,10);
                  TextOut(hDC,x+13*6,y+2,buf,strlen(buf));
                  sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",    // v. anche FileTimeToSystemTime
                    rec.timestamp.day,rec.timestamp.mon,rec.timestamp.year+1980,
                    rec.timestamp.hour,rec.timestamp.min,rec.timestamp.sec);
                  TextOut(hDC,x+(14+10)*6,y+2,buf,strlen(buf));
                  }
                y+=8;
                break;
              }
            }
          switch(ofn->disk) {
            case 'A':
            case 'B':
            case 'C':
            case 'D':
#ifdef USA_RAM_DISK 
            case DEVICE_RAMDISK:
#endif
#if defined(USA_USB_HOST_MSD)
            case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
            case DEVICE_NETWORK:
#endif
              if(!SuperFileFindNext(ofn->disk,&rec))
                goto loop;
              break;
            }

        switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
          case 0:
            y=((rc.right-rc.left)/X_SPACING_SMALLICONS)*
              ((rc.bottom-rc.top)/Y_SPACING_SMALLICONS);
            if(totFiles>y) {
//              EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_VERT,0,
                1+((totFiles-y)/((rc.right-rc.left)/X_SPACING_SMALLICONS)),
                FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //bah
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
//            EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //bah
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            ShowScrollBar(hWnd,SB_HORZ,FALSE); //
            break;
          case 1:
            y=((rc.right-rc.left)/X_SPACING_LARGEICONS)*
              ((rc.bottom-rc.top)/Y_SPACING_LARGEICONS);
            if(totFiles>y) {
//              EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_VERT,0,
                1+((totFiles-y)/((rc.right-rc.left)/X_SPACING_LARGEICONS)),
                FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //bah
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
//            EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //bah
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            ShowScrollBar(hWnd,SB_HORZ,FALSE); //
            break;
          case 2:
            y=(rc.bottom-rc.top)/Y_SPACING_DETAILS;
            if(totFiles>y) {
//              EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_VERT,0,y-totFiles,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
            y=(rc.right-rc.left)/getFontWidth(&hDC->font);
            if(40>y) {   // v. testo sopra
//              EnableScrollBar(hWnd,SB_HORZ,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_HORZ,0,y - 40,FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              ShowScrollBar(hWnd,SB_HORZ,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              ShowScrollBar(hWnd,SB_HORZ,FALSE); //
              }
            break;
          }
        }

      switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
        case 0:
          y+=Y_SPACING_SMALLICONS;
          break;
        case 1:
          y+=Y_SPACING_LARGEICONS;
          break;
        case 2:
          y+=Y_SPACING_DETAILS;
          break;
        }
      hDC->font=GetStockObject(SYSTEM_FONT).font;
      if(GetWindowByte(hWnd,GWL_USERDATA+2)<3) {
        ofn->fsdp.new_request=1;
        mi=getMediaInfo(ofn->disk);
        switch(ofn->disk) {
          case 'A':
          case 'B':
          case 'C':
          case 'D':
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
#endif
            do {
              FSGetDiskProperties(mi,&ofn->fsdp);
              ClrWdt();
              } while(ofn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
            break;
#if defined(USA_USB_HOST_MSD)
          case 'E':
            {
            uint32_t freeSectors,totalSectors,sectorSize;
            if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
              ofn->fsdp.results.free_clusters=freeSectors;
              ofn->fsdp.results.sectors_per_cluster=1;
              ofn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
              ofn->fsdp.properties_status = FS_GET_PROPERTIES_NO_ERRORS;
              SYS_FS_GetDiskInfo(NULL,&ofn->fsdp.results.disk_format); // OCCHIO che cmq è diversa!
              ofn->fsdp.new_request=0;
              }
            else {
              ofn->fsdp.results.free_clusters=ofn->fsdp.results.sectors_per_cluster=ofn->fsdp.results.sector_size=ofn->fsdp.results.disk_format=0;
              ofn->fsdp.properties_status = FS_GET_PROPERTIES_DISK_NOT_MOUNTED;
              }
            }
            break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case DEVICE_NETWORK:
            // mandare STAT o boh
            ofn->fsdp.results.disk_format=FAT32;
            ofn->fsdp.results.free_clusters=1;
            ofn->fsdp.results.sectors_per_cluster=1;
            ofn->fsdp.properties_status = FS_GET_PROPERTIES_NO_ERRORS;
            ofn->fsdp.new_request=0;
            ofn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
            break;
#endif
          }
        }

      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        nmh.code=CDN_INITDONE;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // beh tanto per :) anche se è per file dialog chooser
        }

fine:      
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      if(!GetWindowByte(hWnd,GWL_USERDATA+2)) {
        SetWindowText(GetDlgItem(hWnd,201),"attendere prego...");
        //e fare magari pure un Mount o FSinit..
        }
          
//          drawIcon8(&hWnd->hDC,0,16,folderIcon);
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      // forzare font piccolo??
      int i;

      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
			memset(ofn,sizeof(OPENFILENAME),0);
      if(cs->lpCreateParams) {
        strncpy(ofn->path,(char*)cs->lpCreateParams,sizeof(ofn->path));
        ofn->path[sizeof(ofn->path)]=0;
        }
      HWND myWnd=CreateWindow(MAKECLASS(WC_STATUSBAR),NULL,WS_VISIBLE | WS_CHILD,
        0,cs->cy-8,cs->cx,cs->cy,    // 
        hWnd,(HMENU)201,NULL
        );
      }
      return 0;
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_SPACE:
          if(hWnd->parent && !hWnd->parent->internalState) {
            NMHDR nmh;
            nmh.code=CDN_FILEOK;
            nmh.hwndFrom=hWnd;
            nmh.idFrom=(DWORD)hWnd->menu;
            PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // beh tanto per :) anche se è per file dialog chooser
            }
          // in effetti sa
          break;
        case VK_DELETE:
          {
          // CHIEDERE CONFERMA!! v. altra
//          SuperFileDelete();
          }
          break;
        case 'I':
          {NMHDR nmh;
          nmh.code=CDN_TYPECHANGE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          SendMessage(hWnd,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
          }
          break;
        default:
          return DefDlgProc(hWnd,message,wParam,lParam);
          break;
        }

      break;

    case WM_LBUTTONDOWN:
      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        nmh.code=NM_LDOWN;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
        }
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
    case WM_LBUTTONDBLCLK:
      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        nmh.code=NM_DBLCLK;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
        }
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_NOTIFY:
      switch(lParam) {
        case CDN_TYPECHANGE:
          {
          SetWindowByte(hWnd,GWL_USERDATA+1,(GetWindowByte(hWnd,GWL_USERDATA+1)+1) % 3);
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        }
      break;

    case WM_FILECHANGE:
      break;
      
    case WM_SIZE:
      {
//        MoveWindow(myWnd,0,HIWORD(lParam)-8-1,LOWORD(lParam),8+1,TRUE);// cmq si autogestisce! 
      WINDOWPOS wpos;
      wpos.hwnd=hWnd;
      wpos.hwndInsertAfter=NULL;
      wpos.x=hWnd->nonClientArea.left;
      wpos.y=hWnd->nonClientArea.top;
      wpos.cx=LOWORD(lParam);
      wpos.cy=HIWORD(lParam);
      wpos.flags=SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW;
      SendMessage(GetDlgItem(hWnd,201),WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
      }
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_HSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_VSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    default:
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


typedef struct __attribute((packed)) _XLISTITEM {
  char filename[20];
  S_RECT rc;
//    struct _XLISTITEM *prev;   // volendo :)
//  FILETIME ft; idem
  struct _XLISTITEM *next;
  BYTE disk;
  BYTE type;
  BYTE state;     // b0=selected
  char filler[1];
  } XLISTITEM;
STATIC_ASSERT(!(sizeof(struct _XLISTITEM) % 4),0);

static XLISTITEM *insertDiskItem(XLISTITEM **root) {
  XLISTITEM *item=*root;
  
  if(item) {
    while(item && item->next) {
      item=item->next;
      }
    item->next=malloc(sizeof(XLISTITEM));
    item=item->next;
    }
  else
    *root=item=malloc(sizeof(XLISTITEM));
  item->next=NULL;
  return item;
  }
static XLISTITEM *freeDiskList(XLISTITEM *root) {
  while(root) {
    XLISTITEM *item2=root->next;
    free(root);
    root=item2;
    }
  return NULL;
  }
static XLISTITEM *fillDiskList(char disk,/*const*/ char *path) {
  SearchRec rec;
  int i; 
  BYTE j;
  char label[16 /*TOTAL_FILE_SIZE+1*/];
  WORD totFiles;
  XLISTITEM *item=NULL,*item2;
  MEDIA_INFORMATION *mi;
  
  *label=0;
  totFiles=0;
  if(!disk) {
    i=0;
    goto elenco_dischi;
    }
  mi=getMediaInfo(disk);
  if(!mi || mi->errorCode != MEDIA_NO_ERROR)
    goto fine;
  switch(disk) {
    uint32_t sn;
    case 'A':
    case 'B':
    case 'C':
    case 'D':
#ifdef USA_RAM_DISK 
    case DEVICE_RAMDISK:
#endif
#if defined(USA_USB_HOST_MSD)
    case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
    case DEVICE_NETWORK:
#endif
      SuperFileGetVolume(disk,label,&sn);
      if(*path)
        SuperFileChDir(disk,path);    // va fatto una per una... o creare funzione nel file system...
      else
        SuperFileChDir(disk,ROOTDIR);
      SuperFileGetCWD(disk,path,20);
      i=SuperFileFindFirst(disk,ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
      break;
    default:
      goto fine;
      break;
    }

  if(disk) {
    item2=insertDiskItem(&item);
    strncpy(item2->filename,label,sizeof(item2->filename)-1);
    item2->filename[sizeof(item2->filename)-1]=0;
    item2->type=ATTR_VOLUME;
    item2->state=1; item2->disk=disk;
    } 

elenco_dischi:
  if(!i) {

loop:
    totFiles++;
    
    if(disk) {
      if(*rec.filename != '.' || rec.filename[1]) {   // la "." non ha senso cmq
        item2=insertDiskItem(&item);
        strncpy(item2->filename,rec.filename,sizeof(item2->filename)-1);
        item2->filename[sizeof(item2->filename)-1]=0;
        item2->type=rec.attributes;
        item2->state=0; item2->disk=disk;
        }
      }
    switch(disk) {
      case 'A':
      case 'B':
      case 'C':
      case 'D':
#ifdef USA_RAM_DISK 
      case DEVICE_RAMDISK:
#endif
#if defined(USA_USB_HOST_MSD)
      case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
      case DEVICE_NETWORK:
#endif
        if(!SuperFileFindNext(disk,&rec))
          goto loop;
        break;
      case 0:		// elenco dischi
        *label=0;
        j=0;
        mi=getMediaInfo(i+'A');
        if(mi && mi->errorCode == MEDIA_NO_ERROR) {
          uint32_t timestamp;
          switch(i+'A') {
            case 'A':
            case 'B':
            case 'C':
            case 'D':
#ifdef USA_RAM_DISK 
            case DEVICE_RAMDISK:
#endif
#if defined(USA_USB_HOST_MSD)
            case 'E':
#endif
              SuperFileGetVolume(i+'A',label,&timestamp);
              j=1;
              totFiles++;
              break;
#if defined(USA_WIFI) || defined(USA_ETHERNET)
            case DEVICE_NETWORK:
//              SuperFileGetVolume(networkFile.drive,label,&timestamp);
              strncpy(label,networkFile.serverpath,sizeof(label)-1);
              label[sizeof(label)-1]=0;
              j=1;
              totFiles++;
              break;
#endif
            default:
              break;
            }
          }
        if(j) {
          item2=insertDiskItem(&item);
          strncpy(item2->filename,label,sizeof(item2->filename)-1);
          item2->filename[sizeof(item2->filename)-1]=0;
          
          item2->state=0; 
          item2->disk=i+'A';
          item2->type=0xff;   // disco
          }
        i++;
        if(i<26)
          goto loop;
        break;
      default:
        break;
      }

    }
  
fine:
  return item;
  }

static XLISTITEM *getDiskItemFromPoint(XLISTITEM *root,RECT *rc,BYTE tipoVis,POINTS pt) {
  RECT rc2;
  POINT ptl={pt.x,pt.y};
  
#warning il click sulle icone grandi in filemanager sembra ciucco, va gestito il caso particolare dei DISCHI
  
  rc2.left=rc->left;
  rc2.top=rc->top;
  while(root) {
    switch(tipoVis) {
      case 0:
        if(root->type==0xff) {
          rc2.right=rc2.left+X_SPACING_SMALLICONS_DISK-1;
          rc2.bottom=rc2.top+Y_SPACING_SMALLICONS_DISK-1;
          }
        else {
          rc2.right=rc2.left+X_SPACING_SMALLICONS-1;
          rc2.bottom=rc2.top+Y_SPACING_SMALLICONS-1;
          }
        break;
      case 1:
        if(root->type==0xff) {
          rc2.right=rc2.left+X_SPACING_LARGEICONS_DISK-1;
          rc2.bottom=rc2.top+Y_SPACING_LARGEICONS_DISK-1;
          }
        else {
          rc2.right=rc2.left+X_SPACING_LARGEICONS-1;
          rc2.bottom=rc2.top+Y_SPACING_LARGEICONS-1;
          }
        break;
      case 2:
        rc2.right=rc->right-1;
        rc2.bottom=rc2.top+Y_SPACING_DETAILS-1;
        break;
      }
    if(wndPtInRect(&rc2,ptl))
      break;
    switch(tipoVis) {
      case 0:
        if(root->type==0xff) {
          rc2.left=rc2.left+X_SPACING_SMALLICONS_DISK;
          if(rc2.left>=rc->right) {
            rc2.left=rc->left;
            rc2.top+=Y_SPACING_SMALLICONS_DISK;
            }
          }
        else {
          rc2.left=rc2.left+X_SPACING_SMALLICONS;
          if(rc2.left>=rc->right) {
            rc2.left=rc->left;
            rc2.top+=Y_SPACING_SMALLICONS;
            }
          }
        break;
      case 1:
        if(root->type==0xff) {
          rc2.left=rc2.left+X_SPACING_LARGEICONS_DISK;
          if(rc2.left>=rc->right) {
            rc2.left=rc->left;
            rc2.top+=Y_SPACING_LARGEICONS_DISK;
            }
          }
        else {
          rc2.left=rc2.left+X_SPACING_LARGEICONS;
          if(rc2.left>=rc->right) {
            rc2.left=rc->left;
            rc2.top+=Y_SPACING_LARGEICONS;
            }
          }
        break;
      case 2:
        rc2.top+=Y_SPACING_DETAILS;
        break;
      }
    root=root->next;
    }
  return root;
  }

int getDiskItemPosition(XLISTITEM *root,XLISTITEM *item,RECT *rc,BYTE tipoVis,POINTS *pt) {
  int n=0;
  UGRAPH_COORD_T x=0,y=0;
  
  while(root) {
    if(root == item) {
      if(pt) {
        pt->x=x; pt->y=y;
        }
      break;
      }
    switch(tipoVis) {
      case 0:
        if(root->type==0xff) {
          x+=X_SPACING_SMALLICONS_DISK;
          if(x>=rc->right) {
            x=0;
            y+=Y_SPACING_SMALLICONS_DISK;
            }
          }
        else {
          x+=X_SPACING_SMALLICONS;
          if(x>=rc->right) {
            x=0;
            y+=Y_SPACING_SMALLICONS;
            }
          }
        break;
      case 1:
        if(root->type==0xff) {
          x+=X_SPACING_LARGEICONS_DISK;
          if(x>=rc->right) {
            x=0;
            y+=Y_SPACING_LARGEICONS_DISK;
            }
          }
        else {
          x+=X_SPACING_LARGEICONS;
          if(x>=rc->right) {
            x=0;
            y+=Y_SPACING_LARGEICONS;
            }
          }
        break;
      case 2:
        y+=Y_SPACING_DETAILS;
        break;
      }
    n++;
    root=root->next;
   }
  return n;
  }
XLISTITEM *getSelectedDiskItem(XLISTITEM *root) {

  while(root) {
    if(root->state)
      break;
    root=root->next;
   }
  return root;
  }

int getFileInfo(const char *file,ICON *icon,BYTE size) {   // poi, nel registry :)
  int t=-1;
  ICON aicon=NULL;

  if(stristr(file,".bas")) {
    t=TIPOFILE_BASIC;
    aicon=size ? minibasicIcon : minibasicIcon8;
    }
  else if(stristr(file,".f")) {
    t=TIPOFILE_FORTH;
    aicon=size ? windowIcon : windowIcon8;
    }
  else if(stristr(file,".rex")) {
    t=TIPOFILE_REXX;
    aicon=size ? windowIcon : windowIcon8;
    }
  else if(stristr(file,".jar" /*.class?*/)) {
    t=TIPOFILE_JAVA;
    aicon=size ? windowIcon : windowIcon8;
    }
  else if(stristr(file,".lnk")) {
    t=TIPOFILE_LINK;
    aicon=size ? windowIcon : windowIcon8;
    }
  else if(stristr(file,".JPG") || stristr(file,".BMP") || stristr(file,".PNG")) {
    t=TIPOFILE_IMMAGINE;
    aicon=size ? redBallIcon : redBallIcon8;
    }
  else if(stristr(file,".TXT") || stristr(file,".INI")) {
    t=TIPOFILE_TESTO;
    aicon=size ? fileIcon : fileIcon8;
    }
  else if(stristr(file,".MP3") || stristr(file,".WAV")) {
    t=TIPOFILE_AUDIO;
    aicon=size ? audioIcon : audioIcon8;
    }
  else if(stristr(file,".HTM") || stristr(file,".XML")) {
    t=TIPOFILE_INTERNET;
    aicon=size ? surfIcon : windowIcon8;
    }
  else if(stristr(file,".BAT")) {
    t=TIPOFILE_PROGRAMMA;
    aicon=size ? dosIcon : dosIcon8;
    }
  else if(stristr(file,".ZIP")) {
    t=TIPOFILE_ZIP;
    aicon=size ? zipIcon : zipIcon;
    }
  else if(stristr(file,".")) {    // bah
    t=0;
    aicon=NULL;
    }

  if(icon)
    *icon=aicon;
  return t;
  }


void splitpath(const char *path, char *drv, char *dir, char *name, char *ext) {
  // http://www.sky.franken.de/doxy/explorer/splitpath_8c_source.html
  const char *end; /* end of processed string */
	const char *p;	  /* search pointer */
	const char *s;	  /* copy pointer */

	// extract drive name 
	if(path[0] && path[1]==':') {
		if(drv) {
			*drv++ = *path++;
// a me non serve!			*drv++ = *path++;
//			*drv = '\0';
  		}
    }
  else if(drv)
		*drv = '\0';

  end = path + strlen(path);

	// search for beginning of file extension 
	for(p=end; p>path && *--p!='\\' && *p!='/'; ) {
		if(*p == '.') {
      end = p;
      break;
      }
    }

  if(ext)
    for(s=end; (*ext=*s++); )
      ext++;

  // search for end of directory name 
  for(p=end; p>path; ) {
    if(*--p=='\\' || *p=='/') {
      p++;
      break;
      }
    }

  if(name) {
    for(s=p; s<end; )
      *name++ = *s++;

    *name = '\0';
    }

  if(dir) {
    for(s=path; s<p; )
      *dir++ = *s++;

    *dir = '\0';
    }
  }

LRESULT DefWindowProcDirWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
	XLISTITEM *item;
  DIRLIST *dfn=(DIRLIST*)GET_WINDOW_OFFSET(hWnd,0);
  XLISTITEM *root=(XLISTITEM*)GetWindowLong(hWnd,sizeof(DIRLIST));
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int i; 
      UGRAPH_COORD_T x,y;
      WORD totFiles;
      ICON aicon;
      RECT rc;
      GetClientRect(hWnd,&rc);
      MEDIA_INFORMATION *mi;
      
      HDC hDC=BeginPaint(hWnd,&ps);
      
      if(!GetWindowByte(hWnd,GWL_USERDATA+2))
        fillRectangleWindow(hDC,rc.left,rc.top,rc.right,rc.bottom);
      
      totFiles=0;
      item=root;
      if(GetWindowByte(hWnd,GWL_USERDATA+2)<2) {
//v.patch
        freeDiskList(root);
        item=root=fillDiskList(dfn->disk,dfn->path);
        SetWindowLong(hWnd,sizeof(DIRLIST),(DWORD)root);
        SetWindowByte(hWnd,GWL_USERDATA+2,2);
  			if(hWnd->parent && !hWnd->parent->internalState) {
          NMHDR nmh;
          nmh.code=CDN_INITDONE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
    			PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // beh tanto per :) anche se è per file dialog chooser
          }
        }
      if(item && dfn->disk) 
        item=item->next;    // il primo è etichetta volume! (se disco)

      if(GetWindowByte(hWnd,GWL_USERDATA+1)<1 && item)    // se icone piccole uso font piccolissimo! (se il disco c'è)
        hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font;
      else
        hDC->font=GetStockObject(SYSTEM_FONT).font;

      if(!dfn->disk)
        SetWindowText(hWnd,"Dischi");

      i=0;      
      if(GetWindowLong(hWnd,sizeof(DIRLIST))) {   // se c'è almeno il volume! ossia il disco
        x=0; y=0;
        while(item) {
        
loop:
					totFiles++;
          if(dfn->disk) {   // v. anche item->state=0x0
            if(y<ps.rcPaint.bottom) {
              RECT rc2;
              switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
                case 0:     // icone piccole
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_SMALLICONS-3-3; item->rc.bottom=item->rc.top+y+3+4+8+8+8+2;
                  rc2.left=item->rc.left+1; rc2.top=item->rc.top+1+8+2; rc2.right=item->rc.right-1; rc2.bottom=item->rc.bottom-1;
                  SetTextColor(hDC,item->type & ATTR_HIDDEN ? GRAY192 : WHITE);
                  DrawText(hDC,item->filename,-1,&rc2,DT_TOP | DT_CENTER);
                  getFileInfo(item->filename,&aicon,0);
                  if(item->type & ATTR_DIRECTORY) {
                    drawIcon8(hDC,x+18,y+4,folderIcon8);
                    }
                  else {
                    drawIcon8(hDC,x+18,y+4,aicon ? aicon : windowIcon8);
                    }
                  if(item->state) {
                    drawRectangleWindow(hDC,x+2,y,x+X_SPACING_SMALLICONS-1,y+24+2);
                    }
                  x+=X_SPACING_SMALLICONS;
                  if(x>=rc.right) {
                    x=0;
                    y+=Y_SPACING_SMALLICONS;
                    }
                  break;
                case 1:     // icone grandi
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_LARGEICONS-3-3; item->rc.bottom=item->rc.top+y+3+4+16+8+8+2;
                  rc2.left=item->rc.left+1; rc2.top=item->rc.top+1+16+2; rc2.right=item->rc.right-1; rc2.bottom=item->rc.bottom-1;
                  SetTextColor(hDC,item->type & ATTR_HIDDEN ? GRAY192 : WHITE);
                  DrawText(hDC,item->filename,-1,&rc2,DT_TOP | DT_CENTER);
                  getFileInfo(item->filename,&aicon,1);
                  if(item->type & ATTR_DIRECTORY) {
                    DrawIcon(hDC,x+25,y+4,folderIcon);
                    }
                  else {
                    DrawIcon(hDC,x+25,y+4,aicon ? aicon : windowIcon);
                    }
// in effetti il rettangolo per evidenziare dipende dal testo a capo o no.. farlo restituire da DrawText con flag??
                  if(item->state) {
                    drawRectangleWindow(hDC,x+1,y+1,x+X_SPACING_LARGEICONS-1,y+32+5);
                    }
                  x+=X_SPACING_LARGEICONS;
                  if(x>=rc.right) {
                    x=0;
                    y+=Y_SPACING_LARGEICONS;
                    }
                  break;
                case 2:     // dettagli
                  item->rc.left=x+1; item->rc.top=y+1;
                  item->rc.right=rc.right-1; item->rc.bottom=item->rc.top+y+8+1;
  //                item->filename[16]=0; INUTILE nomi file corti :)
                  SetTextColor(hDC,item->type & ATTR_HIDDEN ? GRAY192 : WHITE);
                  TextOut(hDC,x+2,y+2,item->filename,strlen(item->filename));
                  if(item->type & ATTR_DIRECTORY) {
                    TextOut(hDC,x+13*6+2,y+2,"DIR",3);
                    }
                  else {
                    char buf[32];
                    struct FSstat st;
                    mi=getMediaInfo(dfn->disk);
                    if(mi && mi->errorCode == MEDIA_NO_ERROR) {
                      switch(dfn->disk) {
                        case 'A':
                        case 'B':
                        case 'C':
                        case 'D':
#ifdef USA_RAM_DISK 
                        case DEVICE_RAMDISK:
#endif
#if defined(USA_USB_HOST_MSD)
                        case 'E':
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
                        case DEVICE_NETWORK:
#endif
                          SuperFileStat(dfn->disk,item->filename,&st);
                          break;
                        }
                      }
                    itoa(buf,st.st_size,10);
                    TextOut(hDC,x+13*6,y+2,buf,strlen(buf));
                    FILETIMEPACKED ft;
                    ft.v=st.st_mtime;
                    sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",    // v. anche FileTimeToSystemTime
                      ft.day,ft.mon,ft.year+1980,
                      ft.hour,ft.min,ft.sec);
                    TextOut(hDC,x+(14+10)*6,y+2,buf,strlen(buf));
                    }
                  if(item->state) {
                    drawRectangleWindow(hDC,x,y,rc.right-1,y+9);
                    }
                  y+=8;
                  break;
                }
              }
            }     //if(dfn->disk
          if(!dfn->disk) {		// elenco dischi, v. anche item->state=0xff
            dfn->fsdp.new_request=1;
            mi=getMediaInfo(item->disk);
            switch(item->disk) {
              case 'A':
              case 'B':
              case 'C':
              case 'D':
#ifdef USA_RAM_DISK 
              case DEVICE_RAMDISK:
#endif
                if(mi && mi->errorCode == MEDIA_NO_ERROR) {
                  do {
                    FSGetDiskProperties(mi,&dfn->fsdp);
                    ClrWdt();
                    } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
                  totFiles++;
                  }
                break;
#if defined(USA_USB_HOST_MSD)
              case 'E':
                {
                uint32_t freeSectors,totalSectors,sectorSize,sn,timestamp;
                if(USBMediaInfo.errorCode == MEDIA_NO_ERROR) {
                  if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
                    dfn->fsdp.results.free_clusters=freeSectors;
                    dfn->fsdp.results.sectors_per_cluster=1;
                    dfn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
                    SYS_FS_GetDiskInfo(NULL,&dfn->fsdp.results.disk_format); // OCCHIO che cmq è diversa!
                    dfn->fsdp.properties_status = FS_GET_PROPERTIES_NO_ERRORS;
                    dfn->fsdp.new_request=0;
                    }
                  else {
                    dfn->fsdp.results.free_clusters=dfn->fsdp.results.sectors_per_cluster=dfn->fsdp.results.sector_size=dfn->fsdp.results.disk_format=0;
                    dfn->fsdp.properties_status = FS_GET_PROPERTIES_DISK_NOT_MOUNTED;
                    }
                  totFiles++;
                  }
                }
                break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
              case DEVICE_NETWORK:
                if(mi && mi->errorCode == MEDIA_NO_ERROR) {
                  dfn->fsdp.results.free_clusters=1;
                  dfn->fsdp.results.sectors_per_cluster=1;
                  dfn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE;
                  dfn->fsdp.new_request=0;
                  dfn->fsdp.properties_status = FS_GET_PROPERTIES_NO_ERRORS;
                  
                  // v. https://stackoverflow.com/questions/56060299/how-to-check-free-space-in-a-ftp-server
                  totFiles++;
                  }
                break;
#endif
              default:
                break;
              }
            if(!dfn->fsdp.new_request) {
              switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
                RECT rc2;
                case 0:     // icone piccole
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_SMALLICONS_DISK-3-3; item->rc.bottom=item->rc.top+y+3+4+8+8+8+2;
                  drawIcon8(hDC,x+   getFontWidth(&hDC->font)*4,y+4,diskIcon8);
                  rc2.left=item->rc.left+1; rc2.top=item->rc.top+1+8+2; rc2.right=item->rc.right-1; rc2.bottom=item->rc.bottom-1;
                  {char buf[16]={item->disk,0};
                  if(*item->filename) {
                    strcpy(buf,item->filename);
                    StrTrim(buf," ");
                    }
                  DrawText(hDC,buf,-1,&rc2,DT_TOP | DT_CENTER);
                  }
                  if(item->state) {
                    drawRectangleWindow(hDC,x+1,y+1,x+39,y+31);
                    }
                  x+=X_SPACING_SMALLICONS_DISK;
                  if(x>=rc.right) {
                    x=0;
                    y+=Y_SPACING_SMALLICONS_DISK;
                    }
                  break;
                case 1:     // icone grandi
                  {SIZE sz;
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_LARGEICONS_DISK-3-3; item->rc.bottom=item->rc.top+y+3+4+16+8+8+2;
                  DrawIcon(hDC,x+6*getFontWidth(&hDC->font),y+4,diskIcon);		// 
                  {char buf[16]={item->disk,0};
                  if(*item->filename) {
                    strcpy(buf,item->filename);
                    StrTrim(buf," ");
                    }
                  rc2.left=item->rc.left+1; rc2.top=item->rc.top+2+16+1; rc2.right=item->rc.right-1; rc2.bottom=item->rc.bottom-1;
                  DrawText(hDC,buf,-1,&rc2,DT_TOP | DT_CENTER);
                  }
                  sz.cx=13*getFontWidth(&hDC->font);
                  sz.cy=8;
                  drawRectangleWindow(hDC,x+1,y+4+16+10,x+sz.cx,y+4+16+10+sz.cy);
                  if(dfn->fsdp.results.total_clusters) {
                    sz.cx=(sz.cx*(dfn->fsdp.results.total_clusters-dfn->fsdp.results.free_clusters)) /
                      dfn->fsdp.results.total_clusters;
                    fillRectangleWindowColor(hDC,x+2,y+5+16+10,x+2+sz.cx-1,y+5+16+10-1+sz.cy,
                      hDC->pen.color);
                    }
                  if(item->state) {
                    drawRectangleWindow(hDC,x+1,y+1,x+79,y+42);
                    }
                  x+=X_SPACING_LARGEICONS_DISK;    // X_SPACING_LARGEICONS_DISK METTERE OVUNQUE
                  if(x>=rc.right) {
                    x=0;
                    y+=Y_SPACING_LARGEICONS_DISK;
                    }
                  }
                  break;
                case 2:     // dettagli
                  {char buf[16];
                  item->rc.left=x+1; item->rc.top=y+1;
                  item->rc.right=rc.right-1; item->rc.bottom=item->rc.top+y+8+1;
                  hDC->font.bold=1;   // non è bello ma ok :)
                  buf[0]=item->disk; buf[1]=0;
                  TextOut(hDC,x+2,y+2,buf,strlen(buf)); 
                  hDC->font.bold=0;
                  TextOut(hDC,x+12,y+2,item->filename,strlen(item->filename)); 
                  sprintf(buf,"%lu/",
                    dfn->fsdp.results.free_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024);
                  TextOut(hDC,x+16*  6 /*getFontWidth(&hDC->font)*/,y+2,buf,strlen(buf));
                  sprintf(buf,"%lu",
                    dfn->fsdp.results.total_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024);
                  TextOut(hDC,x+24* 6/*getFontWidth(&hDC->font)*/,y+2,buf,strlen(buf));
                  TextOut(hDC,x+32*6 /*getFontWidth(&hDC->font)*/,y+2,"Kbytes",6);
                  if(item->state) {
                    drawRectangleWindow(hDC,x,y,rc.right-1,y+9);
                    }
                  y+=10;
                  }
                  break;
                }
              }
            }
          item=item->next;
          }   // while item

        switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
          case 0:
            y=((rc.right-rc.left)/X_SPACING_SMALLICONS)*
              ((rc.bottom-rc.top)/Y_SPACING_SMALLICONS);
            if(totFiles>y) {
//              EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_VERT,0,
                1+((totFiles-y)/((rc.right-rc.left)/X_SPACING_SMALLICONS)),
                FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //bah
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
//            EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //bah
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            ShowScrollBar(hWnd,SB_HORZ,FALSE); //
            break;
          case 1:
            y=((rc.right-rc.left)/X_SPACING_LARGEICONS)*
              ((rc.bottom-rc.top)/Y_SPACING_LARGEICONS);
            if(totFiles>y) {
//              EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_VERT,0,
                1+((totFiles-y)/((rc.right-rc.left)/X_SPACING_LARGEICONS)),
                FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,TRUE); //
              }
            else {
 //             EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //bah
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
//            EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //bah
            SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
            SetScrollPos(hWnd,SB_HORZ,0,FALSE);
            ShowScrollBar(hWnd,SB_HORZ,FALSE); //
            break;
          case 2:
            y=(rc.bottom-rc.top)/Y_SPACING_DETAILS;
            if(totFiles>y) {
//              EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_VERT,0,y-totFiles,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
            y=(rc.right-rc.left)/getFontWidth(&hDC->font);
            if(40>y) {   // v. testo sopra
//              EnableScrollBar(hWnd,SB_HORZ,ESB_ENABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_HORZ,0,y - 40,FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              ShowScrollBar(hWnd,SB_HORZ,TRUE); //
              }
            else {
//              EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //bah
              SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
              SetScrollPos(hWnd,SB_HORZ,0,FALSE);
              ShowScrollBar(hWnd,SB_HORZ,FALSE); //
              }
// si potrebbe mettere un flag che alternativamente fa una cosa e poi l'altra
            break;
          }

        if(dfn->disk) {
          switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
            case 0:
              y+=28;
              break;
            case 1:
              y+=38;
              break;
            case 2:
              y+=6;
              break;
            }
          if(GetWindowByte(hWnd,GWL_USERDATA+2)<3) {
            dfn->fsdp.new_request=1;
            mi=getMediaInfo(dfn->disk);
            switch(dfn->disk) {
              case 'A':
              case 'B':
              case 'C':
              case 'D':
    #ifdef USA_RAM_DISK 
              case DEVICE_RAMDISK:
    #endif
                do {
                  FSGetDiskProperties(mi,&dfn->fsdp);
                  ClrWdt();
                  } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
                break;
    #if defined(USA_USB_HOST_MSD)
              case 'E':
                {
                uint32_t freeSectors,totalSectors,sectorSize;
                if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
                  dfn->fsdp.results.free_clusters=freeSectors;
                  dfn->fsdp.results.sectors_per_cluster=1;
                  dfn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
                  dfn->fsdp.properties_status = FS_GET_PROPERTIES_NO_ERRORS;
                  dfn->fsdp.new_request=0;
                  SYS_FS_GetDiskInfo(NULL,&dfn->fsdp.results.disk_format); // OCCHIO che cmq è diversa!
                  }
                else {
                  dfn->fsdp.results.free_clusters=dfn->fsdp.results.sectors_per_cluster=dfn->fsdp.results.sector_size=dfn->fsdp.results.disk_format=0;
                  dfn->fsdp.properties_status = FS_GET_PROPERTIES_DISK_NOT_MOUNTED;
                  }
                }
                break;
    #endif
    #if defined(USA_WIFI) || defined(USA_ETHERNET)
              case DEVICE_NETWORK:
                dfn->fsdp.results.free_clusters=1;
                dfn->fsdp.results.sectors_per_cluster=1;
                dfn->fsdp.properties_status = FS_GET_PROPERTIES_NO_ERRORS;
                dfn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
                dfn->fsdp.new_request=0;
                break;
    #endif
              case 0:		// elenco dischi
                break;
              default:
                break;
              }
            }
          else
            goto print_totals;
          if(dfn->fsdp.properties_status == FS_GET_PROPERTIES_NO_ERRORS) {
            char buf[32],buf2[32];

print_totals:          
            sprintf(buf,"%lu Kbytes free",dfn->fsdp.results.free_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024); 
            SetWindowText(GetDlgItem(hWnd,201),buf);
            SetWindowByte(hWnd,GWL_USERDATA+2,3);
            item=root;   // recupero etichetta volume
            strcpy(buf2,*item->filename ? item->filename : "Disk"); 
            StrTrim(buf2," "); 
            sprintf(buf,"%s(%c) %s",buf2,toupper(dfn->disk),
              *dfn->path ? dfn->path : ASTERISKS);
//#warning togliere asterisk ?!
            
            SetWindowText(hWnd,buf);
            }
          else {
            SetWindowText(GetDlgItem(hWnd,201),"?  Kbytes free");
            }
          }		// if disk
        else {
          char buf[16];
          i=0;
          while(root) {
						i++;
            root=root->next;
						}
          sprintf(buf,"%u dischi presenti",i); 
          SetWindowText(GetDlgItem(hWnd,201),buf);
          }
        }
      else {
        SetWindowByte(hWnd,GWL_USERDATA+2,1);
        if(dfn->disk) {
          SetWindowText(hWnd,"Disco");
          char buf[32];
          sprintf(buf,"Inserire disco nell'unità %c:",dfn->disk);
          SetWindowText(GetDlgItem(hWnd,201),buf);
          }
        else
          SetWindowText(GetDlgItem(hWnd,201),"Nessun disco rilevato");
        }

      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      if(!GetWindowByte(hWnd,GWL_USERDATA+2)) {
        DrawText(hDC,"attendere prego...",-1,&hWnd->paintArea,DT_VCENTER | DT_CENTER);
        //e fare magari pure un Mount o FSinit..
        }
//          drawIcon8(&hWnd->hDC,0,16,folderIcon);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
			memset(dfn,sizeof(DIRLIST),0);
      SetWindowLong(hWnd,sizeof(DIRLIST),0); 
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      if(cs->lpCreateParams) {
        strncpy(dfn->path,(char*)cs->lpCreateParams,sizeof(dfn->path));
        dfn->path[sizeof(dfn->path)-1]=0;
        }

      CreateWindow(MAKECLASS(WC_STATUSBAR),NULL,WS_VISIBLE | WS_CHILD,
        0,cs->cy-8,cs->cx,cs->cy,    // se thickborder deve andare più in giù e + larga, pare CMQ SI AUTOGESTISCE!
        hWnd,(HMENU)201,NULL
        );
      
/*      v. shellexecute, ev. fare splitpath,*/
      splitpath(dfn->path,&dfn->disk,NULL,NULL,NULL);
      if(dfn->disk)   // se dischi, parto con icone grandi altrimenti piccole!
        SetWindowByte(hWnd,GWL_USERDATA+1,0);
      else
        SetWindowByte(hWnd,GWL_USERDATA+1,1);
      }
      return 0;
      break;
    case WM_DESTROY:
      freeDiskList(root);
//      m_WndFileManager[0]=NULL;
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_SIZE:
//        MoveWindow(myWnd,0,HIWORD(lParam)-8-1,LOWORD(lParam),8+1,TRUE);// cmq si autogestisce! 
      {
      WINDOWPOS wpos;
      wpos.hwnd=hWnd;
      wpos.hwndInsertAfter=NULL;
      wpos.x=hWnd->nonClientArea.left;
      wpos.y=hWnd->nonClientArea.top;
      wpos.cx=LOWORD(lParam);
      wpos.cy=HIWORD(lParam);
      wpos.flags=SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW;
      SendMessage(GetDlgItem(hWnd,201),WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_KEYDOWN:
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      item=getSelectedDiskItem(root);
      switch(wParam) {
        case VK_SPACE:
/*          if(GetAsyncKeyState(VK_MENU) & 0x8000 || GetAsyncKeyState(VK_CONTROL) & 0x8000)
#warning togliere, gestire da main loop
            return DefWindowProc(hWnd,message,wParam,lParam);
          else {*/
            if(item)
              item->state ^= 1; // non è proprio giusto.. deseleziona solo! ma ha senso cmq? usa button
            
new_key_selected:
//v. patch
            GetSubMenu(GetMenu(hWnd),0);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent && !hWnd->parent->internalState) {
              NMHDR nmh;
              nmh.code=NM_KEYDOWN;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
              }
//            }
          break;
        case VK_RETURN:
          if(item)
            goto dblclk;
          break;
        case VK_ESCAPE:
          if(item) {
            item->state = 0;
						goto new_key_selected;
            }
          break;
        case VK_UP:
          {
            POINTS pt;
            int n;
            if(item)
              item->state=0;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
  					goto new_item_selected;
          }
	        break;
        case VK_DOWN:
          {
            POINTS pt;
            int n;
            if(item)
              item->state=0;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
						goto new_item_selected;
          }
          break;
        case VK_LEFT:
          {
            POINTS pt;
            int n;
            if(item)
              item->state=0;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
            // finire!
            else
              item=root;
						goto new_item_selected;
          }
          break;
        case VK_RIGHT:
          {
            POINTS pt;
            int n;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
            if(item) {
              item->state=0;
							item=item->next;    // FINIRE CON riga succ ecc!
              }
            else
              item=root;
						goto new_item_selected;
          }
          break;
        case VK_HOME:
          if(item)
            item->state=0;
          item=root;
          
new_item_selected:
          if(item)
            item->state=1;
					goto new_key_selected;
          break;
        case VK_END:
          if(item)
            item->state=0;
          item=root;
          while(item && item->next)
            item=item->next;
          if(item) {// dovrebbe anche scrollare per mostrare...
            item->state=1;
//						goto new_item_selected;
            }
					goto new_key_selected;
          break;

        case VK_BACK:
          if(dfn->disk && (dfn->path[0]=='\\' || dfn->path[0]=='/') && !dfn->path[1]) {   // se root di un disco, esco a "tutti i dischi"
            dfn->disk=0;
            goto rebuild;
            }
          else if(strchr(dfn->path,'\\') || strchr(dfn->path,'/')) {    // se in subfolder, vado a root
            // alla veloce per ora!
            dfn->path[0]='\\'; dfn->path[1]=0;
rebuild:
            SetWindowByte(hWnd,GWL_USERDATA+2,0);
            InvalidateRect(hWnd,NULL,TRUE);
            }
          break;
          
        case VK_DELETE:
          {
          // CHIEDERE CONFERMA!! 
          if(item) {
            switch(item->type) {
              case 0xff:
                break;
              default:
                if(item->type & ATTR_DIRECTORY) {
                  MessageBeep(MB_ICONINFORMATION);
                // finire!
                  }
                else {
                  SUPERFILE f;
                  f.drive=item->disk;
                  SuperFileDelete(&f,item->filename);
                  }
                SetWindowByte(hWnd,GWL_USERDATA+2,0); 
                break;
              }
            }
          }
          break;
        case VK_F5:
          goto aggiorna;
          break;
/* prove...        case 'I':
          {NMHDR nmh;
          nmh.code=CDN_TYPECHANGE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          SendMessage(hWnd,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
          }
          break;
		  */
          
        }
      return 0 /*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;
    case WM_CHAR:   // case sensitive!
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      item=getSelectedDiskItem(root);
      if(isalnum(wParam)) {
        item=getSelectedDiskItem(root);
        if(item && item->next) {
          item->state=0;
          item=item->next;
          }
        else {
          if(dfn->disk && root) 
            root=root->next;    // il primo è etichetta volume! (se disco)
          item=root;
          }
        while(item) {
          if(toupper(item->filename[0])==toupper(wParam))
            break;
          item=item->next;
          }// dovrebbe anche scrollare per mostrare...
        if(item) {
          item->state=1;
          }
        goto new_key_selected;
        }
      return 0 /*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;
    case WM_LBUTTONDOWN:
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      item=getDiskItemFromPoint(root,&rc,
        GetWindowByte(hWnd,GWL_USERDATA+1),MAKEPOINTS(lParam));
      if(item) {
        XLISTITEM *item2=getSelectedDiskItem(root);
        if(item2 != item) {
          if(item2)
            item2->state=0;
          item->state=1;
          InvalidateRect(hWnd,NULL,TRUE);
          }
        if(hWnd->parent && !hWnd->parent->internalState) {
          NMHDR nmh;
          nmh.code=NM_LDOWN;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
          }
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_LBUTTONDBLCLK:
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      item=getDiskItemFromPoint(root,&rc,
        GetWindowByte(hWnd,GWL_USERDATA+1),MAKEPOINTS(lParam));
      if(item) {
dblclk:        
        switch(item->type) {
          case 0xff:
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000)      // se shift, nuova finestra :)
            {
              char buf[2]={item->disk,0};
              ShellExecute(NULL,"explore",NULL,buf,NULL,SW_SHOWNORMAL);
              }
            else {
              dfn->disk=item->disk;
              SetWindowByte(hWnd,GWL_USERDATA+2,0); 
						  InvalidateRect(hWnd,NULL,TRUE);
              }
            break;
          default:
            if(item->type & ATTR_DIRECTORY) {
              if(GetAsyncKeyState(VK_SHIFT) & 0x8000) {      // se shift, nuova finestra :) 
                char buf[2]={item->disk,0};
                *dfn->path=0;
                ShellExecute(NULL,"explore",NULL,buf,item->filename,SW_SHOWNORMAL);
                }
              else {
                strcpy(dfn->path,item->filename);
                SetWindowByte(hWnd,GWL_USERDATA+2,0); 
                InvalidateRect(hWnd,NULL,TRUE);
                }
              }
            else {
              char buf[3]={item->disk,':',0};
              ShellExecute(NULL,"open",item->filename,NULL,buf[0] ? buf : NULL,SW_SHOWNORMAL);
              }
            break;
          }
        }
      return 0/*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;

    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case 32:
            dfn->disk=0; *dfn->path=0;
            goto aggiorna;
            break;
          case 32+1:
            dfn->disk='A'; *dfn->path=0;
            goto aggiorna;
            break;
          case 32+2:
            dfn->disk='B'; *dfn->path=0;
            goto aggiorna;
            break;
          case 32+3:
            dfn->disk='C'; *dfn->path=0;
            goto aggiorna;
            break;
#if defined(USA_USB_HOST_MSD)
          case 32+4:
            dfn->disk='E'; *dfn->path=0;
            goto aggiorna;
            break;
#endif
#ifdef USA_RAM_DISK 
          case 32+5:
            dfn->disk='R'; *dfn->path=0;
            goto aggiorna;
            break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case 32+6:
            dfn->disk='F'; *dfn->path=0;
            goto aggiorna;
            break;
#endif

          case 16+1:
	          SetWindowByte(hWnd,GWL_USERDATA+1,1);		// icone piccole
            goto aggiorna2;
            break;
          case 16+2:
	          SetWindowByte(hWnd,GWL_USERDATA+1,0);		// icone grandi
            goto aggiorna2;
            break;
          case 16+3:
	          SetWindowByte(hWnd,GWL_USERDATA+1,2);		// dettagli
            goto aggiorna2;
            break;
          case 16+4:
	          // aggiorna.. :) sempre (v. anche F5
aggiorna:
	          SetWindowByte(hWnd,GWL_USERDATA+2,0);		// aggiorna contenuto
aggiorna2:
            InvalidateRect(hWnd,NULL,TRUE);
            break;
            
          case 2:
            {
            MEDIA_INFORMATION *mi=getMediaInfo(dfn->disk);
            if(mi && mi->errorCode == MEDIA_NO_ERROR) {
              if(dfn->disk && root) 
                root=root->next;    // il primo è etichetta volume! (se disco)
              if(item=getSelectedDiskItem(root)) {    // il menu dovrebbe esser disattivato ovviamente :)
                char buf[32];
                buf[0]=dfn->disk;
                buf[1]=':';
                strcpy(&buf[2],item->filename);
                if(item->type & ATTR_DIRECTORY) {
                  if(CopyFileEx(buf,"R:",NULL,NULL,NULL,COPY_FILE_AND_SUBDIR) <= 0) {
                    MessageBeep(MB_ICONERROR);
                    }
                  }
                else {
                  if(CopyFile(buf,"R:",0) <= 0) {
                    MessageBeep(MB_ICONERROR);
                    }
                  }
                SetWindowByte(hWnd,GWL_USERDATA+2,0); 
                // refresh sono generati da handle_filechanges!
                }
              }
            }
            break;
          case 3:
            break;
          case 4:
            {
            MEDIA_INFORMATION *mi=getMediaInfo(dfn->disk);
            if(mi)
              FSformat(mi,0,-1,0,timeGetTime(),NULL,0);
              SetWindowByte(hWnd,GWL_USERDATA+2,0); 
              InvalidateRect(hWnd,NULL,TRUE);
            }
            break;
          case 7:
            {
            MEDIA_INFORMATION *mi=getMediaInfo(dfn->disk);
            if(mi && mi->errorCode == MEDIA_NO_ERROR) {
              if(SuperFileMkDir(dfn->disk,"Folder1")) {
                MessageBeep(MB_ICONERROR);
                }
              SetWindowByte(hWnd,GWL_USERDATA+2,0); 
              }
            }
            break;
          case 6:
//	          SendMessage(hWnd,WM_CLOSE,0,0); SC_CLOSE??
	          PostMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,0);
            break;
            
          default:
            return DefWindowProc(hWnd,message,wParam,lParam);
            break;
          }
        }
      return 1;
      break;
    case WM_NOTIFY:
      switch(lParam) {
        case CDN_TYPECHANGE:
          {
          SetWindowByte(hWnd,GWL_USERDATA+1,(GetWindowByte(hWnd,GWL_USERDATA+1)+1) % 3);
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        }
      break;
   	case WM_INITMENU:
   	case WM_INITMENUPOPUP:    // in windows non lo mettevo, ma parrebbe utile cmq
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      item=getSelectedDiskItem(root);
      EnableMenuItem(GetMenu(hWnd)->menuItems[0].menu,2,MF_BYCOMMAND | (item ? MF_ENABLED : MF_DISABLED));
      EnableMenuItem(GetMenu(hWnd)->menuItems[0].menu,4,MF_BYCOMMAND | (item ? MF_ENABLED : MF_DISABLED));
      EnableMenuItem(GetMenu(hWnd)->menuItems[0].menu,7,MF_BYCOMMAND | (item ? MF_ENABLED : MF_DISABLED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,32+1,MF_BYCOMMAND | (dfn->disk=='A' ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,32+2,MF_BYCOMMAND | (dfn->disk=='B' ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,32+3,MF_BYCOMMAND | (dfn->disk=='C' ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,32+4,MF_BYCOMMAND | (dfn->disk=='D' ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,32+5,MF_BYCOMMAND | (dfn->disk=='E' ? MF_CHECKED : MF_UNCHECKED));
      CheckMenuItem(GetMenu(hWnd)->menuItems[0].menu,32+6,MF_BYCOMMAND | (dfn->disk=='F' ? MF_CHECKED : MF_UNCHECKED));
      break;

    case WM_FILECHANGE:
      SetWindowByte(hWnd,GWL_USERDATA+2,0);
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    case WM_HSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
    case WM_VSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


#define X_SPACING_CONTROLPANEL 50
#define Y_SPACING_CONTROLPANEL 40
void subControlWC(HDC hDC,UGRAPH_COORD_T *x,UGRAPH_COORD_T *y,RECT *rc,ICON icon,
  const char *text,const char *text2,BYTE w1) {

	if(!text2) {
    DrawIcon(hDC,*x+(X_SPACING_CONTROLPANEL+2-16)/2,*y+3,icon);		// METTERE Icon s !
    int x2;
    x2=(X_SPACING_CONTROLPANEL-6*(int)strlen(text))/2; 
    if(x2<0)
      x2=0; 
    TextOut(hDC,*x+x2+2,*y+6+16+2,text,strlen(text));
    // usare DrawText con rc?
  //        DrawText(hDC,text,-1,rc,DT_VTOP | DT_CENTER);
    *x+=X_SPACING_CONTROLPANEL;
    if((*x+(X_SPACING_CONTROLPANEL/2))>=rc->right) {
      *x=0;
      *y+=Y_SPACING_CONTROLPANEL;
      }
    }
  else {    // tipo lista
  	TextOut(hDC,*x,*y,text,strlen(text));
    TextOut(hDC,*x+6*w1,*y,text2,strlen(text2));
    // usare DrawText con rc?
  //        DrawText(hDC,text,-1,rc,DT_VTOP | DT_CENTER);
    *y+=10;
    }
	}
static signed char getWCItemFromPoint(RECT *rc,POINTS pt,BYTE which,BYTE count) {
  BYTE i=0;
  RECT rc2;
  
  switch(which) {
    default:
      i=pt.y / (8+1+1);
      return i<count ? i : -1;
      break;
    case 0:
    {
      POINT ptl={pt.x,pt.y};
      rc2.left=rc->left;
      rc2.top=rc->top;
      while(i<count) {
        rc2.right=rc2.left+X_SPACING_CONTROLPANEL-1;
        rc2.bottom=rc2.top+Y_SPACING_CONTROLPANEL-1;
        if(wndPtInRect(&rc2,ptl))
          return i;
        rc2.left=rc2.left+X_SPACING_CONTROLPANEL;
        if(rc2.left+(Y_SPACING_CONTROLPANEL/2) >= rc->right) {
          rc2.left=rc->left;
          rc2.top+=Y_SPACING_CONTROLPANEL-1;
          }
        i++;
        }
      }
      break;
    }
  return -1;
  }

const char *PRESENTE="Presente",*NON_PRESENTE="Non presente";
const char *CONNESSA="Connessa",*NON_CONNESSA="Non connessa";
const char *OK_STRING="OK";
extern BYTE _bpp;
extern const char _PC_PIC_CPU_C[];
LRESULT controlFontCB(const LOGFONT *lplf,const TEXTMETRIC *lptm,DWORD dwType,LPARAM lpData) {
  HDC hDC=(HDC)lpData;
  HWND hWnd=hDC->hWnd;
  const char string1[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
          string2[]="ABCDEfghij01234";
  int n=((int)GetWindowByte(hWnd,GWL_USERDATA+1))*4;    // così posso arrivare a 1000~
  int n2=GetScrollPos(hWnd,SB_VERT)*100;

  if((lplf->lfPitchAndFamily & 0xf0)==FF_DONTCARE && lplf->lfHeight==8) {   // voglio vedere entrambi i "font system"... truschino
    ((LOGFONT*)lplf)->lfHeight=6;
    controlFontCB(lplf,lptm,dwType,lpData);
    n+=6+2;
    ((LOGFONT*)lplf)->lfHeight=8;
    }
  if(hDC->advanced) {   // v.sotto
    }
  if(dwType & TRUETYPE_FONTTYPE) {    // faccio così per far coesistere i vari tipi... ev. sistemare
    ((LOGFONT*)lplf)->lfHeight=20; ((LOGFONT*)lplf)->lfWidth=20;
    SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
          FW_NORMAL,0,0,0,lplf->lfCharSet,
          lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
    TextOut(hDC,1,n-n2,string1,strlen(string1));
    n+=lplf->lfHeight;
    ((LOGFONT*)lplf)->lfHeight=40; ((LOGFONT*)lplf)->lfWidth=40;
    SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
          FW_NORMAL,0,0,0,lplf->lfCharSet,
          lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
    TextOut(hDC,1,n-n2,string2,strlen(string2));
    n+=lplf->lfHeight;
    }
  else {
    SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
            FW_NORMAL,0,0,0,lplf->lfCharSet,
            lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
    if(lplf->lfHeight<=13) {
      TextOut(hDC,1,n-n2,string1,strlen(string1));
      n+=(lplf->lfPitchAndFamily & 0xf0) != FF_DONTCARE ? (lplf->lfHeight*7)/4 : lplf->lfHeight;
      n+=1;
      if(lplf->lfHeight >= 8) {    // solo >=8 ossia non system piccolo
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
            FW_BOLD,0,0,0,lplf->lfCharSet,
            lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
        TextOut(hDC,1,n-n2,string2,strlen(string2));
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
            FW_NORMAL,TRUE,0,0,lplf->lfCharSet,
            lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
        TextOut(hDC,(lptm->tmMaxCharWidth)*sizeof(string2)+2,n-n2,string2,strlen(string2));
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
            FW_NORMAL,FALSE,TRUE,0,lplf->lfCharSet,
            lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
        TextOut(hDC,(lptm->tmMaxCharWidth)*sizeof(string2)*2+2,n-n2,string2,strlen(string2));
        n+=(lplf->lfPitchAndFamily & 0xf0) != FF_DONTCARE ? (lplf->lfHeight*7)/4 : lplf->lfHeight;
        n+=6;
        }
      }
    else {
      TextOut(hDC,1,n-n2,string2,strlen(string2));
      n+=(lplf->lfPitchAndFamily & 0xf0) != FF_DONTCARE ? (lplf->lfHeight*4)/2 : lplf->lfHeight;
      n+=4;
      if((lplf->lfPitchAndFamily & 0xf0)==FF_DONTCARE && lplf->lfHeight==8)
        n+=4;   // boh rompe il cazzo senza motivo
      }
    }
  SetWindowByte(hWnd,GWL_USERDATA+1,n/4);
  }
LRESULT DefWindowProcControlPanel(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int i; 
      UGRAPH_COORD_T x,y;
      char buf[64];      
      HDC hDC=BeginPaint(hWnd,&ps);
      RECT rc;
      GetClientRect(hWnd,&rc);

			x=1; y=1;

      switch(GetWindowByte(hWnd,GWL_USERDATA)) {
        case 0:
          subControlWC(hDC,&x,&y,&rc,diskIcon, "Dischi",NULL,0);		// METTERE Icon s !
          subControlWC(hDC,&x,&y,&rc,printerIcon, "Stampanti",NULL,0);
          subControlWC(hDC,&x,&y,&rc,networkIcon, "Rete",NULL,0);
          subControlWC(hDC,&x,&y,&rc,keyboardIcon, "Tastiera",NULL,0);
          subControlWC(hDC,&x,&y,&rc,mouseIcon, "Mouse",NULL,0);
          subControlWC(hDC,&x,&y,&rc,videoIcon, "Schermo",NULL,0);
          subControlWC(hDC,&x,&y,&rc,videoIcon, "Font",NULL,0);
          subControlWC(hDC,&x,&y,&rc,audioIcon, "Audio",NULL,0);
          subControlWC(hDC,&x,&y,&rc,deviceIcon, "Sistema",NULL,0);
          subControlWC(hDC,&x,&y,&rc,deviceIcon, "Power",NULL,0);
          {
            TextOut(hDC,rc.left,rc.bottom-16,BREAKTHROUGH_COPYRIGHT_STRING,strlen(BREAKTHROUGH_COPYRIGHT_STRING));
            TextOut(hDC,rc.left,rc.bottom-8,_PC_PIC_CPU_C,strlen(_PC_PIC_CPU_C));
          }
          SetWindowByte(hWnd,GWL_USERDATA+1,10);
          break;
        case 1:
          {uint32_t sn;
          *buf=0;
          if(SDMediaInfo.errorCode == MEDIA_NO_ERROR) {
            SuperFileGetVolume('A',buf,&sn);
            subControlWC(hDC,&x,&y,&rc,NULL, "A:",buf,8);
            }
          *buf=0;
          if(FDCMediaInfo.errorCode == MEDIA_NO_ERROR) {
            SuperFileGetVolume('B',buf,&sn);
            subControlWC(hDC,&x,&y,&rc,NULL, "B:",buf,8);
            }
          *buf=0;
          if(HDMediaInfo.errorCode == MEDIA_NO_ERROR) {
            SuperFileGetVolume('C',buf,&sn);
            subControlWC(hDC,&x,&y,&rc,NULL, "C:",buf,8);
            }
          *buf=0;
          if(0 /*HD2 MediaInfo.errorCode == MEDIA_NO_ERROR*/) {
            subControlWC(hDC,&x,&y,&rc,NULL, "D:",buf,8);
            }
#if defined(USA_USB_HOST_MSD)
          *buf=0;
          if(USBMediaInfo.errorCode == MEDIA_NO_ERROR) {
            uint32_t sn;
            i=SuperFileGetVolume('E', buf, &sn);
            subControlWC(hDC,&x,&y,&rc,NULL, "E:",buf,8);
            }
#endif
#ifdef USA_RAM_DISK 
          *buf=0;
          if(RAMMediaInfo.errorCode == MEDIA_NO_ERROR/*RAMdiscArea*/) {
            SuperFileGetVolume('R',buf,&sn);
            subControlWC(hDC,&x,&y,&rc,NULL, "R:",buf,8);
            }
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          *buf=0;
          sprintf(buf,"<%s >",networkFile.serverpath);   // lascio spazio se vuoto
          subControlWC(hDC,&x,&y,&rc,NULL, "F:",buf,8);   // fare...
#endif
          SetWindowByte(hWnd,GWL_USERDATA+1,6);//PARAMETRIZZARE!
          }
          break;
        case 2:
          sprintf(buf,"%s %s",GetStatus(SOUTH_BRIDGE,NULL) & 0x00001000 ? CONNESSA : NON_CONNESSA,
            GetStatus(SOUTH_BRIDGE,NULL) & 0x00000B00 ? "(errore)" : STR_EMPTY);    
          subControlWC(hDC,&x,&y,&rc,NULL, "LPT1:",buf,8);
          GetStatus(SOUTH_BRIDGE,buf);
          subControlWC(hDC,&x,&y,&rc,NULL, "LPT2:",
            buf[4] ? CONNESSA : NON_CONNESSA,8);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 3:
#ifdef USA_ETHERNET
          sprintf(buf,"%u.%u.%u.%u (%s)",
//            ip.v[0],ip.v[1],ip.v[2],ip.v[3], n ? 0 : 1);
/*finire*/            BiosArea.MyIPAddr.v[0],BiosArea.MyIPAddr.v[1],BiosArea.MyIPAddr.v[2],BiosArea.MyIPAddr.v[3],
                  MACIsLinked() ? CONNESSA : NON_CONNESSA);
          subControlWC(hDC,&x,&y,&rc,NULL, "Ethernet",buf,10);
#endif
#ifdef USA_WIFI
          {uint8_t n;
          IP_ADDR ip;
          m2m_periph_gpio_get_val(M2M_PERIPH_GPIO18,&n); // non va...
          ip.Val=myIp.ip /*BiosArea.MyIPAddr2*/;
          sprintf(buf,"%u.%u.%u.%u (%s %u)",
            ip.v[0],ip.v[1],ip.v[2],ip.v[3], !n ? CONNESSA : NON_CONNESSA,myRSSI);
          subControlWC(hDC,&x,&y,&rc,NULL, "WiFi",buf,10);
          }
#endif
#if !defined(USA_ETHERNET) && !defined(USA_WIFI)
          subControlWC(hDC,&x,&y,&rc,NULL, "Rete","Non presente",10);
#endif
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 4:
          subControlWC(hDC,&x,&y,&rc,NULL, "Tastiera PS/2",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b00000101 ? PRESENTE : NON_PRESENTE,15);
          subControlWC(hDC,&x,&y,&rc,NULL, "Tastiera USB",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b01000000 ? PRESENTE : NON_PRESENTE,15);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 5:
          subControlWC(hDC,&x,&y,&rc,NULL, "Mouse PS/2",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b00001010 ? PRESENTE : NON_PRESENTE,12);
          subControlWC(hDC,&x,&y,&rc,NULL, "Mouse USB",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b10000000 ? PRESENTE : NON_PRESENTE,12);
          if(eXtra & 1)
            subControlWC(hDC,&x,&y,&rc,NULL, "(mouse emulato con shift-frecce)",
          	 "",12);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 6:
          sprintf(buf,"%u; %ux%u,%u bpp", BiosArea.videoMode,Screen.cx,Screen.cy,_bpp);
          subControlWC(hDC,&x,&y,&rc,NULL, "Video mode",buf,12);
          sprintf(buf,"%s; %s",(char*)GET_WINDOW_OFFSET(desktopWindow,sizeof(POINTS)*16),
            GetWindowByte(hWnd,sizeof(POINTS)*16+16+16+4+4) & 1 ? "tiled" : "");
          subControlWC(hDC,&x,&y,&rc,NULL, "Sfondo",      //wallpaper
            buf,12);
          sprintf(buf,"%s (%u)",(char*)GET_WINDOW_OFFSET(desktopWindow,sizeof(POINTS)*16)+16,
            screenSaverTime);
          subControlWC(hDC,&x,&y,&rc,NULL, "Screensaver",
            buf,12);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 7:
          SetWindowByte(hWnd,GWL_USERDATA+1,0);
          EnumFonts(hDC,NULL,controlFontCB,(DWORD)hDC);
//          SetGraphicsMode(hDC,GM_ADVANCED);
//          EnumFontFamilies(hDC,NULL,controlFontCB,(DWORD)hDC);
          // passare param tipo, con ministruct
          // usare   hDC->advanced 
          
          break;
        case 8:
          *buf=0;
          subControlWC(hDC,&x,&y,&rc,NULL, "Scheda audio",PRESENTE /*buf*/,14);
          subControlWC(hDC,&x,&y,&rc,NULL, "MIDI",PRESENTE /*buf*/,14);
          sprintf(buf,"%02X %u %u %u %u",ReadJoystick(0),ReadJoystick(-1),ReadJoystick(-2),ReadJoystick(-3),ReadJoystick(-4));
          subControlWC(hDC,&x,&y,&rc,NULL, "Joystick",buf,14);
          SetWindowByte(hWnd,GWL_USERDATA+1,3);
          break;
        case 9:
          {
          
          *buf=0;
          
          subControlWC(hDC,&x,&y,NULL,NULL,"CPU","PIC32MZ",20);
          sprintf(buf,"%uMHz",GetSpeed());
          subControlWC(hDC,&x,&y,NULL,NULL,"Clock",buf,20);
          sprintf(buf,"%u/%uKB",getTotRAM()/1024,getTotExtRAM()/1024);
          subControlWC(hDC,&x,&y,NULL,NULL,"Memory base/ext",buf,20);
          subControlWC(hDC,&x,&y,NULL,NULL,"BIOS",_PC_PIC_CPU_C,20);
          {
          BYTE d,m,H,M,S;
          uint16_t yy;
          GetRTCC(&d,&m,&yy,&H,&M,&S);
          sprintf(buf,"%02u:%02u:%02u %02u/%02u/%04u",H,M,S,d,m,yy);
          subControlWC(hDC,&x,&y,NULL,NULL,"Real time clock",buf,20);
          }
          sprintf(buf,"Stato: %04X",(GetStatus(SOUTH_BRIDGE,NULL) >> 16) & 0xff);
          subControlWC(hDC,&x,&y,NULL,NULL,"Porta seriale",buf,20);
#ifdef USA_232
          subControlWC(hDC,&x,&y,NULL,NULL,"Porta seriale 2",OK_STRING,20);  // 
#endif
          *buf=0;
          subControlWC(hDC,&x,&y,NULL,NULL,"Contr. floppy drive",buf,20);
          subControlWC(hDC,&x,&y,NULL,NULL,"Controller IDE",OK_STRING,20);
          subControlWC(hDC,&x,&y,NULL,NULL,"Controller SDcard",OK_STRING,20);
          subControlWC(hDC,&x,&y,NULL,NULL,"Controller IRQ",OK_STRING,20);
          subControlWC(hDC,&x,&y,NULL,NULL,"Porta I2C/SPI/SMBus",OK_STRING,20);
          sprintf(buf,"%d°C",ReadTemperature()/16);
          subControlWC(hDC,&x,&y,NULL,NULL,"Temperature sensor",buf,20);
          subControlWC(hDC,&x,&y,NULL,NULL,"Power supply","5.0V / 3.3V",20);    // :) v. anche ReadPower() sotto!
          SetWindowByte(hWnd,GWL_USERDATA+1,13);
          }
          break;
        case 10:
          *buf=0;
          sprintf(buf,"%s", !ReadPowerType() ? "Rete" : "Batteria");   // fare :)
          subControlWC(hDC,&x,&y,&rc,NULL, "Power: ",buf,9);
          sprintf(buf,"%u%%",ReadPower());
          subControlWC(hDC,&x,&y,&rc,NULL, "Livello: ",buf,9);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        }


fine:      
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,hDC->brush.color);
      
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      // forzare font piccolo??
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      SetWindowByte(hWnd,GWL_USERDATA+0,(BYTE)(int)cs->lpCreateParams);    // posso scegliere quale sottogruppo aprire
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      cs->lpCreateParams;   // impostare direttamente da qua...
      }
      return 0;
      break;
    case WM_DESTROY:
//      m_WndControlPanel=NULL;
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_LBUTTONDOWN:
      {RECT rc;
      int8_t n;
      GetClientRect(hWnd,&rc);
      if(GetWindowByte(hWnd,GWL_USERDATA+0) == 5)
        goto mouseclick;
      n=getWCItemFromPoint(&rc,MAKEPOINTS(lParam),GetWindowByte(hWnd,GWL_USERDATA+0),
        GetWindowByte(hWnd,GWL_USERDATA+1));
      if(n>=0) {
        if(hWnd->parent && !hWnd->parent->internalState) {
          NMHDR nmh;
          nmh.code=NM_LDOWN;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
          }
//inutile se non facciamo selezione cmq...        InvalidateRect(hWnd,NULL,TRUE);
        }
      else
        InvalidateRect(hWnd,NULL,TRUE);     // aggiorno videata!
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
    case WM_LBUTTONDBLCLK:
      {RECT rc;
      int8_t n;
      GetClientRect(hWnd,&rc);
help:      
      n=getWCItemFromPoint(&rc,MAKEPOINTS(lParam),GetWindowByte(hWnd,GWL_USERDATA+0),
        GetWindowByte(hWnd,GWL_USERDATA+1));
      switch(GetWindowByte(hWnd,GWL_USERDATA+0)) {
        case 0:
//          SetWindowByte(hWnd,GWL_USERDATA+0,n+1); 
          PostMessage(hWnd,WM_COMMAND,MAKELONG(n+1,0),0);
//          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case 1:     // dischi
          switch(n) {
            HDC hDC;
            DC myDC;
            case 0:
              if(SDMediaInfo.errorCode == MEDIA_NO_ERROR) {    // provo a scrivere!
                SUPERFILE f;
                char buf[16];
                f.drive='A';
                GetTempFileName(NULL,"CTR",0,buf);
                if(SuperFileOpen(&f,buf,OPEN_WRITE,TYPE_TEXT | SHARE_NONE)) {
                  SuperFileWrite(&f,_PC_PIC_CPU_C,strlen(_PC_PIC_CPU_C));
                  SuperFileClose(&f);
                  }
created:
                hDC=GetDC(hWnd,&myDC);
                //fillRect(
                TextOut(hDC,rc.left,rc.bottom-16,"File creato",11);
                ReleaseDC(hWnd,hDC);
                }
              break;
            case 1:
              break;
            case 2:
              break;
            case 3:   // IL PROBLEMA è CHE NON SO A CHE DEVICE CORRISPONDE LA RIGA... dovrei salvarlo ma ok
              if(/*RAMMediaInfo.errorCode == MEDIA_NO_ERROR*/RAMdiscArea) {    // provo a scrivere!
                SUPERFILE f;
                char buf[16];
                f.drive='R';
                GetTempFileName(STR_EMPTY /*per non avere path!*/,"CTR",0,buf); 
                if(SuperFileOpen(&f,buf,OPEN_WRITE,TYPE_TEXT | SHARE_NONE)) {
                  SuperFileWrite(&f,_PC_PIC_CPU_C,strlen(_PC_PIC_CPU_C));
                  SuperFileClose(&f);
                  }
                goto created;
                }
              break;
            case 4:
              if(/*RAMMediaInfo.errorCode == MEDIA_NO_ERROR*/RAMdiscArea) {    // provo a scrivere!
                SUPERFILE f;
                char buf[16];
                f.drive='R';
                GetTempFileName(STR_EMPTY /*per non avere path!*/,"CTR",0,buf); 
                if(SuperFileOpen(&f,buf,OPEN_WRITE,TYPE_TEXT | SHARE_NONE)) {
                  SuperFileWrite(&f,_PC_PIC_CPU_C,strlen(_PC_PIC_CPU_C));
                  SuperFileClose(&f);
                  }
                }
              break;
            case 5:
              break;
            case 6:
              break;
            default:
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }
          break;
        case 2:     // stampanti
          switch(n) {
            HDC hDC;
            DC myDC;
            case 0:
              printScreen();    // beh tanto per :D
printed:              
              hDC=GetDC(hWnd,&myDC);
              //fillRect(
              TextOut(hDC,rc.left,rc.bottom-16,"Pagina inviata alla stampante",29);
              ReleaseDC(hWnd,hDC);
              break;
            case 1:
              {SUPERFILE f; f.drive=DEVICE_LPT2;
              SuperFileWrite(&f,"Prova\x0c",6);
              }
              goto printed;
              break;
            default:
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }
          break;
        case 3:     // rete
          switch(n) {
            case 0:
#ifdef USA_ETHERNET
/*              m2m_ping_req(dstIP.Val, ttl, ping_cb);
ICMPSendPing()
 *               while(!*(unsigned long*)internetBuffer && tOut<3000) {
                m2m_wifi_handle_events(NULL);
                tOut++;
                __delay_ms(1);
                }*/
#endif
              break;
            case 1:
#ifdef USA_WIFI
              {
              IP_ADDR dstIP;
              WORD tOut=0,ttl=64;
              dstIP.v[0]=192; dstIP.v[0]=168; dstIP.v[0]=1; dstIP.v[0]=2;
extern void ping_cb(uint32_t u32IPAddr, uint32_t u32RTT, uint8_t u8ErrorCode);
              m2m_ping_req(dstIP.Val, ttl, ping_cb);
              while(/*!*(unsigned long*)internetBuffer && */ tOut<3000) {
                m2m_wifi_handle_events(NULL);
                tOut++;
                __delay_ms(1);
                }
              }
#endif
              break;
            default:
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }
          break;
        case 4:     // tastiera
          {char buf[16];
          HDC hDC;
          DC myDC;
          hDC=GetDC(hWnd,&myDC);
          extern struct KEYPRESS keypress;
          sprintf(buf,"tasto: %u, %02x %02x",keypress.key,keypress.modifier,keypress.state);
          //fillRect(
          TextOut(hDC,rc.left,rc.bottom-16,buf,strlen(buf));
          ReleaseDC(hWnd,hDC);
          }
          break;
        case 5:     // mouse
          goto mouseclick;
          break;
        case 6:     // video
          if(!(GetAsyncKeyState(VK_SHIFT) & 0x8000))
            drawRGBCube(hWnd->clientArea.left+32,hWnd->clientArea.top+48,64);
          else
            rayTrace(hWnd->clientArea.left+32,hWnd->clientArea.top+32,hWnd->clientArea.left+32+100,hWnd->clientArea.top+32+80);
          break;
        case 7:     // font

          break;
        case 8:     // audio
          SetAudioWave(0,1,(rand() % 20000) +20,0,8,(rand() % 90) +10,0,0);
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case 9:     // dispositivi
          {char buf[32];
          HDC hDC;
          DC myDC;
          hDC=GetDC(hWnd,&myDC);
          switch(n) {
            case 0:   // cpu
              break;
            case 1:   // clock
              break;
            case 2:   // memoria
              break;
            case 3:   // bios
              break;
            case 4:   // RTC
#ifdef USA_WIFI
        			m2m_wifi_get_sytem_time();   // intanto :)
#endif
              break;
            case 5:
              {SUPERFILE f; f.drive=DEVICE_COM1;
              SuperFileOpen(&f,NULL,0,TYPE_TEXT | SHARE_NONE);
              SuperFileWrite(&f,"TheQuickBrownFoxJumpsOverTheLazysDogsBack",41);
              SuperFileClose(&f);
              }
              break;
            case 6:   // ctrl floppy
              break;
            case 7:   // ctrl IDE
              break;
            case 8:   // ctrl SD
              break;
            case 9:   // IRQ
              hDC=GetDC(hWnd,&myDC);
              sprintf(buf,"%08X %08X %08X %08X %08X %08X %08X",IFS0,IFS1,IFS2,IFS3,IFS4,IFS5,IFS6);
              //fillRect(
              TextOut(hDC,rc.left,rc.bottom-8,buf,strlen(buf));
              ReleaseDC(hWnd,hDC);
              break;
            case 10:   // I2C SPI 
              break;
            case 11:   // temperature
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            case 12:   // voltage
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            default:
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }
          }
          break;
        case 10:     // power
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        nmh.code=NM_DBLCLK;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
      
    case WM_KEYDOWN:
      if(GetWindowByte(hWnd,GWL_USERDATA+0) != 4) {
        switch(wParam) {
          case VK_SPACE:
            if(GetAsyncKeyState(VK_MENU) & 0x8000 || GetAsyncKeyState(VK_CONTROL) & 0x8000)
              return DefWindowProc(hWnd,message,wParam,lParam);
            else {
              RECT rc;
              GetClientRect(hWnd,&rc);
              BYTE n=getWCItemFromPoint(&rc,MAKEPOINTS(lParam),GetWindowByte(hWnd,GWL_USERDATA+0),
                GetWindowByte(hWnd,GWL_USERDATA+1));

              if(hWnd->parent && !hWnd->parent->internalState) {
                NMHDR nmh;
                nmh.code=NM_KEYDOWN;
                nmh.hwndFrom=hWnd;
                nmh.idFrom=(DWORD)hWnd->menu;
                PostMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
                }
              }
            break;
          case VK_UP:
            if(!GetWindowByte(hWnd,GWL_USERDATA+0)) {
              }
            PostMessage(hWnd,WM_VSCROLL,MAKELONG(SB_PAGEUP,0),(LPARAM)NULL);    // usato per Font
            break;
          case VK_DOWN:
            if(!GetWindowByte(hWnd,GWL_USERDATA+0)) {
              }
            PostMessage(hWnd,WM_VSCROLL,MAKELONG(SB_PAGEDOWN,0),(LPARAM)NULL);    // usato per Font
            break;
          case VK_LEFT:
            if(!GetWindowByte(hWnd,GWL_USERDATA+0)) {
              }
            break;
          case VK_RIGHT:
            {
            RECT rc;
            GetClientRect(hWnd,&rc);
            BYTE n=getWCItemFromPoint(&rc,MAKEPOINTS(lParam),GetWindowByte(hWnd,GWL_USERDATA+0),
              GetWindowByte(hWnd,GWL_USERDATA+1));
            if(!GetWindowByte(hWnd,GWL_USERDATA+0)) {
              }

            }
            break;

          case VK_F1:
          case VK_F13:
            goto help;
            break;
          case VK_F5:
            goto aggiorna;
            break;
           
          case VK_ESCAPE:
          case VK_BACK:
            goto resetta2;
            break;
            
          default:
            return DefWindowProc(hWnd,message,wParam,lParam);
            break;
          }
        return 0;
        }
// else continua
    case WM_KEYUP:
      if(GetWindowByte(hWnd,GWL_USERDATA+0) == 4) {
        char buf[16]; 
        HDC hDC;
        DC myDC;
        RECT rc;
        GetClientRect(hWnd,&rc);
        hDC=GetDC(hWnd,&myDC);
        extern struct KEYPRESS keypress;
        sprintf(buf,"tasto: %u, %02x %02x (%u)",keypress.key /*(wParam >> 16) & 0xff*/,
                keypress.modifier,keypress.state /*(wParam >> 31) */,wParam & 0xffff);
        fillRectangleWindow(hDC,rc.left,rc.bottom-16,rc.right,rc.bottom);
        TextOut(hDC,rc.left,rc.bottom-16,buf,strlen(buf));
        ReleaseDC(hWnd,hDC);
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;

    case WM_CHAR:
      if(GetWindowByte(hWnd,GWL_USERDATA+0) != 4) {
        switch(wParam) {
          case 'a':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(7+1,0),0);
            break;
          case 'd':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(0+1,0),0);
            break;
          case 'f':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(6+1,0),0);
            break;
          case 'm':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(4+1,0),0);
            break;
          case 'p':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(9+1,0),0);
            break;
          case 'r':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(2+1,0),0);
            break;
          case 's':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(1+1,0),0);
            break;
          case 'S':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(5+1,0),0);
            break;
          case 't':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(3+1,0),0);
            break;
          case 'i':
            PostMessage(hWnd,WM_COMMAND,MAKELONG(8+1,0),0);
            break;
          default:
            if(isalpha(wParam))
              MessageBeep(MB_ICONASTERISK);
            return DefWindowProc(hWnd,message,wParam,lParam);
            break;
          }
        }
      break;

    case WM_RBUTTONDOWN:    // a volte esce un contextmenu... strano...
    case WM_RBUTTONDBLCLK:
        {
        char buf[20]; 
        HDC hDC;
        DC myDC;
        RECT rc;
        if(GetWindowByte(hWnd,GWL_USERDATA+0) == 5 && !(wParam & MK_CONTROL) /*!(GetAsyncKeyState(VK_CONTROL) & 0x8000)*/) { 
mouseclick:
          GetClientRect(hWnd,&rc);
          hDC=GetDC(hWnd,&myDC);
          extern struct MOUSE mouse;
          sprintf(buf,"mouse: %02X [%s], %u, %u",
                  mouse.buttons,message==WM_MOUSEMOVE ? "move" : 
                  ((message==WM_LBUTTONDBLCLK || message==WM_RBUTTONDBLCLK) ? 
                  "dclick" : "click"),
                  LOWORD(lParam),HIWORD(lParam));
          fillRectangleWindow(hDC,rc.left,rc.bottom-16,rc.right,rc.bottom);
          TextOut(hDC,rc.left,rc.bottom-16,buf,strlen(buf));
          ReleaseDC(hWnd,hDC);
          }
        else {
      // per ora!
resetta:
          SetWindowByte(hWnd,GWL_USERDATA+0,0);   // ritorno alla videata principale
          SetWindowText(hWnd,"Pannello di controllo");
          InvalidateRect(hWnd,NULL,TRUE);
          return DefWindowProc(hWnd,message,wParam,lParam);
          }
        }
      break;
      
    case WM_MOUSEMOVE:
      if(GetWindowByte(hWnd,GWL_USERDATA+0) == 5)
        goto mouseclick;
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case 1:
          case 2:
          case 3:
          case 4:
          case 5:
          case 6:
          case 7:
          case 8:
          case 9:
          case 10:
            SetScrollPos(hWnd,SB_VERT,0,FALSE);
            if(LOWORD(wParam) == 7 || LOWORD(wParam) == 9) {    // font, devices
              RECT rc;
              GetClientRect(hWnd,&rc);
              // EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   // no..
              SetScrollRange(hWnd,SB_VERT,0,rc.bottom<160 ? 5 : 3,FALSE); // andrebbe fatto in proporzione... e sul totale righe font
              ShowScrollBar(hWnd,SB_VERT,TRUE);
              }
            else {
              // EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   // no..
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              ShowScrollBar(hWnd,SB_VERT,FALSE); //
              }
            SetWindowByte(hWnd,GWL_USERDATA+0,LOBYTE(LOWORD(wParam)));
            SetWindowText(hWnd,hWnd->menu->menuItems[0].menu->menuItems[LOBYTE(LOWORD(wParam))-1].text);
            break;
          case 11:
resetta2:
            ShowScrollBar(hWnd,SB_VERT,FALSE); //
            goto resetta;
            break;
          case 16:
// inutile :)            InvalidateRect(hWnd,NULL,TRUE);
            break;
          }  
aggiorna:        
        InvalidateRect(hWnd,NULL,TRUE);
        }
      return 1;
      break;
      
    case WM_VSCROLL:
      {
      int16_t i,imin,imax;
      i=GetScrollPos(hWnd,SB_VERT);
      switch(LOWORD(wParam)) {
        case SB_PAGEUP:
          if(i>0)
            i--;
          break;
        case SB_PAGEDOWN:
          GetScrollRange(hWnd,SB_VERT,&imin,&imax);
          if(i<(imax-1))      // mah direi così
            i++;
          break;
        }// usare anche LINEUP ecc e ritarare
      SetScrollPos(hWnd,SB_VERT,i,TRUE);
      InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

int subPrintTaskManager(HDC hDC,UGRAPH_COORD_T y,HWND hWnd,BYTE threadID,const char *caption) {
  char buffer[64];
  CLASS c; 
  BYTE id,n;
  FONT myFont=GetStockObject(SYSTEM_FONT).font;
  UGRAPH_COORD_T x;
  BYTE xx=hWnd ? getFontWidth(&hWnd->font) : getFontWidth(&myFont);   // bah ok :)
  
  x=2;
  SetTextColor(hDC,WHITE);
  if(hWnd) {
    strncpy(buffer,hWnd->caption,20);
    buffer[20]=0;
    TextOut(hDC,x,y,buffer,strlen(buffer));
    c=hWnd->class;
    if(!c.class)
      c.class=MAKEFOURCC('D','F','L','T');
    GetWindowThreadProcessId(hWnd,&id);
    n=GetThreadPriority(GetThreadFromID(id));
    SetTextColor(hDC,n==THREAD_PTY_HIGH ? LIGHTRED : (n==THREAD_PTY_MIDDLE ? LIGHTGREEN : LIGHTBLUE));
    sprintf(buffer,"%c%c%c%c %02X %u  %u",c.class4[0],c.class4[1],c.class4[2],c.class4[3],
      hWnd->status,hWnd->zOrder,id);
    TextOut(hDC,x+21*xx,y,buffer,strlen(buffer));
    }
  else if(threadID!=THREAD_ID_INVALID) {
    TextOut(hDC,x,y,caption,strlen(caption));
    sprintf(buffer,"%u",threadID);
    n=GetThreadPriority(GetThreadFromID(threadID)); 
    SetTextColor(hDC,n==THREAD_PTY_HIGH ? LIGHTRED : (n==THREAD_PTY_MIDDLE ? LIGHTGREEN : LIGHTBLUE));
    TextOut(hDC,x+32*xx,y,buffer,strlen(buffer));
    }
  sprintf(buffer,"%uKB",0);			// fare! forse con thread...
  TextOut(hDC,x+36*xx,y,buffer,strlen(buffer));
  }
LRESULT DefWindowProcTaskManager(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int i; 
      UGRAPH_COORD_T x,y;
      BYTE yy;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      HDC hDC=BeginPaint(hWnd,&ps);
      yy=getFontHeight(&hWnd->font)+1; 

      i=4 /*fisse*/; y=2;
      
      subPrintTaskManager(hDC,y,desktopWindow,0,NULL);
      y+=yy;
      subPrintTaskManager(hDC,y,taskbarWindow,0,NULL);
      y+=yy;
      
      HWND myWnd=rootWindows;
      while(myWnd) {
        if(y<((rc  /*ps.rcPaint*/.bottom)-getFontHeight(&hWnd->font)/*statusbar*/)) {
          subPrintTaskManager(hDC,y,myWnd,0,NULL);
          y+=yy;
          }
        i++;
        myWnd=myWnd->next;
        }
      
      subPrintTaskManager(hDC,y,NULL,rootThreads->next->ID,"Window manager");
      y+=yy;
      subPrintTaskManager(hDC,y,NULL,rootThreads->ID,"Idle");
      y+=yy;
      
      y=(rc.bottom-getFontHeight(&hWnd->font)/*statusbar*/)/getFontHeight(&hWnd->font);
      // forse meglio DOPO clientPaint? o non importa? verificare 2024
      SetScrollPos(hWnd,SB_VERT,0,FALSE);
      if(i>y) {
//        EnableScrollBar(hWnd,SB_VERT,ESB_ENABLE_BOTH);   //
        SetScrollRange(hWnd,SB_VERT,0,i-y,FALSE);
        ShowScrollBar(hWnd,SB_VERT,TRUE); //
        }
      else {
//        EnableScrollBar(hWnd,SB_VERT,ESB_DISABLE_BOTH);   //
        SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
        ShowScrollBar(hWnd,SB_VERT,FALSE); //
        }
      i=42; // v.sopra, circa
      x=(rc.right-getFontHeight(&hWnd->font)/*statusbar*/)/getFontWidth(&hWnd->font);
      // forse meglio DOPO clientPaint? o non importa? verificare 2024
      SetScrollPos(hWnd,SB_HORZ,0,FALSE);
      if(i>x) {
//        EnableScrollBar(hWnd,SB_HORZ,ESB_ENABLE_BOTH);   //
        SetScrollRange(hWnd,SB_HORZ,0,i-x,FALSE);
        ShowScrollBar(hWnd,SB_HORZ,TRUE); //
        }
      else {
//        EnableScrollBar(hWnd,SB_HORZ,ESB_DISABLE_BOTH);   //
        SetScrollRange(hWnd,SB_HORZ,0,0,FALSE);
        ShowScrollBar(hWnd,SB_HORZ,FALSE); //
        }
      
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,hDC->brush.color);
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      // forzare font piccolo??
      int i;
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      HWND myWnd=CreateWindow(MAKECLASS(WC_STATUSBAR),"CPU speed:",WS_VISIBLE | WS_CHILD,
        0,cs->cy-8,cs->cx,cs->cy,    // se thickborder deve andare più in giù e + larga, pare
        hWnd,(HMENU)201,NULL
        );
      SetTimer(hWnd,1,15000,NULL);
      }
      return 0;
      break;
    case WM_DESTROY:
      KillTimer(hWnd,1);
      //taskmanwnd=NULL
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_SIZE:
//      BYTE height=getFontHeight(&hWnd->font);
//      MoveWindow(myWnd,0,HIWORD(lParam)-height-1,LOWORD(lParam),height+1,TRUE);
      {
      WINDOWPOS wpos;
      wpos.hwnd=hWnd;
      wpos.hwndInsertAfter=NULL;
      wpos.x=hWnd->nonClientArea.left;
      wpos.y=hWnd->nonClientArea.top;
      wpos.cx=LOWORD(lParam);
      wpos.cy=HIWORD(lParam);
      wpos.flags=SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOACTIVATE | SWP_NOZORDER | SWP_SHOWWINDOW;
      SendMessage(GetDlgItem(hWnd,201),WM_WINDOWPOSCHANGED,0,(LPARAM)&wpos);
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_KEYDOWN:
      switch(wParam) {
        case VK_ESCAPE:
          PostMessage(hWnd,WM_CLOSE,0,0);
          break;
        case VK_F5:
          PostMessage(hWnd,WM_TIMER,0,0);   // così calcolo CPU e aggiorno tutto :)
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;

    case WM_COMMAND:
      return 1;
      break;
      
    case WM_TIMER:
      {
      char buffer[32];
      
      sprintf(buffer,"RAM: %uKB/%uKB; CPU speed: %uMHz",getTotRAM()/1024,getTotExtRAM()/1024,GetSpeed());
      SetWindowText(GetDlgItem(hWnd,201),buffer);
      }
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
      
    case WM_VSCROLL:
      InvalidateRect(hWnd,NULL,TRUE);
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcCmdShellWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int i;
  char ch;
  BYTE x,y,xs,ys,offsetPrompt=0;
  x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
  char *cmdline=(char*)GET_WINDOW_OFFSET(hWnd,5);
  char *lastCmdline=GET_WINDOW_OFFSET(hWnd,5+32);
  i=strlen(cmdline);
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      HDC hDC=BeginPaint(hWnd,&ps);
      xs=getFontWidth(&hDC->font);
      ys=getFontHeight(&hDC->font);
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);

      TextOut(hDC,rc.left+1,rc.top+1+y*ys,prompt,strlen(prompt)); 
      TextOut(hDC,rc.left+1+xs*strlen(prompt),rc.top+1+y*ys,cmdline,i);
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      { HDC hDC=(HDC)wParam;

        // if hDC->flags &  ?? usare per ottimizzare
        //   DrawWindow(hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right,hWnd->paintArea.bottom,b,BLACK /*b*/);		// colore?

        fillRectangleWindowColorRect(hDC,&hWnd->paintArea,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);
        return 1;
      }
      break;
    case WM_TIMER:
      {
      DC myDC;
      HDC hDC=GetDC(hWnd,&myDC);
      RECT rc;
      GetClientRect(hWnd,&rc);    // fingo cursore :D
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);
//      FillRectangleWindow(hDC,rc.left+1+6*3+x*6,rc.top+1+y*8,rc.left+1+6*3+x*6+6,rc.top+1+y*8);
      xs=getFontWidth(&hDC->font);
      ys=getFontHeight(&hDC->font);
      TextOut(hDC,rc.left+1+xs*strlen(prompt)+x*xs,rc.top+1+y*ys,GetWindowByte(hWnd,0) ? "_" : " ",1);
      SetWindowByte(hWnd,0,!GetWindowByte(hWnd,0));
      ReleaseDC(hWnd,hDC);
      }
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);   //
      SetWindowByte(hWnd,GWL_USERDATA+2,15);   //white
      SetWindowByte(hWnd,GWL_USERDATA+3,0);   //black
      SetWindowLong(hWnd,0,0);   // cursore & stringa!
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      if(cs->lpCreateParams) {
        strcpy(cmdline,(char*)cs->lpCreateParams);    // ed eseguire
        PostMessage(hWnd,WM_CHAR,'\r',0);
        }
      SetTimer(hWnd,1,500,NULL);    // per cursore...
      }
      return 0;
      break;
    case WM_DESTROY:
      KillTimer(hWnd,1);
      //shelldos=NULL
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_SETFOCUS:
      hWnd->focus=1;
      SetTimer(hWnd,1,500,NULL);    // per cursore...
      return 0;
      break;
    case WM_KILLFOCUS:
      hWnd->focus=0;
      KillTimer(hWnd,1);
      return 0;
      break;
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case 1:
            {
//extern MINIBASIC minibasicInstance;
//            minibasic(&minibasicInstance,NULL);
            strcpy(cmdline,"BASIC");    // :)
      			SetWindowByte(hWnd,GWL_USERDATA+0,5);
            InvalidateRect(hWnd,NULL,TRUE);
            }
            break;
          case 2:
            PostMessage(hWnd,WM_CLOSE,0,0);
            break;
          }
        }
      return 1;
      break;
      
    case WM_KEYDOWN:
      switch(wParam) {
        default:// per F10
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        case VK_RIGHT:
        case VK_F1:
          if(i<strlen(lastCmdline)) {
            cmdline[i]=lastCmdline[i];
            cmdline[++i]=0;
      			SetWindowByte(hWnd,GWL_USERDATA+0,i);
            InvalidateRect(hWnd,NULL,TRUE);// FARE SOLO ZONA LINEA ATTUALE!
            }
          break;
        case VK_LEFT:
          goto bksp;
          break;
        case VK_F3:
          while(i<strlen(lastCmdline)) {
            cmdline[i]=lastCmdline[i];
            cmdline[++i]=0;
            }
    			SetWindowByte(hWnd,GWL_USERDATA+0,i);
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case VK_UP:
          strcpy(cmdline,lastCmdline);
    			SetWindowByte(hWnd,GWL_USERDATA+0,strlen(cmdline));
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      break;
    case WM_CHAR:
      {
      
      DC myDC;
      HDC hDC;
      RECT rc;
      
      if(GetAsyncKeyState(VK_MENU) & 0x8000)    // ALT per menu
//        if(!matchAccelerator(hWnd->menu,LOBYTE(wParam)))
        return DefWindowProc(hWnd,message,wParam,lParam);
      if(GetAsyncKeyState(VK_CONTROL) & 0x8000)    // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
//        if(!matchAccelerator(hWnd->menu,LOBYTE(wParam)))
        return DefWindowProc(hWnd,message,wParam,lParam);
      
      GetClientRect(hWnd,&rc);
//      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      
      switch(wParam) {
        case '\x1b':
          cmdline[0]=0;
  				x=0;
					y++;
          break;
        case '\x8':
bksp:
          if(i > 0) {
            cmdline[--i]=0;
						x--;
      			SetWindowByte(hWnd,GWL_USERDATA+0,x);
//  					SendMessage(hWnd,WM_PRINTCHAR,' ',0); non serve, il cursore sbianchetta cmq; e fa danni (?)
//      			SetWindowByte(hWnd,GWL_USERDATA+0,x);
            }
          break;
        case '\t':
          goto putchar;
          break;
        case '\r':
					SendMessage(hWnd,WM_PRINTCHAR,'\n',0);
//          x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);

          KillTimer(hWnd,1);
          hDC=GetDC(hWnd,&myDC);
//      TextOut(hDC,rc.left+1+6*strlen(prompt)+x*6,rc.top+1+y*8," "); FARE per pulire cursore!
#ifdef USA_BREAKTHROUGH
          m_stdout=/*m_stdin=*/m_stderr=DEVICE_WINDOW;
#else
          m_stdout=/*m_stdin=*/m_stderr=DEVICE_CON;
#endif
          strcpy(lastCmdline,cmdline);
          execCmd(cmdline,NULL,cmdline);
          cmdline[0]=0;
          m_stdout=m_stdin=m_stderr=DEVICE_CON;   // direi di togliere e lasciare fisso DEVICE_WINDOW (vedi InitWindows
     			
          SetTimer(hWnd,1,500,NULL);    // per cursore...
					SendMessage(hWnd,WM_PRINTCHAR,'\n',0); 
          
  //il prompt esce una riga più sopra, poi con PAINT è ok... in minibasic è ok -> SEMBRA DOVUTO A POSTMESSAGE nei WM_PRINTCHAR! con send è ok, v.
          
          
          x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
          xs=getFontWidth(&hDC->font);
          ys=getFontHeight(&hDC->font);
          TextOut(hDC,rc.left+1+x*xs,rc.top+1+y*ys,prompt,strlen(prompt));
          SetWindowByte(hWnd,GWL_USERDATA+0,strlen(prompt));
          ReleaseDC(hWnd,hDC);
          break;
        default:
          ch=LOBYTE(wParam);
          if(isprint(ch)) {
            if(i < 31) {
              cmdline[i++]=ch;
              cmdline[i]=0;
							x++;
              lParam=0xffffffff;
              offsetPrompt=strlen(prompt);
              goto putchar;
              }
            else {
              MessageBeep(MB_ICONASTERISK);   // bah togliere per tasti sistema :)
//              return DefWindowProc(hWnd,message,wParam,lParam);
              }
            }
          break;
        }
      SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
      }
      break;
      
    case WM_PRINTCHAR:
      {
      DC myDC;
      HDC hDC;
      RECT rc;
      ch=LOBYTE(wParam);
      
putchar:
      // o FARE InvalidateRect(hWnd,NULL,TRUE) ??
      x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
      GetClientRect(hWnd,&rc);
      hDC=GetDC(hWnd,&myDC);
      if(lParam != 0xffffffff)
        SetTextColor(hDC,LOWORD(lParam));
      else
        SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);
      switch(ch) {
        case '\t':
  				x++;
  				while(!(x & 7))
            x++;
          break;
        case 0x0c:
          fillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]); 
          // o fare semplicemente un InvalidateRect(mInstance->hWnd,NULL,TRUE) ??
          x=y=0;
          break;
        case '\r':
          x=0;
          break;
        case '\n':
          x=0; y++;
          if(y>=rc.bottom/8) {
            y--;
            rc.top+=8;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
          break;
        default:
          if(isprint(ch)) {
            drawCharWindow(hDC,1+(x+offsetPrompt)*6,1+y*8,ch);
            x++;
            if(x>=rc.right/6) {
              SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
              SendMessage(hWnd,WM_PRINTCHAR,'\n',0);
              x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
              }
            }
          break;
        }
      SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
      ReleaseDC(hWnd,hDC);
      }
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

static void textScreenSaver(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,const char *fileScreensaver) {
  TextOut(hDC,x,y,fileScreensaver,12);
  }
static void clockScreenSaver(HDC hDC,UGRAPH_COORD_T x,UGRAPH_COORD_T y,BYTE m) {
  PIC32_DATE date;
  PIC32_TIME time;
  char buf[16];
  
  SetTimeFromNow(now,&date,&time);
  if(!m) {
    sprintf(buf,"%02u:%02u:%02u",time.hour,time.min,time.sec);
    // O USARE DIGIT v. orologio!
    }
  else {
    sprintf(buf,"%02u/%02u/%04u",date.mday,date.mon,date.year);
    // O USARE DIGIT v. orologio!
    }
  TextOut(hDC,x,y,buf,strlen(buf));
  }
static void fishScreenSaver(HDC hDC) {
  LOGBRUSH logbrushBlack;
  logbrushBlack.lbColor=BLACK;
  logbrushBlack.lbHatch=NULL;
  logbrushBlack.lbStyle=BS_SOLID;
	BRUSH brushBlack=CreateBrushIndirect(&logbrushBlack);
	HRGN rgnNew, rgnBlack;
	static int16_t a=0x7fff,b=0x7fff,c=0x7fff,d=0x7fff,e=5;		//Creates these variables and declares them as whole numbers
//	a = 480; //Starting point for Left moving fish
//	b = 280;
//	c = -100; //Starting point for Right moving fish
//	d = 100;
//	e=5;
	static int8_t Counter=0,Move1=0,Move2=0;
//	Move1 = 0;
//	Move2 = 0;
	int8_t MouseRight,MouseDown;
	MouseRight = 0;
	MouseDown = 0;
  static uint8_t Color1=1,Color2=5, nWidth;
//	Color1 = 1; //Starting color for Left moving fish
//	Color2 = 5; //Starting color for Right moving fish
//	Counter = 0;  //reates values for variables
  PEN pen1,pen2,hOldPen;

	SetPolyFillMode(hDC,WINDING);

//Jason Anthony 
//This is my Kimmie Fish Screensaver for Win 10, coded in QB64.
//It's also free on SourceForge.net, just search Fish Screensaver.
// ported by G.Dar 23/11/23 MORTE AGLI UMANI!!!

  if(a==0x7fff)
    a=Screen.cx+20;
  if(b==0x7fff)
    b=Screen.cy-(rand() % 40)-40;
  if(c==0x7fff)
    c=-40;
  if(d==0x7fff)
    d=(rand() % 40)+40;

//	Color1=RGB()
	nWidth=(rand() & 1) +1;
	pen1=CreatePen(PS_SOLID,nWidth,textColors[Color1]);
	nWidth=(rand() & 1) +1;
	pen2=CreatePen(PS_SOLID,nWidth,textColors[Color2]);

	rgnBlack=CreateRectRgn(a,b,a+42,b+22);
	FillRgn(hDC,rgnBlack, brushBlack);

  if(a >= Screen.cx) 
    Move1 = -1; //Defines movement is to the left
  if(Move1 == -1) 
    a -= (rand() & 1) + 1;
  if(a<0) 
    a += Screen.cx;			//Stops Movement at -100, then starts again at 800
  if(a == 1)
    Color1++;		//Creates color change for every trip
  if(Color1 >= 16)
    Color1 = 1;	//This limits color to 15, then restarts at 1
  if(a == 2)
    b = Screen.cy-(rand() % 40)-40;  //Creates different starting height for fish path every trip
  
	/*hOldPen=(PEN)*/SelectObject(hDC,OBJ_PEN,(GDIOBJ)pen1);

  //  _Limit 20 'Limits loop to 20 frames per second
  MoveTo(hDC,a+13,b+0); //Left moving fish sprite - (a,b) above creates starting point - Line 1
  LineTo(hDC,a+18,b+2); // Color1,BF 
  MoveTo(hDC,a+8,b+2);
  LineTo(hDC,a+13,b+4); //, Color1, BF //line 2
  MoveTo(hDC,a+18,b+2);
  LineTo(hDC,a+23,b+4); // Color1, BF
  Counter++;   // This starts a counting sequence
  if(Counter > 5) 
    Counter = 0; //This limits counting to 20, then restarts at 0
  if(Counter < 5 * .7)   {
    MoveTo(hDC,a+33,b+2); //This line is shown for .7 frames of the total 20 frames per second
    LineTo(hDC,a+35,b+4);  // Color1, BF //This line is shown for .7 frames of the total 20 frames per second
    }
  MoveTo(hDC,a+5,b+4);
  LineTo(hDC,a+8,b+6); // Color1, BF //line 3
  MoveTo(hDC,a+23,b+4);
  LineTo(hDC,a+25,b+6);// Color1, BF
  MoveTo(hDC,a+30,b+4);
  LineTo(hDC,a+33,b+6);// Color1, BF
  MoveTo(hDC,a+3,b+6);
  LineTo(hDC,a+5,b+8); //Color1, BF //line 4
  MoveTo(hDC,a+9,b+6);
  LineTo(hDC,a+11,b+8);// Color1, BF
  MoveTo(hDC,a+23,b+6);
  LineTo(hDC,a+30,b+8);// Color1, BF
  MoveTo(hDC,a+0,b+8);
  LineTo(hDC,a+3,b+10);// Color1, BF //line 5
  if(Counter < 5 * .5)  {
    MoveTo(hDC,a+15,b+8);
    LineTo(hDC,a+18,b+10);// Color1, BF
    }
  MoveTo(hDC,a+28,b+8);
  LineTo(hDC,a+30,b+10);// Color1, BF
  MoveTo(hDC,a+0,b+10);
  LineTo(hDC,a+3,b+12);// Color1, BF //line 6
  MoveTo(hDC,a+13,b+10);
  LineTo(hDC,a+15,b+12);// Color1, BF
  if(Counter < 5 * .5)  {
    MoveTo(hDC,a+15,b+10);
    LineTo(hDC,a+18,b+12);// Color1, BF
    }
  MoveTo(hDC,a+28,b+10);
  LineTo(hDC,a+30,b+12);// Color1, BF
  MoveTo(hDC,a+3,b+12);
  LineTo(hDC,a+8,b+14);// Color1, BF //line 7
  MoveTo(hDC,a+23,b+12);
  LineTo(hDC,a+30,b+14);// Color1, BF
  MoveTo(hDC,a+5,b+14);
  LineTo(hDC,a+8,b+16);// Color1, BF //line 8
  MoveTo(hDC,a+23,b+14);
  LineTo(hDC,a+25,b+16);// Color1, BF
  MoveTo(hDC,a+30,b+14);
  LineTo(hDC,a+33,b+16); //Color1, BF
  MoveTo(hDC,a+8,b+16);
  LineTo(hDC,a+13,b+18);// Color1, BF //line 9
  MoveTo(hDC,a+18,b+16);
  LineTo(hDC,a+23,b+18);// Color1, BF
  if(Counter < 5 * .7)  {
    MoveTo(hDC,a+33,b+16);
    LineTo(hDC,a+35,b+18);// Color1, BF
    }
  MoveTo(hDC,a+13,b+18);
  LineTo(hDC,a+18,b+20); //Color1, BF //End of left moving fish sprite - Line 10


	rgnBlack=CreateRectRgn(c,d,c+42,d+22);
	FillRgn(hDC,rgnBlack,brushBlack);
  if(c < 1)
    Move2 = 1; //Defines movement is to the right
  if(Move2 == 1)
    c += (rand() & 1) + 1;
  if(c >= Screen.cx) 
    c -= Screen.cx; //tops Movement at 740, then starts again at -840
  if(c == 2)
    Color2++;     //Creates color change for every trip
  if(Color2 >= 16)
    Color2 = 1;    //This limits color to 15, then restarts at 1
  if(c == 3)
    d = (rand() % 40)+40;
	SelectObject(hDC,OBJ_PEN,(GDIOBJ)pen2);
  MoveTo(hDC,c+22,d+0);
  LineTo(hDC,c+17,d+2);// Color2, BF //Right moving fish sprite - (c,d) above creates starting point - Line 1
  MoveTo(hDC,c+27,d+2);
  LineTo(hDC,c+22,d+4);// Color2, BF //line 2
  MoveTo(hDC,c+17,d+2);
  LineTo(hDC,c+12,d+4);// Color2, BF
  Counter++;
  if(Counter > 5)
    Counter = 0;
  if(Counter < 5 * .7) {
    MoveTo(hDC,c+2,d+2);
    LineTo(hDC,c+0,d+4);// Color2, BF
    }
  MoveTo(hDC,c+30,d+4);
  LineTo(hDC,c+27,d+6);// Color2, BF //line 3
  MoveTo(hDC,c+12,d+4);
  LineTo(hDC,c+10,d+6);// Color2, BF
  MoveTo(hDC,c+5,d+4);
  LineTo(hDC,c+2,d+6);// Color2, BF
  MoveTo(hDC,c+32,d+6);
  LineTo(hDC,c+30,d+8 );// Color2, BF //line 4
  MoveTo(hDC,c+26,d+6);
  LineTo(hDC,c+24,d+8 );// Color2, BF
  MoveTo(hDC,c+12,d+6);
  LineTo(hDC,c+5,d+8 );// Color2, BF
  MoveTo(hDC,c+35,d+8);
  LineTo(hDC,c+32,d+10);// Color2, BF //line 5
  if(Counter < 5 * .5)  {
    MoveTo(hDC,c+20,d+8);
    LineTo(hDC,c+17,d+10);// Color2, BF
    }
  MoveTo(hDC,c+7,d+8);
  LineTo(hDC,c+5,d+10);// Color2, BF
  MoveTo(hDC,c+35,d+10);
  LineTo(hDC,c+32,d+12);// Color2, BF //line 6
  MoveTo(hDC,c+22,d+10);
  LineTo(hDC,c+20,d+12);// Color2, BF
  if(Counter < 5 * .5)  {
    MoveTo(hDC,c+20,d+10);
    LineTo(hDC,c+17,d+12);// Color2, BF
    }
  MoveTo(hDC,c+7,d+10);
  LineTo(hDC,c+5,d+12);// Color2, BF
  MoveTo(hDC,c+32,d+12);
  LineTo(hDC,c+27,d+14);// Color2, BF //line 7
  MoveTo(hDC,c+12,d+12);
  LineTo(hDC,c+5,d+14);// Color2, BF
  MoveTo(hDC,c+30,d+14);
  LineTo(hDC,c+27,d+16);// Color2, BF //line 8
  MoveTo(hDC,c+12,d+14);
  LineTo(hDC,c+10,d+16);// Color2, BF
  MoveTo(hDC,c+5,d+14);
  LineTo(hDC,c+2,d+16);// Color2,BF
  MoveTo(hDC,c+27,d+16);
  LineTo(hDC,c+22,d+18);// Color2, BF //line 9
  MoveTo(hDC,c+17,d+16);
  LineTo(hDC,c+12,d+18);// Color2, BF
  if(Counter < 5 * .7)  {
    MoveTo(hDC,c+2,d+16);
    LineTo(hDC,c+0,d+18); //Color2, BF
    }
  MoveTo(hDC,c+22,d+18);
  LineTo(hDC,c+17,d+20);// Color2, BF //End of right moving fish sprite - Line 10

/*    while (_MouseInput) {//Starts mouse movement loop
      MouseRight = MouseRight + _MouseMovementX;
      MouseDown = MouseDown + _MouseMovementY;
  }*/

  SelectObject(hDC,OBJ_PEN,(GDIOBJ)hOldPen);

  DeleteObject(OBJ_BRUSH,(GDIOBJ)brushBlack);
  DeleteObject(OBJ_PEN,(GDIOBJ)pen1);
  DeleteObject(OBJ_PEN,(GDIOBJ)pen2);
  }

LRESULT DefWindowProcDC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_NCPAINT:    // qua evito, fa tutto erasebackground
//      calculateClientArea(hWnd,&hWnd->clientArea);    // 
      SendMessage(hWnd,WM_NCCALCSIZE,FALSE,(LPARAM)&hWnd->clientArea);
      GetWindowRect(hWnd,&hWnd->paintArea);    // così forzo redraw totale/background
      break;
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
			BYTE i;
      BYTE attrib=GetWindowByte(hWnd,sizeof(POINTS)*16+16+16+4+4);
      GFX_COLOR f=GetWindowWord(hWnd,GWL_USERDATA+0),b=GetWindowWord(hWnd,GWL_USERDATA+2);
      char *fileScreensaver=(char*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16)+16;
      HWND myWnd;
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      hDC->pen=CreatePen(PS_SOLID,1,WHITE);
      hDC->brush=CreateSolidBrush(desktopColor);

			if(inScreenSaver) {
        char buf[16];
        i=rand() % 100; i=max(i,8); i=min(i,80);
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(i,(i*2)/3,0,0,rand() & 1 ? FW_NORMAL : FW_BOLD,
          rand() & 1,rand() & 1,rand() & 1,ANSI_CHARSET,
          OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,VARIABLE_PITCH | (rand() & 1 ? FF_ROMAN : FF_SWISS),NULL));
        img_ofs_y=rand() % rc.bottom; img_ofs_x=rand() % rc.right;
        SetTextColor(hDC,rand());
        SetBkColor(hDC,BLACK /*desktopColor*/);
        if(!stricmp(fileScreensaver,"clock")) {
          screenSaverRefresh=10;
          clockScreenSaver(hDC,img_ofs_x-rc.right/2,img_ofs_y-rc.bottom/2,0);
          }
        else if(!stricmp(fileScreensaver,"date")) {
          screenSaverRefresh=100;   // max 127!
          clockScreenSaver(hDC,img_ofs_x-rc.right/2,img_ofs_y-rc.bottom/2,1);
          }
        else if(!stricmp(fileScreensaver,"fish")) {   // in teoria questo non abbisogna del ERASEBKGND... gestire
          screenSaverRefresh=4;
          fishScreenSaver(hDC);
          }
        else if(!stricmp(fileScreensaver,"powersaver")) {
          screenSaverRefresh=50;
          setPowerMode(0);
          }
        else {
/*char* reverseCopy(char* destination, const char* source, int num) {
    char* d = destination;
    while (num) {
        *d++ = source[--num];
    }
    *d = '\0';
    return destination;
}*/
          textScreenSaver(hDC,img_ofs_x-rc.right/2,img_ofs_y-rc.bottom/2,fileScreensaver);
          }
        hDC->font=GetStockObject(SYSTEM_FONT).font;
				}
      else {
        activeMenu=NULL; activeMenuWnd=NULL; activeMenuCntX=activeMenuCntY=0; inTrackMenu=FALSE;   // direi

        if(IntersectRect(NULL,&ps.rcPaint,&taskbarWindow->nonClientArea)) {
          SendMessage(taskbarWindow,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);		// ncpaint fa/implica pure paint qua...
          InvalidateRect(taskbarWindow,NULL,TRUE);
          }
        
        HWND myWnd=rootWindows;
        while(myWnd) {
          if(IntersectRect(NULL,&ps.rcPaint,&myWnd->nonClientArea)) {
            SendMessage(myWnd,WM_NCPAINT,(WPARAM)NULL /*upd region*/,0);		// ncpaint fa/implica pure paint qua...
            InvalidateRect(myWnd,NULL,TRUE);
            }

    //#warning usare qua, ma dentro nc_paint
          // magari legato a CS_OWNDC o CS_SAVEBITS ?
    //      DrawWindow(myWnd->nonClientArea.left,myWnd->nonClientArea.top,
    //              myWnd->nonClientArea.right,myWnd->nonClientArea.bottom,
    //              windowInactiveForeColor,windowBackColor); // se la finestra è sotto le altre, come qua!

          myWnd=myWnd->next;
          }
        
        DrawCursor(mousePosition.x,mousePosition.y,mouseCursor,0);
        // in teoria prendere quello di WindowFromPoint(mousePosition);
        }
      
      DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      BYTE attrib=GetWindowByte(hWnd,sizeof(POINTS)*16+16+16+4+4);
      GFX_COLOR f=GetWindowWord(hWnd,GWL_USERDATA+0),b=GetWindowWord(hWnd,GWL_USERDATA+2);
			char *fileWallpaper=(char*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16);
      char *fileScreensaver=(char*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16)+16;
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      BYTE xs=getFontWidth(&hDC->font);
      BYTE ys=getFontHeight(&hDC->font);

 			if(inScreenSaver) {
        if(stricmp(fileScreensaver,"fish") && stricmp(fileScreensaver,"powersaver")) {   // questo/i non abbisogna del ERASEBKGND... 
          DrawWindow(hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right,hWnd->paintArea.bottom,
            b,BLACK /*b*/,0);		// colore?
//#warning sembra disegnare rettangolo oltre il bottom, serve -1??
//PaintDesktop(hDC) 
          }
				}
			else {
//PaintDesktop(hDC) 
        DrawWindow(hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right,hWnd->paintArea.bottom,b,b,1);
      
extern const unsigned char W95train[45398];
extern const unsigned char W95lion[42090];
extern const unsigned char W3tartan[5132];
extern const unsigned char WindowXP[49741];

      if(!strcmp(fileWallpaper,"w95lion.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16)=(BYTE*)W95lion;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16+4)=sizeof(W95lion);
        }
      else if(!strcmp(fileWallpaper,"w95train.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16)=(BYTE*)W95train;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16+4)=sizeof(W95train);
        }
      else if(!strcmp(fileWallpaper,"w3tartan.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16)=(BYTE*)W3tartan;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16+4)=sizeof(W3tartan);
        }
      else if(!strcmp(fileWallpaper,"windowxp.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16)=(BYTE*)WindowXP;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16+4)=sizeof(WindowXP);
        }
      else {
        SUPERFILE **myJpegFile=(SUPERFILE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16);
        *myJpegFile=malloc(sizeof(SUPERFILE));
        (*myJpegFile)->drive=currDrive;
        if(SuperFileOpen(*myJpegFile,fileWallpaper,OPEN_READ,TYPE_BINARY | SHARE_READ)) {   
          *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16+4)=0xffffffff;    // marker File vs. Rom
          }
        else {
          free(*myJpegFile);
          SetWindowLong(hWnd,sizeof(POINTS)*16+16+16+4,0);
          SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(80,60,0,0,FW_NORMAL /*FW_BOLD*/,FALSE,FALSE,FALSE,ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_ROMAN,NULL));
//          img_ofs_y=(hWnd->paintArea.bottom-56)/2; img_ofs_x=(hWnd->paintArea.right-strlen(fileWallpaper)*48)/2;
          SetTextColor(hDC,BRIGHTYELLOW);
          SetBkColor(hDC,desktopColor);
          if(GetWindowByte(taskbarWindow,GWL_USERDATA+0) & 2)    // on top
            hWnd->paintArea.bottom-=16;
          DrawText(hDC,fileWallpaper,-1,&hWnd->paintArea,DT_VCENTER | DT_CENTER | DT_NOPREFIX | DT_SINGLELINE);
          DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);

          hDC->font=GetStockObject(SYSTEM_FONT).font;
          
//        SetTextColor(hDC,WHITE);
//        TextOut(hDC,(rc.right-rc.left-12*6)/2,0,"BREAKTHROUGH");
          }
        }

			if(*GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16)) {
				int x,x1,mcu_x,x_decimate,y_decimate;
				int y,y1,mcu_y;

        x=hWnd->paintArea.left;
        y=hWnd->paintArea.top;
        x1=hWnd->paintArea.right;
        y1=hWnd->paintArea.bottom;
        
//        drawJPGfile(hDC,(char*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16),x,y,&x1,&y1,1);
       // vorrei usarla ma qua sia per velocità che per "tiled" per ora lascio...
        
extern unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, 
  unsigned char *pBytes_actually_read, void *pCallback_data);

	      pjpeg_image_info_t JPG_Info;
        bool status;
				if(!(status=pjpeg_decode_init(&JPG_Info,pjpeg_need_bytes_callback,
          (DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16),0))) {
    			if(attrib & 1) {   // tiled
            img_ofs_y=img_ofs_x=0;
            }
          else {
            img_ofs_y=((int)hWnd->paintArea.bottom-(int)JPG_Info.m_height)/2; 
            img_ofs_x=((int)hWnd->paintArea.right-(int)JPG_Info.m_width)/2;
            }
          if(img_ofs_x<0) img_ofs_x=0;
          if(img_ofs_y<0) img_ofs_y=0;
          
rifo:
          mcu_x=0; mcu_y=0;
					for(;;) {
            uint8_t xPixels;
						if(status = pjpeg_decode_mcu())
							goto error_compressed;
            y_decimate=0;

            if(JPG_Info.m_width>hWnd->paintArea.right)
              xPixels=((DWORD)(hWnd->paintArea.right /*479...*/+1)*8)/JPG_Info.m_width;
              // NON dovremmo calcolarlo così ma lasciarlo uscire da sotto, SOLO CHE STA MERDA DI JPEG lo rende un casino!
            else
              xPixels=8;
            
						for(y=0; y < JPG_Info.m_MCUHeight; y += 8) {
							y1=(mcu_y*JPG_Info.m_MCUHeight) + y;
              x_decimate=0;
							for(x=0; x < JPG_Info.m_MCUWidth; x += 8) {
								x1=((DWORD)xPixels*(((DWORD)mcu_x*JPG_Info.m_MCUWidth) + x)/8)  /* * JPG_Info.m_comps*/;

								// Compute source byte offset of the block in the decoder's MCU buffer.
								uint32_t src_ofs = (x*8) + (y*16);
								const uint8_t *pSrcR = JPG_Info.m_pMCUBufR + src_ofs;
								const uint8_t *pSrcG = JPG_Info.m_pMCUBufG + src_ofs;
								const uint8_t *pSrcB = JPG_Info.m_pMCUBufB + src_ofs;

								uint8_t bx,by,yPixels=0;
                GFX_COLOR mypixels[64],*mypixelsp=mypixels;
								for(by=0; by<8; by++) {
									for(bx=0; bx<8; bx++) {
                    
//										DrawPixel(img_ofs_x+x1+bx, img_ofs_y+y1+by, Color565(*pSrcR,*pSrcG,*pSrcB));
                    x_decimate+= hWnd->paintArea.right   +1;
                    if(x_decimate >= JPG_Info.m_width) {
                      x_decimate -= JPG_Info.m_width;
                      *mypixelsp++=Color565(*pSrcR,*pSrcG,*pSrcB);
                      // e plotto.. FINIRE!
                      }
                    
										pSrcR++; pSrcG++; pSrcB++;
										}
                  y_decimate+=hWnd->paintArea.bottom;
                  if(y_decimate >= JPG_Info.m_height) {
                    y_decimate -= JPG_Info.m_height;
                    // e plotto..
                    }
									}
                
                yPixels=8;
  							PixelBlt(img_ofs_x+x1,img_ofs_y+y1,img_ofs_x+x1+xPixels,img_ofs_y+y1+yPixels,mypixels);
								}
							}

						mcu_x++;      // in x ogni blocco è già 16 pixel (con YUV, pare)
						if(mcu_x >= JPG_Info.m_MCUSPerRow) {
							mcu_x=0;
							mcu_y++;
							}
						ClrWdt();
						}
					}
error_compressed:
   			if(attrib & 1) {   // tiled
          SUPERFILE **myJpegFile=(SUPERFILE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16);
          SuperFileSeek(*myJpegFile,0,SEEK_SET);
  				if(!(status=pjpeg_decode_init(&JPG_Info, pjpeg_need_bytes_callback,
            (DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16),0))) {
            img_ofs_x+=JPG_Info.m_width;
            if(img_ofs_x>=hWnd->paintArea.right) {
              img_ofs_x=0;
              img_ofs_y+=JPG_Info.m_height;
              if(img_ofs_y<hWnd->paintArea.bottom)
                goto rifo;
              }
            else
              goto rifo;
            }
          }
        
//error_jpeg:
					;
        if((*(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16+4)) == 0xffffffff) {
          SUPERFILE **myJpegFile=(SUPERFILE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+16);
          SuperFileClose(*myJpegFile);
          free(*myJpegFile); 
          SetWindowLong(hWnd,sizeof(POINTS)*16+16+16+4,0);
          }
				}
      
      SetTextColor(hDC,ORANGE);
      img_ofs_y=hWnd->paintArea.bottom-ys*3-14;
      img_ofs_x=hWnd->paintArea.right-(xs)*(14+10)-2;
      char buffer[16];
      BYTE i;
// bah no      if(GetWindowLong(hWnd,sizeof(POINTS)*16+16+16+4)) {    // se non è già sullo sfondo, lo scrivo qua
        i=15;
        GetUserName(buffer,&i);
        TextOut(hDC,img_ofs_x,img_ofs_y,"Utente:",7);
        TextOut(hDC,img_ofs_x+xs*10+2,img_ofs_y,buffer,strlen(buffer));
//        }
      i=15;
      GetComputerName(buffer,&i);
      img_ofs_y += ys+1;
      TextOut(hDC,img_ofs_x,img_ofs_y,"Stazione:",9);
      TextOut(hDC,img_ofs_x+xs*10+2,img_ofs_y,buffer,strlen(buffer));
#if defined(USA_WIFI) || defined(USA_ETHERNET)
      IP_ADDR IPAddress=GetIPAddress(-1);
			sprintf(buffer,"%u.%u.%u.%u",IPAddress.v[0],IPAddress.v[1],IPAddress.v[2],IPAddress.v[3]);
      img_ofs_y += ys+1;
      TextOut(hDC,img_ofs_x,img_ofs_y,"Indirizzo:",10);
      TextOut(hDC,img_ofs_x+xs*10+2,img_ofs_y,buffer,strlen(buffer));
#endif
			}

          
      DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
			memset(GET_WINDOW_OFFSET(hWnd,0),sizeof(POINTS)*16+16+16+4+4+1,0);
      if(cs->lpCreateParams)
        strncpy(GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16),cs->lpCreateParams,15);
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(BRIGHTYELLOW,desktopColor));
      }
      return 0;
      break;
    case WM_CLOSE:    // per ALT-F4 finale
//#warning verificare se serve, v.sc_close e taskbar
      quitSignal=TRUE;
//      PostMessage(NULL,WM_QUIT,0,0);
      DestroyWindow(hWnd);
      break;
      
    case WM_KEYDOWN:
      // per ev. gestire TrackPopupMenu, ALT-F4 finale... (in effetti vanno a DefWindowProc...
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_MENUSELECT:   // idem
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        BYTE i;
        switch(LOWORD(wParam)) { 
          case 1:
            {char buf[2]={currDrive,0};
            ShellExecute(NULL,"explore",NULL,GetAsyncKeyState(VK_CONTROL) & 0x8000 ? NULL : buf,
              NULL,SW_SHOWNORMAL);
            }
            break;
          case 2:
            ShellExecute(NULL,"control",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 3:
            ShellExecute(NULL,"taskman",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 4:
//            ShellExecute(GetDesktopWindow() /*NULL*/,"calc",NULL,NULL,NULL,SW_SHOWNORMAL);
            // NON Potevi passare NULL, se è una dialog!
            ShellExecute(NULL,"calc",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 5:
            ShellExecute(NULL,"clock",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 10:
            ShellExecute(NULL,"calendar",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 6:
            if(GetAsyncKeyState(VK_SHIFT) & 0x8000 || GetAsyncKeyState(VK_CONTROL) & 0x8000) {        // tanto per.. lancio DOS!
              ShellExecute(NULL,"dos",NULL,NULL,NULL,SW_SHOWNORMAL);
              }
            else {
              if(MessageBox(NULL,"Esegui","futuro",MB_OKCANCEL))
                ShellExecute(NULL,STR_EMPTY /* contenuto message box */,NULL,NULL,NULL,SW_SHOWNORMAL);
              }
            break;
          case 8:
            ShellExecute(NULL,"basic",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 9:
            ShellExecute(NULL,"surf",NULL,NULL,NULL,SW_SHOWNORMAL /*SW_SHOWMAXIMIZED*/);
            break;
          case 11:
            ShellExecute(NULL,"notepad",NULL,NULL,NULL,SW_SHOWNORMAL /*SW_SHOWMAXIMIZED*/);
            break;
          case 7:
            quitSignal=TRUE;
//    				PostMessage(NULL,WM_QUIT,0,0);
            // uscire da tutto...
            break;
            
          case 17:    // always on top
            i=GetWindowByte(taskbarWindow,GWL_USERDATA+0) ^ 2;
            SetWindowByte(taskbarWindow,GWL_USERDATA+0,i);
//            StdBeep(100+ (i & 2)*100); //debug!
            InvalidateRect(taskbarWindow,NULL,TRUE);
            break;
          case 18:    // mostra desktop
            MinimizeWindows(NULL,NULL);
            break;
          case 19:    //orologio
            i=GetWindowByte(taskbarWindow,GWL_USERDATA+0) ^ 1;
            SetWindowByte(taskbarWindow,GWL_USERDATA+0,i);
            if(i & 1) {
              SetTimer(hWnd,1,60000,NULL);
              // finire completare :) ricreare, oppure
              ShowWindow(GetDlgItem(taskbarWindow,202),SW_SHOW);
              }
            else {
              KillTimer(hWnd,1);
         			//DestroyWindow(GetDlgItem(taskbarWindow,202)); 
              ShowWindow(GetDlgItem(taskbarWindow,202),SW_HIDE);
              }
            break;
          }
        return 1;
        }
      break;
/*    case WM_SYSCOMMAND:
      switch(wParam & 0xfff0) {// qua??? ha senso?
        case SC_CLOSE:    // per ALT-F4 finale!
          quitSignal=TRUE;    // però chiedere conferma :)
//          PostMessage(NULL,WM_QUIT,0,0);
          break;
        }
      break;*/
      
    case WM_HOTKEY:
      switch(wParam) {      // in lParam i modifier OCCHIO DIVERSI DAL SOLITO!
        case IDHOT_SNAPDESKTOP:   // tecnicamente dovrei accettare DESKTOP solo se desktopWindow! 
//        case IDHOT_SNAPWINDOW:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
      
    case WM_RBUTTONDOWN:
      if(wParam & MK_CONTROL) {
        SetXY(0,0,100);
        printWindows(rootWindows);
        }
      else {
        ShellExecute(NULL,"control",(char*)6/*schermo*/,NULL,NULL,SW_SHOWNORMAL);
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_SIZE:   // serve per resize schermo dovuti a cambio video (win-F4 ecc
      if(taskbarWindow)
        SetWindowPos(taskbarWindow,NULL,0,Screen.cy-(11)-1,Screen.cx-1,Screen.cy-1,SWP_NOZORDER | SWP_NOACTIVATE);
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

static void updateTaskbarClock(HWND hWnd) {
  
  HWND myWnd=GetDlgItem(hWnd,202);
  if(myWnd) {
    PIC32_DATE date;
    PIC32_TIME time;
    char buf[8];
    SetTimeFromNow(now,&date,&time);
    sprintf(buf,"%02u:%02u",time.hour,time.min);
    SetWindowText(myWnd,buf);
    }
  }
LRESULT DefWindowProcTaskBar(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_NCPAINT:    // per non avere il bordo "da attivo"
      nonClientPaint(hWnd,(RECT*)wParam,0,FALSE/*TRUE*/);
      break;
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
			BYTE i;
      BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);

      i=0;
      HWND myWnd=rootWindows;
      while(myWnd) {    // qua TUTTE e in ordine di lista/creazione
        drawIcon8(hDC,30+8+i*14,1,myWnd->icon ? myWnd->icon : standardIcon);  // v. anche Desktopclass e taskbar e nonclientpaint
        i++;
        myWnd=myWnd->next;
        }
      
			if(attrib & 1) {
        updateTaskbarClock(hWnd);
        }
			if(attrib & 2) {
//				on top
				}

      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
      fillRectangleWindowColorRect(hDC,&hWnd->paintArea,hDC->brush.color);
      return 1;
      }
      break;
    case WM_SHOWWINDOW:
      InvalidateRect(hWnd,NULL,TRUE);     // qua gestisco il caso particolare... SW_OTHERUNZOOM SW_OTHERZOOM
			return 0;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      // ev. forzare dimensioni e/o posizione
//  cs->x=0;
//  cs->y=Screen.cy-(11)-1;
//      cs->cx=Screen.cx-1;
//      cs->cy=8+2+2 -1;
      BYTE attrib=(DWORD)cs->lpCreateParams;
      SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
      CreateWindow(MAKECLASS(WC_BUTTON),"Start",WS_BORDER | WS_VISIBLE | WS_TABSTOP |
        WS_CHILD | BS_CENTER | BS_PUSHBUTTON,
        0,0,5*6+2,9,
        hWnd,(HMENU)201,NULL
        );
			if(attrib & 1) {   // orologio OCCHIO POI SOTTO USIAMO HIDEWindow...
        CreateWindow(MAKECLASS(WC_STATIC),"00:00",WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED | SS_SIMPLE /* per bianco! SS_CENTER*/ | SS_NOTIFY,  // in effetti potrebbe essere attivo per set clock..
          cs->cx-5*6-4,0,5*6+2,9,
          hWnd,(HMENU)202,NULL
          );
        SetTimer(hWnd,1,60000,NULL);
				}
      if(/*eXtra & 16*/ GetID(AUDIO_CARD) == 'A') {      // se scheda audio... bah
        CreateWindow(MAKECLASS(WC_STATIC),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED | SS_ICON | SS_NOTIFY,
          cs->cx-5*6-4-11,0,8,9,
          hWnd,(HMENU)203,NULL
          );
        SendMessage(GetDlgItem(hWnd,203),STM_SETICON,(WPARAM)speakerIcon,0);
        }
      if(1 /*ReadPowerType() == 1*/) {      // se batteria  PER ORA SEMPRE :)
        CreateWindow(MAKECLASS(WC_STATIC),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED | SS_ICON | SS_NOTIFY,
          cs->cx-5*6-4-11*2,0,8,9,
          hWnd,(HMENU)204,NULL
          );
        SendMessage(GetDlgItem(hWnd,204),STM_SETICON,(WPARAM)batteryIcon,0);
        }
      
      if(attrib & 2) {
//				on top
				}
      }
      return 0;
      break;
    case WM_CTLCOLOR:
      return GRAY096;
      break;
/*    case WM_PARENTNOTIFY:
      switch(LOWORD(wParam)) {
        case WM_LBUTTONDOWN:
          SendMessage(hWnd,WM_COMMAND,MAKELONG(
          break;
        }
        return DefWindowProc(hWnd->parent,message,wParam,lParam);
      break;*/
    case WM_KEYDOWN:
      // per ev. gestire TrackPopupMenu, ALT-F4 finale...
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          }
        }
      else if(HIWORD(wParam) == BN_CLICKED) {   // è 0!! coglioni :D
        switch(LOWORD(wParam)) { 
          case 201:
            SendMessage(taskbarWindow,WM_SYSCOMMAND,MAKELONG(SC_TASKLIST,0),0);
            break;
          }
        }
      else if(HIWORD(wParam) == STN_CLICKED) {   // 0 idem...
        switch(LOWORD(wParam)) { 
          case 203:   // audio
            {char buffer[16];
            sprintf(buffer,"%s: volume %u%%",eXtra & 16 /*&& GetID(AUDIO_CARD) == 'A' v. sopra*/ ? 
              "Attivo" : "Non attivo",volumeAudio);
            MessageBox(NULL,buffer,"Audio",MB_OK);
            }
            break;
          case 204:   // power
            {char buffer[16];
            BYTE j=ReadPower(),i=ReadPowerType();
            sprintf(buffer,"%s (%u%%)",!i ? "Mains" : "Battery",j);    // 
            MessageBox(NULL,buffer,"Power",MB_OK);
            }
            break;
          }
        }
      else if(HIWORD(wParam) == STN_DBLCLK) {   // 1
        switch(LOWORD(wParam)) { 
          case 202:   // orologio, 
            PostMessage(desktopWindow,WM_COMMAND,MAKELONG(5,0),0);
            break;
          }
        }
      return 1;
      break;
    case WM_SYSCOMMAND:
      switch(wParam & 0xfff0) {
        case SC_TASKLIST:
/* o NO? bah in effetti direi SEMPRE*/            if(activeMenu==&menuStart) {
//              activeMenuWnd=desktopWindow;    //truschini per chiudere bene! MA E' già così?! v. subito sotto
            SendMessage(desktopWindow,WM_MENUSELECT,MAKELONG(0,0xffff),0);
            }
          else {
            if(TrackPopupMenu((HMENU)&menuStart,TPM_RETURNCMD | TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
              1,hWnd->nonClientArea.top /*mousePosition.x,mousePosition.y qua fisso!*/,0,desktopWindow /*hWnd*/,NULL)) {
              }
            }
          break;
        case SC_CLOSE:    // per ALT-F4 finale!
          quitSignal=TRUE;    // però chiedere conferma :)
//          PostMessage(NULL,WM_QUIT,0,0);
          break;
        }
      break;
    case WM_LBUTTONDOWN:
// idem SEMPRE!           if(activeMenu==&menuStart) {
      SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_RBUTTONDOWN:
      {MENU myMenuStart2;//      copiare menu in ram e metterci i flag!
      // POTREBBE andare, ma dev'essere allocato globalmente perché track per ora non blocca...
      BYTE i=0;
      do {
        myMenuStart2.menuItems[i]=menuStart2.menuItems[i];
        i++;
        } while(*menuStart2.menuItems[i].text);
      if(GetWindowByte(hWnd,GWL_USERDATA+0) & 2)    // always on top
        myMenuStart2.menuItems[0].flags |= MF_CHECKED;
      if((GetWindowByte(hWnd,GWL_USERDATA+0) & 1) && IsWindowVisible(GetDlgItem(hWnd,202)))    // clock OCCHIO v. anche attrib & 1
        myMenuStart2.menuItems[1].flags |= MF_CHECKED;
      if(TrackPopupMenu((HMENU)&/*myMenuStart2*/menuStart2,TPM_RETURNCMD | TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_SHOWCHECKS,
        mousePosition.x,hWnd->nonClientArea.top,0,desktopWindow /*hWnd*/,NULL)) {
        }
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_TIMECHANGE:
    case WM_TIMER:
      {BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
			if(attrib & 1) {
        updateTaskbarClock(hWnd);
        }
      }
      break;
      
    case WM_ACTIVATE:   
      // ovviamente non va...
      if(GetWindowByte(hWnd,GWL_USERDATA+0) & 2) { //se non on top, non passare davanti su messaggi da children/orologio
        return DefWindowProc(hWnd,message,wParam,lParam);
        }
      break;
    case WM_WINDOWPOSCHANGING:
      {WINDOWPOS *wpos=(WINDOWPOS*)lParam;
        int i;
        i=hWnd->style & WS_BORDER ? 2 : 0;
        i+=hWnd->style & WS_THICKFRAME ? 2 : 0;
        // gestire volendo, per forzare dimensioni! (anche in create
//  wpos->x=0;
//  wpos->y=Screen.cy-(11)-1;
//      wpos->cx=Screen.cx-1;
//      wpos->cy=8+2+2 -1;
      }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


LRESULT DefWindowProcVideoCap(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
			BYTE i;
      HWND myWnd;
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      RECT rc;
      GetClientRect(hWnd,&rc);
      
      SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,WHITE));
      SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(desktopColor));

#ifdef USA_USB_HOST_UVC   // usare BBitBlt per immagine a video ? ma in finestra...
      int x,y,y1,x1;
#warning stretchare img , come fatto in desktopwindow!
      bool status=false; 
      char *nomefile=NULL;
      BYTE m;
      WORD divider=0;
#define clip(n) { if(n<0) n=0; else if(n>255) n=255; }

      m=0;    // CALCOLARE m da dimensioni window, ev. fare stretch
      if(m)
        APP_VideoDataSubSetDefault(m);
      // per FORZARE le nuove impostazioni, so dovrebbe fare un DETACH software o forse può bastare
  //          appData.state = APP_STATE_ZERO_BANDWIDTH_INTERFACE_SET; ...

      img_ofs_x=rc.right-rc.left-appData.videoStreamFormat.videoSize.cx;
      img_ofs_y=rc.bottom-rc.top-appData.videoStreamFormat.videoSize.cy-10;

      if(appData.deviceIsConnected) {
        x=0;
        capturingVideo=CAPTURE_CAPTURE;

        while(capturingVideo != CAPTURE_DONE) {
          SYS_Tasks();
          divider++;
          if(divider > 4000) {    // 1/3 1/4 sec
            divider=0;
            SetCursor(&hourglassCursorSm);
            handle_events();
//            if(hitCtrlC(TRUE))
//              break;
            }
          }
        if(capturingVideo==CAPTURE_DONE) {
          capturingVideo=CAPTURE_IDLE;

          if(appData.videoStreamFormat.format == USB_UVC_FORMAT_MJPG) {
            pjpeg_image_info_t JPG_Info;
            int mcu_x=0;
            int mcu_y=0;
        		int x_decimate,y_decimate;
            if(status=pjpeg_decode_init(&JPG_Info, pjpeg_need_bytes_callback, NULL,0)) {
              goto error_webcam;
              }
            else {
              for(;;) {

                if(status = pjpeg_decode_mcu())
                  goto error_compressed;

                for(y=0; y < JPG_Info.m_MCUHeight; y += 8) {
                  y1=(mcu_y*JPG_Info.m_MCUHeight) + y;
                  for(x=0; x < JPG_Info.m_MCUWidth; x += 8) {
                    x1=(mcu_x*JPG_Info.m_MCUWidth) + x  /* * JPG_Info.m_comps*/;

                    uint32_t src_ofs = (x * 8U) + (y * 16U);
                    const uint8_t *pSrcR = JPG_Info.m_pMCUBufR + src_ofs;
                    const uint8_t *pSrcG = JPG_Info.m_pMCUBufG + src_ofs;
                    const uint8_t *pSrcB = JPG_Info.m_pMCUBufB + src_ofs;

                    uint8_t bx,by;
                    for(by=0; by<8; by++) {
                      for(bx=0; bx<8; bx++) {
                        drawRectangleColor(img_ofs_x+1+x1+bx,img_ofs_y+1+y1+by,
                          img_ofs_x+1+x1+bx  +1, img_ofs_y+1+y1+by  +1,
                          Color565(*pSrcR,*pSrcG,*pSrcB));
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

  error_webcam:
  error_compressed:
                ;
              }
            }
          else {     // YUY
            if(*nomefile) {
              SUPERFILE f;
              int bitBuf,bitCnt;
              BYTE quality=70;
              BYTE subsample = quality <= 90 ? 1 : 0;
              BYTE obuf[appData.videoStreamFormat.videoSize.cx*3*16],*pBuf;
              int Y, U, Y2, V;
              int r,g,b;
              int c,d,e;
              int byteCnt;
          		int x_decimate,y_decimate;

              x=y=0;

              while(VideoBufferLen2>0) {
                int Y = *VideoBufferPtr2++;
                int U = *VideoBufferPtr2++;
                int Y2= *VideoBufferPtr2++;
                int V = *VideoBufferPtr2++;
                int r,g,b;

                int c = Y-16;
                int d = U-128;
                int e = V-128;
                c *= 298;
                b = (c + 516*d + 128) >> 8;   // blue
                clip(b);
                g = (c - 100*d - 208*e + 128) >> 8; // green
                clip(g);
                r = (c + 409*e + 128) >> 8;   // red
                clip(r);

        //        YUV2RGB[Y][U][V];     // USARE!!

                drawRectangleWindowColor(hDC,img_ofs_x+1+x1+x++,img_ofs_y+1+y1+y,
                  img_ofs_x+1+x1+x++  +1,img_ofs_y+1+y1+y  +1,
                  Color565(r,g,b));

                c = Y2-16;
                c *= 298;
                b = (c + 516*d + 128) >> 8;   // blue
                clip(b);
                g = (c - 100*d - 208*e + 128) >> 8; // green
                clip(g);
                r = (c + 409*e + 128) >> 8;   // red
                clip(r);

                drawRectangleWindowColor(hDC,img_ofs_x+1+x1+x++,img_ofs_y+1+y1+y,
                  img_ofs_x+1+x1+x++  +1,img_ofs_y+1+y1+y  +1,
                  Color565(r,g,b));

                VideoBufferLen2-=4;
                if(x>=appData.videoStreamFormat.videoSize.cx) {
                  x=0;
                  y++;
                  }
                }
              }
            }
          }
        else {
          TextOut(hDC,0,0,"No webcam detected.");
          }
        }
fine_webcam: ;

#else
			DrawText(hDC,"Video unavailable",-1,&rc,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
#endif

      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT*)lParam;
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(CYAN,BLACK));
      SetWindowLong(hWnd,0,0);    // azzero dati vari
      hWnd->scrollSizeX=hWnd->scrollSizeY=0;
      }
      return 0;
      break;
      
    case WM_COMMAND:
      if(!lParam && (HIWORD(wParam) == 0 || HIWORD(wParam) == 1)) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        switch(LOWORD(wParam)) { 
          case 1:
            break;
	        }
        }
      break;
    case WM_LBUTTONDOWN: 
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_RBUTTONDOWN:
      // impostazioni video...
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;

#ifdef USA_USB_HOST_UVC   // 
    case WM_CAP_ABORT:
//      SetWindowByte(hWnd,0,0);    // è per fermare video cattura...
      break;
    case WM_CAP_DLG_VIDEODISPLAY:
      break;
    case WM_CAP_DLG_VIDEOCOMPRESSION:
      break;
    case WM_CAP_DLG_VIDEOFORMAT:
      break;
    case WM_CAP_DLG_VIDEOSOURCE:
      break;
    case WM_CAP_DRIVER_CONNECT:
      SetWindowByte(hWnd,0,GetWindowByte(hWnd,0) | 1);    // attivo cattura
      break;
    case WM_CAP_DRIVER_DISCONNECT:
      SetWindowByte(hWnd,0,GetWindowByte(hWnd,0) & ~1);    // fermocattura
      break;
    case WM_CAP_DRIVER_GET_CAPS:
      //wParam = (WPARAM) (wSize); 
      //lParam = (LPARAM) (LPVOID) (LPCAPDRIVERCAPS) (psCaps); 
      break;
    case WM_CAP_DRIVER_GET_NAME:
      strncpy((char*)lParam,"UVC USB Videocapture by G.Dar");
      break;
    case WM_CAP_DRIVER_GET_VERSION:
      strncpy((char*)lParam,"UVC USB 1.0");
      break;
    case WM_CAP_EDIT_COPY:
      break;
    case WM_CAP_GET_USER_DATA:
      return hWnd->userdata;
      break;
    case WM_CAP_GET_VIDEOFORMAT:
      break;
    case WM_CAP_GET_SEQUENCE_SETUP:
      //wParam = (WPARAM) (wSize); 
      //lParam = (LPARAM) (LPVOID) (LPCAPTUREPARMS) (psCapParms); 
      break;
    case WM_CAP_GRAB_FRAME:
      break;
    case WM_CAP_SET_CALLBACK_ERROR:
      break;
    case WM_CAP_SET_CALLBACK_FRAME:
      break;
    case WM_CAP_SET_CALLBACK_STATUS:
      break;
    case WM_CAP_SET_CALLBACK_VIDEOSTREAM:
      break;
    case WM_CAP_SET_CALLBACK_WAVESTREAM:
      break;
    case WM_CAP_SET_OVERLAY:
      SetWindowByte(hWnd,0,GetWindowByte(hWnd,0) | 2);    // overlay
      break;
    case WM_CAP_SET_PREVIEW:
      SetWindowByte(hWnd,0,GetWindowByte(hWnd,0) & ~2);    // overlay
      break;
    case WM_CAP_SET_PREVIEWRATE:
      SetWindowByte(hWnd,1,wParam);    // mS
      SetTimer(hWnd,1,wParam,NULL);    // timer x preview
      break;
    case WM_CAP_SET_SCALE:
      
      break;
    case WM_CAP_SET_SEQUENCE_SETUP:
      //wParam = (WPARAM) (wSize); 
      //lParam = (LPARAM) (LPVOID) (LPCAPTUREPARMS) (psCapParms); 
      break;
    case WM_CAP_SET_USER_DATA:
      hWnd->userdata=lParam;
      break;
    case WM_CAP_SET_VIDEOFORMAT:
      break;
    case WM_CAP_SINGLE_FRAME:
      break;
    case WM_CAP_STOP:
//      SetWindowByte(hWnd,0,0);    // è per fermare video cattura...
      break;
#endif
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


#ifdef USA_USB_HOST_UVC   // 
BOOL capGetDriverDescription(BYTE wDriverIndex,char *lpszName,
  uint16_t cbName,char *lpszVer,uint16_t cbVer) {

  if(wDriverIndex==0) {
    // trovare hwnd..
    capDriverGetName(hWnd,lpszName,cbName);
    capDriverGetVersion(hWnd,lpszVer,cbVer);
    }
  }

HWND capCreateCaptureWindow(const char lpszWindowName, DWORD dwStyle,
  UGRAPH_COORD_T x,UGRAPH_COORD_T y,UGRAPH_COORD_T nWidth,UGRAPH_COORD_T nHeight,
  HWND hwndParent,int nID) {
  
  return CreateWindow(MAKECLASS(WC_VIDEOCLASS),lpszWindowName,dwStyle,
    x,y,nWidth,nHeight,hwndParent,nID,0);
  }

CAPVIDEOCALLBACK Capvideocallback;
LRESULT Capvideocallback(HWND hWnd,VIDEOHDR *lpVHdr) {
  }
CAPSTATUSCALLBACK Capstatuscallback;
LRESULT Capstatuscallback(HWND hWnd,int nID,const char *lpsz) {
  }
CAPERRORCALLBACK capErrorCallback;
LRESULT capErrorCallback(HWND hWnd,int nID,const char *lpsz) {
  }


// codice di test da vidosender:

		hWnd = capCreateCaptureWindow (
			(LPSTR)STR_EMPTY, // window name if pop-up 
			WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS,       // window style 
			0,0,biRawBitmap.biWidth,biRawBitmap.biHeight,
			m_Parent->m_hWnd, 1); 	  // parent, child ID 


		if(hWnd) {
		// Register the callback function before connecting capture driver. 
			setCallbackOnError(ErrorCallbackProc); 
	   	setCallbackOnStatus(StatusCallbackProc); 
			setCallbackOnFrame(FrameCallbackProc); 
			i=driverConnect(); 
			if(i<=0) {				// se non c'e', distruggo (v. distruttore) tutto fin qui
				setCallbackOnFrame();
				setCallbackOnError();
				setCallbackOnStatus(); 
				DestroyWindow(hWnd);
				goto no_hwnd;
				}
			capDriverGetCaps(hWnd,&captureCaps,sizeof(CAPDRIVERCAPS));
			capCaptureGetSetup(hWnd,&captureParms, sizeof(CAPTUREPARMS));
			captureParms.dwRequestMicroSecPerFrame = (DWORD) (1.0e6 / framesPerSec);
			captureParms.wPercentDropForError=99;		// anche se ci sono fotogrammi persi, non mi interessa!
			captureParms.fYield = TRUE;
			captureParms.dwIndexSize=100000;		// non dovrebbe servire...
			captureParms.fCaptureAudio = bAudio;
			captureParms.vKeyAbort=0;
			captureParms.fAbortRightMouse =FALSE;
			captureParms.fAbortLeftMouse =FALSE;
			captureParms.wNumVideoRequested=8;
			captureParms.wNumAudioRequested=5;
			captureParms.wStepCaptureAverageFrames=0;
			captureParms.dwAudioBufferSize =8000 /*wfex.nSamplesPerSec non e' ancora assengato, qua!*/;
			i=capCaptureSetSetup(hWnd,&captureParms,sizeof(CAPTUREPARMS)); 
			biBaseRawBitmap=biRawBitmap;
			biBaseRawBitmap.biCompression=0;
			biBaseRawBitmap.biBitCount=24;
			i=setVideoFormat(&biRawBitmap);

//			BITMAPINFOHEADER bi;
			i=getVideoFormat(&biRawBitmap);	// certe telecamere/schede si fanno i cazzi loro (SENZA dare errore), in questo modo vedo che fotogrammi mi dara'...


			overlay(FALSE); preview(FALSE);
			allowOverlay ? overlay(TRUE) : preview(TRUE);
#endif
