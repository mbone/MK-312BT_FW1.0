/*
 * MK-312BT Firmware - Main Arduino Sketch
 *
 * ATmega16 @ 8 MHz external crystal
 *
 * This is the main entry point. The firmware flow is:
 *   setup():
 *     1. Initialize hardware peripherals (USART, SPI, ADC, GPIO)
 *     2. Initialize LCD, DAC, mode engine, config
 *     3. Run self-test (DAC voltage check, FET calibration)
 *     4. Initialize pulse generator (Timer1/Timer2)
 *     5. Enable watchdog and interrupts
 *
 *   loop():
 *     1. Reset watchdog timer
 *     2. Handle button input (mode selection, menu navigation)
 *     3. Read level pots and MA knob via ADC, update DAC intensity
 *     4. Run mode dispatcher (parameter engine or bytecode)
 *     5. For audio modes, process audio inputs to modulate intensity
 *     6. Convert channel_a/channel_b register values to pulse generator parameters
 *     7. Update LCD display periodically
 *
 * All live channel state (gate, freq, width, intensity, ramp) is read
 * directly from channel_a / channel_b (ChannelBlock). g_mk312bt_state
 * holds only the small set of globals that are not channel-specific.
 */

#include <avr/wdt.h>
#include <util/delay.h>
#include "MK312BT_Memory.h"
#include "MK312BT_Constants.h"
#include "MK312BT_Modes.h"
#include "lcd.h"
#include "MK312BT_Utils.h"
#include "mode_programs.h"
#include "mode_dispatcher.h"
#include "channel_mem.h"
#include "config.h"
#include "audio_processor.h"
#include "eeprom.h"
#include "menu.h"
#include "serial.h"
#include "dac.h"
#include "pulse_gen.h"
#include "prng.h"
#include "user_programs.h"

volatile MK312BTState g_mk312bt_state;
eeprom_config_t g_menu_config;

unsigned long last_menu_update = 0;
unsigned long last_ramp_update = 0;
unsigned long last_mode_update = 0;
unsigned long last_button_poll = 0;

uint8_t CurrentModeIX = 0;

uint16_t ChannelAPwrBase;
uint16_t ChannelBPwrBase;
uint16_t ChannelAModulationBase;
uint16_t ChannelBModulationBase;

uint16_t MAInput = 0;

static uint8_t last_power_level = 0xFF;

static void applyPowerLevel(void) {
  uint8_t pl = g_menu_config.power_level;
  if (pl == last_power_level) return;
  last_power_level = pl;
  switch (pl) {
    case 0:
      ChannelAPwrBase = PWR_LEVEL_LOW_BASE;
      ChannelBPwrBase = PWR_LEVEL_LOW_BASE;
      ChannelAModulationBase = PWR_LEVEL_LOW_MOD;
      ChannelBModulationBase = PWR_LEVEL_LOW_MOD;
      break;
    case 2:
      ChannelAPwrBase = PWR_LEVEL_HIGH_BASE;
      ChannelBPwrBase = PWR_LEVEL_HIGH_BASE;
      ChannelAModulationBase = PWR_LEVEL_HIGH_MOD;
      ChannelBModulationBase = PWR_LEVEL_HIGH_MOD;
      break;
    default:
      ChannelAPwrBase = PWR_LEVEL_NORMAL_BASE;
      ChannelBPwrBase = PWR_LEVEL_NORMAL_BASE;
      ChannelAModulationBase = PWR_LEVEL_NORMAL_MOD;
      ChannelBModulationBase = PWR_LEVEL_NORMAL_MOD;
      break;
  }
}

static uint8_t ramp_scale_intensity(uint8_t intensity, uint8_t ramp_val) {
    return (uint8_t)(((uint16_t)intensity * ramp_val) >> 8);
}

