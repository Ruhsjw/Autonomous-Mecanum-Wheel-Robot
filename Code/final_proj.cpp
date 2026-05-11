#include <WiFi.h>
#include <WiFiUdp.h>
#include "html510.h"
#include "html.h"

#define PWM1_PIN 10
#define DIR1_PIN 8
#define PWM2_PIN 7
#define DIR2_PIN 6
#define ENCODER1_PIN 5
#define ENCODER2_PIN 4
#define ENCODER3_PIN 1
#define ENCODER4_PIN 9
#define PWM_RES 8
#define PWM_CNT_MAX 255
#define PWM_FREQ 4096
#define ENCODER_MAX_READ 42

#define KP 0.4
#define KI 0.1
#define KD 0.2

#define DEBUG 0

const char* ssid = "myEsp32";
const char* password = "esp32pass";

IPAddress MyIP(192,168,1,136);
HTML510Server server(80);
uint8_t dir = 1;

int left_target_speed = 0;
int left_target_speed_prev = 0;
int left_output_speed = 0;
int left_measured_count_prev = 0;
int left_measured_count = 0;
int left_measured_speed_prev = 0;
int left_measured_speed = 0;
int left_acceleration = 0;
int left_summed_error = 0;

int right_target_speed = 0;
int right_target_speed_prev = 0;
int right_output_speed = 0;
int right_measured_count_prev = 0;
int right_measured_count = 0;
int right_measured_speed_prev = 0;
int right_measured_speed = 0;
int right_acceleration = 0;
int right_summed_error = 0;

uint8_t speed_update_flag = 0;
hw_timer_t *timer = NULL;



// handler for pwm duty cycle
void handleSpeed()
{
    String value = server.getText();
    if (DEBUG)
    // server.sendplain("200");
    // Serial.print("Speed: ");
    // Serial.print("Here: ");
    // Serial.println(value);
    int comma = value.indexOf(',');
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
    // if (dir)
    // {
    //     ledcWrite(PWM1_PIN, target_speed);
    // }
    // else
    // {
    //     ledcWrite(PWM1_PIN, PWM_CNT_MAX - target_speed);
    // }
}

// handler for pwm frequency
void handleDir()
{
    // dir = !dir;
    // Serial.print("Dir = ");
    // Serial.println(dir);
    // if (dir)
    // {
    //     digitalWrite(DIR1_PIN, LOW);
    //     ledcWrite(PWM1_PIN, speed);
    // }
    // else
    // {
    //     digitalWrite(DIR1_PIN, HIGH);
    //     ledcWrite(PWM1_PIN, PWM_CNT_MAX - speed);
    // }
    return;
}

// handler for sending the webpage
void handleRoot()
{
    server.sendhtml(body);
    Serial.println("body sent");
}

// handler for sending the count
void handleMeasuredSpeed()
{
    String s = "Left speed is " + String(left_measured_speed) + ", target speed is " + String(left_target_speed) + ", output speed is" + String(left_output_speed) + ", right speed is " + String(right_measured_speed) + ", target speed is " + String(right_target_speed);
    server.sendplain(s);
}

// ISR for pin change interrupt
void IRAM_ATTR left_risingISR()
{
    if (digitalRead(ENCODER2_PIN) == LOW)   ++ left_measured_count;
    else                                    -- left_measured_count;
}

// ISR for pin change interrupt
void IRAM_ATTR right_risingISR()
{
    if (digitalRead(ENCODER4_PIN) == LOW)   ++ right_measured_count;
    else                                    -- right_measured_count;
}

void IRAM_ATTR onTimer()
{
    left_measured_speed = left_measured_count - left_measured_count_prev;
    left_measured_speed = map(left_measured_speed, 0, ENCODER_MAX_READ, 0, PWM_CNT_MAX);
    left_measured_count_prev = left_measured_count;
    right_measured_speed = right_measured_count - right_measured_count_prev;
    right_measured_speed = map(right_measured_speed, 0, ENCODER_MAX_READ, 0, PWM_CNT_MAX);
    right_measured_count_prev = right_measured_count;
    speed_update_flag = 1;
}

