#include <vm.h>
#include <common.h>
#include <stdio.h>
#include <compiler.h>
#include <stdarg.h>
#include <stdlib.h>
#include <errno.h>
#include <lines.h>
#include <object.h>
#include <rain_memory.h>
#include <string.h>
#include <convert.h>

#ifdef DEBUG_TRACE_EXECUTION
#include <debug.h>
#endif

VM vm;

static void reset_stack()
{
    vm.stack_top = vm.stack;
}

static void runtime_error(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputs("\n", stderr);

    size_t inst = vm.ip - vm.chunk->code - 1;
    size_t line = get_line_number(&vm.chunk->line_encoding, inst);
    fprintf(stderr, "[line %zu] in script\n", line);
    reset_stack();
}

void init_vm()
{
    reset_stack();
    vm.objects = NULL;
    init_hash_table(&vm.globals);
    init_hash_table(&vm.strings);
}

static Value peek(int64_t distance)
{
    return vm.stack_top[-1 - distance];
}

static void concatenate()
{
    ObjString* b = AS_STRING(pop());
    ObjString* a = AS_STRING(pop());
    size_t res_len = a->len + b->len;
    char* res_chars = ALLOCATE(char, res_len);
    memcpy(res_chars, a->chars, a->len);
    memcpy(res_chars + a->len, b->chars, b->len);
    push(OBJ_VAL((Obj*)take_str(res_chars, res_len)));
}

#define READ_INST() (*(vm.ip++))

static size_t read_inst_index(size_t offset_size)
{
    size_t offset = 0;
    size_t index = read_chunk_const(vm.ip, &offset, offset_size);
    vm.ip += offset;
    return index;
}

static Value read_const(size_t offset_size)
{
    size_t index = read_inst_index(offset_size);
    return vm.chunk->consts.values[index];
}

static uint32_t read_jump()
{
    uint8_t* data = (uint8_t*)vm.ip;
    uint32_t offset = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16) | ((uint32_t)data[2] << 8) | (uint32_t)data[3];
    for(size_t i = 0; i < sizeof(uint32_t); i += sizeof(inst_type))
    {
        vm.ip++;
    }
    return offset;
}

#define READ_STRING(offset_size) AS_STRING(read_const(offset_size))

