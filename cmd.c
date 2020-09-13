
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <im2.h>

#include "common.h"
#include "timing.h"
#include "irq.h"

#define _CMD_C_INCLUDED
#include "cmd.h"

uchar *p_keys_map, *p_toggle_keys_map;

uchar cmds_head = 0, cmds_tail = 0;

uchar cmds[MAX_CMDS];

uchar cmd_toggle = 0;

uchar cmd_prev_toggle_keys = 0;

uchar cmds_get_next()
{
    uchar cmd;

    cmd = 0;
    if (cmds_head != cmds_tail)
    {
        cmd = cmds[cmds_head];
        cmds_head = (cmds_head + 1) & CMDS_SIZE_MASK;
    }

    return cmd;
}

void cmds_put(uchar cmd)
{
    uchar n;

    n = (cmds_tail + 1) & CMDS_SIZE_MASK;

    // If ring buffer not full
    if (cmds_head != n)
    {
        cmds[cmds_tail] = cmd;
        cmds_tail = n;
    }
}

uchar cmd_toggle_is_enabled(uchar cmd_t)
{
    return (cmd_toggle & cmd_t) != 0 ? 1 : 0;
}

void cmd_toggle_set(uchar cmd_t, uchar enable)
{
    cmd_toggle = enable ? (cmd_toggle | cmd_t) : (cmd_toggle & ~cmd_t);
}

uchar cmd_toggle_snapshot_update(struct t_cmd_toggle_snapshot *snapshot, uchar cmd_t)
{
    snapshot->prev_state = snapshot->state;
    snapshot->state = cmd_toggle_is_enabled(cmd_t);

    return ((snapshot->prev_state != 0) && (snapshot->state == 0) ||
            (snapshot->prev_state == 0) && (snapshot->state != 0));
}

/*
 * Reads the command that is specified by the state of the keyboard
 */
uchar cmd_read_kbd(void);

#asm
._cmd_read_kbd
    ld a,(_p_keys_map)
    ld l,a
    ld a,(_p_keys_map+1)
    ld h,a
    ld d,0
    ld c,$FE            ; low byte for IO port = 0xFE for ULA

_cmd_read_kbd_1:
    ld a,(hl)            ; read A15-A8 byte of address
    and a
    jr z,_cmd_read_kbd_exit
    ld b,a
    in a,(c)            ; key state bits of a row
    inc hl
    and (hl)            ; and with mask
    jr nz,_cmd_read_kbd_next
    ; key pressed
    ld a,d
    inc hl
    or (hl)                ; or with cmd
    ld d,a
    inc hl
    jp _cmd_read_kbd_1

_cmd_read_kbd_next:
    ; key not pressed
    inc hl
    inc hl
    jp _cmd_read_kbd_1

_cmd_read_kbd_exit:
    ld l,d
    ret
#endasm

/*
 * Reads the state of toggle keys and toggles the commands whose keys were pressed
 * (where transition from off to on happened)
 */
void cmd_read_kbd_toggle_state(void);

#asm
._cmd_read_kbd_toggle_state
    ld a,(_p_toggle_keys_map)
    ld l,a
    ld a,(_p_toggle_keys_map+1)
    ld h,a
    ld de,1                ; D - current state of toggle keys, E - current bit
    ld c,$FE            ; low byte for IO port = 0xFE for ULA

_cmd_read_kbd_toggle_1:
    ld a,(hl)            ; read A15-A8 byte of address
    and a
    jr z,_cmd_read_kbd_toggle_exit
    ld b,a
    in a,(c)            ; key state bits of a row
    inc hl
    and (hl)            ; and with mask
    jr nz,_cmd_read_kbd_toggle_next

    ; key pressed
    ld a,d
    or e                ; or with cmd
    ld d,a

    ; does it represent a change from prev state?
    ld a,(_cmd_prev_toggle_keys)
    and e
    jr nz,_cmd_read_kbd_toggle_next

    ; prev state was not pressed, current state pressed - toggle the bit
    ld a,(_cmd_toggle)
    xor e
    ld (_cmd_toggle),a

_cmd_read_kbd_toggle_next:
    ; key not pressed
    inc hl
    sla e
    jp _cmd_read_kbd_toggle_1

_cmd_read_kbd_toggle_exit:
    ld a,d
    ld (_cmd_prev_toggle_keys),a
    ret
#endasm

extern void cmd_isr(void);
M_BEGIN_ISR(cmd_isr)
{
    #asm
    rst $38       ; Keep the keyboard and FRAMES working by running also a standard ISR

    ; Read the command keys
    call _cmd_read_kbd
    xor a
    cp l
    jr z,_cmd_isr_no_enqueue

    ; If one or more is pressed, enqueue to cmd queue
    push hl
    call _cmds_put
    pop hl

_cmd_isr_no_enqueue:

#ifndef NDEBUG
    ; display cmd bits on screen as pixels in row 0
    ld a,l
    ld hl,16384+26
    ld (hl),a
#endif

    ; Update the toggle key states
    call _cmd_read_kbd_toggle_state

#ifndef NDEBUG
    ; display toggle bits on screen as pixels
    ld a,(_cmd_toggle)
    ld hl,16384+27
    ld (hl),a
#endif

._cmd_isr_exit

   #endasm
}
M_END_ISR

void cmd_init(uchar *a_keys_map, uchar *a_toggle_keys_map)
{
    p_keys_map = a_keys_map;
    p_toggle_keys_map = a_toggle_keys_map;

    irq_set_isr(cmd_isr);
}
