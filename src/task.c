#include <task.h>
#include <mm/slab.h>
#include <mm/vmalloc.h>
#include <irqflags.h>
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

/* TASK_READY threads, next to run first. init_task is never on it */
static struct list_head run_queue;

static struct kmem_cache *task_cache;
static uint32_t next_pid = 1;

void init_tasks(void) {
    task_cache = kmem_cache_create("task_struct", sizeof(struct task_struct), 0);
    if (!task_cache)
        PANIC("no memory for the task_struct cache");
    INIT_LIST_HEAD(&init_task.run_list);
    INIT_LIST_HEAD(&run_queue);
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

/* Dummy frame for new kthreads */
struct kthread_frame {
    struct switch_frame sw;
    uint32_t entry_ret;
    void (*fn)(void *);
    void *arg;
};

/* the first code a new thread runs */
static void kthread_entry(void (*fn)(void *), void *arg) {
    irq_enable();
    fn(arg);
    /* exiting means switching away for good and having someone else free
     * this stack, which needs the scheduler to pick that someone */
    PANIC("kthread %s returned, exit needs the scheduler", current->name);
}

struct task_struct *kthread_create(void (*fn)(void *), void *arg, const char *name) {
    struct task_struct *task = alloc_task(name);
    struct kthread_frame *frame;
    uint32_t flags;

    if (!task)
        return NULL;

    frame = (struct kthread_frame *)task->esp - 1;
    *frame = (struct kthread_frame){
        .sw.ret = (uint32_t)kthread_entry,
        .fn = fn,
        .arg = arg,
    };
    task->esp = (uint32_t)frame;
    task->state = TASK_READY;

    flags = irq_save();
    list_add_tail(&task->run_list, &run_queue);
    irq_restore(flags);
    return task;
}

/* only schedule() picks next, so every thread switched to came off the run queue */
static void context_switch(struct task_struct *next) {
    struct task_struct *prev = current;
    uint32_t flags;

    if (next == prev)
        return;
    ASSERT(next->state == TASK_READY);
    ASSERT(!next->stack || (next->esp >= (uint32_t)next->stack &&
                            next->esp < (uint32_t)next->stack + THREAD_SIZE));

    /* This is for later, where the timer starts preempting */
    /* We do not want to mess up with switch_to */
    flags = irq_save();

    if (prev->state == TASK_RUNNING)
        prev->state = TASK_READY;
    next->state = TASK_RUNNING;
    current = next;
    switch_to(prev, next);

    irq_restore(flags);
}

/* Round robin. A caller still TASK_RUNNING only gives up the CPU and goes to
 * the back of the queue, one that set itself TASK_BLOCKED first stays off it.
 * With nothing ready, init_task runs */
void schedule(void) {
    struct task_struct *next = &init_task;
    uint32_t flags = irq_save();

    if (current->state == TASK_RUNNING && current != &init_task)
        list_add_tail(&current->run_list, &run_queue);
    if (!list_empty(&run_queue)) {
        next = list_first_entry(&run_queue, struct task_struct, run_list);
        list_del_init(&next->run_list);
    }
    context_switch(next);

    irq_restore(flags);
}

/* What main() turns into once it is done setting up */
void cpu_idle(void) {
    ASSERT(current == &init_task);

    for (;;) {
        /* IRQs off from the check to the hlt, or an IRQ waking a thread in
         * between would be slept through until the next IRQ */
        irq_disable();
        if (list_empty(&run_queue))
            safe_halt();
        else
            irq_enable();
        schedule();
    }
}
