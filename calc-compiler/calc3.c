
#include <stdio.h>

int main(int argc, char *argv[]) {
    
    char *p = argv[1];
    char op, op_last = 0;
    // 符号キーのための変数
    int sign;
    // 掛け算、割り算で毎回違うラベル名をつけるための変数
    int mul_count = 0;
    int div_count = 0;

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
        // 基本はcalc1と同じ
        // imulが使えないので、(ecx * 10)を{(ecx * 8) + (ecx * 2)}にした。
        // * 8 は左に３つシフト、* 2は左に一つシフトすることで実現した
        // ecxの値を保持しておく際には、ebxを使った。eaxは計算結果を保持しているため使えないが、ebxは演算する数なのでこの処理のあと更新されるため使える。
        printf("\tmovl $0, %%ecx\n");
        sign = 1;
        while ((*p >= '0' && *p <= '9') || *p == 'S') {
            if (*p >= '0' && *p <= '9') {
                printf("\tmovl $%c, %%edx\n", *p);
                printf("\tmovl %%ecx, %%ebx\n");
                printf("\tshl $3, %%ecx\n");
                printf("\tshl $1, %%ebx\n");
                printf("\taddl %%ebx, %%ecx\n");
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
        // 基本的にはcalc1と同じ
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
                // 掛け算は２進数の筆算の方法で行った
                // 負の数の掛け算については符号なし整数とみて計算することで、32ビットを超えた分が捨てられ、結果的に正しい計算結果となる
                // 小さな負の掛け算でも32回ループを回すので効率が悪いが、32回程度なのでそのままにした
                case '*':
                // 答えを0で初期化
                printf("\tmovl $0, %%ecx\n");
                // 乗算のスタートラベル
                printf(".L_mul_start%d:\n", mul_count);
                printf("\tcmpl $0, %%ebx\n");
                printf("\tje .L_mul_end%d\n", mul_count);
                // かける数の一番右のビットが１かどうか見て、もし1だったらかけられる数(eax)をecxに足す
                printf("\ttestl $1, %%ebx\n");
                printf("\tjz .L_mul_skip%d\n", mul_count);
                printf("\taddl %%eax, %%ecx\n");
                printf(".L_mul_skip%d:\n", mul_count);

                // かけられる数の桁を一つ大きくして、かける数の一番右のビットを捨てて次のビットを右端に移動
                printf("\tshl $1, %%eax\n");
                printf("\tshr $1, %%ebx\n");

                // スタートラベルに戻る
                printf("\tjmp .L_mul_start%d\n", mul_count);
                printf(".L_mul_end%d:\n", mul_count);
                // 答えをeaxに移動
                printf("\tmovl %%ecx, %%eax\n");
                mul_count++;
                break;
                case '/':   

                // 被除数の符号判定
                // 負ならスタックに1を積み、正なら0を積む
                // 負なら符号反転して絶対値をとる
                printf("\tcmpl $0, %%eax\n");
                printf("\tjge .L_eax_posi%d\n", div_count); 
                printf("\tneg %%eax\n"); 
                printf("\tpushq $1\n");
                printf("\tjmp .L_eax_end%d\n", div_count); 
                printf(".L_eax_posi%d:\n", div_count);
                printf("\tpushq $0\n");
                printf(".L_eax_end%d:\n", div_count);

                // 除数の符号判定　操作は被除数と同じ
                printf("\tcmpl $0, %%ebx\n");
                printf("\tjge .L_ebx_posi%d\n", div_count); 
                printf("\tneg %%ebx\n");   
                printf("\tpushq $1\n");
                printf("\tjmp .L_ebx_end%d\n", div_count); 
                printf(".L_ebx_posi%d:\n", div_count);
                printf("\tpushq $0\n");
                printf(".L_ebx_end%d:\n", div_count);

                // 32回ループのためにediを設定
                printf("\tmovl $32, %%edi\n");
                printf("\tmovl $0, %%ecx\n");
                // 余りは0で初期化
                printf("\tmovl $0, %%edx\n");

                // 除算のスタートラベル
                printf(".L_div_start%d:\n", div_count);

                //eaxの一番左を取ってedxの一番右に入れる
                printf("\tshl $1, %%eax\n");
                printf("\trcl $1, %%edx\n");

                //ecxの一番右に０を入れる。つまり左に一個ずらす
                printf("\tshl $1, %%ecx\n");
                //edx<ebxならスキップ
                printf("\tcmp %%ebx, %%edx\n");
                printf("\tjb .L_div_skip%d\n", div_count);

                //もし引けるならecxの一番右を１に変えて
                printf("\tsub %%ebx, %%edx\n");
                printf("\tinc %%ecx\n");
                printf(".L_div_skip%d:\n", div_count);
                //ループのカウンタを-1して、最終的に０になったら終了
                printf("\tdecl %%edi\n");
                printf("\tjnz .L_div_start%d\n", div_count);

                // 商をeaxに移動
                printf("\tmovl %%ecx, %%eax\n");

                //符号の判定
                //スタックに積まれた二つのフラグに対して負のフラグであれば符号反転する
                printf("\tpopq %%rdx\n");
                printf("\tcmp $0, %%rdx\n");
                printf("\tje .L_first_end%d\n", div_count);
                printf("\tneg %%eax\n");
                printf(".L_first_end%d:\n", div_count);
                printf("\tpopq %%rdx\n");
                printf("\tcmp $0, %%rdx\n");
                printf("\tje .L_second_end%d\n", div_count);
                printf("\tneg %%eax\n");
                printf(".L_second_end%d:\n", div_count);
                div_count++;

                break;
            }
            op_last = 0;
        }
        if (*p == '\0') {
            break; 
        }
        while (*p != '\0' && !(*p >= '0' && *p <= '9') && *p != 'S') {
            // calc1と同じ
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
                case 'C': // MC (Memory Clear)
                printf("\tmovl $0, memory_storage(%%rip)\n");
                break;
                case 'R': // MR (Memory Recall)
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

