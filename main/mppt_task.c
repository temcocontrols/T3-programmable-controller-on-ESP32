#include "mppt_task.h"
#include "INA228.h"
#include "driver/gpio.h"
#include "user_data.h"
#include "driver/ledc.h"
#include "modbus.h"

#define MPPT_POWER_LED	27
//#define MPPT_OUTPUT_ON_OFF	GPIO_NUM_5
#define MPPT_OUTPUT_EN	33
#define MPPT_OUTPUT_PWM	32
#define MPPT_OUTPUT_RELAY	GPIO_NUM_15

#define MPPT_POWER_LED_SEL (1ULL<<MPPT_POWER_LED)
#define MPPT_OUTPUT_EN_SEL	(1ULL<<MPPT_OUTPUT_EN)
#define MPPT_OUTPUT_PWM_SEL	(1ULL<<MPPT_OUTPUT_PWM)
#define MPPT_OUTPUT_RELAY_SEL	(1ULL<<MPPT_OUTPUT_RELAY)

#define MPPT_HS_TIMER          LEDC_TIMER_0
#define MPPT_HS_MODE           LEDC_HIGH_SPEED_MODE
#define MPPT_HS_CH0_GPIO       (32)
#define MPPT_HS_CH0_CHANNEL    LEDC_CHANNEL_0
#define MPPT_CH_NUM			   1

#define MANUAL_MODE 0
#define MPPT_MODE 1
#define CC_CV_PSU_MODE 2
#define PID_MODE 3

#define MAX_PWM_PULSE	250

mppt_t gMPPT;
extern uint16_t Test[50];

void mppt_pwm_init(void)
{
	int ch;

	ledc_timer_config_t mppt_pwm_timer = {
		.duty_resolution = LEDC_TIMER_8_BIT, // resolution of PWM duty
		.freq_hz = 100000,                      // frequency of PWM signal
		.speed_mode = MPPT_HS_MODE,           // timer mode
		.timer_num = MPPT_HS_TIMER,            // timer index
		.clk_cfg = LEDC_USE_APB_CLK,//LEDC_AUTO_CLK,              // Auto select the source clock
	};

	ledc_timer_config(&mppt_pwm_timer);
	ledc_channel_config_t mppt_channel[MPPT_CH_NUM] = {
		{
			.channel    = MPPT_HS_CH0_CHANNEL,
			.duty       = 0,
			.gpio_num   = MPPT_HS_CH0_GPIO,
			.speed_mode = MPPT_HS_MODE,
			.hpoint     = 0,
			.timer_sel  = MPPT_HS_TIMER
		},
	};

	for (ch = 0; ch < MPPT_CH_NUM; ch++) {
		ledc_channel_config(&mppt_channel[ch]);
	}

	ledc_set_duty(mppt_channel[0].speed_mode, mppt_channel[0].channel, gMPPT.output_pwm);
	ledc_update_duty(mppt_channel[0].speed_mode, mppt_channel[0].channel);
}

void mppt_task_init(void)
{
	gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set
	io_conf.pin_bit_mask = MPPT_POWER_LED_SEL | MPPT_OUTPUT_EN_SEL | MPPT_OUTPUT_RELAY_SEL;//| MPPT_OUTPUT_PWM_SEL ;
	//disable pull-down mode
	io_conf.pull_down_en = 0;
	//disable pull-up mode
	io_conf.pull_up_en = 0;
	//configure GPIO with the given settings
	gpio_config(&io_conf);

	gpio_set_level(MPPT_POWER_LED, 0);

	ina228_i2c_init();
	ina228_init(I2C_MASTER_NUM, INA228_SLAVE_ADDRESS);
	ina228_init(I2C_MASTER_NUM, INA228_SLAVE_OUTPUT_ADDRESS);

	mppt_pwm_init();

	xTaskCreate(mppt_task, "mppt_task", 2048*2, NULL, 2, NULL);
	xTaskCreate(ina228_read_task, "ina228_read_task", 2048*2, NULL, 2, NULL);
}

