/*  tinypvm.c
*  Stack-based Virtual Machine
*  Compile: gcc -O2 -std=c11 -otinypvm tinypvm.c
*/
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

enum {
    OP_HALT = 0x00,
    OP_PUSH = 0x01,
    OP_ADD = 0x02,
    OP_SUB = 0x03,
    OP_MUL = 0x04,
    OP_DIV = 0x05,
    OP_PRINT = 0x06,
    OP_DUP = 0x07,
    OP_POP = 0x08
};

#define STACK_SIZE 1024
typedef struct {
    uint8_t *code; // bytecode array
    size_t code_len; // length of bytecode
    size_t ic; // instruction counter
    int32_t stack[STACK_SIZE]; // data stack
    int sp; // next free slot (stack pointer)
} VM;

static void vm_init(VM *vm, uint8_t *code, size_t code_len) {
    vm->code = code;
    vm->code_len = code_len;
    vm->ic = 0;
    vm->sp = 0;
    memset(vm->stack, 0, sizeof(vm->stack));
}

static int vm_read_u8(VM *vm, uint8_t *out) {
    if (vm->ic >= vm->code_len) return 0; // end of code
    *out = vm->code[vm->ic++]; // fetch byte and increment ic
    return 1;
}

static int vm_read_i32(VM *vm, int32_t *out) {
    if (vm->ic + 4 > vm->code_len) return 0;
    uint8_t b0 = vm->code[vm->ic++];
    uint8_t b1 = vm->code[vm->ic++];
    uint8_t b2 = vm->code[vm->ic++];
    uint8_t b3 = vm->code[vm->ic++];
    *out = (int32_t)((b0) | (b1 << 8) | (b2 << 16) | (b3 << 24));
    return 1;
}

static void push(VM *vm, int32_t v) {
    if (vm->sp >= STACK_SIZE) {
        fprintf(stderr, "Stack overflow\n");
        exit(1);
    }
    vm->stack[vm->sp++] = v;
}

static int32_t pop(VM *vm) {
    if (vm->sp <= 0) {
        fprintf(stderr, "Stack underflow\n");
        exit(1);
    }
    return vm->stack[--vm->sp];
}

static void run(VM *vm) {
    while (vm->ic < vm->code_len) {
        uint8_t op;
        if (!vm_read_u8(vm, &op)) break;
        switch (op) {
            case OP_HALT:
                return;
            case OP_PUSH: {
                int32_t v;
                if (!vm_read_i32(vm, &v)) { fprintf(stderr, "Unexpected EOF on PUSH\n"); return; }
                push(vm, v);
                break;
            }
            case OP_ADD: {
                int32_t b = pop(vm), a = pop(vm);
                push(vm, a + b);
                break;
            }
            case OP_SUB: {
                int32_t b = pop(vm), a = pop(vm);
                push(vm, a - b);
                break;
            }
            case OP_MUL: {
                int32_t b = pop(vm), a = pop(vm);
                push(vm, a * b);
                break;
            }
            case OP_DIV: {
                int32_t b = pop(vm), a = pop(vm);
                if (b == 0) { fprintf(stderr, "Division by zero\n"); return; }
                push(vm, a / b);
                break;
            }
            case OP_PRINT: {
                int32_t v = pop(vm);
                printf("%d\n", v);
                break;
            }
            case OP_DUP: {
                if (vm->sp <= 0) { fprintf(stderr, "Stack underflow on DUP\n"); return; }
                push(vm, vm->stack[vm->sp - 1]);
                break;
            }
            case OP_POP: {
                pop(vm);
                break;
            }
            default:
                fprintf(stderr, "Unknown opcode 0x%02x at ic=%zu\n", op, vm->ic-1);
                return;
        }
    }
}


static uint8_t example_prog[] = {
    // PUSH 10
    OP_PUSH, 0x0a, 0x00, 0x00, 0x00,
    // PUSH 20
    OP_PUSH, 0x14, 0x00, 0x00, 0x00,
    // ADD
    OP_ADD,
    // PUSH 2
    OP_PUSH, 0x02, 0x00, 0x00, 0x00,
    // MUL
    OP_MUL,
    // PRINT (should print (10+20)*2 = 60)
    OP_PRINT,
    // HALT
    OP_HALT
};

int main(int argc, char **argv) {
    uint8_t *code = NULL;
    size_t len = 0;

    if (argc >= 2) {
        // read program from file
        const char *path = argv[1];
        FILE *f = fopen(path, "rb");
        if (!f) { perror("fopen"); return 1; }
        fseek(f, 0, SEEK_END);
        long s = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (s <= 0) { fprintf(stderr, "Empty or invalid file\n"); fclose(f); return 1; }
        code = malloc(s);
        if (!code) { fprintf(stderr, "malloc failed\n"); fclose(f); return 1; }
        if (fread(code, 1, s, f) != (size_t)s) { fprintf(stderr, "read error\n"); free(code); fclose(f); return 1; }
        fclose(f);
        len = s;
    } else {
        code = example_prog;
        len = sizeof(example_prog);
    }

    VM vm;
    vm_init(&vm, code, len);
    run(&vm);

    if (argc >= 2) free(code);
    return 0;
}