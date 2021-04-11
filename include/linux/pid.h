#ifndef _LINUX_PID_H
#define _LINUX_PID_H

#include <linux/rcupdate.h>

// 枚举类型定义的 ID 类型不包括线程组 ID；
// 线程组 ID 无非是线程组组长的 PID，再单独定义一项是不必要的；
enum pid_type
{
    PIDTYPE_PID,
    PIDTYPE_PGID,
    PIDTYPE_SID,
    PIDTYPE_MAX
};

/*
 * What is struct pid?
 *
 * A struct pid is the kernel's internal notion of a process identifier.
 * It refers to individual tasks, process groups, and sessions.  While
 * there are processes attached to it the struct pid lives in a hash
 * table, so it and then the processes that it refers to can be found
 * quickly from the numeric pid value.  The attached processes may be
 * quickly accessed by following pointers from struct pid.
 *
 * Storing pid_t values in the kernel and refering to them later has a
 * problem.  The process originally with that pid may have exited and the
 * pid allocator wrapped, and another process could have come along
 * and been assigned that pid.
 *
 * Referring to user space processes by holding a reference to struct
 * task_struct has a problem.  When the user space process exits
 * the now useless task_struct is still kept.  A task_struct plus a
 * stack consumes around 10K of low kernel memory.  More precisely
 * this is THREAD_SIZE + sizeof(struct task_struct).  By comparison
 * a struct pid is about 64 bytes.
 *
 * Holding a reference to struct pid solves both of these problems.
 * It is small so holding a reference does not consume a lot of
 * resources, and since a new struct pid is allocated when the numeric pid
 * value is reused (when pids wrap around) we don't mistakenly refer to new
 * processes.
 */


/*
 * struct upid is used to get the id of the struct pid, as it is
 * seen in particular namespace. Later the struct pid is found with
 * find_pid_ns() using the int nr and struct pid_namespace *ns.
 */

// pid 的管理围绕着 2 个数据结构展开；

// struct upid 表示特定的命名空间中可见的信息；
struct upid {
    /* Try to keep pid_chain in the same cacheline as nr for find_pid */
    int                     nr;         // 表示 ID 的数值；
    struct pid_namespace    *ns;        // ns 是指向该 ID 所属的命名空间的指针；
    struct hlist_node       pid_chain;  // 使用内核的标准方法实现了散列溢出链表；
};

// struct pid 表示内核对 PID 的内部表示；
struct pid
{
    atomic_t            count;              // 引用计数器；
    /* lists of tasks that use this pid */
    // 使用该 pid 的进程的列表；
    struct hlist_head   tasks[PIDTYPE_MAX]; // 散列的表头，对应于 1 个 ID 类型；
                                            // 1 个 ID 可能用于几千个进程；
                                            // 所有共享同一给定 ID 的 task_struct 实例，都通过该列表连接起来；
                                            // PIDTYPE_MAX 表示 ID 类型的数目；
    struct rcu_head     rcu;
                                            // 1 个进程可以在多个命名空间中可见，而在各个命名空间中的局部 ID 各不相同；
    int                 level;              // 可以看到该进程的命名空间的数目；
                                            // 即包含该进程的命名空间在命名空间层次结构中的深度；
    struct upid         numbers[1];         // numbers 是 1 个 upid 实例的数组；
                                            // 每个数组项都对应 1 个命名空间；
                                            // 该数组形式上只有一个数组项，如果一个进程只包含在全局命名空间中，确实如此；
                                            // 由于该数组位于结构的末尾，因此，只要分配更多的内存空间，即可向数组添加附加的项；
};

extern struct pid init_struct_pid;

// 用于将 task_struct 连接到表头在 struct pid 中的散列表上；
struct pid_link
{
    struct hlist_node node;     // 用作散列元素；
    struct pid *pid;            // 指向进程所属的 pid 结构实例；
};

static inline struct pid *get_pid(struct pid *pid)
{
    if (pid)
        atomic_inc(&pid->count);
    return pid;
}

extern void FASTCALL(put_pid(struct pid *pid));
extern struct task_struct *FASTCALL(pid_task(struct pid *pid, enum pid_type));
extern struct task_struct *FASTCALL(get_pid_task(struct pid *pid,
                        enum pid_type));

extern struct pid *get_task_pid(struct task_struct *task, enum pid_type type);

/*
 * attach_pid() and detach_pid() must be called with the tasklist_lock
 * write-held.
 */
extern int FASTCALL(attach_pid(struct task_struct *task,
                enum pid_type type, struct pid *pid));
extern void FASTCALL(detach_pid(struct task_struct *task, enum pid_type));
extern void FASTCALL(transfer_pid(struct task_struct *old,
                  struct task_struct *new, enum pid_type));

struct pid_namespace;
extern struct pid_namespace init_pid_ns;

/*
 * look up a PID in the hash table. Must be called with the tasklist_lock
 * or rcu_read_lock() held.
 *
 * find_pid_ns() finds the pid in the namespace specified
 * find_pid() find the pid by its global id, i.e. in the init namespace
 * find_vpid() finr the pid by its virtual id, i.e. in the current namespace
 *
 * see also find_task_by_pid() set in include/linux/sched.h
 */
extern struct pid *FASTCALL(find_pid_ns(int nr, struct pid_namespace *ns));
extern struct pid *find_vpid(int nr);
extern struct pid *find_pid(int nr);

/*
 * Lookup a PID in the hash table, and return with it's count elevated.
 */
extern struct pid *find_get_pid(int nr);
extern struct pid *find_ge_pid(int nr, struct pid_namespace *);

extern struct pid *alloc_pid(struct pid_namespace *ns);
extern void FASTCALL(free_pid(struct pid *pid));
extern void zap_pid_ns_processes(struct pid_namespace *pid_ns);

/*
 * the helpers to get the pid's id seen from different namespaces
 *
 * pid_nr()    : global id, i.e. the id seen from the init namespace;
 * pid_vnr()   : virtual id, i.e. the id seen from the namespace this pid
 *               belongs to. this only makes sence when called in the
 *               context of the task that belongs to the same namespace;
 * pid_nr_ns() : id seen from the ns specified.
 *
 * see also task_xid_nr() etc in include/linux/sched.h
 */

static inline pid_t pid_nr(struct pid *pid)
{
    pid_t nr = 0;
    if (pid)
        nr = pid->numbers[0].nr;
    return nr;
}

pid_t pid_nr_ns(struct pid *pid, struct pid_namespace *ns);

static inline pid_t pid_vnr(struct pid *pid)
{
    pid_t nr = 0;
    if (pid)
        nr = pid->numbers[pid->level].nr;
    return nr;
}

#define do_each_pid_task(pid, type, task)               \
    do {                                \
        struct hlist_node *pos___;              \
        if (pid != NULL)                    \
            hlist_for_each_entry_rcu((task), pos___,    \
                &pid->tasks[type], pids[type].node) {

#define while_each_pid_task(pid, type, task)                \
            }                       \
    } while (0)

#endif /* _LINUX_PID_H */
