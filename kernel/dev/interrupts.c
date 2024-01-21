#include "interrupts.h"
#include "list/list.h"
#include "mem/kheap.h"
#include "kairax/string.h"
#include "sync/spinlock.h"

list_t interrupts = {0,};
spinlock_t interrupts_lock = 0;

int is_irq_free(int irq) 
{
    struct list_node* current_node = interrupts.head;
    struct interrupt* intrpt = NULL;

    for (size_t i = 0; i < interrupts.size; i++) {
        
        intrpt = (struct interrupt*) current_node->element;

        if (intrpt->irq == irq) {
            return 0;
        }
        
        // Переход на следующий элемент
        current_node = current_node->next;
    }

    return 1;
}

int get_free_irq()
{
    int start_irq = 20;
    int max_irq = 128;
    for (int i = start_irq; i < max_irq; i ++) {
        if (is_irq_free(i)) {
            return i;
        }
    }

    return -1;
}

int alloc_irq(int type, const char* desc)
{
    int irq = -1;
    acquire_spinlock(&interrupts_lock);

    irq = get_free_irq();
    if (irq == -1) {
        goto exit;
    }

    struct interrupt* interpt = kmalloc(sizeof(struct interrupt));
    interpt->irq = irq;
    interpt->type = type;
    interpt->desc = strdup(desc);
    list_add(&interrupts, interpt);

exit:
    release_spinlock(&interrupts_lock);
    return irq;
}