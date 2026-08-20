#include "hvac_state_machine.h"

#include "hardware_manager.h"
#include "hvac_states.h"
#include "sensor_manager.h"

#include "esp_log.h"

static const char *TAG = "HVAC_FSM";

//~ QUEUE
QueueHandle_t hvac_queue = NULL;

//~ SYSTEM STATE
hvac_state_t g_current_hvac_state = STATE_IDLE;
hvac_flt_t g_current_hvac_fault = FLT_NONE;
hvac_cmd_t g_current_hvac_cmd = CMD_OFF;

//~ FAN MODE
static bool fan_auto = true;

//~ TIMER
static TimerHandle_t state_pacing_timer = NULL;
static hvac_state_t g_next_pending_state = STATE_IDLE;

//~ STATE UPDATE
static void update_state(hvac_state_t state)
{
  ESP_LOGI(TAG, "State Transition: %d -> %d", g_current_hvac_state, state);

  g_current_hvac_state = state;

}

//~ FAULT UPDATE
static void update_fault(hvac_flt_t fault)
{
  if (fault != FLT_NONE)
  {
    ESP_LOGE(TAG, "HVAC Fault Occurred: Fault Code %d", fault);
  }

  g_current_hvac_fault = fault;

}

//~ SAFE SHUTDOWN
static void safe_shutdown_actuators(void)
{
  set_heater_state(0);
  set_fan_state(0);

  stop_flame_proving_monitor();
  stop_tach_monitoring();

  if (state_pacing_timer != NULL)
  {
    xTimerStop(state_pacing_timer, portMAX_DELAY);
  }

  if (flame_proving_timer != NULL)
  {
    xTimerStop(flame_proving_timer, portMAX_DELAY);
  }

  if (fan_warmup_timer != NULL)
  {
    xTimerStop(fan_warmup_timer, portMAX_DELAY);
  }

  if (tach_window_timer != NULL)
  {
    xTimerStop(tach_window_timer, portMAX_DELAY);
  }
}

//~ STATE DELAY CALLBACK
static void state_delay_callback(TimerHandle_t xTimer)
{
  (void)xTimer;

  hvac_cmd_t cmd = CMD_STATE_DELAY_COMPLETE;

  xQueueSend(hvac_queue, &cmd, 0);
}

// TRANSITION DELAY
static void transition_with_delay(hvac_state_t target_state,
                                  uint32_t delay_ms)
{
  g_next_pending_state = target_state;

  update_state(STATE_WAIT_DELAY);

  xTimerChangePeriod(state_pacing_timer, pdMS_TO_TICKS(delay_ms),
                     portMAX_DELAY);

  xTimerStart(state_pacing_timer, portMAX_DELAY);
}

// FAULT ISR
void IRAM_ATTR fault_isr_handler(void *arg)
{
  (void)arg;
  hvac_cmd_t cmd = CMD_OFF;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(hvac_queue, &cmd, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken == pdTRUE)
  {
    portYIELD_FROM_ISR();
  }
}

// INITIALIZATION
esp_err_t hvac_state_machine_init(void)
{
  ESP_LOGI(TAG, "Initializing HVAC State Machine...");

  hvac_queue = xQueueCreate(10, sizeof(hvac_cmd_t));

  if (hvac_queue == NULL)
  {
    ESP_LOGE(TAG, "Failed to create HVAC command queue!");

    return ESP_FAIL;
  }
  state_pacing_timer = xTimerCreate("StatePacingTimer", pdMS_TO_TICKS(4000),
                                    pdFALSE, NULL, state_delay_callback);

  if (state_pacing_timer == NULL)
  {
    ESP_LOGE(TAG, "Failed to create state pacing timer!");

    return ESP_FAIL;
  }
  g_current_hvac_state = STATE_IDLE;
  g_current_hvac_fault = FLT_NONE;
  g_current_hvac_cmd = CMD_OFF;
  fan_auto = true;
  safe_shutdown_actuators();
  BaseType_t task_result = xTaskCreate(
      hvac_state_machine_task, "hvac_state_machine", 4096, NULL, 5, NULL);

  if (task_result != pdPASS)
  {
    ESP_LOGE(TAG, "Failed to create HVAC state machine task!");

    return ESP_FAIL;
  }
  return ESP_OK;
}

