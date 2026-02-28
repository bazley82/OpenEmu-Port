#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "resource.h"
#include "../../common/sound.h"
#include "../../common/memorymap.h"
#include "../../common/supervision.h"
#include "../../common/controls.h"
#include "../../common/types.h"

int scrSize = 2;
int finished = 0;
int execute = 0;
int romloaded = 0;
int autoframeskipenab = 0;
int frameskiprate = 0;
unsigned int romlenght;

/*  Make the class name into a global variable  */
char szClassName[ ] = "PotatorSDL";

static HWND window;
static HDC hdc,bitmaphdc;
static HBITMAP hbitmap;
BITMAPV4HEADER bmp_info;

HMENU menu;
DWORD threadID;
HANDLE runthread=INVALID_HANDLE_VALUE;

uint8* buffer;
unsigned int buffer_size = 0;

unsigned short screenbuffer[161*161];

const char tabkeytext[48][8] = {"0","1","2","3","4","5","6","7","8","9","A","B","C",
"D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X",
"Y","Z","SPACE","UP","DOWN","LEFT","RIGHT","TAB","SHIFT","DEL","INSERT","HOME","END","ENTER"};
const DWORD tabkey[48]={0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x41,0x42,0x43,
0x44,0x45,0x46,0x47,0x48,0x49,0x4a,0x4b,0x4c,0x4d,0x4e,0x4f,0x50,0x51,0x52,0x53,0x54,0x55,
0x56,0x57,0x58,0x59,0x5a,VK_SPACE,VK_UP,VK_DOWN,VK_LEFT,VK_RIGHT,VK_TAB,VK_SHIFT,VK_DELETE,
VK_INSERT,VK_HOME,VK_END,VK_RETURN};


//This is a modified version of the code used in
//DeSmuME and is Copyright DeSmuME team 2005-2007
DWORD WINAPI run( LPVOID lpParameter)
{
     char txt[80];
     BITMAPV4HEADER bmi;
     int wait=0;
     u64 freq;
     u64 OneFrameTime;
     int framestoskip=0;
     int framesskipped=0;
     int skipnextframe=0;
     u64 lastticks=0;
     u64 curticks=0;
     u64 diffticks=0;
     u32 framecount=0;
     u64 onesecondticks=0;
     int fps=0;
     int fpsframecount=0;
     u64 fpsticks=0;

     //CreateBitmapIndirect(&bmi);
     memset(&bmi, 0, sizeof(bmi));
     bmi.bV4Size = sizeof(bmi);
     bmi.bV4Planes = 1;
     bmi.bV4BitCount = 16;
     bmi.bV4V4Compression = BI_RGB|BI_BITFIELDS;
     bmi.bV4RedMask = 0x001F;
     bmi.bV4GreenMask = 0x03E0;
     bmi.bV4BlueMask = 0x7C00;
     bmi.bV4Width = 160;
     bmi.bV4Height = -160;

     QueryPerformanceFrequency((LARGE_INTEGER *)&freq);
     QueryPerformanceCounter((LARGE_INTEGER *)&lastticks);
     OneFrameTime = freq / 60;

     while(!finished)
     {
          while(execute)
          {
               //controls_update();

               supervision_exec((int16*)screenbuffer,1);
               
               //controls_reset();

               if (!skipnextframe)
               {
                  #if SIZE==1
			      SetDIBitsToDevice(hdc, 0, 0, 160*scrSize, 160*scrSize, 0, 0, 0, 160*scrSize, screenbuffer, (BITMAPINFO*)&bmi, DIB_RGB_COLORS);
			      #else
                  RECT r ;
			      GetClientRect(window,&r) ;
                  StretchDIBits (hdc, 0, 0, r.right-r.left, r.bottom-r.top, 0, 0, 160, 160, screenbuffer, (BITMAPINFO*)&bmi, DIB_RGB_COLORS,SRCCOPY);
			      #endif
                  fpsframecount++;
                  QueryPerformanceCounter((LARGE_INTEGER *)&curticks);
                  if(curticks >= fpsticks + freq)
                  {
                     fps = fpsframecount;
                     sprintf(txt,"Potator %dfps (%i %%)", fps*(frameskiprate+1), (int)((fps*100)/60)*(frameskiprate+1));
                     SetWindowText(window, txt);
                     fpsframecount = 0;
                     QueryPerformanceCounter((LARGE_INTEGER *)&fpsticks);
                  }

                  framesskipped = 0;

                  if (framestoskip > 0)
                     skipnextframe = 1;
               }
               else
               {
                  framestoskip--;

                  if (framestoskip < 1)
                     skipnextframe = 0;
                  else
                     skipnextframe = 1;

                  framesskipped++;
               }

               if (autoframeskipenab)
               {
                  framecount++;

                  if (framecount > 60)
                  {
                     framecount = 1;
                     onesecondticks = 0;
                  }

                  QueryPerformanceCounter((LARGE_INTEGER *)&curticks);
                  diffticks = curticks-lastticks;

                  if ((onesecondticks+diffticks) > (OneFrameTime * (u64)framecount) &&
                      framesskipped < 9)
                  {                     
                     // Skip the next frame
                     skipnextframe = 1;
 
                     // How many frames should we skip?
                     framestoskip = 1;
                  }
                  else if ((onesecondticks+diffticks) < (OneFrameTime * (u64)framecount))
                  {
                     // Check to see if we need to limit speed at all
                     for (;;)
                     {
                        QueryPerformanceCounter((LARGE_INTEGER *)&curticks);
                        diffticks = curticks-lastticks;
                        if ((onesecondticks+diffticks) >= (OneFrameTime * (u64)framecount))
                           break;
                     }
                  }

                  onesecondticks += diffticks;
                  lastticks = curticks;
               }
               else
               {
                  if (framestoskip < 1)
                     framestoskip = frameskiprate + 1;
               }

               if(controls_read(0)!=0) controls_state_write(0,0);
               Sleep(0);
          }
          execute = FALSE;
          Sleep(500);
     }
     return 1;
}

