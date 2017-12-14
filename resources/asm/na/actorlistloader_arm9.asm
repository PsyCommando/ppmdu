; For use with ARMIPS v0.7d
; By: psycommando@gmail.com
; 2016/09/17
; For Explorers of Sky North American ONLY!
; ------------------------------------------------------------------------------
; Copyright © 2016 Guillaume Lavoie-Drapeau <psycommando@gmail.com>
; This work is free. You can redistribute it and/or modify it under the
; terms of the Do What The Fuck You Want To Public License, Version 2,
; as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
; ------------------------------------------------------------------------------
;Meant to be included inside the arm9.bin file!
.relativeinclude on
.nds
.arm

.definelabel ActorEntryLen, 12

;Beginning of the npc table inside the arm9.bin of the NA Explorers of sky game
.org 0x020A6910
.area 0x20A9208 - .

;Place marker to indicate PPMD applied a mod! ;Place a marker to indicate that we applied the hack
  .ascii "PATCH PPMD ActorLoader 0.1"
  dcb 0
  .align 4

;Uncomment desired method of access to the file's content
  ;.include "actor_accessor_fstream.asm" ;;;<============================== Because of the way actor function access the actor data, filestreams are not available!!!!
  .include "actor_accessor_sir0.asm"

;Accessor, returns address entry specified (r0 = npc id) returns in r0 the address of the entry
  ActorAccessor:
    push r1,r2,r14
      mov     r2,r0                   ;Save actor id in r2
      bl      GetActorListAddress
      mov     r1,ActorEntryLen        ;Each entries in the table is 12 bytes long
      smulbb  r1,r2,r1                ;Multiply by the size of a table entry
      add     r0,r0,r1                ;set it to the correct entry
    pop r1,r2,r15
    .pool
    .align 4
;END ActorAccessor

;Custom hook for function 0206B24
  ActorListCustomHook0206B24:
    push r0,r14
      bl      GetActorListAddress
      mov     r5,r0
      bl      GetNbActors
      mov     r4,r0
      ;mov r0,r6 ;Get the counter as actor id
      ;bl ActorAccessor
      ;mov r5,r0 ;put the address into r5, r0 gets overwritten after so we don't care
      ;ldr r1,[r5,4h]
    pop r0,r15
  .pool
  .align 4
;END ActorAccessor

;Custom hook for function 0206549C (r1=actorid,) returns in r2 the first short, and in r3 the address of the entry!
  ActorListCustomHook0206549C:
    push r0,r12,r14
      mov     r0,r1                 ;Get the actor id into r0
      mov     r12,r0                ;Save entry id in r12!
      bl      GetActorListAddress
      mov     r1,ActorEntryLen      ;Each entries in the table is 12 bytes long
      smulbb  r1,r12,r1             ;Multiply entry id by the size of a table entry
      add     r3,r0,r1              ;Place the entry's address into r3
      ldrsh   r2,[r3]               ;Place the first half-word in the entry into r2
      ;mov     r1,r12
    pop r0,r12,r15
  .pool
  .align 4
;END ActorAccessor

;Filename for actors file
  ActorListFilePath:
    .ascii "rom0:BALANCE/actor_list.bin"
    dcb 0 ;Ending 0!
  .align 4
;Fill up the rest with junk so we know if something went wrong
        .fill  (0x20A9208 - .), 255 ;Null out the rest of the table
.endarea


;Approx locations to replace:

;-------------------------------------
; 0x20240B0 Hook
;-------------------------------------
.org 0x20240B0
.area (0x2024114 - .)
  push    r4-r8,r14
  mov     r8,r0
  mov     r7,r1
  ;ldr     r5,=20A7FF0h ; unused!
  mov     r6,0h
  ;ldr     r4,=182h ;Fill in the size with accessor!

  ;Get the nb of actor here in the 2 instructions we've freed!
  ;bl GetNbActors
  ;mov r4,r0
  bl ActorListCustomHook0206B24

  b       @@LBL2
  @@LOOP_BEG1:
  ldr     r1,[r5,4h] ;<================ ACCESSOR HERE

  ;Get the address of the actor entry, place it in r5.
  ;bl ActorListCustomHook0206B24 ;This will set the register up correctly!

  mov     r0,r7
  bl      202364Ch          //0202364C Compare NPC names!
  cmp     r0,0h
  beq     @@LBL1
  mov     r1,r6,lsl 10h
  mov     r0,r8
  mov     r1,r1,asr 10h
  bl      2023D64h
  pop     r4-r8,r15
  @@LBL1:
  add     r6,r6,1h
  add     r5,r5,0Ch ;Don't need to do this with the accessor!
  @@LBL2:
  cmp     r6,r4
  blt     @@LOOP_BEG1
  mov     r0,0h
  pop     r4-r8,r15
  .pool
  .fill (0x2024114 - .), 0
