/*
 * MEAM510 hacks for Vive Interface V2
 * Dec 2021
 * Use at your own risk
 * 
 * Mark Yim
 * University of Pennsylvania
 * copyright (c) 2021 All Rights Reserved
 */

#include "vive.h"

// #define DEBUG
// #define DEBUG2

Vive* Vive::s_instance = nullptr;

// ================= MCPWM capture integration =================

// MCPWM capture ISR trampoline
bool IRAM_ATTR Vive::captureTrampoline(mcpwm_cap_channel_handle_t cap_chan,
                                       const mcpwm_capture_event_data_t *edata,
                                       void *user_ctx)
{
  (void)cap_chan;
  auto *self = static_cast<Vive*>(user_ctx);
  if (self) {
    self->onCapture(edata);
  }
  // allow context switch if higher-priority task becomes ready
  return true;
}

// Actual MCPWM capture ISR handler
void IRAM_ATTR Vive::onCapture(const mcpwm_capture_event_data_t *edata)
{
  // Edge type: POS = rising, NEG = falling
  bool isRise = (edata->cap_edge == MCPWM_CAP_EDGE_POS);
  bool isFall = (edata->cap_edge == MCPWM_CAP_EDGE_NEG);

  // Convert hardware ticks to microseconds
  uint32_t ticks = edata->cap_value;
  uint32_t us = (uint32_t)((uint64_t)ticks * 1000000ULL / m_resolutionHz);

  if (isRise) {
    m_usRising = us;
  }

  if (isFall) {
    m_usFalling = us;

    // Original logic: processPulse() runs when a full pulse (rise+fall) arrived
    if (m_vivestatus == VIVE_RECEIVING) {
      processPulse();
    }
  }
}

// Initialize MCPWM capture hardware on m_pin
esp_err_t Vive::initCapture()
{
  esp_err_t err;

  // 1) Capture timer
  mcpwm_capture_timer_config_t tcfg = {};
  tcfg.group_id      = m_groupID;                        // MCPWM group 0
  tcfg.clk_src       = MCPWM_CAPTURE_CLK_SRC_DEFAULT;
  tcfg.resolution_hz = m_resolutionHz;                   // 80 MHz

  err = mcpwm_new_capture_timer(&tcfg, &m_capTimer);
  if (err != ESP_OK) return err;

  err = mcpwm_capture_timer_enable(m_capTimer);
  if (err != ESP_OK) return err;

  err = mcpwm_capture_timer_start(m_capTimer);
  if (err != ESP_OK) return err;

  // 2) Capture channel on our GPIO
  mcpwm_capture_channel_config_t ccfg = {};
  ccfg.gpio_num = (gpio_num_t)m_pin;
  ccfg.prescale = 1;               // capture every edge

  ccfg.flags.pos_edge          = 1;  // rising edges
  ccfg.flags.neg_edge          = 1;  // falling edges
  ccfg.flags.pull_up           = 1;  // good for active-low Vive signal
  ccfg.flags.pull_down         = 0;
  ccfg.flags.invert_cap_signal = 0;
  ccfg.flags.io_loop_back      = 0;

  err = mcpwm_new_capture_channel(m_capTimer, &ccfg, &m_capChannel);
  if (err != ESP_OK) return err;

  // 3) Register callback
  mcpwm_capture_event_callbacks_t cbs = {};
  cbs.on_cap = &Vive::captureTrampoline;

  err = mcpwm_capture_channel_register_event_callbacks(m_capChannel, &cbs, this);
  if (err != ESP_OK) return err;

  // 4) Enable capture
  err = mcpwm_capture_channel_enable(m_capChannel);
  if (err != ESP_OK) return err;

  // Also set Arduino pin mode
  pinMode(m_pin, INPUT_PULLUP);

  return ESP_OK;
}

// ============================================================

Vive::Vive(int pin, int group_ID)
{
  m_pin = pin;
  m_groupID = group_ID;
}


uint16_t Vive::xCoord()
{
  uint16_t average_x = 0;
  for (int i = 0; i < average_list_length; ++i)
  {
    average_x += average_list_x[i];
  }
  average_x /= average_list_length;
  return average_x;
}

uint16_t Vive::yCoord()
{
  uint16_t average_y = 0;
  for (int i = 0; i < average_list_length; ++i)
  {
    average_y += average_list_y[i];
  }
  average_y /= average_list_length;
  return average_y;
}

int Vive::isKPulse(uint32_t pulsewidth)
{
  if (pulsewidth < 74 || 
     (pulsewidth > 83 && pulsewidth < 95) || 
     (pulsewidth > 106 && pulsewidth < 117) ||
     (pulsewidth > 127 && pulsewidth < 137) 
  ) return false;
  else return true;
}