void update_motor()
{
    left_acceleration = left_measured_speed - left_measured_speed_prev;
    left_measured_speed_prev = left_measured_speed; 
    left_summed_error += (left_target_speed - left_measured_speed);
    // if ((left_target_speed >= 0 && left_target_speed_prev < 0) || (left_target_speed < 0 && left_target_speed_prev >= 0))
    // {
    //     left_summed_error = 0;
    //     left_target_speed_prev = left_target_speed;
    // }
    left_output_speed = KP * (left_target_speed - left_measured_speed) + KD * left_acceleration + KI * left_summed_error;
    left_output_speed = left_output_speed < 256 ? left_output_speed : 255;
    left_output_speed = left_output_speed > - 256 ? left_output_speed : - 255;

    right_acceleration = right_measured_speed - right_measured_speed_prev;
    right_measured_speed_prev = right_measured_speed; 
    right_summed_error += (right_target_speed - right_measured_speed);
    // if ((right_target_speed >= 0 && right_target_speed_prev < 0) || (right_target_speed < 0 && right_target_speed_prev >= 0))
    // {
    //     right_summed_error = 0;
    //     right_target_speed_prev = right_target_speed;
    // }
    right_output_speed = (float)KP * (float)(right_target_speed - right_measured_speed) + (float)KD * right_acceleration + (float)KI * right_summed_error;
    right_output_speed = right_output_speed < 256 ? right_output_speed : 255;
    right_output_speed = right_output_speed > - 256 ? right_output_speed : - 255;

    // Serial.print(left_output_speed);
    // Serial.print(", ");
    // Serial.println(right_output_speed);

    if (left_output_speed >= 0)
    {
        digitalWrite(DIR1_PIN, HIGH);
        ledcWrite(PWM1_PIN, PWM_CNT_MAX - left_output_speed);
    }
    else
    {
        digitalWrite(DIR1_PIN, LOW);
        ledcWrite(PWM1_PIN, - left_output_speed);
    }

    if (right_output_speed >= 0)
    {
        digitalWrite(DIR2_PIN, HIGH);
        ledcWrite(PWM2_PIN, PWM_CNT_MAX - right_output_speed);
    }
    else
    {
        digitalWrite(DIR2_PIN, LOW);
        ledcWrite(PWM2_PIN, - right_output_speed);
    }
}

void setup()
{
    // setup wifi
    Serial.begin(115200);
    delay(5000); 
    Serial.print("Access point "); Serial.println(ssid);
    WiFi.softAP(ssid, password);
    WiFi.softAPConfig(MyIP, IPAddress(192,168,1,1), IPAddress(255, 255, 255, 0));
    Serial.print("AP IP address"); 
    Serial.println(MyIP);
    Serial.print("Use this URL to connect: http://");
    Serial.print(WiFi.localIP()); Serial.println("/");

    // setup server
    server.begin();
    server.attachHandler("/", handleRoot);
    server.attachHandler("/joy?xy=", handleSpeed);
    server.attachHandler("/hit ",handleDir);
    server.attachHandler("/measuredSpeed", handleMeasuredSpeed);

    // initialize pwm output pin
    pinMode(PWM1_PIN, OUTPUT);
    pinMode(DIR1_PIN, OUTPUT);
    ledcAttach(PWM1_PIN, PWM_FREQ, PWM_RES);
    pinMode(PWM2_PIN, OUTPUT);
    pinMode(DIR2_PIN, OUTPUT);
    ledcAttach(PWM2_PIN, PWM_FREQ, PWM_RES);

    // setup encoder pin and set up ISR for the pin
    pinMode(ENCODER1_PIN, INPUT_PULLUP);
    pinMode(ENCODER2_PIN, INPUT_PULLUP);
    pinMode(ENCODER3_PIN, INPUT_PULLUP);
    pinMode(ENCODER4_PIN, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(ENCODER1_PIN), left_risingISR, RISING);
    // attachInterrupt(digitalPinToInterrupt(ENCODER2_PIN), left_risingISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENCODER3_PIN), right_risingISR, RISING);
    // attachInterrupt(digitalPinToInterrupt(ENCODER4_PIN), right_risingISR, CHANGE);

    // setup timer interrupt
    timer = timerBegin(10000);
    timerAttachInterrupt(timer, &onTimer);
    // timerStart(timer);
    timerAlarm(timer, 500, true, 0);
    Serial.println("Finished setup");
}

void loop()
{
    server.serve();
    if (speed_update_flag)
    {
        speed_update_flag = 0;
        update_motor();
    }
    // delay(10);
}
