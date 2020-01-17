/*----------------------------------------------------------------------------s
NAME
	WacomMultiTouch.cpp

PURPOSE
	Sample code helper module for dynamic linking to wacommt.dll

COPYRIGHT
	Copyright (c) Wacom Technology Corp., 2010-2014 All Rights Reserved
	All rights reserved.

---------------------------------------------------------------------------- */

#include <windows.h>
#include "WacomMultiTouch.h"

HINSTANCE gpWacomMTLib = NULL;
WACOMMTINITIALIZE WacomMTInitializeInternal = NULL;
WACOMMTQUIT WacomMTQuitInternal = NULL;
WACOMMTGETATTACHEDDEVICEIDS WacomMTGetAttachedDeviceIDs = WacomMTGetAttachedDeviceIDsInternal;
WACOMMTGETDEVICECAPABILITIES WacomMTGetDeviceCapabilities = WacomMTGetDeviceCapabilitiesInternal;
WACOMMTREGISTERATTACHCALLBACK WacomMTRegisterAttachCallback = WacomMTRegisterAttachCallbackInternal;
WACOMMTREGISTERDETACHCALLBACK WacomMTRegisterDetachCallback = WacomMTRegisterDetachCallbackInternal;
WACOMMTREGISTERFINGERREADCALLBACK WacomMTRegisterFingerReadCallback = WacomMTRegisterFingerReadCallbackInternal;
WACOMMTREGISTERBLOBREADCALLBACK WacomMTRegisterBlobReadCallback = WacomMTRegisterBlobReadCallbackInternal;
WACOMMTREGISTERRAWREADCALLBACK WacomMTRegisterRawReadCallback = WacomMTRegisterRawReadCallbackInternal;
WACOMMTUNREGISTERFINGERREADCALLBACK WacomMTUnRegisterFingerReadCallback = WacomMTUnRegisterFingerReadCallbackInternal;
WACOMMTUNREGISTERBLOBREADCALLBACK WacomMTUnRegisterBlobReadCallback = WacomMTUnRegisterBlobReadCallbackInternal;
WACOMMTUNREGISTERRAWREADCALLBACK WacomMTUnRegisterRawReadCallback = WacomMTUnRegisterRawReadCallbackInternal;
WACOMMTMOVEREGISTEREDFINGERREADCALLBACK WacomMTMoveRegisteredFingerReadCallback = WacomMTMoveRegisteredFingerReadCallbackInternal;
WACOMMTMOVEREGISTEREDBLOBREADCALLBACK WacomMTMoveRegisteredBlobReadCallback = WacomMTMoveRegisteredBlobReadCallbackInternal;
WACOMMTREGISTERFINGERREADHWND WacomMTRegisterFingerReadHWND = WacomMTRegisterFingerReadHWNDInternal;
WACOMMTREGISTERBLOBREADHWND WacomMTRegisterBlobReadHWND = WacomMTRegisterBlobReadHWNDInternal;
WACOMMTREGISTERRAWREADHWND WacomMTRegisterRawReadHWND = WacomMTRegisterRawReadHWNDInternal;
WACOMMTUNREGISTERFINGERREADHWND WacomMTUnRegisterFingerReadHWND = WacomMTUnRegisterFingerReadHWNDInternal;
WACOMMTUNREGISTERBLOBREADHWND WacomMTUnRegisterBlobReadHWND = WacomMTUnRegisterBlobReadHWNDInternal;

