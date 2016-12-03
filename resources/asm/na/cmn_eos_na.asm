; For use with ARMIPS v0.7d
; By: psycommando@gmail.com
; 2016/08/18
; For Explorers of Sky North American ONLY!
; ------------------------------------------------------------------------------
; Copyright © 2016 Guillaume Lavoie-Drapeau <psycommando@gmail.com>
; This work is free. You can redistribute it and/or modify it under the
; terms of the Do What The Fuck You Want To Public License, Version 2,
; as published by Sam Hocevar. See http://www.wtfpl.net/ for more details.
; ------------------------------------------------------------------------------


; Overlays load offsets
.definelabel Overlay_0010_offset, 0x022BCA80
.definelabel Overlay_0011_offset, 0x022DC240
.definelabel Overlay_0013_offset, 0x0238A140

; Known function offsets from the game's binaries:
.definelabel LoadFileFromRom, 0x2008C3C     ;(r0 = Return Struct that will contain the AllocatedBuffLen + BufferPtr, r1 = FilePath, r2 = Unknown usually 0 )
.definelabel HandleSIR0,      0x201F4B4     ;(r0 = Pointer to SIR0 Data beg will be placed at the address in r0!, r1 = BufferPtr pointer to the raw file data in memory! )
.definelabel DebugPrint,      0x200C240     ;

; Alloc
.definelabel MemAlloc,        0x2001170     ;(r0 = SzAlloc, r1 = Align?) Returns r0 = PtrAllocated, r1 is possibly the nb of elements of the size specified to allocate! A bit like calloc()
.definelabel MemFree,         0x2001188     ;(r0 = BufToFree)
.definelabel MemAlloc2,       0x2001390     ;(r0 = unknown, r1 = SzAlloc, r1 = Align?) Returns r0 = PtrAllocated (Sould use MemAlloc instead to organize the parameters!!!)

.definelabel MemZeroFill,     0x2003250     ;(r0 = PtrBuf, r1 = LengthBuffer)

; file streams
.definelabel FStreamAlloc,    0x2008168     ;() Is usually done first, before any reading is done. Seems to instantiate the Filestream?
.definelabel FStreamCtor,     0x2008204     ;(r0 = PtrFStreamStruct)  Zeroes the content of the struct
.definelabel FStreamFOpen,    0x2008210     ;(r0 = PtrFStreamStruct, r1 = PtrFPath) Open the file for reading
.definelabel FStreamSeek,     0x20082A8     ;(r0 = PtrFStreamStruct, r1 = OffsetToSeekTo, r2 = unknown?(usually 0) )
;2008244h
.definelabel FStreamRead,     0x2008254     ;(r0 = PtrFStreamStruct, r1 = PtrOutBuffer, r2 = NbBytesToRead ) Read the ammount of bytes specified to the buffer, for the FStream object
.definelabel FStreamClose,    0x20082C4     ;(r0 = PtrFStreamStruct)  Close the filestream
.definelabel FStreamDealloc,  0x2008194     ;() ???
