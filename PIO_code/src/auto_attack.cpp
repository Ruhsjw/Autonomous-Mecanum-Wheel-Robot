#include "auto_attack.h"


static const int HIT_V = 70;          
static const int HIT_W = 0;          
static const uint16_t MOVE_TICKS = 10; 
static const uint16_t PAUSE_TICKS = 6; 


static const bool USE_CYCLE_MODE = true;
static const uint8_t CYCLES = 4;

static const uint8_t FWD_COUNT = 4;
static const uint8_t BWD_COUNT = 4;



enum class AttackState : uint8_t {
  IDLE = 0,
  NAVIGATING,
  WAIT_NAV_DONE,
  HIT_FWD,
  HIT_FWD_PAUSE,
  HIT_BWD,
  HIT_BWD_PAUSE,
  DONE
};

static AttackState st = AttackState::IDLE;
static uint16_t tick_counter = 0;


static uint8_t cycle_done = 0;
static uint8_t fwd_done = 0;
static uint8_t bwd_done = 0;

static bool running = false;


static inline void enter_state(AttackState ns) {
  st = ns;
  tick_counter = 0;
}

bool auto_attack_is_running() {
  return running;
}

void auto_attack_cancel() {
  running = false;
  enter_state(AttackState::IDLE);

  set_speed(0, 0, 0);
}

void auto_attack_start(uint16_t target_x, uint16_t target_y) {
  
  cycle_done = 0;
  fwd_done = 0;
  bwd_done = 0;

  
  set_target(target_x, target_y);

  running = true;
  enter_state(AttackState::NAVIGATING);
}


void auto_attack_update() {
  if (!running) return;

  
  tick_counter++;

  switch (st) {

    case AttackState::IDLE:
      running = false;
      set_speed(0, 0, 0);
      break;

    case AttackState::NAVIGATING:
      
      enter_state(AttackState::WAIT_NAV_DONE);
      break;

    case AttackState::WAIT_NAV_DONE:
      
      if (!pos_pid_active && point_queue_num == 0) {
       
        if (USE_CYCLE_MODE) {
          cycle_done = 0;
        } else {
          fwd_done = 0;
          bwd_done = 0;
        }
        enter_state(AttackState::HIT_FWD);
      }
      break;

    case AttackState::HIT_FWD:
      
      set_speed(+HIT_V, 0, HIT_W);

      if (tick_counter >= MOVE_TICKS) {
        set_speed(0, 0, 0);
        enter_state(AttackState::HIT_FWD_PAUSE);
      }
      break;

    case AttackState::HIT_FWD_PAUSE:
    
      set_speed(0, 0, 0);

      if (tick_counter >= PAUSE_TICKS) {
        enter_state(AttackState::HIT_BWD);
      }
      break;

    case AttackState::HIT_BWD:
      
      set_speed(-HIT_V, 0, HIT_W);

      if (tick_counter >= MOVE_TICKS) {
        set_speed(0, 0, 0);
        enter_state(AttackState::HIT_BWD_PAUSE);
      }
      break;

    case AttackState::HIT_BWD_PAUSE:
      set_speed(0, 0, 0);

      if (tick_counter >= PAUSE_TICKS) {
      
        if (USE_CYCLE_MODE) {
          cycle_done++;
          if (cycle_done >= CYCLES) {
            enter_state(AttackState::DONE);
          } else {
            enter_state(AttackState::HIT_FWD);
          }
        } else {
          
          fwd_done++;
          bwd_done++;

          if (fwd_done >= FWD_COUNT && bwd_done >= BWD_COUNT) {
            enter_state(AttackState::DONE);
          } else {
            enter_state(AttackState::HIT_FWD);
          }
        }
      }
      break;

    case AttackState::DONE:
      set_speed(0, 0, 0);
      running = false;
      enter_state(AttackState::IDLE);
      break;
  }
}
