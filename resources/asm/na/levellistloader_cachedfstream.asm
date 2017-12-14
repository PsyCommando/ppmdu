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
; This loads the level_list.bin file one entry at a time when needed using file streams.
; 
;.relativeinclude on
.nds
.arm

;Since we don't need to load it with streams, don't do anything!
    TryLoadLevelList:
      bx r14
    ;END

;Check if we need to load the level list for this entry! (r0 = level id, r1=nothing) Return in r0, 1 if should load, 0 if not
    CheckIfShouldGetLvlEntryFromFile:
      ldr r1,=LevelEntryBuffer_lastlvlid
      ldr r1,[r1]
      cmp r0,r1
      bne @@NeedToLoad
      mov r0,0h
      bx r14
      @@NeedToLoad:
      mov r0,1h
      bx r14
      .pool
      .align 4
    ;END

;LevelListAccessor (r0 = levelid) return in r0 the address of the entry in memory
    LevelListAccessor:
      push r1,r4,r14
      mov r4,r0
      bl CheckIfShouldGetLvlEntryFromFile

      cmp r0,0h

      ;If we don't need to load the file, load from cache and return!
      ldreq r0,=LevelEntryBuffer_lastentry
      beq @@End

      ;Else if we need to load the file do it!
      mov r0,r4                         ;Put level id back in r0!
      bl GetLevelEntryDirectlyFromFile  ;The entry's address will be in r0 already

      @@End:
      pop r1,r4,r15
      .pool
      .align 4
    ;END

;Seek within level list instead of loading it! (r0 = level id!) Returns r0 with a pointer to the entry in the entry buffer.
      GetLevelEntryDirectlyFromFile:
        push r1,r2,r4,r5,r14
        mov r4,r0         ;free up r0 to pass some parameters
        ldr r0,=LevelEntryBuffer_lastlvlid
        str r4,[r0]       ;Save the level id to the cache!
        ;sub r13,r13,48h   ;we'll alloc the file stream on the stack
        bl FStreamAlloc

        ;Construct file stream
        ;add r0,r13,0h
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        bl FStreamCtor

        ;Open filestream
        ;add r0,r13,0h          ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        ldr r1,=LevelListFPath ;level_list.bin
        add r1,5h              ;Add 5 bytes to skip the "rom0:" part
        bl FStreamFOpen

        ;Seek to table ptr offset
        ;add r0,r13,0h   ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        mov r1,4h
        mov r2,0h
        bl FStreamSeek

        ;Read Pointer
        ;add r0,r13,0h   ;Set r0 to filestream object
        ;sub r13,r13,4h  ;Alloc 4 bytes on stack
        ;add r1,r13,0h   ;Set allocated stack as dest buffer
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        ldr r1,=LevelList_PointerBuffer
        mov r2,4h       ;Set nb bytes to read to 4
        bl FStreamRead
        ldr r1,=LevelList_PointerBuffer
        ldr r1,[r1]     ;Load the 4bytes buffer into r1!
        mov r5,r1       ;copy the value into r5 so we can use it later
        ;add r13,r13,4h  ;Dealloc Alloc 4 bytes on stack

        ;Seek to pointer
        ;add r0,r13,0h   ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        ;r1 is already set!
        mov r2,0h
        bl FStreamSeek

        ;Seek to the correct entry!
        mov r0,12      ;An entry is 12 bytes
        mla r1,r4,r0,r5 ;set position to seek to: (levelid * 12) + tablebeg
        ;add r0,r13,0h   ;Set r0
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        mov r2,0h
        bl FStreamSeek  ;Seeke to the entry

        ;Read entry to buffer!
        ;add r0,r13,0h   ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        ldr r1,=LevelEntryBuffer_lastentry ;Set target buffer
        mov r2,12       ;Set nb bytes to read to 12
        bl FStreamRead

        ;Copy string!
        ;add r0,r13,0h   ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        ldr r1,=LevelEntryBuffer_lastentry
        ldr r1,[r1,8h] ;The string ptr is at 8 bytes in the entry
        mov r2,0h
        bl FStreamSeek  ;Seeke to the string
        ;add r0,r13,0h   ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        ldr r1,=LevelEntryBuffer_lastname ;Set string buffer as target buffer
        mov r2,10h      ;Set nb of bytes to read to 16!
        bl FStreamRead  ;Read filestream

        ;Replace string pointer!
        ldr r0,=LevelEntryBuffer_lastname   ;Address of the string buffer
        ldr r1,=LevelEntryBuffer_lastentry
        str r0,[r1,8h] ;The string ptr is at 8 bytes in the entry

        ;Close and Dealloc stream
        ;add r0,r13,0h   ;Set r0 to filestream object
        ldr r0,=LevelList_FileStream  ;Set r0 to filestream object
        bl FStreamClose
        bl FStreamDealloc

        ;Return
        ldr r0,=LevelEntryBuffer_lastentry
        ;add r13, r13, 48h ;dealloc filestream
        pop r1,r2,r4,r5,r15

        .pool
        LevelEntryBuffer_lastlvlid:
          dcd 0 ;holds the last levelid so we can cache calls to this and not reload the file all the time
        LevelEntryBuffer_lastentry:
          dcd 0,0,0 ;an entry is 12 bytes
        .align 4
        LevelEntryBuffer_lastname:
          dcb 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 ;reserve 16bytes string for writing the last string for the last entry here!
        LevelList_PointerBuffer:
          dcd 0
        LevelList_FileStream: ;Put the file stream struct here!
          defs 0x48,0
        .align 4
      ;END
