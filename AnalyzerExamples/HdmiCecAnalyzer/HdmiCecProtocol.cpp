#include "HdmiCecProtocol.h"

namespace HdmiCec
{

const char* GetProtocolName()
{
    return "HDMI CEC";
}

const char* GetFullProtocolName()
{
    return "HDMI Consumer Electronics Control (CEC)";
}

const char* GetChannelName()
{
    return "CEC";
}

const char* GetDevAddressString( DevAddress devAddress )
{
    switch( devAddress )
    {
        case DevAddress_TV:          return "TV";
        case DevAddress_Recorder1:   return "Recorder1";
        case DevAddress_Recorder2:   return "Recorder2";
        case DevAddress_Tuner1:      return "Tuner1";
        case DevAddress_Player1:     return "Player1";
        case DevAddress_AudioSystem: return "AudioSystem";
        case DevAddress_Tuner2:      return "Tuner2";
        case DevAddress_Tuner3:      return "Tuner3";
        case DevAddress_Player2:     return "Player2";
        case DevAddress_Recorder3:   return "Recorder3";
        case DevAddress_Tuner4:      return "Tuner4";
        case DevAddress_Player3:     return "Player3";
        case DevAddress_Reserved1:   return "Reserved1";
        case DevAddress_Reserved2:   return "Reserved2";
        case DevAddress_FreeUse:     return "FreeUse";
        case DevAddress_UnregBcast:  return "Unreg/Bcast";
        default: break;
    }
    return "Invalid";
}

const char* GetOpCodeString( OpCode opCode )
{
    switch( opCode )
    {
        case OpCode_ActiveSource:           return "ActiveSource";
        case OpCode_ImageViewOn:            return "ImageViewOn";
        case OpCode_TextViewOn:             return "TextViewOn";
        case OpCode_InactiveSource:         return "InactiveSource";
        case OpCode_RequestActiveSource:    return "RequestActiveSource";
        case OpCode_RoutingChange:          return "RoutingChange";
        case OpCode_RoutingInformation:     return "RoutingInformation";
        case OpCode_SetStreamPath:          return "SetStreamPath";
        case OpCode_Standby:                return "Standby";
        case OpCode_RecordOff:              return "RecordOff";
        case OpCode_RecordOn:               return "RecordOn";
        case OpCode_RecordStatus:           return "RecordStatus";
        case OpCode_RecordTvScreen:         return "RecordTvScreen";
        case OpCode_ClearAnalogueTimer:     return "ClearAnalogueTimer";
        case OpCode_ClearDigitalTimer:      return "ClearDigitalTimer";
        case OpCode_ClearExternalTimer:     return "ClearExternalTimer";
        case OpCode_SetAnalogueTimer:       return "SetAnalogueTimer";
        case OpCode_SetDigitalTimer:        return "SetDigitalTimer";
        case OpCode_SetExternalTimer:       return "SetExternalTimer";
        case OpCode_SetTimerProgramTitle:   return "SetTimerProgramTitle";
        case OpCode_TimerClearedStatus:     return "TimerClearedStatus";
        case OpCode_TimerStatus:            return "TimerStatus";
        case OpCode_CecVersion:             return "CecVersion";
        case OpCode_GetCecVersion:          return "GetCecVersion";
        case OpCode_GivePhysicalAddress:    return "GivePhysicalAddress";
        case OpCode_GetMenuLanguage:        return "GetMenuLanguage";
        case OpCode_ReportPhysicalAddress:  return "ReportPhysicalAddress";
        case OpCode_SetMenuLanguage:        return "SetMenuLanguage";
        case OpCode_DeckControl:            return "DeckControl";
        case OpCode_DeckStatus:             return "DeckStatus";
        case OpCode_GiveDeckStatus:         return "GiveDeckStatus";
        case OpCode_Play:                   return "Play";
        case OpCode_GiveTunerDeviceStatus:  return "GiveTunerDeviceStatus";
        case OpCode_SelectAnalogueService:  return "SelectAnalogueService";
        case OpCode_SelectDigitalService:   return "SelectDigitalService";
        case OpCode_TunerDeviceStatus:      return "TunerDeviceStatus";
        case OpCode_TunerStepDecrement:     return "TunerStepDecrement";
        case OpCode_TunerStepIncrement:     return "TunerStepIncrement";
        case OpCode_DeviceVendorId:         return "DeviceVendorId";
        case OpCode_GiveDeviceVendorId:     return "GiveDeviceVendorId";
        case OpCode_VendorCommand:          return "VendorCommand";
        case OpCode_VendorCommandWithId:    return "VendorCommandWithId";
        case OpCode_VendorRemoteButtonDown: return "VendorRemoteButtonDown";
        case OpCode_VendorRemoteButtonUp:   return "VendorRemoteButtonUp";
        case OpCode_SetOsdString:           return "SetOsdString";
        case OpCode_GiveOsdName:            return "GiveOsdName";
        case OpCode_SetOsdName:             return "SetOsdName";
        case OpCode_MenuRequest:            return "MenuRequest";
        case OpCode_MenuStatus:             return "MenuStatus";
        case OpCode_UserControlPressed:     return "UserControlPressed";
        case OpCode_UserControlReleased:    return "UserControlReleased";
        case OpCode_GiveDevicePowerStatus:  return "GiveDevicePowerStatus";
        case OpCode_ReportPowerStatus:      return "ReportPowerStatus";
        case OpCode_FeatureAbort:           return "FeatureAbort";
        case OpCode_Abort:                  return "Abort";
        case OpCode_GiveAudioStatus:        return "GiveAudioStatus";
        case OpCode_GiveSystemAudioModeStatus: return "GiveSystemAudioModeStatus";
        case OpCode_ReportAudioStatus:      return "ReportAudioStatus";
        case OpCode_SetSystemAudioMode:     return "SetSystemAudioMode";
        case OpCode_SystemAudioModeRequest: return "SystemAudioModeRequest";
        case OpCode_SystemAudioModeStatus:  return "SystemAudioModeStatus";
        case OpCode_SetAudioRate:           return "SetAudioRate";
        default: break;
    }
    return "Invalid";
}

const char* GetFrameTypeString( FrameType type )
{
    switch( type )
    {
        case FrameType_StartSeq: return "StartSeq";
        case FrameType_Header:   return "Header";
        case FrameType_OpCode:   return "OpCode";
        case FrameType_Operand:  return "Operand";
        case FrameType_EOM:      return "EOM";
        case FrameType_ACK:      return "ACK";
        default: break;
    }
    return "Invalid";
}

} // namespace HdmiCec
