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
#ifdef USA_USB_HOST_UVC		// occhio la usiamo pure in breakthrough
#include "usb_host_uvc.h"
extern BYTE *VideoBufferPtr2;
extern DWORD VideoBufferLen2;
extern enum CAPTURING_VIDEO capturingVideo;
#endif

extern volatile unsigned long now;
extern BYTE SDcardOK,USBmemOK,HDOK,FDOK;
extern BYTE *RAMdiscArea;
extern SIZE Screen;
extern DWORD extRAMtot;



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


LRESULT DefWindowProcStaticWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i,x,y;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      
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
          DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,DT_VCENTER | DT_LEFT |
            (hWnd->style & SS_NOPREFIX ? DT_NOPREFIX : 0) | 
            ((hWnd->style & SS_TYPEMASK == SS_LEFTNOWORDWRAP) ? DT_SINGLELINE : 0));
          break;
        case SS_CENTER:
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY192);
          SetBkColor(hDC,c);
          DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,DT_VCENTER | DT_CENTER |
            (hWnd->style & SS_NOPREFIX ? DT_NOPREFIX : 0));
          break;
        case SS_RIGHT:
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY192);
          SetBkColor(hDC,c);
          DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,DT_VCENTER | DT_RIGHT |
            (hWnd->style & SS_NOPREFIX ? DT_NOPREFIX : 0));
          break;
        case SS_ICON:
          drawIcon8(hDC,ps.rcPaint.left,ps.rcPaint.top,hWnd->icon);
          break;
        case SS_BITMAP:
//          DrawIcon(hDC,0,0,hWnd->icon);
          break;
        case SS_BLACKFRAME:
          SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BLACK));
          drawRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          break;
        case SS_GRAYFRAME:
          SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,LIGHTGRAY));
          drawRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          break;
        case SS_WHITEFRAME:
          SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,WHITE));
          drawRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
          break;
        case SS_BLACKRECT:
          fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,BLACK);
          break;
        case SS_GRAYRECT:
          fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,GRAY192);
          break;
        case SS_WHITERECT:
          fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom,WHITE);
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
          dis.rcItem=ps.rcPaint;
          dis.CtlType=ODT_STATIC;
          SendMessage(hWnd->parent,WM_DRAWITEM,(WPARAM)(DWORD)hWnd->menu,(LPARAM)&dis);
          }
          break;
        }
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
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
      DrawFrameControl(hDC,&hWnd->paintArea,0,c);
      // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+0 ma qua non c'è nessun stato cmq));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
// qua non uso      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      if(!(cs->style & SS_NOTIFY))    // bah..
        cs->style |= WS_DISABLED;   // 
      if((cs->style & 0xff) == SS_ICON)
        cs->cx=cs->cy=8 + (hWnd->style & WS_BORDER ? 2 : 0); // type ecc..
      else
        cs->cy=getFontHeight(&hWnd->font) + (cs->style & WS_BORDER ? 2 : 0); // 
      }
      return 0;
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_SETFOCUS:
      if(hWnd->style & SS_NOTIFY) {   // bah..
        hWnd->focus=1;
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_SETFOCUS),(LPARAM)hWnd);
        }
      return 0;
      break;
    case WM_KILLFOCUS:
      hWnd->focus=0;
      return 0;
      break;

    case WM_LBUTTONDOWN:
      if(hWnd->style & SS_NOTIFY && hWnd->parent /*&& hWnd->enabled*/)
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((BYTE)(DWORD)hWnd->menu,STN_CLICKED),(LPARAM)hWnd);
      break;
    case WM_LBUTTONDBLCLK:
      if(hWnd->style & SS_NOTIFY && hWnd->parent /*&& hWnd->enabled*/)
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((BYTE)(DWORD)hWnd->menu,STN_DBLCLK),(LPARAM)hWnd);
      break;
/* verificare che fa, trasforma static in edit??    case STN_ENABLE:
      break;
    case STN_DISABLE:
 */
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcEditWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  char *s;
  s=(char*)GET_WINDOW_OFFSET(hWnd,0);
  BYTE startSel,endSel;
  startSel=GetWindowByte(hWnd,GWL_USERDATA+2);
  endSel=GetWindowByte(hWnd,GWL_USERDATA+3);
  
  switch(message) {
    case WM_PAINT:
      {
      int i,x,y;
      PAINTSTRUCT ps;
      
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR  /*WM_CTLCOLOREDIT*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }
      
      if(hWnd->parent)
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_UPDATE),(LPARAM)hWnd);
      
      x=ps.rcPaint.left; y=ps.rcPaint.top;
      while(*s) {
        if(startSel || endSel) {
          if(x>=startSel && x<=endSel)
            SetTextColor(hDC,BLACK);
          else
            SetTextColor(hDC,WHITE);
          }
        else
          SetTextColor(hDC,WHITE);
        SetBkColor(hDC,c);
        drawCharWindow(hDC,x*getFontWidth(&hDC->font),y,*s++);
        x++;
        }
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
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
      DrawFrameControl(hDC,&hWnd->paintArea,0,c);
      //finire DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+0 stato?caret?));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      SetWindowLong(hWnd,0,0);    // azzero content
      strcpy((char*)GET_WINDOW_OFFSET(hWnd,0),cs->lpszName);    // sicuramente più grande!
      if(!(hWnd->style & WS_DISABLED))		// boh sì :) per pulizia
				caretPosition.x=caretPosition.y=0;
      }
      return 0;
      break;
    case WM_CLOSE:
      KillTimer(hWnd,31);
      DestroyWindow(hWnd);
      break;
      
    case WM_LBUTTONDOWN:
      {
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // per prova!
      SetWindowByte(hWnd,GWL_USERDATA+3,0);
      //startsel ecc
      BYTE x=GET_X_LPARAM(lParam) / getFontWidth(&hWnd->font);
      x=min(x,63); x=min(x,GetWindowByte(hWnd,GWL_USERDATA+0));
      SetWindowByte(hWnd,GWL_USERDATA+0,x);
      caretPosition.x=x *getFontWidth(&hWnd->font); caretPosition.y= 0* getFontHeight(&hWnd->font); 
      InvalidateRect(hWnd,NULL,TRUE);
      if(hWnd->enabled)
        SendMessage(hWnd,WM_SETFOCUS,0,0);
      return 0 /*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;
    case WM_CHAR:
      { char buf[8];
      int i;
      DC myDC;
      HDC hDC;
      POINT pt;
      
//      if(hWnd->enabled) {
        if(GetAsyncKeyState(VK_CONTROL)) {   // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
          // non so van gestiti qua o da fuori... 
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
            }
          }
        else {
          switch(wParam) {
            case VK_DELETE:
              goto do_del;
              break;
            case VK_LEFT:
              if(GetAsyncKeyState(VK_SHIFT)) {
                }
              else {
                }
              break;
            case VK_RIGHT:
              if(GetAsyncKeyState(VK_SHIFT)) {
                }
              else {
                }
              break;
            case VK_UP:
              if(hWnd->style & ES_MULTILINE) {
                if(GetAsyncKeyState(VK_SHIFT)) {
                  }
                else {
                  }
                }
              break;
            case VK_DOWN:
              if(hWnd->style & ES_MULTILINE) {
                if(GetAsyncKeyState(VK_SHIFT)) {
                  }
                else {
                  }
                }
              break;
            case VK_RETURN:   // gestire tasti strani..
              s[0]=0;  //GESTIRE multilinea o boh!
              caretPosition.x=0; caretPosition.y=0;
              SetWindowByte(hWnd,GWL_USERDATA+0,0);   // 
              break;
            case VK_BACK:
              if((pt.x=GetWindowByte(hWnd,GWL_USERDATA+0)) > 0) {
                SetWindowByte(hWnd,GWL_USERDATA+0,--pt.x); s[pt.x]=0; }
              hDC=GetDC(hWnd,&myDC); 
              caretPosition.x=pt.x *getFontWidth(&hDC->font); caretPosition.y=pt.y  *getFontHeight(&hDC->font); 
              ReleaseDC(hWnd,hDC);    InvalidateRect(hWnd,NULL,TRUE); // per velocità!
              break;
            default:
              buf[0]=LOBYTE(wParam); buf[1]=0;
							if(!GetAsyncKeyState(VK_SHIFT))    // gestisco SHIFT, xché i CHAR son solo maiuscoli!
								buf[0]=tolower(buf[0]);
              hDC=GetDC(hWnd,&myDC);
              pt.x=GetWindowByte(hWnd,GWL_USERDATA+0);   // curpos
              pt.y=0;   // curpos
              s[pt.x]=buf[0]; s[pt.x+1]=0;
//              TextOut(hDC,pt.x *6 * hDC->font.size,pt.y  *8*hDC->font.size,buf); PER ORA FACCIO PRIMA a Invalidate!
              if(pt.x<64   )       // curpos
                pt.x++;
              caretPosition.x=pt.x *getFontWidth(&hWnd->font); caretPosition.y=pt.y*getFontHeight(&hWnd->font);
              ReleaseDC(hWnd,hDC);
              SetWindowByte(hWnd,GWL_USERDATA+0,pt.x);   // 
              InvalidateRect(hWnd,NULL,TRUE);
              break;
            }

          SetWindowByte(hWnd,GWL_USERDATA+1,GetWindowByte(hWnd,GWL_USERDATA+1) | 2);
          if(1  /*sempre? hWnd->style & BS_NOTIFY*/)
            SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd);
          }
//        }
//      else
//        return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_SETFOCUS:
      if(!hWnd->focus) {
        int i;
        drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,TRUE);
        SetTimer(hWnd,31,caretTime*10,NULL);
        }
      
//                InvalidateRect(hWnd,NULL,TRUE);

      if(hWnd->style & BS_NOTIFY)
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_SETFOCUS),(LPARAM)hWnd);
      hWnd->focus=1;
      return 0;
      break;
    case WM_KILLFOCUS:
      {
      drawCaret(hWnd,caretPosition.x,caretPosition.y,standardCaret,FALSE);
      KillTimer(hWnd,31);
      if(hWnd->style & BS_NOTIFY)
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_KILLFOCUS),(LPARAM)hWnd);
      hWnd->focus=0;
      }
      return 0;
      break;
    case WM_SETTEXT:
      {
      strncpy(s,(const char *)lParam,   64-1);
      s[   64-1]=0;
      SetWindowByte(hWnd,GWL_USERDATA+0,strlen(s));   // curpos ecc
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // 
      SetWindowByte(hWnd,GWL_USERDATA+3,0);   // 
      InvalidateRect(hWnd,NULL,TRUE);
      caretPosition.x=(DWORD)GetWindowByte(hWnd,GWL_USERDATA+0)*getFontWidth(&hWnd->font); caretPosition.y= getFontHeight(&hWnd->font);
      if(1  /*sempre? hWnd->style & BS_NOTIFY*/)
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd);
      }
      return 1;
      break;
    case WM_GETTEXT:
      return (DWORD)(char*)GET_WINDOW_OFFSET(hWnd,0);
      break;
    case WM_GETTEXTLENGTH:
      return strlen((char*)GET_WINDOW_OFFSET(hWnd,0));
      break;

    case EM_CANUNDO:
      break;
    case EM_CHARFROMPOS:
      {
      BYTE x=GET_X_LPARAM(lParam) / 6;
      if(x<63)
        return s[x];
      }      
      break;
    case EM_GETLINE:
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
      break;
    case EM_LIMITTEXT:
