#include <Arduino.h>
#include "params.h"
#include "Adafruit_VL53L0X.h"
#include "Wire.h"

#include <WiFi.h>
#include <WiFiUdp.h>
#include "html510.h"
#include "html.h"
#include "params.h"
#include "motor.h"
#include "vive.h"


bool estop_flag = false;
uint8_t vive_priority = 0;  // vive 1 = priority 0; vive 2 = priority 1

const char* ssid = "myEsp32";
const char* password = "esp32pass";

IPAddress MyIP(192,168,1,136);
HTML510Server server(80);

Motor motor_1("front_left", DIR1_PIN, PWM1_PIN, ENCODER1_1_PIN, ENCODER1_2_PIN);
Motor motor_2("front_right", DIR2_PIN, PWM2_PIN, ENCODER2_1_PIN, ENCODER2_2_PIN);
Motor motor_3("rear_right", DIR3_PIN, PWM3_PIN, ENCODER3_1_PIN, ENCODER3_2_PIN);
Motor motor_4("rear_left", DIR4_PIN, PWM4_PIN, ENCODER4_1_PIN, ENCODER4_2_PIN);


Adafruit_VL53L0X tof_1 = Adafruit_VL53L0X();
Adafruit_VL53L0X tof_2 = Adafruit_VL53L0X();
Adafruit_VL53L0X tof_3 = Adafruit_VL53L0X();
Adafruit_VL53L0X tof_4 = Adafruit_VL53L0X();

uint8_t tophat_flag = 0;
uint8_t wifi_comms_num = 0;

Vive vive_1(VIVE_PIN_1, 0);
Vive vive_2(VIVE_PIN_2, 1);

uint16_t vive_1_x = 0;
uint16_t vive_1_y = 0;
uint16_t vive_2_x = 0;
uint16_t vive_2_y = 0;
uint16_t vive_x = 0;
uint16_t vive_y = 0;


uint8_t speed_update_flag = 0;
uint8_t tof_update_flag = 0;
uint8_t vive_update_flag = 0;
uint8_t wall_follow_flag = 0;
uint8_t servo_mode = 0;
uint8_t motor_update_flag = 0;
uint8_t set_target_flag = 0;
uint8_t auto_attack_update_flag = 0;
uint8_t nexus_attack_counter = 0;
uint8_t attack_flag_red = 0;


uint8_t cap_low_tower_flag = 0;
uint8_t cap_high_tower_flag = 0;
uint8_t attack_nexus_flag = 0;
uint8_t attack_tower_nexus_flag = 0;
uint8_t cap_low_tower_flag_blue = 0;
uint8_t cap_high_tower_flag_blue = 0;
uint8_t attack_nexus_flag_blue = 0;
uint8_t attack_flag_blue = 0;
uint8_t auto_flag_red = 0;


hw_timer_t *timer = NULL;
hw_timer_t *tophat_timer = NULL;

uint16_t tof_1_value = 0;
uint16_t tof_2_value = 0;
uint16_t tof_3_value = 0;
uint16_t tof_4_value = 0;


// Wall-following parameters
uint16_t * list[] = {&tof_1_value, &tof_2_value, &tof_3_value, &tof_4_value};
uint8_t counter = 0;
uint16_t * front = list[counter];
uint16_t * right = list[counter + 1];
uint8_t noninterrupt_f_flag = 0;
uint8_t noninterrupt_b_flag = 0;
uint8_t noninterrupt_l_flag = 0;
uint8_t noninterrupt_r_flag = 0;
uint8_t noninterrupt_flag = 0;
uint8_t state = 0;
uint8_t tower_noninterrupt_f_flag = 0;
uint8_t tower_noninterrupt_b_flag = 0;
uint8_t tower_noninterrupt_l_flag = 0;
uint8_t tower_noninterrupt_r_flag = 0;


int paramA, paramB;

uint16_t target_vive_x = 0;
uint16_t target_vive_y = 0;

void set_target(uint16_t target_x, uint16_t target_y);

// === Position PID globals (Vive-based) ===

// Target position in Vive global coordinates
float pos_pid_target_x = 0.0f;
float pos_pid_target_y = 0.0f;

// Whether position PID control is active
bool pos_pid_active = false;

// PID gains for position control (tune these!)
float pos_pid_kp = 0.2f;     // proportional gain
float pos_pid_ki = 0.001f;      // integral gain (start at 0)
float pos_pid_kd = 0.05f;    // derivative gain

// Internal state
float pos_pid_err_x_int  = 0.0f;
float pos_pid_err_y_int  = 0.0f;
float pos_pid_prev_err_x = 0.0f;
float pos_pid_prev_err_y = 0.0f;

// Assumed control period (seconds) @ 20Hz
const float pos_pid_dt = 1.0f / 20.0f;

// Deadband radius around target (same units as Vive coords)
float pos_pid_deadband = 50.0f;  // e.g. 50 units

// Max command magnitude in the "joystick" units expected by set_speed()
float pos_pid_max_cmd = 80.0f;

// Robot heading in global frame (deg). If you later have a real heading,
// replace this with your actual orientation source.
float pos_pid_robot_heading_deg = 0.0f;

void pos_pid_set_target(uint16_t x, uint16_t y)
{
    pos_pid_target_x = static_cast<float>(x);
    pos_pid_target_y = static_cast<float>(y);

    // Reset PID state
    pos_pid_err_x_int  = 0.0f;
    pos_pid_err_y_int  = 0.0f;
    pos_pid_prev_err_x = 0.0f;
    pos_pid_prev_err_y = 0.0f;

    pos_pid_active = true;
}



