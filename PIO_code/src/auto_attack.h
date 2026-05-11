
#include <Arduino.h>


extern bool pos_pid_active;
extern uint8_t point_queue_num;

void set_target(uint16_t target_x, uint16_t target_y);  
void set_speed(int x, int y, int angle);                


void auto_attack_start(uint16_t target_x, uint16_t target_y);
void auto_attack_cancel();
void auto_attack_update();  
bool auto_attack_is_running();
