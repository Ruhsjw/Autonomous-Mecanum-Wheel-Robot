#ifndef PARAMS_H
#define PARAMS_H

/*
motor 1 --- motor 2
         |
         |
         |
         |
         |
motor 4 --- motor 3
*/

// motor 1 data
#define PWM1_PIN 1
#define DIR1_PIN 2
#define ENCODER1_1_PIN 4
#define ENCODER1_2_PIN 5

// motor 2 data
#define PWM2_PIN 16
#define DIR2_PIN 15
#define ENCODER2_1_PIN 6
#define ENCODER2_2_PIN 7

// motor 3 data
#define PWM3_PIN 9
#define DIR3_PIN 8
#define ENCODER3_1_PIN 17
#define ENCODER3_2_PIN 18

// motor 4 data
#define PWM4_PIN 13
#define DIR4_PIN 12
#define ENCODER4_1_PIN 11
#define ENCODER4_2_PIN 10


#define MOTOR_PWM_RES 8
#define MOTOR_PWM_CNT_MAX 255
#define MOTOR_PWM_FREQ 4096
#define MOTOR_ENCODER_MAX_READ 100

#define MOTOR_KP 0.25f
#define MOTOR_KI 0.1f
#define MOTOR_KD 0.1f

#define MOTOR_KI_LEAK 0.99f

#define MOTOR_COUNT_KP 0.3f
#define MOTOR_COUNT_KI 0.01f
#define MOTOR_COUNT_KD 0.07f
#define MOTOR_COUNT_KI_LEAK 0.95

#define MOTOR_MIN_DUTY 30
#define MOTOR_DEADZONE 5.0f

#define SENSOR_I2C_SDA 14
#define SENSOR_I2C_SCL 19
#define I2C_MUX_ADDR 0x70

#define VIVE_PIN_1 21
#define VIVE_PIN_2 20

#define VIVE_X_DIFF 200
#define VIVE_Y_DIFF 150

#define SERVO_MOTOR_PIN 38

#define WHEEL_DIAMETER 80.0f
#define COUNTS_PER_REV 1120.0f
#define WHEEL_CIRC_MM PI * WHEEL_DIAMETER
#define COUNTS_PER_MM COUNTS_PER_REV / (WHEEL_CIRC_MM)

#define FIELD_WIDTH_MM   1524.0f  
#define FIELD_HEIGHT_MM  3658.0f


#define LOWER_TOWER_RED_X 4500
#define LOWER_TOWER_RED_Y 4700

#define LOWER_TOWER_BLUE_X 4500
#define LOWER_TOWER_BLUE_Y 3300

#define HIGHER_TOWER_RED_X 2950
#define HIGHER_TOWER_RED_Y 4670

#define HIGHER_TOWER_BLUE_X 2950
#define HIGHER_TOWER_BLUE_Y 3500

#define NEXUS_RED_X 4450
#define NEXUS_RED_Y 6280

#define NEXUS_BLUE_X 4650
#define NEXUS_BLUE_Y 1900


// Define sector area calculation 
#define RED_X_MIN   2000
#define RED_Y_MIN   4100
#define RED_X_MAX   3800
#define RED_Y_MAX   7000

#define PURPLE_X_MIN 2000
#define PURPLE_Y_MIN 1000
#define PURPLE_X_MAX (RED_X_MAX)
#define PURPLE_Y_MAX (RED_Y_MIN)


#define GREEN_X_MIN   3800
#define GREEN_Y_MIN   4400
#define GREEN_X_MAX   7000
#define GREEN_Y_MAX   8000

#define BLUE_X_MIN   3800
#define BLUE_Y_MIN   1000
#define BLUE_X_MAX   7000
#define BLUE_Y_MAX   3600

#define PINK_X_MIN   3800
#define PINK_Y_MIN   3600
#define PINK_X_MAX   4300
#define PINK_Y_MAX   4400

#define ORANGE_X_MIN   4700
#define ORANGE_Y_MIN   3600
#define ORANGE_X_MAX   7000
#define ORANGE_Y_MAX   4400


#define P0_X 3100
#define P0_Y 6450

#define P1_X 3900
#define P1_Y 6460

#define P2_X 4000
#define P2_Y 4700

#define P3_X 5150
#define P3_Y 4700

#define P6_X 4000
#define P6_Y 3300

#define P7_X 5150
#define P7_Y 3300

#define P8_X 3950
#define P8_Y 1600

#define P9_X 3150
#define P9_Y 1600

#define P10_X 4000
#define P10_Y 6300

#define P11_X 5150
#define P11_Y 6300

#define P12_X 4000
#define P12_Y 1800

#define P13_X 5150
#define P13_Y 1800



#define TOWER_X_MIN_MM   700.0f  
#define TOWER_X_MAX_MM   900.0f   
#define TOWER_Y_MIN_MM   1700.0f  
#define TOWER_Y_MAX_MM   1900.0f 

#define BLUE_X_MIN_MM    (RED_X_MAX_MM)   
#define BLUE_Y_MIN_MM    0.0f
#define BLUE_X_MAX_MM    (FIELD_WIDTH_MM)
#define BLUE_Y_MAX_MM    (TOWER_Y_MIN_MM) 

#define GREEN_X_MIN_MM   (RED_X_MAX_MM)
#define GREEN_Y_MIN_MM   (TOWER_Y_MAX_MM) 
#define GREEN_X_MAX_MM   (FIELD_WIDTH_MM)
#define GREEN_Y_MAX_MM   (FIELD_HEIGHT_MM)

#define DEBUG
#endif