void ina228_read_task(void* arg)
{

	while(1)
	{
		Test[47] = gMPPT.input_voltage = (uint32_t)(ina228_voltage(I2C_MASTER_NUM, INA228_SLAVE_ADDRESS)*1000);
		inputs[0].value = (uint32_t)(ina228_voltage(I2C_MASTER_NUM, INA228_SLAVE_ADDRESS)*1000);
		inputs[1].value = Test[48] = gMPPT.input_current = (uint32_t)(ina228_current(I2C_MASTER_NUM, INA228_SLAVE_ADDRESS)*1000);
		inputs[2].value = Test[49] = gMPPT.input_power = (uint32_t)(ina228_power(I2C_MASTER_NUM, INA228_SLAVE_ADDRESS)*1000);
		inputs[3].value = Test[46] = gMPPT.input_energy = (uint32_t)(ina228_energy(I2C_MASTER_NUM, INA228_SLAVE_ADDRESS)*1000);
		inputs[4].value = Test[40] = gMPPT.output_voltage = (uint32_t)(ina228_voltage(I2C_MASTER_NUM, INA228_SLAVE_OUTPUT_ADDRESS)*1000);
		inputs[5].value = Test[41] = gMPPT.output_current = (uint32_t)(ina228_current(I2C_MASTER_NUM, INA228_SLAVE_OUTPUT_ADDRESS)*1000);
		inputs[6].value = Test[42] = gMPPT.output_power = (uint32_t)(ina228_power(I2C_MASTER_NUM, INA228_SLAVE_OUTPUT_ADDRESS)*1000);
		inputs[7].value = Test[43] = gMPPT.output_energy = (uint32_t)(ina228_energy(I2C_MASTER_NUM, INA228_SLAVE_OUTPUT_ADDRESS)*1000);
		//Test[45] = outputs[0].value/1000;

		vTaskDelay(2000 / portTICK_RATE_MS);//pdMS_TO_TICKS(1000));
	}
}

