%macro _swapgs 1
    cmp QWORD [rsp + %1], 0x8
    je %%cont
    swapgs
%%cont:
%endmacro