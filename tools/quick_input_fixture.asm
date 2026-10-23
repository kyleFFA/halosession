.code
PUBLIC fixturePoll
PUBLIC fixtureSite
fixturePoll PROC FRAME
 push rbx
 .pushreg rbx
 sub rsp,20h
 .allocstack 20h
 .endprolog
 mov rbx,rcx
 xor eax,eax
fixtureSite LABEL NEAR
 mov rbx,[rsp+20h]
 add rsp,20h
 pop rbx
 ret
fixturePoll ENDP
END
