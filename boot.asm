org 0x7C00
use16

    cli
    mov ax, 0x0013
    int 0x10
    sti

    mov ax, 0xA000
    mov es, ax

    xor di, di
    mov cx, 320*200
    mov al, 0x01
    rep stosb

    mov si, 10
    mov bx, 40

.triangle_row:
    push bx
    push si

    mov di, 10
    mov ax, si
    mov cx, 320
    mul cx
    add ax, di
    mov bp, ax

    mov ax, si
    sub ax, 9
    cmp ax, 0
    jle .skip_row_draw
    mov cx, ax

.draw_triangle_pixel:
    mov di, bp
    mov dx, cx
.draw_pixel_loop:
    mov byte [es:di], 0x0D
    inc di
    dec dx
    jnz .draw_pixel_loop

.skip_row_draw:
    pop si
    pop bx
    inc si
    dec bx
    jnz .triangle_row

mov ax, 180
mov cx, 320
mul cx
add ax, 150
mov [text_off], ax

    mov si, text_str
    mov bx, [text_off]
    call draw_string

.hang:
    jmp .hang

draw_string:
    push bp
    mov bp, sp
.next_char:
    lodsb
    cmp al, 0
    je .done_string

    mov di, chars_db
    xor dx, dx
.find_loop:
    mov al, [si-1]
    mov cl, [di]
    cmp cl, 0
    je .char_not_found
    cmp cl, al
    je .found_index
    inc di
    inc dx
    jmp .find_loop

.char_not_found:
    add bx, 6
    jmp .next_char

.found_index:
    mov ax, dx
    shl ax, 2
    add ax, dx

    mov di, font_table
    add di, ax

    push si
    push bp
    push ds
    mov cx, 5
    mov si, di
    mov di, bx
.col_loop:
    mov al, [si]
    mov di, bx
    mov bp, 0
.row_loop:
    test al, 1
    jz .skip_pixel
    mov byte [es:di], 0x0D
.skip_pixel:
    add di, 320
    shr al, 1
    inc bp
    cmp bp, 7
    jl .row_loop

    add bx, 1
    inc si
    dec cx
    jnz .col_loop

    add bx, 1

    pop ds
    pop bp
    pop si

    jmp .next_char

.done_string:
    pop bp
    ret

text_off dw 0
text_str db 'Malysheva O.M. NMT-333901', 0

chars_db db 'M','a','l','y','s','h','e','v',' ','O','.','N','T','-','3','9','0','1', 0

font_table:
    db 0x7F, 0x06, 0x18, 0x06, 0x7F
    db 0x38, 0x44, 0x44, 0x44, 0x7C
    db 0x00, 0x41, 0x7F, 0x40, 0x00
    db 0x0C, 0x50, 0x50, 0x50, 0x3C
    db 0x26, 0x49, 0x49, 0x49, 0x32
    db 0x7F, 0x08, 0x04, 0x04, 0x78
    db 0x38, 0x54, 0x54, 0x54, 0x08
    db 0x3C, 0x40, 0x20, 0x10, 0x0C
    db 0x00, 0x00, 0x00, 0x00, 0x00
    db 0x3E, 0x41, 0x41, 0x41, 0x3E
    db 0x00, 0x60, 0x60, 0x00, 0x00
    db 0x7F, 0x02, 0x04, 0x08, 0x7F
    db 0x01, 0x01, 0x7F, 0x01, 0x01
    db 0x08, 0x08, 0x08, 0x08, 0x08
    db 0x42, 0x49, 0x49, 0x49, 0x36
    db 0x46, 0x49, 0x49, 0x29, 0x1E
    db 0x3E, 0x45, 0x49, 0x51, 0x3E
    db 0x00, 0x41, 0x7F, 0x40, 0x00

times 510 - ($ - $$) db 0
dw 0xAA55
