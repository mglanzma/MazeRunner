@ checkPlayerWin.s

/* function to check if the player has won based on the win conditions */
.global checkPlayerWin

checkPlayerWin:
    cmp r0, #3
    blt return_false
    
    cmp r1, #22 
    beq return_true
    cmp r1, #23 
    beq return_true
    cmp r1, #54 
    beq return_true
    cmp r1, #55 
    beq return_true

    b return_false

return_true:
    mov r0, #1
    mov pc, lr

return_false:
    mov r0, #0
    mov pc, lr