// Rotate a vector (dx, dy) by theta_deg clockwise.
// This converts global desired motion into robot-local motion.
//
// Example: global dx=0, dy=100 (forward),
//          theta=90 deg clockwise → robot-local dx=100, dy=0 (right)
//
// Output is written into dx_out, dy_out.
void rotateGlobalToLocal(int dx_global, int dy_global, float theta_deg,
                         int &dx_out, int &dy_out)
{
    // Convert degrees to radians
    float th = theta_deg * PI / 180.0f;

    // CLOCKWISE rotation matrix
    // [ x' ]   [ cosθ   sinθ ] [ x ]
    // [ y' ] = [−sinθ   cosθ ] [ y ]

    float x_local_f =  dx_global * cosf(th) + dy_global * sinf(th);
    float y_local_f = -dx_global * sinf(th) + dy_global * cosf(th);

    // Convert back to int
    dx_out = (int)x_local_f;
    dy_out = (int)y_local_f;
}


void move(uint16_t x, uint16_t y, int degree)
{
    motor_1.set_target_distance(y + x + 3.8 * degree);
    motor_2.set_target_distance(y - x - 3.8 * degree);
    motor_3.set_target_distance(y + x - 3.8 * degree);
    motor_4.set_target_distance(y - x + 3.8 * degree);
}

// robot-frame velocities
// vx: forward, vy: left, w: CCW rotation
void mecanumKinematicsInt(int vx, int vy, int w,
                          int &fl, int &fr,
                          int &rl, int &rr)
{
    // Integer mixing
    int t_fl =  vy + vx - w;
    int t_fr =  vy - vx + w;
    int t_rl =  vy - vx - w;
    int t_rr =  vy + vx + w;

    // Find max
    int maxMag = abs(t_fl);
    if (abs(t_fr) > maxMag) maxMag = abs(t_fr);
    if (abs(t_rl) > maxMag) maxMag = abs(t_rl);
    if (abs(t_rr) > maxMag) maxMag = abs(t_rr);

    // Prevent division by 0
    if (maxMag < 100) maxMag = 100;

    // Normalize back to [-100, 100]
    fl = t_fl * 100 / maxMag;
    fr = t_fr * 100 / maxMag;
    rl = t_rl * 100 / maxMag;
    rr = t_rr * 100 / maxMag;
}



// handler for pwm duty cycle
void handleSpeed()
{
    ++ wifi_comms_num;
    String value = server.getText();
    value.replace("%2C", ",");
    server.sendplain("200");
    #ifdef DEBUG
    Serial.print("Speed: ");
    Serial.println(value);
    #endif
    int comma1 = value.indexOf(',');
    int comma2 = value.indexOf(',', comma1 + 1);
    int comma3 = value.indexOf(',', comma2 + 1);
    
    #ifdef JOYSTICK
    int x = value.substring(0, comma).toInt();
    int y = value.substring(comma+1).toInt();
    int l = y + x;
    int r = y - x;
    int m = abs(l) > abs(r) ? abs(l) : abs(r);
    if (m > 100)
    {
        l *= 100.0 / m;
        r *= 100.0 / m;
    }
    // TODO
    left_target_speed = map(l, -100, 100, - PWM_CNT_MAX, PWM_CNT_MAX);
    right_target_speed = map(r, -100, 100, - PWM_CNT_MAX, PWM_CNT_MAX);
    #endif

    #ifndef JOYSTICK
    uint8_t mode = value.substring(0, comma1).toInt();
    float x = value.substring(comma1 + 1, comma2).toInt();
    float y = value.substring(comma2 + 1, comma3).toInt();
    float w = value.substring(comma3 + 1).toInt();
    #ifdef DEBUG
    Serial.print("data = ");
    Serial.print(x);
    Serial.print(", ");
    Serial.println(y);
    #endif

    int fl, fr, rl, rr;
    mecanumKinematicsInt(x, y, w, fl, fr, rl, rr);

    fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    

    #ifdef DEBUG
    // Serial.print("command = ");
    // Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
    #endif

    motor_1.set_target_speed(fl);
    motor_2.set_target_speed(fr);
    motor_3.set_target_speed(rr);
    motor_4.set_target_speed(rl);
    #endif
}




// handler for sending the webpage
void handleRoot()
{
    server.sendhtml(body);
    #ifdef DEBUG
        Serial.println("body sent");
    #endif
}

void handleSetVals()
{
    ++ wifi_comms_num;
    // Expect "123,456" from getText()
    String txt = server.getText();  // reads until space / end-of-line

    int commaIndex = txt.indexOf(',');
    if (commaIndex > 0)
    {
        String s1 = txt.substring(0, commaIndex);
        String s2 = txt.substring(commaIndex + 1);

        long v1 = s1.toInt();
        long v2 = s2.toInt();

        // clamp to uint16 range
        if (v1 < 0) v1 = 0; else if (v1 > 65535) v1 = 65535;
        if (v2 < 0) v2 = 0; else if (v2 > 65535) v2 = 65535;

        target_vive_x = (uint16_t)v1;
        target_vive_y = (uint16_t)v2;
        set_target(target_vive_x, target_vive_y);
    }

  // Optional: send a simple ACK
    server.sendplain("OK");
}

void handleEStop() {
    ++ wifi_comms_num;
  // Set flag and stop everything you need
  estop_flag = true;

  // Example: zero joystick commands, stop motors, disable modes
  // joy_dx = joy_dy = joy_w = 0;
  // wall_following_flag = false;
  // servo_enabled = false;
  // stopAllMotors();

  server.sendplain("ESTOP");  // simple ACK
}

// handler for sending the count
// void handleMeasuredSpeed()
// {
//     String s = "Left speed is " + String(left_measured_speed) + ", target speed is " + String(left_target_speed) + ", output speed is" + String(left_output_speed) + ", right speed is " + String(right_measured_speed) + ", target speed is " + String(right_target_speed);
//     server.sendplain(s);
// }

void IRAM_ATTR onTimer()
{
    motor_update_flag = 1;
    speed_update_flag = 1;
    tof_update_flag = 1;
    vive_update_flag = 1;
    auto_attack_update_flag = 1;
}

void IRAM_ATTR on_tophat_timer()
{
    tophat_flag = 1;
}

void I2C_switch_channel(uint8_t channel)
{
    if (channel > 7) return;
    Wire.beginTransmission(I2C_MUX_ADDR);
    Wire.write(1 << channel);
    Wire.endTransmission();
}