void loadROM(char* filename)
{
	u32 length;
	
	if(romloaded) supervision_reset();

	if (buffer != 0)
		free(buffer);

	FILE *romfile = fopen(filename, "rb");
	
	if (romfile == (FILE *)-1)
		printf("fopen(): Unable to open file!\n");

	fseek(romfile, 0, SEEK_END);
	buffer_size = ftell(romfile);
	fseek(romfile, 0, SEEK_SET);

	buffer = (unsigned char *)malloc(buffer_size);

	uint32 bytesread = fread(buffer, buffer_size, 1, romfile);

	if (!fclose(romfile))
		printf("fclose(): Unable to close file!\n");
	
	romloaded = 1;
}

#define mMENUSIZE(OPTION1,OPTION2,OPTION3,OPTION4)			   \
		CheckMenuItem(menu,IDM_SIZE1,OPTION1); \
		CheckMenuItem(menu,IDM_SIZE2,OPTION2); \
		CheckMenuItem(menu,IDM_SIZE3,OPTION3); \
		CheckMenuItem(menu,IDM_SIZE4,OPTION4); 

void set_size(int refresh)
{
    HMENU	hMenu=GetMenu(window);	 
    RECT	wind;
    switch (scrSize)
    {
	    case 1:	mMENUSIZE(MF_CHECKED,MF_UNCHECKED,MF_UNCHECKED,MF_UNCHECKED);break;
	    case 2: mMENUSIZE(MF_UNCHECKED,MF_CHECKED,MF_UNCHECKED,MF_UNCHECKED);break;
	    case 3: mMENUSIZE(MF_UNCHECKED,MF_UNCHECKED,MF_CHECKED,MF_UNCHECKED);break;
	    case 4: mMENUSIZE(MF_UNCHECKED,MF_UNCHECKED,MF_UNCHECKED,MF_CHECKED);break;
    }

    wind.top=0;wind.left=0;wind.right=160*scrSize;wind.bottom=160*scrSize;
    AdjustWindowRect(&wind,WS_SIZEBOX|WS_OVERLAPPEDWINDOW|WS_VISIBLE,TRUE);
    SetWindowPos(window,NULL,0,0,wind.right-wind.left,wind.bottom-wind.top,SWP_NOMOVE|SWP_NOZORDER|SWP_SHOWWINDOW);
}