//    case EM_SETLIMITTEXT:
      break;
    case EM_LINEINDEX:
      break;
    case EM_LINEFROMCHAR:
      {
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
      break;
    case EM_SETHANDLE:
      break;
    case EM_SETPASSWORDCHAR:
      break;
    case EM_SETREADONLY:
      break;
    case EM_SETEVENTMASK:
      break;
    case EM_SETSEL:
      {
      int i;
      i=strlen(s);
      wParam=max(wParam,i);
      lParam=min(lParam,i);
      SetWindowByte(hWnd,GWL_USERDATA+2,wParam);
      SetWindowByte(hWnd,GWL_USERDATA+3,lParam);
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
        SetClipboardData(CF_TEXT,s);    // 
        CloseClipboard();
        }
      break;
    case WM_CUT:
do_cut:
      if(OpenClipboard(hWnd)) {
        SetClipboardData(CF_TEXT,s);    // 
        *s=0;
        CloseClipboard();
        }
      break;
    case WM_PASTE:
do_paste:
      if(OpenClipboard(hWnd)) {
        if(GetClipboardData(CF_TEXT))    // 
          SetWindowText(hWnd,GetClipboardData(CF_TEXT));
        CloseClipboard();
        }
      break;
    case WM_CLEAR:
do_del:
      *s=0;     // gestire Sel!
      break;
      
    case WM_GETDLGCODE: 
      return DLGC_HASSETSEL | DLGC_WANTALLKEYS | DLGC_WANTCHARS | DLGC_WANTARROWS;
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

LRESULT DefWindowProcListboxWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  int i;
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int x,y;
      LISTITEM *item;
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;

      if(hWnd->style & (LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE)) {
        DRAWITEMSTRUCT dis;
        dis.CtlID=(DWORD)hWnd->menu;
        dis.itemID=0;
        dis.itemState=GetWindowByte(hWnd,GWL_USERDATA+1);   // TRADURRE! ODS_CHECKED ecc
        dis.hwndItem=hWnd;
        dis.hDC=hDC;
        dis.rcItem=ps.rcPaint;
        dis.CtlType=ODT_LISTBOX;
        SendMessage(hWnd->parent,WM_DRAWITEM,(WPARAM)(DWORD)hWnd->menu,(LPARAM)&dis);
        }
      else {
        if(hWnd->style & WS_CHILD) {
          if((c=SendMessage(hWnd->parent,WM_CTLCOLOR  /*WM_CTLCOLORLISTBOX*/,
            (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
            c=hDC->brush.color;
            }
          }

        item=(LISTITEM *)GetWindowLong(hWnd,0);    // listitem *
        x=0; y=0;
        while(item && y<hWnd->scrollPosY) {   // finire...
          item=item->next;
          y++;
          }
        y=0;
        while(item) {
          if(item->state & 1) {
            SetTextColor(hDC,BLACK /*c*/);
            SetBkColor(hDC,(hWnd->active ? windowForeColor : windowInactiveForeColor));
            fillRectangleWindowColor(hDC,ps.rcPaint.left,y,ps.rcPaint.right,y+getFontHeight(&hWnd->font),
              /*hWnd->active ? */ windowForeColor /*: windowInactiveForeColor*/);
            }
          // in effetti Windows non sembra usare colore attivo/inattivo, ma sempre attivo; e quando la finestra si disattiva la riga pare deselezionarsi.
          else {
            SetBkColor(hDC,c);
            SetTextColor(hDC,(hWnd->active ?  windowForeColor : windowInactiveForeColor));
            fillRectangleWindowColor(hDC,ps.rcPaint.left,y,ps.rcPaint.right,y+getFontHeight(&hWnd->font),c);
            }
          if(hWnd->style & LBS_USETABSTOPS)
            ;
          TextOut(hDC,x+1,y,item->data);    // finire :)
          y+=getFontHeight(&hWnd->font);
          drawHorizLineWindowColor(hDC,ps.rcPaint.left,y++,ps.rcPaint.right,windowForeColor);
          if(y>=ps.rcPaint.bottom)
            break;
          item=item->next;
          }
        }
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
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
      DrawFrameControl(hDC,&hWnd->paintArea,0,c);
      // USARE DrawFrameControl(hDC,&rc,DFC_LISTBOX,GetWindowByte(hWnd,GWL_USERDATA+0  caratterist. speciali??));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      SetWindowLong(hWnd,0,0);    // azzero root
      }
      return 0;
      break;
    case WM_SETREDRAW:
      if(wParam)
        hWnd->style &= ~LBS_NOREDRAW;
      else
        hWnd->style |= LBS_NOREDRAW;
      break;
    case WM_CHAR:
      { char buf[8];
        int j;
//        if(hWnd->enabled) {   // ma se è disabled arrivano o no? penso di no..
          switch(wParam) {
            case VK_UP:
              j=GetWindowWord(hWnd,GWL_USERDATA+0);
              if(j>0) {
                j--;
                SendMessage(hWnd,LB_SETSEL,0,j);
                SendMessage(hWnd,LB_SETSEL,1,j-1);
                SetWindowWord(hWnd,GWL_USERDATA+0,j);
                }
              break;
            case VK_DOWN:
              j=GetWindowWord(hWnd,GWL_USERDATA+0);
              if(j<GetWindowWord(hWnd,GWL_USERDATA+2)) {
                SendMessage(hWnd,LB_SETSEL,0,j);
                j++;
                SendMessage(hWnd,LB_SETSEL,1,j);
                SetWindowWord(hWnd,GWL_USERDATA+0,j);
                }
              break;
            default:
              buf[0]=wParam; buf[1]=0;
              SendMessage(hWnd,LB_SELECTSTRING,0xffffffff,(LPARAM)buf);    // prova!!
            }
  //        }
  //      else
  //        return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_CHARTOITEM:
      //Sent by a list box with the LBS_WANTKEYBOARDINPUT style to its owner in response to a WM_CHAR message.
      break;
    case WM_GETDLGCODE: 
      return DLGC_WANTALLKEYS | DLGC_WANTCHARS | DLGC_WANTARROWS;
      break;
      
    case WM_LBUTTONDOWN:
      {
      i=SendMessage(hWnd,LB_ITEMFROMPOINT,0,lParam);
      if(i != LB_ERR) {
        int j;
        j=SendMessage(hWnd,LB_GETCURSEL,0,0);
        SendMessage(hWnd,LB_SETSEL,0,j);
        SendMessage(hWnd,LB_SETSEL,1,i);
        SetWindowWord(hWnd,GWL_USERDATA+0,i);
        }
      if(hWnd->style & LBS_NOSEL)
        ;
      if(hWnd->style & LBS_EXTENDEDSEL)
        ;
      if(hWnd->style & LBS_NOTIFY) {
        if(hWnd->parent) {
          NMHDR nmh;
          SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,LBN_SELCHANGE),(LPARAM)hWnd);
          nmh.code=LBN_SELCHANGE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // volendo NMHDR ecc
          }
        }
      if(!(hWnd->style & LBS_NOREDRAW))    // solo se cambiata?
        InvalidateRect(hWnd,NULL,TRUE);
      } 
      return 0 /*DefWindowProc(hWnd,message,wParam,lParam)*/;
      break;
    case WM_LBUTTONDBLCLK:
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
          SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);    // finire
          }
        }
      if(!(hWnd->style & LBS_NOREDRAW))    // solo se cambiata?
        InvalidateRect(hWnd,NULL,TRUE);
      break;

    case LB_ADDSTRING:
      {LISTITEM *item,*item2;
      i=0;
      item=item2=(LISTITEM *)GetWindowLong(hWnd,0);
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
      if(hWnd->style & WS_VSCROLL) {
        RECT rc;
        SetScrollRange(hWnd,SB_VERT,0,SendMessage(hWnd,LB_GETCOUNT,0,0),
                !(hWnd->style & LBS_NOREDRAW));   // mah diciamo così redraw
        i=SendMessage(hWnd,LB_GETCURSEL,0,0);
        if(i) {
          GetClientRect(hWnd,&rc);
          rc.bottom /= i*(getFontHeight(&hWnd->font) +1);
          i -= rc.bottom;
          if(i<0)
            i=0;
          }
        SetScrollPos(hWnd,SB_VERT,i,!(hWnd->style & LBS_NOREDRAW));
        }
      if(!(hWnd->style & LBS_NOREDRAW))
        InvalidateRect(hWnd,NULL,TRUE);
      return i;
      }
      break;
    // unire con dlgdir dirdlg??
    case LB_ADDFILE:  // serve qualche campo in più per data, dim, tipo file..
      {LISTITEM *item,*item2;
      i=0;
      item=item2=(LISTITEM *)GetWindowLong(hWnd,0);
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
      SYS_FS_FSTAT stat;
      SYS_FS_HANDLE myFileHandle;
      char *lpPathSpec=(char*)lParam;
      char drive;
      BYTE attr;

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

      switch(drive) {   //
        case 'A':
          if(!SDcardOK) {
no_disc:
            return 0;
            }
          i=FindFirst(lpPathSpec, attr, &rec);
          break;
        case 'B':
          if(!FDOK)
            goto no_disc;
          break;
        case 'C':
          if(!HDOK)
            goto no_disc;
          i=IDEFindFirst(lpPathSpec, attr, &rec);
          break;
        case 'D':
          break;
#ifdef USA_USB_HOST_MSD
        case 'E':
          {
          i=1;
          if((myFileHandle=SYS_FS_DirOpen("/")) != SYS_FS_HANDLE_INVALID) {
            i=SYS_FS_DirSearch(myFileHandle,lpPathSpec,attr,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
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
          i=RAMFindFirst(lpPathSpec, attr, &rec);
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
loop:
        SendMessage(hWnd,LB_ADDSTRING,0,(LPARAM)rec.filename);

        switch(drive) {
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
#ifdef USA_USB_HOST_MSD
          case 'E':
            i=SYS_FS_DirSearch(myFileHandle,lpPathSpec,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
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
        }
      SetWindowWord(hWnd,GWL_USERDATA+0,0);
      SetWindowWord(hWnd,GWL_USERDATA+2,i);
      return 1;
      }
      break;
    case LB_INSERTSTRING:   // no SORT qua!
      {LISTITEM *item,*item2;
      i=0;
      item=item2=(LISTITEM *)GetWindowLong(hWnd,0);
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
      {LISTITEM *item,*item2;
      item2=item=(LISTITEM *)GetWindowLong(hWnd,0);
      while(wParam-- && item) {
        item2=item;
        item=item->next;
        }
      if(item2==(LISTITEM *)GetWindowLong(hWnd,0)) {
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
      {LISTITEM *item,*item2;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
      while(item) {
        item2=item->next;
        free(item);
        item=item2;
       }
      SetWindowLong(hWnd,0,(DWORD)NULL);

      SetWindowWord(hWnd,GWL_USERDATA+0,0);
      SetWindowWord(hWnd,GWL_USERDATA+2,0);

      goto updatelista;
      }
      break;
    case LB_SETSEL:
      {LISTITEM *item,*item2;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
      if(lParam==0xffffffff) {
        while(item) {
          if(wParam)
            item->state |= 1;
          else
            item->state &= ~1;
          item=item->next;
          }
        lParam=0;
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
          }
        else
          return LB_ERR;
        }
      }
      if(wParam)
        SetWindowWord(hWnd,GWL_USERDATA+0,lParam);
      goto updatelista;
      break;
    case LB_GETSEL:
      {LISTITEM *item;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
      item=getListItemFromCnt(item,lParam);
      if(item)
        return item->state & 1;
      else
        return LB_ERR;
      }
      break;
    case LB_GETTEXT:
      {LISTITEM *item;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)item->data;
      else
        return LB_ERR;
      }
      break;
    case LB_GETTEXTLEN:
      {LISTITEM *item;
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)strlen(item->data);
      else
        return LB_ERR;
      }
      break;
    case LB_GETCOUNT:
      {LISTITEM *item;
      i=0;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
      while(item) {
        i++;
        item=item->next;
        }
      SetWindowWord(hWnd,GWL_USERDATA+2,i);
      return i;
      }
      break;
    case LB_GETCURSEL:
      {LISTITEM *item;
      i=0;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
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
    case LB_GETITEMDATA:
      {LISTITEM *item;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
      item=getListItemFromCnt(item,wParam);
      if(item)
        return (DWORD)item->data;
      else
        return LB_ERR;
      }
      break;
    case LB_SETITEMDATA:
      {LISTITEM *item;
      item=(LISTITEM *)GetWindowLong(hWnd,0);
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
      {LISTITEM *item;
      i=0;
      if(wParam==0xffffffff)
        item=(LISTITEM *)GetWindowLong(hWnd,0);
      else
        item=getListItemFromCnt(item,wParam);
      while(item) {
        if(!strnicmp(item->data,(char*)lParam,strlen((char*)lParam))) {
          int j;
          if(message==LB_SELECTSTRING) {
            j=SendMessage(hWnd,LB_GETCURSEL,0,0);
            SendMessage(hWnd,LB_SETSEL,0,j);
            SendMessage(hWnd,LB_SETSEL,0,i);
            SetWindowWord(hWnd,GWL_USERDATA+0,i);
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
      {LISTITEM *item;
      item=getListItemFromCnt(item,wParam);
      if(item)
        return 0;     // finire!
      else
        return LB_ERR;
      }
      break;
    case LB_GETLISTBOXINFO:
      {LISTITEM *item;
      item=getListItemFromCnt(item,wParam);
      if(item)
        return 0;     // finire! items per column
      else
        return LB_ERR;
      }
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
      
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;

      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORBTN*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }

      switch(hWnd->style & (BS_LEFT | BS_RIGHT)) {
        case BS_LEFTTEXT:
    //	Places text on the left side of the radio button or check box when combined with a radio button or check box style. Same as the BS_RIGHTBUTTON style.
        case BS_LEFT:
    //Left-justifies the text in the button rectangle. However, if the button is a check box or radio button that does not have the BS_RIGHTBUTTON style, the text is left justified on the right side of the check box or radio button.
        case BS_TEXT:
        default:
          align= DT_LEFT;
          break;
        case BS_CENTER:
          align= DT_CENTER;
          break;
        case BS_RIGHT:
          align= DT_RIGHT;
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
            default:
              DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,align |
                (hWnd->style & 0 /*BS_NOPREFIX non ce..*/ ? DT_NOPREFIX : 0));
              break;
            case BS_ICON:
              x=0; y=0;
              drawIcon8(hDC,x,y,hWnd->icon);
              break;
            }
          if(hWnd->focus)
            drawHorizLineWindowColor(hDC,ps.rcPaint.left+1,ps.rcPaint.bottom-1,ps.rcPaint.right-1,CYAN);
          break;
          
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          i=ps.rcPaint.bottom-ps.rcPaint.top-2;
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
          hDC->pen.color=hDC->foreColor;
          drawRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
          // USARE DrawFrameControl(hDC,&ps.rcPaint,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+  ));
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              break;
            case BS_TOP:
//              break;
            case BS_BOTTOM:
// x ora              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              break;
            case BS_ICON:
              break;
            }
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY160);
          SetBkColor(hDC,c);
          switch(hWnd->style & 0x0ff0) {
            default:
              DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,align |
                (hWnd->style & 0 /*BS_NOPREFIX non ce..*/ ? DT_NOPREFIX : 0));
              break;
            case BS_ICON:
              x=0; y=0;
              drawIcon8(hDC,x,y,hWnd->icon);
              break;
            }
          if(hWnd->focus)
            ;
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          hDC->pen.color=hWnd->active ? WHITE : GRAY192;
          drawEllipseWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
          // USARE DrawFrameControl(hDC,&ps.rcPaint,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+ ));
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              break;
            case BS_TOP:
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*getFontWidth(&hDC->font))/2,0,hWnd->caption);
              break;
            case BS_BOTTOM:
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*getFontWidth(&hDC->font))/2,ps.rcPaint.bottom-getFontHeight(&hDC->font),hWnd->caption);
              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              break;
            case BS_ICON:
            case BS_OWNERDRAW:
            case BS_USERBUTTON:
              break;
            }
          SetTextColor(hDC,hWnd->enabled ? WHITE : GRAY160);
          SetBkColor(hDC,c);
          switch(hWnd->style & 0x0ff0) {
            default:
              DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,align |
                (hWnd->style & 0 /*BS_NOPREFIX non ce..*/ ? DT_NOPREFIX : 0));
              break;
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
              dis.rcItem=ps.rcPaint;
              dis.CtlType=ODT_BUTTON;
              SendMessage(hWnd->parent,WM_DRAWITEM,(WPARAM)(DWORD)hWnd->menu,(LPARAM)&dis);
              }
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
          DrawFrameControl(hDC,&hWnd->paintArea,0,c);
          // USARE DrawFrameControl(&hWnd->hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+  ));
          break;
        case BS_CHECKBOX:   // BUTTON FINIRE ECC
        case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
          DrawFrameControl(hDC,&hWnd->paintArea,0,c);
          break;
        case BS_RADIOBUTTON:   // BUTTON FINIRE ECC
          DrawFrameControl(hDC,&hWnd->paintArea,0,c);
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
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
      return 0;
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
          SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_DOUBLECLICKED),(LPARAM)hWnd);
        }
      else
        return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    case WM_KEYDOWN:
      break;
    case WM_CHAR:
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
                  SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
                break;
              case VK_RETURN:
                break;
              default:  // hot key??
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
                  if(GetAsyncKeyState(VK_MENU)) {
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
              default:  // hot kbey??
                
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
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
//      SetWindowByte(hWnd,GWL_USERDATA+0 ,cs->style & 0x0f);   // salvo il tipo, anche se in effetti sarebbe anche in style..
      int i;
      char *p;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      SetWindowByte(hWnd,GWL_USERDATA+1,!(hWnd->style & WS_DISABLED));    // stato
      if(p=strchr(hWnd->caption,'&'))
        SetWindowByte(hWnd,GWL_USERDATA+2,p[1]);    // hotkey

      }
      return 0;
      break;
    case WM_SETTEXT:
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_GETDLGCODE: 
      return DLGC_BUTTON | DLGC_WANTALLKEYS | DLGC_WANTCHARS | 
        ((hWnd->style & 0x0f == BS_DEFPUSHBUTTON) ? DLGC_DEFPUSHBUTTON : 0);
      break;

    case BM_SETSTATE:
      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          SetWindowByte(hWnd,GWL_USERDATA+1,wParam);
          InvalidateRect(hWnd,NULL,TRUE);
          if(hWnd->enabled) {
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
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
            SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_SETFOCUS),(LPARAM)hWnd);
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
            SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_KILLFOCUS),(LPARAM)hWnd);
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

LRESULT DefWindowProcStatusWindow(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;
      
      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORSTATIC*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }

      if(hWnd->style & SS_ENDELLIPSIS) {  //SS_PATHELLIPSIS SS_WORDELLIPSIS
        }

			i=GetWindowByte(hWnd,GWL_USERDATA);		// 2bit tipo 2bit stato, x 2
			switch(i & 0b00000011) {
        case 1:     // icona verde-rosso-giallo
          ps.rcPaint.right-=10;
    			switch(i & 0b00001100) {
            case 0:
              break;
            case 1:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTGREEN));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTGREEN));
              goto led1;
            case 2:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTYELLOW));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTYELLOW));
              goto led1;
            case 3:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTRED));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTRED));