void setup() {
  cli();
  wdt_reset();
  WDTCR |= (1 << WDTOE) | (1 << WDE);
  WDTCR = 0x00;
  initializeHardware();
  initialize_lcd();
  lcd_backlight_on();
  serial_init();

  prng_init((uint16_t)TCNT0 ^ ((uint16_t)TCNT1L << 8) ^ 0x5A3C);

  dac_init();
  mode_dispatcher_init();
  config_init();
  audio_init();

  config_load_from_eeprom();
  config_apply_to_memory();

  if (!eeprom_load_config(&g_menu_config)) {
    eeprom_init_defaults(&g_menu_config);
  }
  if (g_menu_config.top_mode >= MODE_COUNT) {
    g_menu_config.top_mode = 0;
  }
  if (g_menu_config.top_mode >= MODE_USER1 && g_menu_config.top_mode < MODE_SPLIT) {
    if (!user_prog_is_valid(g_menu_config.top_mode - MODE_USER1)) {
      g_menu_config.top_mode = 0;
    }
  }

  static const char chk_hw[] PROGMEM = "Check hardware";
  if (!dacTest() || !fetCalibrate()) {
    lcd_clear();
    lcd_set_cursor(0, 0);
    lcd_write_string_P(PSTR("FET OR DAC FAIL - HALT"));
    lcd_set_cursor(0, 1);
    lcd_write_string_P(chk_hw);
    while (1) { wdt_reset(); PORTD ^= (1<<PORTD_BIT_LED_A)|(1<<PORTD_BIT_LED_B); _delay_ms(200); }
  }

  displayStartupScreen();

  _delay_ms(300);
  lcd_clear();
  lcd_set_cursor(8, 0);
  lcd_write_string_P(PSTR("MK-312BT"));
  lcd_set_cursor(0, 1);
  lcd_write_string_P(PSTR("Selftest OK MK02"));
  _delay_ms(1500);
  menuInit();
  menuShowStartup();
  while (1) {
    wdt_reset();
    serial_process();
    lcd_enable_buttons();
    _delay_us(50);
    bool ok   = !digitalRead(BUTTON_OK_PIN);
    bool up   = !digitalRead(BUTTON_UP_PIN);
    bool down = !digitalRead(BUTTON_DOWN_PIN);
    bool men  = !digitalRead(BUTTON_MENU_PIN);
    lcd_disable_buttons();
    if (ok || up || down || men) {
      _delay_ms(50);
      lcd_enable_buttons();
      _delay_us(50);
      ok   = !digitalRead(BUTTON_OK_PIN);
      up   = !digitalRead(BUTTON_UP_PIN);
      down = !digitalRead(BUTTON_DOWN_PIN);
      men  = !digitalRead(BUTTON_MENU_PIN);
      lcd_disable_buttons();
      if (ok || up || down || men) {
        while (1) {
          wdt_reset();
          _delay_ms(20);
          lcd_enable_buttons();
          _delay_us(50);
          ok   = !digitalRead(BUTTON_OK_PIN);
          up   = !digitalRead(BUTTON_UP_PIN);
          down = !digitalRead(BUTTON_DOWN_PIN);
          men  = !digitalRead(BUTTON_MENU_PIN);
          lcd_disable_buttons();
          if (!ok && !up && !down && !men) break;
        }
        break;
      }
    }
    _delay_ms(20);
  }

  applyPowerLevel();

  dac_write_channel_a(DAC_MAX_VALUE);
  dac_write_channel_b(DAC_MAX_VALUE);

  pulse_gen_init();

  last_button_poll = millis();

  CurrentModeIX = g_menu_config.top_mode;
  mode_dispatcher_select_mode(CurrentModeIX);

  sei();
  wdt_enable(WDTO_2S);
  menuStartOutput();

  mode_dispatcher_update();

  last_menu_update = millis();
  last_ramp_update = millis();
  last_mode_update = millis();
}

