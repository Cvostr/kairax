#include "acpi_namespace.h"
#include "aml.h"
#include "mem/kheap.h"
#include "kairax/string.h"

// глобальный указатель на корневую namespace. больше одной все равно не бывает
static struct acpi_namespace *acpi_root_namespace = NULL;

struct ns_node *new_ns_node()
{
    struct ns_node *res = kmalloc(sizeof(struct ns_node));
    if (res == NULL)
        return NULL;
    memset(res, 0, sizeof(struct ns_node));
    return res;
}

struct acpi_namespace *acpi_make_root_ns()
{
    struct acpi_namespace *ns = kmalloc(sizeof(struct acpi_namespace));
    ns->root = new_ns_node();
    
    struct aml_name_string *name = kmalloc(sizeof(struct aml_name_string) + 4);
    name->segments_num = 1;
    name->base = 0;
    name->from_root = 1;

    // '\_GPE' - General events in GPE register block.
    memcpy(name->segments[0].seg_s, "_GPE", 4);
    acpi_ns_add_named_object(ns, NULL, name, NULL);

    // '\_PR'
    memcpy(name->segments[0].seg_s, "_PR_", 4);
    acpi_ns_add_named_object(ns, NULL, name, NULL);

    // '\_SB'
    memcpy(name->segments[0].seg_s, "_SB_", 4);
    acpi_ns_add_named_object(ns, NULL, name, NULL);

    // '\_SI'
    memcpy(name->segments[0].seg_s, "_SI_", 4);
    acpi_ns_add_named_object(ns, NULL, name, NULL);

    // '\_SI'
    memcpy(name->segments[0].seg_s, "_TZ_", 4);
    acpi_ns_add_named_object(ns, NULL, name, NULL);

    acpi_root_namespace = ns;

    kfree(name);

    return ns;
}

struct acpi_namespace *acpi_get_root_ns()
{
    return acpi_root_namespace;
}

void ns_node_intrusive_list_add(struct ns_node_intrusive_list *list, struct ns_node *node)
{
    if (node->prev != NULL || node->next != NULL) {
        // Если элемент уже в каком либо списке - выходим
        return;
    }

    if (list->head == NULL) {
        // Первого элемента не существует
        list->head = node;
        node->prev = NULL;
    } else {
        list->tail->next = node;
        node->prev = list->tail;
    }

    // Добавляем в конец - следующего нет
    node->next = NULL;

    list->tail = node;
}

struct ns_node *ns_node_find_child_with_name(struct ns_node *node, const char *name)
{
    struct ns_node* cur = node->children.head;
    //printk("ns_node_find_child_with_name: cur %p\n", cur);

    while (cur != NULL)
    {
        if (memcmp(cur->name, name, 4) == 0)
        {
            return cur;
        }

        cur = cur->next;
    }

    return NULL;
}

struct ns_node *resolve_parent(struct acpi_namespace *ns, struct ns_node *scope, struct aml_name_string *name, char *node_name)
{
    struct ns_node* cur = NULL;

    if (name->from_root == 1)
    {
        cur = ns->root;

        if (name->segments_num == 0)
        {
            // мы обратились к корневой node
            return ns->root;
        }
    }

    cur = scope;
    if (cur == NULL)
        cur = ns->root;

    // Обработаем смещения назад '^'
    for (int i = 0; i < name->base; i ++)
    {
        cur = cur->parent;
    }

    if (name->segments_num == 0)
    {
        // мы обратились по пустой строке, такого быть не должно
        return NULL;
    }

    int seg_i;
    for (seg_i = 0; seg_i < name->segments_num - 1; seg_i ++)
    {
        //printk("processing seg %s\n", name->segments[seg_i].seg_s);
        cur = ns_node_find_child_with_name(cur, name->segments[seg_i].seg_s);
        if (cur == NULL)
        {
            return NULL;
        }
    }

    memcpy(node_name, name->segments[seg_i].seg_s, 4);

    return cur;
}

int acpi_ns_add_named_object(struct acpi_namespace *ns, struct ns_node *scope, struct aml_name_string *name, struct aml_node *node)
{
    //printk("ACPI: NS adding object %s to scope %s\n", name->segments[0].seg_s, scope->name);
    char new_node_name[4];

    struct ns_node *parent = resolve_parent(ns, scope, name, new_node_name);
    if (parent == NULL)
    {
        //printk("NO PARENT!!!\n");
        return -ENOENT;
    }

    struct ns_node *nsnode = new_ns_node();
    nsnode->object = node;
    memcpy(nsnode->name, new_node_name, 4);

    ns_node_intrusive_list_add(&parent->children, nsnode);
    nsnode->parent = parent;

    return 0;
}

struct ns_node *acpi_ns_get_node(struct acpi_namespace *ns, struct ns_node *scope, struct aml_name_string *name)
{
    /*
    printk("acpi_ns_get_node: name (root=%i, base=%i, segments=%i, seg=%s)\n", 
        name->from_root, name->base, name->segments_num, name->segments > 0 ? name->segments->seg_s : "empty");
    */
    struct ns_node* cur = NULL;

    if (name->from_root == 1)
    {
        cur = ns->root;

        if (name->segments_num == 0)
        {
            // мы обратились к корневой node
            return ns->root;
        }
    }

    cur = scope;
    if (cur == NULL)
        cur = ns->root;

    // Обработаем смещения назад '^'
    for (int i = 0; i < name->base; i ++)
    {
        cur = cur->parent;
    }

    if (name->segments_num == 0)
    {
        // мы обратились по пустой строке, такого быть не должно
        return NULL;
    }

    int seg_i;
    for (seg_i = 0; seg_i < name->segments_num; seg_i ++)
    {
        //printk("processing seg %s\n", name->segments[seg_i].seg_s);
        cur = ns_node_find_child_with_name(cur, name->segments[seg_i].seg_s);
        if (cur == NULL)
        {
            return NULL;
        }
    }

    return cur;
}