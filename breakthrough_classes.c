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
extern APP_DATA appData;
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
extern struct KEYPRESS keypress;

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
void drawRectangleWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
void fillRectangleWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
void fillRectangleWindowColor(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2,GFX_COLOR c);
void drawEllipseWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
void fillEllipseWindow(HDC hDC, UGRAPH_COORD_T x1, UGRAPH_COORD_T y1, UGRAPH_COORD_T x2, UGRAPH_COORD_T y2);
BOOL nonClientPaint(HWND hWnd,const RECT *rc);
BOOL clientPaint(HWND hWnd,const RECT *rc);


extern HMENU activeMenu,activeMenuParent;
extern BYTE activeMenuCntX,activeMenuCntY;
extern HWND activeMenuWnd;
extern RECT activeMenuRect;

extern HWND rootWindows,taskbarWindow;
extern HWND desktopWindow;

extern BYTE inScreenSaver;
extern GFX_COLOR windowForeColor, windowInactiveForeColor, windowBackColor, desktopColor;

extern const GFX_COLOR standardCursorSm[];
extern const GFX_COLOR hourglassCursorSm[];
extern const GFX_COLOR standardIcon[];
extern int8_t caretTime;

extern const MENU menuStart;

extern const GFX_COLOR folderIcon8[];
extern const GFX_COLOR fileIcon8[];
extern const GFX_COLOR folderIcon[];
extern const GFX_COLOR fileIcon[];

extern const GFX_COLOR standardCaret[];


static DWORD jpegLen;
static BYTE *jpegPtr;
static SUPERFILE *jpegFile;   // metterli tutti dentro la classe desktopclass... union..
static unsigned char pjpeg_need_bytes_callback(unsigned char* pBuf, unsigned char buf_size, unsigned char *pBytes_actually_read, void *pCallback_data) {
  uint32_t n;
  DWORD *myjpegLen=(DWORD*)pCallback_data;
  BYTE **myjpegPtr=((BYTE **)pCallback_data)+1;
  SUPERFILE *myjpegFile=((SUPERFILE *)pCallback_data)+1;

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
      
      switch(hWnd->style & 0xff) {
        case SS_LEFT:
          SetTextColor(hDC,WHITE);
          SetBkColor(hDC,c);
          x=y=0;
          TextOut(hDC,x,y,hWnd->caption);
          break;
        case SS_CENTER:
          SetTextColor(hDC,WHITE);
          SetBkColor(hDC,c);
          i=strlen(hWnd->caption)*6*hDC->font.size;
          if(i>=ps.rcPaint.right)
            i=ps.rcPaint.right;
          x=(ps.rcPaint.right-i)/2; y=0;
          TextOut(hDC,x,0,hWnd->caption);
          break;
        case SS_RIGHT:
          SetTextColor(hDC,WHITE);
          SetBkColor(hDC,c);
          i=strlen(hWnd->caption)*6*hDC->font.size;
          if(i>=ps.rcPaint.right)
            i=ps.rcPaint.right;
          x=ps.rcPaint.right-i; y=0;
          TextOut(hDC,x,y,hWnd->caption);
          break;
        case SS_ICON:
          x=y=0;
          drawIcon8(hDC,x,y,hWnd->icon);
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
      cs->style |= WS_DISABLED;   // direi
      if((cs->style & 0xff) == SS_ICON)
        cs->cx=cs->cy=8 + (hWnd->style & WS_BORDER ? 2 : 0); // type ecc..
      else
        cs->cy=hWnd->font.size*8 + (hWnd->style & WS_BORDER ? 2 : 0); // 
      }
      return 0;
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
      char *s;
      BYTE startSel,endSel;
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
      
      s=(char*)GET_WINDOW_OFFSET(hWnd,0);
      startSel=GetWindowByte(hWnd,GWL_USERDATA+1);
      endSel=GetWindowByte(hWnd,GWL_USERDATA+2);
      x=0; y=ps.rcPaint.top+1;
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
        drawCharWindow(hDC,x*6*hDC->font.size,y,*s++);
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
      }
      return 0;
      break;
    case WM_CLOSE:
      KillTimer(hWnd,31);
      break;
      
    case WM_LBUTTONDOWN:
      //startsel ecc
      break;
    case WM_CHAR:
      { char buf[8];
      int i;
      DC myDC;
      HDC hDC;
      POINT pt;
      char *s;
      s=(char*)GET_WINDOW_OFFSET(hWnd,0);
      
      if(hWnd->enabled) {
        switch(wParam) {
          case 'C':
            if(keypress.modifier & 0b00010001) {   // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
              if(OpenClipboard(hWnd)) {
                SetClipboardData(CF_TEXT,s);    // 
                CloseClipboard();
                }
              }
            break;
          case 'X':
            if(keypress.modifier & 0b00010001) {   // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
              if(OpenClipboard(hWnd)) {
                SetClipboardData(CF_TEXT,s);    // 
                *s=0;
                CloseClipboard();
                }
              }
            break;
          case 'V':
            if(keypress.modifier & 0b00010001) {   // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
              if(OpenClipboard(hWnd)) {
                if(GetClipboardData(CF_TEXT))    // 
                  SetWindowText(hWnd,GetClipboardData(CF_TEXT));
                CloseClipboard();
                }
              }
            break;
          case VK_RETURN:   // gestire tasti strani..
          default:
            buf[0]=wParam; buf[1]=0;
            hDC=GetDC(hWnd,&myDC);
            pt.x=GetWindowByte(hWnd,GWL_USERDATA+1) * hDC->font.size;   // curpos
            pt.y=1;   // curpos
            TextOut(hDC,pt.x*6*hDC->font.size,pt.y*8*hDC->font.size,buf);
            ReleaseDC(hWnd,hDC);
            if(pt.x<64   )       // curpos
              pt.x++;
            SetWindowByte(hWnd,GWL_USERDATA+1,pt.x);   // 
            break;
          }
      
        SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,EN_CHANGE),(LPARAM)hWnd);
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
        DC myDC;
        HDC hDC;
        GetClientRect(hWnd,&rc);
        hDC=GetDC(hWnd,&myDC);
        i=GetWindowByte(hWnd,GWL_USERDATA+0) * 6*hDC->font.size;   // curpos
        ReleaseDC(hWnd,hDC);
        i=rc.left+i;
        if(i>=rc.right)
          i=rc.right;
        drawCaret(hWnd,i,rc.top+1,standardCaret);
        SetTimer(hWnd,31,caretTime*10,NULL);
        }
      hWnd->focus=1;
      return 0;
      break;
    case WM_KILLFOCUS:
      KillTimer(hWnd,31);
      hWnd->focus=0;
      return 0;
      break;
    case WM_SETTEXT:
      {char *s;
      s=(char*)GET_WINDOW_OFFSET(hWnd,0);
      strncpy(s,(const char *)lParam,   64-1);
      s[   64-1]=0;
      SetWindowByte(hWnd,GWL_USERDATA+0,0);   // curpos ecc
      SetWindowByte(hWnd,GWL_USERDATA+1,0);   // curpos ecc
      SetWindowByte(hWnd,GWL_USERDATA+2,0);   // curpos ecc
      InvalidateRect(hWnd,NULL,TRUE);
      }
      return 1;
      break;

    case EM_CANUNDO:
      break;
    case EM_CHARFROMPOS:
      break;
    case EM_GETLINE:
      break;
    case EM_GETMARGINS:
      break;
    case EM_GETMODIFY:
      break;
    case EM_GETHANDLE:
      break;
    case EM_LIMITTEXT:
