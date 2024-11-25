@ getFrameDigit.s

/* function to return the correct sprite frame based on the clock digit parameter */
.global getFrameDigit

getFrameDigit:
    cmp r0, #0          
    beq return_48     
    cmp r0, #1          
    beq return_56     
    cmp r0, #2          
    beq return_64     
    cmp r0, #3          
    beq return_72     
    cmp r0, #4          
    beq return_80     
    cmp r0, #5          
    beq return_88     
    cmp r0, #6          
    beq return_96     
    cmp r0, #7          
    beq return_104     
    cmp r0, #8          
    beq return_112     
    cmp r0, #9          
    beq return_120     

    //if we get here something went wrong return a default int like 0
    mov r0, #40 
    mov pc, lr     

return_48:
    mov r0, #48           
    mov pc, lr     

return_56:
    mov r0, #56           
    mov pc, lr     

return_64:
    mov r0, #64           
    mov pc, lr     

return_72:
    mov r0, #72           
    mov pc, lr     

return_80:
    mov r0, #80           
    mov pc, lr     

return_88:
    mov r0, #88          
    mov pc, lr     

return_96:
    mov r0, #96           
    mov pc, lr     

return_104:
    mov r0, #104           
    mov pc, lr     

return_112:
    mov r0, #112           
    mov pc, lr     

return_120:
    mov r0, #120           
    mov pc, lr     
