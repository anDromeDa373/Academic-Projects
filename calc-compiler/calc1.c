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
    printf("memory_storage:\n");
    printf("\t.long 0\n");
    printf("\t.section .note.GNU-stack,\"\",@progbits\n");
    printf(".text\n");
    printf(".globl main\n");
    printf("main:\n");
    printf("\tpushq %%rbp\n");
    printf("\tmovq %%rsp, %%rbp\n");

    while (*p) {
        // ecx,edxを使って一文字づつ数字を入力する
        // Sは入力される数として扱うため、Sも読み込む
        printf("\tmovl $0, %%ecx\n");
        sign = 1;
        while ((*p >= '0' && *p <= '9') || *p == 'S') {
            if (*p >= '0' && *p <= '9') {
                // num = num * 10 + (*p - '0')
                printf("\tmovl $%c, %%edx\n", *p);
                printf("\timul $10, %%ecx\n");
                printf("\taddl %%edx, %%ecx\n");
            }
            else if (*p == 'S') {
                sign = sign * (-1);
            }
            p++;
        }
        if (sign == (-1)) {
            printf("\tneg %%ecx\n");
        }
        // op_lastによって数字の前に演算キーが入力されたかを見ている
        // 演算子が入力されていない時は演算される数なので必ずeaxに入るようにする
        // 一つ前に演算子がついているならば、それは演算する数であるのでebxに入れる
        if (op_last == 0) {
            printf("\tmovl %%ecx, %%eax\n");
        }
        else {
            printf("\tmovl %%ecx, %%ebx\n");

            switch (op_last) {
                case '+':
                printf("\taddl %%ebx, %%eax\n"); 
                break;
                case '-':
                printf("\tsubl %%ebx, %%eax\n"); 
                break;
                case '*':
                printf("\timul %%ebx, %%eax\n"); 
                break;
                case '/':   
                printf("\tcdq\n");          
                printf("\tidiv %%ebx\n");  
                break;
            }
            op_last = 0;
        }

        if (*p == '\0') {
            break; 
        }
        
        while (*p != '\0' && !(*p >= '0' && *p <= '9')) {
            // *p = op_lastにしなかったのは、普通演算子とメモリ機能を分けるためである。
            // 分けることで、次の全体ループに入った時に、演算子が普通のときだけ、新たに入った数字をebxに入れて演算の準備をできるようにした。
            // 分けないと、普通演算がない時にも、値がebxの中に入ってしまい、メモリ機能を使うときに、eaxに入っていて欲しいものがebxに入っているという事態が起きてしまう。
            // 普通演算子の時だけ、op_lastを更新しなければいけない。
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
                break;
                case 'M': // M-
                printf("\tsubl %%eax, memory_storage(%%rip)\n");
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
    
    return 0;
}