led1:
              Ellipse(hDC,ps.rcPaint.right+1,ps.rcPaint.top+1,ps.rcPaint.right+1+8,ps.rcPaint.top+1+8);
              break;
            }
          break;
        case 2:     // ? / !
          ps.rcPaint.right-=8+2;
    			switch(i & 0b00001100) {
            char *p;
            RECT rc={ps.rcPaint.right,ps.rcPaint.top,ps.rcPaint.right+10,ps.rcPaint.bottom};
            case 0:
              break;
            case 1:
              p="?";
              goto text1;
              break;
            case 2:
              p="-";
              goto text1;
              break;
            case 3:
              p="!";
text1:
              DrawText(hDC,p,1,&ps.rcPaint,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        case 3:     // OK / NO
          ps.rcPaint.right-=16+2;
    			switch(i & 0b00001100) {
            char *p;
            RECT rc={ps.rcPaint.right,ps.rcPaint.top,ps.rcPaint.right+18,ps.rcPaint.bottom};
            case 0:
              break;
            case 1:
              p="NO";
              goto text11;
              break;
            case 2:
              p="?";
              goto text11;
              break;
            case 3:
              p="OK";
text11:
              DrawText(hDC,p,-1,&ps.rcPaint,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        default:
          goto skippa_seconda;
          break;
				}
			switch(i & 0b00110000) {
        case 1<<4:
          ps.rcPaint.right-=10;
    			switch(i & 0b11000000) {
            case 0:
              break;
            case 1:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTGREEN));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTGREEN));
              goto led2;
            case 2:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTYELLOW));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTYELLOW));
              goto led2;
            case 3:
              SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,BRIGHTRED));
              SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(LIGHTRED));
led2:
              Ellipse(hDC,ps.rcPaint.right+1,ps.rcPaint.top+1,ps.rcPaint.right+1+8,ps.rcPaint.top+1+8);
            }
          break;
        case 2<<4:
          ps.rcPaint.right-=8+2;
    			switch(i & 0b00001100) {
            char *p;
            RECT rc={ps.rcPaint.right,ps.rcPaint.top,ps.rcPaint.right+10,ps.rcPaint.bottom};
            case 0:
              break;
            case 1:
              p="?";
              goto text2;
              break;
            case 2:
              p="-";
              goto text2;
              break;
            case 3:
              p="!";
text22: 
              DrawText(hDC,p,1,&ps.rcPaint,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        case 3<<4:
          ps.rcPaint.right-=16+2;
    			switch(i & 0b11000000) {
            char *p;
            RECT rc={ps.rcPaint.right,ps.rcPaint.top,ps.rcPaint.right+18,ps.rcPaint.bottom};
            case 0:
              break;
            case 1:
              p="NO";
              goto text22;
              break;
            case 2:
              p="?";
              goto text22;
              break;
            case 3:
              p="OK";
text2:
              DrawText(hDC,p,-1,&ps.rcPaint,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
              break;
            }
          break;
        default:
          break;
				}
skippa_seconda:

      SetTextColor(hDC,GRAY224);
      SetBkColor(hDC,c);
      DrawText(hDC,hWnd->caption,-1,&ps.rcPaint,DT_VCENTER | DT_LEFT |
        DT_SINGLELINE  );
      switch(hWnd->style & SS_TYPEMASK) {
        case SS_ICON:
          drawIcon8(hDC,ps.rcPaint.left,ps.rcPaint.top,hWnd->icon);
          break;
        }

      EndPaint(hWnd,&ps);
      }
      return 0;
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
      DrawFrameControl(hDC,&hWnd->paintArea,0,c);
      // USARE DrawFrameControl(hDC,&rc,DFC_BUTTON,GetWindowByte(hWnd,GWL_USERDATA+0 ma qua non c'è nessun stato cmq));
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i; cs->style |= WS_DISABLED; cs->style &= ~WS_CAPTION;
// qua non uso      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
/*      if((cs->style & 0xff) == SS_ICON)
        cs->cx=cs->cy=8 + (hWnd->style & WS_BORDER ? 2 : 0); // type ecc..
      else
        cs->cy=getFontHeight(&hWnd->font) + (cs->style & WS_BORDER ? 2 : 0); // */
      strncpy(hWnd->caption,cs->lpszName,sizeof(hWnd->caption)-1);  // perché non metto WS_CAPTION
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      if(hWnd->parent) {
        RECT rc;
        GetClientRect(hWnd->parent,&rc);
        cs->x=rc.left;
        cs->y=rc.bottom-11;   // occhio cmq a come viene creata, al primo giro
        cs->cx=rc.right-1;
        cs->cy=10;
        }
			SetWindowLong(hWnd,GWL_USERDATA,0);		// pulisco
      }
      return 0;
      break;
    case WM_SETTEXT:    // gestisco, per gestire ev. 2° pane/icons
      strncpy(hWnd->caption,(const char *)lParam,sizeof(hWnd->caption)-1);
      hWnd->caption[sizeof(hWnd->caption)-1]=0;
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_SETFOCUS:
      return 0;
      break;
    case WM_KILLFOCUS:
      return 0;
      break;

    case WM_MOVE:   // questo non so... 
    case WM_SIZE:   // idem CMQ RIEnTREREBBE da SetWindowPos!! verificare che serve..
      break;
    case WM_CHILDACTIVATE:
reposition:
      if(hWnd->parent) {
        RECT rc;
        GetClientRect(hWnd->parent,&rc);
        SetWindowPos(hWnd,NULL,rc.left,rc.bottom-11,rc.right-1,10,
                SWP_NOSENDCHANGING | SWP_NOZORDER | SWP_NOACTIVATE);
        }
      return 0;
      break;
    case WM_SHOWWINDOW:
      if(lParam==SW_PARENTOPENING)
        goto reposition;
      return 0;
      break;
      
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
      
      if(hWnd->style & PBS_MARQUEE) {
        fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+rand() % i,ps.rcPaint.bottom,    // finire con marquee vero :D
          f);
        }
      else {
        i=GetWindowByte(hWnd,3);
        if(hWnd->style & PBS_CAPTION)
          sprintf(hWnd->caption,"%u%%",i);
        if(hWnd->style & PBS_VERTICAL) {
          i=((ps.rcPaint.bottom-ps.rcPaint.top)*(i-GetWindowByte(hWnd,1))) /
            (GetWindowByte(hWnd,2)-GetWindowByte(hWnd,1));
          fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top+i,ps.rcPaint.left,ps.rcPaint.bottom,
            f);
          }
        else {
          i=((ps.rcPaint.right-ps.rcPaint.left)*(i-GetWindowByte(hWnd,1))) /
            (GetWindowByte(hWnd,2)-GetWindowByte(hWnd,1));
          fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.bottom,
            f);
          }
        if(hWnd->style & PBS_CAPTION) {
          SetTextColor(hDC,WHITE /*textColors[GetWindowByte(hWnd,4) & 15]*/);
          SetBkColor(hDC,b);
          DrawText(hDC,hWnd->caption,-1,&ps.rcPaint, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
          }
        }

      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      GFX_COLOR b=GetWindowWord(hWnd,GWL_USERDATA+2);
      DrawFrameControl(hDC,&hWnd->paintArea,0,b);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      cs->style |= WS_DISABLED;   // direi
      if(cs->style & WS_CAPTION)
        cs->style |= PBS_CAPTION;   //:)
      cs->style &= ~WS_CAPTION;
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

LRESULT DefWindowProcSpinWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
#define ARROW_SIZE 5
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      RECT rc=ps.rcPaint;
      BYTE size=8 /*getFontWidth(&hDC->font) meglio*/;
      
      SetTextColor(hDC,hWnd->enabled ? windowForeColor : windowInactiveForeColor);
      SetBkColor(hDC,windowBackColor);
      SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,hWnd->enabled ? windowForeColor : windowInactiveForeColor));
      ps.rcPaint.right--;
      ps.rcPaint.bottom--;
          
      if(hWnd->style & UDS_HORZ) {
        rc.top+=1; rc.bottom-=getFontHeight(&hDC->font);
        drawLineWindow(hDC,ps.rcPaint.left+3,ps.rcPaint.bottom-(getFontHeight(&hDC->font))/2-1,
                ps.rcPaint.left+(ps.rcPaint.right-ps.rcPaint.left)/2+2,ps.rcPaint.bottom-(getFontHeight(&hDC->font))+1);
        drawLineWindow(hDC,ps.rcPaint.left+(ps.rcPaint.right-ps.rcPaint.left)/2+2,ps.rcPaint.bottom-2,
                ps.rcPaint.left+3,ps.rcPaint.bottom-(getFontHeight(&hDC->font))/2-1);
        drawRectangleWindow(hDC,ps.rcPaint.left,rc.bottom,ps.rcPaint.right,rc.bottom+getFontHeight(&hDC->font));
        // FINIRE!
        }
      else {
        if(hWnd->style & UDS_ALIGNRIGHT) {    // frecce a dx, testo a sx
          rc.left += 1;
          rc.right -= (size+2);   // uso dim testo per largh frecce :)
          drawRectangleWindow(hDC,rc.right+1,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
          drawLineWindow(hDC,ps.rcPaint.right-1,ps.rcPaint.top+ARROW_SIZE,
                ps.rcPaint.right-(size)/2,ps.rcPaint.top+1);
          drawLineWindow(hDC,ps.rcPaint.right-(size)/2-1,ps.rcPaint.top+1,
                ps.rcPaint.right-(size)+1,ps.rcPaint.top+ARROW_SIZE);
          drawHorizLineWindow(hDC,rc.right+1,(ps.rcPaint.bottom-ps.rcPaint.top)/2,
                ps.rcPaint.right);
          drawLineWindow(hDC,ps.rcPaint.right-1,ps.rcPaint.bottom-ARROW_SIZE,
                ps.rcPaint.right-(size)/2,ps.rcPaint.bottom-1);
          drawLineWindow(hDC,ps.rcPaint.right-(size)/2-1,ps.rcPaint.bottom-1,
                ps.rcPaint.right-(size)+1,ps.rcPaint.bottom-ARROW_SIZE);
          }
        else if(hWnd->style & UDS_ALIGNLEFT) {    // frecce a sx, testo a dx
          rc.right -= 1;
          rc.left += (size+1);    // uso dim testo per largh frecce :)
          drawRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,rc.left,ps.rcPaint.bottom);
          drawLineWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+ARROW_SIZE,
                ps.rcPaint.left+(size)/2,ps.rcPaint.top+1);
          drawLineWindow(hDC,ps.rcPaint.left+(size)/2-1,ps.rcPaint.top+1,
                ps.rcPaint.left+(size)-1,ps.rcPaint.top+ARROW_SIZE);
          drawHorizLineWindow(hDC,ps.rcPaint.left,(ps.rcPaint.bottom-ps.rcPaint.top)/2,
                ps.rcPaint.left+(size)-1);
          drawLineWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.bottom-ARROW_SIZE,
                ps.rcPaint.left+(size)/2,ps.rcPaint.bottom-1);
          drawLineWindow(hDC,ps.rcPaint.left+(size)/2-1,ps.rcPaint.bottom-1,
                ps.rcPaint.left+(size)-1,ps.rcPaint.bottom-ARROW_SIZE);
          }
        }
      if(hWnd->style & UDS_NOTHOUSANDS) {
        }
      sprintf(hWnd->caption,"%u",GetWindowByte(hWnd,GWL_USERDATA+3));
      DrawText(hDC,hWnd->caption,-1,&rc,
              DT_VCENTER | (hWnd->style & UDS_ALIGNRIGHT ? DT_RIGHT : DT_LEFT) | DT_SINGLELINE);
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      // COLORE CTLCOLOR qua o no?? bah sì, direi..
      DrawFrameControl(hDC,&hWnd->paintArea,0,windowBackColor);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      SetWindowLong(hWnd,GWL_USERDATA,0);
      SetWindowByte(hWnd,GWL_USERDATA+0,1);   // step
      }
      return 0;
      break;
    case WM_CHAR:
      {int8_t i=GetWindowByte(hWnd,GWL_USERDATA+3);
      int8_t m=GetWindowByte(hWnd,GWL_USERDATA+1),M=GetWindowByte(hWnd,GWL_USERDATA+2);
      int8_t step=GetWindowByte(hWnd,GWL_USERDATA+0);
      switch(wParam) {
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
            SetWindowByte(hWnd,GWL_USERDATA+3,i);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent) {
              NMHDR nmh;
//              nmh.code=UDN_CLK;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
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
            SetWindowByte(hWnd,GWL_USERDATA+3,i);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent) {
              NMHDR nmh;
//              nmh.code=UDN_CLK;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
              }
            }
          break;
        case VK_ESCAPE:   // boh?
          SetWindowByte(hWnd,GWL_USERDATA+3,m);
          InvalidateRect(hWnd,NULL,TRUE);
          if(hWnd->parent) {
            NMHDR nmh;
//              nmh.code=UDN_CLK;
            nmh.hwndFrom=hWnd;
            nmh.idFrom=(DWORD)hWnd->menu;
            SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
            }
          break;
        }
      }
      break;
    case WM_LBUTTONDOWN:
      {int8_t i=GetWindowByte(hWnd,GWL_USERDATA+3);
      int8_t m=GetWindowByte(hWnd,GWL_USERDATA+1),M=GetWindowByte(hWnd,GWL_USERDATA+2);
      int8_t step=GetWindowByte(hWnd,GWL_USERDATA+0);
      int8_t t=0;
      RECT rc;
      POINT pt={LOWORD(lParam),HIWORD(lParam)};
      GetClientRect(hWnd,&rc);
      if(hWnd->style & UDS_HORZ) {
        rc.top = rc.bottom-getFontHeight(&hWnd->font);
        rc.right /= 2;
        if(PtInRect(&rc,pt))
          t=-1;
        rc.left=rc.right; rc.right *= 2;
        if(PtInRect(&rc,pt))
          t=1;
        }
      else {
        if(hWnd->style & UDS_ALIGNRIGHT) {    // frecce a dx, testo a sx
          rc.left = rc.right-getFontWidth(&hWnd->font);
          rc.bottom /= 2;
          if(PtInRect(&rc,pt))
            t=1;
          rc.top=rc.bottom; rc.bottom *= 2;
          if(PtInRect(&rc,pt))
            t=-1;
          }
        else if(hWnd->style & UDS_ALIGNLEFT) {    // frecce a sx, testo a dx
          rc.right = rc.left-getFontWidth(&hWnd->font);
          rc.bottom /= 2;
          if(PtInRect(&rc,pt))
            t=1;
          rc.top=rc.bottom; rc.bottom *= 2;
          if(PtInRect(&rc,pt))
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
      SetWindowByte(hWnd,GWL_USERDATA+3,i);
      InvalidateRect(hWnd,NULL,TRUE);
      if(hWnd->parent) {
        NMHDR nmh;
//              nmh.code=UDN_CLK;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
        }
      }
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
#define THUMBLENGTH (size/2)
#define THUMBWIDTH (4)
      
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      uint8_t pxPerTicks;

      SetTextColor(hDC,hWnd->enabled ? windowForeColor : windowInactiveForeColor);
      SetBkColor(hDC,windowBackColor);
      SelectObject(hDC,OBJ_PEN,(GDIOBJ)CreatePen(PS_SOLID,1,hWnd->enabled ? windowForeColor : windowInactiveForeColor));
      ps.rcPaint.left++;    // -(THUMBWIDTH) ... a seconda di horz o vert..
      ps.rcPaint.top++;
      ps.rcPaint.right--;
      ps.rcPaint.bottom--;
          
      if(hWnd->style & TBS_VERT) {
        size=ps.rcPaint.right-ps.rcPaint.left;
        if(step && M>m)
          pxPerTicks=(ps.rcPaint.bottom-ps.rcPaint.top-(THUMBWIDTH-2))/((M-m)/step);
        fillRectangleWindow(hDC,ps.rcPaint.left+(size/2)-1,ps.rcPaint.top+1,
                ps.rcPaint.left+(size/2)+1,ps.rcPaint.bottom);
        if(!(hWnd->style & TBS_LEFT) || hWnd->style & TBS_BOTH) {    // 
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=M; i>=m; i-=step)
                drawHorizLineWindow(hDC,(ps.rcPaint.right-ps.rcPaint.left)/2,ps.rcPaint.top+(i-m)*pxPerTicks,
                  ps.rcPaint.right-1);
              }
            else {  // fare...
              }
            }
          }
        if(hWnd->style & TBS_LEFT || hWnd->style & TBS_BOTH) {    // 
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=M; i>=m; i-=step)
                drawHorizLineWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+(i-m)*pxPerTicks,
                  (ps.rcPaint.right-ps.rcPaint.left)/2);
              }
            else {  // fare...
              }
            }
          }
        if(!(hWnd->style & TBS_NOTHUMB)) {
          fillRectangleWindow(hDC,
            ps.rcPaint.left+THUMBLENGTH/2,
            ps.rcPaint.bottom-(val-m)*pxPerTicks-THUMBWIDTH/2+THUMBWIDTH/2,
            ps.rcPaint.right-THUMBLENGTH/2,
            ps.rcPaint.bottom-(val-m)*pxPerTicks+THUMBWIDTH/2+THUMBWIDTH/2);
          }
        }
      else {
        size=ps.rcPaint.bottom-ps.rcPaint.top;
        if(step && M>m)
          pxPerTicks=(ps.rcPaint.right-ps.rcPaint.left-(THUMBWIDTH-2))/((M-m)/step);
        else
          pxPerTicks=0;
        fillRectangleWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+(size/2)-1,
                ps.rcPaint.right,ps.rcPaint.top+(size/2)+1);
        if(!(hWnd->style & TBS_TOP) || hWnd->style & TBS_BOTH) {    // COGLIONI! BOTTOM è 0
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=m; i<=M; i+=step)
                drawVertLineWindow(hDC,ps.rcPaint.left+(i-m)*pxPerTicks,(ps.rcPaint.bottom-ps.rcPaint.top)/2,
                  ps.rcPaint.bottom-1);
              }
            else {  // fare...
              }
            }
          }
        if(hWnd->style & TBS_TOP || hWnd->style & TBS_BOTH) {    // 
          if(!(hWnd->style & TBS_NOTICKS)) {
            if(hWnd->style & TBS_AUTOTICKS) {
              for(i=m; i<=M; i+=step) 
                drawVertLineWindow(hDC,ps.rcPaint.left+(i-m)*pxPerTicks,ps.rcPaint.top+1,
                  (ps.rcPaint.bottom-ps.rcPaint.top)/2);
              }
            else {  // fare...
              }
            }
          }
        if(!(hWnd->style & TBS_NOTHUMB)) {
          fillRectangleWindow(hDC,
            ps.rcPaint.left+(val-m)*pxPerTicks-THUMBWIDTH/2+THUMBWIDTH/2-1,
            ps.rcPaint.top+THUMBLENGTH/2,
            ps.rcPaint.left+(val-m)*pxPerTicks+THUMBWIDTH/2+THUMBWIDTH/2-1,
            ps.rcPaint.bottom-THUMBLENGTH/2);
          }
        }
      
      EndPaint(hWnd,&ps);
      }
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      // COLORE CTLCOLOR qua o no?? bah sì, direi..
      DrawFrameControl(hDC,&hWnd->paintArea,0,windowBackColor);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
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
    case WM_CHAR:
      switch(wParam) {
        case VK_LEFT:     // 
          if(!(hWnd->style & TBS_VERT)) {
            if(val >= m-step)
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_LINELEFT,0),(DWORD)hWnd);
            }
          break;
        case VK_UP:
          if(hWnd->style & TBS_VERT) {
            if(val <= M+step)   // 
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_LINEUP,0),(DWORD)hWnd);
            }
          break;
        case VK_RIGHT:     // TB_LINEDOWN
          if(!(hWnd->style & TBS_VERT)) {
            if(val <= M-step)
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_LINERIGHT,0),(DWORD)hWnd);
            }
          break;
        case VK_DOWN:
          if(hWnd->style & TBS_VERT) {
            if(val >= m+step)   // 
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_LINEDOWN,0),(DWORD)hWnd);
            }
          break;
        case VK_HOME:    // TB_TOP
          if(hWnd->style & TBS_VERT)    // beh sì :)
            SetWindowByte(hWnd,GWL_USERDATA+3,M);
          else
            SetWindowByte(hWnd,GWL_USERDATA+3,m);
          if(hWnd->style & TBS_VERT) {
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_TOP,0),(DWORD)hWnd);
            }
          else {
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_LEFT,0),(DWORD)hWnd);
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
              SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_BOTTOM,0),(DWORD)hWnd);
            }
          else {
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_TOP,0),(DWORD)hWnd);
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
              SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_PAGEDOWN,0),(DWORD)hWnd);
            }
          else {
            if(val <= M-step)
              val += step;
            else
              val=M;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_PAGEDOWN,0),(DWORD)hWnd);
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
              SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_PAGEUP,0),(DWORD)hWnd);
            }
          else {
            if(val >= m+step)   // 
              val -= step;
            else
              val=m;
            SetWindowByte(hWnd,GWL_USERDATA+3,val);
            if(hWnd->parent)
              SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_PAGEUP,0),(DWORD)hWnd);
            }
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        }
      if(hWnd->parent) {
        NMHDR nmh;
        nmh.code=WM_VSCROLL;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // e verificare con WM_scroll
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

        if(pt.y > (DWORD)(val-m)*pxPerTicks) {
          if(val >= m+step)   // 
            val -= step;
          else
            val=m;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_PAGEDOWN,0),(DWORD)hWnd);
          //NMHDR sotto...
          }
        else {
          if(val <= M+step)   // 
            val += step;
          else
            val=M;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            SendMessage(hWnd->parent,WM_VSCROLL,MAKEWORD(SB_PAGEUP,0),(DWORD)hWnd);
          }
        }
      else {
        if(step && M>m)
          pxPerTicks=(rc.right-rc.left-THUMBWIDTH)/((M-m)/step);

        if(pt.x > (DWORD)(val-m)*pxPerTicks) {
          if(val <= M+step)   // 
            val += step;
          else
            val=M;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_PAGERIGHT,0),(DWORD)hWnd);
          }
        else {
          if(val >= m+step)   // 
            val -= step;
          else
            val=m;
          SetWindowByte(hWnd,GWL_USERDATA+3,val);
          if(hWnd->parent)
            SendMessage(hWnd->parent,WM_HSCROLL,MAKEWORD(SB_PAGELEFT,0),(DWORD)hWnd);
          }
        }
      InvalidateRect(hWnd,NULL,TRUE);
      if(hWnd->parent) {
        NMHDR nmh;
        nmh.code=WM_VSCROLL;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
        }
      }
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

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }



