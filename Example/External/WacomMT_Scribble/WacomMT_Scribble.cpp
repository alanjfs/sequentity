/*----------------------------------------------------------------------------
NAME
   WacomMT_Scribble.cpp

PURPOSE
   Sample code showing how to use the Wacom Feel(TM) Multi-Touch API and
   the Wintab32 API.

COPYRIGHT
   Copyright (c) Wacom Technology Corp., 2012-2014 All Rights Reserved
   All rights reserved.

---------------------------------------------------------------------------- */

#include "stdafx.h"
#include "WacomMT_Scribble.h"

#include <vector>
#include <map>
#include <utility>
#include <algorithm>
#include <string>
#include <sstream>

#include "WacomMultiTouch.h"
#include "WintabUtils.h"

///////////////////////////////////////////////////////////////////////////////
// Wintab support headers
//
#define PACKETDATA   (PK_X | PK_Y | PK_BUTTONS | PK_NORMAL_PRESSURE)
#define PACKETMODE   PK_BUTTONS
#include "pktdef.h"

// #define DRAW_CONTACT_RECT
// #define TRACE_FINGER_DATA
//#define TRACE_PEN_DATA

// #define TEST_FINGER_DROP_PACKETS
//#define TRACE_TO_FRAME_LOGFILE

#define MAX_LOADSTRING 100

// Small factor to render display tablet finger circles.
// to pixels assuming .27 pixel size. Sould use system api to get this value.
#define  DISPLAY_TAB_DRAW_SIZE_FACTOR     0.27f

#if defined(TRACE_FINGER_DATA)
#define FINGER_DATA_TRACE(...)  WacomTrace(__VA_ARGS__)
#else
#define FINGER_DATA_TRACE(...)
#endif

#if defined(TRACE_PEN_DATA)
#define PEN_DATA_TRACE(...)  WacomTrace(__VA_ARGS__)
#else
#define PEN_DATA_TRACE(...)
#endif

typedef std::unique_ptr<WacomMTHitRect> WacomMTHitRectPtr;

// Global Variables:
HINSTANCE                        hInst = NULL;
std::wstring                     szTitle = L"WacomMT_Scribble Pen, Consumer, Finger, HWND";
std::wstring                     szWindowClass = L"WACOMMT_SCRIBBLE";
HWND                             g_mainWnd = NULL;
HDC                              g_hdc = NULL;
HWND                             g_hWndAbout = NULL;

// Cached client rect (system coordinates).
// Used for evaluating whether or not to render pen data by verifying whether
// the returned pen data (sys coords) falls within the client rect. Returned 
// touch contact locations use this rect to interpolate where they should be drawn.
// Similar interpolation done for raw and blob data rendering as well.
// This rect needs to be updated when the app is moved or resized.
RECT                             g_clientRect = {0, 0, 0, 0};

bool                             g_ShowTouchSize = true;
bool                             g_ShowTouchID = false;

std::map<int, WacomMTCapability> g_caps;
std::vector<int>                 g_devices;
HCTX                             g_tabCtx = NULL;
std::map<int, COLORREF>          g_fingerColorMap;
HBRUSH                           g_noConfidenceBrush = NULL;
std::map<int, WacomMTHitRectPtr> g_lastWTHitRect;

bool                             g_useConfidenceBits = true;
bool                             g_ObserverMode = false;
enum EDataType
{
   ENoData,
   EFingerData,
   EBlobData,
   ERawData
}                                g_DataType = EFingerData;
bool                             g_UseHWND = true;
bool                             g_UseWinHitRect = true;

CRITICAL_SECTION g_graphicsCriticalSection;

namespace WacomTraceFrame
{
#if defined(TRACE_TO_FRAME_LOGFILE)
   std::vector<std::pair<int, std::vector<unsigned short>>> g_frameHistoryBuffer;
   FILE *g_frameLogFile = NULL;

   char *MakeLogFileName()
   {
      SYSTEMTIME syst = {0};
      GetLocalTime(&syst);

      // Select file to save samples
      static char strCSVFileName[MAX_PATH] = {0};
      sprintf(strCSVFileName, "%04d%02d%02d_%02d%02d%02d%04d.csv", syst.wYear, syst.wMonth, syst.wDay, syst.wHour, syst.wMinute, syst.wSecond, syst.wMilliseconds);
      return strCSVFileName;
   }

   void SaveFrameHistory(int cols, int rows)
   {
      g_frameLogFile = fopen(MakeLogFileName(), "w+");
      if (g_frameLogFile)
      {
         int index = 0;
         for ( std::vector<std::pair<int, std::vector<unsigned short>>>::iterator iter = g_frameHistoryBuffer.begin();
               iter != g_frameHistoryBuffer.end();
               ++iter)
         {
            fprintf(g_frameLogFile, "FRAME:%d, INDEX:%d\n", iter->first, index++);
            for (int y = 0; y < rows; y++)
            {
               for (int x = 0; x < cols; x++)
               {
                  unsigned short val = iter->second[y*cols+x];
                  if (val > 0)
                  {
                     fprintf(g_frameLogFile, "%d,", val);
                  }
                  else
                  {
                     fprintf(g_frameLogFile, " ,");
                  }
               }
               fprintf(g_frameLogFile, "\n");
            }
            fprintf(g_frameLogFile, "\n");
         }
         fclose(g_frameLogFile);
         g_frameLogFile = NULL;
      }
   }

   void SaveTraceFrame(int count, unsigned short *buffer, int frameNumber)
   {
      std::vector<unsigned short> frame(count);
      std::copy(buffer, buffer + count, frame.begin());
      g_frameHistoryBuffer.push_back(std::make_pair(frameNumber, frame));
   }
#else
   void SaveFrameHistory(int, int) {}
   void SaveTraceFrame(int, unsigned short*, int) {}
#endif
}

//---------------------------------------------------------------------
namespace TCStats
{
   std::map<int, LARGE_INTEGER>  tcStatsMap;

   void AddFingerID(int fingerID_I)
   {
      if (!tcStatsMap.count(fingerID_I))
      {
         LARGE_INTEGER perfCount = {0};
         if (::QueryPerformanceCounter(&perfCount))
         {
            tcStatsMap[fingerID_I] = perfCount;
         }
      }
   }

   LARGE_INTEGER GetCurrentCount(int fingerID_I)
   {
      LARGE_INTEGER retval = {0};
      if (tcStatsMap.count(fingerID_I))
      {
         retval = tcStatsMap[fingerID_I];
      }

      return retval;
   }

   void RemoveFingerID(int fingerID_I)
   {
      if (tcStatsMap.count(fingerID_I))
      {
         tcStatsMap.erase(fingerID_I);
      }
   }

