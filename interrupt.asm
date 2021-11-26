; *************************************************************
; Copyright (c) 2021 roscopeco <AT> gmail <DOT> com
; *************************************************************
;
        section .text                     ; This is normal code

        include "xosera_m68k_defs.inc"

install_intr::
                movem.l D0-D7/A0-A6,-(A7)

                or.w    #$0200,SR               ; disable interrupts

                move.l  #XM_BASEADDR,A0         ; get Xosera base addr
                move.b  #$0F,D0                 ; all interrupt source bits
                move.b  D0,XM_TIMER+2(A0)       ; clear out any prior pending interrupts
                move.b  #$08,D0                 ; vsync interrupt
                move.b  D0,XM_SYS_CTRL(A0)      ; enable VSYNC interrupt

                move.l  #Xosera_intr,$68        ; set interrupt vector
                and.w   #$F0FF,SR               ; enable interrupts

                movem.l (A7)+,D0-D7/A0-A6
                rts

remove_intr::
                movem.l D0-D7/A0-A6,-(A7)

                move.l  #XM_BASEADDR,A0         ; get Xosera base addr
                move.b  #$00,D0                 ; no interrupts
                move.b  D0,XM_SYS_CTRL(A0)      ; enable VSYNC interrupt
                move.b  #$0F,D0                 ; all interrupt source bits
                move.b  D0,XM_TIMER+2(A0)       ; clear out any prior pending interrupts
                move.l  $60,D0                  ; copy spurious int handler
                move.l  D0,$68

                movem.l (A7)+,D0-D7/A0-A6
                rts

; interrupt routine
Xosera_intr:
                movem.l D0-D2/A0,-(A7)
                move.l  #XM_BASEADDR,A0         ; Get Xosera base addr
                movep.w XM_XR_ADDR(A0),D2       ; save aux_addr value

                move.w  pb_gfx_ctrl,D0          ; Get requested PB GFX_CTRL
                move.w  #XR_PB_GFX_CTRL,D1      ; And set in Xosera
                movep.w D1,XM_XR_ADDR(A0)
                movep.w D0,XM_XR_DATA(A0)                

                tst.b   pa_flip_needed          ; Buffer flip needed?
                beq.s   .DONE                   ; Done if not...

                move.w  back_pa_buf,D0          ; Else, swap the buffers...
                move.w  current_pa_buf,back_pa_buf
                move.w  D0,current_pa_buf
                clr.b   pa_flip_needed          ; ... and reset the flag.

                move.w  #XR_PA_DISP_ADDR,D1     ; Inform Xosera about the change
                movep.w D1,XM_XR_ADDR(A0)
                movep.w D0,XM_XR_DATA(A0)

.DONE
                move.b  #$0F,D0                 ; Set all interrupt source bits...
                move.b  D0,XM_TIMER+2(A0)       ; ... and clear out the pending interrupts

                movep.w D2,XM_XR_ADDR(A0)       ; restore aux_addr
                movem.l (A7)+,D0-D2/A0

                addi.l  #1,vblank_count         ; Increment vblank counter (used for waits)
                
                rte