LRESULT CALLBACK WndProc(HWND wnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg) {
                
		case WM_COMMAND:
             switch(LOWORD(wParam))
             {
                  case IDM_OPEN:
                       {
                            OPENFILENAME ofn;
                            char filename[MAX_PATH] = "";
                            execute=FALSE; //Stop emulation while opening new rom
                            
                            ZeroMemory(&ofn, sizeof(ofn));
                            ofn.lStructSize = sizeof(ofn);
                            ofn.hwndOwner = window;
                            ofn.lpstrFilter = "Supervision files (*.sv)\0*.sv\0\0";
                            ofn.nFilterIndex = 1;
                            ofn.lpstrFile =  filename;
                            ofn.nMaxFile = MAX_PATH;
                            ofn.lpstrDefExt = "sv";
                            
                            if(!GetOpenFileName(&ofn))
                            {
                                 if (romloaded)
                                    execute=TRUE;
                                 return 0;
                            }

                            loadROM(filename);
                            supervision_load((u8*)buffer, (uint32)buffer_size);
                            execute=TRUE;
                       }
                  break;
                  
                  case IDM_WEBSITE:
                       ShellExecute(NULL, "open", "http://normmatt.com", NULL, NULL, SW_SHOWNORMAL);
                  break;
                  
                  case IDM_ABOUT:
                       MessageBox(NULL,"Potator 0.8\nCopyright(C) Normmatt 2007",szClassName,MB_ICONEXCLAMATION|MB_OK);
                  break;
                  
                  case IDM_SIZE1:
                  case IDM_SIZE2:
                  case IDM_SIZE3:
                  case IDM_SIZE4:
                       scrSize=LOWORD(wParam)-IDM_SIZE1+1;
                       set_size(1);
                  break;
                  
                  case IDM_QUIT:
                       PostMessage(window, WM_QUIT, 0, 0);
                  break;
             }
        break;
        
        case WM_KEYDOWN:
        {
             //if(wParam=='1'){PostMessage(window, WM_DESTROY, 0, 0);return 0;}
             if(wParam=='0'){frameskiprate=0;return 0;}
             if(wParam=='1'){frameskiprate=1;return 0;}
             if(wParam=='2'){frameskiprate=2;return 0;}
             if(wParam=='3'){frameskiprate=3;return 0;}
             if(wParam=='4'){frameskiprate=4;return 0;}
             if(wParam=='5'){frameskiprate=5;return 0;}
             if(wParam=='6'){frameskiprate=6;return 0;}
             if(wParam=='7'){frameskiprate=7;return 0;}
             
             if(wParam==VK_UP){
             controls_state_write(1,0x08);
             return 0; }
             if(wParam==VK_DOWN){
             controls_state_write(1,0x04);
             return 0; }
             if(wParam==VK_LEFT){
             controls_state_write(1,0x02);
             return 0; }
             if(wParam==VK_RIGHT){
             controls_state_write(1,0x01);
             return 0; }
             if(wParam==0x58){ //x
             controls_state_write(1,0x10);
             return 0; }
             if(wParam==0x43){ //c
             controls_state_write(1,0x20);
             return 0; }
             if(wParam==VK_F1){
             controls_state_write(1,0x40);
             return 0; }
             if(wParam==VK_F2){
             controls_state_write(1,0x80);
             return 0; }
        }
        break;
        
        /*case WM_KEYUP:
        {
             if(wParam==VK_UP){
             controls_state_write(0,0);
             return 0; }
             if(wParam==VK_DOWN){
             controls_state_write(0,0);
             return 0; }
             if(wParam==VK_LEFT){
             controls_state_write(0,0);
             return 0; }
             if(wParam==VK_RIGHT){
             controls_state_write(0,0);
             return 0; }
             if(wParam==0x58){ //x
             controls_state_write(0,0);
             return 0; }
             if(wParam==0x43){ //c
             controls_state_write(0,0);
             return 0; }
             if(wParam==VK_F1){
             controls_state_write(0,0);
             return 0; }
             if(wParam==VK_F2){
             controls_state_write(0,0);
             return 0; }
        }
        break;*/
		
		case WM_CLOSE: DestroyWindow(wnd); return 0;
		case WM_DESTROY: PostQuitMessage(0); return 0;
		default: return DefWindowProc(wnd, msg, wParam, lParam);
	}
	return FALSE;
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)