//    case EM_SETLIMITTEXT:
      break;
    case EM_LINEINDEX:
      break;
    case EM_LINEFROMCHAR:
      break;
    case EM_LINELENGTH:
      break;
    case EM_POSFROMCHAR:
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
    case EM_SETSEL:
      {char *s;
      int i;
      s=(char*)GET_WINDOW_OFFSET(hWnd,0);
      i=strlen(s);
      wParam=max(wParam,i);
      lParam=min(lParam,i);
      SetWindowByte(hWnd,GWL_USERDATA+1,wParam);
      SetWindowByte(hWnd,GWL_USERDATA+2,lParam);
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
      break;
    case WM_CUT:
      break;
    case WM_PASTE:
      break;
    case WM_CLEAR:
      break;
      
    case WM_GETDLGCODE: 
      break;

    case WM_TIMER:
      {
      DC myDC;
      HDC hDC=GetDC(hWnd,&myDC);
      RECT rc;
      BYTE x,y;
/*      GetClientRect(hWnd,&rc);    // fingo cursore :D
      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
      SetBkColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]);
			x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
//      FillRectangleWindow(hDC,rc.left+1+6*3+x*6,rc.top+1+y*8,rc.left+1+6*3+x*6+6,rc.top+1+y*8);
      TextOut(hDC,rc.left+1+6*strlen(prompt)+x*6,rc.top+1+y*8,GetWindowByte(hWnd,0) ? "_" : " ");
      SetWindowByte(hWnd,0,!GetWindowByte(hWnd,0));*/
      ReleaseDC(hWnd,hDC);
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
          SetBkColor(hDC,c);
          SetTextColor(hDC,(hWnd->active ? windowForeColor : windowInactiveForeColor));
					fillRectangleWindowColor(hDC,ps.rcPaint.left,y,ps.rcPaint.right,y+hWnd->font.size*8,c);
          }
        // in effetti Windows non sembra usare colore attivo/inattivo, ma sempre attivo; e quando la finestra si disattiva la riga pare deselezionarsi.
        else {
          SetTextColor(hDC,c);
          SetBkColor(hDC,(hWnd->active ? windowForeColor : windowInactiveForeColor));
					fillRectangleWindowColor(hDC,ps.rcPaint.left,y,ps.rcPaint.right,y+hWnd->font.size*8,
            hWnd->active ? windowForeColor : windowInactiveForeColor);
          }
        if(hWnd->style & LBS_USETABSTOPS)
          ;
        TextOut(hDC,x+1,y,item->data);    // finire :)
        y+=hWnd->font.size*8;
        drawHorizLineWindowColor(hDC,ps.rcPaint.left,y++,ps.rcPaint.right,windowForeColor);
        if(y>=ps.rcPaint.bottom)
          break;
        item=item->next;
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
        if(hWnd->enabled) {
          buf[0]=wParam; buf[1]=0;
          SendMessage(hWnd,LB_SELECTSTRING,0xffffffff,(LPARAM)buf);    // prova!!
          }
        else
          return DefWindowProc(hWnd,message,wParam,lParam);
      }
      return 0;
      break;
    case WM_CHARTOITEM:
      //Sent by a list box with the LBS_WANTKEYBOARDINPUT style to its owner in response to a WM_CHAR message.
      break;
    case WM_GETDLGCODE: 
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
        if(hWnd->parent)
          SendMessage(hWnd->parent,WM_NOTIFY,LBN_SELCHANGE,0);    // finire
        }
      if(!(hWnd->style & LBS_NOREDRAW))    // solo se cambiata?
        InvalidateRect(hWnd,NULL,TRUE);
      }
      break;
    case WM_LBUTTONDBLCLK:
      if(hWnd->style & LBS_NOSEL)
        ;
      if(hWnd->style & LBS_EXTENDEDSEL)
        ;
      if(hWnd->style & LBS_NOTIFY) {
        if(hWnd->parent)
          SendMessage(hWnd->parent,WM_NOTIFY,LBN_DBLCLK,0);    // finire
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
          rc.bottom /= i*(hWnd->font.size*8 +1);
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
          break;
        case 'C':
          if(!HDOK)
            goto no_disc;
          i=IDEFindFirst(lpPathSpec, attr, &rec);
          break;
        case 'D':
          break;
    #if defined(USA_USB_HOST_MSD)
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
      #if defined(USA_USB_HOST_MSD)
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
      item=item2=(LISTITEM *)GetWindowLong(hWnd,0);
      i=0;
      if(item) {
        if(lParam == 0xffffffff) {
fine_lista:
          while(item2 && item2->next) {
            i++;
            item2=item2->next;
            }
          }
        else {
          item2=getListItemFromCnt(item,wParam);
          if(!item2)
            goto fine_lista;
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
      hWnd->font.size=max(lParam/8,1);   // no, è per ownerdraw, singola riga :) ma ok
      goto updatelista;
      break;
    case LB_GETITEMHEIGHT:
      return hWnd->font.size*8;
      break;
    case LB_ITEMFROMPOINT:
      {int y;
      y=GET_Y_LPARAM(lParam);
      y /= hWnd->font.size*8+1;   // così al volo!
      return MAKELONG(y,  0 /*1 se fuori client area...?! forse scrollbar */);
      }
      break;
    case LB_GETITEMRECT:
      {RECT *rc;
      rc=(RECT*)lParam;
      GetClientRect(hWnd,rc);
      rc->top=wParam*(hWnd->font.size*8+1);
      // ovviamente finire... trovare chi è visualizzato e la posizione...
      rc->bottom=rc->top+(hWnd->font.size*8+1);
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
        hWnd->focus=1;
        return 0;
        }
      break;
    case WM_KILLFOCUS:
      if(hWnd->enabled) {
        hWnd->focus=0;
        return 0;
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
      
      HDC hDC=BeginPaint(hWnd,&ps);
      int c=hDC->brush.color;

      if(hWnd->style & WS_CHILD) {
        if((c=SendMessage(hWnd->parent,WM_CTLCOLOR /*WM_CTLCOLORBTN*/,
          (WPARAM)NULL,(LPARAM)hWnd)) == 0xffffffff) {
          c=hDC->brush.color;
          }
        }

      switch(hWnd->style & 0x0f) {
        case BS_PUSHBUTTON:   // BUTTON FINIRE ECC
        case BS_DEFPUSHBUTTON:   // BUTTON FINIRE ECC
          if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
            SetTextColor(hDC,WHITE);
            }
          else {
            SetTextColor(hDC,GRAY160);
            }
          SetBkColor(hDC,c);
          switch(hWnd->style & 0x0ff0) {
            case BS_LEFTTEXT:
              //boh
            case BS_LEFT:
              x=0; y=(ps.rcPaint.bottom-8)/2;
              if(y<0) y=0;
              TextOut(hDC,x,y,hWnd->caption);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              x=(ps.rcPaint.right-i)/2; y=(ps.rcPaint.bottom-hDC->font.size*8)/2;
              if(y<0) y=0;
              TextOut(hDC,x,y,hWnd->caption);
              break;
            case BS_RIGHT:
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              x=ps.rcPaint.right-i; y=(ps.rcPaint.bottom-hDC->font.size*8)/2;
              if(y<0) y=0;
              TextOut(hDC,x,y,hWnd->caption);
              break;
            case BS_TOP:
              x=(ps.rcPaint.right-i)/2; y=0;
              if(y<0) y=0;
              TextOut(hDC,x,y,hWnd->caption);
              break;
            case BS_BOTTOM:
              x=(ps.rcPaint.right-i)/2; y=ps.rcPaint.bottom-hDC->font.size*8;
              TextOut(hDC,x,y,hWnd->caption);
              break;
            case BS_VCENTER:
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              x=(ps.rcPaint.right-i)/2; y=(ps.rcPaint.bottom-hDC->font.size*8)/2;
              if(y<0) y=0;
              TextOut(hDC,x,y,hWnd->caption);
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
          hDC->pen.color=hWnd->active ? WHITE : GRAY192;
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
              TextOut(hDC,i+1,0,hWnd->caption);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              TextOut(hDC,ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size,0,hWnd->caption);
              break;
            case BS_TOP:
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_BOTTOM:
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,ps.rcPaint.bottom-hDC->font.size*8,hWnd->caption);
              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1)) {   // il secondo byte è lo stato
                drawLineWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.top+i);
                drawLineWindow(hDC,ps.rcPaint.left+i,ps.rcPaint.top,ps.rcPaint.left,ps.rcPaint.top+i);
                }
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,(ps.rcPaint.bottom-hDC->font.size*8)/2,hWnd->caption);
              break;
            case BS_ICON:
              drawIcon8(hDC,0,0,hWnd->icon);
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
              TextOut(hDC,0,0,hWnd->caption);
              break;
            case BS_TEXT:
              //boh
            case BS_CENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_RIGHT:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              TextOut(hDC,ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size,0,hWnd->caption);
              break;
            case BS_TOP:
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,0,hWnd->caption);
              break;
            case BS_BOTTOM:
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,ps.rcPaint.bottom-hDC->font.size*8,hWnd->caption);
              break;
            case BS_VCENTER:
              if(GetWindowByte(hWnd,GWL_USERDATA+1))    // il secondo byte è lo stato
                fillEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              else
                drawEllipseWindow(hDC,ps.rcPaint.left+1,ps.rcPaint.top+1,ps.rcPaint.left+i-1,ps.rcPaint.top+i-1);
              i=strlen(hWnd->caption)*6*hDC->font.size;
              if(i>=ps.rcPaint.right)
                i=ps.rcPaint.right;
              TextOut(hDC,(ps.rcPaint.right-strlen(hWnd->caption)*6*hDC->font.size)/2,(ps.rcPaint.bottom-hDC->font.size*8)/2,hWnd->caption);
              break;
            case BS_ICON:
              drawIcon8(hDC,0,0,hWnd->icon);
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
      if(hWnd->enabled) {
        if(hWnd->parent)
          SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
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
                SetWindowByte(hWnd,GWL_USERDATA+1,!GetWindowByte(hWnd,GWL_USERDATA+1));
                InvalidateRect(hWnd,NULL,TRUE);
                if(hWnd->parent)
                  SendMessage(hWnd->parent,WM_COMMAND,MAKELONG((WORD)(DWORD)hWnd->menu,BN_CLICKED),(LPARAM)hWnd);
                break;
              case VK_RETURN:
                break;
              default:  // hot key??
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
                  }
                break;
              }
            break;
          case BS_CHECKBOX:   // BUTTON FINIRE ECC
          case BS_AUTOCHECKBOX:   // BUTTON FINIRE ECC
            switch(wParam) {
              default:  // hot key??
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
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
                if(GetWindowByte(hWnd,GWL_USERDATA+2)) {   // il terzo byte è hotkey
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
//      SetWindowByte(hWnd,GWL_USERDATA+0 ,cs->style & 0x0f);   // salvo il tipo, anche se in effetti sarebbe anche in style..
      int i;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
      SetWindowByte(hWnd,GWL_USERDATA+1,!(hWnd->style & WS_DISABLED));    // stato

      }
      return 0;
      break;
    case WM_SETTEXT:
      InvalidateRect(hWnd,NULL,TRUE);
      return 1;
      break;
    case WM_GETDLGCODE: 
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

LRESULT DefWindowProcProgressWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:	// For some common controls, the default WM_PAINT message processing checks the wParam parameter. If wParam is non-NULL, the control assumes that the value is an HDC and paints using that device context.
    { 
      int i,x,y;
      PAINTSTRUCT ps;
      HDC hDC=BeginPaint(hWnd,&ps);
      BYTE attrib=GetWindowByte(hWnd,0);
      GFX_COLOR f=GetWindowWord(hWnd,GWL_USERDATA+0),b=GetWindowWord(hWnd,GWL_USERDATA+2);
      
      if(attrib & PBS_MARQUEE) {
        fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+rand() % i,ps.rcPaint.bottom,    // finire con marquee vero :D
          f);
        }
      else {
        i=GetWindowByte(hWnd,3);
        if(attrib & 1)
          sprintf(hWnd->caption,"%u%%",i);
        i=((ps.rcPaint.right-ps.rcPaint.left)*(i-GetWindowByte(hWnd,1))) /
          (GetWindowByte(hWnd,2)-GetWindowByte(hWnd,1));
        fillRectangleWindowColor(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.left+i,ps.rcPaint.bottom,
          f);
        if(attrib & 1) {
          i=strlen(hWnd->caption)*6*hDC->font.size;
          x=ps.rcPaint.right-ps.rcPaint.left;
          y=ps.rcPaint.bottom-ps.rcPaint.top;
          if(i>=x)
            i=x;
          x=(x-i)/2; y=(y-8*hDC->font.size)/2;     // usare FONT..
          SetTextColor(hDC,WHITE /*textColors[GetWindowByte(hWnd,4) & 15]*/);
          SetBkColor(hDC,b);
          TextOut(hDC,x,y,hWnd->caption);
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
      SetWindowByte(hWnd,0,0);    // attrib
      SetWindowByte(hWnd,1,0); SetWindowByte(hWnd,2,100);   // default
      SetWindowByte(hWnd,3,0);    // valore
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(windowForeColor,windowForeColor/2));   // colori f/b
      }
      return 0;
      break;
    case PBM_SETRANGE:
      SetWindowByte(hWnd,1,LOWORD(lParam));    // 
      SetWindowByte(hWnd,2,HIWORD(lParam));    // 
      InvalidateRect(hWnd,NULL,TRUE);
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


LRESULT DefWindowProcFileDlgWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      SearchRec rec;
      SYS_FS_FSTAT stat;
      SYS_FS_HANDLE myFileHandle;
      int i; 
      UGRAPH_COORD_T x,y;
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,DWL_USER);
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
            TextOut(hDC,ps.rcPaint.left+10,ps.rcPaint.top+10,"Inserire disco:");
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
        if(GetWindowByte(hWnd,GWL_USERDATA+2)<2)
          SetWindowByte(hWnd,GWL_USERDATA+2,2);
        
