#ifndef KFUNCTIONS_TABLE_H
#define KFUNCTIONS_TABLE_H

struct kernel_function {
    const char* name;
    void*       func_ptr;
};

void* kfunctions_get_by_name(const char* name); 

#endif