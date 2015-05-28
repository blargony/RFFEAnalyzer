//Some flags/masks
#define TX_HOST_TO_DEVICE	0x01
#define TX_DEVICE_TO_HOST	0x00

#define DATA_FRAME			0x02
#define ACK_FRAME			0x01
#define ECHO_FRAME			0x02
#define BAT_FRAME			0x04

#define ACKED				0x04
#define NACKED				0x00

#define EXTENDED_KEY		0x08
#define STANDARD_KEY		0x00

#define BREAK_CODE			0x10
#define MAKE_CODE			0x00

#define PRINT_SCREEN		0x20
#define PAUSE_BREAK			0x40

#define ERROR_FRAME			0x80

#define MOVEMENT_FRAME		0x20

//Scancodes (Keyboard)
#define KEY_F1				0x05
#define KEY_F2				0x06
#define KEY_F3				0x04
#define KEY_F4				0x0C
#define KEY_F5				0x03
#define KEY_F6				0x0B
#define KEY_F7				0x83
#define KEY_F8				0x0A
#define KEY_F9				0x01
#define KEY_F10				0x09
#define KEY_F11				0x78
#define KEY_F12				0x07

#define KEY_ESC				0x76
#define KEY_SCROLLLOCK		0x7E
#define KEY_ACCENT			0x0E
#define KEY_MINUS			0x4E
#define KEY_EQUAL			0x55
#define KEY_BACKSPACE		0x66
#define KEY_TAB				0x0D
#define KEY_LBRACE			0x54
#define KEY_RBRACE			0x5B
#define KEY_BSLASH			0x5D
#define KEY_CAPSLOCK		0x58
#define KEY_COLON			0x4C
#define KEY_QUOTE			0x52
#define KEY_ENTER			0x5A
#define KEY_COMMA			0x41
#define KEY_PERIOD			0x49
#define KEY_FSLASH			0x4A

#define KEY_LSHIFT			0x12
#define KEY_RSHIFT			0x59
#define KEY_LCONTROL		0x14
#define KEY_LALT			0x11
#define KEY_SPACE			0x29
#define KEY_ASTERISK		0x7C
#define KEY_NUMLOCK			0x77

#define KEY_NUMPAD_1		0x69
#define KEY_NUMPAD_2		0x72
#define KEY_NUMPAD_3		0x7A
#define KEY_NUMPAD_4		0x6B
#define KEY_NUMPAD_5		0x73
#define KEY_NUMPAD_6		0x74
#define KEY_NUMPAD_7		0x6C
#define KEY_NUMPAD_8		0x75
#define KEY_NUMPAD_9		0x7D
#define KEY_NUMPAD_0		0x70
#define KEY_NUMPAD_DECIMAL	0x71
#define KEY_NUMPAD_PLUS		0x79
#define KEY_NUMPAD_MINUS	0x7B

#define KEY_Q				0x15
#define KEY_W				0x1D
#define KEY_E				0x24
#define KEY_R				0x2D
#define KEY_T				0x2C
#define KEY_Y				0x35
#define KEY_U				0x3C
#define KEY_I				0x43
#define KEY_O				0x44
#define KEY_P				0x4D

#define KEY_A				0x1C
#define KEY_S				0x1B
#define KEY_D				0x23
#define KEY_F				0x2B
#define KEY_G				0x34
#define KEY_H				0x33
#define KEY_J				0x3B
#define KEY_K				0x42
#define KEY_L				0x4B

#define KEY_Z				0x1A
#define KEY_X				0x22
#define KEY_C				0x21
#define KEY_V				0x2A
#define KEY_B				0x32
#define KEY_N				0x31
#define KEY_M				0x3A

#define KEY_1				0x16
#define KEY_2				0x1E
#define KEY_3				0x26
#define KEY_4				0x25
#define KEY_5				0x2E
#define KEY_6				0x36
#define KEY_7				0x3D
#define KEY_8				0x3E
#define KEY_9				0x46
#define KEY_0				0x45

