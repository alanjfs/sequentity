/*----------------------------------------------------------------------------
 
FILE NAME
	WacomMultiTouchTypes.h
 
PURPOSE
	Wacom MultiTouch API data types
 
COPYRIGHT
	Copyright WACOM Technology, Inc. 2010 - 2013.
	All rights reserved.
 
 ----------------------------------------------------------------------------*/
#ifndef WacomMultitouchTypes_h
#define WacomMultitouchTypes_h

#ifdef __cplusplus
extern "C"
{
#endif

#define WACOM_MULTI_TOUCH_API_VERSION				4


//////////////////////////////////////////////////////////////////////////////
typedef enum _WacomMTError
{
	WMTErrorSuccess				= 0,
	WMTErrorDriverNotFound		= 1,
	WMTErrorBadVersion			= 2,
	WMTErrorAPIOutdated			= 3,
	WMTErrorInvalidParam			= 4,
	WMTErrorQuit					= 5,
	WMTErrorBufferTooSmall		= 6
} WacomMTError;


typedef enum _WacomMTDeviceType
{
	WMTDeviceTypeOpaque			= 0,
	WMTDeviceTypeIntegrated		= 1
} WacomMTDeviceType;


enum _WacomMTCapabilityFlags
{
	WMTCapabilityFlagsRawAvailable			= (1 <<  0),
	WMTCapabilityFlagsBlobAvailable			= (1 <<  1),
	WMTCapabilityFlagsSensitivityAvailable	= (1 <<  2),
	WMTCapabilityFlagsReserved					= (1 << 31)
};
typedef int WacomMTCapabilityFlags;


typedef enum _WacomMTFingerState
{
	WMTFingerStateNone			= 0,
	WMTFingerStateDown			= 1,
	WMTFingerStateHold			= 2,
	WMTFingerStateUp				= 3
} WacomMTFingerState;


typedef enum _WacomMTBlobType
{
	WMTBlobTypePrimary			= 0,
	WMTBlobTypeVoid				= 1
} WacomMTBlobType;


typedef enum _WacomMTProcessingMode
{
	WMTProcessingModeNone				= 0,
	WMTProcessingModeObserver			= (1 << 0),
	WMTProcessingModePassThrough		= (1 << 1),
	// bits 3-30 are reserved for future use
	WMTProcessingModeReserved			= (1 << 31)
} WacomMTProcessingMode;



//////////////////////////////////////////////////////////////////////////////
typedef struct _WacomMTCapability
{
	int							Version;
	int							DeviceID;
	WacomMTDeviceType			Type;
	float							LogicalOriginX;
	float							LogicalOriginY;
	float							LogicalWidth;
	float							LogicalHeight;
	float							PhysicalSizeX;
	float							PhysicalSizeY;
	int							ReportedSizeX;
	int							ReportedSizeY;
	int							ScanSizeX;
	int							ScanSizeY;
	int							FingerMax;
	int							BlobMax;
	int							BlobPointsMax;
	WacomMTCapabilityFlags	CapabilityFlags;
} WacomMTCapability;


typedef struct _WacomMTFinger
{
	int							FingerID;
	float							X;
	float							Y;
	float							Width;
	float							Height;
	unsigned short				Sensitivity;
	float							Orientation;
	bool							Confidence;
	WacomMTFingerState		TouchState;
} WacomMTFinger;


typedef struct _WacomMTFingerCollection
{
	int							Version;
	int							DeviceID;
	int							FrameNumber;
	int							FingerCount;
	WacomMTFinger				*Fingers;
} WacomMTFingerCollection;


typedef struct _WacomMTBlobPoint
{
	float							X;
	float							Y;
	unsigned short				Sensitivity;
} WacomMTBlobPoint;


typedef struct _WacomMTBlob
{
	int							BlobID;
	float							X;
	float							Y;
	bool							Confidence;
	WacomMTBlobType			BlobType;
	int							ParentID;
	int							PointCount;
	WacomMTBlobPoint			*BlobPoints;
} WacomMTBlob;


typedef struct _WacomMTBlobAggregate
{
	int							Version;
	int							DeviceID;
	int							FrameNumber;
	int							BlobCount;
	WacomMTBlob					*BlobArray;
} WacomMTBlobAggregate;


typedef struct _WacomMTRawData
{
	int							Version;
	int							DeviceID;
	int							FrameNumber;
	int							ElementCount;
	unsigned short				*Sensitivity;
} WacomMTRawData;


typedef struct _WacomMTHitRect
{
	float							originX;
	float							originY;
	float							width;
	float							height;
} WacomMTHitRect;



#ifdef __cplusplus
}
#endif

#endif /* WacomMultitouchTypes_h */

