; For use with ARMIPS v0.7d
; By: psycommando@gmail.com
; 2016/09/16
; For Explorers of Sky North American ONLY!
; ------------------------------------------------------------------------------
; Copyright © 2016 Guillaume Lavoie-Drapeau <psycommando@gmail.com>
; This work is free. You can redistribute it and/or modify it under the
; terms of the Do What The Fuck You Want To Public License, Version 2,
; as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
; ------------------------------------------------------------------------------
; This loads the level_list.bin file completely in memory once.
;
.nds
.arm

;Loads the Level list file!
LevelListLoader:
    push    r0,r1,r2,r14
    ;First reserve 8 bytes on the stack
    ;sub     r13,r13,8h

    ;Prepare our parameters and call the file loading function
    ;add     r0,r13,0h             ;Return struct for the loaded file data
    ldr     r0,=RetSzAndFBuff     ;
    ldr     r1,=LevelListFPath    ;Load our custom file path ptr
    ldr     r2,=0h                ;Not sure what this does. Its usually 1, 6,or sometimes 0x30F Maybe byte align??
    bl      LoadFileFromRom       ;This will return the loaded file size in bytes!!

    ;Check if filesize non-zero
  ;  cmp     r0, 0h
  ;    bne     @@Continue            ;Branch ahead if there's no problem
    ;mov     r0,1h
    ;ldr     r1,=LevellistError
    ;bl      DebugPrint

@@Continue:
    ;Copy the 2 dwords returned on the stack earlier to our dedicated variables
    ldr     r0,=LevelListFileBufferPtr
    ;ldr     r1,[r13]
    ldr     r1,=RetSzAndFBuff
    ldr     r1,[r1]
    str     r1,[r0],4h
    ;ldr     r1,[r13,4h]
    ldr     r1,=RetSzAndFBuff
    ldr     r1,[r1,4h]
    str     r1,[r0],4h

    ;Prepare the SIR0
    ldr     r0,=LevelListTablePtr   ;This is where the pointer to the data from the SIR0 will be placed!
    ;ldr     r1,[r13]                ;This is the pointer to the filebuffer we just put on the stack
    ldr     r1,=RetSzAndFBuff
    ldr     r1,[r1]
    bl      HandleSIR0              ;This converts the offset to be memory relative, when needed

    ;Finally, dealloc the 8 bytes on the stack
    ;add     r13,r13,8h
    pop     r0,r1,r2,r15
    ;Pool constants here
    .pool
;END LevelListLoader

;Run a check to see if the pointer to the buffer is null.
ShouldLoadLevelList:
    push    r1,r2,r3,r14
    ldr     r0,=LevelListFileBufferPtr
    ldr     r1,=LevelListTablePtr
    ldr     r0,[r0]
    ldr     r1,[r1]
    cmp r0,0h
        moveq r0,1h
        popeq r1,r2,r3,r15
    ;cmpne r1,0h
        ;moveq r0,1h
        ;popeq r1-r3,r15
@@ReturnFalse:
    mov r0,0h
    pop r1,r2,r3,r15
    ;Pool constants here
    .pool
;END ShouldLoadLevelList

;TryLoadLevelList: Load the level list if needed!
TryLoadLevelList:
  push r0,r14
  bl ShouldLoadLevelList
  cmp r0, 0h
    beq @@end           ;If the file is already loaded, just jump out
  bl  LevelListLoader
@@end:
  pop r0,r15
;END

;For directly getting the level list address with no fuss involved
GetLevelListAddress:
  ldr r0,=LevelListTablePtr
  ldr r0,[r0]
  bx r14
;END

;Get Or Load The Level List into R0!
GetOrLoadLevelList:
  push r14
  bl ShouldLoadLevelList
  cmp r0, 0h
      beq @@GetAddress ;If the file is already loaded, just jump to accessing the table
@@LoadTable:
      bl  LevelListLoader
@@GetAddress:
  ldr r0,=LevelListTablePtr
  ldr r0,[r0]
  pop r15
  .pool
;END

;Access the content of the buffer much like the original function!
LevelListAccessor:
    push r1,r2,r3,r4,r14
    ;First, save the entry index in r0
    mov r3,r0
    ;Check if we must load the file
    ;bl ShouldLoadLevelList
    ;cmp r0, 0h
        ;beq @@GetValue ;If the file is already loaded, just jump to accessing the table
 ;@@LoadTable:
    ;bl  LevelListLoader
 ;@@GetValue:

    ;cmp     r3,0xCF0
    ;  bne @@Continue
    ;.msg "Well shit"
 ;@@Continue:
    ;bl GetOrLoadLevelList ;Get the pointer to the level list, or load the level list! It'll end up in R0!
    ;TEST
    bl      GetLevelListAddress
    mov     r1,0Ch        ;Each entries in the table is 12 bytes long
    smulbb  r1,r3,r1    ;Multiply by the size of a table entry
    ;ldr     r4, =LevelListTablePtr
    ;ldr     r0, [r4]      ;Load our pointer
    add     r0,r0,r1    ;set it to the correct entry
    ;ldr     r0, [r4,r1] ;Load the actual
    pop     r1,r2,r3,r4,r15
    ;Pool constants here
    .pool
;END LevelListAccessor

;.definelabel LevelListFileBufferPtr, 0x020A46EC
LevelListFileBufferPtr:
    dcd 0
    dcd 0
;.definelabel LevelListTablePtr, 0x020A46F4
LevelListTablePtr:
    dcd 0
RetSzAndFBuff: ; return value for the file loading function on the heap since we messed up the stack otherwise
    dcd 0
    dcd 0
.align 4 ;align the string on 4bytes