   void UpdateFinger(const WacomMTFinger& finger)
   {
      if (finger.TouchState == WMTFingerStateDown)
      {
         AddFingerID(finger.FingerID);
      }
      else if (finger.TouchState == WMTFingerStateUp)
      {
         LARGE_INTEGER freq = {0};
         QueryPerformanceFrequency(&freq);

         LARGE_INTEGER nowCount = {0};
         QueryPerformanceCounter(&nowCount);

         LARGE_INTEGER curCount = GetCurrentCount(finger.FingerID);

         double elapse = (double)(nowCount.QuadPart - curCount.QuadPart);
         elapse /= freq.QuadPart;
         elapse *= 1000;

         RemoveFingerID(finger.FingerID);
      }
   }
}

namespace DropPacketTests
{
#if defined(TEST_FINGER_DROP_PACKETS)
   int g_NumDownCounts = 0;

   void TestFingerDropPackets(WacomMTFingerState state, int count)
   {
      if (state == WMTFingerStateDown)
      {
         g_NumDownCounts++;
      }
      else if (state == WMTFingerStateUp)
      {
         g_NumDownCounts--;
         if ((count == 1) && (g_NumDownCounts != 0))
         {
            std::stringstream msg;
            msg << "OOPS - unbalanced Down/Up states: " << "[" << g_NumDownCounts << "] "
               << (g_NumDownCounts > 0 ? "Missing Up states" : "Missing Down states");
            WacomTrace(msg.str().c_str());
            g_NumDownCounts = 0;
         }
      }
   }
#else
   void TestFingerDropPackets(WacomMTFingerState, int) {}
#endif
}

//---------------------------------------------------------------------

// Forward declarations of functions included in this code module:
ATOM              MyRegisterClass(HINSTANCE hInstance);
BOOL              InitInstance(HINSTANCE, int);
LRESULT CALLBACK  WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK  About(HWND, UINT, WPARAM, LPARAM);
void              ClearScreen();

///////////////////////////////////////////////////////////////////////////////
// Multi-touch API support functions.
WacomMTHitRectPtr GetAppHitRect();
int FingerCallback(WacomMTFingerCollection *fingerData, void *userData);
int BlobCallback(WacomMTBlobAggregate *blobData, void *userData);
int RawCallback(WacomMTRawData *rawData, void *userData);
void AttachCallback(WacomMTCapability deviceInfo, void *userRef);
void DetachCallback(int deviceID, void *userRef);
void DrawFingerData(int count, WacomMTFinger *fingers, int device);
void DrawBlobData(int count, WacomMTBlob *blobs, int device);
void DrawRawData(int count, unsigned short* rawBuf, int device);
void DumpCaps(int deviceID);
bool ClientHitRectChanged(const WacomMTHitRectPtr& wtHitRect_I, int deviceID);
WacomMTError RegisterForData(int deviceID);
WacomMTError MoveCallback(int deviceID);
WacomMTError UnregisterForData(int deviceID);

///////////////////////////////////////////////////////////////////////////////
BOOL Circle(HDC hDC, int x, int y, int r)
{
   return Ellipse(hDC, x - r, y - r, x + r, y + r);
}

BOOL CenterEllipse(HDC hDC, int x, int y, int w, int h)
{
   return Ellipse(hDC, x - w, y - h, x + w, y + h);
}

///////////////////////////////////////////////////////////////////////////////
WacomMTProcessingMode CurrentMode(void)
{
   return g_ObserverMode ? WMTProcessingModeObserver : WMTProcessingModeNone;
}

std::wstring GetTitle(void)
{
   std::wstring title = L"WacomMT_Scribble Pen";
   title.append(L", ");
   title.append(g_ObserverMode ? L"Observer" : L"Consumer");
   title.append(L", ");
   switch (g_DataType)
   {
      case ENoData:
         title.append(L"No Touch");
      break;
      case EFingerData:
         title.append(L"Finger");
      break;
      case EBlobData:
         title.append(L"Blob");
      break;
      case ERawData:
         title.append(L"Raw");
      break;
   }
   title.append(L", ");
   title.append(g_UseHWND ? L"HWND" : g_UseWinHitRect ? L"Windowed" : L"Full Screen");
   return title;
}

///////////////////////////////////////////////////////////////////////////////
// Wintab support functions.
HCTX InitWintabAPI(HWND hwnd_I);
void DrawPenData(POINT point_I, UINT pressure_I, bool bMoveToPoint_I);
void Cleanup( void );

int APIENTRY _tWinMain(HINSTANCE hInstance,
                       HINSTANCE hPrevInstance,
                       LPTSTR    lpCmdLine,
                       int       nCmdShow)
{
   UNREFERENCED_PARAMETER(hPrevInstance);
   UNREFERENCED_PARAMETER(lpCmdLine);

   //TODO - add command options to show all three data types: eg: -t == show touch; -r == show raw, -b == show blob,

   // TODO: Place code here.
   MSG msg;
   HACCEL hAccelTable;

   InitializeCriticalSection(&g_graphicsCriticalSection);

   // Initialize global strings
   MyRegisterClass(hInstance);

   // Perform application initialization:
   if (!InitInstance (hInstance, nCmdShow))
   {
      return FALSE;
   }

   hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_WACOMMT_SCRIBBLE));

   // Main message loop:
   while (GetMessage(&msg, NULL, 0, 0))
   {
      if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
      {
         TranslateMessage(&msg);
         DispatchMessage(&msg);
      }
   }

   WacomMTQuit();

   if (g_noConfidenceBrush)
   {
      DeleteObject(g_noConfidenceBrush);
   }

   DeleteCriticalSection(&g_graphicsCriticalSection);

   return (int)msg.wParam;
}

//
//  FUNCTION: MyRegisterClass()
//
//  PURPOSE: Registers the window class.
//
//  COMMENTS:
//
//    This function and its usage are only necessary if you want this code
//    to be compatible with Win32 systems prior to the 'RegisterClassEx'
//    function that was added to Windows 95. It is important to call this function
//    so that the application will get 'well formed' small icons associated
//    with it.
//
ATOM MyRegisterClass(HINSTANCE hInstance)
{
   WNDCLASSEX wcex;

   wcex.cbSize = sizeof(WNDCLASSEX);

   wcex.style           = CS_HREDRAW | CS_VREDRAW;
   wcex.lpfnWndProc     = WndProc;
   wcex.cbClsExtra      = 0;
   wcex.cbWndExtra      = 0;
   wcex.hInstance       = hInstance;
   wcex.hIcon           = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_WACOMMT_SCRIBBLE));
   wcex.hCursor         = LoadCursor(NULL, IDC_ARROW);
   wcex.hbrBackground   = (HBRUSH)(COLOR_WINDOW+1);
   wcex.lpszMenuName    = MAKEINTRESOURCE(IDC_WACOMMT_SCRIBBLE);
   wcex.lpszClassName   = szWindowClass.c_str();
   wcex.hIconSm         = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

   return RegisterClassEx(&wcex);
}