int Vive::isJPulse(uint32_t pulsewidth)
{
  if (pulsewidth < 74 || 
     (pulsewidth > 83 && pulsewidth < 93) || 
     (pulsewidth > 106 && pulsewidth < 117) ||
     (pulsewidth > 127 && pulsewidth < 137) 
  ) return true;
  else return false;
}

// move checkflag to be backwards checking...think about whether tu link w/spuriuos
void Vive::processPulse() {
  // static int checkflag=0;

  if (m_lastFalling != m_usFalling)
  {
    int pulsewidth = m_usFalling - m_usRising;

    if (pulsewidth > m_sweepWidth)
    {
      if (pulsewidth > 140)   // pulse too long. bad pulse
      {
        #ifdef DEBUG2
        Serial.printf("P%d Spur %d width:%d \n", m_pin, m_spurious, pulsewidth);
        #endif
        m_pulseType = 0;
      }
      else if (isKPulse(pulsewidth))
      {
        m_pulseType = KTYPE;
        #ifdef DEBUG
        Serial.printf("KPin%d width=%d, ", m_pin, pulsewidth);
        #endif
      } 
      else if (isJPulse(pulsewidth))
      {
        m_pulseType = JTYPE;
        #ifdef DEBUG
        Serial.printf("JPin%d width=%d, ", m_pin, pulsewidth);
        #endif
      }
    }
    else    // x sweep or y sweep
    {
      #ifdef DEBUG
      Serial.printf("Pin%d:%d  r=%d\n", m_pin, pulsewidth, m_usRising - m_lastFalling);
      #endif

      if (m_pulseType == JTYPE)
      {
        // distance in Y direction: interval between last wide pulse's falling
        // and this sweep pulse's rising
        m_yCoord = m_usRising - m_lastFalling;
        average_list_y[average_pointer_y] = m_yCoord;
        average_pointer_y = (average_pointer_y + 1) % average_list_length;
      }
      if (m_pulseType == KTYPE)  
      {
        // distance in X direction: same idea
        m_xCoord = m_usRising - m_lastFalling;
        average_list_x[average_pointer_x] = m_xCoord;
        average_pointer_x = (average_pointer_x + 1) % average_list_length;
      }

      m_spurious = 0;
    }

    if (m_spurious++ > 60) m_vivestatus = VIVE_SYNC_ONLY; 

    m_lastFalling = m_usFalling;
  }
}

void Vive::start()
{
  // no more attachInterrupt; MCPWM uses this pointer directly
  s_instance = this;
}

void Vive::begin()
{
  Serial.println("Vive setup");

  // tie s_instance
  start();

  // initialize MCPWM capture
  esp_err_t err = initCapture();
  if (err != ESP_OK) {
    Serial.printf("Vive MCPWM init failed: %s\n", esp_err_to_name(err));
  }
}

void Vive::begin(int pin)
{
  m_pin = pin;
  begin();
}

void Vive::stop()
{
  // disable capture channel & timer if allocated
  if (m_capChannel) {
    mcpwm_capture_channel_disable(m_capChannel);
    m_capChannel = nullptr;
  }
  if (m_capTimer) {
    mcpwm_capture_timer_stop(m_capTimer);
    m_capTimer = nullptr;
  }
}

int Vive::status()
{
  return m_vivestatus;
}

uint32_t Vive::sync(int reps)
{
  int i = 0;
  uint32_t lastFallingLocal;
  uint32_t startms = millis();

  lastFallingLocal = m_usFalling;

  // count pulses over enough time for reps cycles
  while (millis() - startms < (reps + 1) * 1000 / 120)
  {
    if (lastFallingLocal != m_usFalling)
    {
      lastFallingLocal = m_usFalling;
      i++;
    }
    yield();
  }

  if (i == 0)
  {
    m_vivestatus = VIVE_NO_SIGNAL; // just for debugging info
    #ifdef DEBUG2
    ets_printf("no signal ");
    #endif
  }
  else if (i < 2 * reps)
  {
    m_vivestatus = VIVE_SYNC_ONLY; // just for debugging info
    #ifdef DEBUG2
    ets_printf("missing some pulses %d/%d ", i, 2*reps);
    #endif
  } 
  else
  {
    m_vivestatus = VIVE_RECEIVING;
    #ifdef DEBUG2
    ets_printf(" Sweep and Sync received ");
    #endif
  }

  return m_vivestatus;
}
