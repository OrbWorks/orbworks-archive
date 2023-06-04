//
// beep types used by scanBeep() and scanSendParams()
//
#define BEEP_No_Beep           0x00
#define BEEP_One_Short_High    0x01
#define BEEP_Two_Short_High    0x02
#define BEEP_Three_Short_High  0x03
#define BEEP_Four_Short_High   0x04
#define BEEP_Five_Short_High   0x05

#define BEEP_One_Short_Low     0x06
#define BEEP_Two_Short_Low     0x07
#define BEEP_Three_Short_Low   0x08
#define BEEP_Four_Short_Low    0x09
#define BEEP_Five_Short_Low    0x0A

#define BEEP_One_Long_High     0x0B
#define BEEP_Two_Long_High     0x0C
#define BEEP_Three_Long_High   0x0D
#define BEEP_Four_Long_High    0x0E
#define BEEP_Five_Long_High    0x0F

#define BEEP_One_Long_Low      0x10
#define BEEP_Two_Long_Low      0x11
#define BEEP_Three_Long_Low    0x12
#define BEEP_Four_Long_Low     0x13
#define BEEP_Five_Long_Low     0x14

#define BEEP_Fast_Warble       0x15
#define BEEP_Slow_Warble       0x16
#define BEEP_Mix1              0x17
#define BEEP_Mix2              0x18
#define BEEP_Mix3              0x19
#define BEEP_Mix4              0x1A

#define BEEP_Decode_Beep       0x1B
#define BEEP_Bootup_Beep       0x1C
#define BEEP_Parameter_Entry_Error_Beep  0x1D
#define BEEP_Parameter_Defaults_Beep     0x1E
#define BEEP_Parameter_Entered_Beep      0x1F
#define BEEP_Host_Convert_Error_Beep     0x20
#define BEEP_Transmit_Error_Beep         0x21
#define BEEP_Parity_Error_Beep           0x22

//
// bar code types returned by scanGetDataType()
//
#define BCTYPE_NOT_APPLICABLE                       0x00
#define BCTYPE_CODE39                               0x01
#define BCTYPE_CODABAR                              0x02
#define BCTYPE_CODE128                              0x03
#define BCTYPE_D2OF5                                0x04
#define BCTYPE_IATA2OF5                             0x05
#define BCTYPE_I2OF5                                0x06
#define BCTYPE_CODE93                               0x07
#define BCTYPE_UPCA                                 0x08
#define BCTYPE_UPCA_2SUPPLEMENTALS                  0x48
#define BCTYPE_UPCA_5SUPPLEMENTALS                  0x88
#define BCTYPE_UPCE0                                0x09
#define BCTYPE_UPCE0_2SUPPLEMENTALS                 0x49
#define BCTYPE_UPCE0_5SUPPLEMENTALS                 0x89
#define BCTYPE_EAN8                                 0x0A
#define BCTYPE_EAN8_2SUPPLEMENTALS                  0x4A
#define BCTYPE_EAN13_5SUPPLEMENTALS                 0x8B
#define BCTYPE_EAN8_5SUPPLEMENTALS                  0x8A
#define BCTYPE_EAN13                                0x0B
#define BCTYPE_EAN13_2SUPPLEMENTALS                 0x4B
#define BCTYPE_MSI_PLESSEY                          0x0E
#define BCTYPE_EAN128                               0x0F
#define BCTYPE_UPCE1                                0x10
#define BCTYPE_UPCE1_2SUPPLEMENTALS                 0x50
#define BCTYPE_UPCE1_5SUPPLEMENTALS                 0x90
#define BCTYPE_CODE39_FULL_ASCII                    0x13
#define BCTYPE_TRIOPTIC_CODE39                      0x15
#define BCTYPE_BOOKLAND_EAN                         0x16
#define BCTYPE_COUPON_CODE                          0x17
#define BCTYPE_ISBT128                              0x19
#define BCTYPE_CODE32                               0x20

//
// scanning angle used by scanAngle() and scanGetAngle()
//
#define ANGLE_WIDE    0xB6
#define ANGLE_NARROW  0xB5

//
// triggering modes used by scanTriggerMode() and scanGetTriggerMode()
#define TRIG_LEVEL    0x00
#define TRIG_PULSE    0x02
#define TRIG_HOST     0x08

//
// bar code type used in scanEnableType()
//
#define BAR_CODE39        0x00
#define BAR_UPCA          0x01
#define BAR_UPCE          0x02
#define BAR_EAN13         0x03
#define BAR_EAN8          0x04
#define BAR_D25           0x05
#define BAR_I2OF5         0x06
#define BAR_CODABAR       0x07
#define BAR_CODE128       0x08
#define BAR_CODE93        0x09
#define BAR_TRIOPTIC39    0x0D
#define BAR_UCC_EAN128    0x0E
#define BAR_MSI_PLESSEY   0x0B
#define BAR_UPCE1         0x0C
#define BAR_BOOKLAND_EAN  0x53
#define BAR_ISBT128       0x54
#define BAR_COUPON        0x55