void tof_init()
{
    // Setup I2C comms
    Wire.begin(SENSOR_I2C_SDA, SENSOR_I2C_SCL);
    Wire.setClock(40000);

    // Startup tof_1
    I2C_switch_channel(0);
    if (!tof_1.begin()) {
        Serial.println(F("Failed to boot tof1"));
        while(1);
    }
    tof_1.startRangeContinuous();

    // Startup tof_2
    I2C_switch_channel(1);
    if (!tof_2.begin()) {
        Serial.println(F("Failed to boot tof2"));
        while(1);
    }
    tof_2.startRangeContinuous();

    // Startup tof_3
    I2C_switch_channel(2);
    if (!tof_3.begin()) {
        Serial.println(F("Failed to boot tof3"));
        while(1);
    }
    tof_3.startRangeContinuous();

    // Startup tof_4
    I2C_switch_channel(3);
    if (!tof_4.begin()) {
        Serial.println(F("Failed to boot tof4"));
        while(1);
    }
    tof_4.startRangeContinuous();
}


void set_speed(int x, int y, int angle)
{
    int fl, fr, rl, rr;
    mecanumKinematicsInt(x, y, angle, fl, fr, rl, rr);

    fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
    

    #ifdef DEBUG
    // Serial.print("command = ");
    // Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
    #endif

    motor_1.set_target_speed(fl);
    motor_2.set_target_speed(fr);
    motor_3.set_target_speed(rr);
    motor_4.set_target_speed(rl);
}



void follow_edge_state()
{
    if (noninterrupt_l_flag)
    {
        -- noninterrupt_l_flag;
        int x, y = 0;
        rotateGlobalToLocal(-100, 0, counter * 90, x, y);
        set_speed(x, y, 0);
        return;
    }
    if (*front < 150)
    {
        if (state == 0)
        {
            ++ state;
            noninterrupt_l_flag = 25;
            counter = (counter - 1) % 4;
            front = list[(counter) % 4];
            right = list[(counter + 1) % 4];
        }
        else if (state == 1)
        {
            ++ state;
            counter = (counter - 1) % 4;
            front = list[(counter) % 4];
            right = list[(counter + 1) % 4];
        }
        else if (state == 2)
        {
            ++ state;
            counter = (counter - 1) % 4;
            front = list[(counter) % 4];
            right = list[(counter + 1) % 4];
        }
        else if (state == 3)
        {
            ++ state;
            noninterrupt_l_flag = 25;
        }
        else if (state == 4)
        {
            state = 0;
            counter = (counter - 1) % 4;
            front = list[(counter) % 4];
            right = list[(counter + 1) % 4];
        }
        return;
    }
    // Serial.printf("front = %d, right = %d", *front, *right);
    if (*right <= 80)
    {
        int x, y = 0;
        rotateGlobalToLocal(-50, 80, counter * 90, x, y);
        set_speed(x, y, 0);
    }
    else if (*right >= 100)
    {
        int x, y = 0;
        rotateGlobalToLocal(40, 70, counter * 90, x, y);
        set_speed(x, y, 0);
    }
    else
    {
        int x, y = 0;
        rotateGlobalToLocal(0, 100, counter * 90, x, y);
        set_speed(x, y, 0);
    }
}