#define X_SPACING_LARGEICONS 60
#define Y_SPACING_LARGEICONS 40
#define X_SPACING_SMALLICONS 40
#define Y_SPACING_SMALLICONS 32
#define Y_SPACING_DETAILS 8

LRESULT DefWindowProcFileDlgWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      SearchRec rec;
      SYS_FS_FSTAT stat;
      SYS_FS_HANDLE myFileHandle;
      int i;
      uint32_t sn;
      UGRAPH_COORD_T x,y;
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
      char label[16 /*TOTAL_FILE_SIZE+1*/];
      WORD totFiles;
      
      HDC hDC=BeginPaint(hWnd,&ps);

      if(!GetWindowByte(hWnd,GWL_USERDATA+2))
        fillRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
      totFiles=0;

      switch(ofn->disk) {
        case 'A':
          if(!SDcardOK) {
no_disc:
            SetWindowByte(hWnd,GWL_USERDATA+2,1);
            SetWindowText(hWnd,"Disco");
            char buf[32];
            sprintf(buf,"Inserire disco nell'unità %c:",ofn->disk);
            DrawText(hDC,buf,-1,&ps.rcPaint,DT_CENTER | DT_VCENTER);
            goto fine;
            }
          FSgetVolume(label,&sn);
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
          IDEFSgetVolume(label,&sn);
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
          RAMFSgetVolume(label,&sn);
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
        if(GetWindowByte(hWnd,GWL_USERDATA+2)<2)
          SetWindowByte(hWnd,GWL_USERDATA+2,2);
        
loop:
					totFiles++;
          if(y<ps.rcPaint.bottom) {
            switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
              RECT rc;
              case 0:     // icone piccole
                hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font;
                rc.left=ps.rcPaint.left+4; rc.top=y+4+8+2; rc.right=ps.rcPaint.right-4; rc.bottom=ps.rcPaint.bottom-1;
                DrawText(hDC,rec.filename,-1,&rc,DT_TOP | DT_CENTER);
                if(rec.attributes & ATTR_DIRECTORY) {
                  drawIcon8(hDC,x+18,y+4,folderIcon8);
                  }
                else {
                  drawIcon8(hDC,x+18,y+4,fileIcon8);
                  }
                x+=X_SPACING_SMALLICONS;
                if(x>=ps.rcPaint.right) {
                  x=0;
                  y+=Y_SPACING_SMALLICONS;
                  }
                break;
              case 1:     // icone grandi
                rc.left=ps.rcPaint.left+4; rc.top=y+4+16+2; rc.right=ps.rcPaint.right-4; rc.bottom=ps.rcPaint.bottom-1;
                DrawText(hDC,rec.filename,-1,&rc,DT_TOP | DT_CENTER);
                if(rec.attributes & ATTR_DIRECTORY) {
                  DrawIcon(hDC,x+25,y+4,folderIcon);
                  }
                else {
                  DrawIcon(hDC,x+25,y+4,fileIcon);
                  }
                x+=X_SPACING_LARGEICONS;
                if(x>=ps.rcPaint.right) {
                  x=0;
                  y+=Y_SPACING_LARGEICONS;
                  }
                break;
              case 2:     // dettagli
//                rec.filename[16]=0; INUTILE nomi file corti :)
                TextOut(hDC,x+2,y+2,rec.filename); 
                if(rec.attributes & ATTR_DIRECTORY) {
                  TextOut(hDC,x+13*6+2,y+2,"DIR");
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

        switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
          case 0:
            if(((totFiles*32)/Y_SPACING_SMALLICONS)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*32)/Y_SPACING_SMALLICONS),FALSE);
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
            if(((totFiles*X_SPACING_LARGEICONS)/Y_SPACING_LARGEICONS)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*X_SPACING_LARGEICONS)/Y_SPACING_LARGEICONS),FALSE);
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
            if((totFiles*8)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / (totFiles*8),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            if((40*6)>(ps.rcPaint.right-ps.rcPaint.left)) {   // v. testo sopra
              hWnd->style |= WS_HSCROLL;
              SetScrollRange(hWnd,SB_HORZ,0,(ps.rcPaint.right-ps.rcPaint.left)*(ps.rcPaint.right-ps.rcPaint.left) / (40*6),FALSE);
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

      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        nmh.code=CDN_INITDONE;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // beh tanto per :) anche se è per file dialog chooser
        }

fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
      if(!GetWindowByte(hWnd,GWL_USERDATA+2)) {
        DrawText(hDC,"attendere prego...",-1,&hWnd->paintArea,DT_CENTER | DT_VCENTER);
        //e fare magari pure un Mount o FSinit..
        }
          
//          drawIcon8(&hWnd->hDC,0,16,folderIcon);
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      // forzare font piccolo??
      int i;

      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
			memset(ofn,sizeof(OPENFILENAME),0);
      }
      return 0;
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_SPACE:
          if(hWnd->parent && !hWnd->parent->internalState) {
            NMHDR nmh;
            nmh.code=CDN_FILEOK;
            nmh.hwndFrom=hWnd;
            nmh.idFrom=(DWORD)hWnd->menu;
            SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // beh tanto per :) anche se è per file dialog chooser
            }
          // in effetti sa
          break;
        case 'i':
        case 'I':
          {NMHDR nmh;
          nmh.code=CDN_TYPECHANGE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          SendMessage(hWnd,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh);
          }
          break;
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }

      break;

    case WM_LBUTTONDOWN:
        if(hWnd->parent && !hWnd->parent->internalState) {
          NMHDR nmh;
          nmh.code=NM_LDOWN;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
          }
      break;
    case WM_LBUTTONDBLCLK:
        if(hWnd->parent && !hWnd->parent->internalState) {
          NMHDR nmh;
          nmh.code=NM_DBLCLK;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
          SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
          }
      break;
      
    case WM_NOTIFY:
      switch(lParam) {
        case CDN_TYPECHANGE:
          {
          OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,0);
          SetWindowByte(hWnd,GWL_USERDATA+1,(GetWindowByte(hWnd,GWL_USERDATA+1)+1) % 3);
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        }
      break;

    case WM_FILECHANGE:
      break;
      
    default:
//      return DefWindowProc(hWnd,message,wParam,lParam);
      return DefDlgProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }


typedef struct __attribute((packed)) _XLISTITEM {
    char filename[20];
    S_RECT rc;
//    struct _XLISTITEM *prev;   // volendo :)
    struct _XLISTITEM *next;
    BYTE disk;
    BYTE type;
    BYTE state;     // b0=selected
} XLISTITEM;