#define GETPROCADDRESS(type, func) \
	func = (type)GetProcAddress(gpWacomMTLib, #func); \
	if (!func){ err = GetLastError(); UnloadWacomMTLib(); return false; }



///////////////////////////////////////////////////////////////////////////////
bool LoadWacomMTLib( void )
{
	int err = 0;

	gpWacomMTLib = LoadLibraryA("wacommt.dll");
	if (!gpWacomMTLib)
	{
		err = GetLastError();
		return false;
	}

	WacomMTInitializeInternal = (WACOMMTINITIALIZE)GetProcAddress( gpWacomMTLib, "WacomMTInitialize" );
	WacomMTQuitInternal = (WACOMMTQUIT)GetProcAddress( gpWacomMTLib, "WacomMTQuit" );
	if (!WacomMTInitializeInternal || !WacomMTQuitInternal)
	{
		err = GetLastError();
		UnloadWacomMTLib();
		return false;
	}

	GETPROCADDRESS( WACOMMTGETATTACHEDDEVICEIDS,             WacomMTGetAttachedDeviceIDs );
	GETPROCADDRESS( WACOMMTGETDEVICECAPABILITIES,            WacomMTGetDeviceCapabilities );
	GETPROCADDRESS( WACOMMTREGISTERATTACHCALLBACK,           WacomMTRegisterAttachCallback );
	GETPROCADDRESS( WACOMMTREGISTERDETACHCALLBACK,           WacomMTRegisterDetachCallback );
	GETPROCADDRESS( WACOMMTREGISTERFINGERREADCALLBACK,       WacomMTRegisterFingerReadCallback );
	GETPROCADDRESS( WACOMMTREGISTERBLOBREADCALLBACK,         WacomMTRegisterBlobReadCallback );
	GETPROCADDRESS( WACOMMTREGISTERRAWREADCALLBACK,          WacomMTRegisterRawReadCallback );
	GETPROCADDRESS( WACOMMTUNREGISTERFINGERREADCALLBACK,     WacomMTUnRegisterFingerReadCallback );
	GETPROCADDRESS( WACOMMTUNREGISTERBLOBREADCALLBACK,       WacomMTUnRegisterBlobReadCallback );
	GETPROCADDRESS( WACOMMTUNREGISTERRAWREADCALLBACK,        WacomMTUnRegisterRawReadCallback );
	GETPROCADDRESS( WACOMMTMOVEREGISTEREDFINGERREADCALLBACK, WacomMTMoveRegisteredFingerReadCallback );
	GETPROCADDRESS( WACOMMTMOVEREGISTEREDBLOBREADCALLBACK,   WacomMTMoveRegisteredBlobReadCallback );
	GETPROCADDRESS( WACOMMTREGISTERFINGERREADHWND,           WacomMTRegisterFingerReadHWND );
	GETPROCADDRESS( WACOMMTREGISTERBLOBREADHWND,             WacomMTRegisterBlobReadHWND );
	GETPROCADDRESS( WACOMMTREGISTERRAWREADHWND,              WacomMTRegisterRawReadHWND );
	GETPROCADDRESS( WACOMMTUNREGISTERFINGERREADHWND,         WacomMTUnRegisterFingerReadHWND );
	GETPROCADDRESS( WACOMMTUNREGISTERBLOBREADHWND,           WacomMTUnRegisterBlobReadHWND );

	return true;
}



///////////////////////////////////////////////////////////////////////////////
void UnloadWacomMTLib( void )
{
	if (gpWacomMTLib)
	{
		FreeLibrary(gpWacomMTLib);
		gpWacomMTLib = NULL;
	}

	WacomMTInitializeInternal = NULL;
	WacomMTQuitInternal = NULL;
	WacomMTGetAttachedDeviceIDs = WacomMTGetAttachedDeviceIDsInternal;
	WacomMTGetDeviceCapabilities = WacomMTGetDeviceCapabilitiesInternal;
	WacomMTRegisterAttachCallback = WacomMTRegisterAttachCallbackInternal;
	WacomMTRegisterDetachCallback = WacomMTRegisterDetachCallbackInternal;
	WacomMTRegisterFingerReadCallback = WacomMTRegisterFingerReadCallbackInternal;
	WacomMTRegisterBlobReadCallback = WacomMTRegisterBlobReadCallbackInternal;
	WacomMTRegisterRawReadCallback = WacomMTRegisterRawReadCallbackInternal;
	WacomMTUnRegisterFingerReadCallback = WacomMTUnRegisterFingerReadCallbackInternal;
	WacomMTUnRegisterBlobReadCallback = WacomMTUnRegisterBlobReadCallbackInternal;
	WacomMTUnRegisterRawReadCallback = WacomMTUnRegisterRawReadCallbackInternal;
	WacomMTMoveRegisteredFingerReadCallback = WacomMTMoveRegisteredFingerReadCallbackInternal;
	WacomMTMoveRegisteredBlobReadCallback = WacomMTMoveRegisteredBlobReadCallbackInternal;
	WacomMTRegisterFingerReadHWND = WacomMTRegisterFingerReadHWNDInternal;
	WacomMTRegisterBlobReadHWND = WacomMTRegisterBlobReadHWNDInternal;
	WacomMTRegisterRawReadHWND = WacomMTRegisterRawReadHWNDInternal;
	WacomMTUnRegisterFingerReadHWND = WacomMTUnRegisterFingerReadHWNDInternal;
	WacomMTUnRegisterBlobReadHWND = WacomMTUnRegisterBlobReadHWNDInternal;
}



///////////////////////////////////////////////////////////////////////////////
WacomMTError WacomMTInitialize(int libraryAPIVersion)
{
	if (LoadWacomMTLib())
	{
		return WacomMTInitializeInternal(libraryAPIVersion);
	}
	return WMTErrorDriverNotFound;
}



///////////////////////////////////////////////////////////////////////////////
void WacomMTQuit(void)
{
	if (WacomMTQuitInternal)
	{
		WacomMTQuitInternal();
	}
	UnloadWacomMTLib();
}

#pragma warning(push)
#pragma warning(disable: 4100) // the params for the following functions are unused

//The rest of these are stubs to prevent a crash if the driver isn't loaded
int WacomMTGetAttachedDeviceIDsInternal(int *deviceArray, size_t bufferSize)
{
	return 0;
}
WacomMTError WacomMTGetDeviceCapabilitiesInternal(int deviceID, WacomMTCapability *capabilityBuffer)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterAttachCallbackInternal(WMT_ATTACH_CALLBACK attachCallback, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterDetachCallbackInternal(WMT_DETACH_CALLBACK detachCallback, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterFingerReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, WMT_FINGER_CALLBACK fingerCallback, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterBlobReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, WMT_BLOB_CALLBACK blobCallback, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterRawReadCallbackInternal(int deviceID, WacomMTProcessingMode mode, WMT_RAW_CALLBACK rawCallback, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTUnRegisterFingerReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTUnRegisterBlobReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTUnRegisterRawReadCallbackInternal(int deviceID, WacomMTProcessingMode mode, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTMoveRegisteredFingerReadCallbackInternal(int deviceID, WacomMTHitRect *oldHitRect, WacomMTProcessingMode mode, WacomMTHitRect *newHitRect, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTMoveRegisteredBlobReadCallbackInternal(int deviceID, WacomMTHitRect *oldHhitRect, WacomMTProcessingMode mode, WacomMTHitRect *newHitRect, void *userData)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterFingerReadHWNDInternal(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterBlobReadHWNDInternal(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTRegisterRawReadHWNDInternal(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTUnRegisterFingerReadHWNDInternal(HWND hWnd)
{
	return WMTErrorQuit;
}
WacomMTError WacomMTUnRegisterBlobReadHWNDInternal(HWND hWnd)
{
	return WMTErrorQuit;
}
#pragma warning(pop)

