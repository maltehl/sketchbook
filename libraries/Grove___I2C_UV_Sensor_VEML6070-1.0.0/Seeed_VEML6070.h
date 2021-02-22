#ifndef _SEEED_VE_H
#define _SEEED_VE_H

#include "arduino.h"
#include "Wire.h"


#ifndef SEEED_DN_DEFINES
#define SEEED_DN_DEFINES

#ifdef ARDUINO_SAMD_VARIANT_COMPLIANCE
  #define SERIAL_DB SerialUSB
#else
  #define SERIAL_DB Serial
#endif


typedef int            s32;
typedef long unsigned int   u32;
typedef short          s16;
typedef unsigned short u16;
typedef char           s8;
typedef unsigned char  u8;

typedef enum	
{
    NO_ERROR=0,
    ERROR_PARAM=-1,
    ERROR_COMM =-2,
    ERROR_OTHERS=-128,
}err_t;


#define CHECK_RESULT(a,b)   do{if(a=b)  {    \
                            SERIAL_DB.print(__FILE__);    \
                            SERIAL_DB.print(__LINE__);   \
                            SERIAL_DB.print(" error code =");  \
                            SERIAL_DB.println(a);                   \
                            return a;   \
                            }}while(0)

#endif

#define ENABLE 1
#define DISABLE 0
#define INT_102_STEP    0
#define INT_145_STEP    1


#define IIC_ADDR_H      0X39
#define IIC_ADDR_L      0X38
#define IIC_ADDR_ARA    0X0C




/*Three IIC address,Different functions are implemented by different addresses*/
/* F**K!!! */
#define ENABLE_BIT          0X01
#define INT_ENABLE_BIT      0X20
#define THRES_BIT           0X10

#define INTEGRA_BIT         0X0C

/**Divide UV index into five levels:LOW,MODERATE,HIGH,VERY_HIGH,EXTREME.
 * 
*/
typedef enum 
{
    LOW_LEVEL=0,
    MODERATE, 
    HIGH_LEVEL,
    VERY_HIGH, 
    EXTREME,
} RISK_LEVEL;


/** Tntegration time,When Rset =300kΩ，T=125ms，Rset = 600KΩ,T=-250ms
 * 
 * */
typedef enum
{
    HALF_OF_T=0X00,
    ONE_T=0X04,
    TWO_T=0X08,
    FOUR_T=0X0C,
}integration_time_t;



class VEML6070
{
    public:
        VEML6070();
        ~VEML6070(){}
        err_t clear_ack();
        err_t set_cmd_reg();
        err_t read_step(u16 &step);

        err_t init();
        err_t disable();
        err_t enable();
        err_t set_interrupt(bool thre,bool stat);
        err_t set_integra_time(integration_time_t T);
        void wait_for_ready();
        RISK_LEVEL convert_to_risk_level(u16 uvs_step);
    private:
        u8 addr_h;
        u8 addr_l;
        u8 addr_ara;
        u8 cmd_reg;
};








#endif
