#ifndef HD44780_ANALYZER_SETTINGS
#define HD44780_ANALYZER_SETTINGS

#include <AnalyzerSettings.h>
#include <AnalyzerTypes.h>

class HD44780AnalyzerSettings : public AnalyzerSettings
{
public:
	HD44780AnalyzerSettings();
	virtual ~HD44780AnalyzerSettings();

	virtual bool SetSettingsFromInterfaces();
	void UpdateInterfacesFromSettings();
	virtual void LoadSettings( const char* settings );
	virtual const char* SaveSettings();

  Channel mEChannel, mRSChannel, mRWChannel, mDBChannel[8];

  bool mMarkTimingErrors,mIgnoreEPulsesWhenBusy,mStartIn4BitMode,mDoNotGenerateBusyCheckFrames;

  U32 mEnableCycleMin, mEnablePulseWidthMin, mAddressSetupMin, mAddressHoldMin,
      mDataWriteSetupMin, mDataWriteHoldMin, mDataReadDelayMax, mDataReadHoldMin,
      mBusyTimeClearHome, mBusyTimeCmdChar;

  void ClearAndAddChannels();

protected:
	std::auto_ptr< AnalyzerSettingInterfaceChannel > mEChannelInterface, mRSChannelInterface, mRWChannelInterface,
                                                   mDBChannelInterface[8];

  std::auto_ptr< AnalyzerSettingInterfaceBool > mMarkTimingErrorsInterface, mIgnoreEPulsesWhenBusyInterface, mStartIn4BitModeInterface,
                                                mDoNotGenerateBusyCheckFramesInterface;

  std::auto_ptr< AnalyzerSettingInterfaceInteger > mEnableCycleMinInterface, mEnablePulseWidthMinInterface, mAddressSetupMinInterface,
                                                   mAddressHoldMinInterface, mDataWriteSetupMinInterface, mDataWriteHoldMinInterface,
                                                   mDataReadDelayMaxInterface, mDataReadHoldMinInterface,
                                                   mBusyTimeClearHomeInterface, mBusyTimeCmdCharInterface;
};

#define _MS                     (1e3)
#define _US                     (1e6)
#define _NS                     (1e9)

#define _BV(x)                  (1<<x)

#define LCD_INIT1_DELAY         (40e-3)
#define LCD_INIT2_DELAY         (4.1e-3)
#define LCD_INIT3_DELAY         (100e-6)

#define LCD_CLR                 0    // DB0: clear display
#define LCD_CLR_MASK            (_BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(2) | _BV(1) | _BV(0))

#define LCD_HOME                1    // DB1: return to home position
#define LCD_HOME_MASK           (_BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(2) | _BV(1))

#define LCD_ENTRY_MODE          2    // DB2: set entry mode
#define LCD_ENTRY_INC           1    // DB1: 1=increment, 0=decrement
#define LCD_ENTRY_SHIFT         0    // DB0: 1=display shift on
#define LCD_ENTRY_MASK          (_BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3) | _BV(2))

#define LCD_DISPLAYMODE         3    // DB3: turn lcd/cursor on
#define LCD_DISPLAYMODE_ON      2    // DB2: turn display on
#define LCD_DISPLAYMODE_CURSOR  1    // DB1: turn cursor on
#define LCD_DISPLAYMODE_BLINK   0    // DB0: blinking cursor
#define LCD_DISPLAYMODE_MASK    (_BV(7) | _BV(6) | _BV(5) | _BV(4) | _BV(3))

#define LCD_MOVE                4    // DB4: move cursor/display
#define LCD_MOVE_DISP           3    // DB3: move display (0-> cursor)
#define LCD_MOVE_RIGHT          2    // DB2: move right (0-> left)
#define LCD_MOVE_MASK           (_BV(7) | _BV(6) | _BV(5) | _BV(4))

#define LCD_FUNCTION            5    // DB5: function set
#define LCD_FUNCTION_8BIT       4    // DB4: set 8BIT mode (0->4BIT mode)
#define LCD_FUNCTION_2LINES     3    // DB3: two lines (0->one line)
#define LCD_FUNCTION_10DOTS     2    // DB2: 5x10 font (0->5x7 font)
#define LCD_FUNCTION_MASK       (_BV(7) | _BV(6) | _BV(5))

#define LCD_CGRAM               6    // DB6: set CG RAM address
#define LCD_CGRAM_MASK          (_BV(7) | _BV(6))

#define LCD_DDRAM               7    // DB7: set DD RAM address
#define LCD_DDRAM_MASK          (_BV(7))

#define LCD_BUSY                7    // DB7: LCD is busy

#endif //HD44780_ANALYZER_SETTINGS
