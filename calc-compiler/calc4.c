#include <stdio.h>

// 計算のロジックを再帰関数として扱いたい
// ポインタの位置を共有するため、ダブルポインタ
void compile(char **p) {
    char op, op_last = 0;
    int sign;

    // 文字列が終わるか、閉じカッコが来るまで
    while (**p != '\0' && **p != ')') {
        
        // ecx, edxを使って入力を処理する準備
        printf("\tmovl $0, %%ecx\n");
        sign = 1;

        // 数値またはカッコ内の式の読み込み
        
        // 数字や(の前にSがある場合に対応
        while (**p == 'S') {
            sign = sign * (-1);
            (*p)++;
        }

        if (**p == '(') {
            (*p)++; 
            // 再帰的に式を評価する。結果はeaxに入って戻ってくる
            // 再帰の結果を新しい数値入力として扱う
            // 再帰呼び出しの結果(eax)を、数値入力レジスタ(ecx)に移すことでカッコ内の計算結果が通常の数値入力と同じ扱いになる
            // eaxを保存するためにpushする
            if (op_last != 0) {
                printf("\tpushq %%rax\n"); // カッコ前に演算がある場合のみ保存
            }

            compile(p); // 再帰呼び出し

            if (op_last != 0) {
                printf("\tmovl %%eax, %%ecx\n"); // 結果をecxへ
                printf("\tpopq %%rax\n");        // 左辺を復帰
            } else {
                printf("\tmovl %%eax, %%ecx\n"); // 結果をecxへ (まだ演算子がない場合)
            }

            if (**p == ')') {
                (*p)++; 
            }
        } else {
            while ((**p >= '0' && **p <= '9') || **p == 'S') {
                if (**p >= '0' && **p <= '9') {
                    printf("\tmovl $%c, %%edx\n", **p);
                    printf("\timul $10, %%ecx\n");
                    printf("\taddl %%edx, %%ecx\n");
                }
                else if (**p == 'S') {
                    sign = sign * (-1);
                }
                (*p)++;
            }
        }

        // 符号をつける
        if (sign == (-1)) {
            printf("\tneg %%ecx\n");
        }

        // op_lastによって数字の前に演算キーが入力されたかを見る
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

        // 文字列終了または)ならループ終了
        if (**p == '\0' || **p == ')') {
            break; 
        }
        
        // 次が数値または()になるまで読み進める
        while (**p != '\0' && **p != ')' && !(**p >= '0' && **p <= '9') && **p != '(') {
            op = **p;
            (*p)++;
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
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <expression>\n", argv[0]);
        return 1;
    }
    
    char *p = argv[1];

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

    // 再帰関数を呼び出す
    compile(&p);

    printf("\tleaq L_fmt(%%rip), %%rdi\n");
    printf("\tmovslq %%eax, %%rsi\n");
    printf("\tmovb $0, %%al\n");
    printf("\tcall printf\n");
    printf("\tleave\n");
    printf("\tret\n");
    
    return 0;
}