//~ MAIN FSM TASK
void hvac_state_machine_task(void *pvParameters)
{
  (void)pvParameters;
  hvac_cmd_t event;
  ESP_LOGI(TAG, "HVAC State Machine Task Started.");

  while (1)
  {
    if (xQueueReceive(hvac_queue, &event, portMAX_DELAY) == pdTRUE)
    {

      g_current_hvac_cmd = event;

      // GLOBAL OFF COMMAND
      if (event == CMD_OFF)
      {
        safe_shutdown_actuators();
        update_fault(FLT_NONE);
        update_state(STATE_IDLE);
        fan_auto = true;
        ESP_LOGI(TAG, "System returned to IDLE");
        continue;
      }

      // FAN ON COMMAND
      if (event == CMD_FAN_ON)
      {
        fan_auto = false;
        set_fan_state(1);
        if (g_current_hvac_state == STATE_IDLE)
        {
          update_state(STATE_FAN_CIRCULATE);
        }

        continue;
      }

      // FAN AUTO COMMAND
      if (event == CMD_FAN_AUTO)
      {
        fan_auto = true;
        /*
         * If we are not heating, turn the fan off.
         */
        if (g_current_hvac_state == STATE_FAN_CIRCULATE ||
            g_current_hvac_state == STATE_IDLE)
        {
          set_fan_state(0);
          update_state(STATE_IDLE);
        }
        continue;
      }

      // INTER-STATE DELAY
      if (g_current_hvac_state == STATE_WAIT_DELAY)
      {
        if (event == CMD_STATE_DELAY_COMPLETE)
        {
          hvac_state_t next = g_next_pending_state;
          update_state(next);

          if (next == STATE_IGNITION)
          {
            set_heater_state(1);
            xTimerStart(flame_proving_timer, portMAX_DELAY);

            start_flame_proving_monitor();
          }
          else if (next == STATE_WARMUP)
          {
            xTimerStart(fan_warmup_timer, portMAX_DELAY);
          }
          else if (next == STATE_VERIFY_RPM)
          {
            set_fan_state(1);
            start_tach_monitoring();
            xTimerStart(tach_window_timer, portMAX_DELAY);
          }
        }
        continue;
      }

      // NORMAL STATE MACHINE
      switch (g_current_hvac_state)
      {
      case STATE_IDLE:
        if (event == CMD_HEAT)
        {
          set_fan_state(0);
          transition_with_delay(STATE_IGNITION, 1000);
        }

        break;

      case STATE_FAN_CIRCULATE:
        if (event == CMD_HEAT)
        {
          transition_with_delay(STATE_IGNITION, 1000);
        }

        break;

      case STATE_IGNITION:
        if (event == CMD_FLAME_DETECTED)
        {
          stop_flame_proving_monitor();
          xTimerStop(flame_proving_timer, portMAX_DELAY);
          transition_with_delay(STATE_WARMUP, 1000);
        }
        else if (event == CMD_FLAME_TIMEOUT)
        {
          safe_shutdown_actuators();
          update_fault(FLT_FLAME);
          update_state(STATE_FAULT);
        }

        break;

      case STATE_WARMUP:
        if (event == CMD_WARMUP_DONE)
        {
          xTimerStop(fan_warmup_timer, portMAX_DELAY);
          transition_with_delay(STATE_VERIFY_RPM, 500);
        }

        break;

      case STATE_VERIFY_RPM:
        if (event == CMD_FAN_OK)
        {
          xTimerStop(tach_window_timer, portMAX_DELAY);

          stop_tach_monitoring();
          transition_with_delay(STATE_RUNNING, 1000);
        }
        else if (event == CMD_TACH_TIMEOUT)
        {
          safe_shutdown_actuators();
          update_fault(FLT_FAN);
          update_state(STATE_FAULT);
        }

        break;

      case STATE_RUNNING:
        /*
         * Heating is running.
         *
         * In FAN AUTO mode the fan remains on
         * while heating.
         *
         * In FAN ON mode the fan also remains on.
         */
        set_heater_state(1);
        set_fan_state(1);
        break;

      case STATE_FAULT:
        /*
         * System remains locked out until
         * CMD_OFF is received.
         */
        set_heater_state(0);
        set_fan_state(0);
        break;

      default:
        safe_shutdown_actuators();
        update_state(STATE_IDLE);
        break;
      }
    }
  }
}