#ifndef TASK_H
#define TASK_H
#include <stdint.h>
#include <list.h>
#include <kernel.h>

#define THREAD_SIZE (2 * PAGE_SIZE)
#define TASK_NAME_LEN 16

enum task_state {
    TASK_RUNNING,   /* it is current */
    TASK_READY,     /* waiting on the run queue for the CPU */
    TASK_BLOCKED,   /* waiting for something else, not runnable */
    TASK_DEAD,      /* exited, waiting for another thread to free its stack */
};

struct task_struct {
    /* saved stack pointer while not running */
    uint32_t esp;
    enum task_state state;
    uint32_t pid;
    char name[TASK_NAME_LEN];
    /* lowest address of the kernel stack, NULL for init_task */
    void *stack;
    /* run queue while TASK_READY, a wait queue while TASK_BLOCKED */
    struct list_head run_list;
};

_Static_assert(__builtin_offsetof(struct task_struct, esp) == 0, "switch_to.S uses [task] for task->esp");

struct switch_frame {
    uint32_t edi, esi, ebx, ebp;
    /* The same return address that switch_to ends with */
    uint32_t ret;
};

extern struct task_struct init_task;
extern struct task_struct *current;

void init_tasks(void);
struct task_struct *alloc_task(const char *name);
void free_task(struct task_struct *task);

struct task_struct *switch_to(struct task_struct *prev, struct task_struct *next);

struct task_struct *kthread_create(void (*fn)(void *), void *arg, const char *name);
void context_switch(struct task_struct *next);

#endif