XLISTITEM *insertDiskItem(XLISTITEM **root) {
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
static XLISTITEM *fillDiskList(char disk,char *path) {
  SYS_FS_FSTAT stat;
  SYS_FS_HANDLE myFileHandle;
  SearchRec rec;
  int i; 
  BYTE j;
  char label[16 /*TOTAL_FILE_SIZE+1*/];
  WORD totFiles;
  XLISTITEM *item=NULL,*item2;
  
  *label=0;
  totFiles=0;
  switch(disk) {
    uint32_t sn;
    case 'A':
      if(!SDcardOK) {
no_disc:
        goto fine;
        }
      FSgetVolume(label,&sn);
      if(*path)
        FSchdir(path);    // va fatto una per una... o creare funzione nel file sistem...
      else
        FSchdir("\\");
      i=FindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
      break;
    case 'B':
      if(!FDOK)
        goto no_disc;
      break;
    case 'C':
      if(!HDOK)
        goto no_disc;
      IDEFSgetVolume(label,&sn);
      if(*path)
        IDEFSchdir(path);
      else
        IDEFSchdir("\\");
      i=IDEFindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
      break;
    case 'D':
      break;
#if defined(USA_USB_HOST_MSD)
    case 'E':
      if(!USBmemOK) 
        goto no_disc;
      uint32_t timestamp=0;
      i=SYS_FS_DriveLabelGet(NULL, label, &sn, &timestamp);
      i=1;
      if((myFileHandle=SYS_FS_DirOpen("/")) != SYS_FS_HANDLE_INVALID) {
        i=SYS_FS_DirSearch(myFileHandle,*path ? path : ASTERISKS,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
        if(!i) {
          strcpy(rec.filename,stat.fname);
          rec.attributes=stat.fattrib;
          rec.timestamp=MAKELONG(stat.fsize,stat.ftime);    // VERIFICARE!
          }
        }
      break;
#endif
#ifdef USA_RAM_DISK 
    case DEVICE_RAMDISK:
      if(!RAMdiscArea)
        goto no_disc;
      RAMFSgetVolume(label,&sn); 
      if(*path)
        RAMFSchdir(path);
      else
        RAMFSchdir("\\");
      i=RAMFindFirst(ASTERISKS, ATTR_MASK ^ ATTR_VOLUME, &rec);
      break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
    case DEVICE_NETWORK:
      break;
#endif
    case 0:		// elenco dischi
      i=0;		// lo uso come selezionatore disco, da 'A' a 'Z'
      break;
    default:
      goto no_disc;
      break;
    }

  if(disk) {
    item2=insertDiskItem(&item);
    strncpy(item2->filename,label,sizeof(item2->filename)-1);
    item2->filename[sizeof(item2->filename)-1]=0;
    item2->type=ATTR_VOLUME;
    item2->state=1; item2->disk=disk;
    } 
  
  if(!i) {

loop:
    totFiles++;
    
    if(disk) {
      item2=insertDiskItem(&item);
      strncpy(item2->filename,rec.filename,sizeof(item2->filename)-1);
      item2->filename[sizeof(item2->filename)-1]=0;
      item2->type=rec.attributes;
      item2->state=0; item2->disk=disk;
      }
    switch(disk) {
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
        i=SYS_FS_DirSearch(myFileHandle,*path ? path : ASTERISKS,SYS_FS_ATTR_MASK ^ SYS_FS_ATTR_VOL,&stat) == SYS_FS_RES_SUCCESS ? 0 : 1;
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
      case 0:		// elenco dischi
        *label=0; j=0;
        switch(i+'A') {
          case 'A':
            if(SDcardOK) {
              uint32_t n;
              FSgetVolume(label,&n);
              j=1;
              totFiles++;
              }
            break;
          case 'B':
            break;
          case 'C':
            if(HDOK) {
              uint32_t n;
              IDEFSgetVolume(label,&n);
              j=1;
              totFiles++;
              }
            break;
          case 'D':
            break;
#if defined(USA_USB_HOST_MSD)
          case 'E':
            {
            uint32_t sn,timestamp;
            if(USBmemOK) {
              SYS_FS_DriveLabelGet(NULL, label, &sn, &timestamp);
              j=1;
              totFiles++;
              }
            }
            break;
#endif
#ifdef USA_RAM_DISK 
          case DEVICE_RAMDISK:
            if(RAMdiscArea) {
              uint32_t n;
              RAMFSgetVolume(label,&n);
              j=1;
              totFiles++;
              }
            break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          case DEVICE_NETWORK:
              totFiles++;
            break;
#endif
          default:
            break;
          }
        if(j) {
          item2=insertDiskItem(&item);
          strncpy(item2->filename,label,sizeof(item2->filename)-1);
          item2->filename[sizeof(item2->filename)-1]=0;
          
          item2->state=0; item2->disk=i+'A';
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
  
  rc2.left=rc->left;
  rc2.top=rc->top;
  while(root) {
    switch(tipoVis) {
      case 0:
        rc2.right=rc2.left+X_SPACING_SMALLICONS-1;
        rc2.bottom=rc2.top+Y_SPACING_SMALLICONS-1;
        break;
      case 1:
        rc2.right=rc2.left+X_SPACING_LARGEICONS-1;
        rc2.bottom=rc2.top+Y_SPACING_LARGEICONS-1;
        break;
      case 2:
        rc2.right=rc->right-1;
        rc2.bottom=rc2.top+Y_SPACING_DETAILS-1;
        break;
      }
    if(PtInRect(&rc2,ptl))
      break;
    switch(tipoVis) {
      case 0:
        rc2.left=rc2.left+X_SPACING_SMALLICONS;
        if(rc2.left>=rc->right) {
          rc2.left=rc->left;
          rc2.top+=Y_SPACING_SMALLICONS;
          }
        break;
      case 1:
        rc2.left=rc2.left+X_SPACING_LARGEICONS;
        if(rc2.left>=rc->right) {
          rc2.left=rc->left;
          rc2.top+=Y_SPACING_LARGEICONS;
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
        x+=X_SPACING_SMALLICONS;
        if(x>=rc->right) {
          x=0;
          y+=Y_SPACING_SMALLICONS;
          }
        break;
      case 1:
        // è diversa se dischi! gestire
        x+=X_SPACING_LARGEICONS;
        if(x>=rc->right) {
          x=0;
          y+=Y_SPACING_LARGEICONS;
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

LRESULT DefWindowProcDirWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int i; 
      UGRAPH_COORD_T x,y;
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      WORD totFiles;
      XLISTITEM *item;
      
      HDC hDC=BeginPaint(hWnd,&ps);
      
      if(!GetWindowByte(hWnd,GWL_USERDATA+2))
        fillRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
      
      totFiles=0;
      item=(XLISTITEM*)GetWindowLong(hWnd,sizeof(DIRLIST));
      if(GetWindowByte(hWnd,GWL_USERDATA+2)<2) {
        freeDiskList(item);
        item=fillDiskList(dfn->disk,dfn->path);
        SetWindowLong(hWnd,sizeof(DIRLIST),(DWORD)item);
        SetWindowByte(hWnd,GWL_USERDATA+2,2);
  			if(hWnd->parent && !hWnd->parent->internalState) {
          NMHDR nmh;
          nmh.code=CDN_INITDONE;
          nmh.hwndFrom=hWnd;
          nmh.idFrom=(DWORD)hWnd->menu;
    			SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // beh tanto per :) anche se è per file dialog chooser
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
              switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
                RECT rc;
                case 0:     // icone piccole
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_SMALLICONS-3-3; item->rc.bottom=item->rc.top+y+3+4+8+8+8+2;
                  rc.left=item->rc.left+1; rc.top=item->rc.top+1+8+2; rc.right=item->rc.right-1; rc.bottom=item->rc.bottom-1;
                  DrawText(hDC,item->filename,-1,&rc,DT_TOP | DT_CENTER);
                  if(item->type & ATTR_DIRECTORY) {
                    drawIcon8(hDC,x+18,y+4,folderIcon8);
                    }
                  else {
                    drawIcon8(hDC,x+18,y+4,fileIcon8);
                    }
                  if(item->state) {
                    drawRectangleWindow(hDC,x+2,y,x+X_SPACING_SMALLICONS-1,y+24+2);
                    }
                  x+=X_SPACING_SMALLICONS;
                  if(x>=ps.rcPaint.right) {
                    x=0;
                    y+=Y_SPACING_SMALLICONS;
                    }
                  break;
                case 1:     // icone grandi
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_LARGEICONS-3-3; item->rc.bottom=item->rc.top+y+3+4+16+8+8+2;
                  rc.left=item->rc.left+1; rc.top=item->rc.top+1+16+2; rc.right=item->rc.right-1; rc.bottom=item->rc.bottom-1;
                  DrawText(hDC,item->filename,-1,&rc,DT_TOP | DT_CENTER);
                  if(item->type & ATTR_DIRECTORY) {
                    DrawIcon(hDC,x+25,y+4,folderIcon);
                    }
                  else {
                    DrawIcon(hDC,x+25,y+4,fileIcon);
                    }
// in effetti il rettangolo per evidenziare dipende dal testo a capo o no.. farlo restituire da DrawText con flag??
                  if(item->state) {
                    drawRectangleWindow(hDC,x+1,y+1,x+X_SPACING_LARGEICONS-1,y+32+5);
                    }
                  x+=X_SPACING_LARGEICONS;
                  if(x>=ps.rcPaint.right) {
                    x=0;
                    y+=Y_SPACING_LARGEICONS;
                    }
                  break;
                case 2:     // dettagli
                  item->rc.left=x+1; item->rc.top=y+1;
                  item->rc.right=ps.rcPaint.right-1; item->rc.bottom=item->rc.top+y+8+1;
  //                item->filename[16]=0; INUTILE nomi file corti :)
                  TextOut(hDC,x+2,y+2,item->filename); 
                  if(item->type & ATTR_DIRECTORY) {
                    TextOut(hDC,x+13*6+2,y+2,"DIR");
                    }
                  else {
                    char buf[32];
                    struct FSstat st;
                    SYS_FS_FSTAT fsst;
                    switch(dfn->disk) {
                      case 'A':
                        FSstat(item->filename,&st);
                        break;
                      case 'C':
                        IDEFSstat(item->filename,&st);
                        break;
                      case 'E':
                        SYS_FS_FileStat(item->filename,&fsst);
                        st.st_mtime=MAKELONG(fsst.ftime,fsst.fdate);
                        st.st_size=fsst.fsize;
                        break;
#ifdef USA_RAM_DISK 
                      case DEVICE_RAMDISK:
                        RAMFSstat(item->filename,&st);
                        break;
#endif
                      }
                    sprintf(buf,"%u",st.st_size);
                    TextOut(hDC,x+13*6,y+2,buf);
                    sprintf(buf,"%02u/%02u/%04u %02u:%02u:%02u",
                      (st.st_mtime >> 16) & 31,
                      (st.st_mtime >> (5+16)) & 15,
                      (st.st_mtime >> (9+16)) + 1980,
                      (st.st_mtime >> 11) & 31,
                      (st.st_mtime >> 5) & 63,
                      st.st_mtime & 63);
                    TextOut(hDC,x+(14+10)*6,y+2,buf);
                    }
                  if(item->state) {
                    drawRectangleWindow(hDC,x,y,ps.rcPaint.right-1,y+9);
                    }
                  y+=8;
                  break;
                }
              }
            }
          if(!dfn->disk) {		// elenco dischi, v. anche item->state=0xff
            dfn->fsdp.new_request=1;
            switch(item->disk) {
              case 'A':
                if(SDcardOK) {
                  do {
                    FSGetDiskProperties(&dfn->fsdp);
                    ClrWdt();
                    } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
                  totFiles++;
                  }
                break;
              case 'B':
                break;
              case 'C':
                if(HDOK) {
                  do {
                    IDEFSGetDiskProperties(&dfn->fsdp);
                    ClrWdt();
                    } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
                  totFiles++;
                  }
                break;
              case 'D':
                break;
#if defined(USA_USB_HOST_MSD)
              case 'E':
                {
                uint32_t freeSectors,totalSectors,sectorSize,sn,timestamp;
                if(USBmemOK) {
                  if(SYS_FS_DriveSectorGet(NULL,&totalSectors,&freeSectors,&sectorSize)==SYS_FS_RES_SUCCESS) {
                    dfn->fsdp.results.free_clusters=freeSectors;
                    dfn->fsdp.results.sectors_per_cluster=1;
                    dfn->fsdp.results.sector_size=MEDIA_SECTOR_SIZE; 
                    }
                  dfn->fsdp.new_request=0;
                  totFiles++;
                  }
                }
                break;
#endif
#ifdef USA_RAM_DISK 
              case DEVICE_RAMDISK:
                if(RAMdiscArea) {
                  do {
                    RAMFSGetDiskProperties(&dfn->fsdp);
                    ClrWdt();
                    } while(dfn->fsdp.properties_status == FS_GET_PROPERTIES_STILL_WORKING);
                  totFiles++;
                  }
                break;
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
              case DEVICE_NETWORK:
                  totFiles++;
                break;
#endif
              default:
                break;
              }
            if(!dfn->fsdp.new_request) {
              switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
                RECT rc;
                case 0:     // icone piccole
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_SMALLICONS-3-3; item->rc.bottom=item->rc.top+y+3+4+8+8+8+2;
                  drawIcon8(hDC,x+   getFontWidth(&hDC->font)*4,y+4,diskIcon8);
                  rc.left=item->rc.left+1; rc.top=item->rc.top+1+8+2; rc.right=item->rc.right-1; rc.bottom=item->rc.bottom-1;
                  {char buf[2]={item->disk,0};
                  DrawText(hDC,*item->filename ? item->filename : buf,-1,&rc,DT_TOP | DT_CENTER);
                  }
                  if(item->state) {
                    drawRectangleWindow(hDC,x+1,y+1,x+39,y+31);
                    }
                  x+=40;
                  if(x>=ps.rcPaint.right) {
                    x=0;
                    y+=32;
                    }
                  break;
                case 1:     // icone grandi
                  {SIZE sz;
                  item->rc.left=x+3; item->rc.top=y+3;
                  item->rc.right=item->rc.left+X_SPACING_LARGEICONS-3-3; item->rc.bottom=item->rc.top+y+3+4+16+8+8+2;
                  DrawIcon(hDC,x+6*6/*getFontWidth(&hDC->font)*/,y+4,diskIcon);		// METTERE diskIcon!
                  {char buf[2]={item->disk,0};
                  TextOut(hDC,x+4+(13-(*item->filename ? strlen(item->filename) : 1))*6/*getFontWidth(&hDC->font)*//2,y+4+16+2,
                    *item->filename ? item->filename : buf);
                  }
//                  rc.left=item->rc.left+1; rc.top=item->rc.top+1+8+2; rc.right=item->rc.right-4; rc.bottom=item->rc.bottom-1;
// bah qua no                  DrawText(hDC,rec.filename,-1,&rc,DT_TOP | DT_CENTER);
                  sz.cx=13*6/*getFontWidth(&hDC->font)*/;
                  sz.cy=8;
                  drawRectangleWindow(hDC,x+1,y+4+16+10,x+sz.cx,y+4+16+10+sz.cy);
                  if(dfn->fsdp.results.total_clusters) {
                    sz.cx=(sz.cx*(dfn->fsdp.results.total_clusters-dfn->fsdp.results.free_clusters)) /
                      dfn->fsdp.results.total_clusters;
                    fillRectangleWindowColor(hDC,x+2,y+5+16+10,x+2+sz.cx-1,y+5+16+10-1+sz.cy,
                      hDC->pen.color);
                    }
                  if(item->state) {
                    drawRectangleWindow(hDC,x+1,y+1,x+79,y+49);
                    }
                  x+=80;
                  if(x>=ps.rcPaint.right) {
                    x=0;
                    y+=50;
                    }
                  }
                  break;
                case 2:     // dettagli
                  {char buf[16];
                  item->rc.left=x+1; item->rc.top=y+1;
                  item->rc.right=ps.rcPaint.right-1; item->rc.bottom=item->rc.top+y+8+1;
                  hDC->font.bold=1;   // non è bello ma ok :)
                  buf[0]=item->disk; buf[1]=0;
                  TextOut(hDC,x+2,y+2,buf); 
                  hDC->font.bold=0;
                  TextOut(hDC,x+12,y+2,item->filename); 
                  sprintf(buf,"%lu/",
                    dfn->fsdp.results.free_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024);
                  TextOut(hDC,x+16*  6 /*getFontWidth(&hDC->font)*/,y+2,buf);
                  sprintf(buf,"%lu",
                    dfn->fsdp.results.total_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024);
                  TextOut(hDC,x+24* 6/*getFontWidth(&hDC->font)*/,y+2,buf);
                  TextOut(hDC,x+32*6 /*getFontWidth(&hDC->font)*/,y+2,"Kbytes");
                  if(item->state) {
                    drawRectangleWindow(hDC,x,y,ps.rcPaint.right-1,y+9);
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
            if(((totFiles*X_SPACING_SMALLICONS)/Y_SPACING_SMALLICONS)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
  //              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*32)/Y_SPACING_LARGEICONS),FALSE);
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
            if(((totFiles*X_SPACING_LARGEICONS)/Y_SPACING_LARGEICONS)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
  //              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*X_SPACING_LARGEICONS)/Y_SPACING_LARGEICONS),FALSE);
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
            if((totFiles*Y_SPACING_DETAILS)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
  //              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / (totFiles*8),FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            else {
              hWnd->style &= ~WS_VSCROLL;
              SetScrollRange(hWnd,SB_VERT,0,0,FALSE);
              SetScrollPos(hWnd,SB_VERT,0,FALSE);
              }
            if((40*6)>(ps.rcPaint.right-ps.rcPaint.left)) {   // v. testo sopra
              hWnd->style |= WS_HSCROLL;
              SetScrollRange(hWnd,SB_HORZ,0,(ps.rcPaint.right-ps.rcPaint.left)*(ps.rcPaint.right-ps.rcPaint.left) / (40*6),FALSE);
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
              case 0:		// elenco dischi
                break;
              default:
                break;
              }
            }
          else
            goto print_totals;
          if(dfn->fsdp.properties_status == FS_GET_PROPERTIES_NO_ERRORS) {
            char buf[32];

print_totals:          
            sprintf(buf,"%lu",dfn->fsdp.results.free_clusters*dfn->fsdp.results.sectors_per_cluster*dfn->fsdp.results.sector_size/1024); 
            TextOut(hDC,4,y+2,buf); 
            SetWindowByte(hWnd,GWL_USERDATA+2,3);

            item=(XLISTITEM*)GetWindowLong(hWnd,sizeof(DIRLIST));   // recupero etichetta volume
            sprintf(buf,"%s(%c) %s",*item->filename ? item->filename : "Disk",toupper(dfn->disk),
              *dfn->path ? dfn->path : ASTERISKS);
            SetWindowText(hWnd,buf);
            }
          else
            TextOut(hDC,4,y+2,"?"); 
          TextOut(hDC,70,y+2,"Kbytes free");
          }
        }
      else {
        SetWindowByte(hWnd,GWL_USERDATA+2,1);
        if(dfn->disk) {
          SetWindowText(hWnd,"Disco");
          char buf[32];
          sprintf(buf,"Inserire disco nell'unità %c:",dfn->disk);
          DrawText(hDC,buf,-1,&ps.rcPaint,DT_CENTER | DT_VCENTER);
          }
        }

      EndPaint(hWnd,&ps);
      }
      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      if(!GetWindowByte(hWnd,GWL_USERDATA+2)) {
        DrawText(hDC,"attendere prego...",-1,&hWnd->paintArea,DT_VCENTER | DT_CENTER);
        //e fare magari pure un Mount o FSinit..
        }
//          drawIcon8(&hWnd->hDC,0,16,folderIcon);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
			memset(dfn,sizeof(DIRLIST),0);
      SetWindowLong(hWnd,sizeof(DIRLIST),0);
      }
      return 0;
      break;
    case WM_CLOSE:
      freeDiskList((XLISTITEM *)GetWindowLong(hWnd,sizeof(DIRLIST)));
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_CHAR:
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      XLISTITEM *root=(XLISTITEM *)GetWindowLong(hWnd,sizeof(DIRLIST));
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      XLISTITEM *item=getSelectedDiskItem(root);
      switch(wParam) {
        case VK_SPACE:
/*          if(GetAsyncKeyState(VK_MENU) || GetAsyncKeyState(VK_CONTROL))
#warning togliere, gestire da main loop
            return DefWindowProc(hWnd,message,wParam,lParam);
          else {*/
            if(item)
              item->state ^= 1; // non è proprio giusto.. deseleziona solo! ma ha senso cmq? usa button
            
new_key_selected:
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent && !hWnd->parent->internalState) {
              NMHDR nmh;
              nmh.code=NM_KEYDOWN;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
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
            InvalidateRect(hWnd,NULL,TRUE);
            if(hWnd->parent && !hWnd->parent->internalState) {
              NMHDR nmh;
              nmh.code=NM_KEYDOWN;
              nmh.hwndFrom=hWnd;
              nmh.idFrom=(DWORD)hWnd->menu;
              SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh/*NMKEY */); // servirebbe un messaggio custom, volendo
              }
            }
          break;
        case VK_UP:
          {
            POINTS pt;
            int n;
            if(item)
              item->state=0;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
          }
          break;
        case VK_DOWN:
          {
            POINTS pt;
            int n;
            if(item)
              item->state=0;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
          }
          break;
        case VK_LEFT:
          {
            POINTS pt;
            int n;
            if(item)
              item->state=0;
//            n=getDiskItemPosition(item,item,&rc,GetWindowByte(hWnd,GWL_USERDATA+1),&pt);
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
            if(item) {
              item->state=1;
              InvalidateRect(hWnd,NULL,TRUE);
            } }
          if(hWnd->parent && !hWnd->parent->internalState) {
            NMHDR nmh;
            nmh.code=NM_LDOWN;
            nmh.hwndFrom=hWnd;
            nmh.idFrom=(DWORD)hWnd->menu;
            SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh/*NMKEY */); // servirebbe un messaggio custom, volendo
            } 
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
          break;
        }
      return 0 /*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;
    case WM_LBUTTONDOWN:
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      XLISTITEM *root=(XLISTITEM *)GetWindowLong(hWnd,sizeof(DIRLIST));
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      XLISTITEM *item=getDiskItemFromPoint(root,&rc,
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
          SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
          }
        }
      return 0 /*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;
    case WM_LBUTTONDBLCLK:
      {
      RECT rc;
      GetClientRect(hWnd,&rc);
      XLISTITEM *root=(XLISTITEM *)GetWindowLong(hWnd,sizeof(DIRLIST));
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      if(dfn->disk && root) 
        root=root->next;    // il primo è etichetta volume! (se disco)
      XLISTITEM *item=getDiskItemFromPoint(root,&rc,
        GetWindowByte(hWnd,GWL_USERDATA+1),MAKEPOINTS(lParam));
      if(item) {
dblclk:        
        switch(item->type) {
          case 0xff:
            if(GetAsyncKeyState(VK_SHIFT))      // se shift, nuova finestra :)
            {
              char buf[2]={item->disk,0};
              ShellExecute(NULL,"explore",NULL,buf,NULL,SW_SHOWNORMAL);
              }
            else {
              dfn->disk=item->disk;
              SetWindowByte(hWnd,GWL_USERDATA+2,0); InvalidateRect(hWnd,NULL,TRUE);
              }
            break;
          default:
            if(item->type & ATTR_DIRECTORY) {
              if(GetAsyncKeyState(VK_SHIFT)) {      // se shift, nuova finestra :) PER ORA VA SOLO SE PREMI UN TASTO VERO INSIEME
                char buf[2]={item->disk,0}; *dfn->path=0;
                ShellExecute(NULL,"explore",NULL,buf,item->filename,SW_SHOWNORMAL);
                }
              else {
                strcpy(dfn->path,item->filename);
                SetWindowByte(hWnd,GWL_USERDATA+2,0); InvalidateRect(hWnd,NULL,TRUE);
                }
              }
            else
              ShellExecute(NULL,"open",item->filename,NULL,NULL,SW_SHOWNORMAL);
            break;
          }
        }
      return 0/*DefWindowProc(hWnd,message,wParam,lParam)*/;
      }
      break;

    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
        switch(LOWORD(wParam)) { 
          case 32:
            dfn->disk=0; *dfn->path=0;
	          SetWindowByte(hWnd,GWL_USERDATA+2,0);		// aggiorna contenuto
            break;
          case 32+1:
            dfn->disk='A'; *dfn->path=0;
	          SetWindowByte(hWnd,GWL_USERDATA+2,0);		// aggiorna contenuto
            break;
          case 32+2:
            dfn->disk='C'; *dfn->path=0;
	          SetWindowByte(hWnd,GWL_USERDATA+2,0);		// aggiorna contenuto
            break;
          case 32+3:
            dfn->disk='E'; *dfn->path=0;
	          SetWindowByte(hWnd,GWL_USERDATA+2,0);		// aggiorna contenuto
            break;
          case 32+4:
            dfn->disk='R'; *dfn->path=0;
	          SetWindowByte(hWnd,GWL_USERDATA+2,0);		// aggiorna contenuto
            break;

          case 16+1:
	          SetWindowByte(hWnd,GWL_USERDATA+1,1);		// icone piccole
            break;
          case 16+2:
	          SetWindowByte(hWnd,GWL_USERDATA+1,0);		// icone grandi
            break;
          case 16+3:
	          SetWindowByte(hWnd,GWL_USERDATA+1,2);		// dettagli
            break;
          case 16+4:
	          // aggiorna.. :) sempre
            break;
            
          case 6:
//	          SendMessage(hWnd,WM_CLOSE,0,0); SC_CLOSE??
	          SendMessage(hWnd,WM_SYSCOMMAND,SC_CLOSE,0);
            break;
            
          default:
            return DefWindowProc(hWnd,message,wParam,lParam);
            break;
          }
        InvalidateRect(hWnd,NULL,TRUE);
        }
      return 1;
      break;
    case WM_NOTIFY:
      switch(lParam) {
        case CDN_TYPECHANGE:
          {
          DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
          SetWindowByte(hWnd,GWL_USERDATA+1,(GetWindowByte(hWnd,GWL_USERDATA+1)+1) % 3);
          InvalidateRect(hWnd,NULL,TRUE);
          }
          break;
        }
      break;

    case WM_FILECHANGE:
      // CheckMenuItem(explorerMenu3,)
      // UncheckMenuItem(explorerMenu3,)
      SetWindowByte(hWnd,GWL_USERDATA+2,0);
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
    TextOut(hDC,*x+x2+2,*y+6+16+2,text);
    // usare DrawText con rc?
  //        DrawText(hDC,text,-1,rc,DT_VTOP | DT_CENTER);
    *x+=X_SPACING_CONTROLPANEL;
    if((*x+(X_SPACING_CONTROLPANEL/2))>=rc->right) {
      *x=0;
      *y+=Y_SPACING_CONTROLPANEL;
      }
    }
  else {    // tipo lista
  	TextOut(hDC,*x,*y,text);
    TextOut(hDC,*x+6*w1,*y,text2);
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
        if(PtInRect(&rc2,ptl))
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
          string2[]="ABCDEFghijklm0123456789";
  int n=((int)GetWindowByte(hWnd,GWL_USERDATA+1))*4;

  if((lplf->lfPitchAndFamily & 0xf0)==FF_DONTCARE && lplf->lfHeight==8) {   // voglio vedere entrambi i "font system"... truschino
    ((LOGFONT *)lplf)->lfHeight=6;
    controlFontCB(lplf,lptm,dwType,lpData);
    n+=6+2;
    ((LOGFONT *)lplf)->lfHeight=8;
    }
  SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
          FW_NORMAL,0,0,0,lplf->lfCharSet,
          lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
  if(lplf->lfHeight<=12) {
    TextOut(hDC,1,n,string1);
    n+=(lplf->lfPitchAndFamily & 0xf0) != FF_DONTCARE ? (lplf->lfHeight*3)/2 : lplf->lfHeight;
    n+=2;
    if(lplf->lfHeight >= 8) {    // solo >=8 ossia non system piccolo
      SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
          FW_BOLD,0,0,0,lplf->lfCharSet,
          lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
      TextOut(hDC,1,n,string2);
      SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
          FW_NORMAL,TRUE,0,0,lplf->lfCharSet,
          lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
      TextOut(hDC,lplf->lfWidth*sizeof(string2)+2,n,string2);
      SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(lplf->lfHeight,lplf->lfWidth,lplf->lfEscapement,lplf->lfOrientation,
          FW_NORMAL,FALSE,TRUE,0,lplf->lfCharSet,
          lplf->lfOutPrecision,lplf->lfClipPrecision,lplf->lfQuality,lplf->lfPitchAndFamily,lplf->lfFaceName));
      TextOut(hDC,lplf->lfWidth*sizeof(string2)*2+2,n,string2);
      n+=(lplf->lfPitchAndFamily & 0xf0) != FF_DONTCARE ? (lplf->lfHeight*3)/2 : lplf->lfHeight;
      n+=2;
      }
    }
  else {
    TextOut(hDC,1,n,string2);
    n+=(lplf->lfPitchAndFamily & 0xf0) != FF_DONTCARE ? (lplf->lfHeight*3)/2 : lplf->lfHeight;
    n+=2;
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

			x=1; y=1;

      switch(GetWindowByte(hWnd,GWL_USERDATA)) {
        case 0:
          subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "Dischi",NULL,0);		// METTERE Icon s !
          subControlWC(hDC,&x,&y,&ps.rcPaint,printerIcon, "Stampanti",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,networkIcon, "Rete",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,keyboardIcon, "Tastiera",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,mouseIcon, "Mouse",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,videoIcon, "Schermo",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,videoIcon, "Font",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,audioIcon, "Audio",NULL,0);
          subControlWC(hDC,&x,&y,&ps.rcPaint,deviceIcon, "Dispositivi",NULL,0);
          {
            TextOut(hDC,ps.rcPaint.left,ps.rcPaint.bottom-16,BREAKTHROUGH_COPYRIGHT_STRING);
            TextOut(hDC,ps.rcPaint.left,ps.rcPaint.bottom-8,_PC_PIC_CPU_C);
          }
          SetWindowByte(hWnd,GWL_USERDATA+1,9);
          break;
        case 1:
          {uint32_t sn;
          *buf=0;
          if(SDcardOK) {
            FSgetVolume(buf,&sn);
            subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "A:",buf,8);
            }
          *buf=0;
          if(FDOK) {
            subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "B:",buf,8);
            }
          *buf=0;
          if(HDOK) {
            IDEFSgetVolume(buf,&sn);
            subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "C:",buf,8);
            }
          *buf=0;
          subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "D:",buf,8);
#if defined(USA_USB_HOST_MSD)
          *buf=0;
          subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "E:",buf,8);
#endif
#ifdef USA_RAM_DISK 
          *buf=0;
          if(RAMdiscArea) {
            RAMFSgetVolume(buf,&sn);
            subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "R:",buf,8);
            }