uint8_t auto_tower_capture()
{
    // Serial.printf("back interrupt = %d, flag = %d\n", tower_noninterrupt_b_flag, cap_low_tower_flag);
    if (tower_noninterrupt_b_flag)
    {
        -- tower_noninterrupt_b_flag;
        if (!tower_noninterrupt_b_flag)
        {
            set_speed(0, 0, 0);
            return 0;
        }
        int x, y = 0;
        rotateGlobalToLocal(0, -100, 0, x, y);
        set_speed(x, y, 0);
        return 0;
    }
    else if (tower_noninterrupt_l_flag)
    {
        -- tower_noninterrupt_l_flag;
        if (!tower_noninterrupt_l_flag)
        {
            set_speed(0, 0, 0);
            return 0;
        }
        int x, y = 0;
        rotateGlobalToLocal(-100, 0, 0, x, y);
        set_speed(x, y, 0);
        return 0;
    }
    else if (tower_noninterrupt_r_flag)
    {
        -- tower_noninterrupt_r_flag;
        if (!tower_noninterrupt_r_flag)
        {
            set_speed(0, 0, 0);
            return 0;
        }
        int x, y = 0;
        rotateGlobalToLocal(100, 0, 0, x, y);
        set_speed(x, y, 0);
        return 0;
    }
    else if (tower_noninterrupt_f_flag)
    {
        -- tower_noninterrupt_f_flag;
        if (!tower_noninterrupt_f_flag)
        {
            set_speed(0, 0, 0);
            return 0;
        }
        int x, y = 0;
        rotateGlobalToLocal(0, 100, 0, x, y);
        set_speed(x, y, 0);
        return 0;
    } 

    if (attack_flag_red)
    {
        if (!set_target_flag)
        {
            nexus_attack_counter = 100;
            set_target_flag = 1;
        }
        else if (nexus_attack_counter)
        {
            if ((nexus_attack_counter % 2) == 0)
            {
                tower_noninterrupt_f_flag = 4;
            }
            else
            {
                tower_noninterrupt_b_flag = 3;
            }
            -- nexus_attack_counter;
        }
        else
        {
            attack_flag_red = 0;
            set_target_flag = 0;
        }
    }
    else if (attack_flag_blue)
    {

        if (!set_target_flag)
        {
            nexus_attack_counter = 100;
            set_target_flag = 1;
        }
        else if (nexus_attack_counter)
        {
            if ((nexus_attack_counter % 2) == 0)
            {
                tower_noninterrupt_f_flag = 3;
            }
            else
            {
                tower_noninterrupt_b_flag = 4;
            }
            -- nexus_attack_counter;
        }
        else
        {
            attack_flag_blue = 0;
            set_target_flag = 0;
        }
    }
    // capture low tower
    else if (cap_low_tower_flag)
    {
        // if have not move near, move to front
        if (!set_target_flag)
        {
            set_target(LOWER_TOWER_RED_X, LOWER_TOWER_RED_Y);
            set_target_flag = 1;
        }
        // if already arrived (pos pid update complete so nore more path following), move back 150ms to hit the button
        else if (!pos_pid_active)
        {
            tower_noninterrupt_b_flag = 14;
            cap_low_tower_flag = 0;
            set_target_flag = 0;
        }
    }
    else if (attack_nexus_flag)
    {
        // if have not move near, move to front
        if (!set_target_flag)
        {
            set_target(NEXUS_RED_X, NEXUS_RED_Y);
            if (cap_high_tower_flag)
            {
                nexus_attack_counter = 12;
            }
            else
            {
                nexus_attack_counter = 100;
            }
            set_target_flag = 1;
        }
        
        // else if (!pos_pid_active)
        // {
        //     tower_noninterrupt_f_flag = 15;
        //     attack_nexus_flag = 0;
        //     set_targetv_flag = 0;
        // }

        // if already arrived (pos pid update complete so nore more path following), move back 150ms to hit the button
        else if (!pos_pid_active)
        {
            if (nexus_attack_counter)
            {
                if ((nexus_attack_counter % 2) == 0)
                {
                    tower_noninterrupt_f_flag = 4;
                }
                else
                {
                    tower_noninterrupt_b_flag = 3;
                }
                -- nexus_attack_counter;
            }
            else
            {
                attack_nexus_flag = 0;
                set_target_flag = 0;
                if (cap_high_tower_flag)
                {
                    tower_noninterrupt_b_flag = 15;
                    tower_noninterrupt_l_flag = 8;
                }
            }
        }
    }
    // else if (auto_flag_red)
    // {
    //     // if have not move near, move to front
    //     if (!set_target_flag)
    //     {
    //         set_target(LOWER_TOWER_RED_X, LOWER_TOWER_RED_Y);
    //         set_target_flag = 1;
    //     }
    //     // if already arrived (pos pid update complete so nore more path following), move back 150ms to hit the button
    //     else if (!pos_pid_active)
    //     {
    //         tower_noninterrupt_b_flag = 12;
    //         auto_flag_red = 0;
    //         set_target_flag = 0;
    //     }
    // }
    else if (cap_high_tower_flag)
    {
        // if have not move near, move to front
        if (!set_target_flag)
        {
            set_target(HIGHER_TOWER_RED_X, HIGHER_TOWER_RED_Y);
            set_target_flag = 1;
        }
        // if already arrived (pos pid update complete so nore more path following), move back 150ms to hit the button
        else if (!pos_pid_active)
        {
            tower_noninterrupt_l_flag = 12;
            cap_high_tower_flag = 0;
            set_target_flag = 0;
        }
    }
    else if (cap_low_tower_flag_blue)
    {
        if (!set_target_flag)
        {
            set_target(LOWER_TOWER_BLUE_X, LOWER_TOWER_BLUE_Y);
            set_target_flag = 1;
        }
        else if (!pos_pid_active)
        {
            tower_noninterrupt_f_flag = 12;
            cap_low_tower_flag_blue = 0;
            set_target_flag = 0;
        }
    }
    else if (attack_nexus_flag_blue)
    {
        // if have not move near, move to front
        if (!set_target_flag)
        {
            set_target(NEXUS_BLUE_X, NEXUS_BLUE_Y);
            nexus_attack_counter = 100;
            set_target_flag = 1;
        }
        
        // else if (!pos_pid_active)
        // {
        //     tower_noninterrupt_f_flag = 15;
        //     attack_nexus_flag = 0;
        //     set_target_flag = 0;
        // }

        // if already arrived (pos pid update complete so nore more path following), move back 150ms to hit the button
        else if (!pos_pid_active)
        {
            if (nexus_attack_counter)
            {
                if ((nexus_attack_counter % 2) == 0)
                {
                    tower_noninterrupt_b_flag = 4;
                }
                else
                {
                    tower_noninterrupt_f_flag = 3;
                }
                -- nexus_attack_counter;
            }
            else
            {
                attack_nexus_flag = 0;
                set_target_flag = 0;
                if (cap_high_tower_flag)
                {
                    tower_noninterrupt_b_flag = 8;
                    tower_noninterrupt_l_flag = 35;
                    tower_noninterrupt_r_flag = 8;
                }
            }
        }
    }
    return 0;
}

// void follow_edge()
// {
//     if (noninterrupt_flag > 0)
//     {
//         -- noninterrupt_flag;
//         return;
//     }
//     if (*front <= 150)
//     {
//         int fl, fr, rl, rr;
//         mecanumKinematicsInt(0, 0, 100, fl, fr, rl, rr);

//         fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
        

//         #ifdef DEBUG
//         Serial.print("command = ");
//         Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
//         #endif

//         motor_1.set_target_speed(fl);
//         motor_2.set_target_speed(fr);
//         motor_3.set_target_speed(rr);
//         motor_4.set_target_speed(rl);
//         noninterrupt_flag = 20;
//         return;
//     }
//     // else
//     // {
//     //     int fl, fr, rl, rr;
//     //     mecanumKinematicsInt(0, 100, 0, fl, fr, rl, rr);

//     //     fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//     //     fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//     //     rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//     //     rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
        

//     //     #ifdef DEBUG
//     //     Serial.print("command = ");
//     //     Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
//     //     #endif

//     //     motor_1.set_target_speed(fl);
//     //     motor_2.set_target_speed(fr);
//     //     motor_3.set_target_speed(rr);
//     //     motor_4.set_target_speed(rl);
//     // }
//     if (*right <= 80)
//     {
//         int fl, fr, rl, rr;
//         mecanumKinematicsInt(-65, 80, 50, fl, fr, rl, rr);

//         fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
        

//         #ifdef DEBUG
//         Serial.print("command = ");
//         Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
//         #endif