void Charging_Algorithm(){
     /////////////////////// CC-CV BUCK PSU ALGORITHM //////////////////////////////
      //
      // PSU and CC-CV mode charging algorithm issue, charging current setpoint, if the setpoint is greater than the input source rating, it will cause the PWM to continuously increase, lowering the input voltage until it is less than the battery voltage, and restart
      // If a smaller value is set, the maximum power cannot be utilized
	Str_points_ptr ptr;
    if(gMPPT.MPPT_Mode == CC_CV_PSU_MODE){                                                              // CC-CV PSU mode
      //if(PSUcurrentMax>=currentCharging || PSUcurrentMax==0.0000 || currentOutput<0.02){PSUcurrentMax = currentCharging;} // Initialize PSU input maximum current

//     ptr = put_io_buf(VAR , 0);
//     gMPPT.voltageBatteryMax = ptr.pvar->value;
//     ptr = put_io_buf(VAR, 1);
//     gMPPT.currentCharging = ptr.pvar->value;

      if(gMPPT.output_current > gMPPT.currentCharging)     {gMPPT.PWM--;}                             // Current above limit → decrease duty cycle
      // PSU mode and PSU charging mode need to be distinguished, charging mode can squeeze the input source for charging, PSU mode does not necessarily need to, so temporarily disable this judgment 20220811
      //if(currentOutput>PSUcurrentMax)       {PWM--;}                               // Current above external maximum value → decrease duty cycle
      //else
      else if(gMPPT.output_voltage > gMPPT.voltageBatteryMax){gMPPT.PWM--;}                             // Voltage above → decrease duty cycle
      else if(gMPPT.output_voltage < gMPPT.voltageBatteryMax){gMPPT.PWM++;}                             // Increase duty cycle when output is below charging voltage (only for CC-CV mode)
      else{}                                                                       // Do nothing when the set output voltage is reached
      if(gMPPT.PWM>MAX_PWM_PULSE)
    	  gMPPT.PWM = MAX_PWM_PULSE;
      //PWM_Modulation();                                                            // Set PWM signal to Buck PWM GPIO
      gMPPT.voltageInputPrev = gMPPT.input_voltage;                                             // Store previously recorded voltage
    }
    ///////////////////////  MPPT & CC-CV charging algorithm ///////////////////////  MPPT mode only prevents voltage from dropping to battery voltage protection and restarting unnecessary actions
    else if(gMPPT.MPPT_Mode == MPPT_MODE){
      if(gMPPT.output_current > gMPPT.currentCharging){gMPPT.PWM--;}                                         // Current above → decrease duty cycle
      else if(gMPPT.output_current > gMPPT.voltageBatteryMax){gMPPT.PWM--;}                                  // Voltage above → decrease duty cycle
      else{                                                                             // MPPT algorithm
        if(gMPPT.output_current > 100 && gMPPT.input_voltage >= (gMPPT.output_voltage + gMPPT.voltageDropout + 1)){       // No reverse current, process PWM when input is greater than battery voltage to prevent excessive voltage drop 20220803
          if(gMPPT.input_power > gMPPT.powerInputPrev && gMPPT.input_voltage > gMPPT.voltageInputPrev)     {gMPPT.PWM--;}   //  ↑P ↑V ; →MPP //D--  Power up and voltage up, continue to raise voltage
          else if(gMPPT.input_power > gMPPT.powerInputPrev && gMPPT.input_voltage < gMPPT.voltageInputPrev){gMPPT.PWM++;}   //  ↑P ↓V ; MPP← //D++  Power up and voltage down, continue to lower voltage
          else if(gMPPT.input_power < gMPPT.powerInputPrev && gMPPT.input_voltage > gMPPT.voltageInputPrev){gMPPT.PWM++;}   //  ↓P ↑V ; MPP→ //D++  Power down, voltage up, try to lower voltage
          else if(gMPPT.input_power < gMPPT.powerInputPrev && gMPPT.input_voltage < gMPPT.voltageInputPrev){gMPPT.PWM--;}   //  ↓P ↓V ; ←MPP  //D--  Power down, voltage down, try to raise voltage
          else if(gMPPT.output_voltage > gMPPT.voltageBatteryMax)                           {gMPPT.PWM--;}   //  MP MV ; reach MPP
          else if(gMPPT.output_voltage < gMPPT.voltageBatteryMax)                           {gMPPT.PWM++;}   //  MP MV ; reach MPP
        }else{
          gMPPT.PWM--;
        }

        if(gMPPT.output_current <= 0){gMPPT.PWM = gMPPT.PWM + 2;}  // Output current negative value
        gMPPT.powerInputPrev   = gMPPT.input_power;                                               //  Store previously recorded power
        gMPPT.voltageInputPrev = gMPPT.input_voltage;                                             //  Store previously recorded voltage
      }
      if(gMPPT.PWM>MAX_PWM_PULSE)
    	  gMPPT.PWM = MAX_PWM_PULSE;
    }
}