#endif
#if defined(USA_WIFI) || defined(USA_ETHERNET)
          *buf=0;
          subControlWC(hDC,&x,&y,&ps.rcPaint,diskIcon, "F:","< >",8);   // fare...
#endif
          SetWindowByte(hWnd,GWL_USERDATA+1,6);//PARAMETRIZZARE!
          }
          break;
        case 2:
          sprintf(buf,"%s %s",GetStatus(SOUTH_BRIDGE,NULL) & 0x00001000 ? CONNESSA : NON_CONNESSA,
            GetStatus(SOUTH_BRIDGE,NULL) & 0x00000B00 ? "(errore)" : "");    
          subControlWC(hDC,&x,&y,&ps.rcPaint,printerIcon, "LPT1:",buf,8);
          GetStatus(SOUTH_BRIDGE,buf);
          subControlWC(hDC,&x,&y,&ps.rcPaint,printerIcon, "LPT2:",
            buf[4] ? CONNESSA : NON_CONNESSA,8);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 3:
#ifdef USA_ETHERNET
          sprintf(buf,"%u.%u.%u.%u (%s)",
//            ip.v[0],ip.v[1],ip.v[2],ip.v[3], n ? 0 : 1);
/*finire*/            BiosArea.MyIPAddr.v[0],BiosArea.MyIPAddr.v[1],BiosArea.MyIPAddr.v[2],BiosArea.MyIPAddr.v[3],
                  MACIsLinked() ? CONNESSA : NON_CONNESSA);
          subControlWC(hDC,&x,&y,&ps.rcPaint,networkIcon, "Ethernet",buf,10);