static InterpretResult run()
{
    for(;;)
    {
#ifdef DEBUG_TRACE_EXECUTION
        printf("        ");
        for(Value* slot = vm.stack; slot < vm.stack_top; slot++)
        {
            printf("[ ");
            print_value(*slot);
            printf(" ]");
        }
        printf("\n");
        disassemble_inst(vm.chunk, (size_t)(vm.ip - vm.chunk->code));
#endif
        inst_type inst;
        switch(inst = READ_INST())
        {
            case OP_RETURN:
            {
                // Exit program
                return INTERPRET_OK;
            }
            case OP_CONST_BYTE:
            {
                Value constant = read_const(1);
                push(constant);
                break;
            }
            case OP_CONST_SHORT:
            {
                Value constant = read_const(2);
                push(constant);
                break;
            }
            case OP_CONST_WORD:
            {
                Value constant = read_const(4);
                push(constant);
                break;
            }
            case OP_CONST_LONG:
            {
                Value constant = read_const(8);
                push(constant);
                break;
            }
            case OP_NULL:
            {
                push(NULL_VAL);
                break;
            }
            case OP_TRUE:
            {
                push(BOOL_VAL(true));
                break;
            }
            case OP_FALSE:
            {
                push(BOOL_VAL(false));
                break;
            }
            case OP_NEGATE:
            {
                if(IS_INT(peek(0)))
                {
                    push(INT_VAL(-AS_INT(pop())));
                }
                else if(IS_FLOAT(peek(0)))
                {
                    push(FLOAT_VAL(-AS_FLOAT(pop())));
                }
                else
                {
                    runtime_error("Operand must be an integer or float");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_ADD:
            {
                if(IS_STRING(peek(0)) && IS_STRING(peek(1)))
                {
                    concatenate();
                }
                else if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a + b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a + (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a + b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a + b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SUB:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a - b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a - (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a - b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a - b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_MUL:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a * b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a * (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a * b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a * b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DIV:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b == 0)
                    {
                        runtime_error("Divide by 0 error");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a / b));
                }
                else if(IS_INT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a / (double)b));
                }
                else if(IS_FLOAT(peek(0)) && IS_INT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    int64_t a = AS_INT(pop());
                    push(FLOAT_VAL((double)a / b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(FLOAT_VAL(a / b));
                }
                else
                {
                    runtime_error("Operands must be integers or floats");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_REM:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b == 0)
                    {
                        runtime_error("Divide by 0 error");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a % b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_NOT:
            {
                if(IS_BOOL(peek(0)))
                {
                    push(BOOL_VAL(!AS_BOOL(pop())));
                }
                else
                {
                    runtime_error("Operand must be a boolean");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_NOT:
            {
                if(IS_INT(peek(0)))
                {
                    push(INT_VAL(~AS_INT(pop())));
                }
                else
                {
                    runtime_error("Operand must be an integer");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_AND:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a & b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_OR:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a | b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_BIT_XOR:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(INT_VAL(a ^ b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SHIFT_LEFT:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b < 0)
                    {
                        runtime_error("Shift value can't be negative");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a << b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SHIFT_ARITH_RIGHT:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b < 0)
                    {
                        runtime_error("Shift value can't be negative");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    push(INT_VAL(a >> b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_SHIFT_LOGIC_RIGHT:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    if(b < 0)
                    {
                        runtime_error("Shift value can't be negative");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    if(a < 0 && b > 0)
                    {
                        a &= 0x7fffffffffffffff;
                    }
                    push(INT_VAL(a >> b));
                }
                else
                {
                    runtime_error("Operands must be integers");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_EQL:
            {
                Value b = pop();
                Value a = pop();
                if(a.type == VAL_NULL || b.type == VAL_NULL || a.type == b.type)
                {
                    push(BOOL_VAL(values_eql(a, b)));
                }
                else
                {
                    runtime_error("Operands must be the same type");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GREATER:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(BOOL_VAL(a > b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(BOOL_VAL(a > b));
                }
                else
                {
                    runtime_error("Operands must be the same type and a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_LESS:
            {
                if(IS_INT(peek(0)) && IS_INT(peek(1)))
                {
                    int64_t b = AS_INT(pop());
                    int64_t a = AS_INT(pop());
                    push(BOOL_VAL(a < b));
                }
                else if(IS_FLOAT(peek(0)) && IS_FLOAT(peek(1)))
                {
                    double b = AS_FLOAT(pop());
                    double a = AS_FLOAT(pop());
                    push(BOOL_VAL(a < b));
                }
                else
                {
                    runtime_error("Operands must be the same type and a number");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_CAST_BOOL:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(val);
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(BOOL_VAL(false));
                        break;
                    }
                    case VAL_INT:
                    {
                        push(BOOL_VAL(AS_INT(val) != 0));
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        push(BOOL_VAL(AS_FLOAT(val) != 0.0));
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                if(AS_STRING(val)->len == 4 && memcmp(AS_CSTRING(val), "true", 4) == 0)
                                {
                                    push(BOOL_VAL(true));
                                }
                                else if(AS_STRING(val)->len == 5 && memcmp(AS_CSTRING(val), "false", 5) == 0)
                                {
                                    push(BOOL_VAL(false));
                                }
                                else
                                {
                                    runtime_error("Cannot cast '%s' to bool", AS_CSTRING(val));
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                break;
                            }
                            default:
                            {
                                push(BOOL_VAL(true));
                                break;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_CAST_INT:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(INT_VAL(AS_BOOL(val) ? 1 : 0));
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(INT_VAL(0));
                        break;
                    }
                    case VAL_INT:
                    {
                        push(val);
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        push(INT_VAL((int64_t)AS_FLOAT(val)));
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                int64_t int_val = 0;
                                if(!str_to_int(&int_val, AS_CSTRING(val), AS_STRING(val)->len))
                                {
                                    runtime_error("Cannot cast '%s' to int", AS_CSTRING(val));
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                push(INT_VAL(int_val));
                                break;
                            }
                            default:
                            {
                                runtime_error("Unsupported conversion of object to int");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_CAST_STR:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(OBJ_VAL((Obj*)(AS_BOOL(val) ? copy_str("true", 4) : copy_str("false", 5))));
                        break;
                    }
                    case VAL_INT:
                    {
                        int64_t num = AS_INT(val);
                        char* res_chars = int_to_dec_str(num);
                        push(OBJ_VAL((Obj*)take_str(res_chars, strlen(res_chars))));
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(OBJ_VAL((Obj*)copy_str("null", 4)));
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        double num = AS_FLOAT(val);
                        char* res_chars = float_to_str(num);
                        push(OBJ_VAL((Obj*)take_str(res_chars, strlen(res_chars))));
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                push(val);
                                break;
                            }
                            default:
                            {
                                runtime_error("Unsupported conversion of object to string");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_CAST_FLOAT:
            {
                Value val = pop();
                switch(val.type)
                {
                    case VAL_BOOL:
                    {
                        push(FLOAT_VAL(AS_BOOL(val) ? 1.0 : 0.0));
                        break;
                    }
                    case VAL_NULL:
                    {
                        push(FLOAT_VAL(0.0));
                        break;
                    }
                    case VAL_INT:
                    {
                        push(FLOAT_VAL((double)AS_INT(val)));
                        break;
                    }
                    case VAL_FLOAT:
                    {
                        push(val);
                        break;
                    }
                    case VAL_OBJ:
                    {
                        switch(OBJ_TYPE(val))
                        {
                            case OBJ_STRING:
                            {
                                double float_val = 0;
                                if(!str_to_float(&float_val, AS_CSTRING(val), AS_STRING(val)->len))
                                {
                                    runtime_error("Cannot cast '%s' to float", AS_CSTRING(val));
                                    return INTERPRET_RUNTIME_ERROR;
                                }
                                push(FLOAT_VAL(float_val));
                                break;
                            }
                            default:
                            {
                                runtime_error("Unsupported conversion of object to float");
                                return INTERPRET_RUNTIME_ERROR;
                            }
                        }
                        break;
                    }
                    default:
                    {
                        runtime_error("Unknown type");
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_PRINT:
            {
                print_value(pop());
                printf("\n");
                break;
            }
            case OP_POP:
            {
                pop();
                break;
            }
            case OP_DEFINE_GLOBAL_BYTE:
            {
                ObjString* name = READ_STRING(1);
                if(hash_table_find_str(&vm.globals, name->chars, name->len, name->hash) == NULL)
                {
                    hash_table_insert(&vm.globals, name, AS_BOOL(peek(0)), peek(1));
                    pop();
                    pop();
                }
                else
                {
                    runtime_error("Redefinition of global variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DEFINE_GLOBAL_SHORT:
            {
                ObjString* name = READ_STRING(2);
                if(hash_table_find_str(&vm.globals, name->chars, name->len, name->hash) == NULL)
                {
                    hash_table_insert(&vm.globals, name, AS_BOOL(peek(0)), peek(1));
                    pop();
                    pop();
                }
                else
                {
                    runtime_error("Redefinition of global variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DEFINE_GLOBAL_WORD:
            {
                ObjString* name = READ_STRING(4);
                if(hash_table_find_str(&vm.globals, name->chars, name->len, name->hash) == NULL)
                {
                    hash_table_insert(&vm.globals, name, AS_BOOL(peek(0)), peek(1));
                    pop();
                    pop();
                }
                else
                {
                    runtime_error("Redefinition of global variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_DEFINE_GLOBAL_LONG:
            {
                ObjString* name = READ_STRING(8);
                if(hash_table_find_str(&vm.globals, name->chars, name->len, name->hash) == NULL)
                {
                    hash_table_insert(&vm.globals, name, AS_BOOL(peek(0)), peek(1));
                    pop();
                    pop();
                }
                else
                {
                    runtime_error("Redefinition of global variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
            case OP_GET_GLOBAL_BYTE:
            {
                ObjString* name = READ_STRING(1);
                Value value;
                if(!hash_table_get(&vm.globals, name, &value))
                {
                    runtime_error("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_GET_GLOBAL_SHORT:
            {
                ObjString* name = READ_STRING(2);
                Value value;
                if(!hash_table_get(&vm.globals, name, &value))
                {
                    runtime_error("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_GET_GLOBAL_WORD:
            {
                ObjString* name = READ_STRING(4);
                Value value;
                if(!hash_table_get(&vm.globals, name, &value))
                {
                    runtime_error("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_GET_GLOBAL_LONG:
            {
                ObjString* name = READ_STRING(8);
                Value value;
                if(!hash_table_get(&vm.globals, name, &value))
                {
                    runtime_error("Undefined variable '%s'", name->chars);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case OP_SET_GLOBAL_BYTE:
            {
                ObjString* name = READ_STRING(1);
                Value value = peek(0);
                switch(hash_table_is_const(&vm.globals, name))
                {
                    case 1:
                    {
                        hash_table_set(&vm.globals, name, value);
                        break;
                    }
                    case 2:
                    {
                        runtime_error("Assigning to constant '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    default:
                    {
                        runtime_error("Undefined variable '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_SET_GLOBAL_SHORT:
            {
                ObjString* name = READ_STRING(2);
                Value value = peek(0);
                switch(hash_table_is_const(&vm.globals, name))
                {
                    case 1:
                    {
                        hash_table_set(&vm.globals, name, value);
                        break;
                    }
                    case 2:
                    {
                        runtime_error("Assigning to constant '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    default:
                    {
                        runtime_error("Undefined variable '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_SET_GLOBAL_WORD:
            {
                ObjString* name = READ_STRING(4);
                Value value = peek(0);
                switch(hash_table_is_const(&vm.globals, name))
                {
                    case 1:
                    {
                        hash_table_set(&vm.globals, name, value);
                        break;
                    }
                    case 2:
                    {
                        runtime_error("Assigning to constant '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    default:
                    {
                        runtime_error("Undefined variable '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_SET_GLOBAL_LONG:
            {
                ObjString* name = READ_STRING(8);
                Value value = peek(0);
                switch(hash_table_is_const(&vm.globals, name))
                {
                    case 1:
                    {
                        hash_table_set(&vm.globals, name, value);
                        break;
                    }
                    case 2:
                    {
                        runtime_error("Assigning to constant '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                    default:
                    {
                        runtime_error("Undefined variable '%s'", name->chars);
                        return INTERPRET_RUNTIME_ERROR;
                    }
                }
                break;
            }
            case OP_GET_LOCAL_BYTE:
            {
                size_t slot = read_inst_index(1);
                push(vm.stack[slot]);
                break;
            }
            case OP_GET_LOCAL_SHORT:
            {
                size_t slot = read_inst_index(2);
                push(vm.stack[slot]);
                break;
            }
            case OP_GET_LOCAL_WORD:
            {
                size_t slot = read_inst_index(4);
                push(vm.stack[slot]);
                break;
            }
            case OP_GET_LOCAL_LONG:
            {
                size_t slot = read_inst_index(8);
                push(vm.stack[slot]);
                break;
            }
            case OP_SET_LOCAL_BYTE:
            {
                size_t slot = read_inst_index(1);
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_SET_LOCAL_SHORT:
            {
                size_t slot = read_inst_index(2);
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_SET_LOCAL_WORD:
            {
                size_t slot = read_inst_index(4);
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_SET_LOCAL_LONG:
            {
                size_t slot = read_inst_index(8);
                vm.stack[slot] = peek(0);
                break;
            }
            case OP_JUMP_IF_FALSE:
            {
                uint32_t offset = read_jump();
                if(IS_BOOL(peek(0)) && AS_BOOL(peek(0)) == false)
                {
                    vm.ip += offset;
                }
                break;
            }
            case OP_JUMP:
            {
                uint32_t offset = read_jump();
                vm.ip += offset;
                break;
            }
            case OP_LOOP:
            {
                uint32_t offset = read_jump();
                vm.ip -= offset;
                break;
            }
            default:
            {
                runtime_error("Unknown instruction %u", inst);
                return INTERPRET_RUNTIME_ERROR;
            }
        }
    }
}

#undef READ_INST
#undef READ_STRING

InterpretResult interpret(const char* src)
{
    Chunk chunk;
    init_chunk(&chunk);
    if(!compile(src, &chunk))
    {
        free_chunk(&chunk);
        return INTERPRET_COMPILE_ERROR;
    }
    vm.chunk = &chunk;
    vm.ip = vm.chunk->code;

    InterpretResult res = run();

    free_chunk(&chunk);
    return res;
}

void push(Value value)
{
    *vm.stack_top = value;
    vm.stack_top++;
}

Value pop()
{
    vm.stack_top--;
    return *vm.stack_top;
}

void free_vm()
{
    free_hash_table(&vm.globals);
    free_hash_table(&vm.strings);
    free_objs();
}