.endarea


;-------------------------------------
; 0x2024114 Hook
;-------------------------------------
.org 0x2024114
.area (0x2024178 - .)
  push    r4-r8,r14
  mov     r8,r0
  mov     r7,r1
  ;ldr     r5,=20A7FF0h
  mov     r6,0h
  ;ldr     r4,=182h

  ;Get the nb of actor here in the 2 instructions we've freed!
  ;bl GetNbActors
  ;mov r4,r0
  bl ActorListCustomHook0206B24

  b       @@LBL1
  @@LOOP_BEG1:
  ldr     r1,[r5,4h]
  ;Get the address of the actor entry, place it in r5.
  ;bl ActorListCustomHook0206B24 ;This will set the register up correctly!

  mov     r0,r7
  bl      202364Ch
  cmp     r0,0h
  beq     @@LBL2
  mov     r1,r6,lsl 10h
  mov     r0,r8
  mov     r1,r1,asr 10h
  bl      2023DC0h
  pop     r4-r8,r15
  @@LBL2:
  add     r6,r6,1h
  add     r5,r5,0Ch
  @@LBL1:
  cmp     r6,r4
  blt     @@LOOP_BEG1
  mov     r0,0h
  pop     r4-r8,r15
  ;02024170 020A7FF0
  ;02024174 00000182
  .pool
  .fill (0x2024178 - .), 0
.endarea

;-------------------------------------
; 0x2024178 Hook
;-------------------------------------
.org 0x2024184
.area (0x20241DC - .)
    push    r4-r8,r14
    mov     r8,r0
    mov     r7,r1
    ;ldr     r5,=20A7FF0h
    mov     r6,0h
    ;ldr     r4,=182h

    ;Get the nb of actor here in the 2 instructions we've freed!
    ;bl GetNbActors
    ;mov r4,r0
    bl ActorListCustomHook0206B24

    b       @@LBL1
    @@LOOP_BEG1:
    ldr     r1,[r5,4h]
  ;Get the address of the actor entry, place it in r5.
    ;bl ActorListCustomHook0206B24 ;This will set the register up correctly!
    mov     r0,r7
    bl      202364Ch
    cmp     r0,0h
    beq     @@LBL2
    mov     r1,r6,lsl 10h
    mov     r0,r8
    mov     r1,r1,asr 10h
    bl      2023FB4h
    pop     r4-r8,r15
    @@LBL2:
    add     r6,r6,1h
    add     r5,r5,0Ch
    @@LBL1:
    cmp     r6,r4
    blt     @@LOOP_BEG1
    mov     r0,0h
    pop     r4-r8,r15
    ;020241D4 020A7FF0
    ;020241D8 00000182
    .pool
    .fill (0x20241DC - .), 0
.endarea

;-------------------------------------
; 0x2065050 Hook
;-------------------------------------
.org 0x20650C0
.area 16
    mov     r0,r2           ;020650C0 E3A0000C mov     r0,0Ch
    bl      ActorAccessor   ;020650C4 E1610082 smulbb  r1,r2,r0
    ldrsh   r0,[r0]         ;020650C8 E59F03C8 ldr     r0,=20A7FF0h
    nop                     ;020650CC E19000F1 ldrsh   r0,[r0,r1]
.endarea
.org 0x2065498 ;Can replace address to actor table in datapool
.area 4
  .pool
  .fill (0x2065498 + 4) - .,0
.endarea

;-------------------------------------
; 0x206549C Hook
;-------------------------------------
.org 0x20654D0
.area 20
  push r14                            ;020654D0 E3A0100C mov     r1,0Ch
  mov r1,r14                          ;020654D4 E163018E smulbb  r3,r14,r1
  bl  ActorListCustomHook0206549C     ;020654D8 E59FC484 ldr     r12,=20A7FF0h
  pop r14                             ;020654DC E19C20F3 ldrsh   r2,[r12,r3]
  nop                                 ;020654E0 E08C3003 add     r3,r12,r3
.endarea
.org 0x2065964  ;Can replace address to actor table in datapool
.area 4
  .pool
  .fill (0x2065964 + 4) - .,0
.endarea

;-------------------------------------
; 0x2065B14 Hook
;-------------------------------------
.org 0x2065B14
.area (0x2065B3C - .)
push r14
  mvn     r1,0h
  cmp     r0,r1
  moveq   r0,0h
  popeq   r15 ;bxeq    r14
  ;ldr     r2,=20A7FF0h
  bl ActorAccessor    ;mov     r1,0Ch
  ;smlabb  r0,r0,r1,r2
  ldrh    r0,[r0,8h]
  pop     r15             ;bx      r14
  .pool
  .fill (0x2065B3C - .), 0
.endarea