uint8_t readAndUpdateChannel(uint8_t c) {
  if (*POT_LOCKOUT_FLAGS & 0x01) {
    return 0;
  }

  uint8_t menu_ramp = menuGetRampPercent();

  if (c == 0) {
    uint16_t v = analogRead(ADC_CHANNEL_LEVEL_A_PIN);
    uint8_t bv = min(99, max(0, (uint8_t)(v >> 3)));
uint16_t dac_val = ChannelAPwrBase +
  ((uint32_t)ChannelAModulationBase * (uint32_t)(DAC_MAX_VALUE - v)) / 1024;
    if (dac_val > DAC_MAX_VALUE) dac_val = DAC_MAX_VALUE;

    uint8_t intensity = ramp_scale_intensity(channel_a.intensity_value, channel_a.ramp_value);
    intensity = (uint8_t)(((uint16_t)intensity * menu_ramp) / 100);
    dac_val = DAC_MAX_VALUE - (uint16_t)(((uint32_t)(DAC_MAX_VALUE - dac_val) * intensity) >> 8);

    dac_write_channel_a(dac_val);
    return bv;
  } else {
    uint16_t v = analogRead(ADC_CHANNEL_LEVEL_B_PIN);
    uint8_t bv = min(99, max(0, (uint8_t)(v / 8)));
    uint16_t dac_val = ChannelBPwrBase +
      ((uint32_t)ChannelBModulationBase * (uint32_t)(DAC_MAX_VALUE - v)) / 1024;
    if (dac_val > DAC_MAX_VALUE) dac_val = DAC_MAX_VALUE;

    uint8_t intensity = ramp_scale_intensity(channel_b.intensity_value, channel_b.ramp_value);
    intensity = (uint8_t)(((uint16_t)intensity * menu_ramp) / 100);
    dac_val = DAC_MAX_VALUE - (uint16_t)(((uint32_t)(DAC_MAX_VALUE - dac_val) * intensity) >> 8);

    dac_write_channel_b(dac_val);
    return bv;
  }
}

void runningLine1() {
  readAndUpdateChannel(0);
  readAndUpdateChannel(1);

  if (!(*POT_LOCKOUT_FLAGS & 0x08)) {
    MAInput = analogRead(ADC_MULTI_ADJ_VR3G1);
    if (MAInput > DAC_MAX_VALUE) MAInput = DAC_MAX_VALUE;

    system_config_t* sys_config = config_get();
   sys_config->multi_adjust = (uint8_t)(MAInput >> 2);
  }
}

void loop() {
  wdt_reset();

  serial_process();

  {
    uint8_t deferred_result = mode_dispatcher_poll_deferred();
    if (deferred_result) {
      if (deferred_result == 3) {
        menuStartOutput();
      } else if (deferred_result == 1) {
        CurrentModeIX = mode_dispatcher_get_mode();
        config_get()->current_mode = CurrentModeIX;
        g_menu_config.top_mode = CurrentModeIX;
        menuStartOutput();
      }
    }
  }

  if (millis() - last_button_poll >= 20) {
    last_button_poll = millis();
    handleUserInput();
  }

  applyPowerLevel();
  //config_sync_from_eeprom_config(&g_menu_config);
  runningLine1();

  if (millis() - last_mode_update >= 4) {
    last_mode_update = millis();
    mode_dispatcher_update();

    // --- Audio processing for audio modes ---
    uint8_t cur_mode = mode_dispatcher_get_mode();
    if (cur_mode >= MODE_AUDIO1 && cur_mode <= MODE_AUDIO3) {
        audio_process_channel_a();
        audio_process_channel_b();
    }
  }

  uint8_t gate_a  = channel_a.gate_value & GATE_ON_BIT;
  uint8_t freq_a  = channel_a.freq_value;
  uint8_t width_a = channel_a.width_value;

  uint8_t gate_b  = channel_b.gate_value & GATE_ON_BIT;
  uint8_t freq_b  = channel_b.freq_value;
  uint8_t width_b = channel_b.width_value;

  uint16_t period_a_us = (freq_a < 2) ? 65000 : (uint16_t)((uint32_t)freq_a * 256UL);
  uint16_t period_b_us = (freq_b < 2) ? 65000 : (uint16_t)((uint32_t)freq_b * 256UL);

  uint8_t width_a_us = (uint8_t)(70 + ((uint16_t)width_a * 180) >> 8);
  uint8_t width_b_us = (uint8_t)(70 + ((uint16_t)width_b * 180) >> 8);

  pulse_set_width_a(width_a_us);
  pulse_set_width_b(width_b_us);
  pulse_set_frequency_a(period_a_us);
  pulse_set_frequency_b(period_b_us);

  bool output_on = menuIsOutputEnabled();
  uint8_t pulse_a_on = (output_on && gate_a && freq_a >= 2) ? PULSE_ON : PULSE_OFF;
  uint8_t pulse_b_on = (output_on && gate_b && freq_b >= 2) ? PULSE_ON : PULSE_OFF;
  pulse_set_gate_a(pulse_a_on);
  pulse_set_gate_b(pulse_b_on);

  static uint8_t pwm_phase = 0;
pwm_phase++;

uint8_t brightness_a = ((uint16_t)channel_a.intensity_value * channel_a.width_value) >> 8;
uint8_t brightness_b = ((uint16_t)channel_b.intensity_value * channel_b.width_value) >> 8;

//bool output_on = menuIsOutputEnabled();
bool active_a = output_on && (channel_a.gate_value & GATE_ON_BIT) && (channel_a.freq_value >= 2);
bool active_b = output_on && (channel_b.gate_value & GATE_ON_BIT) && (channel_b.freq_value >= 2);

if (active_a && (brightness_a > pwm_phase))
    PORTD &= ~(1 << PORTD_BIT_LED_A);   // LED A on
else
    PORTD |= (1 << PORTD_BIT_LED_A);    // LED A off

if (active_b && (brightness_b > pwm_phase))
    PORTD &= ~(1 << PORTD_BIT_LED_B);   // LED B on
else
    PORTD |= (1 << PORTD_BIT_LED_B);    // LED B off


  if (millis() - last_menu_update >= 200) {
    last_menu_update = millis();
    if (menu_state.current_menu == MENU_MAIN) {
      menuShowMode(g_menu_config.top_mode);
    }
  }

  if (menuIsRampActive() && millis() - last_ramp_update >= 20) {
    last_ramp_update = millis();
    menuHandleRampUp();
  }
}