{
    int i;
    HWND hwnd;               /* This is the handle for our window */
    MSG messages;            /* Here messages to the application are saved */
    WNDCLASSEX wincl={0};        /* Data structure for the windowclass */
    RECT rect={0};
    
    InitCommonControls();
    
    /* init hardware */
	supervision_init(); //Init the emulator	

    /* The Window structure */
    wincl.hInstance = hInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WndProc;      /* This function is called by windows */
    wincl.style = 0;                 /* Catch double-clicks */
    wincl.cbSize = sizeof (WNDCLASSEX);

    /* Use default icon and mouse-pointer */
    wincl.hIcon = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon (NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor (NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;                 /* No menu */
    wincl.cbClsExtra = 0;                      /* No extra bytes after the window class */
    wincl.cbWndExtra = 0;                      /* structure or the window instance */
    /* Use Windows's default color as the background of the window */
    wincl.hbrBackground = (HBRUSH) COLOR_BACKGROUND;

    /* Register the window class, and if it fails quit the program */        
    if(!RegisterClassEx(&wincl)) { 
        MessageBox(NULL,"Window registration failed!",szClassName,MB_ICONEXCLAMATION|MB_OK);
        return 1;
    }


    /* create window */
	AdjustWindowRectEx(&rect,WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION|WS_VISIBLE,0,WS_EX_CLIENTEDGE);
	window=CreateWindowEx(WS_EX_CLIENTEDGE,szClassName,szClassName,WS_SYSMENU|WS_MINIMIZEBOX|WS_CAPTION|WS_VISIBLE,CW_USEDEFAULT,CW_USEDEFAULT,(rect.right-rect.left)+160*scrSize,(rect.bottom-rect.top)+160*scrSize,NULL,NULL,hInstance,NULL);
	if(window==NULL) { MessageBox(NULL,"Window creation failed!",szClassName,MB_ICONEXCLAMATION|MB_OK); return 1; }
	
	hdc=GetDC(window);
	
	menu = LoadMenu(hInstance, "MENU_PRINCIPAL");
	SetMenu(window, menu);
	
	ShowWindow(window,nCmdShow);
	UpdateWindow(window);
	
	runthread = CreateThread(NULL, 0, run, NULL, 0, &threadID);
	
	switch (scrSize)
    {
	    case 1:	mMENUSIZE(MF_CHECKED,MF_UNCHECKED,MF_UNCHECKED,MF_UNCHECKED);break;
	    case 2: mMENUSIZE(MF_UNCHECKED,MF_CHECKED,MF_UNCHECKED,MF_UNCHECKED);break;
	    case 3: mMENUSIZE(MF_UNCHECKED,MF_UNCHECKED,MF_CHECKED,MF_UNCHECKED);break;
	    case 4: mMENUSIZE(MF_UNCHECKED,MF_UNCHECKED,MF_UNCHECKED,MF_CHECKED);break;
    }
	
	//SetTimer(window,0,1,NULL); /* inaccurate for strict timing; 16ms should be 60hz, but that results in slowness here */


    /* Run the message loop. It will run until GetMessage() returns 0 */
    while (GetMessage (&messages, NULL, 0, 0))
    {
        /* Translate virtual-key messages into character messages */
        TranslateMessage(&messages);
        /* Send message to WindowProcedure */
        DispatchMessage(&messages);
    }
    
	DeleteObject(hbitmap);
	//DeleteDC(bitmaphdc);
	ReleaseDC(window,hdc);
	
	//timeEndPeriod(1);
    
    UnregisterClass(wincl.lpszClassName,hInstance);

    /* The program return-value is 0 - The value that PostQuitMessage() gave */
    return messages.wParam;
}
