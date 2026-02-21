# MK-312BT Firmware Architecture

This document describes the architecture of the MK-312BT firmware reimplementation: module responsibilities, function call graphs, data flow, and hardware mappings.

---

## Table of Contents

1. [System Block Diagram](#system-block-diagram)
2. [Module Overview](#module-overview)
3. [Per-Module Function Reference](#per-module-function-reference)
4. [Complete Function Call Graph](#complete-function-call-graph)
5. [Data Structure Hierarchy](#data-structure-hierarchy)
6. [Hardware Register Map](#hardware-register-map)
7. [Timing Diagram](#timing-diagram)
8. [Data Flow Descriptions](#data-flow-descriptions)
9. [Memory Map](#memory-map)
10. [Serial Protocol Reference](#serial-protocol-reference)

---

## System Block Diagram

```
                 ┌─────────────────────────────────────────────────┐
                 │              ATmega16 @ 8 MHz                    │
                 │                                                  │
                 │  ┌──────────┐  ┌──────────┐  ┌──────────────┐  │
                 │  │  Timer1  │  │  Timer2  │  │    USART     │  │
                 │  │ 16-bit   │  │  8-bit   │  │  19200 baud  │  │
                 │  │ CTC /8   │  │ CTC /8   │  │              │  │
                 │  └────┬─────┘  └────┬─────┘  └──────┬───────┘  │
                 │       │              │                │          │
                 │  ┌────▼─────────────▼──┐        ┌───▼──────┐   │
                 │  │   interrupts.c      │        │ serial.c │   │
                 │  │  Timer ISRs         │        │ Protocol │   │
                 │  │  Biphasic pulse     │        │ Handler  │   │
                 │  │  state machine      │        └───┬──────┘   │
                 │  └────┬────────────────┘            │          │
                 │       │ volatile shared              │          │
                 │  ┌────▼────────────────┐       ┌────▼──────┐   │
                 │  │    pulse_gen.c      │       │serial_mem │   │
                 │  │  ChannelPulseState  │       │ virtual   │   │
                 │  │  pending_*/dirty    │       │ addr map  │   │
                 │  └─────────────────────┘       └───────────┘   │
                 └─────────────────────────────────────────────────┘
                           │                           │
         ┌─────────────────┼───────────────────────────┘
         │                 │
         ▼                 ▼
   ┌───────────┐    ┌──────────────────────────────────────────────┐
   │  H-bridge │    │                MK312BT.ino (main loop)         │
   │  FET gate │    │                                              │
   │  PB0-PB3  │    │  setup()  →  loop() ~50 Hz                  │
   │           │    │                                              │
   │  Ch A:    │    │  ┌──────────┐  ┌────────────┐  ┌─────────┐  │
   │   PB2 +   │    │  │handleUser│  │mode_dispat-│  │ serial  │  │
   │   PB3 -   │    │  │Input()   │  │cher_update │  │_process │  │
   │  Ch B:    │    │  │ 20ms     │  │ ()  8ms    │  │()  poll │  │
   │   PB0 +   │    │  └────┬─────┘  └─────┬──────┘  └─────────┘  │
   │   PB1 -   │    │       │               │                      │
   └───────────┘    │  ┌────▼─────┐   ┌─────▼──────────────────┐  │
         │          │  │ menu.c   │   │    param_engine.c       │  │
         │          │  │ 4-button │   │  intensity/freq/width   │  │
         ▼          │  │ LCD menu │   │  ramp engine  ~244 Hz   │  │
   ┌───────────┐    │  └────┬─────┘   └─────┬──────────────────-┘  │
   │Transformer│    │       │               │                      │
   │ (isolated)│    │  ┌────▼─────┐   ┌─────▼──────────────────┐  │
   │  Ch A out │    │  │ eeprom.c │   │  mode_dispatcher.c     │  │
   │  Ch B out │    │  │ config.c │   │  bytecode interpreter  │  │
   └───────────┘    │  └──────────┘   │  module chaining       │  │
                    │                 └─────┬──────────────────-┘  │
                    │                       │                      │
                    │            ┌──────────┼──────────┐           │
                    │            │          │          │           │
                    │       ┌────▼───┐ ┌────▼────┐ ┌───▼────────┐ │
                    │       │channel_│ │mode_pro-│ │user_prog-  │ │
                    │       │mem.c   │ │grams.c  │ │rams.c      │ │
                    │       │Ch A/B  │ │PROGMEM  │ │EEPROM slot │ │
                    │       │blocks  │ │bytecode │ │programs    │ │
                    │       └────────┘ └─────────┘ └────────────┘ │
                    │                                              │
                    │  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
                    │  │  dac.c   │  │  adc.c   │  │  lcd.c   │   │
                    │  │ LTC1661  │  │ 6-ch ADC │  │ HD44780  │   │
                    │  │ 10-bit   │  │ level/MA │  │ 4-bit    │   │
                    │  │ dual DAC │  │ audio/bat│  │ 2x16 LCD │   │
                    │  └────┬─────┘  └────┬─────┘  └────┬─────┘   │
                    └───────┼─────────────┼──────────────┼─────────┘
                            │             │              │
                            ▼             ▼              ▼
                      ┌──────────┐  ┌──────────┐  ┌──────────┐
                      │ LTC1661  │  │  PA0-PA7 │  │  PC0-PC7 │
                      │ SPI DAC  │  │ ADC pins │  │ LCD/btns │
                      │ PORTD PD4│  │          │  │ mux'd    │
                      └──────────┘  └──────────┘  └──────────┘
```

---

## Module Overview

| Module | File(s) | Responsibility |
|--------|---------|----------------|
| **Main Loop** | MK312BT.ino | Setup, loop dispatch, DAC calculation, ramp scaling |
| **Pulse Generator** | pulse_gen.c/h | Double-buffered pulse parameters, Timer1/Timer2 setup |
| **ISRs** | interrupts.c | Timer1/Timer2 biphasic pulse state machine |
| **DAC Driver** | dac.c/h | LTC1661 10-bit dual DAC over SPI |
| **ADC Driver** | adc.c/h | 10-bit ADC reads (level pots, audio, battery, MA knob) |
| **LCD Driver** | lcd.c/h | HD44780 4-bit LCD, PORTC pin multiplex with buttons |
| **Menu System** | menu.c/h | 4-button navigation, all UI screens, ramp-up logic |
| **Mode Dispatcher** | mode_dispatcher.c/h | Mode selection, bytecode execution, gate timer, output copy |
| **Param Engine** | param_engine.c/h | Autonomous intensity/freq/width sweeping, MA/ADV scaling |
| **Mode Programs** | mode_programs.c/h | PROGMEM bytecode library (36 modules) |
| **Channel Memory** | channel_mem.c/h | ChannelBlock struct (64 bytes per channel), address mapping |
| **Config Manager** | config.c/h | Runtime system_config_t singleton, EEPROM sync |
| **EEPROM Driver** | eeprom.c/h | Byte-level EEPROM r/w, config struct save/load, user prog slots |
| **Serial Protocol** | serial.c/h | MK-312BT serial protocol, key exchange, encryption |
| **Serial Memory** | serial_mem.c/h | Virtual address translation (Flash/RAM/EEPROM regions) |
| **User Programs** | user_programs.c/h | 7-slot user program cache, SET-bytecode execution |
| **Audio Processor** | audio_processor.c/h | Audio envelope follower, writes intensity mod registers |
| **PRNG** | prng.c/h | 16-bit LCG PRNG (seeded from hardware timer noise) |
| **Utils/Diagnostics** | utils.c | DAC self-test, FET calibration, current sense ADC |

---

## Per-Module Function Reference

### MK312BT.ino — Main Loop

```
setup()
  └─ initializeHardware()          Configure GPIO, USART, ADC, SPI, Timer0
  └─ initialize_lcd()              HD44780 init sequence
  └─ serial_init()                 Reset serial protocol state
  └─ prng_init(TCNT0^TCNT1L)       Seed PRNG from hardware timer noise
  └─ dac_init()                    SPI master mode, wake LTC1661
  └─ mode_dispatcher_init()        Init channel blocks, param engine, user progs
  └─ config_init()                 Set factory defaults
  └─ config_load_from_eeprom()     Load saved config or keep defaults
  └─ dacTest()                     Verify SPI DAC communication
  └─ fetCalibrate()                Measure H-bridge FET baselines
  └─ menuInit()                    Define custom LCD chars, init state
  └─ menuShowStartup()             Splash screen + battery %
  └─ waitForAnyButton()            Block until first button press
  └─ pulse_gen_init()              Start Timer1/Timer2 CTC, gates OFF
  └─ sei()                         Enable global interrupts
  └─ wdt_enable(WDTO_2S)           Arm 2-second watchdog

loop()   [~50 Hz iteration]
  ├─ wdt_reset()
  ├─ serial_process()              Poll USART, process complete packets
  ├─ handleUserInput()             every 20 ms — button poll → menu dispatch
  ├─ applyPowerLevel()             DAC base/modulation on power level change
  ├─ config_sync_from_eeprom_config()  Sync advanced settings into runtime state
  ├─ runningLine1()                Read level pots + MA knob, update DAC
  ├─ mode_dispatcher_update()      every 8 ms — tick param engine, copy output
  ├─ audio_process_channel_a/b()   Only in Audio 1-3 modes
  ├─ [ramp scaling + pulse_set_*]  Apply ramp, set pulse parameters
  ├─ menuShowMode()                every 200 ms — refresh LCD status
  └─ menuHandleRampUp()            every 30 ms if ramp active

readAndUpdateChannel(ch)
  ├─ analogRead(LEVEL_A or LEVEL_B)
  ├─ [DAC calculation]
  │    dac_val = DACbase + (Modulation * (1023 - v)) / 1024
  └─ dac_write_channel_a/b(dac_val)

applyPowerLevel()
  └─ [sets ChannelXPwrBase / ChannelXModulationBase]
       Low:    base=650, mod=220
       Normal: base=590, mod=330
       High:   base=500, mod=440
```

---

### pulse_gen.c / interrupts.c — Pulse Generator

```
pulse_gen.c (main-loop side):
  pulse_gen_init()
    └─ Timer1: CTC /8, OCR1A=500, TIMSK |= OCIE1A
    └─ Timer2: CTC /8, OCR2=250, TIMSK |= OCIE2
    └─ pulse_ch_a.gate = PULSE_OFF, pulse_ch_b.gate = PULSE_OFF

  pulse_set_width_a(width_us)          cli() → pending_width = max(20,w) → dirty=1 → sei()
  pulse_set_width_b(width_us)          cli() → pending_width = max(20,w) → dirty=1 → sei()
  pulse_set_frequency_a(period_us)     cli() → pending_period = max(500,p) → dirty=1 → sei()
  pulse_set_frequency_b(period_us)     cli() → pending_period = max(500,p) → dirty=1 → sei()
  pulse_set_gate_a(on)                 pulse_ch_a.gate = on
  pulse_set_gate_b(on)                 pulse_ch_b.gate = on

interrupts.c (ISR side):

  ISR(TIMER1_COMPA_vect)   [Channel A, Timer1 16-bit, ~244 Hz base]
    ├─ if params_dirty: copy pending_width/period → width_ticks/period_ticks
    └─ Phase state machine:
         PH_GAP         → if gate OFF: re-arm gap; else → PH_POSITIVE
         PH_POSITIVE    → PB2=1, PB3=0, OCR1A=width_ticks, → PH_DEADTIME1
         PH_DEADTIME1   → PB2=0, PB3=0, OCR1A=DEADTIME(4us), → PH_NEGATIVE
         PH_NEGATIVE    → PB2=0, PB3=1, OCR1A=width_ticks, → PH_DEADTIME2
         PH_DEADTIME2   → PB2=0, PB3=0
                           gap = period - 2*width - 2*DEADTIME
                           OCR1A = gap → PH_GAP

  ISR(TIMER2_COMP_vect)    [Channel B, Timer2 8-bit, ~244 Hz base]
    ├─ if gap_remaining > 0: OCR2=min(chunk,250), gap_remaining -= chunk; return
    ├─ if params_dirty: copy pending_width/period → width_ticks/period_ticks
    └─ Same 5-phase state machine as Timer1 except:
         gap > 250 → split into multiple 250us ISR firings via gap_remaining

  ISR(SPI_STC_vect)
    └─ volatile discard = SPDR  (prevent SPI overrun flag)

ChannelPulseState (volatile, shared between ISR and main loop):
  gate           PULSE_ON / PULSE_OFF
  width_ticks    active pulse half-width in µs
  period_ticks   total pulse period in µs
  phase          PH_GAP / PH_POSITIVE / PH_DEADTIME1 / PH_NEGATIVE / PH_DEADTIME2
  gap_remaining  (Ch B only) µs remaining in long gap
  pending_width  double-buffer: written by main, read by ISR when dirty=1
  pending_period double-buffer: written by main, read by ISR when dirty=1
  params_dirty   flag: 1 = ISR should copy pending values on next tick
```

---

### dac.c — LTC1661 DAC Driver

```
dac_init()
  └─ DDRB |= SCK|MOSI, DDRD |= DAC_CS, SPCR = SPE|MSTR|SPR1
  └─ dac_wake()

dac_write_channel_a(value)   ─► dac_send_word(DAC_CMD_LOAD_A|DAC_CMD_UPDATE, value)
dac_write_channel_b(value)   ─► dac_send_word(DAC_CMD_LOAD_B|DAC_CMD_UPDATE, value)
dac_load_a(value)            ─► dac_send_word(DAC_CMD_LOAD_A, value)
dac_load_b(value)            ─► dac_send_word(DAC_CMD_LOAD_B, value)
dac_update()                 ─► dac_send_word(DAC_CMD_UPDATE, 0)
dac_update_both(a, b)
  └─ dac_load_a(a) → dac_load_b(b) → dac_update()
dac_sleep()                  ─► dac_send_word(DAC_CMD_SLEEP, 0)
dac_wake()                   ─► dac_send_word(DAC_CMD_WAKE, 0)
dac_scale_8bit_to_10bit(v)   ─► (uint16_t)(((uint32_t)v * 1023) / 255)

  [internal]
  dac_send_word(cmd, value)
    └─ CS low → spi_transfer(cmd_hi_byte) → spi_transfer(cmd_lo_byte) → CS high
  dac_spi_transfer(byte)
    └─ SPDR = byte → spin on SPSR.SPIF → return SPDR

Inverted relationship: DAC=1023 → minimum output, DAC=0 → maximum output
```

---

### adc.c — ADC Driver

```
adc_read_level_a()    ─► analogRead(PIN_LEVEL_A)  PA4  0-1023
adc_read_level_b()    ─► analogRead(PIN_LEVEL_B)  PA5  0-1023
adc_read_audio_a()    ─► analogRead(PIN_AUDIO_A)  PA7  0-1023  (right line-in)
adc_read_audio_b()    ─► analogRead(PIN_AUDIO_B)  PA6  0-1023  (left line-in/mic)
adc_read_battery()    ─► analogRead(PIN_BATTERY)  PA3  0-1023  (12V divider)
adc_init()            ─► no-op (hardware configured in initializeHardware)
adc_cycle_channels()  ─► no-op (synchronous reads used)
```

---

### lcd.c — HD44780 LCD Driver

```
initialize_lcd()              Full HD44780 4-bit init sequence (with delays)
lcd_clear()                   CMD 0x01, 2ms delay
lcd_set_cursor(col, row)      CMD 0x80 + DDRAM address
lcd_write_string(str)         ─► lcd_disable_buttons() → per-char lcd_write_char() → lcd_enable_buttons()
lcd_write_char(c)             ─► lcd_send_byte(c, RS=1)
lcd_write_number(n)           Extract digits in reverse → print in order
lcd_write_custom_char(loc)    ─► lcd_send_byte(loc, RS=1)
lcd_create_char(loc, bitmap)  CMD 0x40 + loc*8 → send 8 bitmap bytes
lcd_show_progress(step,total) Draw progress bar on row 1

lcd_enable_buttons()          DDRC &= ~0xF0, PORTC |= 0xF0 (inputs with pullup)
lcd_disable_buttons()         DDRC |= 0xF0, PORTC &= ~(1<<PC0) (outputs)
lcd_backlight_on()            PORTD &= ~(1<<PD7)   (active low)
lcd_backlight_off()           PORTD |= (1<<PD7)

  [*_raw variants] — same as above but caller manages bus ownership
  lcd_command_raw, lcd_write_string_raw, lcd_set_cursor_raw,
  lcd_write_custom_char_raw

  [internal]
  lcd_send_byte(val, rs)    ─► lcd_write_nibble(high) → lcd_pulse_enable()
                               lcd_write_nibble(low)  → lcd_pulse_enable()
  lcd_write_nibble(n)       ─► PORTC = (PORTC & ~0xF0) | (n<<4)
  lcd_pulse_enable()        ─► PORTC |= E_bit → 1us delay → PORTC &= ~E_bit
  lcd_command(cmd)          ─► lcd_send_byte(cmd, RS=0) + extra delay if clear/home
```

---

### menu.c — Menu System

```
menuInit()
  └─ lcd_create_char() for 5 battery level icons + up/down arrows
  └─ menu_state = {MENU_MAIN, pos=0, edit=false}

menuShowStartup()
  └─ lcd_clear → "MK-312BT Firmware" → battery % → backlight on

menuShowMode(mode)
  └─ Line 1: [mode name 8 chars] [ramp%|"  OK"] [battery icon]
  └─ Line 2: "A[pct] B[pct] [mode_short]"

menuHandleButton(event)
  └─ dispatches to:
       menu_handle_main()         ─► cycle modes, start output, options entry
       menu_handle_options()      ─► ramp/favorite/power/advanced/save/reset
       menu_handle_power_level()  ─► Low/Normal/High selection
       menu_handle_advanced()     ─► 8 advanced parameter display
       menu_handle_advanced_edit() ─► increment/decrement 0-255 value
       menu_handle_split()        ─► Ch A and Ch B mode selection

menuStartRamp() / menuHandleRampUp() / menuIsRampActive()
  └─ ramp_counter 0→100, called every 30 ms from main loop
  └─ menuGetRampPercent() used by main loop to scale pulse width

menuStartOutput() / menuStopOutput() / menuIsOutputEnabled()

  [internal]
  get_battery_percent()
    └─ adc_read_battery() → clamp 584-676 → map to 0-100%
       584 = 0%  (≈10.6V), 676 = 100% (≈12.3V)
  cycle_mode(current, forward)
    └─ cycle_index() → skip unavailable user modes
  mode_is_available(mode)
    └─ user_prog_is_valid() for User1-7 slots; always true for built-in modes
  display_split_channel()
    └─ "Ch A: [mode]" or "Ch B: [mode]" on LCD
```

---

### mode_dispatcher.c — Mode Execution Engine

```
mode_dispatcher_init()
  └─ channel_mem_init()     Load defaults into channel_a and channel_b
  └─ param_engine_init()    Reset tick counter
  └─ user_programs_init()   Load all 7 user program slots from EEPROM
  └─ eeprom_load_split_modes() Restore split A/B mode selections

mode_dispatcher_select_mode(mode)
  └─ channel_load_defaults(&channel_a) and channel_b
  └─ init_mode_modules(mode)
       └─ switch(mode): execute_module(N) for each module in mode's setup list
  └─ For MODE_RANDOM1: random1_init()
  └─ For MODE_SPLIT:   init_split_mode()

mode_dispatcher_update()   [called every 8 ms]
  ├─ param_engine_tick()
  ├─ param_engine_check_module_trigger() → execute any triggered module
  ├─ step_gate_timer()      Advance gate on/off timer
  ├─ For MODE_RANDOM1: random1_tick()
  └─ copy_to_output()       channel_a/b → g_mk312bt_state

  [internal]
  execute_module(index)     Bytecode interpreter:
    ├─ Reads module from PROGMEM via module_table[index]
    ├─ apply_a = (channel_a.apply_channel & 0x01)
    ├─ apply_b = (channel_a.apply_channel & 0x02)
    └─ Per-opcode dispatch:
         0x00         END — stop execution
         0x20-0x3F    COPY — multi-byte write: [opcode][addr_hi][addr_lo][data...]
                        count = opcode & 0x1F (1-31 bytes)
                        writes to channel_get_reg_ptr(addr)
         0x40-0x4F    MEMOP — [opcode][channel_offset]
                        op = (opcode & 0x0C) >> 2
                        0=STORE to bank, 1=LOAD from bank, 2=DIV2, 3=RAND
         0x50-0x5F    MATHOP — [opcode][channel_offset][operand]
                        op = (opcode & 0x0C) >> 2
                        0=ADD, 1=AND, 2=OR, 3=XOR
         0x80+        SET — [opcode][value]
                        bit6: 0=route via apply_channel, 1=force ch B
                        bits5:0 = offset from 0x80 (ch A) or 0x180 (ch B)

  step_gate_timer()
    └─ if gate_select active: decrement timer
    └─ on expiry: toggle gate_phase → reload timer from gate_ontime/offtime
    └─ apply gate_value or 0 depending on phase

  copy_to_output()
    └─ g_mk312bt_state.channel_a_gate      = channel_a.gate_value
    └─ g_mk312bt_state.channel_a_intensity = channel_a.intensity_value
    └─ g_mk312bt_state.channel_a_freq      = channel_a.freq_value
    └─ g_mk312bt_state.channel_a_width     = channel_a.width_value
    └─ [same for channel B]

Mode → Module mapping:
  MODE_WAVES   (0x76): modules 11, 12
  MODE_STROKE  (0x77): modules 3, 4
  MODE_CLIMB   (0x78): modules 5, 8
  MODE_COMBO   (0x79): modules 13, 33
  MODE_INTENSE (0x7A): modules 14, 2
  MODE_RHYTHM  (0x7B): module 15
  MODE_AUDIO1  (0x7C): module 23 + gate flags
  MODE_AUDIO2  (0x7D): module 23 + gate flags
  MODE_AUDIO3  (0x7E): module 34
  MODE_RANDOM1 (0x7F): random1_init() → sub-mode switching
  MODE_RANDOM2 (0x80): module 32
  MODE_TOGGLE  (0x81): module 18
  MODE_ORGASM  (0x82): modules 24-27 (chained)
  MODE_TORMENT (0x83): module 28
  MODE_PHASE1  (0x84): modules 20, 35
  MODE_PHASE2  (0x85): modules 21, 35
  MODE_PHASE3  (0x86): modules 22, 35
  MODE_USER1-7 (0x87-0x8D): user_prog_execute(slot)
  MODE_SPLIT   (0x8E): init_split_mode() — independent A/B channels
```

---

### param_engine.c — Parameter Modulation Engine

```
param_engine_init()    Reset tick_counter = 0

param_engine_tick()    [called from mode_dispatcher_update, ~125 Hz]
  ├─ tick_counter++  (uint8_t, wraps 255→0)
  ├─ step_channel(&channel_a, ma_raw, cfg)
  ├─ step_channel(&channel_b, ma_raw, cfg)
  └─ step_next_module_timer() for both channels

  step_channel(ch, ma_raw, cfg)
    ├─ STEP_GROUP(ch, ma_raw, adv_ramp,      0x9C)  → ramp params
    ├─ STEP_GROUP(ch, ma_raw, adv_intensity,  0xA5)  → intensity params
    ├─ STEP_GROUP(ch, ma_raw, adv_frequency,  0xAE)  → frequency params
    └─ STEP_GROUP(ch, ma_raw, adv_width,      0xB7)  → width params

  step_param_group(ch, ma_raw, adv_val, value, mn, mx, rate, step, act_min, act_max, select, timer)
    ├─ if timer_should_fire(select) == false: return 0
    ├─ Resolve sweep_lo = *mn, sweep_hi = *mx (possibly scaled by MA or ADV)
    ├─ if sweep_lo <= sweep_hi: increment *value by *step
    │    if overflow: handle_action(*act_max, ...)
    └─ if sweep_lo > sweep_hi: decrement *value by *step
         if underflow: handle_action(*act_min, ...)

  handle_action(action, value, mn, mx, act_min, act_max, select, boundary_val, ch)
    ├─ 0xFF ACTION_REVERSE:  *value = boundary_val; swap(*mn,*mx); swap(*act_min,*act_max)
    ├─ 0xFE ACTION_REV_TOGGLE: REVERSE + toggle gate alt-polarity flag
    ├─ 0xFD ACTION_LOOP:     jump value to opposite end of range
    ├─ 0xFC ACTION_STOP:     *select &= ~TIMER_MASK (disable timer)
    └─ 0x00-0xDB MODULE_NUM: pending_module_X = action (triggers execute_module next tick)

  timer_should_fire(select)
    ├─ SEL_TIMER_244HZ: always return 1
    ├─ SEL_TIMER_30HZ:  return (tick_counter & 0x07) == 0   (~30 Hz)
    └─ SEL_TIMER_1HZ:   return tick_counter == 0            (~1 Hz)

  Select byte format:
    bits 1:0  = timer rate (00=static, 01=244Hz, 10=30Hz, 11=1Hz)
    bits 4:2  = min source (0=own field, 1=ADV param, 2=MA knob, 3=other ch, 4-7=inverted)
    bits 7:5  = rate source (0=own field, 1=ADV param, 2=MA knob, 3=other ch, 4-7=inverted)
```

---

### mode_programs.c — Bytecode Module Library

```
module_table[0..35]   PROGMEM pointers to 36 bytecode modules
module_sizes[0..35]   Size in bytes of each module

Module groups:
  0-2:   Gate control (OFF, ON, width parameters)
  3-4:   Stroke A/B — intensity modulation with reverse-toggle
  5-10:  Climb A/B — frequency sweep with chaining to next module
  11-12: Waves A/B — MA-controlled width/freq sweep
  13,33: Combo A/B — gate + freq/width control
  14:    Intense A — MA range adjustment
  15-17: Rhythm — next_module_timer, width toggle
  18-19: Toggle — alternating Ch A/Ch B gate
  20-22,35: Phase — phase shift control, intensity select
  23:    Audio — freq/width select for audio input modes
  24-27: Orgasm — width progression with module chaining
  28-31: Torment — random intensity and parameter variation
  32:    Random2 — random rate/step selection
  34:    Audio3 — audio mode with static frequency
```

---

### channel_mem.c — Channel Register Blocks

```
channel_mem_init()
  └─ channel_load_defaults(&channel_a)
  └─ channel_load_defaults(&channel_b)

channel_load_defaults(ch)
  └─ memcpy_P(ch, channel_defaults_P, sizeof(ChannelBlock))

channel_get_reg_ptr(addr)
  └─ 0x4080-0x40BF → &channel_a + (addr - 0x4080)
  └─ 0x4180-0x41BF → &channel_b + (addr - 0x4180)
  └─ else → &scratch_byte  (invalid address sink)

ChannelBlock layout (64 bytes, base = 0x80 for ch A, 0x180 for ch B):
  +00  unused_80, unused_81
  +02  retry_count, output_control_flags
  +04  cond_module, apply_channel
  +06  ma_range_min, ma_range_max
  +08  routine_timer_lo/mid/hi, routine_timer_slower
  +0C  bank, random_min, random_max
  +0F  audio_trigger_module
  +10  gate_value, gate_want_a, gate_want_b
  +13  unused_93, next_module_timer_cur, next_module_timer_max
  +16  next_module_select, next_module_number
  +18  gate_ontime, gate_offtime, gate_select, gate_transitions
  +1C  ramp_value, ramp_min, ramp_max, ramp_rate
  +20  ramp_step, ramp_action_min, ramp_action_max, ramp_select, ramp_timer
  +25  intensity_value, intensity_min, intensity_max, intensity_rate
  +29  intensity_step, intensity_action_min, intensity_action_max, intensity_select, intensity_timer
  +2E  freq_value, freq_min, freq_max, freq_rate
  +32  freq_step, freq_action_min, freq_action_max, freq_select, freq_timer
  +37  width_value, width_min, width_max, width_rate
  +3B  width_step, width_action_min, width_action_max, width_select, width_timer
```

---

### config.c — Runtime Configuration

```
config_init()                Set factory defaults
config_set_defaults()        Populate system_config_t with hardcoded defaults
config_load_from_eeprom()
  └─ eeprom_load_config() → if valid: config_sync_from_eeprom_config()
  └─ eeprom_load_split_modes()
config_get()                 Return &system_config  (singleton access)
config_sync_from_eeprom_config(ecfg)
  └─ Copy adv_ramp_level, adv_ramp_time, adv_depth, adv_tempo,
         adv_frequency, adv_effect, adv_width, adv_pace
config_apply_to_memory()
  └─ Write intensity/freq/width mod values to g_mk312bt_state channel fields

system_config_t fields (22 bytes):
  current_mode, power_level, split_mode, split_a_mode, split_b_mode
  intensity_a, intensity_b
  frequency_a, frequency_b
  width_a, width_b
  multi_adjust, audio_gain
  adv_ramp_level, adv_ramp_time, adv_depth, adv_tempo
  adv_frequency, adv_effect, adv_width, adv_pace
```

---

### eeprom.c — EEPROM Persistent Storage

```
mk312bt_eeprom_write_byte(addr, data)
  └─ Wait EEWE clear → EEAR=addr → EEDR=data → EEMWE=1 → EEWE=1

mk312bt_eeprom_read_byte(addr)
  └─ Wait EEWE clear → EEAR=addr → EERE=1 → return EEDR

eeprom_save_config(config)
  └─ Write magic (0xE3) → write 20 fields → write checksum (XOR of all 21 bytes)

eeprom_load_config(config)
  └─ Read all 22 bytes → validate magic + checksum → return 1 if OK, 0 if corrupt

eeprom_init_defaults(config)    Fill config with factory defaults + recalculate checksum

eeprom_save_split_modes(a, b)   Write at EEPROM 0x016, 0x017
eeprom_load_split_modes(*a, *b) Read from EEPROM 0x016, 0x017

eeprom_save_user_prog(slot, buf)  Write 32 bytes at user_prog_addr(slot)
eeprom_load_user_prog(slot, buf)  Read 32 bytes, return 1 if magic byte valid
eeprom_user_prog_valid(slot)      Check if slot[0] == USER_PROG_MAGIC (0xE3)
eeprom_erase_user_prog(slot)      Write 0xFF to all 32 bytes of slot

EEPROM Layout:
  0x000-0x015  eeprom_config_t (22 bytes) — magic, fields, checksum
  0x016        split_a_mode
  0x017        split_b_mode
  0x018-0x01F  reserved
  0x020-0x03F  User program slot 0 (32 bytes)
  0x040-0x05F  User program slot 1
  0x060-0x07F  User program slot 2
  0x080-0x09F  User program slot 3
  0x0A0-0x0BF  User program slot 4
  0x0C0-0x0DF  User program slot 5
  0x0E0-0x0FF  User program slot 6
```

---

### serial.c — MK-312BT Serial Protocol

```
serial_init()          Reset rx_index, expected_bytes, encryption state

serial_process()       [polled from main loop]
  ├─ if UCSRA.RXC: read UDR, decrypt if enabled
  ├─ Accumulate into rx_buffer[16]
  └─ When expected_bytes received:
       validate checksum → serial_handle_read/write/key_exchange_command()

serial_handle_read_command(addr)
  └─ serial_mem_read(addr) → send [0x22][value][checksum]

serial_handle_write_command(addr, length)
  └─ for i in 0..length: serial_mem_write(addr+i, rx_buffer[3+i])
  └─ send [0x06] OK

serial_handle_key_exchange(host_key)
  └─ box_key = prng_next()
  └─ serial_set_encryption_key(box_key, host_key)
  └─ send [0x21][box_key][checksum]

serial_set_encryption_key(box, host)
  └─ encryption_key = box ^ host ^ 0x55
  └─ encryption_enabled = true

serial_calculate_checksum(data, length)
  └─ sum bytes [0..length-2], return low byte

Protocol command dispatch (first byte, unencrypted):
  0x00  SYNC     — ignored
  0x08  RESET    — reset protocol state, send 0x06
  0x3C  READ     — 4 bytes total: [0x3C][addr_hi][addr_lo][csum]
  0xXD  WRITE    — (X+1) bytes: [cmd][addr_hi][addr_lo][data...][csum]
  0x2F  KEY_EXCH — 3 bytes: [0x2F][host_key][csum]
```

---

### serial_mem.c — Virtual Address Translation

```
serial_mem_read(address)
  ├─ 0x0000-0x00FF  → read_flash(address)    — ROM constants
  ├─ 0x4000-0x43FF  → read_ram(address)       — mapped SRAM
  └─ 0x8000-0x81FF  → read_eeprom_region()    — EEPROM

serial_mem_write(address, value)
  ├─ 0x4000-0x43FF  → write_ram(address, value)
  └─ 0x8000-0x81FF  → write_eeprom_region()

read_ram() address map:
  0x4064  ADC Level A (analogRead >> 2, read-only)
  0x4065  ADC Level B (analogRead >> 2, read-only)
  0x400F  POT_LOCKOUT_FLAGS
  0x407B  Current mode (mode index + 0x76 offset)
  0x41F4  Power level
  0x420D  Multi-Adjust value
  0x4080-0x40BF  channel_a struct (64 bytes)
  0x4180-0x41BF  channel_b struct (64 bytes)

write_ram() address map:
  0x4070  Box command:
    0x00 = reload current mode
    0x10 = next mode
    0x11 = previous mode
    0x12 = mode 0 (Waves)
  0x407B  Set current mode
  0x4080-0x40BF  Write channel_a fields
  0x4180-0x41BF  Write channel_b fields

read/write_eeprom_region():
  0x8000  Current mode (stored as mode + 0x76)
  0x8002  Power level
  0x800D-0x8014  Advanced settings (8 bytes)
  all others: raw mk312bt_eeprom_read/write_byte()
```

---

### user_programs.c — User Program Storage

```
user_programs_init()
  └─ for slot 0-6: eeprom_load_user_prog(slot, prog_cache[slot])

user_prog_is_valid(slot)
  └─ prog_cache[slot][0] == USER_PROG_MAGIC (0xE3)

user_prog_execute(slot)
  └─ Interpret SET opcodes (0x80+) only
  └─ Writes to channel_a or channel_b per apply_channel routing
  └─ Stops at opcode 0x00 or end of 32-byte buffer

user_prog_write(slot, buf)
  └─ memcpy to prog_cache[slot]
  └─ eeprom_save_user_prog(slot, buf)

user_prog_erase(slot)
  └─ memset 0xFF → eeprom_erase_user_prog(slot)

user_prog_read(slot, buf)
  └─ memcpy from prog_cache[slot]

User program format (32 bytes):
  [0]    0xE3  USER_PROG_MAGIC
  [1-30] SET bytecode: [0x80+offset][value] pairs
  [31]   0x00  end-of-program guard
```

---

### audio_processor.c — Audio Envelope Follower

```
audio_init()   no-op

audio_process_channel_a()
  ├─ raw = adc_read_audio_a()           (0-1023, PA7)
  ├─ level = |raw - 512|               (0-511, rectified)
  ├─ gain = config_get()->audio_gain   (0-255)
  ├─ scaled = (level * gain) / 255     (0-511, may saturate)
  ├─ clamp to 0-255
  └─ write to CHANNEL_A_INTENSITY_MOD

audio_process_channel_b()
  └─ same using adc_read_audio_b() (PA6) → CHANNEL_B_INTENSITY_MOD
```

---

### prng.c — PRNG

```
prng_init(seed)       state = (seed != 0) ? seed : 0xACE1

prng_next16()         state = (state * 1103515245UL + 12345UL) & 0xFFFF
                      return state

prng_next()           return (uint8_t)(prng_next16() >> 8)
```

---

### utils.c — Hardware Diagnostics

```
dacTest()
  └─ Send known SPI command → verify SPIF → return 1 OK / 0 fail

fetCalibrate()
  ├─ fets_all_off()
  ├─ dac_write_channel_a(FET_CAL_DAC_VALUE=128)
  ├─ adc_read_blocking() baseline (no pulse)
  ├─ for each FET pair (PB2/PB3, PB0/PB1):
  │    fet_test_pulse(pin_pos, pin_neg) → measure current delta
  └─ Store baselines in fet_baseline_a_pos/neg, fet_baseline_b_pos/neg

fetGetBaselineA() / fetGetBaselineB()  Return stored baseline values

adcReadOutputCurrent()   adc_read_blocking(PA0) — output current sense

  [internal]
  adc_read_blocking(channel)    Direct ADMUX/ADCSRA manipulation
  adc_read_current_avg()        3-sample average
  fet_test_pulse(pos, neg)      Short biphasic pulse, measure current
  fets_all_off()                PORTB &= ~0x0F
```

---

## Complete Function Call Graph

```
MK312BT.ino setup()
  ├── initializeHardware()
  ├── initialize_lcd()
  ├── serial_init()
  ├── prng_init()
  ├── dac_init()
  │   └── dac_wake() ──► dac_send_word() ──► dac_spi_transfer()
  ├── mode_dispatcher_init()
  │   ├── channel_mem_init()
  │   │   └── channel_load_defaults() ──► memcpy_P()
  │   ├── param_engine_init()
  │   ├── user_programs_init()
  │   │   └── eeprom_load_user_prog() ──► mk312bt_eeprom_read_byte()
  │   └── eeprom_load_split_modes() ──► mk312bt_eeprom_read_byte()
  ├── config_init()
  │   └── config_set_defaults()
  ├── config_load_from_eeprom()
  │   └── eeprom_load_config() ──► mk312bt_eeprom_read_byte()
  ├── dacTest()
  ├── fetCalibrate()
  │   ├── dac_write_channel_a() ──► dac_send_word()
  │   └── adc_read_blocking()
  ├── menuInit()
  │   └── lcd_create_char()
  ├── menuShowStartup()
  │   └── get_battery_percent() ──► adc_read_battery()
  ├── waitForAnyButton()
  │   └── lcd_enable_buttons() + digitalRead()
  └── pulse_gen_init()

MK312BT.ino loop()
  ├── wdt_reset()
  ├── serial_process()
  │   ├── serial_mem_read()
  │   │   ├── read_ram() ──► channel_get_reg_ptr(), config_get(),
  │   │   │                   mode_dispatcher_get_mode(), adc_read_level_*()
  │   │   └── read_eeprom_region() ──► mk312bt_eeprom_read_byte()
  │   └── serial_mem_write()
  │       ├── write_ram() ──► mode_dispatcher_select_mode(), config_get()
  │       └── write_eeprom_region() ──► mk312bt_eeprom_write_byte()
  ├── handleUserInput()  [every 20 ms]
  │   ├── lcd_enable_buttons()
  │   ├── menuHandleButton()
  │   │   ├── menu_handle_main()
  │   │   │   ├── mode_dispatcher_select_mode()
  │   │   │   │   ├── channel_load_defaults()
  │   │   │   │   └── execute_module() ──► channel_get_reg_ptr()
  │   │   │   ├── menuStartOutput()
  │   │   │   ├── mode_dispatcher_pause/resume()
  │   │   │   └── eeprom_save_config()
  │   │   ├── menu_handle_advanced() / menu_handle_advanced_edit()
  │   │   └── menu_handle_split()
  │   │       └── mode_dispatcher_set_split_modes()
  │   └── lcd_disable_buttons()
  ├── applyPowerLevel()
  ├── config_sync_from_eeprom_config()
  ├── runningLine1()  [every loop]
  │   ├── readAndUpdateChannel(0)
  │   │   ├── adc_read_level_a()
  │   │   └── dac_write_channel_a() ──► dac_send_word() ──► dac_spi_transfer()
  │   └── readAndUpdateChannel(1)
  │       ├── adc_read_level_b()
  │       └── dac_write_channel_b()
  ├── mode_dispatcher_update()  [every 8 ms]
  │   ├── param_engine_tick()
  │   │   ├── step_channel(&channel_a, ...)
  │   │   │   └── STEP_GROUP → step_param_group()
  │   │   │       └── handle_action() ──► pending_module trigger
  │   │   └── step_channel(&channel_b, ...)
  │   ├── param_engine_check_module_trigger()
  │   │   └── if triggered: execute_module()
  │   ├── step_gate_timer()
  │   └── copy_to_output() ──► writes g_mk312bt_state
  ├── audio_process_channel_a/b()  [if audio mode]
  │   └── adc_read_audio_a/b() ──► writes INTENSITY_MOD registers
  ├── pulse_set_width_a/b()      cli/sei double-buffer
  ├── pulse_set_frequency_a/b()  cli/sei double-buffer
  ├── pulse_set_gate_a/b()
  ├── menuShowMode()  [every 200 ms]
  │   ├── adc_read_level_a/b()
  │   ├── get_battery_percent() ──► adc_read_battery()
  │   └── lcd_set_cursor_raw() / lcd_write_string_raw() / lcd_write_custom_char_raw()
  └── menuHandleRampUp()  [every 30 ms if ramp active]

ISR(TIMER1_COMPA_vect)        [autonomous, ~244 Hz minimum]
  └── reads/writes pulse_ch_a fields
  └── writes OCR1A, PORTB (PB2/PB3)

ISR(TIMER2_COMP_vect)         [autonomous, ~244 Hz minimum]
  └── reads/writes pulse_ch_b fields
  └── writes OCR2, PORTB (PB0/PB1)

ISR(SPI_STC_vect)
  └── discards SPDR (prevents overrun)
```

---

## Data Structure Hierarchy

```
g_mk312bt_state  (MK312BTState, 15 bytes — output state read by main loop)
  channel_a_gate          0 or 1
  channel_a_intensity     0-255  (maps to DAC modulation)
  channel_a_freq          0-255  (maps to pulse period)
  channel_a_width         0-255  (maps to pulse width)
  channel_b_gate          0 or 1
  channel_b_intensity     0-255
  channel_b_freq          0-255
  channel_b_width         0-255
  multi_adjust            0-255  (MA knob reading)
  power_level             0-2
  output_control_flags
  pot_lockout_flags

channel_a / channel_b  (ChannelBlock, 64 bytes each — param engine workspace)
  [see channel_mem.c section above for full field list]

system_config  (system_config_t, 22 bytes — runtime settings)
  current_mode, power_level, split_mode, split_a_mode, split_b_mode
  intensity_a/b, frequency_a/b, width_a/b
  multi_adjust, audio_gain
  adv_ramp_level, adv_ramp_time, adv_depth, adv_tempo
  adv_frequency, adv_effect, adv_width, adv_pace

pulse_ch_a / pulse_ch_b  (ChannelPulseState, volatile — ISR shared)
  gate, width_ticks, period_ticks, phase, gap_remaining
  pending_width, pending_period, params_dirty

eeprom_config  (eeprom_config_t, 22 bytes — persistent storage)
  magic (0xE3), top_mode, favorite_mode, power_level
  intensity_a/b, frequency_a/b, width_a/b
  multi_adjust, audio_gain, split_mode
  adv_ramp_level, adv_ramp_time, adv_depth, adv_tempo
  adv_frequency, adv_effect, adv_width, adv_pace
  checksum (XOR of all preceding bytes)

prog_cache[7][32]  (user_programs.c — RAM cache of EEPROM slots)
  [0]    0xE3 USER_PROG_MAGIC
  [1-30] SET bytecode pairs
  [31]   0x00 end marker
```

---

## Hardware Register Map

```
PORT A — ADC Inputs (all inputs, analog)
  PA7  Audio A  (right line-in)
  PA6  Audio B  (left line-in / mic)
  PA5  Level pot B
  PA4  Level pot A
  PA3  Battery voltage (12V rail via divider)
  PA2  V+ measurement
  PA1  Multi-Adjust knob
  PA0  Output current sense

PORT B — H-bridge & SPI
  PB7  SCK  (SPI clock → DAC)
  PB6  MISO (not connected)
  PB5  MOSI (SPI data → DAC)
  PB4  SS   (not used as slave select)
  PB3  Ch A Gate−  (H-bridge FET, negative phase)
  PB2  Ch A Gate+  (H-bridge FET, positive phase)
  PB1  Ch B Gate−
  PB0  Ch B Gate+

PORT C — LCD / Buttons (multiplexed)
  PC7  LCD_DB7  /  Menu button  (PC0 selects mode)
  PC6  LCD_DB6  /  Up button
  PC5  LCD_DB5  /  OK button
  PC4  LCD_DB4  /  Down button
  PC3  LCD_RS   (register select: 0=command, 1=data)
  PC2  LCD_E    (enable strobe)
  PC1  LCD_RW   (always 0 = write)
  PC0  Mode select: LOW = LCD output, HIGH = button input (with pullups)

PORT D — USART, DAC CS, LEDs, Audio
  PD7  LCD backlight  (active low)
  PD6  Ch A activity LED
  PD5  Ch B activity LED
  PD4  DAC chip select  (active low, LTC1661)
  PD3  Audio switch 1  (MA / line-in right)
  PD2  Audio switch 2  (MA / line-in left+mic)
  PD1  USART TX
  PD0  USART RX

Timers:
  Timer0  General-purpose tick (millis() in Arduino framework)
  Timer1  Channel A pulse generator — 16-bit CTC, /8 prescaler, COMPA ISR
  Timer2  Channel B pulse generator — 8-bit CTC, /8 prescaler, COMP ISR

USART:
  Baud: 19200, 8N1
  RX polled in serial_process()

SPI (master):
  CPOL=0, CPHA=0, fosc/16
  Used exclusively for LTC1661 DAC
  DAC CS = PD4 (software controlled)

EEPROM:
  ATmega16 internal, 512 bytes
  Accessed via EEAR/EEDR/EECR registers
  Blocking reads/writes with EEWE ready-wait
```

---

## Timing Diagram

```
                   Main loop ~50 Hz (20 ms period, not fixed)
                   │
         ┌─────────┼─────────────────────────────────┐
         │         │                                  │
         │  wdt_reset()                               │
         │  serial_process()  ──────────── continuous poll
         │         │                                  │
         ├── every 20 ms ──────────────────────────── handleUserInput()
         │                                            button debounce + menu
         ├── every 8 ms ───────────────────────────── mode_dispatcher_update()
         │                                            param_engine_tick()
         │                                            ~125 Hz effective rate
         ├── every loop ─────────────────────────────  runningLine1()
         │                                            ADC + DAC update
         ├── every 200 ms ──────────────────────────── menuShowMode()
         │                                            LCD refresh
         └── every 30 ms (if ramp active) ──────────  menuHandleRampUp()

                   param_engine tick_counter  (uint8_t, wraps 0-255)
                   │
         ──────────┼──────────────────────────────────────────
         244 Hz    │ every tick   (tick_counter & 0x00) — all param groups with timer=244Hz
          30 Hz    │ every 8 ticks  (tick_counter & 0x07 == 0)
           1 Hz    │ every 256 ticks  (tick_counter == 0)

                   Timer1/Timer2 ISRs  (autonomous, hardware-driven)
                   │
         ──────────┼─────────────────────────────────────────────────────
                   │ PH_GAP        OCR = period - 2*width - 2*deadtime
                   │ PH_POSITIVE   OCR = width_ticks        (PB2=1, PB3=0)
                   │ PH_DEADTIME1  OCR = 4 µs               (PB2=0, PB3=0)
                   │ PH_NEGATIVE   OCR = width_ticks        (PB2=0, PB3=1)
                   │ PH_DEADTIME2  OCR = 4 µs               (PB2=0, PB3=0)
                   │ → back to PH_GAP
                   │
                   │ width range:  20-255 µs
                   │ period range: 500-65535 µs  (15 Hz - 2 kHz)
```

---

## Data Flow Descriptions

### User Input → Mode Change

```
Button press
  → handleUserInput() (20 ms poll)
  → menuHandleButton() → menu_handle_main()
  → cycle_mode() — skip unavailable user modes
  → mode_dispatcher_select_mode(new_mode)
      → channel_load_defaults() both channels
      → execute_module(N) for mode's setup modules
         → channel_get_reg_ptr() translates addresses
         → writes intensity/freq/width/gate fields in ChannelBlock
  → mode_dispatcher_update() on next 8 ms tick
      → param_engine_tick() begins sweeping the new parameters
```

### Parameter Sweep Cycle

```
mode_dispatcher_update() every 8 ms
  → param_engine_tick()
      → step_channel(channel_a)
          → for each group (ramp/intensity/freq/width):
              → timer_should_fire(select)  — check rate
              → resolve sweep bounds (MA-scaled or ADV-scaled)
              → increment/decrement value by step
              → if at boundary: handle_action()
                  → REVERSE: swap min/max and action codes
                  → LOOP: jump to opposite end
                  → STOP: disable timer
                  → MODULE(n): set pending_module_a = n
      → step_channel(channel_b)   same for Ch B
  → if pending_module_a set: execute_module(n) — reconfigure ch A
  → step_gate_timer() — toggle gate on/off
  → copy_to_output()
      → g_mk312bt_state.channel_a_intensity = channel_a.intensity_value
      → g_mk312bt_state.channel_a_freq      = channel_a.freq_value
      → g_mk312bt_state.channel_a_width     = channel_a.width_value
      → g_mk312bt_state.channel_a_gate      = channel_a.gate_value
      [same for ch B]
  ↓
  main loop reads g_mk312bt_state
  → pulse period = freq_value * 256 µs  (range: 2048-65280 µs = 15-488 Hz)
  → pulse width  = map(width_value, 0-255, 20-200 µs) × ramp_percent/100
  → pulse_set_frequency_a(period_us)  — cli/sei, pending_period, dirty=1
  → pulse_set_width_a(width_us)       — cli/sei, pending_width, dirty=1
  → pulse_set_gate_a(gate)
  ↓
  ISR(TIMER1_COMPA_vect) — next compare match
  → if params_dirty: copy pending_period/width → period_ticks/width_ticks
  → drive PB2/PB3 per phase state machine
  → update OCR1A for next phase duration
```

### Serial Host Control

```
Host PC (e.g. ErosTek PC software or custom script)
  → RS-232 at 19200 baud
  → serial_process() polls USART RXC flag
  → Decrypt bytes (XOR key, after key exchange)
  → Accumulate into rx_buffer[16]
  → On packet complete: validate checksum
  → READ 0x3C: serial_mem_read(addr) → 0x22 reply
  → WRITE 0xND: serial_mem_write(addr, data) × N bytes → 0x06 OK
  → KEY_EXCHANGE 0x2F: send box_key, derive session key
  ↓
  serial_mem_write() virtual address map:
    0x4080-0x40BF → channel_a fields  (live parameter control)
    0x4180-0x41BF → channel_b fields
    0x4070        → box command (mode change, reload)
    0x8000+       → EEPROM (persistent config)
```

### DAC Intensity Path

```
Level pot A (PA4)
  → analogRead() 0-1023 = v
  → dac_val = ChannelAPwrBase + (ChannelAModulationBase × (1023 - v)) / 1024
      Power level Normal:  base=590, mod=330
      v=0   (pot minimum) → dac_val = 590 + 330 = 920 (low output, DAC inverted)
      v=1023 (pot maximum) → dac_val = 590 +   0 = 590 (higher output)
  → if intensity_mod active:
      headroom = 1023 - dac_val
      dac_val += headroom × (255 - int_mod) / 255
  → clamp to 1023
  → dac_write_channel_a(dac_val)  SPI → LTC1661
  → LTC1661 VOUT_A → power supply voltage → transformer → output amplitude
```

---

## Memory Map

### Flash (16 KB)

```
0x0000-0x0050   Interrupt vector table
0x0050-0x03FF   Initialization and main loop (MK312BT.ino compiled)
0x0400-0x17FF   Module drivers (lcd, dac, adc, pulse_gen, serial, etc.)
0x1800-0x1FFF   Mode dispatcher, param engine, channel mem
0x2000-0x27FF   Mode programs bytecode (PROGMEM in mode_programs.c)
0x2800-0x37FF   Menu system, config, eeprom, user programs
0x3800-0x3DFF   String tables (PROGMEM: mode names, menu strings)
0x3E00-0x3FFF   Bootloader
```

### SRAM (1 KB, 0x0060-0x045F)

```
0x0060-0x00FF   AVR register file + stack area
0x0100-0x01FF   Global variables:
                  g_mk312bt_state (15 bytes)
                  g_menu_config (22 bytes)
                  channel_a (64 bytes)
                  channel_b (64 bytes)
                  system_config (22 bytes)
                  pulse_ch_a (12 bytes, volatile)
                  pulse_ch_b (12 bytes, volatile)
                  prog_cache (7×32 = 224 bytes)
                  rx_buffer (16 bytes)
                  line_buffer/line_buffer2 (17+17 bytes)
0x0200-0x045F   Stack (grows downward from 0x045F)
```

### EEPROM (512 bytes)

```
0x000-0x015   eeprom_config_t (22 bytes) — magic 0xE3, settings, XOR checksum
0x016         split_a_mode
0x017         split_b_mode
0x018-0x01F   reserved (8 bytes)
0x020-0x03F   User program slot 0 (32 bytes)
0x040-0x05F   User program slot 1
0x060-0x07F   User program slot 2
0x080-0x09F   User program slot 3
0x0A0-0x0BF   User program slot 4
0x0C0-0x0DF   User program slot 5
0x0E0-0x0FF   User program slot 6
0x100-0x1FF   unused
```

### Serial Virtual Address Space

```
0x0000-0x00FF   Flash ROM (read-only)
                  0x00FC: BOX_MODEL = 0x0C
                  0x00FD: FW_VER_MAJ = 0x01
                  0x00FE: FW_VER_MIN = 0x06
                  0x00FF: FW_VER_INT = 0x00
0x4000-0x43FF   RAM (read/write, mapped to SRAM state)
                  0x4064: ADC Level A (read-only)
                  0x4065: ADC Level B (read-only)
                  0x400F: POT_LOCKOUT_FLAGS
                  0x407B: Current mode
                  0x4070: Box command register
                  0x4080-0x40BF: channel_a (64 bytes)
                  0x4180-0x41BF: channel_b (64 bytes)
                  0x41F4: Power level
                  0x420D: Multi-Adjust value
0x8000-0x81FF   EEPROM (read/write, raw EEPROM access)
```

---

## Serial Protocol Reference

### Handshake

```
1. Host sends 0x00 (up to 12 times)
2. Device replies 0x07 when ready
3. Host sends: [0x2F] [host_key] [checksum]
4. Device replies: [0x21] [box_key] [checksum]
5. Session key = box_key ^ host_key ^ 0x55
6. All subsequent host→device bytes (except first byte of packet) XOR-encrypted
```

### Commands

```
READ     [0x3C] [addr_hi] [addr_low] [csum]
  Reply: [0x22] [value]   [csum]

WRITE 1  [0x4D] [addr_hi] [addr_low] [data]        [csum]
WRITE 2  [0x5D] [addr_hi] [addr_low] [data] [data] [csum]
WRITE N  [0x(N+3)D] [addr_hi] [addr_low] [data×N]  [csum]
  Reply: [0x06]  (success)
         [0x07]  (checksum error)

RESET    [0x08]
  Reply: [0x06]
```

### Checksum

```
Sum all bytes except the checksum byte itself.
Return low byte (sum & 0xFF).
```
