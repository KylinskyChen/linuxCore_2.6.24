#ifndef _LINUX_PID_NS_H
#define _LINUX_PID_NS_H

#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/threads.h>
#include <linux/nsproxy.h>
#include <linux/kref.h>

struct pidmap {
       atomic_t nr_free;
       void *page;
};

#define PIDMAP_ENTRIES         ((PID_MAX_LIMIT + 8*PAGE_SIZE - 1)/PAGE_SIZE/8)

// PID 命名空间的表示方式；
// PID 分配器需要依靠该结构的某些部分生成唯一的 ID；
struct pid_namespace {
    struct kref             kref;
    struct pidmap           pidmap[PIDMAP_ENTRIES];
    int                     last_pid;
    // 每个 PID 命名空间都具有一个进程；
    // 其作用相当于全局的 init 进程；
    // init 的一个目的是对孤儿进程调用 wait4；
    // 命名空间局部的 init 变体也必须完成该工作；
    // child_reaper 保存了指向该进程的 task_struct 指针；
    struct task_struct      *child_reaper;
    struct kmem_cache       *pid_cachep;
    int                     level;
    // parent 是指向父命名空间的指针；
    // 层次表示当前命名空间在命名空间层次结构中的深度；
    // 初始命名空间的 level 为 0；
    // level 较高的命名空间的 ID，对 level 较低命名空间来说是可见的；
    struct pid_namespace    *parent;
#ifdef CONFIG_PROC_FS
    struct vfsmount         *proc_mnt;
#endif
};

extern struct pid_namespace init_pid_ns;

#ifdef CONFIG_PID_NS
static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
    if (ns != &init_pid_ns)
        kref_get(&ns->kref);
    return ns;
}

extern struct pid_namespace *copy_pid_ns(unsigned long flags, struct pid_namespace *ns);
extern void free_pid_ns(struct kref *kref);

static inline void put_pid_ns(struct pid_namespace *ns)
{
    if (ns != &init_pid_ns)
        kref_put(&ns->kref, free_pid_ns);
}

#else /* !CONFIG_PID_NS */
#include <linux/err.h>

static inline struct pid_namespace *get_pid_ns(struct pid_namespace *ns)
{
    return ns;
}

static inline struct pid_namespace *
copy_pid_ns(unsigned long flags, struct pid_namespace *ns)
{
    if (flags & CLONE_NEWPID)
        ns = ERR_PTR(-EINVAL);
    return ns;
}

static inline void put_pid_ns(struct pid_namespace *ns)
{
}

#endif /* CONFIG_PID_NS */

static inline struct pid_namespace *task_active_pid_ns(struct task_struct *tsk)
{
    return tsk->nsproxy->pid_ns;
}

static inline struct task_struct *task_child_reaper(struct task_struct *tsk)
{
    BUG_ON(tsk != current);
    return tsk->nsproxy->pid_ns->child_reaper;
}

#endif /* _LINUX_PID_NS_H */