//
//   FUNCTION: InitInstance(HINSTANCE, int)
//
//   PURPOSE: Saves instance handle and creates main window
//
//   COMMENTS:
//
//        In this function, we save the instance handle in a global variable and
//        create and display the main program window.
//
BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   g_mainWnd = CreateWindow(szWindowClass.c_str(),
      szTitle.c_str(),
      WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT,
      0,
      CW_USEDEFAULT,
      0,
      NULL,
      NULL,
      hInstance,
      NULL );
   if (!g_mainWnd)
   {
      return FALSE;
   }

   g_hdc = GetDC(g_mainWnd);

   // Create a brush to fill in non-confident contact ellipses.
   g_noConfidenceBrush = CreateSolidBrush(RGB(255,128,0));     // orange

   WacomMTError res = WacomMTInitialize(WACOM_MULTI_TOUCH_API_VERSION);
   if (res == WMTErrorSuccess)
   {
      int deviceCount = WacomMTGetAttachedDeviceIDs(NULL, 0);
      if (deviceCount)
      {
         int newCount = 0;
         while (newCount != deviceCount)
         {
            g_devices.resize(deviceCount, 0);
            newCount = WacomMTGetAttachedDeviceIDs(&g_devices[0], deviceCount * sizeof(int));
         }

         int loopCount = (int)g_devices.size();
         for (int idx = 0; idx < loopCount; idx++)
         {
            WacomMTCapability cap = {0};
            res = WacomMTGetDeviceCapabilities(g_devices[idx], &cap);
            g_caps[g_devices[idx]] = cap;
            DumpCaps(g_devices[idx]);
         }
      }
      res = WacomMTRegisterAttachCallback(AttachCallback, NULL);
      res = WacomMTRegisterDetachCallback(DetachCallback, NULL);

      int loopCount = (int)g_devices.size();
      for (int idx = 0; idx < loopCount; idx++)
      {
         res = RegisterForData(g_devices[idx]);
      }
   }

   nCmdShow = SW_MAXIMIZE;
   ShowWindow(g_mainWnd, nCmdShow);
   UpdateWindow(g_mainWnd);

   return TRUE;
}