void initializeHardware() {
  cli();

  DDRB = 0xFF;
  DDRC = 0xFF;
  DDRD = 0xFF;

  PORTB = 0x00;
  PORTC = 0x00;
  PORTD = PORTD_INIT_STATE;

  UBRRH = (uint8_t)(USART_UBRR_VALUE >> 8);
  UBRRL = (uint8_t)USART_UBRR_VALUE;
  UCSRC = (1 << URSEL) | (1 << UCSZ1) | (1 << UCSZ0);
  UCSRB = (1 << RXEN) | (1 << TXEN);

  ADMUX  = ADC_VREF_AVCC;
  ADCSRA = (1 << ADEN) | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

  SPCR = SPI_MASTER_MODE | SPI_CLOCK_DIV_16;

  GICR  = 0x00;
  MCUCR = (1 << ISC01) | (1 << ISC11);

  sei();
}

void handleUserInput() {
  ButtonEvent event = BUTTON_NONE;

  static bool last_up = true, last_down = true, last_menu = true, last_ok = true;
  static unsigned long last_event_ms = 0;

  lcd_enable_buttons();
  _delay_us(50);

  bool up_reading   = !digitalRead(BUTTON_UP_PIN);
  bool down_reading = !digitalRead(BUTTON_DOWN_PIN);
  bool menu_reading = !digitalRead(BUTTON_MENU_PIN);
  bool ok_reading   = !digitalRead(BUTTON_OK_PIN);

  lcd_disable_buttons();

  unsigned long now = millis();
  bool debounce_ok = (now - last_event_ms) >= 150;

  bool up_pressed   = debounce_ok && (up_reading   && !last_up);
  bool down_pressed = debounce_ok && (down_reading && !last_down);
  bool menu_pressed = debounce_ok && (menu_reading && !last_menu);
  bool ok_pressed   = debounce_ok && (ok_reading   && !last_ok);

  last_up   = up_reading;
  last_down = down_reading;
  last_menu = menu_reading;
  last_ok   = ok_reading;

  if (menu_pressed) {
    event = BUTTON_MENU;
  } else if (ok_pressed) {
    event = BUTTON_OK;
  } else if (up_pressed) {
    event = BUTTON_UP;
  } else if (down_pressed) {
    event = BUTTON_DOWN;
  }

  if (event != BUTTON_NONE) {
    last_event_ms = now;
    *POT_LOCKOUT_FLAGS = 0x00;
    menuHandleButton(event);
    if (CurrentModeIX != g_menu_config.top_mode) {
      CurrentModeIX = g_menu_config.top_mode;
      mode_dispatcher_select_mode(CurrentModeIX);
    }
  }
}

void displayStartupScreen() {
  lcd_clear();
  lcd_set_cursor(0, 0);
  lcd_write_string_P(PSTR("Initializing..."));
  _delay_ms(50);
  for (uint8_t i = 1; i <= 16; i++) {
    lcd_show_progress(i, 16);
    _delay_ms(50);
  }
}
