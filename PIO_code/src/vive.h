#ifndef VIVE510
#define VIVE510

#include <arduino.h>
#include <FunctionalInterrupt.h>
#include <algorithm>

extern "C" {
  #include "driver/mcpwm_prelude.h"
  #include "esp_err.h"
}

// vive status errors
#define VIVE_NO_SIGNAL  0
#define VIVE_SYNC_ONLY  1 
#define VIVE_RECEIVING  2

#define average_list_length 5

#define KTYPE 2
#define JTYPE 1

class Vive
{
    private:
        volatile uint32_t m_usRising;   // rising edge time in us
        volatile uint32_t m_usFalling;  // falling edge time in us
        volatile uint32_t status_check_time;

        uint16_t m_xCoord;
        uint16_t m_yCoord;

        int m_vivestatus = 0;
        int m_pin;               // signal input pin

        int m_sweepWidth = 50;
        int m_pulseType;         // 1 is J, 2 is K

        int average_list_x[average_list_length] = {};
        int average_pointer_x = 0;
        int average_list_y[average_list_length] = {};
        int average_pointer_y = 0;

        uint32_t m_lastFalling;
        int      m_spurious;

        int m_groupID = 0;

        // original classification helpers
        int isJPulse(uint32_t pulsewidth);
        int isKPulse(uint32_t pulsewidth);
        void processPulse();     // unchanged logic

        // ===== MCPWM capture integration =====
        uint32_t                    m_resolutionHz = 80'000'000; // 80 MHz timer
        mcpwm_cap_timer_handle_t    m_capTimer   = nullptr;
        mcpwm_cap_channel_handle_t  m_capChannel = nullptr;

        // MCPWM capture ISR
        static bool IRAM_ATTR captureTrampoline(mcpwm_cap_channel_handle_t cap_chan,
                                                const mcpwm_capture_event_data_t *edata,
                                                void *user_ctx);
        void IRAM_ATTR onCapture(const mcpwm_capture_event_data_t *edata);

        // init MCPWM capture on m_pin
        esp_err_t initCapture();

        static Vive* s_instance;

    public:
        int pulsewidth_list[200] = {};
        int type_list[200]       = {};
        int temp_counter         = 0;

        Vive(int pin, int group_ID = 0);

        uint16_t xCoord();
        uint16_t yCoord();
        uint32_t sync(int reps);
        int status();
        void stop();
        void start();      // now just ties s_instance to this
        void begin();
        void begin(int pin);
};

#endif