//
//  FUNCTION: WndProc(HWND, UINT, WPARAM, LPARAM)
//
//  PURPOSE:  Processes messages for the main window.
//
//  WM_COMMAND - process the application menu
//  WM_PAINT   - Paint the main window
//  WM_DESTROY - post a quit message and return
//
//
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   switch (message)
   {
      case WM_CREATE:
         {
            WINDOWINFO appWindowInfo = {0};
            appWindowInfo.cbSize = sizeof(appWindowInfo);
            GetWindowInfo(hWnd, &appWindowInfo);
            g_clientRect = appWindowInfo.rcClient;
         }
         g_tabCtx = InitWintabAPI(hWnd);
         if (!g_tabCtx)
         {
            ShowError("Could Not Open Tablet Context.");
            SendMessage(hWnd, WM_DESTROY, 0, 0L);
         }

//       SetTimer(hWnd, 1, 1000, NULL);
      break;

      case WM_TIMER:
//    {
//       DWORD serialNumber = 0;
//       DWORD baseType = 0;
//       gpWTInfoA(WTI_CURSORS + (-5), CSR_PHYSID, &serialNumber);
//       gpWTInfoA(WTI_CURSORS + (-5), CSR_TYPE, &baseType);
//       WacomTrace("Cursor Data %08lX,%08lX\n", sn, tp);
//    }
      return DefWindowProc(hWnd, message, wParam, lParam);

      case WM_CLOSE:
//       KillTimer(hWnd, 1);
      return DefWindowProc(hWnd, message, wParam, lParam);

      case WM_KEYDOWN:
         switch(wParam)
         {
            case VK_ESCAPE:
               ClearScreen();
            break;

            case 'S':
               WacomTraceFrame::SaveFrameHistory(g_caps[0].ScanSizeX, g_caps[0].ScanSizeY);
            break;

            case 'P':
            break;

            case VK_LEFT:
            break;

            case VK_RIGHT:
            break;

            default:
            break;
         }
      break;

      case WM_COMMAND:
      {
         HMENU menu = GetMenu(g_mainWnd);
         WORD wmId = LOWORD(wParam);
         WORD wmEvent = HIWORD(wParam);
         // Parse the menu selections:
         switch (wmId)
         {
            case IDM_ABOUT:
               if (!IsWindow(g_hWndAbout))
               {
                  CreateDialog(hInst, MAKEINTRESOURCE(IDD_ABOUTBOX), hWnd, About);
                  ShowWindow(g_hWndAbout, SW_SHOW);
               }
            break;

            case IDM_OPTIONS_USECONFIDENCEBITS:
               g_useConfidenceBits = !g_useConfidenceBits;
               CheckMenuItem(menu, IDM_OPTIONS_USECONFIDENCEBITS, (g_useConfidenceBits ? MF_CHECKED : MF_UNCHECKED));
               ClearScreen();
            break;

            case IDM_OBSERVER:
            case IDM_CONSUMER:
            {
               bool newMode = wmId == IDM_OBSERVER;
               if (g_ObserverMode != newMode)
               {
                  for (size_t idx = 0; idx < g_devices.size(); idx++)
                  {
                     UnregisterForData(g_devices[idx]);
                  }
                  g_ObserverMode = newMode;
                  CheckMenuItem(menu, IDM_OBSERVER, (g_ObserverMode ? MF_CHECKED : MF_UNCHECKED));
                  CheckMenuItem(menu, IDM_CONSUMER, (g_ObserverMode ? MF_UNCHECKED : MF_CHECKED));
                  for (size_t idx = 0; idx < g_devices.size(); idx++)
                  {
                     RegisterForData(g_devices[idx]);
                  }
               }
               SetWindowTextW(hWnd, GetTitle().c_str());
               ClearScreen();
            }
            break;

            case IDM_SHOW_TOUCH_SIZE:
            case IDM_SHOW_TOUCH_ID:
            {
               g_ShowTouchSize = false;
               g_ShowTouchID = false;

               g_ShowTouchSize = ( wmId == IDM_SHOW_TOUCH_SIZE );
               g_ShowTouchID = ( wmId == IDM_SHOW_TOUCH_ID );

               CheckMenuItem(menu, IDM_SHOW_TOUCH_SIZE, (g_ShowTouchSize ? MF_CHECKED : MF_UNCHECKED));
               CheckMenuItem(menu, IDM_SHOW_TOUCH_ID, (g_ShowTouchID ? MF_CHECKED : MF_UNCHECKED));

               ClearScreen();
            }
            break;

            case IDM_FINGER:
            case IDM_BLOB:
            case IDM_RAW:
            {
               EDataType typeHit = (wmId == IDM_FINGER) ? EFingerData : (wmId == IDM_BLOB) ? EBlobData : ERawData;
               for (size_t idx = 0; idx < g_devices.size(); idx++)
               {
                  UnregisterForData(g_devices[idx]);
               }
               if (g_DataType == typeHit)
               {
                  g_DataType = ENoData;
               }
               else
               {
                  g_DataType = typeHit;
               }
               CheckMenuItem(menu, IDM_FINGER, (g_DataType == EFingerData ? MF_CHECKED : MF_UNCHECKED));
               CheckMenuItem(menu, IDM_BLOB, (g_DataType == EBlobData ? MF_CHECKED : MF_UNCHECKED));
               CheckMenuItem(menu, IDM_RAW, (g_DataType == ERawData ? MF_CHECKED : MF_UNCHECKED));
               for (size_t idx = 0; idx < g_devices.size(); idx++)
               {
                  RegisterForData(g_devices[idx]);
               }
               SetWindowTextW(hWnd, GetTitle().c_str());
               ClearScreen();
            }
            break;

            case IDM_WINDOW_HANDLES:
            {
               for (size_t idx = 0; idx < g_devices.size(); idx++)
               {
                  UnregisterForData(g_devices[idx]);
               }
               g_UseHWND = !g_UseHWND;
               CheckMenuItem(menu, IDM_WINDOW_HANDLES, (g_UseHWND ? MF_CHECKED : MF_UNCHECKED));
               CheckMenuItem(menu, IDM_WINDOW_RECT, (g_UseHWND || g_UseWinHitRect ? MF_CHECKED : MF_UNCHECKED));
               EnableMenuItem(menu, IDM_WINDOW_RECT, (g_UseHWND ? MF_GRAYED : MF_ENABLED));
               for (size_t idx = 0; idx < g_devices.size(); idx++)
               {
                  RegisterForData(g_devices[idx]);
               }
               SetWindowTextW(hWnd, GetTitle().c_str());
               ClearScreen();
            }
            break;

            case IDM_WINDOW_RECT:
            {
               for (size_t idx = 0; idx < g_devices.size(); idx++)
               {
                  UnregisterForData(g_devices[idx]);
               }
               g_UseWinHitRect = !g_UseWinHitRect;
               CheckMenuItem(menu, IDM_WINDOW_RECT, (g_UseWinHitRect ? MF_CHECKED : MF_UNCHECKED));
               for (size_t idx = 0; idx < g_devices.size(); idx++)
               {
                  RegisterForData(g_devices[idx]);
               }
               SetWindowTextW(hWnd, GetTitle().c_str());
               ClearScreen();
            }
            break;

            case IDM_ERASE:
            {
               ClearScreen();
            }
            break;

            case IDM_EXIT:
               DestroyWindow(hWnd);
            break;

            default:
            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      }
      break;


      case WM_PAINT:
      {
         PAINTSTRUCT ps = {0};
         HDC hdc = BeginPaint(hWnd, &ps);
         // HACK - forcing a "no-op" LineTo allows display to refresh pen data, upon getting
         // this WM_PAINT message due to the InvalidateRect() call in DrawPenData().
         // If also drawing due to finger ellipses, then this hack wouldn't be needed, but
         // we want to use the pen by itself (no touch data).
         // Hack seems to need to do a drawing operation; MoveToEx by itself doesn't work.
         LineTo(hdc, 0, 0);
         EndPaint(hWnd, &ps);
      }
      break;

      case WM_SETTINGCHANGE:
         if (lParam)
         {
            WacomTrace("WM_SETTINGCHANGE %i, %S\n", wParam, lParam);
         }
         else
         {
            WacomTrace("WM_SETTINGCHANGE %i, NULL\n", wParam);
         }
      break;

      case WM_DESTROY:
      {
         ReleaseDC(hWnd, g_hdc);
         if (g_tabCtx)
         {
            gpWTClose(g_tabCtx);
            g_tabCtx = 0;
         }

         // Return Wintab and MTAPI resources.
         Cleanup();

         PostQuitMessage(0);
      }
      break;

      case WM_SIZE:
      case WM_MOVE:
         {
            WINDOWINFO appWindowInfo = {0};
            appWindowInfo.cbSize = sizeof(appWindowInfo);
            GetWindowInfo(hWnd, &appWindowInfo);
            g_clientRect = appWindowInfo.rcClient;
         }
         if (g_UseHWND)
         {
            // Make sure there's an attached touch tablet.
            for each(int deviceID in g_devices)
            {
               if (g_caps.count(deviceID) && (g_caps[deviceID].Type == WMTDeviceTypeIntegrated))
               {
                  // resend hitrect size and position to MTAPI
                  MoveCallback(deviceID);
               }
            }
         }
         else
         {
            return DefWindowProc(hWnd, message, wParam, lParam);
         }
      break;

      case WM_FINGERDATA:
         DrawFingerData(((WacomMTFingerCollection*)lParam)->FingerCount, ((WacomMTFingerCollection*)lParam)->Fingers, ((WacomMTFingerCollection*)lParam)->DeviceID);
      break;

      case WM_BLOBDATA:
         DrawBlobData(((WacomMTBlobAggregate*)lParam)->BlobCount, ((WacomMTBlobAggregate*)lParam)->BlobArray, ((WacomMTBlobAggregate*)lParam)->DeviceID);
      break;

      // Capture pen data.
      // Note that the data is being sent in system coordinates.
      case WT_PACKET:
      {
         PACKET wintabPkt;
         static POINT ptOld = {0};
         static POINT ptNew = {0};
         static UINT prsOld = 0;
         static UINT prsNew = 0;

         if (gpWTPacket((HCTX)lParam, wParam, &wintabPkt))
         {
            ptNew.x = wintabPkt.pkX;
            ptNew.y = wintabPkt.pkY;
            prsNew = wintabPkt.pkNormalPressure;

            if ((ptNew.x != ptOld.x) || (ptNew.y != ptOld.y))
            {
               bool bMoveToPoint = ((prsOld == 0) && (prsNew > 0));
               PEN_DATA_TRACE("prsOld: %i, prsNew: %i, ptNew: [%i,%i], ptOld: [%i,%i], moveToPoint: %s\n",
                  prsOld, prsNew,
                  ptNew.x, ptNew.y,
                  ptOld.x, ptOld.y,
                  (bMoveToPoint ? "Move" : "Draw"));
               DrawPenData(ptNew, prsNew, bMoveToPoint);

               // Keep track of last time we did move or draw.
               ptOld = ptNew;
               prsOld = prsNew;
            }
         }
      }
      break;

      default:
      return DefWindowProc(hWnd, message, wParam, lParam);
   }
   return 0;
}

