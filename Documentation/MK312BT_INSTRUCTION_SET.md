# MK-312BT Bytecode Instruction Set Architecture

## Overview

The MK-312BT firmware uses a custom bytecode interpreter to execute programs called "modules". Each mode consists of one or more modules that execute sequentially, manipulating memory addresses to control output channels, timing, and transitions.

Based on reverse engineering from the three-twelve-bee project (https://github.com/fenbyfluid/three-twelve-bee).

## Memory Architecture

- **10-bit address space**: 0x000 - 0x3FF (1024 bytes)
- **Mirrored channels**: Channel A and Channel B have parallel memory structures
- **Key addresses**:
  - 0x0084: Module jump trigger (value - 0xC0 = target module)
  - 0x0085: Channel control bits (0x01=A, 0x02=B, 0x03=both)
  - 0x008C: Channel A bank register
  - 0x018C: Channel B bank register
  - 0x008D: Random minimum bound
  - 0x008E: Random maximum bound

## Instruction Format

Instructions are variable-length byte sequences:
- Byte 0: Opcode (determines instruction type and operand count)
- Bytes 1+: Operands (address, values, etc.)

## Opcode Categories

### 1. END (0x00 pattern)

**Bit Pattern**: `(opcode & 0xE0) == 0x00`

**Format**: `[0x00]`

**Operation**: Signals end of module execution

**Example**: `[0x00]`

---

### 2. COPY (0x20 pattern)

**Bit Pattern**: `(opcode & 0xE0) == 0x20`

**Format**: `[opcode, addr_low, value1, value2, ...]`

**Address Calculation**:
```c
address = ((opcode & 0x03) << 8) | addr_low
```

**Length Calculation**:
```c
length = 1 + ((opcode & 0x1C) >> 2)
```

**Operation**: Copies 1-7 bytes to consecutive memory addresses starting at `address`

**Examples**:
- `[0x28, 0x86, 0x08, 0x52]` - Copy 2 bytes (0x08, 0x52) to address 0x086

---

### 3. MEMORY OPS (0x40 pattern)

**Bit Pattern**: `(opcode & 0xF0) == 0x40`

**Format**: `[opcode, addr_low]`

**Address Calculation**:
```c
address = ((opcode & 0x03) << 8) | addr_low
```

**Operation Type** (bits 2-3):
```c
operation = (opcode & 0x0C) >> 2
```

**Operations**:
- **0: STORE** - Copy value from address to bank register
- **1: LOAD** - Copy value from bank register to address
- **2: DIV2** - Right shift address value by 1 (divide by 2)
- **3: RAND** - Generate random value between bounds (0x8D-0x8E), store at address

**Examples**:
- `[0x40, 0x8C]` - Store value at 0x08C to bank
- `[0x44, 0x8C]` - Load bank value to 0x08C
- `[0x48, 0x90]` - Divide value at 0x090 by 2
- `[0x4C, 0x95]` - Generate random, store at 0x095

---

### 4. MATH OPS (0x50 pattern)

**Bit Pattern**: `(opcode & 0xF0) == 0x50`

**Format**: `[opcode, addr_low, value]`

**Address Calculation**:
```c
address = ((opcode & 0x03) << 8) | addr_low
```

**Operation Type** (bits 2-3):
```c
operation = (opcode & 0x0C) >> 2
```

**Operations**:
- **0: ADD** - Add value to memory[address]
- **1: AND** - Bitwise AND of memory[address] with value
- **2: OR** - Bitwise OR of memory[address] with value
- **3: XOR** - Bitwise XOR of memory[address] with value

**Examples**:
- `[0x50, 0xB5, 0x10]` - Add 0x10 to value at 0x0B5
- `[0x54, 0xB5, 0xE3]` - AND value at 0x0B5 with 0xE3
- `[0x58, 0xB5, 0x08]` - OR value at 0x0B5 with 0x08
- `[0x5C, 0xB5, 0xFF]` - XOR value at 0x0B5 with 0xFF

---

### 5. CONDITIONAL EXECUTE (0x10 bit set)

**Bit Pattern**: `(opcode & 0x10) == 0x10` (and not matching COPY, MEMOP, or MATHOP)

**Format**: `[opcode, addr_low]` (2 bytes)

**Status**: Parsed and skipped (2-byte instruction consumed) but not executed in current implementation. Reserved for future use or original firmware compatibility.

---

### 6. SET (0x80 pattern)

**Bit Pattern**: `(opcode & 0x80) == 0x80`

**Format**: `[opcode, value]`

**Address Calculation**:
```c
address = 0x80 + (opcode & 0x3F)
force_channel_b = (opcode & 0x40) != 0
if (force_channel_b) {
    address |= 0x100  // Set bit 8 for channel B
}
```

**Address Range**: Limited to 0x80-0xBF (and 0x180-0x1BF for channel B)

**Operation**: Set memory[address] = value

**Examples**:
- `[0x85, 0x03]` - Set address 0x085 to 0x03
- `[0xC5, 0x03]` - Set address 0x185 to 0x03 (channel B forced)

---

## Module Execution Model

1. **Module Start**: A module executes **once** when called by `execute_module(n)` in `mode_dispatcher.c`. There is no "execute for A then B" — the `apply_channel` field at address 0x085 (bits: 0x01=A, 0x02=B, 0x03=both) controls which channel's registers are written by SET and MATHOP opcodes.

2. **Instruction Loop**: Instructions execute sequentially until the END opcode (0x00 or end of program buffer).

3. **Module Chaining**: Module transitions happen through the parameter engine boundary actions. When a parameter sweep reaches its min or max boundary and the `action_min`/`action_max` field contains a value < 0xFC, the param engine triggers that module number. The module runs in the next `mode_dispatcher_update()` call.

4. **Channel Control**: Address 0x085 (`apply_channel`) determines which channels are affected by SET (0x80+) and MATHOP (0x50+) opcodes. The COPY opcode always writes to the explicit address. Channel B SET opcodes use `opcode | 0x40` to force channel B addressing.

5. **Memory Addressing**: Addresses 0x080-0x0BF map to `channel_a` fields. Addresses 0x180-0x1BF map to `channel_b` fields. The `channel_get_reg_ptr()` function performs this translation.

## Decoding Algorithm

```c
uint8_t decode_instruction_length(const uint8_t* bytecode) {
    uint8_t opcode = bytecode[0];

    // END
    if ((opcode & 0xE0) == 0x00) return 1;

    // COPY
    if ((opcode & 0xE0) == 0x20) {
        uint8_t length = 1 + ((opcode & 0x1C) >> 2);
        return 2 + length;  // opcode + address + values
    }

    // MEMORY OPS
    if ((opcode & 0xF0) == 0x40) return 2;

    // MATH OPS
    if ((opcode & 0xF0) == 0x50) return 3;

    // CONDITIONAL EXECUTE
    if ((opcode & 0x10) == 0x10) return 2;

    // SET
    if ((opcode & 0x80) == 0x80) return 2;

    return 1;  // Unknown/invalid
}
```

## Implementation Notes

- Instructions are packed sequentially in module bytecode
- No alignment requirements
- Maximum module size: ~256 bytes (typical)
- Modules can be chained via conditional execute
- Bank register serves as accumulator for comparisons