//         motor_1.set_target_speed(fl);
//         motor_2.set_target_speed(fr);
//         motor_3.set_target_speed(rr);
//         motor_4.set_target_speed(rl);
//     }
//     else if (tof_2_value >= 130)
//     {
//         int fl, fr, rl, rr;
//         mecanumKinematicsInt(70, 80, -60, fl, fr, rl, rr);

//         fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
        

//         #ifdef DEBUG
//         Serial.print("command = ");
//         Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
//         #endif

//         motor_1.set_target_speed(fl);
//         motor_2.set_target_speed(fr);
//         motor_3.set_target_speed(rr);
//         motor_4.set_target_speed(rl);
//     }
//     else
//     {
//         int fl, fr, rl, rr;
//         mecanumKinematicsInt(0, 100, 0, fl, fr, rl, rr);

//         fl = map(fl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         fr = map(fr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rr = map(rr, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
//         rl = map(rl, -100, 100, - MOTOR_PWM_CNT_MAX, MOTOR_PWM_CNT_MAX);
        

//         #ifdef DEBUG
//         Serial.print("command = ");
//         Serial.printf("%d, %d, %d, %d\n", fl, fr, rr, rl);
//         #endif

//         motor_1.set_target_speed(fl);
//         motor_2.set_target_speed(fr);
//         motor_3.set_target_speed(rr);
//         motor_4.set_target_speed(rl);
//     }
// }



void get_car_angle()
{

}

struct Point
{
    uint16_t x;
    uint16_t y;
};

Point point_queue[5];
uint8_t point_queue_num = 0;

struct Sector
{
    String  name;
    uint8_t id;
    uint16_t x_min;
    uint16_t x_max;
    uint16_t y_min;
    uint16_t y_max;
};

const Sector sectors[] = {
    {"0", 0, RED_X_MIN, RED_X_MAX, RED_Y_MIN, RED_Y_MAX},
    {"1", 1, PURPLE_X_MIN, PURPLE_X_MAX, PURPLE_Y_MIN, PURPLE_Y_MAX},
    {"2", 2, GREEN_X_MIN, GREEN_X_MAX, GREEN_Y_MIN, GREEN_Y_MAX},
    {"3", 3, BLUE_X_MIN, BLUE_X_MAX, BLUE_Y_MIN, BLUE_Y_MAX},
    {"4", 4, PINK_X_MIN, PINK_X_MAX, PINK_Y_MIN, PINK_Y_MAX},
    {"5", 5, ORANGE_X_MIN, ORANGE_X_MAX, ORANGE_Y_MIN, ORANGE_Y_MAX},
    // {"6", 6, GREEN_X_MIN, GREEN_X_MAX, GREEN_Y_MIN, GREEN_Y_MAX},
    // {"7", 7, BLUE_X_MIN, BLUE_X_MAX, BLUE_Y_MIN, BLUE_Y_MAX},
    // {"8", 8, PINK_X_MIN, PINK_X_MAX, PINK_Y_MIN, PINK_Y_MAX},
    // {"9", 9, ORANGE_X_MIN, ORANGE_X_MAX, ORANGE_Y_MIN, ORANGE_Y_MAX}
};

struct Route
{
    uint8_t count = 0;
    Point midpoints[3];
};

const uint8_t num_sectors = 6;

const Route route[num_sectors][num_sectors] = 
{
    {   // sector 0 to other sectors
        {0, {}},
        {0, {}},
        {2, {{P0_X, P0_Y}, {P1_X, P1_Y}}},
        {2, {{P9_X, P9_Y}, {P8_X, P8_Y}}},
        {3, {{P0_X, P0_Y}, {P1_X, P1_Y}, {P2_X, P2_Y}}},
        {3, {{P0_X, P0_Y}, {P1_X, P1_Y}, {P3_X, P3_Y}}}
    }, 
    {   // sector 1
        {0, {}},
        {0, {}},
        {3, {{P9_X, P9_Y}, {P8_X, P8_Y}, {P2_X, P2_Y}}},
        {2, {{P9_X, P9_Y}, {P8_X, P8_Y}}},
        {3, {{P9_X, P9_Y}, {P8_X, P8_Y}, {P6_X, P6_Y}}},
        {3, {{P9_X, P9_Y}, {P8_X, P8_Y}, {P7_X, P7_Y}}}
    }, 
    {   // sector 2
        {2, {{P1_X, P1_Y}, {P0_X, P0_Y}}},
        {2, {{P1_X, P1_Y}, {P0_X, P0_Y}}},
        {0, {}},
        {2, {{P2_X, P2_Y}, {P6_X, P6_Y}}},
        {1, {{P2_X, P2_Y}}},
        {1, {{P3_X, P3_Y}}}
    }, 
    {   // sector 3
        {2, {{P8_X, P8_Y}, {P9_X, P9_Y}}},
        {2, {{P8_X, P8_Y}, {P9_X, P9_Y}}},
        {2, {{P6_X, P6_Y}, {P2_X, P2_Y}}},
        {0, {}},
        {1, {{P6_X, P6_Y}}},
        {1, {{P7_X, P7_Y}}}
    }, 
    {   // sector 4
        {2, {{P1_X, P1_Y}, {P0_X, P0_Y}}},
        {2, {{P8_X, P8_Y}, {P9_X, P9_Y}}},
        {1, {{P2_X, P2_Y}}},
        {1, {{P6_X, P6_Y}}},
        {0, {}},
        {2, {{P6_X, P6_Y}, {P7_X, P7_Y}}}
    }, 
    {   // sector 5
        {3, {{P3_X, P3_Y}, {P1_X, P1_Y}, {P0_X, P0_Y}}},
        {3, {{P7_X, P7_Y}, {P8_X, P8_Y}, {P9_X, P9_Y}}},
        {1, {{P3_X, P3_Y}}},
        {1, {{P7_X, P7_Y}}},
        {2, {{P3_X, P3_Y}, {P2_X, P2_Y}}},
        {0, {}}
    }
};