// Message handler for about box.
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
   UNREFERENCED_PARAMETER(lParam);
   switch (message)
   {
      case WM_INITDIALOG:
         g_hWndAbout = hDlg;
         for each(int deviceID in g_devices)
         {
            if (g_caps.count(deviceID))
            {
               WacomMTRegisterFingerReadHWND(deviceID, WMTProcessingModePassThrough, g_hWndAbout, 5);
            }
         }
      return (INT_PTR)TRUE;

      case WM_COMMAND:
         if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL))
         {
            for each(int deviceID in g_devices)
            {
               if (g_caps.count(deviceID))
               {
                  WacomMTUnRegisterFingerReadHWND(g_hWndAbout);
               }
            }
            DestroyWindow(hDlg);
            g_hWndAbout = NULL;
            return (INT_PTR)TRUE;
         }
      break;
   }
   return (INT_PTR)FALSE;
}

WacomMTError RegisterForData(int deviceID)
{
   WacomMTError res = WMTErrorInvalidParam;

   WacomMTHitRectPtr wtHitRect;
   if (g_caps[deviceID].Type == WMTDeviceTypeIntegrated)
   {
      wtHitRect = GetAppHitRect();
   }

   switch (g_DataType)
   {
      case EFingerData:
         if (g_UseHWND)
         {
            res = WacomMTRegisterFingerReadHWND(deviceID, CurrentMode(), g_mainWnd, 5);
         }
         else
         {
            res = WacomMTRegisterFingerReadCallback(deviceID, wtHitRect.get(), CurrentMode(), FingerCallback, NULL);
         }
      break;

      case EBlobData:
         if (g_UseHWND)
         {
            res = WacomMTRegisterBlobReadHWND(deviceID, CurrentMode(), g_mainWnd, 5);
         }
         else
         {
            res = WacomMTRegisterBlobReadCallback(deviceID, wtHitRect.get(), CurrentMode(), BlobCallback, NULL);
         }
      break;

      case ERawData:
         res = WacomMTRegisterRawReadCallback(deviceID, CurrentMode(), RawCallback, NULL);
      break;
   }

   g_lastWTHitRect[deviceID] = std::move(wtHitRect);

   return res;
}

WacomMTError MoveCallback(int deviceID)
{
   WacomMTError res = WMTErrorInvalidParam;

   WacomMTHitRectPtr wtHitRect = GetAppHitRect();
   if (ClientHitRectChanged(wtHitRect, deviceID))
   {
      switch (g_DataType)
      {
         case EFingerData:
            // move registered callback from prev hit rect to new hit rect
            res = WacomMTMoveRegisteredFingerReadCallback(deviceID, g_lastWTHitRect[deviceID].get(), CurrentMode(), wtHitRect.get(), NULL);
         break;

         case EBlobData:
            // move registered callback from prev hit rect to new hit rect
            res = WacomMTMoveRegisteredBlobReadCallback(deviceID, g_lastWTHitRect[deviceID].get(), CurrentMode(), wtHitRect.get(), NULL);
         break;

         case ERawData:
            res = WacomMTRegisterRawReadCallback(deviceID, CurrentMode(), RawCallback, NULL);
         break;
      }

      g_lastWTHitRect[deviceID] = std::move(wtHitRect);
   }

   return res;
}

WacomMTError UnregisterForData(int deviceID)
{
   WacomMTError res = WMTErrorInvalidParam;

   switch (g_DataType)
   {
      case EFingerData:
         if (g_UseHWND)
         {
            res = WacomMTUnRegisterFingerReadHWND(g_mainWnd);
         }
         else
         {
            res = WacomMTUnRegisterFingerReadCallback(deviceID, g_lastWTHitRect[deviceID].get(), CurrentMode(), NULL);
         }
      break;

      case EBlobData:
         if (g_UseHWND)
         {
            res = WacomMTUnRegisterBlobReadHWND(g_mainWnd);
         }
         else
         {
            res = WacomMTUnRegisterBlobReadCallback(deviceID, g_lastWTHitRect[deviceID].get(), CurrentMode(), NULL);
         }
      break;

      case ERawData:
         res = WacomMTUnRegisterRawReadCallback(deviceID, CurrentMode(), NULL);
      break;
   }

   g_lastWTHitRect[deviceID].reset();

   return res;
}

int FingerCallback(WacomMTFingerCollection *fingerData, void *userData)
{
   if (fingerData)
   {
      DrawFingerData(fingerData->FingerCount, fingerData->Fingers, fingerData->DeviceID);
   }
   return 0;
}

int BlobCallback(WacomMTBlobAggregate *blobData, void *userData)
{
   if (blobData)
   {
      DrawBlobData(blobData->BlobCount, blobData->BlobArray, blobData->DeviceID);
   }
   return 0;
}

int RawCallback(WacomMTRawData *rawData, void *userData)
{
   // rawData->ElementCount should equal caps.ScanX times caps.ScanY
   if (rawData)
   {
      WacomTraceFrame::SaveTraceFrame(rawData->ElementCount, rawData->Sensitivity, rawData->FrameNumber);
      DrawRawData(rawData->ElementCount, rawData->Sensitivity, rawData->DeviceID);
   }
   return 0;
}

void AttachCallback(WacomMTCapability deviceInfo, void *userRef)
{
   if (!g_caps.count(deviceInfo.DeviceID))
   {
      g_devices.push_back(deviceInfo.DeviceID);
      g_caps[deviceInfo.DeviceID] = deviceInfo;
      WacomMTError res = RegisterForData(deviceInfo.DeviceID);
      if (res != WMTErrorSuccess)
      {
         return;
      }
   }
}

void DetachCallback(int deviceID, void *userRef)
{
   if (g_caps.count(deviceID))
   {
      UnregisterForData(deviceID);
      std::vector<int>::const_iterator iter = std::find(g_devices.begin(), g_devices.end(), deviceID);
      if (iter != g_devices.end())
      {
         g_devices.erase(iter);
      }
      g_caps.erase(deviceID);
   }
}

