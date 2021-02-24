#include "Seeed_VEML6070.h"
#include "math.h"




VEML6070::VEML6070()
{
	addr_h = IIC_ADDR_H;
    addr_l = IIC_ADDR_L;
    addr_ara = IIC_ADDR_ARA;

	cmd_reg=0x02;
	Wire.begin();
}

/**clear ACK bit,be used of init or interrupt mode.
 * 
 * */
err_t VEML6070::clear_ack()
{
	err_t ret=NO_ERROR;
	Wire.requestFrom(addr_ara,1);
	return ret;
}

/**Set command register.
 * */
err_t VEML6070::set_cmd_reg()
{
	int ret;
	Wire.begin();
	Wire.beginTransmission(addr_l);
	Wire.write(cmd_reg);
	if(Wire.endTransmission())
	{
		return ERROR_COMM;
	}
	return NO_ERROR;
}


/**Enable the sensor mesurement,default is enable state.
 * */
err_t VEML6070::enable()
{
	err_t ret=NO_ERROR;
	cmd_reg &= ~ENABLE_BIT;
	CHECK_RESULT(ret,set_cmd_reg());
	return ret;
}

/**Disable the sensor mesurement,enter shutdown mode to save power.
 * */
err_t VEML6070::disable()
{
	err_t ret=NO_ERROR;
	cmd_reg |= ENABLE_BIT;
	CHECK_RESULT(ret,set_cmd_reg());
	return ret;
}



/**Read step of UV sensor,0~65535
 * */
err_t VEML6070::read_step(u16 &step)
{
	err_t ret=NO_ERROR;
	u32 time_out_count=0;
	step=0;
	u8 high=0,low=0;
	Wire.requestFrom(addr_h,1);
	while(Wire.available() != 1)
	{
		time_out_count++;
        if(time_out_count>10)  return ERROR_COMM;
        delay(1);
	}
	high=Wire.read();
	step|=(u16)high<<8;
	Wire.requestFrom(addr_l,1);
	while(Wire.available() != 1)
	{
		time_out_count++;
        if(time_out_count>10)  return ERROR_COMM;
        delay(1);
	}
	low=Wire.read();
	step|=low;
	return NO_ERROR;
}


/*param thre, the threshold to be set,0-102 steps,1-145 steps,ONLY SUPPORT TWO CONSTANT THRESHOLD,USER CAN'T CUSTOMIZE,F**C!!!*/
/*param stat,the enable bit,0-disable,1-enable.*/
err_t VEML6070::set_interrupt(bool thre,bool stat)
{
	err_t ret=NO_ERROR;

	clear_ack();

	if(thre) cmd_reg|=THRES_BIT;
	else cmd_reg&=~THRES_BIT;

	if(stat) cmd_reg|=INT_ENABLE_BIT;
	else cmd_reg&=~INT_ENABLE_BIT;

	Serial.print("cmd reg=");
	Serial.println(cmd_reg,HEX);
	return ret;
}

/**The sensor board's RSET = 270kΩ,1/2T=62.25ms,1T=125ms,2T=250ms,4T=500ms.
 * 
*/
err_t VEML6070::set_integra_time(integration_time_t T)
{
	err_t ret=NO_ERROR;
	switch(T)
	{
		case HALF_OF_T:
			cmd_reg&=~HALF_OF_T;
			cmd_reg|=HALF_OF_T;
		break;
		case ONE_T:
			cmd_reg&=~ONE_T;
			cmd_reg|=ONE_T;
		break;
		case TWO_T:
			cmd_reg&=~TWO_T;
			cmd_reg|=TWO_T;
		break;
		case FOUR_T:
			cmd_reg&=~FOUR_T;
			cmd_reg|=FOUR_T;
		break;
		default:
		break;
	}
	CHECK_RESULT(ret,set_cmd_reg());
	return ret;
}

/**Wait a integration time
 * 
 * */
void VEML6070::wait_for_ready()
{
	u8 integra_time=0;
	integra_time=cmd_reg & INTEGRA_BIT;
	
	delay(63*pow(2,integra_time/4));
}



RISK_LEVEL VEML6070::convert_to_risk_level(u16 uvs_step)
{
	u16 risk_level_mapping_table[4] = {2241, 4482, 5976, 8217};
	u32 i;
	for (i = 0; i < 4; i++)
	{

		if (uvs_step <= risk_level_mapping_table[i])
		{
			break;
		}
	}
	return (RISK_LEVEL)i;
}


err_t VEML6070::init()
{
	err_t ret=NO_ERROR;
	CHECK_RESULT(ret,clear_ack());
	CHECK_RESULT(ret,set_cmd_reg());
	return ret;
}