#endif
#ifdef USA_WIFI
          {uint8_t n;
          IP_ADDR ip;
          m2m_periph_gpio_get_val(M2M_PERIPH_GPIO18,&n);
          ip.Val=myIp.ip /*BiosArea.MyIPAddr2*/;
          sprintf(buf,"%u.%u.%u.%u (%s)",
            ip.v[0],ip.v[1],ip.v[2],ip.v[3], n ? CONNESSA : NON_CONNESSA);
          subControlWC(hDC,&x,&y,&ps.rcPaint,networkIcon, "WiFi",buf,10);
          }
#endif
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 4:
          subControlWC(hDC,&x,&y,&ps.rcPaint,keyboardIcon, "Tastiera PS/2",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b00000101 ? PRESENTE : NON_PRESENTE,15);
          subControlWC(hDC,&x,&y,&ps.rcPaint,keyboardIcon, "Tastiera USB",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b01000000 ? PRESENTE : NON_PRESENTE,15);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 5:
          subControlWC(hDC,&x,&y,&ps.rcPaint,mouseIcon, "Mouse PS/2",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b00001010 ? PRESENTE : NON_PRESENTE,12);
          subControlWC(hDC,&x,&y,&ps.rcPaint,mouseIcon, "Mouse USB",
          	GetStatus(SOUTH_BRIDGE,NULL) & 0b10000000 ? PRESENTE : NON_PRESENTE,12);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 6:
          sprintf(buf,"%u; %ux%u,%u bpp", BiosArea.videoMode,Screen.cx,Screen.cy,_bpp);
          subControlWC(hDC,&x,&y,&ps.rcPaint,deviceIcon, "Video mode",buf,12);
          subControlWC(hDC,&x,&y,&ps.rcPaint,deviceIcon, "Sfondo",
            (char *)GET_WINDOW_OFFSET(desktopWindow,sizeof(POINTS)*16),12);
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
          break;
        case 7:
          SetWindowByte(hWnd,GWL_USERDATA+1,0);
          EnumFonts(hDC,NULL,controlFontCB,(DWORD)hDC);
          
          break;
        case 8:
          *buf=0;
          subControlWC(hDC,&x,&y,&ps.rcPaint,deviceIcon, "Scheda audio",PRESENTE /*buf*/,14);
          subControlWC(hDC,&x,&y,&ps.rcPaint,deviceIcon, "MIDI",PRESENTE /*buf*/,14);
          sprintf(buf,"%02X %u %u %u %u",ReadJoystick(0),ReadJoystick(-1),ReadJoystick(-2),ReadJoystick(-3),ReadJoystick(-4));
          subControlWC(hDC,&x,&y,&ps.rcPaint,deviceIcon, "Joystick",buf,14);
          SetWindowByte(hWnd,GWL_USERDATA+1,3);
          break;
        case 9:
          {
          
          *buf=0;
          
          subControlWC(hDC,&x,&y,NULL,NULL,"CPU","PIC32MZ",20);
          sprintf(buf,"%uMHz",SPLLCONbits.PLLMULT*4);
          subControlWC(hDC,&x,&y,NULL,NULL,"Clock",buf,20);
          sprintf(buf,"%u/%uKB",getTotRAM()/1024,extRAMtot/1024);
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
          subControlWC(hDC,&x,&y,NULL,NULL,"Power supply","5.0V / 3.3V",20);    // :)
          SetWindowByte(hWnd,GWL_USERDATA+1,13);
          }
          break;
        }


fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      // forzare font piccolo??
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      }
      return 0;
      break;
    case WM_CLOSE:
      m_WndControlPanel=NULL;
      DestroyWindow(hWnd);
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
          SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
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
      n=getWCItemFromPoint(&rc,MAKEPOINTS(lParam),GetWindowByte(hWnd,GWL_USERDATA+0),
        GetWindowByte(hWnd,GWL_USERDATA+1));
      switch(GetWindowByte(hWnd,GWL_USERDATA+0)) {
        case 0:
//          SetWindowByte(hWnd,GWL_USERDATA+0,n+1); 
          SendMessage(hWnd,WM_COMMAND,MAKELONG(n+1,0),0);
          InvalidateRect(hWnd,NULL,TRUE);
          break;
        case 1:     // dischi
          switch(n) {
            HDC hDC;
            DC myDC;
            case 0:
              if(SDcardOK) {    // provo a scrivere!
                SUPERFILE f;
                char buf[16];
                f.drive='A';
                GetTempFileName(NULL,"CTR",0,buf);
                if(SuperFileOpen(&f,buf,'w')) {
                  SuperFileWrite(&f,_PC_PIC_CPU_C,strlen(_PC_PIC_CPU_C));
                  SuperFileClose(&f);
                  }
created:
                hDC=GetDC(hWnd,&myDC);
                //fillRect(
                TextOut(hDC,rc.left,rc.bottom-16,"File creato");
                ReleaseDC(hWnd,hDC);
                }
              break;
            case 1:
              break;
            case 2:
              break;
            case 3:   // IL PROBLEMA è CHE NON SO A CHE DEVICE CORRISPONDE LA RIGA... dovrei salvarlo ma ok
              if(RAMdiscArea) {    // provo a scrivere!
                SUPERFILE f;
                char buf[16];
                f.drive='R';
                GetTempFileName("" /*per non avere path!*/,"CTR",0,buf); 
                if(SuperFileOpen(&f,buf,'w')) {
                  SuperFileWrite(&f,_PC_PIC_CPU_C,strlen(_PC_PIC_CPU_C));
                  SuperFileClose(&f);
                  }
                goto created;
                }
              break;
            case 4:
              if(RAMdiscArea) {    // provo a scrivere!
                SUPERFILE f;
                char buf[16];
                f.drive='R';
                GetTempFileName("" /*per non avere path!*/,"CTR",0,buf); 
                if(SuperFileOpen(&f,buf,'w')) {
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
              TextOut(hDC,rc.left,rc.bottom-16,"Pagina inviata alla stampante");
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
          TextOut(hDC,rc.left,rc.bottom-16,buf);
          ReleaseDC(hWnd,hDC);
          }
          break;
        case 5:     // mouse
          goto mouseclick;
          break;
        case 6:     // video
          drawRGBCube(hWnd->clientArea.left+32,hWnd->clientArea.top+48,64);
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
              SuperFileOpen(&f,NULL,0);
              SuperFileWrite(&f,"TheQuickBrownFoxJumpsOverTheLazysDogsBack",41);
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
              TextOut(hDC,rc.left,rc.bottom-8,buf);
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
        }
      if(hWnd->parent && !hWnd->parent->internalState) {
        NMHDR nmh;
        nmh.code=NM_DBLCLK;
        nmh.hwndFrom=hWnd;
        nmh.idFrom=(DWORD)hWnd->menu;
        SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      }
      break;
      
    case WM_KEYDOWN:
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
        TextOut(hDC,rc.left,rc.bottom-16,buf);
        ReleaseDC(hWnd,hDC);
        }
      break;
    case WM_CHAR:
      switch(wParam) {
        case VK_SPACE:
          if(GetAsyncKeyState(VK_MENU) || GetAsyncKeyState(VK_CONTROL))
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
              SendMessage(hWnd->parent,WM_NOTIFY,(DWORD)hWnd->menu,(LPARAM)&nmh); // servirebbe un messaggio custom, volendo
              }
            }
          break;
        case VK_UP:
          if(!GetWindowByte(hWnd,GWL_USERDATA+0)) {
            }
          break;
        case VK_DOWN:
          if(!GetWindowByte(hWnd,GWL_USERDATA+0)) {
            }
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
          
        case VK_ESCAPE:
          goto resetta;
          break;
          
        default:
          break;
        }
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;

    case WM_RBUTTONDOWN:
    case WM_RBUTTONDBLCLK:
        {
        char buf[20]; 
        HDC hDC;
        DC myDC;
        RECT rc;
        if(GetWindowByte(hWnd,GWL_USERDATA+0) == 5) { // magari anche GetAsyncKeyState ma per ora non va :)
mouseclick:
          GetClientRect(hWnd,&rc);
          hDC=GetDC(hWnd,&myDC);
          extern struct MOUSE mouse;
          sprintf(buf,"mouse: %02X [%s], %u, %u",mouse.buttons,(message==WM_LBUTTONDBLCLK || message==WM_RBUTTONDBLCLK) ? 
                  "dclick" : "click",mouse.x,mouse.y);
          fillRectangleWindow(hDC,rc.left,rc.bottom-16,rc.right,rc.bottom);
          TextOut(hDC,rc.left,rc.bottom-16,buf);
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
      
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
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
            SetWindowByte(hWnd,GWL_USERDATA+0,LOBYTE(LOWORD(wParam)));
            SetWindowText(hWnd,hWnd->menu->menuItems[0].menu->menuItems[LOBYTE(LOWORD(wParam))-1].text);
            break;
          case 10:
            goto resetta;
            break;
          case 16:
// inutile :)            InvalidateRect(hWnd,NULL,TRUE);
            break;
          }  
        InvalidateRect(hWnd,NULL,TRUE);
        }
      return 1;
      break;
      
    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

LRESULT DefWindowProcTaskManager(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      int i; 
      UGRAPH_COORD_T x,y;
      char buffer[64];
      
      HDC hDC=BeginPaint(hWnd,&ps);

      i=0;
      HWND myWnd=rootWindows;
      while(myWnd) {
        CLASS c;
        strncpy(buffer,myWnd->caption,20);
        buffer[20]=0;
        TextOut(hDC,2,(i*(8+1))+2,buffer);
        c=myWnd->class;
        if(!c.class)
          c.class=MAKEFOURCC('D','F','L','T');
        sprintf(buffer,"%c%c%c%c %02X %u",c.class4[0],c.class4[1],c.class4[2],c.class4[3],
          myWnd->status,myWnd->zOrder);
        TextOut(hDC,2+22*6,(i*(8+1))+2,buffer);
        i++;
        myWnd=myWnd->next;
        }

fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      // forzare font piccolo??
      int i;
      SetTimer(hWnd,1,15000,NULL);
      }
      return 0;
      break;
    case WM_CLOSE:
      KillTimer(hWnd,1);
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_ESCAPE:
          SendMessage(hWnd,WM_CLOSE,0,0);
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
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      BYTE x,y;
      char *cmdline;
      
      HDC hDC=BeginPaint(hWnd,&ps);
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);
			x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
      fillRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
      TextOut(hDC,ps.rcPaint.left+1+x*6,ps.rcPaint.top+1+y*8,prompt);
      cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
      TextOut(hDC,ps.rcPaint.left+1+6*strlen(prompt),ps.rcPaint.top+1,cmdline);
      EndPaint(hWnd,&ps);

      }
      return 1;
      break;
    case WM_TIMER:
      {
      DC myDC;
      HDC hDC=GetDC(hWnd,&myDC);
      RECT rc;
      BYTE x,y;
      GetClientRect(hWnd,&rc);    // fingo cursore :D
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);
			x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