void mppt_task(void* arg)
{
	static bool power_led_trigger=0;
	Str_points_ptr ptr;

	//mppt_pwm_init();
	gMPPT.output_pwm = 127;
	gMPPT.PWM = 0;
	gpio_set_level(MPPT_OUTPUT_EN, 1);
	//gpio_set_level(MPPT_OUTPUT_PWM, 1);
	gpio_set_level(MPPT_POWER_LED, 1);

	ptr = put_io_buf(OUT, 0);
	memcpy(ptr.pout->description,"PER OF CHARGE",strlen("PER OF CHARGE"));
	ptr.pout->digital_analog = 1;
	ptr.pout->auto_manual = 1;
	ptr.pout->range = P0_100_Open;

	ptr = put_io_buf(OUT, 1);
	memcpy(ptr.pout->description,"SWITCH OF CHARGE",strlen("SWITCH OF CHARGE"));
	ptr.pout->digital_analog = 0;
	ptr.pout->auto_manual = 1;
	ptr.pout->range = OFF_ON;

	ptr = put_io_buf(VAR , 0);
	memcpy(ptr.pvar->description, "VOLTAGE SETPOINT", strlen("VOLTAGE SETPOINT"));
	ptr.pvar->digital_analog = 1;
	ptr.pout->auto_manual = 1;
	ptr.pvar->range = Volts;

	ptr = put_io_buf(VAR , 1);
	memcpy(ptr.pvar->description, "CURRENT SETPOINT", strlen("CURRENT SETPOINT"));
	ptr.pvar->digital_analog = 1;
	ptr.pout->auto_manual = 1;
	ptr.pvar->range = Amps;

	ptr = put_io_buf(VAR , 2);
	memcpy(ptr.pvar->description, "CHARGING MODE", strlen("CHARGING MODE"));
	ptr.pvar->digital_analog = 1;
	ptr.pout->auto_manual = 1;
	ptr.pvar->range = unused;
	ptr.pvar->value = 0;


    while (1) {

    	ptr = put_io_buf(VAR , 2);
    	gMPPT.MPPT_Mode = ptr.pvar->value/1000;
    	ptr = put_io_buf(VAR , 1);
    	gMPPT.currentCharging = ptr.pvar->value;
        ptr = put_io_buf(VAR , 0);
        gMPPT.voltageBatteryMax = ptr.pvar->value;

		if (gMPPT.MPPT_Mode == MANUAL_MODE) {
		    // Manual mode handling code
			ptr = put_io_buf(OUT, 0);
			Test[44] = (ptr.pout->value/1000)*MAX_PWM_PULSE/100;

			ptr = put_io_buf(OUT, 1);
			if(ptr.pout->digital_analog == 0)	// For output switch
			{
				if(ptr.pout->value == 1){
					gpio_set_level(MPPT_OUTPUT_RELAY, 1);
				}
				else
					gpio_set_level(MPPT_OUTPUT_RELAY, 0);
			}

			if(gMPPT.output_current > gMPPT.currentCharging)     {gMPPT.PWM--;}

			else{
				gMPPT.PWM = (outputs[0].value/1000)*MAX_PWM_PULSE/100;
			}
			ledc_set_duty(MPPT_HS_MODE, MPPT_HS_CH0_CHANNEL, gMPPT.PWM);//gMPPT.output_pwm);
			ledc_update_duty(MPPT_HS_MODE, MPPT_HS_CH0_CHANNEL);
		} else if (gMPPT.MPPT_Mode == MPPT_MODE || gMPPT.MPPT_Mode == CC_CV_PSU_MODE) {
		    // MPPT mode and PSU handling code
			gpio_set_level(MPPT_OUTPUT_RELAY, 1);
			Charging_Algorithm();
			ledc_set_duty(MPPT_HS_MODE, MPPT_HS_CH0_CHANNEL, gMPPT.PWM);
			ledc_update_duty(MPPT_HS_MODE, MPPT_HS_CH0_CHANNEL);
		} else if (gMPPT.MPPT_Mode == PID_MODE) {
		    // PID mode handling code
			gpio_set_level(MPPT_OUTPUT_RELAY, 1);
			gMPPT.output_pwm = controllers[1].value; // Use PID to control output PWM
			if(gMPPT.input_voltage < gMPPT.output_voltage)
			    gMPPT.output_pwm++;
			else if(gMPPT.input_voltage > gMPPT.output_voltage)
			    gMPPT.output_pwm--;
			else
			{
			    // gMPPT.output_pwm = gMPPT.output_pwm;
			}
		}

        vTaskDelay(100 / portTICK_RATE_MS);//pdMS_TO_TICKS(1000));
    }
}


