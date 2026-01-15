#ifndef _ACPI_NAMESPACE_H
#define _ACPI_NAMESPACE_H

#include "aml_types.h"

struct ns_node_intrusive_list {
    struct ns_node *head;
    struct ns_node *tail;
};

struct ns_node {
    char name[4];
    struct aml_node *object;

    struct ns_node *parent;
    struct ns_node *prev;
    struct ns_node *next;

    struct ns_node_intrusive_list children;
};

struct acpi_namespace {
    struct ns_node *root;
};

void ns_node_intrusive_list_add(struct ns_node_intrusive_list *list, struct ns_node *node);
struct ns_node *ns_node_find_child_with_name(struct ns_node *node, const char *name);

struct acpi_namespace *acpi_make_root_ns();
struct acpi_namespace *acpi_get_root_ns();


int acpi_ns_add_named_object(struct acpi_namespace *ns, struct ns_node *scope, struct aml_name_string *name, struct aml_node *node);
int acpi_ns_add_named_object1(struct acpi_namespace *ns, struct ns_node *scope, const char *name, struct aml_node *node);

struct ns_node *acpi_ns_get_node(struct acpi_namespace *ns, struct ns_node *scope, struct aml_name_string *name);

#endif