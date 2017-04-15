#ifndef __PPJIOCTL_H__
#define __PPJIOCTL_H__

/* Define to use byte-size values for joystick axes, else dword size */
#undef UCHAR_AXES

#define	PPJOY_AXIS_MIN				1
#ifdef UCHAR_AXES
#define	PPJOY_AXIS_MAX				127
#else
#define	PPJOY_AXIS_MAX				32767
#endif


#define FILE_DEVICE_PPJOYBUS			FILE_DEVICE_BUS_EXTENDER
#define FILE_DEVICE_PPORTJOY			FILE_DEVICE_UNKNOWN

#define PPJOYBUS_IOCTL(_index_)	\
	CTL_CODE (FILE_DEVICE_PPJOYBUS, _index_, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PPJOYBUS_INTERNAL_IOCTL(_index_) \
    CTL_CODE (FILE_DEVICE_PPJOYBUS, _index_, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PPORTJOY_IOCTL(_index_)	\
	CTL_CODE (FILE_DEVICE_PPORTJOY, _index_, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define PPORTJOY_INTERNAL_IOCTL(_index_) \
    CTL_CODE (FILE_DEVICE_PPORTJOY, _index_, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_PPJOYBUS_ADD_JOY			PPJOYBUS_IOCTL (0x3)
#define IOCTL_PPJOYBUS_DEL_JOY			PPJOYBUS_IOCTL (0x4)
#define IOCTL_PPJOYBUS_ENUM_JOY			PPJOYBUS_IOCTL (0x5)

#define IOCTL_PPJOYBUS_CHIPMODE			PPJOYBUS_INTERNAL_IOCTL (0x101)
#define	IOCTL_PPJOYBUS_GETCONFIG1		PPJOYBUS_INTERNAL_IOCTL (0x103)

#define IOCTL_PPORTJOY_SET_STATE		PPORTJOY_IOCTL (0x0)
#define IOCTL_PPORTJOY_GET_STATE		PPORTJOY_IOCTL (0x1)
#define IOCTL_PPORTJOY_SET_MAPPING		PPORTJOY_IOCTL (0x2)
#define IOCTL_PPORTJOY_GET_MAPPING		PPORTJOY_IOCTL (0x3)
#define IOCTL_PPORTJOY_DEL_MAPPING		PPORTJOY_IOCTL (0x4)

#define IOCTL_PPORTJOY_SET_FORCE		PPORTJOY_IOCTL (0x5)

typedef struct
{
 ULONG			Size;				/* Number of bytes in this structure */
 ULONG			PortAddress;		/* Base address for LPT port specified in LPTNumber */
 UCHAR			JoyType;			/* Index into joystick type table. 0 is no joystick */
 UCHAR			JoySubType;			/* Sub-model of joystick type, 0 if not applicable */
 UCHAR			UnitNumber;			/* Index of joystick on interface. First unit is 0 */
 UCHAR			LPTNumber;			/* Number of LPT port, 0 is virtual interface */
 USHORT			VendorID;			/* PnP vendor ID for this device */
 USHORT			ProductID;			/* PnP product ID for this device */
} JOYSTICK_CONFIG1, *PJOYSTICK_CONFIG1;

/* Structure to pass information about a joystick to add to the system. Callers should */
/* fill all the fields of JOYSTICK_CONFIG1 except the PortAddress field. */
typedef struct
{
 JOYSTICK_CONFIG1	JoyData;		/* Data for joystick to add */
 ULONG				Persistent;		/* 1= automatically add joystick after reboot */
} JOYSTICK_ADD_DATA, *PJOYSTICK_ADD_DATA;

/* Structure to specify a joystick to delete. Callers should fill in the Size, JoyType, */
/* UnitNumber and LPTNumber fields. Set all other fields to 0 */
typedef struct
{
 JOYSTICK_CONFIG1	JoyData;		/* Data for joystick to delete */
} JOYSTICK_DEL_DATA, *PJOYSTICK_DEL_DATA;

/* Structure to enumerate all the joysticks joysticks currently defined */
typedef struct
{
 ULONG				Count;			/* Number of joystick currently defined */	/* No, num returned below */
 ULONG				Size;			/* Size needed to enumerate all joysticks */
 JOYSTICK_CONFIG1	Joysticks[1];	/* Array of joystick records */		
} JOYSTICK_ENUM_DATA, *PJOYSTICK_ENUM_DATA;

#define	JOYSTICK_STATE_V1	0x53544143

typedef struct
{
 ULONG	Version;
 UCHAR	Data[1];
} JOYSTICK_SET_STATE, *PJOYSTICK_SET_STATE;

#define	JOYSTICK_FORCE_V1	0x21474f44

typedef struct
{
 ULONG	Version;
 UCHAR	Data[1];
} JOYSTICK_SET_FORCE, *PJOYSTICK_SET_FORCE;

typedef struct
{
 UCHAR		NumAxes;
 UCHAR		NumButtons;
 UCHAR		NumHats;
 UCHAR		NumMaps;
 UCHAR		Data[1];
} JOYSTICK_MAP, *PJOYSTICK_MAP;

#define	MAP_SCOPE_DEFAULT	0
#define	MAP_SCOPE_DRIVER	1
#define	MAP_SCOPE_DEVICE	2

#define	JOYSTICK_MAP_V1	0x454E4F47

typedef struct
{
 ULONG		Version;
 UCHAR		MapScope;
 UCHAR		JoyType;
 UCHAR		Pad1;
 UCHAR		Pad2;
 ULONG		MapSize;
 /* Followed by
 JOYSTICK_MAP	MapData;
 */
} JOYSTICKMAP_HEADER, *PJOYSTICKMAP_HEADER;

#endif





