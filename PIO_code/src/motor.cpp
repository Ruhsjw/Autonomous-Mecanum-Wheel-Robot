#include "motor.h"




/**
 * @brief initialization of the motor class
 * @param dir the direction of the count; 1 if forward and 0 if backward
 * @return none
 */
Motor::Motor(String in_motor_name, uint8_t direction_pin, uint8_t pwm_pin, uint8_t encoder_1, uint8_t encoder_2)
{
    motor_name = in_motor_name;
    DIR_PIN = direction_pin;
    PWM_PIN = pwm_pin;
    ENCODER_PIN1 = encoder_1;
    ENCODER_PIN2 = encoder_2;
    
    // Initialize output pins
    pinMode(DIR_PIN, OUTPUT);
    pinMode(PWM_PIN, OUTPUT);
    digitalWrite(DIR_PIN, LOW);
    digitalWrite(PWM_PIN, LOW);
    ledcAttach(PWM_PIN, MOTOR_PWM_FREQ, MOTOR_PWM_RES);
    pinMode(ENCODER_PIN1, INPUT);
    pinMode(ENCODER_PIN2, INPUT);
}


// should be called in setup()
void Motor::begin()
{
    attachInterrupt(digitalPinToInterrupt(ENCODER_PIN1), std::bind(&Motor::risingISR, this), RISING);
}


// Set the target motor speed
void Motor::set_target_speed(int in_target_speed)
{
    in_target_speed = in_target_speed > MOTOR_PWM_CNT_MAX ? MOTOR_PWM_CNT_MAX : in_target_speed;
    in_target_speed = in_target_speed < (- MOTOR_PWM_CNT_MAX) ? (- MOTOR_PWM_CNT_MAX) : in_target_speed;
    target_speed = static_cast<float>(map(in_target_speed, -100, 100, - MOTOR_ENCODER_MAX_READ, MOTOR_ENCODER_MAX_READ));
}

void Motor::test_motor()
{
    int measured_cnt_tmp = measured_count;
    digitalWrite(DIR_PIN, HIGH);
    digitalWrite(PWM_PIN, LOW);
    for (int i = 0; i < 200; ++i)
    {
        Serial.printf("Encoder counter = %d. \n", measured_count - measured_cnt_tmp);
        measured_cnt_tmp = measured_count;
        delay(50);
    }
    digitalWrite(DIR_PIN, LOW);
    digitalWrite(PWM_PIN, LOW);
    delay(1000);
    digitalWrite(DIR_PIN, LOW);
    digitalWrite(PWM_PIN, HIGH);
    for (int i = 0; i < 200; ++i)
    {
        Serial.printf("Encoder counter = %d. \n", measured_count - measured_cnt_tmp);
        measured_cnt_tmp = measured_count;
        delay(50);
    }
}

/**
 * @brief update the speed of the motor, should be called every 50ms
 */
void Motor::update_measured_speed()
{
    measured_speed = measured_count - measured_count_prev;
    // Serial.printf("speed = %f\n", measured_speed);
    measured_speed = static_cast<float>(map(measured_speed, -MOTOR_ENCODER_MAX_READ, MOTOR_ENCODER_MAX_READ, -MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX)); 
    measured_count_prev = measured_count;
}

/**
 * @brief update the counter of the motor, should be called only after update_flag is raised
 */
void Motor::update_motor(uint8_t type)
{
    acceleration = measured_speed - measured_speed_prev;
    measured_speed_prev = measured_speed; 
    speed_summed_error = speed_summed_error * MOTOR_KI_LEAK + (target_speed - measured_speed);
    output_speed = MOTOR_KP * (target_speed - measured_speed) + MOTOR_KD * acceleration + MOTOR_KI * speed_summed_error;
    output_speed = output_speed < 256 ? output_speed : 255;
    output_speed = output_speed > - 256 ? output_speed : - 255;
    if (output_speed > MOTOR_DEADZONE)
    {
        output_speed = MOTOR_MIN_DUTY + (output_speed / MOTOR_PWM_CNT_MAX) * (MOTOR_PWM_CNT_MAX - MOTOR_MIN_DUTY);
    }
    else if (output_speed < - MOTOR_DEADZONE)
    {
        output_speed = - MOTOR_MIN_DUTY - ((- output_speed) / MOTOR_PWM_CNT_MAX) * (MOTOR_PWM_CNT_MAX - MOTOR_MIN_DUTY);
    }
    else
    {
        output_speed = 0.0f;
    }

    int output_pwm = static_cast<int>(output_speed + 0.5f);
    // Serial.printf("%s: target_speed = %.2f, output speed = %d\n", motor_name, target_speed, output_pwm);

    if (output_pwm >= 0)
    {
        digitalWrite(DIR_PIN, HIGH);
        ledcWrite(PWM_PIN, MOTOR_PWM_CNT_MAX - output_pwm);
    }
    else
    {
        digitalWrite(DIR_PIN, LOW);
        ledcWrite(PWM_PIN, - output_pwm);
    }
}

// set target distance
void Motor::set_target_distance(int in_target_distance_mm)
{
    target_count = in_target_distance_mm * COUNTS_PER_MM + measured_count;
    count_summed_error = 0;
    count_error_prev = 0;
    // Serial.printf("target_count is %d, measured_count is %d\n", target_count, measured_count);
}

void IRAM_ATTR Motor::risingISR()
{
    if (digitalRead(ENCODER_PIN2) == LOW)   ++ measured_count;
    else                                    -- measured_count;
}

void Motor::reset_count_err()
{
    count_summed_error = 0;
}

void Motor::reset_speed_err()
{
    speed_summed_error = 0;
}

Motor::~Motor()
{

}
