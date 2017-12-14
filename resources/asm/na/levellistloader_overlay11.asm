; For use with ARMIPS v0.7d
; By: psycommando@gmail.com
; 2016/08/16
; ------------------------------------------------------------------------------
; Copyright © 2016 Guillaume Lavoie-Drapeau <psycommando@gmail.com>
; This work is free. You can redistribute it and/or modify it under the
; terms of the Do What The Fuck You Want To Public License, Version 2,
; as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
; ------------------------------------------------------------------------------
.relativeinclude on
.nds
.arm

;-------------------------------------
; Hook 022F134C
;-------------------------------------
  .org 0x22DE56C
  .area 24  ;We have 24 bytes here
    ldrsh   r3,[r0,8h]
    mov     r0,r3
    bl LevelListSecondHWORDGet;ldr     r2,=20A548Ah
    mov     r2,r0
    mov     r0,0h ;smulbb  r1,r3,r1
    nop ;ldrsh   r2,[r2,r1]
  .endarea

;-------------------------------------
; Hook 022F134C
;-------------------------------------
  .org 0x22F134C
  .area 0x14  ;We have 20 bytes here
    ;Prepare the parameter for the accessor
    mov     r0,r5               ;022F134C E3A0000C mov     r0,0Ch
    bl      LevelListAccessor   ;022F1350 E1630085 smulbb  r3,r5,r0
    mov     r4,r0               ;022F1354 E59F41E4 ldr     r4,=20A5488h
    ldrsh   r0,[r4]             ;022F1358 E19400F3 ldrsh   r0,[r4,r3]
    nop                         ;022F135C E0844003 add     r4,r4,r3
  .endarea
  .org 0x22F1540         ;We need to modify the address here, to use it for our bl above!
  .area 4
    .pool
  .endarea

