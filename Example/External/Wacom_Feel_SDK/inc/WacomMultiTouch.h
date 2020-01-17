/*----------------------------------------------------------------------------
 
FILE NAME
	WacomMultiTouch.h 
 
PURPOSE
	Wacom MultiTouch API
 
COPYRIGHT
	Copyright WACOM Technology, Inc.  2010 - 2014.
	All rights reserved.
 
 ----------------------------------------------------------------------------*/
#ifndef WacomMultitouch_h // {
#define WacomMultitouch_h

#if defined(__OBJC__) // {
#include <Cocoa/Cocoa.h>
#endif // }

#include <stdlib.h>   //for size_t
#include "WacomMultiTouchTypes.h"

#if !defined(WMT_EXPORT) // {
#if defined(_MSC_VER) // {
	#if defined(WACOMMT_EXPORTS) // {
		#define WMT_EXPORT __declspec(dllexport)
	#else // } {
		#define WMT_EXPORT
	#endif // }
#elif defined(__GNUC__) // } {
	#define WMT_EXPORT __attribute__((visibility("default"), weak_import)) extern
#else // } {
	#error UnknownPlatform
	#define WMT_EXPORT
#endif // }
#endif // }


#define WM_FINGERDATA  0x6205
#define WM_BLOBDATA    0x6206
#define WM_RAWDATA     0x6207