loop:
					totFiles++;
          if(y<ps.rcPaint.bottom) {
            switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
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
                  drawIcon8(hDC,x+18,y+4,folderIcon8);
                  }
                else {
                  drawIcon8(hDC,x+18,y+4,fileIcon8);
                  }
                x+=40;
                if(x>=ps.rcPaint.right) {
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
                if(x>=ps.rcPaint.right) {
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

        switch(GetWindowByte(hWnd,GWL_USERDATA+1)) {
          case 0:
            if(((totFiles*32)/40)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*32)/40),FALSE);
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
            if(((totFiles*60)/40)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*60)/40),FALSE);
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
          y+=32;
          break;
        case 1:
          y+=40;
          break;
        case 2:
          y+=10;
          break;
        }
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

      if(hWnd->parent && !hWnd->parent->internalState)
        SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_INITDONE /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser

fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,DWL_USER);
      if(!GetWindowByte(hWnd,GWL_USERDATA+2)) {
        TextOut(hDC,10,0,"attendere prego...");
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
      OPENFILENAME *ofn=(OPENFILENAME *)GET_WINDOW_DLG_OFFSET(hWnd,DWL_USER);
			memset(ofn,sizeof(OPENFILENAME),0);
      }
      return 0;
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_SPACE:
          if(hWnd->parent && !hWnd->parent->internalState)
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
    case WM_GETDLGCODE: 
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