#define EXTENDED_RALT		0x11
#define EXTENDED_RWINDOWS	0x27
#define EXTENDED_MENUS		0x2F
#define EXTENDED_INSERT		0x70
#define EXTENDED_HOME		0x6C
#define EXTENDED_PAGEUP		0x7D
#define EXTENDED_DELETE		0x71
#define EXTENDED_END		0x69
#define EXTENDED_PAGEDOWN	0x7A
#define EXTENDED_UPARROW	0x75
#define EXTENDED_LEFTARROW	0x6B
#define EXTENDED_DOWNARROW	0x72
#define EXTENDED_RIGHTARROW 0x74
#define EXTENDED_RCONTROL	0x14
#define EXTENDED_LWINDOWS	0x1F
#define EXTENDED_NUMPAD_ENTER 0x5A
#define EXTENDED_FSLASH		0x4A

//ACPI Scan Codes
#define EXTENDED_POWER		0x37
#define EXTENDED_SLEEP		0x3F
#define EXTENDED_WAKE		0x5E

//Windows Multimedia Scan Codes
#define EXTENDED_NEXTTRACK	0x4D
#define EXTENDED_PREVIOUSTRACK 0x15
#define EXTENDED_STOP		0x3B
#define EXTENDED_PLAYPAUSE	0x34
#define EXTENDED_MUTE		0x23
#define EXTENDED_VOLUMEUP	0x32
#define EXTENDED_VOLUMEDOWN 0x21
#define EXTENDED_MEDIASELECT 0x50
#define EXTENDED_EMAIL		0x48
#define EXTENDED_CALCULATOR 0x2B
#define EXTENDED_MYCOMPUTER 0x40
#define EXTENDED_WWWSEARCH	0x10
#define EXTENDED_WWWHOME	0x3A
#define EXTENDED_WWWBACK	0x38
#define EXTENDED_WWWFORWARD 0x30
#define EXTENDED_WWWSTOP	0x28
#define EXTENDED_WWWREFRESH 0x20
#define EXTENDED_FAVORITES	0x18

//PS/2 Keyboard Command Set for Host to Device Communication
#define CMD_WRITE_LEDS		0xED
#define CMD_ECHO			0xEE
#define CMD_SET_SCANCODE_SET 0xF0
#define CMD_READ_ID			0xF2
#define CMD_SET_REPEAT		0xF3
#define CMD_KEYBOARD_ENABLE 0xF4
#define CMD_KEYBOARD_DISABLE 0xF5
#define CMD_SET_DEFAULTS	0xF6
#define CMD_SET_ALL_KEYS_REPEAT					0xF7
#define CMD_SET_ALL_KEYS_MAKEBREAK_CODES		0xF8
#define CMD_SET_ALL_KEYS_MAKE_CODES				0xF9
#define CMD_SET_ALL_REPEAT_AND_MAKEBREAK_CODES	0xFA
#define CMD_SET_SINGLE_REPEAT					0xFB
#define CMD_SET_SINGLE_MAKEBREAK_CODES			0xFC
#define CMD_SET_SINGLE_MAKE_CODES				0xFD
#define CMD_RESEND								0xFE
#define CMD_KEYBOARD_RESET	0xFF

//Mouse Codes
#define MOUSE_CMD_SET_SCALING2to1	0xE7
#define MOUSE_CMD_SET_SCALING1to1   0xE6
#define MOUSE_CMD_SET_RESOLUTION	0xE8 
#define MOUSE_CMD_STATUS_REQUEST	0xE9
#define MOUSE_CMD_SET_STREAM_MODE	0xEA
#define MOUSE_CMD_READ_DATA			0xEB
#define MOUSE_CMD_RESET_WRAP_MODE	0xEC
#define MOUSE_CMD_SET_WRAP_MODE		0xEE
#define MOUSE_CMD_SET_REMOTE_MODE	0xF0
#define MOUSE_CMD_GET_DEVICE_ID		0xF2
#define MOUSE_CMD_SET_SAMPLE_RATE	0xF3
#define MOUSE_CMD_ENABLE_DATA_REPORTING		0xF4
#define MOUSE_CMD_DISABLE_DATA_REPORTING	0xF5
#define MOUSE_CMD_SET_DEFAULTS		0xF6
#define MOUSE_CMD_RESEND			0xFE
#define MOUSE_CMD_RESET				0xFF 
