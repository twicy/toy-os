#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/* Linux style doubly linked list */
struct list_head {
    struct list_head *prev;
    struct list_head *next;
};

#define container_of(ptr, type, member) \
    ((type *)((char *)(ptr) - __builtin_offsetof(type, member)))
#define list_entry(ptr, type, member) container_of(ptr, type, member)

/* The linked list starts with a dummy entry, so iterations starts with
 * head->next, which is the real first entry
 */
#define list_first_entry(head, type, member) list_entry((head)->next, type, member)
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)
#define list_for_each_entry(pos, head, member) \
    for (pos = list_first_entry(head, __typeof__(*pos), member); \
         &pos->member != (head); \
         pos = list_entry(pos->member.next, __typeof__(*pos), member))

static inline void INIT_LIST_HEAD(struct list_head *head) {
    head->prev = head;
    head->next = head;
}

static inline int list_empty(const struct list_head *head) {
    return head->next == head;
}

static inline void __list_add(struct list_head *entry, struct list_head *prev,
                              struct list_head *next) {
    next->prev = entry;
    entry->next = next;
    entry->prev = prev;
    prev->next = entry;
}

/* Adding to after the dummy entry */
static inline void list_add(struct list_head *entry, struct list_head *head) {
    __list_add(entry, head, head->next);
}

static inline void list_add_tail(struct list_head *entry, struct list_head *head) {
    __list_add(entry, head->prev, head);
}

static inline void list_del(struct list_head *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    entry->prev = NULL;
    entry->next = NULL;
}

static inline void list_del_init(struct list_head *entry) {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
    INIT_LIST_HEAD(entry);
}

#endif