int get_sector(uint16_t x, uint16_t y)
{
    for (int i = 0; i < num_sectors; ++ i)
    {
        const Sector *s = &sectors[i];
        if ((x >= s->x_min && x < s->x_max) && (y >= s->y_min && y < s->y_max))
        {
            return s->id;
        }
    }
    return -1;
}


void set_target(uint16_t target_x, uint16_t target_y)
{
    int target_location_sector = get_sector(target_x, target_y);
    int current_location_sector = get_sector(vive_1_x, vive_1_y);
    Serial.printf("target sector = %d, current sector = %d. \n", target_location_sector, current_location_sector);

    const Route *tmp_route = &(route[current_location_sector][target_location_sector]);
    for (int i = 0; i < tmp_route->count; ++ i)
    {
        point_queue[tmp_route->count - i] = tmp_route->midpoints[i];
    }
    point_queue[0] = Point{target_x, target_y};
    point_queue_num = tmp_route->count + 1;

    for (int i = 0; i < point_queue_num; ++ i)
    {

        Serial.printf("Point %d is x=%u, y=%u\n", i, point_queue[i].x, point_queue[i].y);
    }
}


void handle_wall_follow()
{
    ++ wifi_comms_num;
    // toggle wall-following state
    wall_follow_flag = !wall_follow_flag;

    // if don't need wall-following, all stop
    if (!wall_follow_flag)
    {
        counter = 0;
        state = 0;
        front = list[(counter) % 4];
        right = list[(counter + 1) % 4];
        noninterrupt_b_flag = 0;
        noninterrupt_f_flag = 0;
        noninterrupt_l_flag = 0;
        noninterrupt_r_flag = 0;
        motor_1.set_target_speed(0);
        motor_2.set_target_speed(0);
        motor_3.set_target_speed(0);
        motor_4.set_target_speed(0);
    }
}

void handle_servo()
{
    ++ wifi_comms_num;
    servo_mode = (servo_mode + 1) % 3;
    if (servo_mode == 1)
    {
        ledcAttach(SERVO_MOTOR_PIN, 50, 12);
        ledcWrite(SERVO_MOTOR_PIN, 410);
    }
    else if (servo_mode == 2)
    {
        ledcAttach(SERVO_MOTOR_PIN, 50, 12);
        ledcWrite(SERVO_MOTOR_PIN, 205);
    }
    else
    {
        ledcDetach(SERVO_MOTOR_PIN);
    }
}

void handleParams() {
    String out;
    if (pos_pid_active)
    {
        out = String(vive_1_x) + ", " + String(vive_1_y) + ", " + String(vive_2_x) + ", " + String(vive_2_y) + ", " + String(vive_x) + ", " + String(vive_y) + ", " + String(point_queue[point_queue_num].x) + ", " + String(point_queue[point_queue_num].y);
    }
    else
    {
        out = String(vive_1_x) + "," + String(vive_1_y) + "," + String(vive_2_x) + "," + String(vive_2_y) + ", " + String(vive_x) + ", " + String(vive_y);
    }
    server.sendplain(out);
}



// Call this at 50 Hz from loop().
// Uses vive_1_x / vive_1_y as current position and
// pos_pid_target_x / pos_pid_target_y as target.
// Outputs velocity commands through set_speed().
void pos_pid_update()
{
    // If PID is not active, do nothing
    if (!pos_pid_active) {
        return;
    }

    // Use Vive 1 as current position
    float x_now = static_cast<float>(vive_x);
    float y_now = static_cast<float>(vive_y);

    // If Vive is not receiving, don't drive blindly
    if (vive_x == 0)
    {
        set_speed(0, 0, 0);
        return;
    }

    // Serial.printf("x_now = %.2f, y_now = %.2f\n", x_now, y_now);

    // Position error in global frame
    float err_x = pos_pid_target_x - x_now;
    float err_y = pos_pid_target_y - y_now;
    // Serial.printf("pos_pid_target_x = %.2f, pos_pid_target_y = %.2f\n", pos_pid_target_x, pos_pid_target_y);
    // Serial.printf("err_x = %.2f, err_y = %.2f\n", err_x, err_y);

    // Distance to target
    float dist_sq = err_x * err_x + err_y * err_y;
    if (dist_sq <= (pos_pid_deadband * pos_pid_deadband)) {
        // Close enough: stop and (optionally) disable PID
        set_speed(0, 0, 0);
        pos_pid_active = false;  // uncomment if you want it to auto-stop controlling
        return;
    }

    // === PID per axis in global frame ===

    // Integrals
    pos_pid_err_x_int += err_x * pos_pid_dt;
    pos_pid_err_y_int += err_y * pos_pid_dt;

    // Derivatives
    float derr_x = (err_x - pos_pid_prev_err_x) / pos_pid_dt;
    float derr_y = (err_y - pos_pid_prev_err_y) / pos_pid_dt;

    pos_pid_prev_err_x = err_x;
    pos_pid_prev_err_y = err_y;

    // PID output in global coordinates (desired global velocity)
    float vx_global = pos_pid_kp * err_x + pos_pid_ki * pos_pid_err_x_int + pos_pid_kd * derr_x;

    float vy_global = pos_pid_kp * err_y + pos_pid_ki * pos_pid_err_y_int + pos_pid_kd * derr_y;

    // Limit magnitude so we don't command insane speeds
    float mag = sqrtf(vx_global * vx_global + vy_global * vy_global);
    if (mag > pos_pid_max_cmd && mag > 1e-3f) {
        float scale = pos_pid_max_cmd / mag;
        vx_global *= scale;
        vy_global *= scale;
    }

    // Convert global velocity to robot-local velocity
    int vx_local = 0;
    int vy_local = 0;
    rotateGlobalToLocal(static_cast<int>(vx_global),
                        static_cast<int>(vy_global),
                        pos_pid_robot_heading_deg,
                        vx_local,
                        vy_local);

    // No rotation control here (w = 0). You can later add a heading PID.
    set_speed(vx_local, vy_local, 0);
}

