#ifndef AML_TYPES_H
#define AML_TYPES_H

#include "kairax/types.h"
#include "sync/semaphore.h"

struct aml_name_string {
    int from_root;
    uint32_t base;
    size_t segments_num;
    union {
        uint32_t seg_i;
        uint8_t seg_s[4];
    } segments[];
};

enum aml_node_type {
    NONE,
    INTEGER,
    METHOD,
    PACKAGE,
    BUFFER,
    STRING,
    DEVICE,
    MUTEX,
    FIELD,
    OP_REGION
};

enum aml_field_type {
    DEFAULT,
    INDEX,
    BANK
};

struct aml_node {
    enum aml_node_type type;

    union {
        uint64_t int_value;

        struct {
            char *str;
        } string;

        struct {
            uint8_t region_space;
            uint64_t offset;
            uint64_t length;
        } opregion;

        struct {
            uint8_t args;
            uint8_t serialized; // synchronized
            uint8_t sync_level;
            // todo: term list
        } method;

        struct
        {
            size_t num_elements;
            struct aml_node **elements;
        } package;

        struct {
            size_t len;
            uint8_t *buffer;
        } buffer;

        struct {
            uint8_t space;
            uint64_t offset;
            uint64_t len;
        } op_region;

        struct {
            enum aml_field_type type;
            uint64_t offset;
            uint32_t len;
            uint8_t flags;

            union {
                struct aml_node *opregion;

                struct {
                    struct aml_node *index;
                    struct aml_node *data;
                } index;

                // TODO: Bank
            } mem;
            
        } field;

        struct {
            uint8_t flags;
            struct semaphore sem;
        } mutex;
    };
};

#endif