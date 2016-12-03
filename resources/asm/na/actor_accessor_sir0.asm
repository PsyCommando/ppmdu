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
; This loads the actor_list.bin file completely in memory once.
;
.nds
.arm

;Loads the Actor list file!
ActorListLoader:
    push    r0,r1,r2,r14
    ;First reserve 8 bytes on the stack
    ;sub     r13,r13,8h

    ;Prepare our parameters and call the file loading function
    ;add     r0,r13,0h             ;Return struct for the loaded file data
    ldr     r0,=ActorRetSzAndFBuff     ;
    ldr     r1,=ActorListFilePath    ;Load our custom file path ptr
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
    ldr     r0,=ActorListFileBufferPtr
    ;ldr     r1,[r13]
    ldr     r1,=ActorRetSzAndFBuff
    ldr     r1,[r1]
    str     r1,[r0],4h
    ;ldr     r1,[r13,4h]
    ldr     r1,=ActorRetSzAndFBuff
    ldr     r1,[r1,4h]
    str     r1,[r0],4h

    ;Prepare the SIR0
    ldr     r0,=ActorListTablePtr   ;This is where the pointer to the data from the SIR0 will be placed!
    ;ldr     r1,[r13]                ;This is the pointer to the filebuffer we just put on the stack
    ldr     r1,=ActorRetSzAndFBuff
    ldr     r1,[r1]
    bl      HandleSIR0              ;This converts the offset to be memory relative, when needed

    ;Finally, dealloc the 8 bytes on the stack
    ;add     r13,r13,8h
    pop     r0,r1,r2,r15
    ;Pool constants here
    .pool
;END ActorListLoader

;Run a check to see if the pointer to the buffer is null.
ShouldLoadActorList:
    push    r1,r2,r3,r14
    ldr     r0,=ActorListFileBufferPtr
    ldr     r1,=ActorListTablePtr
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

;TryLoadActorList: Load the actor list if needed!
TryLoadActorList:
  push r0,r14
  bl ShouldLoadActorList
  cmp r0, 0h
    beq @@end           ;If the file is already loaded, just jump out
  bl  ActorListLoader
@@end:
  pop r0,r15
;END

;For directly getting the Actor list address with no fuss involved
GetActorListAddress:
  ldr r0,=ActorListTablePtr
  ldr r0,[r0]
  ldr r0,[r0] ;; There's a pointer in the subheader!
  bx r14
;END

;For directly getting the Actor list address with no fuss involved
GetNbActors:
  ldr r0,=ActorListTablePtr
  ldr r0,[r0]
  ldr r0,[r0,4] ;; Nb actors is in subheader
  bx r14
;END


ActorListFileBufferPtr:
    dcd 0
    dcd 0
ActorListTablePtr:
    dcd 0
ActorRetSzAndFBuff: ; return value for the file loading function on the heap since we messed up the stack otherwise
    dcd 0
    dcd 0
.align 4 ;align the string on 4bytes