void Rotate(double degrees, const POINT& centerPnt, std::vector<POINT>& points)
{
   double rad = (90 - degrees) * 3.14159 / 180.0;

   for (UINT idx = 0; idx < points.size(); idx++)
   {
      points[idx].x = (long)(cos(rad) * (points[idx].x - centerPnt.x) - (sin(rad) * (points[idx].y - centerPnt.y)) + centerPnt.x);
      points[idx].y = (long)(sin(rad) * (points[idx].x - centerPnt.x) + (cos(rad) * (points[idx].y - centerPnt.y)) + centerPnt.y);
   }
}

void DrawFingerData(int count, WacomMTFinger *fingers, int device)
{
   if (g_devices.size() && count && fingers)
   {
      EnterCriticalSection(&g_graphicsCriticalSection);

      assert(g_hdc);

      for (int index = 0; index < count; index++)
      {
         FINGER_DATA_TRACE("TC[%i], confidence: %i\n", fingers[index].FingerID, fingers[index].Confidence);

         DropPacketTests::TestFingerDropPackets(fingers[index].TouchState, count);

         if (!g_fingerColorMap.count(fingers[index].FingerID))
         {
            g_fingerColorMap[fingers[index].FingerID] = RGB(rand()%255, rand()%255, rand()%255);
         }
         HPEN pen = CreatePen(PS_SOLID, 2, g_fingerColorMap[fingers[index].FingerID]);
         HPEN oldPen = (HPEN)SelectObject(g_hdc, pen);

         TCStats::UpdateFinger(fingers[index]);

         if (fingers[index].TouchState != WMTFingerStateNone)
         {
            // Skip this finger if using confidence bits and it's NOT confident.
            if (g_useConfidenceBits && !fingers[index].Confidence)
            {
               continue;
            }

            // Display tablets report position in pixels.
            double x = fingers[index].X;
            double y = fingers[index].Y;
            if (g_caps[device].Type == WMTDeviceTypeOpaque)
            {
               // If we're using an opaque tablet, then the X and Y values are not in
               // pixels; they are a percentage of the tablet width (eg: 0.123, 0.37, etc.).
               // We need to convert to client pixels.
               x *= g_clientRect.right - g_clientRect.left;
               x += g_clientRect.left;
               y *= g_clientRect.bottom - g_clientRect.top;
               y += g_clientRect.top ;
            }

            if ((x > g_clientRect.left) && (x < g_clientRect.right) &&
                (y > g_clientRect.top)  && (y < g_clientRect.bottom))
            {
               //map to our window
               POINT pt = {(LONG)x, (LONG)y};
               ::ScreenToClient(g_mainWnd, &pt);

               // If width and height are not supported; we will fake it.
               double widthMM = 0.;
               if (fingers[index].Width > 0)
               {
                  // Make larger for visibility, if necessary.
                  if (fingers[index].Width <= 1.0)
                  {
                     // Convert logical value to millimeters by multiplying by physical width.
                     widthMM = fingers[index].Width * g_caps[device].PhysicalSizeX;
                  }
                  else //must be Cintiq so width already in pixels
                  {
                     widthMM = fingers[index].Width * DISPLAY_TAB_DRAW_SIZE_FACTOR; //pixel pitch of Cintiq 24 HD
                  }
               }
               int contactWidthOffset = (int)(widthMM / DISPLAY_TAB_DRAW_SIZE_FACTOR / 2);

               double heightMM = 0.;
               if (fingers[index].Height > 0)
               {
                  // Make larger for visibility, if necessary.
                  if (fingers[index].Height <= 1.0)
                  {
                     // Convert logical value to millimeters by multiplying by physical height.
                     heightMM = fingers[index].Height * g_caps[device].PhysicalSizeY;
                  }
                  else //must be Cintiq so height already in pixels
                  {
                     heightMM = fingers[index].Height * DISPLAY_TAB_DRAW_SIZE_FACTOR; //pixel pitch of Cintiq 24 HD
                  }
               }
               int contactHeightOffset = (int)(heightMM / DISPLAY_TAB_DRAW_SIZE_FACTOR / 2);

               FINGER_DATA_TRACE("width, height (mm): %3.3f,%3.3f\n", widthMM, heightMM);

               // Draw a larger ellipse around essentially a dot.
               // Fill in any con-confident contacts.
               {
                  HBRUSH oBrush = 0;
                  if (!fingers[index].Confidence)
                  {
                     oBrush = (HBRUSH)SelectObject(g_hdc, g_noConfidenceBrush);
                  }

#if defined(DRAW_CONTACT_RECT)
                  {
                     std::vector<POINT> rectPoints(5);

                     rectPoints[0].x = pt.x - contactWidthOffset;
                     rectPoints[0].y = pt.y - contactHeightOffset;
                     rectPoints[1].x = pt.x + contactWidthOffset;
                     rectPoints[1].y = pt.y - contactHeightOffset;
                     rectPoints[2].x = pt.x + contactWidthOffset;
                     rectPoints[2].y = pt.y + contactHeightOffset;
                     rectPoints[3].x = pt.x - contactWidthOffset;
                     rectPoints[3].y = pt.y + contactHeightOffset;
                     rectPoints[4].x = pt.x - contactWidthOffset;
                     rectPoints[4].y = pt.y - contactHeightOffset;

                     int pntCount = rectPoints.size();
                     Rotate(fingers[index].Orientation, pt, rectPoints);
                     PolyPolygon(g_hdc, &rectPoints[0], &pntCount, 1);
                  }
#endif

                  CenterEllipse(g_hdc, pt.x, pt.y, contactWidthOffset, contactHeightOffset);

                  {
                     wchar_t displayText[8] = L"";
                     if ( g_ShowTouchSize )
                     {
                        _stprintf(displayText, TEXT("%.1f"), widthMM);
                     }
                     else if ( g_ShowTouchID )
                     {
                        _stprintf(displayText, TEXT("%i"), fingers[index].FingerID);
                     }
                     // else bubble will be blank
                     TextOut(g_hdc, pt.x, pt.y, displayText, _tcslen(displayText));
                  }

                  if (!fingers[index].Confidence)
                  {
                     SelectObject(g_hdc, oBrush);
                  }
               }

               // Display finger stats in upper left corner.
               {
                  wchar_t fingerStr[128] = L"";
                  _stprintf( fingerStr, TEXT("Finger:%d ID:%d Xtab:%.2f Ytab:%.2f W:%.2f [%.2f mm]  H:%.2f [%.2f mm]  Angle:%.0f      \n"),
                     index, fingers[index].FingerID, fingers[index].X, fingers[index].Y,
                     fingers[index].Width, widthMM, fingers[index].Height, heightMM, fingers[index].Orientation );
                  TextOut(g_hdc, 50, 20, fingerStr, _tcslen(fingerStr));
               }
            }
         }

         SelectObject(g_hdc, oldPen);
         DeleteObject(pen);
      }

      LeaveCriticalSection(&g_graphicsCriticalSection);
   }
}