LRESULT DefWindowProcDirWC(HWND hWnd,uint16_t message,WPARAM wParam,LPARAM lParam) {
  
  switch(message) {
    case WM_PAINT:
      {
      PAINTSTRUCT ps;
      SYS_FS_FSTAT stat;
      SYS_FS_HANDLE myFileHandle;
      SearchRec rec;
      int i; 
      UGRAPH_COORD_T x,y;
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      char label[16 /*TOTAL_FILE_SIZE+1*/];
      WORD totFiles;
      
      HDC hDC=BeginPaint(hWnd,&ps);

      if(!GetWindowByte(hWnd,GWL_USERDATA+1))
        fillRectangleWindow(hDC,ps.rcPaint.left,ps.rcPaint.top,ps.rcPaint.right,ps.rcPaint.bottom);
      totFiles=0;

      switch(dfn->disk) {
        case 'A':
          if(!SDcardOK) {
no_disc:
            SetWindowByte(hWnd,GWL_USERDATA+1,1);
            SetWindowText(hWnd,"Disco");
            TextOut(hDC,ps.rcPaint.left+10,ps.rcPaint.top+10,"Inserire disco:");
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
        if(GetWindowByte(hWnd,GWL_USERDATA+1)<2)
          SetWindowByte(hWnd,GWL_USERDATA+1,2);
        
loop:
					totFiles++;
          if(y<ps.rcPaint.bottom) {
            switch(GetWindowByte(hWnd,GWL_USERDATA+0)) {
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
                  drawIcon8(hDC,x+18,y+4,folderIcon8);
                  }
                else {
                  drawIcon8(hDC,x+18,y+4,fileIcon8);
                  }
                x+=40;
                if(x>=ps.rcPaint.right) {
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
                if(x>=ps.rcPaint.right) {
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

        switch(GetWindowByte(hWnd,GWL_USERDATA+0)) {
          case 0:
            if(((totFiles*32)/40)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*32)/40),FALSE);
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
            if(((totFiles*60)/40)>(ps.rcPaint.bottom-ps.rcPaint.top)) {
              hWnd->style |= WS_VSCROLL;
//              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*8)/(ps.rcPaint.bottom-ps.rcPaint.top)),FALSE); // int math :)
              SetScrollRange(hWnd,SB_VERT,0,(ps.rcPaint.bottom-ps.rcPaint.top)*(ps.rcPaint.bottom-ps.rcPaint.top) / ((totFiles*60)/40),FALSE);
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

      switch(GetWindowByte(hWnd,GWL_USERDATA+0)) {
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
      if(GetWindowByte(hWnd,GWL_USERDATA+1)<3) {
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
        SetWindowByte(hWnd,GWL_USERDATA+1,3);

        sprintf(buf,"%s(%c) %s",*label ? label : "Disk",toupper(dfn->disk),*dfn->path ? dfn->path : ASTERISKS);
        SetWindowText(hWnd,buf);
        }
      else
        TextOut(hDC,4,y+2,"?"); 
      TextOut(hDC,64,y+2," Kbytes free");

      if(hWnd->parent && !hWnd->parent->internalState)
        SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_INITDONE /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser

fine:      
      EndPaint(hWnd,&ps);
      }

      return 1;
      break;
    case WM_ERASEBKGND:
      {HDC hDC=(HDC)wParam;
      
      DrawFrameControl(hDC,&hWnd->paintArea,0,hDC->brush.color);
      
      SetTextColor(hDC,WHITE);
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
      if(!GetWindowByte(hWnd,GWL_USERDATA+1)) {
        TextOut(hDC,10,0,"attendere prego...");
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
      DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
			memset(dfn,sizeof(DIRLIST),0);
      }
      return 0;
      break;
      
    case WM_CHAR:
      switch(wParam) {
        case VK_SPACE:
          if(hWnd->parent && !hWnd->parent->internalState)
            SendMessage(hWnd->parent,WM_NOTIFY,0,CDN_FILEOK /*NMHDR struct*/); // beh tanto per :) anche se è per file dialog chooser
          // in effetti sa
          break;
        case 'I':
          SendMessage(hWnd,WM_NOTIFY,0,CDN_TYPECHANGE);
          break;
          
        default:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
        }
      break;
    case WM_GETDLGCODE: 
      break;

    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // dice https://learn.microsoft.com/en-us/windows/win32/menurc/wm-command anche se non mi sembra di averlo mai fatto...    
        DIRLIST *dfn=(DIRLIST *)GET_WINDOW_OFFSET(hWnd,0);
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

          case 16+1:
	          SetWindowByte(hWnd,GWL_USERDATA+0,0);		// icone grandi
            break;
          case 16+2:
	          SetWindowByte(hWnd,GWL_USERDATA+0,1);		// icone piccole
            break;
          case 16+3:
	          SetWindowByte(hWnd,GWL_USERDATA+0,2);		// dettagli
            break;
          case 16+4:
	          // aggiorna.. :) sempre
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
          SetWindowByte(hWnd,GWL_USERDATA+0,(GetWindowByte(hWnd,GWL_USERDATA+0)+1) % 3);
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
            DestroyWindow(hWnd);
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
      if(keypress.modifier & 0b00000001) {   // ALT-sx 
        return DefWindowProc(hWnd,message,wParam,lParam);
        }
      GetClientRect(hWnd,&rc);
//      SetTextColor(hDC,textColors[GetWindowByte(hWnd,GWL_USERDATA+2) & 15]);
			x=GetWindowByte(hWnd,GWL_USERDATA+0); y=GetWindowByte(hWnd,GWL_USERDATA+1);
      if(keypress.modifier & 0b00010001)    // CTRL per accelerator... si potrebbero lasciar passare quelli non riconosciuti
//        if(!matchAccelerator(hWnd->menu,LOBYTE(wParam)))
        return DefWindowProc(hWnd,message,wParam,lParam);
      switch(wParam) {
        case VK_F10:
          return DefWindowProc(hWnd,message,wParam,lParam);
          break;
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
          if(y>=rc.bottom/8) {
            y--;
            ScrollWindow(hWnd,0,-8,&rc,NULL);
            }
    			SetWindowByte(hWnd,GWL_USERDATA+0,x); SetWindowByte(hWnd,GWL_USERDATA+1,y);
          KillTimer(hWnd,1);
          
          hDC=GetDC(hWnd,&myDC);
//      TextOut(hDC,rc.left+1+6*strlen(prompt)+x*6,rc.top+1+y*8," "); FARE per pulire cursore!
          m_stdout=/*m_stdin=*/m_stderr=DEVICE_WINDOW;
          cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
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
          if(isprint(ch)) {
            cmdline=(char *)GET_WINDOW_OFFSET(hWnd,5);
            i=strlen(cmdline);
            if(i < 31) {
              cmdline[i++]=keypress.modifier & 0b00010001 ? toupper(ch) : tolower(ch);
              // in effetti in windows arrivano solo maiuscole e poi gli SHIFT a parte...
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
//fare          x=0;
          break;
        case 0x0c:
          SelectObject(hDC,OBJ_BRUSH,(GDIOBJ)CreateSolidBrush(textColors[GetWindowByte(hWnd,GWL_USERDATA+3) & 15]));
          FillRect(hDC,&rc,hDC->brush); 
          // o fare semplicemente un InvalidateRect(mInstance->hWnd,NULL,TRUE) ??
          DeleteObject(OBJ_PEN,(GDIOBJ)hDC->brush); //bah ok per ora
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
              x=0;
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
      BYTE attrib=GetWindowByte(hWnd,sizeof(S_POINT)*16+16+4+4);
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
        hDC->font=GetStockObject(SYSTEM_FIXED_FONT).font;

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
      BYTE attrib=GetWindowByte(hWnd,sizeof(S_POINT)*16+16+4+4);
      GFX_COLOR f=GetWindowWord(hWnd,GWL_USERDATA+0),b=GetWindowWord(hWnd,GWL_USERDATA+2);
			char *fileWallpaper=(char *)GET_WINDOW_OFFSET(hWnd,sizeof(S_POINT)*16);
      GRAPH_COORD_T img_ofs_x,img_ofs_y;
      jpegPtr=NULL;

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
          SelectObject(hDC,OBJ_FONT,(GDIOBJ)CreateFont(80,60,0,0,FW_NORMAL /*FW_BOLD*/,FALSE,FALSE,FALSE,ANSI_CHARSET,
            OUT_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_QUALITY,FIXED_PITCH | FF_ROMAN,NULL));
          img_ofs_y=(hWnd->paintArea.bottom-56)/2; img_ofs_x=(hWnd->paintArea.right-strlen(fileWallpaper)*48)/2;
          SetTextColor(hDC,BRIGHTYELLOW);
          SetBkColor(hDC,desktopColor);
          TextOut(hDC,img_ofs_x,img_ofs_y,fileWallpaper);
          DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);

//          printf("%u\n",hDC->font.size);
          

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
            img_ofs_y=(hWnd->paintArea.bottom-JPG_Info.m_height)/2; img_ofs_x=(hWnd->paintArea.right-JPG_Info.m_width)/2;
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
										// USARE BBitBlt(img_ofs_x+x1,img_ofs_y+y1,8,8, )  !
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
            if(img_ofs_x>hWnd->paintArea.right+JPG_Info.m_width) {
              img_ofs_x=0;
              img_ofs_y+=JPG_Info.m_height;
              if(img_ofs_y<hWnd->paintArea.bottom+JPG_Info.m_height)
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
          
      DeleteObject(OBJ_FONT,(GDIOBJ)hDC->font);
      }
      return 1;
      break;
    case WM_CREATE:
      {CREATESTRUCT *cs=(CREATESTRUCT *)lParam;
      SetWindowLong(hWnd,GWL_USERDATA+0,0);    // azzero 
			memset(GET_WINDOW_OFFSET(hWnd,0),sizeof(S_POINT)*16+16+4+4+1,0);
      if(cs->lpCreateParams)
        strncpy(GET_WINDOW_OFFSET(hWnd,sizeof(S_POINT)*16),cs->lpCreateParams,15);
			jpegPtr=(DWORD *)GET_WINDOW_OFFSET(hWnd,sizeof(S_POINT)*16+16+4+4+1);
      SetWindowLong(hWnd,GWL_USERDATA,MAKELONG(BRIGHTYELLOW,desktopColor));
      }
      return 0;
      break;
      
    case WM_COMMAND:
      if(HIWORD(wParam) == 0) {   // menu
        switch(LOWORD(wParam)) { 
          case 1:
    				MessageBox(NULL,"FileManager","futuro",MB_OK);
            break;
          case 2:
    				MessageBox(NULL,"Pannello di controllo","futuro",MB_OK);
            break;
          case 3:
    				SendMessage(taskbarWindow,WM_COMMAND,MAKELONG(SC_TASKLIST,0),0);
            break;
          case 4:
    				MessageBox(NULL,"Calcolatrice","futuro",MB_OK);
            break;
          case 5:
    				MessageBox(NULL,"Orologio","futuro",MB_OK);
            break;
          case 6:
    				MessageBox(NULL,"Esegui","futuro",MB_OK);
            break;
          case 7:
    				PostMessage(NULL,WM_QUIT,0,0);
            // uscire da tutto...
            break;
          }
        return 1;
        }
      break;
    case WM_LBUTTONDOWN:
      if(activeMenu==&menuStart) {   // FINIRE! 
        int x,y;
        x=LOWORD(lParam); y=HIWORD(lParam); // qua in effetti fa lo stesso, se è un popupmenu su schermo
        x+=hWnd->clientArea.left;
        y+=hWnd->clientArea.top;
        if(PtInRect(&activeMenuRect,MAKEPOINT(x,y))) {
          StdBeep(100);
          activeMenuWnd=desktopWindow;    //truschino v. taskbar trackpopup
          SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
          }
        else {
          SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
          }
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
        // clock
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
			memset(GET_WINDOW_OFFSET(hWnd,0),4+4+1,0);
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
          WS_DISABLED | SS_CENTER,  // in effetti potrebbe essere attivo per set clock..
          cs->cx-5*6-4,0,5*6+2,10,
          hWnd,(HMENU)2,NULL
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
          case SC_TASKLIST:
//            CreateDialog(hWnd,
    				MessageBox(NULL,"Task Manager","futuro",MB_OK);
            break;
          }
        }
      if(HIWORD(wParam) == BN_CLICKED) {   // è 0!! coglioni :D
        switch(LOWORD(wParam)) { 
          case 1:
/* o NO? bah in effetti direi SEMPRE*/            if(activeMenu==&menuStart) {
              activeMenuWnd=desktopWindow;    //truschini per chiudere bene!
              SendMessage(desktopWindow,WM_MENUSELECT,MAKELONG(0,0xffff),0);
              }
            else {
              if(TrackPopupMenu((HMENU)&menuStart,TPM_RETURNCMD | TPM_BOTTOMALIGN | TPM_LEFTALIGN | TPM_LEFTBUTTON,
                1,hWnd->nonClientArea.top /*mousePosition.x,mousePosition.y qua fisso!*/,0,desktopWindow /*hWnd*/,NULL)) {
                  }
              }
            break;
          case 2:   // orologio, FARE BUTTON!
            SendMessage(desktopWindow,WM_COMMAND,MAKELONG(5,0),0);
            break;
          }
        }
      return 1;
      break;
    case WM_LBUTTONDOWN:
// idem SEMPRE!           if(activeMenu==&menuStart) {
      SendMessage(hWnd,WM_MENUSELECT,MAKELONG(0,0xffff),0);
      break;
    case WM_TIMER:
      {
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
            SetCursor(hourglassCursorSm);
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
			TextOut(hDC,0,0,"Video unavailable");
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
      break;
    case WM_RBUTTONDOWN:
      // impostazioni video...
      break;

    default:
      return DefWindowProc(hWnd,message,wParam,lParam);
      break;
    }
  return 0;
  }

