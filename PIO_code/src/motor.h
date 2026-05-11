#include <Arduino.h>
#include <FunctionalInterrupt.h>
#include "params.h"

class Motor
{
    private:
        String motor_name;
        uint8_t DIR_PIN;
        uint8_t PWM_PIN;
        uint8_t ENCODER_PIN1;
        uint8_t ENCODER_PIN2;
        float target_speed = 0;
        float target_speed_prev = 0;
        float output_speed = 0;
        int measured_count_prev = 0;
        int measured_count = 0;
        float measured_speed_prev = 0;
        float measured_speed = 0;
        float acceleration = 0;
        float speed_summed_error = 0;
        int target_count = 0;
        float count_summed_error = 0;
        float count_error_prev = 0;
        void risingISR();
    public:
        Motor(String in_motor_name, uint8_t direction_pin, uint8_t pwm_pin, uint8_t encoder_1, uint8_t encoder_2);
        void update_measured_speed();
        void update_motor(uint8_t type = 1);
        void reset_speed_err();
        void reset_count_err();
        void set_target_speed(int in_target_speed);
        void set_target_distance(int in_target_distance_mm);
        void begin();
        void test_motor();
        ~Motor();
};