#include <stdio.h>

int main(int argc, char *argv[]) {
    char *p = argv[1];
    char op, op_last = 0;
    // 符号キーのための変数
    int sign;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <expression>\n", argv[0]);
        return 1;
    }
    printf(".data\n");
    printf("L_fmt:\n");
    printf("\t.ascii \"%%d\\n\\0\"\n");
    printf("L_error_fmt:\n");
    printf("\t.ascii \"E\\n\\0\"\n");
    printf("memory_storage:\n");
    printf("\t.long 0\n");
    printf("\t.section .note.GNU-stack,\"\",@progbits\n");
    printf(".text\n");
    printf(".globl main\n");
    printf("main:\n");
    printf("\tpushq %%rbp\n");
    printf("\tmovq %%rsp, %%rbp\n");

    while (*p) {
        // ecxに入力された数字を保持してから、演算される数eax,演算する数ebxに分ける
        // -2147...8に対応するため、入力は負の数で行い、後で符号を戻す
        // 桁をくり上げるとき、１の位の数を入れる時にオーバーフローチェックをする
        printf("\tmovl $0, %%ecx\n");
        sign = 1;
        while ((*p >= '0' && *p <= '9') || *p == 'S') {
            if (*p >= '0' && *p <= '9') {
                printf("\tmovl $%c, %%edx\n", *p);
                printf("\timul $10, %%ecx\n");
                printf("\tjo .L_overflow_error\n");
                printf("\tsubl %%edx, %%ecx\n");
                printf("\tjo .L_overflow_error\n");
            }
            else if (*p == 'S') {
                sign = sign * (-1);
            }
            p++;
        }
        // 負の数で入力したので比較するsignは１
        // -2147...8 -> 2147...8はオーバーフローになるからチェックが必要
        if (sign == 1) {
            printf("\tneg %%ecx\n");
            printf("\tjo .L_overflow_error\n");
        }
        // calc1と同様の処理
        if (op_last == 0) {
            printf("\tmovl %%ecx, %%eax\n");
        }
        else {
            printf("\tmovl %%ecx, %%ebx\n");
            // + - *の時は単純なオーバーフローチェックをする
            switch (op_last) {
                case '+':
                printf("\taddl %%ebx, %%eax\n"); 
                printf("\tjo .L_overflow_error\n");
                break;
                case '-':
                printf("\tsubl %%ebx, %%eax\n"); 
                printf("\tjo .L_overflow_error\n");
                break;
                case '*':
                printf("\timul %%ebx, %%eax\n"); 
                printf("\tjo .L_overflow_error\n");
                break;
                case '/':  
                // 割る数が0の時即座にエラーにジャンプ 
                printf("\tcmpl $0, %%ebx\n");
                printf("\tje .L_overflow_error\n");
                // idivの時はオーバーフラグが立たないので、例外的に処理する
                // 整数の割り算でオーバーフローするのは-2147...8 / -1 の時のみなのでその例外処理をする
                printf("\tcmpl $-1, %%ebx\n");
                printf("\tjne .L_div_ok\n");
                printf("\tcmpl $0x80000000, %%eax\n");
                printf("\tjne .L_div_ok\n");
                printf("\tjmp .L_overflow_error\n");
                printf(".L_div_ok:\n");
                printf("\tcdq\n");          
                printf("\tidiv %%ebx\n");  
                break;
            }
            op_last = 0;
        }
        if (*p == '\0') {
            break; 
        }
        // 基本はculc1と同じ
        // メモリプラスとメモリマイナスの時だけオーバーフローチェック
        while (*p != '\0' && !(*p >= '0' && *p <= '9')) {
            op = *p;
            p++;
            switch (op) {
                case '+':
                case '-':
                case '*':
                case '/':
                op_last = op;
                break;
                case 'P': // M+
                printf("\taddl %%eax, memory_storage(%%rip)\n");
                printf("\tjo .L_overflow_error\n");
                break;
                case 'M': // M-
                printf("\tsubl %%eax, memory_storage(%%rip)\n");
                printf("\tjo .L_overflow_error\n");
                break;
                case 'C': // MC
                printf("\tmovl $0, memory_storage(%%rip)\n");
                break;
                case 'R': // MR
                printf("\tmovl memory_storage(%%rip), %%eax\n");
                break;
            }
        }
    }
    printf("\tleaq L_fmt(%%rip), %%rdi\n");
    printf("\tmovslq %%eax, %%rsi\n");
    printf("\tmovb $0, %%al\n");
    printf("\tcall printf\n");
    printf("\tleave\n");
    printf("\tret\n");
    printf(".L_overflow_error:\n");
    printf("\tleaq L_error_fmt(%%rip), %%rdi\n"); 
    printf("\tmovb $0, %%al\n");                 
    printf("\tcall printf\n");                   
    printf("\tmovl $1, %%edi\n");                
    printf("\tcall exit\n");
    return 0;
}