;-------------------------------------
; Hook 022F1600
;-------------------------------------
  .org 0x22F1600
  .area 0x14
    ;Prepare the parameter for the accessor
    mov     r0,r8                 ;022F1600 E3A0000C mov     r0,0Ch
    bl      LevelListAccessor     ;022F1604 E1610088 smulbb  r1,r8,r0
    mov     r4,r0                 ;022F1608 E59F20EC ldr     r2,=20A5488h
    ldrsh   r0,[r4]               ;022F160C E19200F1 ldrsh   r0,[r2,r1]
    nop                           ;022F1610 E0824001 add     r4,r2,r1
  .endarea
  .org 0x22F16FC
  .area 4
    .pool
  .endarea

  ;-------------------------------------
  ; Hook 022F1758
  ;-------------------------------------
  .org 0x22F1758
  .area 24
    mov     r0,r4               ;022F1758 E59F1034 ldr     r1,=20A5488h   //<-change       //Level list beg
    ldr     r3,[r2,4h]          ;!! Don't touch !!
    bl      LevelListAccessor   ;022F1760 E3A0000C mov     r0,0Ch
    strh    r4,[r3]             ;!! Don't touch !!
    ldrsh   r1,[r0,4h]          ;022F1768 E1001084 smlabb  r0,r4,r0,r1
    nop                         ;022F176C E1D010F4 ldrsh   r1,[r0,4h]
  .endarea
  .org 0x22F1794
  .area 4
    .pool
  .endarea

  ;-------------------------------------
  ; Hook 0x22F17B4 - This one caused misalignment in the stack!!
  ;-------------------------------------
  .org 0x22F17B4
  .area (0x22F17E4 - 0x22F17B4)
    push r14                  ;022F17B4 E59F0024 ldr     r0,=2324CC0h
    ldr     r0,=2324CC0h       ;022F17B8 E5900004 ldr     r0,[r0,4h]
    ldr     r0,[r0,4h]         ;022F17BC E3500000 cmp     r0,0h
    cmp     r0,0h              ;022F17C0 03E00000 mvneq   r0,0h
    mvneq   r0,0h              ;022F17C4 012FFF1E bxeq    r14
    popeq   r15                ;022F17C8 E1D020F0 ldrsh   r2,[r0]

    ldrsh   r0,[r0]           ;022F17CC E3A0000C mov     r0,0Ch
    bl      LevelListAccessor ;022F17D0 E59F100C ldr     r1,=20A5488h
    ldrsh   r0,[r0]           ;022F17D4 E1600082 smulbb  r0,r2,r0
    pop     r15               ;022F17D8 E19100F0 ldrsh   r0,[r1,r0]
    .pool                     ;022F17DC E12FFF1E bx      r14
    ;.fill   (0x22F17B4 + 0x22F17E4) - . ;022F17E0 02324CC0 eoreqs  r4,r2,0C000h
    ;022F17E4 020A5488 andeq   r5,r10,88000000h
  .endarea

  ;-------------------------------------
  ; Hook 022F1F68
  ;-------------------------------------
  .org 0x22F1F68
  .area 20
    nop ;mov     r0,r4             ;022F1F68 E3A0300C mov     r3,0Ch
    mov     r1,r7             ;!! Don't Touch !!
    mov     r2,r6             ;!! Don't Touch !!
    bl      LevelListCustomHook022F1F68 ;022F1F74 E1640384 smulbb  r4,r4,r3 ;;<== Custom hook for this one since we don't have much space, and a lot of registers are in use!
    nop ;022F1F78 E59F50C8 ldr     r5,=20A5488h ;;;Put the result into r4 since r0 gets overwritten several times after, not r4 and r5
  .endarea
  .org 0x22F1FD8
  .area 4
    ldrsh   r0,[r4]           ;022F1FD8 E19500F4 ldrsh   r0,[r5,r4]
  .endarea
  .org 0x22F2048
  .area 4
    .pool
  .endarea

  ;-------------------------------------
  ; Hook 022FF9FC
  ;-------------------------------------
  .org 0x22FF9FC
  .area 24
    push r14
    mov     r0,r1                 ;022FF9FC E59F200C ldr     r2,=20A5488h
    bl      LevelListAccessor     ;022FFA00 E3A0000C mov     r0,0Ch
    ldr     r0,[r0,8h]            ;022FFA04 E0202091 mla     r0,r1,r0,r2
    ;bx      r14                   ;022FFA08 E5900008 ldr     r0,[r0,8h]
    pop r15
    .pool                         ;022FFA0C E12FFF1E bx      r14
    ;022FFA10 020A5488 andeq   r5,r10,88000000h
  .endarea

  ;-------------------------------------
  ; Hook 0x2310108
  ;-------------------------------------
  .org 0x2310108
  .area 20
    mov     r0,r5             ;02310108 E3A0000C mov     r0,0Ch
    bl      LevelListAccessor ;0231010C E1610085 smulbb  r1,r5,r0
    mov     r4,r0             ;02310110 E59F3210 ldr     r3,=20A5488h
    ldrsh   r0,[r4]           ;02310114 E19300F1 ldrsh   r0,[r3,r1]
    nop                       ;02310118 E0834001 add     r4,r3,r1
  .endarea
  .org 0x2310328
  .area 4
    .pool
  .endarea

  ;-------------------------------------
  ; Hook 0x2310E1C
  ;-------------------------------------
  .org 0x2310E1C
  .area 4
    nop     ;02310E1C E3A0000C mov     r0,0Ch        //<-change
  .endarea
  ;02310E20 E2811902 add     r1,r1,8000h
  ;02310E24 E5861000 str     r1,[r6]
  ;02310E28 E5962004 ldr     r2,[r6,4h]
  .org 0x2310E2C
  .area 4
    nop     ;02310E2C E1610084 smulbb  r1,r4,r0      //<-change
  .endarea
  ;02310E30 E2820A06 add     r0,r2,6000h
  ;02310E34 E5860004 str     r0,[r6,4h]
  ;02310E38 E5952000 ldr     r2,[r5]
  .org 0x2310E3C
  .area 4
    mov     r0,r4  ;02310E3C E59F0080 ldr     r0,=20A5488h  //<-change
  .endarea
  ;02310E40 E2422902 sub     r2,r2,8000h
  ;02310E44 E5852000 str     r2,[r5]
  ;02310E48 E5952004 ldr     r2,[r5,4h]
  .org 0x2310E4C
  .area 4
    bl    LevelListFirstHWORDGet ;02310E4C E19000F1 ldrsh   r0,[r0,r1]    //<-change
  .endarea
  .org 0x2310EC4
  .area 4
    .pool
  .endarea

  ;-------------------------------------
  ; Hook 0x231110C
  ;-------------------------------------
  .org 0x231110C
  .area 20
    mov     r0,r5             ;0231110C E3A0000C mov     r0,0Ch
    bl      LevelListAccessor ;02311110 E1610085 smulbb  r1,r5,r0
    mov     r4,r0             ;02311114 E59F315C ldr     r3,=20A5488h
    ldrsh   r0,[r4]           ;02311118 E19300F1 ldrsh   r0,[r3,r1]
    nop                       ;0231111C E0834001 add     r4,r3,r1
  .endarea
  .org 0x02311278
  .area 4
    .pool
  .endarea

  ;-------------------------------------
  ; Hook 0x23125D4
  ;-------------------------------------
  .org 0x23125D4
  .area 20
    mov     r0,r5             ;023125D4 E3A0000C mov     r0,0Ch
    bl      LevelListAccessor ;023125D8 E1610085 smulbb  r1,r5,r0
    mov     r4,r0             ;023125DC E59F3138 ldr     r3,=20A5488h
    ldrsh   r0,[r4]           ;023125E0 E19300F1 ldrsh   r0,[r3,r1]
    nop                       ;023125E4 E0834001 add     r4,r3,r1
  .endarea
  .org 0x0231271C
  .area 4
    .pool
  .endarea