//      FillRectangleWindow(hDC,rc.left+1+6*3+x*6,rc.top+1+y*8,rc.left+1+6*3+x*6+6,rc.top+1+y*8);
      TextOut(hDC,rc.left+1+6*strlen(prompt)+x*6,rc.top+1+y*8,GetWindowByte(hWnd,0) ? "_" : " ");
      SetWindowByte(hWnd,0,!GetWindowByte(hWnd,0));
      ReleaseDC(hWnd,hDC);
      }
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);   //
      SetWindowByte(hWnd,GWL_USERDATA+2,15);   //white
      SetWindowByte(hWnd,GWL_USERDATA+3,0);   //black
      SetWindowLong(hWnd,0,0);   // cursore & stringa!
      SetTimer(hWnd,1,500,NULL);    // per cursore...
      }
      return 0;
      break;
    case WM_CLOSE:
      KillTimer(hWnd,1);
      DestroyWindow(hWnd);
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
      if(HIWORD(wParam) == 0) {   // menu
        switch(LOWORD(wParam)) { 
          case 1:
            {char *cmdline;
//extern MINIBASIC minibasicInstance;
//            minibasic(&minibasicInstance,NULL);
            cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
            strcpy(cmdline,"BASIC");    // :)
      			SetWindowByte(hWnd,GWL_USERDATA+0,5);
            InvalidateRect(hWnd,NULL,TRUE);
            }
            break;
          case 2:
            SendMessage(hWnd,WM_CLOSE,0,0);
            break;
          }
        }
      return 1;
      break;
      
    case WM_CHAR:
      {char *cmdline;
      BYTE i;
      BYTE x,y;
      DC myDC;
      HDC hDC;
      RECT rc;
      
      cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
      if(GetAsyncKeyState(VK_MENU))    // ALT per menu
//        if(!matchAccelerator(hWnd->menu,LOBYTE(wParam)))
        return DefWindowProc(hWnd,message,wParam,lParam);
      if(GetAsyncKeyState(VK_CONTROL))    // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
//        if(!matchAccelerator(hWnd->menu,LOBYTE(wParam)))
        return DefWindowProc(hWnd,message,wParam,lParam);
      
      GetClientRect(hWnd,&rc);
//      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
			x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
      
      switch(wParam) {
        case VK_F10:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        case VK_ESCAPE:
          cmdline[0]=0;
  				x=0;
					y++;
     			SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
          break;
        case VK_BACK:
          i=strlen(cmdline);
          if(i > 0) {
            cmdline[--i]=0;
						x--;
      			SetWindowByte(hWnd,GWL_USERDATA+0,x);
            }
          ch=' ';
          goto putchar;
          break;
        case VK_TAB:
          goto putchar;
          break;
        case VK_RETURN:
					x=0; y++;
          if(y>=rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
    			SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
          KillTimer(hWnd,1);
          
          hDC=GetDC(hWnd,&myDC);
//      TextOut(hDC,rc.left+1+6*strlen(prompt)+x*6,rc.top+1+y*8," "); FARE per pulire cursore!
          m_stdout=/*m_stdin=*/m_stderr=DEVICE_WINDOW;
          execCmd(cmdline,NULL,cmdline);
          cmdline[0]=0;
          m_stdout=m_stdin=m_stderr=DEVICE_CON;   // direi di togliere e lasciare fisso DEVICE_WINDOW (vedi InitWindows
     			x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
          SetTimer(hWnd,1,500,NULL);    // per cursore...
					x=0; y++;
          if(y>=rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
          TextOut(hDC,rc.left+1+x*6,rc.top+1+y*8,prompt);
          ReleaseDC(hWnd,hDC);
          break;
        default:
          ch=LOBYTE(wParam);
          if(!GetAsyncKeyState(VK_SHIFT))    // gestisco SHIFT, xché i CHAR son solo maiuscoli!
            ch=tolower(ch);
          if(isprint(ch)) {
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
			if(y>=rc.bottom/8) {
				y--;
        ScrollWindow(hWnd,0,-8,&rc,NULL);
				}
			SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
      }
      break;
      
    case WM_PRINTCHAR:
      {
      DC myDC;
      HDC hDC;
      BYTE x,y;
      RECT rc;
      i=WHITE;
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
     			SetWindowByte(hWnd,GWL_USERDATA+0,x);
          break;
        case 0x0c:
          fillRectangleWindowColor(hDC,rc.left,rc.top,rc.right,rc.bottom,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]); 
          // o fare semplicemente un InvalidateRect(mInstance->hWnd,NULL,TRUE) ??
          x=y=0;
     			SetWindowByte(hWnd,GWL_USERDATA+0,0); SetWindowByte(hWnd,GWL_USERDATA+1,0);
          break;
        case '\r':
          x=0;
          break;
        case '\n':
          x=0; y++;
          if(y>=rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
          break;
        default:
          if(isprint(ch)) {
            drawCharWindow(hDC,1+6*strlen(prompt)+x*6,1+y*8,ch);
            x++;
            if(x>=rc.right/6) {
              x=0; y++;
              if(y>=rc.bottom/8) {
                y--;
                ScrollWindow(hWnd,0,-8,&rc,NULL);
                }
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

LRESULT DefWindowProcDC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
			BYTE i;
      BYTE attrib=GetWindowByte(hWnd,sizeof(POINTS)*16+16+4+4);
      HWND myWnd;
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      
      hDC->pen=CreatePen(PS_SOLID,1,WHITE);
      hDC->brush=CreateSolidBrush(desktopColor);

			if(inScreenSaver) {
				i=rand() % 100; i=max(i,8); i=min(i,80);
        SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(i,(i*2)/3,0,0,rand() & 1 ? FW_NORMAL : FW_BOLD,
          rand() & 1,rand() & 1,rand() & 1,ANSI_CHARSET,
          OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,VARIABLE_PITCH | (rand() & 1 ? FF_ROMAN : FF_SWISS),NULL));
        img_ofs_y=rand() % ps.rcPaint.bottom; img_ofs_x=rand() % ps.rcPaint.right;
        SetTextColor(hDC,rand());
        SetBkColor(hDC,BLACK /*desktopColor*/);
/*char* reverseCopy(char* destination, const char* source, int num) {
    char* d = destination;
    while (num) {
        *d++ = source[--num];
    }
    *d = '\0';
    return destination;
}*/
        TextOut(hDC,img_ofs_x-ps.rcPaint.right/2,img_ofs_y-ps.rcPaint.bottom/2,"BREAKTHROUGH");
        hDC->font=GetStockObject(SYSTEM_FONT).font;

				}

      DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      BYTE attrib=GetWindowByte(hWnd,sizeof(POINTS)*16+16+4+4);
      GFX_COLOR f=GetWindowWord(hWnd,GWL_USERDATA+0),b=GetWindowWord(hWnd,GWL_USERDATA+2);
			char *fileWallpaper=(char *)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16);
      GRAPH_COORD_T img_ofs_x,img_ofs_y;

 			if(inScreenSaver) {
	      DrawWindow(hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right,hWnd->paintArea.bottom,b,BLACK /*b*/);		// colore?

				}
			else {

      DrawWindow(hWnd->paintArea.left,hWnd->paintArea.top,hWnd->paintArea.right,hWnd->paintArea.bottom,b,b);
      
extern const unsigned char W95train[45398];
extern const unsigned char W95lion[42090];
extern const unsigned char W3tartan[5132];
extern const unsigned char WindowXP[49741];

      if(!strcmp(fileWallpaper,"w95lion.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16)=(BYTE*)W95lion;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+4)=sizeof(W95lion);
        }
      else if(!strcmp(fileWallpaper,"w95train.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16)=(BYTE*)W95train;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+4)=sizeof(W95train);
        }
      else if(!strcmp(fileWallpaper,"w3tartan.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16)=(BYTE*)W3tartan;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+4)=sizeof(W3tartan);
        }
      else if(!strcmp(fileWallpaper,"windowxp.jpg")) {
        *(BYTE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16)=(BYTE*)WindowXP;
        *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+4)=sizeof(WindowXP);
        }
      else {
        SUPERFILE **myJpegFile=(SUPERFILE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16);
        *myJpegFile=malloc(sizeof(SUPERFILE));
        (*myJpegFile)->drive=currDrive;
        if(SuperFileOpen(*myJpegFile,fileWallpaper,'r')) {   
          *(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+4)=0xffffffff;    // marker File vs. Rom
          }
        else {
          free(*myJpegFile);
          SetWindowLong(hWnd,sizeof(POINTS)*16+16+4,0);
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
			if(GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16)) {
				int x,x1,mcu_x,x_decimate,y_decimate;
				int y,y1,mcu_y;
	      pjpeg_image_info_t JPG_Info;
        bool status;
				if(!(status=pjpeg_decode_init(&JPG_Info,pjpeg_need_bytes_callback,
          (DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16),0))) {
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
  							BBitBlt(img_ofs_x+x1,img_ofs_y+y1,img_ofs_x+x1+xPixels,img_ofs_y+y1+yPixels,mypixels);
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
          SUPERFILE **myJpegFile=(SUPERFILE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16);
          SuperFileSeek(*myJpegFile,0,SEEK_SET);
  				if(!(status=pjpeg_decode_init(&JPG_Info, pjpeg_need_bytes_callback,
            (DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16),0))) {
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
        if(GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16) && (*(DWORD*)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16+4)) == 0xffffffff) {
          SUPERFILE **myJpegFile=(SUPERFILE**)GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16+16);
          SuperFileClose(*myJpegFile);
          free(*myJpegFile); 
          SetWindowLong(hWnd,sizeof(POINTS)*16+16+4,0);
          }
				}
      
      SetTextColor(hDC,ORANGE);
      img_ofs_y=hWnd->paintArea.bottom-(getFontHeight(&hDC->font)+1)*3-14;
      img_ofs_x=hWnd->paintArea.right-(getFontWidth(&hDC->font))*(14+10)-2;
      char buffer[16];
      DWORD i;
// bah no      if(GetWindowLong(hWnd,sizeof(POINTS)*16+16+4)) {    // se non è già sullo sfondo, lo scrivo qua
        i=15;
        GetUserName(buffer,&i);
        TextOut(hDC,img_ofs_x,img_ofs_y,"Utente:");
        TextOut(hDC,img_ofs_x+getFontWidth(&hDC->font)*10+2,img_ofs_y,buffer);
//        }
      GetComputerName(buffer,&i);
      img_ofs_y += getFontHeight(&hDC->font)+1;
      TextOut(hDC,img_ofs_x,img_ofs_y,"Stazione:");
      TextOut(hDC,img_ofs_x+getFontWidth(&hDC->font)*10+2,img_ofs_y,buffer);
#if defined(USA_WIFI) || defined(USA_ETHERNET)
      IP_ADDR IPAddress=GetIPAddress(-1);
			sprintf(buffer,"%u.%u.%u.%u",IPAddress.v[0],IPAddress.v[1],IPAddress.v[2],IPAddress.v[3]);
      img_ofs_y += getFontHeight(&hDC->font)+1;
      TextOut(hDC,img_ofs_x,img_ofs_y,"Indirizzo:");
      TextOut(hDC,img_ofs_x+getFontWidth(&hDC->font)*10+2,img_ofs_y,buffer);
#endif
			}

          
      DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
			memset(GET_WINDOW_OFFSET(hWnd,0),sizeof(POINTS)*16+16+4+4+1,0);
      if(cs->lpCreateParams)
        strncpy(GET_WINDOW_OFFSET(hWnd,sizeof(POINTS)*16),cs->lpCreateParams,15);
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(BRIGHTYELLOW,desktopColor));
      }
      return 0;
      break;
      
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
        BYTE i;
        switch(LOWORD(wParam)) { 
          case 1:
            ShellExecute(NULL,"explore",NULL,"A",NULL,SW_SHOWNORMAL);
            break;
          case 2:
            ShellExecute(NULL,"control",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 3:
            ShellExecute(NULL,"taskman",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 4:
            ShellExecute(GetDesktopWindow() /*NULL*/,"calc",NULL,NULL,NULL,SW_SHOWNORMAL);
            // NON PUOI passare NULL, è una dialog!
            break;
          case 5:
            ShellExecute(NULL,"clock",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 10:
            ShellExecute(NULL,"calendar",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 6:
            if(GetAsyncKeyState(VK_SHIFT) || GetAsyncKeyState(VK_SHIFT)) {    // tanto per.. lancio DOS!
              /*m_Wnd2=*/CreateWindow(MAKECLASS(WC_CMDSHELL),"DOS shell",WS_BORDER | WS_CAPTION | WS_MAXIMIZEBOX | WS_MINIMIZEBOX |
                WS_OVERLAPPEDWINDOW | WS_SYSMENU | WS_VISIBLE,
                240,30,220,150,
                NULL,(MENU *)NULL,(void*)2
                );
//              ShowWindow(m_Wnd2,SW_SHOWNORMAL);
//              UpdateWindow(m_Wnd2);
              }
            else
              MessageBox(NULL,"Esegui","futuro",MB_OK);
            break;
          case 8:
            ShellExecute(NULL,"basic",NULL,NULL,NULL,SW_SHOWNORMAL);
            break;
          case 9:
            ShellExecute(NULL,"surf",NULL,NULL,NULL,SW_SHOWMAXIMIZED);
            break;
          case 7:
            quitSignal=TRUE;
//    				PostMessage(NULL,WM_QUIT,0,0);
            // uscire da tutto...
            break;
            
          case 17:
            i=GetWindowByte(taskbarWindow,GWL_USERDATA+0) ^ 2;
            SetWindowByte(taskbarWindow,GWL_USERDATA+0,i);
//            StdBeep(100+ (i & 2)*100); //debug!
            InvalidateRect(taskbarWindow,NULL,TRUE);
            break;
          case 18:
            MinimizeWindows(NULL,NULL);
            break;
          case 19:
            i=GetWindowByte(taskbarWindow,GWL_USERDATA+0) ^ 1;
            SetWindowByte(taskbarWindow,GWL_USERDATA+0,i);
            if(i & 1)
              SetTimer(hWnd,1,60000,NULL);
              // finire completare :)
            else {
              KillTimer(hWnd,1);
         			DestroyWindow((HWND)GetWindowLong(taskbarWindow,4));
         			SetWindowLong(taskbarWindow,4,0);
              }
            break;
          }
        return 1;
        }
      break;
    case WM_RBUTTONDOWN:
        SetXY(0,0,180);
        printWindows(rootWindows);
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

static void updateTaskbarClock(HWND hWnd) {
  
  HWND myWnd=(HWND)GetWindowLong(hWnd,4);
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
      {int n;
      hWnd->active=0;
      n=nonClientPaint(hWnd,(RECT *)wParam);
      hWnd->active=1;
      return n;
      }
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
      return clientPaint(hWnd,NULL /*l'area accumulata da invalidate...*/);
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      BYTE attrib=GetWindowByte(hWnd,GWL_USERDATA+0);
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      return 1;
      }
      break;
    case WM_SHOWWINDOW:
      InvalidateRect(hWnd,NULL,TRUE);     // qua gestisco il caso particolare... SW_OTHERUNZOOM SW_OTHERZOOM
			return 0;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
			memset(GET_WINDOW_OFFSET(hWnd,0),4+4,0);
      BYTE attrib=(DWORD)cs->lpCreateParams;
      SetWindowByte(hWnd,GWL_USERDATA+0,attrib);
      HWND myWnd=CreateWindow(MAKECLASS(WC_BUTTON),"Start",WS_BORDER | WS_VISIBLE | WS_TABSTOP |
        WS_CHILD | BS_CENTER | BS_PUSHBUTTON,
        0,0,5*6+2,10,
        hWnd,(HMENU)1,NULL
        );
      SetWindowLong(hWnd,0,(DWORD)myWnd);
 			SetWindowLong(hWnd,4,0);
			if(attrib & 1) {   // orologio
        myWnd=CreateWindow(MAKECLASS(WC_STATIC),"00:00",WS_BORDER | WS_VISIBLE | WS_CHILD | 
          WS_DISABLED | SS_SIMPLE /* per bianco! SS_CENTER*/ | SS_NOTIFY,  // in effetti potrebbe essere attivo per set clock..
          cs->cx-5*6-4,0,5*6+2,10,
          hWnd,(HMENU)2,NULL
          );
   			SetWindowLong(hWnd,4,(DWORD)myWnd);
        SetTimer(hWnd,1,60000,NULL);
				}
      myWnd=CreateWindow(MAKECLASS(WC_STATIC),NULL,WS_BORDER | WS_VISIBLE | WS_CHILD | 
        WS_DISABLED | SS_ICON | SS_NOTIFY,
        cs->cx-5*6-4-12,0,8,10,
        hWnd,(HMENU)3,NULL
        );
      myWnd->icon=speakerIcon;
      SetWindowLong(hWnd,8,(DWORD)myWnd);
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
      break;*/
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
        switch(LOWORD(wParam)) { 
          }
        }
      if(HIWORD(wParam) == BN_CLICKED) {   // è 0!! coglioni :D
        switch(LOWORD(wParam)) { 
          case 1:
            SendMessage(taskbarWindow,WM_SYSCOMMAND,MAKELONG(SC_TASKLIST,0),0);
            break;
          }
        }
      if(HIWORD(wParam) == STN_DBLCLK) {   // pure questo è 0!! supercoglioni :D
        switch(LOWORD(wParam)) { 
          case 2:   // orologio, 
            SendMessage(desktopWindow,WM_COMMAND,MAKELONG(5,0),0);
            break;
          case 3:   // audio
            MessageBox(hWnd,"audio","arriva",MB_OK);
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
        case SC_CLOSE:
          quitSignal=TRUE;    // però chiedere conferma :)
          break;
        }
      break;
    case WM_LBUTTONDOWN:
// idem SEMPRE!           if(activeMenu==&menuStart) {
      SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
      break;
    case WM_RBUTTONDOWN:
      if(TrackPopupMenu((HMENU)&menuStart2,TPM_RETURNCMD | TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
        mousePosition.x,hWnd->nonClientArea.top,0,desktopWindow /*hWnd*/,NULL)) {
          }
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

      img_ofs_x=ps.rcPaint.right-ps.rcPaint.left-appData.videoStreamFormat.videoSize.cx;
      img_ofs_y=ps.rcPaint.bottom-ps.rcPaint.top-appData.videoStreamFormat.videoSize.cy-10;

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
			DrawText(hDC,"Video unavailable",-1,&ps.rcPaint,DT_VCENTER | DT_CENTER | DT_SINGLELINE);
#endif

      DeleteObject(OBJ_BRUSH,(GDIOBJ)hDC->brush);
      DeleteObject(OBJ_PEN,(GDIOBJ)hDC->pen);
      EndPaint(hWnd,&ps);
      }
      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
          
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(CYAN,BLACK));
      SetWindowLong(hWnd,0,0);    // azzero dati vari
      }
      return 0;
      break;
      
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
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
			(LPSTR)"", // window name if pop-up 
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