void ClearScreen()
{
   RECT cRect = g_clientRect;
   cRect.right -= cRect.left;
   cRect.left = 0;
   cRect.bottom -= cRect.top;
   cRect.top = 0;

   FillRect(g_hdc, &cRect, (HBRUSH)GetStockObject((g_ObserverMode ? COLOR_WINDOW : COLOR_APPWORKSPACE) + 1));
}

WacomMTHitRectPtr GetAppHitRect()
{
   if (g_UseWinHitRect)
   {
      WINDOWINFO appWindowInfo = {0};
      appWindowInfo.cbSize = sizeof(appWindowInfo);
      GetWindowInfo(g_mainWnd, &appWindowInfo);

      WacomMTHitRect hitRect = {
         (float)appWindowInfo.rcClient.left,
         (float)appWindowInfo.rcClient.top,
         (float)(appWindowInfo.rcClient.right - appWindowInfo.rcClient.left),
         (float)(appWindowInfo.rcClient.bottom - appWindowInfo.rcClient.top)
      };

      return WacomMTHitRectPtr(new WacomMTHitRect(hitRect));
   }
   return WacomMTHitRectPtr();
}

void DrawRawData(int count, unsigned short* rawBuf, int device)
{
   SIZE rawSize = {g_caps[device].ScanSizeX, g_caps[device].ScanSizeY};
   if (count && rawBuf)
   {
      ClearScreen();

      HPEN pen = CreatePen(PS_SOLID, 2, RGB(255,0,0));
      HPEN oldPen = (HPEN)SelectObject(g_hdc, pen);

      for (int sy = 0; sy < rawSize.cy; sy++)
      {
         for (int sx = 0; sx < rawSize.cx; sx++)
         {
            unsigned short value = rawBuf[sy * rawSize.cx + sx];
            if (value > 4)
            {
               int X = sx * (int)g_caps[device].LogicalWidth  / rawSize.cx + (int)g_caps[device].LogicalOriginX;
               int Y = sy * (int)g_caps[device].LogicalHeight / rawSize.cy + (int)g_caps[device].LogicalOriginY;
               int offset = std::max(value * 6 / 255 + 5, 7);

               if ((X > g_clientRect.left) && (X < g_clientRect.right) &&
                   (Y > g_clientRect.top) &&  (Y < g_clientRect.bottom))
               {
                  POINT pt = {X, Y};
                  ::ScreenToClient(g_mainWnd, &pt);

                  Circle(g_hdc, pt.x, pt.y, offset);
               }
            }
         }
      }

      SelectObject(g_hdc, oldPen);
      DeleteObject(pen);
   }
}

POINT FindCenterPoint(int count, WacomMTBlobPoint *points)
{
   UINT32 msig = 0;
   UINT32 wmx = 0;
   UINT32 wmy = 0;

   for (int i = 0; i < count; i++)
   {
      msig += points[i].Sensitivity;
      wmx += (UINT32)(points[i].X * points[i].Sensitivity);
      wmy += (UINT32)(points[i].Y * points[i].Sensitivity);
   }

   POINT center = {0,0};
   if (msig)
   {
      center.x = wmx / msig;
      center.y = wmy / msig;
   }
   return center;
}

void DrawBlob(int count, WacomMTBlobPoint *points)
{
   if (count && points)
   {
      WacomMTBlobPoint apiPoint = points[0];
      POINT curPt = {(LONG)apiPoint.X, (LONG)apiPoint.Y};
      ::ScreenToClient(g_mainWnd, &curPt);

      for (int pointIndex = 1; pointIndex <= count; pointIndex++)
      {
         bool bDrawLine = apiPoint.Sensitivity > 0;
         apiPoint = points[pointIndex == count ? 0 : pointIndex];
         POINT prevPt = curPt;
         curPt.x = (LONG)apiPoint.X;
         curPt.y = (LONG)apiPoint.Y;
         ::ScreenToClient(g_mainWnd, &curPt);

         if (bDrawLine)
         {
            MoveToEx(g_hdc, prevPt.x, prevPt.y, NULL);
            LineTo(g_hdc, curPt.x, curPt.y);
         }
      }
   }
}

void DrawBlobData(int count, WacomMTBlob *blobs, int device)
{
   if (count && blobs)
   {
      blobs->BlobID;
      blobs->BlobType;
      blobs->ParentID;

      ClearScreen();

      HPEN pen = CreatePen(PS_SOLID, 3, RGB(255, 0, 0));
      HPEN oldPen = (HPEN)SelectObject(g_hdc, pen);

      if (blobs->Confidence)
      {
         POINT pt = {(LONG)blobs->X, (LONG)blobs->Y};

         if ((pt.x > g_clientRect.left) && (pt.x < g_clientRect.right) &&
             (pt.y > g_clientRect.top) &&  (pt.y < g_clientRect.bottom))
         {
            //map to our window
            ::ScreenToClient(g_mainWnd, &pt);
            Circle(g_hdc, pt.x, pt.y, 2);
         }

         for (int blobIndex = 0; blobIndex < count; blobIndex++)
         {
            if (blobs[blobIndex].Confidence) // ignore confidence for now
            {
               DrawBlob(blobs[blobIndex].PointCount, blobs[blobIndex].BlobPoints);
            }
         }
      }

      SelectObject(g_hdc, oldPen);
      DeleteObject(pen);
   }
}

///////////////////////////////////////////////////////////////////////////////
// Wintab support functions
//