void handleCapLowTower_red()
{
    ++ wifi_comms_num;
    cap_low_tower_flag = !cap_low_tower_flag;
    if (!cap_low_tower_flag)
    {
        estop_flag = 1;
    }
}

void handleAttackNexus_red()
{
    ++ wifi_comms_num;
    // handle_servo();
    attack_nexus_flag = !attack_nexus_flag;
    if (!attack_nexus_flag)
    {
        estop_flag = 1;
    }
}

void handleCapHighTower_red()
{
    ++ wifi_comms_num;
    cap_high_tower_flag = !cap_high_tower_flag;
    if (!cap_high_tower_flag)
    {
        estop_flag = 1;
    }
}


void handleCapLowTower_blue()
{
    ++ wifi_comms_num;
    cap_low_tower_flag_blue = !cap_low_tower_flag_blue;
    if (!cap_low_tower_flag_blue)
    {
        estop_flag = 1;
    }
}

void handleAttackNexus_blue()
{
    ++ wifi_comms_num;
    // handle_servo();
    attack_nexus_flag_blue = !attack_nexus_flag_blue;
    if (!attack_nexus_flag_blue)
    {
        estop_flag = 1;
    }
}

void handleCapHighTower_blue()
{
    ++ wifi_comms_num;
    cap_high_tower_flag_blue = !cap_high_tower_flag_blue;
    if (!cap_high_tower_flag_blue)
    {
        estop_flag = 1;
    }
}

void handleAttackMode_red()
{
    attack_flag_red = !attack_flag_red;
    if (!attack_flag_red)
    {
        estop_flag = 1;
        attack_flag_red = 0;
        set_target_flag = 0;
    }
}

void handleAttackMode_blue()
{
    attack_flag_blue = !attack_flag_blue;
    if (!attack_flag_blue)
    {
        estop_flag = 1;
        attack_flag_blue = 0;
        set_target_flag = 0;
    }
}

void handleAutoNexus_R()
{
    tower_noninterrupt_f_flag = 20;
    attack_nexus_flag = 1;
    // handle_servo();
}


void handleAutoHigh_R()
{
    tower_noninterrupt_f_flag = 20;
    cap_high_tower_flag = 1;
    // handle_servo();
}



void handleAutoLow_R()
{
    tower_noninterrupt_f_flag = 20;
    cap_low_tower_flag = 1;
    // handle_servo();
}


void handleTowerNexus()
{
    ++ wifi_comms_num;
    tower_noninterrupt_f_flag = 20;
    // attack_tower_nexus_flag = !attack_tower_nexus_flag;
    // if (!attack_tower_nexus_flag)
    // {
    //     estop_flag = 1;
    // }
    cap_low_tower_flag = 1;
    attack_nexus_flag = 1;
    cap_high_tower_flag = 1;
}


void set_heading(int angle)
{
    
}

void setup()
{
    // setup wifi
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(MyIP, IPAddress(192,168,1,1), IPAddress(255, 255, 255, 0));

    // debug information
    #ifdef DEBUG
        Serial.begin(115200);
        Serial.print("Access point "); Serial.println(ssid);
        Serial.print("AP IP address"); 
        Serial.println(MyIP);
        Serial.print("Use this URL to connect: http://");
        Serial.print(WiFi.localIP()); Serial.println("/");
    #endif

    // setup server
    server.begin();
    server.attachHandler("/", handleRoot);
    server.attachHandler("/joy?data=", handleSpeed);
    server.attachHandler("/walltoggle", handle_wall_follow);
    server.attachHandler("/servotoggle", handle_servo);
    server.attachHandler("/params", handleParams);
    server.attachHandler("/setvals?data=", handleSetVals);
    server.attachHandler("/estop", handleEStop);
    server.attachHandler("/toggle1", handleCapLowTower_red);
    server.attachHandler("/toggle2", handleCapHighTower_red);
    server.attachHandler("/toggle3", handleAttackNexus_red);
    server.attachHandler("/toggle4", handleCapLowTower_blue);
    server.attachHandler("/toggle5", handleCapHighTower_blue);
    server.attachHandler("/toggle6", handleAttackNexus_blue);
    server.attachHandler("/toggle7", handleTowerNexus);
    server.attachHandler("/toggle8", handleAttackMode_red);
    server.attachHandler("/toggle9", handleAttackMode_blue);
    server.attachHandler("/toggle10", handleAutoNexus_R);
    server.attachHandler("/toggle11", handleAutoHigh_R);
    server.attachHandler("/toggle12", handleAutoLow_R);
    // server.attachHandler("/hit ",handleDir);
    // server.attachHandler("/measuredSpeed", `MeasuredSpeed);


    // setup timer interrupt at 50ms (20hz)
    timer = timerBegin(10000);
    timerAttachInterrupt(timer, &onTimer);
    timerStart(timer);
    timerAlarm(timer, 500, true, 0);

    
    tophat_timer = timerBegin(10000);
    timerAttachInterrupt(tophat_timer, &on_tophat_timer);
    timerStart(tophat_timer);
    timerAlarm(tophat_timer, 5000, true, 0);

    motor_1.begin();
    motor_2.begin();
    motor_3.begin();
    motor_4.begin();

    tof_init();

    vive_1.begin();
    vive_2.begin();

    pinMode(SERVO_MOTOR_PIN, OUTPUT);
    // ledcAttach(38, 50, 12);
    // ledcWrite(38, 409);

    #ifdef DEBUG
    Serial.println("Finished setup");
    #endif
}

