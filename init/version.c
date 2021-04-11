/*
 *  linux/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *
 *  May be freely distributed as part of Linux.
 */

#include <linux/compile.h>
#include <linux/module.h>
#include <linux/uts.h>
#include <linux/utsname.h>
#include <linux/utsrelease.h>
#include <linux/version.h>

#define version(a) Version_ ## a
#define version_string(a) version(a)

int version_string(LINUX_VERSION_CODE);

// 初始设置；
// 相关的预处理常数在内核中各处进行定义；
// 例如 UTS_RELEASE 在 utsrelease.h 中定义，该文件在链接时通过顶层 Makefile 动态生成；
// uts 的某些结构不能修改，如：将 sysname 换成 Linux 以外的其他值时无意义的，但机器名 machine 可以进行修改；
struct uts_namespace init_uts_ns = {
    .kref = {
        .refcount   = ATOMIC_INIT(2),
    },
    .name = {
        .sysname    = UTS_SYSNAME,
        .nodename   = UTS_NODENAME,
        .release    = UTS_RELEASE,
        .version    = UTS_VERSION,
        .machine    = UTS_MACHINE,
        .domainname = UTS_DOMAINNAME,
    },
};
EXPORT_SYMBOL_GPL(init_uts_ns);

/* FIXED STRINGS! Don't touch! */
const char linux_banner[] =
    "Linux version " UTS_RELEASE " (" LINUX_COMPILE_BY "@"
    LINUX_COMPILE_HOST ") (" LINUX_COMPILER ") " UTS_VERSION "\n";

const char linux_proc_banner[] =
    "%s version %s"
    " (" LINUX_COMPILE_BY "@" LINUX_COMPILE_HOST ")"
    " (" LINUX_COMPILER ") %s\n";