///////////////////////////////////////////////////////////////////////////////
//  FUNCTION: InitWintabAPI()
//
//  PURPOSE: Initialize the Wintab pen API.
//
//  COMMENTS:
//    Loads the Wintab32 DLL and sets up the API function pointers.
//
HCTX InitWintabAPI(HWND hwnd_I)
{
   if (!LoadWintab())
   {
      ShowError("Wintab not available");
      return 0;
   }

   char TabletName[50] = "";
   gpWTInfoA(WTI_DEVICES, DVC_NAME, TabletName);
   gpWTInfoA(WTI_INTERFACE, IFC_WINTABID, TabletName);

   /* check if WinTab available. */
   if (!gpWTInfoA(0, 0, NULL))
   {
      ShowError("WinTab Services Not Available.");
      return FALSE;
   }

   /* get default region */
   LOGCONTEXTA logContext = {0};
   gpWTInfoA(WTI_DEFSYSCTX, 0, &logContext);

   // No need to specify lcInOrg* and lcInExt* as they are the entire
   // physical tablet area by default.

   // Get messages
   logContext.lcOptions |= CXO_MESSAGES;

   // Move the system cursor.
   logContext.lcOptions |= CXO_SYSTEM;

   logContext.lcPktData = PACKETDATA;
   logContext.lcPktMode = PACKETMODE;
   logContext.lcMoveMask = PACKETDATA;
   logContext.lcBtnUpMask = logContext.lcBtnDnMask;

   //// Guarantee the output coordinate space to be in screen coordinates.
   logContext.lcOutOrgX = GetSystemMetrics(SM_XVIRTUALSCREEN);
   logContext.lcOutOrgY = GetSystemMetrics(SM_YVIRTUALSCREEN);
   logContext.lcOutExtX = GetSystemMetrics(SM_CXVIRTUALSCREEN); //SM_CXSCREEN );

   // In Wintab, the tablet origin is lower left.  Move origin to upper left
   // so that it coincides with screen origin.
   logContext.lcOutExtY = -GetSystemMetrics(SM_CYVIRTUALSCREEN);  //SM_CYSCREEN );

   /* open the region */
   return gpWTOpenA(hwnd_I, (LPLOGCONTEXT)&logContext, TRUE);
}

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: DrawPenData()
//
// PURPOSE: drawing routine for pen data
//
void DrawPenData(POINT point_I, UINT pressure_I, bool bMoveToPoint_I)
{
   // Prevent hover drawing.
   if (!pressure_I)
   {
      return;
   }

   EnterCriticalSection(&g_graphicsCriticalSection);

   assert(g_hdc);

   HPEN pen = CreatePen(PS_SOLID, pressure_I / 100, RGB(0, 0, 255));
   HPEN oldPen = (HPEN)SelectObject(g_hdc, pen);

   POINT ptNew = point_I;

   // Compare the new point with cached client rectangle in screen coordinates.
   if ((ptNew.x >= g_clientRect.left) &&
       (ptNew.y >= g_clientRect.top) &&
       (ptNew.x <= g_clientRect.right) &&
       (ptNew.y <= g_clientRect.bottom))
   {
      // Convert from screen to client coordinates to render.
      // This will let us put the app window anywhere on the desktop.
      ::ScreenToClient(g_mainWnd, &ptNew);
      PEN_DATA_TRACE("\tX:%ld  Y:%ld\n", ptNew.x, ptNew.y);

      // Move to a starting point if so directed.
      // Prevents streaks from last draw point or edge of client.
      if (bMoveToPoint_I)
      {
         PEN_DATA_TRACE("MoveTo: %i, %i\n", ptNew.x, ptNew.y);
         MoveToEx(g_hdc, ptNew.x, ptNew.y, NULL);
      }
      else
      {
         PEN_DATA_TRACE("LineTo: %i, %i\n", ptNew.x, ptNew.y);
         LineTo(g_hdc, ptNew.x, ptNew.y);
      }
   }

   SelectObject(g_hdc, oldPen);
   DeleteObject(pen);

   InvalidateRect(g_mainWnd, NULL, FALSE);

   LeaveCriticalSection(&g_graphicsCriticalSection);
}

//////////////////////////////////////////////////////////////////////////////
// Purpose
//    Release resources we used in this example.
//
void Cleanup(void)
{
   // Release MTAPI resources.
   WacomMTQuit();

   // Release Wintab resources.
   UnloadWintab();
}

//////////////////////////////////////////////////////////////////////////////
// Purpose
//    Dump the MT capabilities of the specified device.
//
void DumpCaps(int deviceID)
{
   WacomTrace("MT Capabilities for deviceID: %i\n", deviceID);
   WacomTrace("\tVersion: %i\n", g_caps[deviceID].Version);
   WacomTrace("\tDeviceID: %i\n", g_caps[deviceID].DeviceID);
   WacomTrace("\tType: %i\n", (int)g_caps[deviceID].Type);
   WacomTrace("\tLogicalOriginX: %f\n", g_caps[deviceID].LogicalOriginX);
   WacomTrace("\tLogicalOriginY: %f\n", g_caps[deviceID].LogicalOriginY);
   WacomTrace("\tLogicalWidth: %f\n", g_caps[deviceID].LogicalWidth);
   WacomTrace("\tLogicalHeight: %f\n", g_caps[deviceID].LogicalHeight);
   WacomTrace("\tPhysicalSizeX: %f\n", g_caps[deviceID].PhysicalSizeX);
   WacomTrace("\tPhysicalSizeY: %f\n", g_caps[deviceID].PhysicalSizeY);
   WacomTrace("\tReportedSizeX: %i\n", g_caps[deviceID].ReportedSizeX);
   WacomTrace("\tReportedSizeY: %i\n", g_caps[deviceID].ReportedSizeY);
   WacomTrace("\tScanSizeX: %i\n", g_caps[deviceID].ScanSizeX);
   WacomTrace("\tScanSizeY: %i\n", g_caps[deviceID].ScanSizeY);
   WacomTrace("\tFingerMax: %i\n", g_caps[deviceID].FingerMax);
   WacomTrace("\tBlobMax: %i\n", g_caps[deviceID].BlobMax);
   WacomTrace("\tBlobPointsMax: %i\n", g_caps[deviceID].BlobPointsMax);
   WacomTrace("\tCapabilityFlags: 0x%X\n", g_caps[deviceID].CapabilityFlags);
}

//////////////////////////////////////////////////////////////////////////////
// Purpose
//    Returns true if client hit rect changed from last time.
//
bool ClientHitRectChanged(const WacomMTHitRectPtr& wtHitRect_I, int deviceID)
{
   if (!wtHitRect_I && !g_lastWTHitRect[deviceID]) return false;
   if (!wtHitRect_I || !g_lastWTHitRect[deviceID]) return true;
   return ( (wtHitRect_I->originX != g_lastWTHitRect[deviceID]->originX)
         || (wtHitRect_I->originY != g_lastWTHitRect[deviceID]->originY)
         || (wtHitRect_I->width   != g_lastWTHitRect[deviceID]->width  )
         || (wtHitRect_I->height  != g_lastWTHitRect[deviceID]->height ));
}

//////////////////////////////////////////////////////////////////////////////