#if defined(__cplusplus) // {
extern "C"
{
#endif // }

	//////////////////////////////////////////////////////////////////////////////
	typedef void (* WMT_ATTACH_CALLBACK)(WacomMTCapability deviceInfo, void *userData);
	typedef void (* WMT_DETACH_CALLBACK)(int deviceID, void *userData);

	typedef int (* WMT_FINGER_CALLBACK)(WacomMTFingerCollection *fingerPacket, void *userData);
	typedef int (* WMT_BLOB_CALLBACK)(WacomMTBlobAggregate *blobPacket, void *userData);
	typedef int (* WMT_RAW_CALLBACK)(WacomMTRawData *blobPacket, void *userData);



	//////////////////////////////////////////////////////////////////////////////
#if defined(WACOMMT_EXPORTS) || defined(__GNUC__) || defined(__OBJC__) // {

	/// Initializes the Wacom Multitouch API to a specific version
	/// Older version compatible apps should work with newer versions of the DLL
	/// provided they specify the version requested as the same
	WMT_EXPORT WacomMTError WacomMTInitialize(int libraryAPIVersion);

	/// Shuts down the Wacom Multitouch API
	WMT_EXPORT void         WacomMTQuit(void);

	/// Gives access to a list of attached devices
	WMT_EXPORT int          WacomMTGetAttachedDeviceIDs(int *deviceArray, size_t bufferSize);

	/// Returns the capabilities of a particular device
	WMT_EXPORT WacomMTError WacomMTGetDeviceCapabilities(int deviceID, WacomMTCapability *capabilityBuffer);

	/// Registers a callback to receive calls when devices are connected
	WMT_EXPORT WacomMTError WacomMTRegisterAttachCallback(WMT_ATTACH_CALLBACK attachCallback, void *userData);

	/// Unregisters a callback, so that it will not continue to receive notifications when devices are connected
	WMT_EXPORT WacomMTError WacomMTRegisterDetachCallback(WMT_DETACH_CALLBACK detachCallback, void *userData);

	/// Registers for finger data within a specific hit rect or anywhere if NULL is supplied for the hitrect
	WMT_EXPORT WacomMTError WacomMTRegisterFingerReadCallback(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, WMT_FINGER_CALLBACK fingerCallback, void *userData);

	/// Registers for blob data within a specific hit rect or anywhere if NULL is supplied for the hitrect
	WMT_EXPORT WacomMTError WacomMTRegisterBlobReadCallback(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, WMT_BLOB_CALLBACK blobCallback, void *userData);

	/// Registers for raw data for the full tablet
	WMT_EXPORT WacomMTError WacomMTRegisterRawReadCallback(int deviceID, WacomMTProcessingMode mode, WMT_RAW_CALLBACK rawCallback, void *userData);

	/// Unregisters for finger data within a specific hitrect
	WMT_EXPORT WacomMTError WacomMTUnRegisterFingerReadCallback(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, void *userData);

	/// Unregisters for blob data within a specific hitrect
	WMT_EXPORT WacomMTError WacomMTUnRegisterBlobReadCallback(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, void *userData);

	/// Unregisters for raw data for the full tablet
	WMT_EXPORT WacomMTError WacomMTUnRegisterRawReadCallback(int deviceID, WacomMTProcessingMode mode, void *userData);

	/// Moves the registered finger read call back from one hitrect to another hitrect
	WMT_EXPORT WacomMTError WacomMTMoveRegisteredFingerReadCallback(int deviceID, WacomMTHitRect *oldHitRect, WacomMTProcessingMode mode, WacomMTHitRect *newHitRect, void *userData);

	/// Moves the registered blob read call back from one hitrect to another hitrect
	WMT_EXPORT WacomMTError WacomMTMoveRegisteredBlobReadCallback(int deviceID, WacomMTHitRect *oldHhitRect, WacomMTProcessingMode mode, WacomMTHitRect *newHitRect, void *userData);

#if defined(_MSC_VER) // {
	/// Registers a window to receive callbacks with finger read data
	WMT_EXPORT WacomMTError WacomMTRegisterFingerReadHWND(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth);

	/// Registers a window to receive callbacks with blob read data
	WMT_EXPORT WacomMTError WacomMTRegisterBlobReadHWND(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth);

	/// Registers a window to receive callbacks with raw read data. This call is deprecated
	WMT_EXPORT WacomMTError WacomMTRegisterRawReadHWND(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth);

	/// Unregisters a window to receive callbacks with finger read data
	WMT_EXPORT WacomMTError WacomMTUnRegisterFingerReadHWND(HWND hWnd);

	/// Unregisters a window to receive callbacks with blob read data
	WMT_EXPORT WacomMTError WacomMTUnRegisterBlobReadHWND(HWND hWnd);

#endif // }

#if defined(__OBJC__) // {

	/// Registers a window or view to receive callbacks with finger read data
	WMT_EXPORT WacomMTError WacomMTRegisterFingerReadID(int deviceID, WacomMTProcessingMode mode, id window, int bufferDepth);

	/// Registers a window or view to receive callbacks with blob data
	/// The object must implement the protocol WacomMTWindowBlobRegistration
	WMT_EXPORT WacomMTError WacomMTRegisterBlobReadID(int deviceID, WacomMTProcessingMode mode, id window, int bufferDepth);

	/// Unregisters a window or view to receive callbacks with finger read data
	WMT_EXPORT WacomMTError WacomMTUnRegisterFingerReadID(id window);

	/// Unregisters a window or view to receive callbacks with blob data
	WMT_EXPORT WacomMTError WacomMTUnRegisterBlobReadID(id window);
#endif // }

#else // } {
	// This code should not normally be used by an application.
	typedef WacomMTError ( * WACOMMTINITIALIZE )                       ( int );
	typedef void         ( * WACOMMTQUIT )                             ( void );
	typedef int          ( * WACOMMTGETATTACHEDDEVICEIDS )             ( int*, size_t );
	typedef WacomMTError ( * WACOMMTGETDEVICECAPABILITIES )            ( int, WacomMTCapability* );
	typedef WacomMTError ( * WACOMMTREGISTERATTACHCALLBACK )           ( WMT_ATTACH_CALLBACK, void* );
	typedef WacomMTError ( * WACOMMTREGISTERDETACHCALLBACK )           ( WMT_DETACH_CALLBACK, void* );
	typedef WacomMTError ( * WACOMMTREGISTERFINGERREADCALLBACK )       ( int, WacomMTHitRect*, WacomMTProcessingMode, WMT_FINGER_CALLBACK, void* );
	typedef WacomMTError ( * WACOMMTREGISTERBLOBREADCALLBACK )         ( int, WacomMTHitRect*, WacomMTProcessingMode, WMT_BLOB_CALLBACK, void* );
	typedef WacomMTError ( * WACOMMTREGISTERRAWREADCALLBACK )          ( int, WacomMTProcessingMode, WMT_RAW_CALLBACK, void* );
	typedef WacomMTError ( * WACOMMTUNREGISTERFINGERREADCALLBACK )     ( int, WacomMTHitRect*, WacomMTProcessingMode, void* );
	typedef WacomMTError ( * WACOMMTUNREGISTERBLOBREADCALLBACK )       ( int, WacomMTHitRect*, WacomMTProcessingMode, void* );
	typedef WacomMTError ( * WACOMMTUNREGISTERRAWREADCALLBACK )        ( int, WacomMTProcessingMode, void* );
	typedef WacomMTError ( * WACOMMTMOVEREGISTEREDFINGERREADCALLBACK ) ( int, WacomMTHitRect*, WacomMTProcessingMode, WacomMTHitRect*, void* );
	typedef WacomMTError ( * WACOMMTMOVEREGISTEREDBLOBREADCALLBACK )   ( int, WacomMTHitRect*, WacomMTProcessingMode, WacomMTHitRect*, void* );
	typedef WacomMTError ( * WACOMMTREGISTERFINGERREADHWND )           ( int, WacomMTProcessingMode, HWND, int );
	typedef WacomMTError ( * WACOMMTREGISTERBLOBREADHWND )             ( int, WacomMTProcessingMode, HWND, int );
	typedef WacomMTError ( * WACOMMTREGISTERRAWREADHWND )              ( int, WacomMTProcessingMode, HWND, int );
	typedef WacomMTError ( * WACOMMTUNREGISTERFINGERREADHWND )         ( HWND );
	typedef WacomMTError ( * WACOMMTUNREGISTERBLOBREADHWND )           ( HWND );

	extern WACOMMTINITIALIZE                       WacomMTInitializeInternal;
	extern WACOMMTQUIT                             WacomMTQuitInternal;
	extern WACOMMTGETATTACHEDDEVICEIDS             WacomMTGetAttachedDeviceIDs;
	extern WACOMMTGETDEVICECAPABILITIES            WacomMTGetDeviceCapabilities;
	extern WACOMMTREGISTERATTACHCALLBACK           WacomMTRegisterAttachCallback;
	extern WACOMMTREGISTERDETACHCALLBACK           WacomMTRegisterDetachCallback;
	extern WACOMMTREGISTERFINGERREADCALLBACK       WacomMTRegisterFingerReadCallback;
	extern WACOMMTREGISTERBLOBREADCALLBACK         WacomMTRegisterBlobReadCallback;
	extern WACOMMTREGISTERRAWREADCALLBACK          WacomMTRegisterRawReadCallback;
	extern WACOMMTUNREGISTERFINGERREADCALLBACK     WacomMTUnRegisterFingerReadCallback;
	extern WACOMMTUNREGISTERBLOBREADCALLBACK       WacomMTUnRegisterBlobReadCallback;
	extern WACOMMTUNREGISTERRAWREADCALLBACK        WacomMTUnRegisterRawReadCallback;
	extern WACOMMTMOVEREGISTEREDFINGERREADCALLBACK WacomMTMoveRegisteredFingerReadCallback;
	extern WACOMMTMOVEREGISTEREDBLOBREADCALLBACK   WacomMTMoveRegisteredBlobReadCallback;
	extern WACOMMTREGISTERFINGERREADHWND           WacomMTRegisterFingerReadHWND;
	extern WACOMMTREGISTERBLOBREADHWND             WacomMTRegisterBlobReadHWND;
	extern WACOMMTREGISTERRAWREADHWND              WacomMTRegisterRawReadHWND;
	extern WACOMMTUNREGISTERFINGERREADHWND         WacomMTUnRegisterFingerReadHWND;
	extern WACOMMTUNREGISTERBLOBREADHWND           WacomMTUnRegisterBlobReadHWND;

	bool           LoadWacomMTLib( void );
	void           UnloadWacomMTLib( void );
	WacomMTError   WacomMTInitialize(int libraryAPIVersion);
	void           WacomMTQuit(void);
	int            WacomMTGetAttachedDeviceIDsInternal(int *deviceArray, size_t bufferSize);
	WacomMTError   WacomMTGetDeviceCapabilitiesInternal(int deviceID, WacomMTCapability *capabilityBuffer);
	WacomMTError   WacomMTRegisterAttachCallbackInternal(WMT_ATTACH_CALLBACK attachCallback, void *userData);
	WacomMTError   WacomMTRegisterDetachCallbackInternal(WMT_DETACH_CALLBACK detachCallback, void *userData);
	WacomMTError   WacomMTRegisterFingerReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, WMT_FINGER_CALLBACK fingerCallback, void *userData);
	WacomMTError   WacomMTRegisterBlobReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, WMT_BLOB_CALLBACK blobCallback, void *userData);
	WacomMTError   WacomMTRegisterRawReadCallbackInternal(int deviceID, WacomMTProcessingMode mode, WMT_RAW_CALLBACK rawCallback, void *userData);
	WacomMTError   WacomMTUnRegisterFingerReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, void *userData);
	WacomMTError   WacomMTUnRegisterBlobReadCallbackInternal(int deviceID, WacomMTHitRect *hitRect, WacomMTProcessingMode mode, void *userData);
	WacomMTError   WacomMTUnRegisterRawReadCallbackInternal(int deviceID, WacomMTProcessingMode mode, void *userData);
	WacomMTError   WacomMTMoveRegisteredFingerReadCallbackInternal(int deviceID, WacomMTHitRect *oldHitRect, WacomMTProcessingMode mode, WacomMTHitRect *newHitRect, void *userData);
	WacomMTError   WacomMTMoveRegisteredBlobReadCallbackInternal(int deviceID, WacomMTHitRect *oldHhitRect, WacomMTProcessingMode mode, WacomMTHitRect *newHitRect, void *userData);
	WacomMTError   WacomMTRegisterFingerReadHWNDInternal(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth);
	WacomMTError   WacomMTRegisterBlobReadHWNDInternal(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth);
	WacomMTError   WacomMTRegisterRawReadHWNDInternal(int deviceID, WacomMTProcessingMode mode, HWND hWnd, int bufferDepth);
	WacomMTError   WacomMTUnRegisterFingerReadHWNDInternal(HWND hWnd);
	WacomMTError   WacomMTUnRegisterBlobReadHWNDInternal(HWND hWnd);
#endif // }

#if defined(__cplusplus) // {
}
#endif // }

#if defined(__OBJC__) // {
//Both of the following protocols are implemented by WindowMonitor. If you intend to subclass WindowMonitor
// you should plan on overriding on of the data available methods below. Further, if you override
// windowDidResize, windowDidMove, or WindowDidClose, you need to call the super versions so that this
// code and update the locations and sizes of your hit rects for you.

/// This protocol defines the call back that is required for
/// for a window or view to receive callbacks with finger data
@protocol WacomMTWindowFingerRegistration<NSObject>
@required
-(void) FingerDataAvailable:(WacomMTFingerCollection *)packet data:(void *)userData;
@end

/// This protocol defines the call back that is required for
/// for a window or view to receive callbacks with blob data
@protocol WacomMTWindowBlobRegistration<NSObject>
@required
-(void) BlobDataAvailable:(WacomMTBlobAggregate *)packet data:(void *)userData;
@end
#endif // }


#endif // WacomMultitouch_h }

