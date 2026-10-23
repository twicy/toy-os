#include <task.h>
#include <mm/slab.h>
#include <mm/vmalloc.h>
#include <debug.h>
#include <string.h>
#include <stddef.h>

struct task_struct init_task = {
    .state = TASK_RUNNING,
    .pid = 0,
    .name = "main",
    .stack = NULL,
};

struct task_struct *current = &init_task;

static struct kmem_cache *task_cache;
static uint32_t next_pid = 1;

void init_tasks(void) {
    task_cache = kmem_cache_create("task_struct", sizeof(struct task_struct), 0);
    if (!task_cache)
        PANIC("no memory for the task_struct cache");
    INIT_LIST_HEAD(&init_task.run_list);
}

struct task_struct *alloc_task(const char *name) {
    struct task_struct *task = kmem_cache_alloc(task_cache);

    if (!task)
        return NULL;

    task->stack = vmalloc(THREAD_SIZE);
    if (!task->stack) {
        kmem_cache_free(task_cache, task);
        return NULL;
    }

    /* empty: the stack grows down from its top */
    task->esp = (uint32_t)task->stack + THREAD_SIZE;
    task->state = TASK_BLOCKED;
    task->pid = next_pid++;
    strlcpy(task->name, name, TASK_NAME_LEN);
    INIT_LIST_HEAD(&task->run_list);
    return task;
}

void free_task(struct task_struct *task) {
    ASSERT(task != current && task != &init_task);

    vfree(task->stack);
    kmem_cache_free(task_cache, task);
}