void get_vive_data()
{
    if (vive_1.status() == VIVE_RECEIVING)
    {
        vive_1_x = vive_1.xCoord();
        vive_1_y = vive_1.yCoord();
        if (vive_1_y > 5500)
        {
            vive_priority = 1;
        }
        // Serial.printf("vive1 X %d, Y %d\n",vive_1.xCoord(), vive_1.yCoord());
    }
    else
    {
        vive_1_x = 0;
        vive_1_y = 0;
        vive_1.sync(1);
    }

        
    if (vive_2.status() == VIVE_RECEIVING)
    {
        vive_2_x = vive_2.xCoord();
        vive_2_y = vive_2.yCoord();
        if (vive_2_y < 5000)
        {
            vive_priority = 0;
        }
        // Serial.printf("vive2 X %d, Y %d\n",vive_2.xCoord(), vive_2.yCoord());
    }
    else
    {
        vive_2_x = 0;
        vive_2_y = 0;
        vive_2.sync(1);
    }

    if (vive_priority)
    {
        vive_x = vive_2_x - 200;
        vive_y = vive_2_y + 150;
    }
    else
    {
        vive_x = vive_1_x;
        vive_y = vive_1_y - 150;
    }

    // if (vive_1_x != 0)
    // {
    //     vive_x = vive_1_x;
    //     vive_y = vive_1_y;
    // }
    // else if (vive_2_x != 0)
    // {
    //     vive_x = vive_2_x + 100;
    //     vive_y = vive_2_y + 0;
    // }
    // else
    // {
    //     vive_x = 0;
    //     vive_y = 0;
    // }
}



void loop()
{
    // while(1)
    // {
    //     I2C_switch_channel(4);
    //     Wire.beginTransmission(0x28);
    //     Wire.write(1);
    //     Wire.endTransmission(false);
    //     Wire.requestFrom(0x28, 1);
    //     int answer;
    //     if (Wire.available())
    //         answer = Wire.read();
    //     Serial.println(answer);
    //     delay(500);
    // }
    // motor_1.test_encoder();
    // motor_2.test_encoder();
    // motor_3.test_encoder();
    // motor_4.test_encoder();
    // while(1);
    server.serve();

    // Serial.printf("tophat counter = %d. \n", tophat_comms_counter);
    if (tophat_flag)
    {
        tophat_flag = 0;
        // Serial.printf("TOPHAT send %d. \n", wifi_comms_num);
        I2C_switch_channel(4);
        Wire.beginTransmission(0x28);
        Wire.write(wifi_comms_num);
        Wire.endTransmission(false);
        Wire.requestFrom(0x28, 1);
        int answer;
        if (Wire.available())
            answer = Wire.read();
        // Serial.println(answer);
        if (answer)
        {
            estop_flag = 0;
        }
        else 
        {
            motor_1.set_target_speed(0);
            motor_2.set_target_speed(0);
            motor_3.set_target_speed(0);
            motor_4.set_target_speed(0);
            estop_flag = 1;
        }
        wifi_comms_num = 0;
    }

    
    if (estop_flag)
    {
        wall_follow_flag = 0;
        point_queue_num = 0;
        motor_1.set_target_speed(0);
        motor_2.set_target_speed(0);
        motor_3.set_target_speed(0);
        motor_4.set_target_speed(0);
        servo_mode = 0;
        ledcDetach(SERVO_MOTOR_PIN);
        pos_pid_active = 0;
        cap_high_tower_flag = 0;
        cap_low_tower_flag = 0;
        attack_nexus_flag = 0;
        attack_tower_nexus_flag = 0;
        tower_noninterrupt_b_flag = 0;
        tower_noninterrupt_f_flag = 0;
        tower_noninterrupt_l_flag = 0;
        tower_noninterrupt_r_flag = 0;
        noninterrupt_b_flag = 0;
        noninterrupt_f_flag = 0;
        noninterrupt_flag = 0;
        noninterrupt_l_flag = 0;
        noninterrupt_r_flag = 0;
        cap_low_tower_flag_blue = 0;
        cap_high_tower_flag_blue = 0;
        attack_nexus_flag_blue = 0;
        attack_flag_blue = 0;
        attack_flag_red = 0;
        auto_flag_red = 0;
        nexus_attack_counter = 0;
        estop_flag = 0;
    }

    if (motor_update_flag)
    {
        motor_update_flag = 0;
        motor_1.update_measured_speed();
        motor_2.update_measured_speed();
        motor_3.update_measured_speed();
        motor_4.update_measured_speed();
    }

    if (speed_update_flag)
    {
        speed_update_flag = 0;
        if (wall_follow_flag)
            follow_edge_state();
        // Serial.println("update motor");
        // motor_1.update_motor_speed();
        // motor_2.update_motor_speed();
        // motor_3.update_motor_speed();
        // motor_4.update_motor_speed();
        motor_1.update_motor();
        motor_2.update_motor();
        motor_3.update_motor();
        motor_4.update_motor();
    }
    // delay(10);
    if (tof_update_flag)
    {
        tof_update_flag = 0;
        I2C_switch_channel(0);
        if (tof_1.isRangeComplete())    tof_1_value = tof_1.readRangeResult();
        I2C_switch_channel(1);
        if (tof_2.isRangeComplete())    tof_2_value = tof_2.readRangeResult();
        I2C_switch_channel(2);
        if (tof_3.isRangeComplete())    tof_3_value = tof_3.readRangeResult();
        I2C_switch_channel(3);
        if (tof_4.isRangeComplete())    tof_4_value = tof_4.readRangeResult();
        // Serial.printf("%u, %u, %u, %u\n", tof_1_value, tof_2_value, tof_3_value, tof_4_value);
    }

    if (vive_update_flag)
    {
        vive_update_flag = 0;
        get_vive_data();
        pos_pid_update();
    }
    
	// if (vive_1.temp_counter == 100)
	// {
	// 	for (int i = 0; i < 100; ++i)
	// 		Serial.printf("pulse = %d, type = %d\n", vive_1.pulsewidth_list[i], vive_1.type_list[i]);
	// 	vive_1.temp_counter = 0;
    // }
    if (!pos_pid_active)
    {
        if (point_queue_num)
        {
            pos_pid_set_target(point_queue[point_queue_num - 1].x, point_queue[point_queue_num - 1].y);
            -- point_queue_num;
        }
    }

    if (auto_attack_update_flag)
    {
        auto_tower_capture();
        auto_attack_update_flag = 0;
    }


}
