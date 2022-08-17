#ifndef QP_HEADER_INCLUDED
#define QP_HEADER_INCLUDED

/** \file qp.h
 *
 * \brief QP: Quick Print library.
 *
 * This is a header-only library intended to be quickly dropped into a project
 * in order to show useful prints without relying on whatever logging library is
 * already in place.
 *
 * For that purpose this library has it's own implementation of "file:line"
 * headers, timestamps and ratelimiting.
 *
 * Minimal configuration is supported by defining certain macros before
 * including "qp.h". For example if "QP_RATELIMIT_INTERVAL" is already defined
 * then that value is used instead of the default.
 *
 * The following macros are useful to define ahead of time:
 * - #QP_PRINT: This should be printf-like function that shows output somewhere useful
 * - #QP_NL: If the underlying logging mechanism inserts newlines by itself this
 *      should be defined to "".
 * - #QP_RATELIMIT_INTERVAL
 * - #QP_TIME_HEADER
 * - #qp_militime_now
 * - #qp_nanotime_now
 */

/** Default interval (in miliseconds) for rate-limited prints */
#ifndef QP_RATELIMIT_INTERVAL
    #define QP_RATELIMIT_INTERVAL 1000
#endif

#if defined(__KERNEL__) && !defined(__UBOOT__)
    #define QP_PROJECT_LINUX_KERNEL
#endif

#ifndef QP_NO_AUTO_INCLUDE
    #if defined(QP_PROJECT_LINUX_KERNEL)
        #include <linux/version.h>
    #elif defined(__GLIBC__)
        #include <stdio.h>
        #include <stdlib.h>
        #include <errno.h>
        #include <sys/time.h>
        #include <execinfo.h>
        #include <pthread.h>
        #include <linux/if_packet.h>
    #endif
#endif

/**
 * End-of-line terminator.
 *
 * This can be defined to "" if QP_PRINT handles newlines internally.
 */
#ifndef QP_NL
    #define QP_NL "\n"
#endif

/** Continuation marker.
 *
 * This is inserted at the start of QP_PRINT calls in order to avoid starting on
 * a newline. It is empty by default.
 */
#ifndef QP_CONT
    #if defined(QP_PROJECT_LINUX_KERNEL)
        #define QP_CONT KERN_CONT
    #else
        #define QP_CONT ""
    #endif
#endif

/** Print implementation without any output.
 *
 * This can be used to quickly silence prints inside a translation unit.
 */
#define QP_PRINT_IMPL_NULL(str, ...) do { \
    } while (0)

/** Print implementation using a default printf */
#define QP_PRINT_IMPL_PRINTF(str, ...) do { \
        printf(str, ## __VA_ARGS__); \
    } while (0)

/** Print implementation using stderr (userspace default) */
#define QP_PRINT_IMPL_STDERR(str, ...) do { \
        fprintf(stderr, str, ## __VA_ARGS__); \
    } while (0)

/** Print implementation using stdout */
#define QP_PRINT_IMPL_STDOUT(str, ...) do { \
        fprintf(stdout, str, ## __VA_ARGS__); \
    } while (0)

/** Print implementation using /dev/console device
 *
 * This will result in userspace output being combined with that of the kernel.
 * The /dev/console device is opened and close for each print statement.
 */
#define QP_PRINT_IMPL_DEV_CONSOLE(str, ...) do { \
        FILE *fp = fopen("/dev/console", "a"); \
        if (fp) { \
            fprintf(fp, str, ## __VA_ARGS__); \
            fclose(fp); \
        } \
    } while (0)

/** Print implementation using standard linux printk */
#define QP_PRINT_IMPL_LINUX_KERNEL(str, ...) printk(str, ## __VA_ARGS__)
/** Print implementation using linux trace_printk */
#define QP_PRINT_IMPL_LINUX_KERNEL_TRACE(str, ...) trace_printk(str, ## __VA_ARGS__)
/** Print implementation using linux early_printk */
#define QP_PRINT_IMPL_LINUX_KERNEL_EARLY(str, ...) early_printk(str, ## __VA_ARGS__)

/** Main print function
 *
 * By default this is defined to either #QP_PRINT_IMPL_STDERR or
 * #QP_PRINT_IMPL_LINUX_KERNEL.
 *
 * This can be overriden by explicitly defining QP_PRINT to one of the
 * QP_PRINT_IMPL functions before including qp.h
 */
#ifdef QP_PRINT
    /* external */

#elif defined(__KERNEL__)
    /* kernel default */
    #define QP_PRINT QP_PRINT_IMPL_LINUX_KERNEL
#else
    /* userspace default */
    #define QP_PRINT QP_PRINT_IMPL_STDERR
#endif

#define QP_TIME_HEADER_NONE 0
#define QP_TIME_HEADER_4_3 1
#define QP_TIME_HEADER_5_6 2

/** Header timing configuration */
#ifndef QP_TIME_HEADER
    #define QP_TIME_HEADER QP_TIME_HEADER_NONE
#endif

#if QP_TIME_HEADER == QP_TIME_HEADER_5_6
    #define QP_PRINT_LOC(str, ...) do { \
            qp_nanotime_t nanonow = qp_nanotime_now(); \
            QP_PRINT("[%05lu.%06lu] %s(%d): " str, \
                    ((unsigned long)nanonow) / 1000000000 % 100000, \
                    ((unsigned long)nanonow) / 1000 % 1000000, \
                    __FUNCTION__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#elif QP_TIME_HEADER == QP_TIME_HEADER_4_3
    #define QP_PRINT_LOC(str, ...) do { \
            qp_militime_t milinow = qp_militime_now(); \
            QP_PRINT("[%04lu.%03lu] %s(%d): " str, \
                    ((unsigned long)milinow) / 1000 % 10000, \
                    ((unsigned long)milinow) % 1000, \
                    __FUNCTION__, __LINE__, ## __VA_ARGS__); \
    } while (0)
#else
    #define QP_PRINT_LOC(str, ...) \
            QP_PRINT("%s(%d): " str, \
                    __FUNCTION__, __LINE__, ## __VA_ARGS__)
#endif

/** Print source code location without any other message. */
#define QP_TRACE() QP_PRINT_LOC("trace" QP_NL)

/** Macro which evaluates as true only once it it's scope (not thread-safe). */
#define QP_ONCE() ({ \
            static int once_called; \
            int r = once_called; \
            once_called = 1; \
            !r; \
        })

/** QP_PRINT but only once (based on #QP_ONCE) */
#define QP_PRINT_ONCE(...) do { \
        if (QP_ONCE()) \
            QP_PRINT(__VA_ARGS__); \
    } while (0)

/** QP_PRINT_LOC but only once (based on #QP_ONCE) */
#define QP_PRINT_LOC_ONCE(...) do { \
        if (QP_ONCE()) \
            QP_PRINT_LOC(__VA_ARGS__); \
    } while (0)

#ifdef QP_PROJECT_LINUX_KERNEL
    #if !defined(LINUX_VERSION_CODE)
        #warning Defined __KERNEL__ but no LINUX_VERSION_CODE, please include <linux/version.h>
        #define QP_KERNEL_VERSION_OLDER_THAN(a, b, c) 0
    #else
        #define QP_KERNEL_VERSION_OLDER_THAN(a, b, c) LINUX_VERSION_CODE < KERNEL_VERSION(a, b, c)
    #endif
#endif

/* Milisecond timing: */
typedef unsigned long qp_militime_t;

#define qp_militime_now_ktime() ({ \
        u64 _nowns = ktime_get_ns(); \
        do_div(_nowns, 1000000); \
        ((qp_militime_t)_nowns); \
    })

#ifdef qp_militime_now
    /* external */
#elif defined(__KERNEL__)
    /* kernel default */
    #define qp_militime_now() (((jiffies) * 1000) / HZ)
#else
    /* userspace default */
    #define qp_militime_now() ({ \
            struct timeval tv; \
            gettimeofday(&tv, 0); \
            ((qp_militime_t)tv.tv_sec * 1000lu + (tv.tv_usec / 1000)); \
    })
#endif

/* High-precission low-overhead nanosecond timing: */
#ifdef qp_nanotime_now
    /* external */
#elif defined(QP_PROJECT_LINUX_KERNEL)
    #ifndef QP_NO_AUTO_INCLUDE
        #include <linux/hrtimer.h>
    #endif
    typedef s64 qp_nanotime_t;
    /* Based on generic ktime. */
    #define qp_nanotime_now() (ktime_to_ns(ktime_get_real()))
#else
    typedef unsigned long long qp_nanotime_t;
    /* userspace default */
    #define qp_nanotime_now() ({ \
            struct timeval tv; \
            gettimeofday(&tv, NULL); \
            (((qp_nanotime_t)tv.tv_sec) * 1000000000LLU + (tv.tv_usec * 1000LLU)); \
    })
#endif

#ifdef QP_NO_LOCKS
    /* Nothing .*/
#elif defined(__KERNEL__)
    #if !defined(CONFIG_SMP)
        #define QP_NO_LOCKS
    #elif defined(DEFINE_SPINLOCK)
        #define QP_LOCK_DEFINE(lock) DEFINE_SPINLOCK((lock))
    #elif defined(SPIN_LOCK_UNLOCKED)
        #define QP_LOCK_DEFINE(lock) spinlock_t lock = SPIN_LOCK_UNLOCKED
    #else
        #warning Kernel spinlocks not supported
        #define QP_NO_LOCKS
    #endif

    #ifndef QP_NO_LOCKS
        #define QP_LOCK(lock) unsigned long flags##lock; spin_lock_irqsave(&lock, flags##lock)
        #define QP_UNLOCK(lock) spin_unlock_irqrestore(&lock, flags##lock)
    #endif
#elif defined(_POSIX_THREADS)
    #define QP_LOCK_DEFINE(lock) pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER
    #define QP_LOCK(lock) pthread_mutex_lock(&lock)
    #define QP_UNLOCK(lock) pthread_mutex_unlock(&lock)
#elif !defined(QP_LOCK_DEFINE)
    //#warning "locking not supported"
    #define QP_NO_LOCKS
#endif

#ifdef QP_NO_LOCKS
    #define QP_LOCK_DEFINE(lock) struct { } lock __attribute__((unused))
    #define QP_LOCK(lock)
    #define QP_UNLOCK(lock)
#endif

#ifndef unlikely
    #define unlikely(x) __builtin_expect(!!(x), 0)
#endif
#ifndef likely
    #define likely(x) __builtin_expect(!!(x), 1)
#endif

#if !defined(do_div)
#define do_div(a, b) ({ \
        int r; \
        if (b) { \
            r = (a) % (b); \
            (a) /= (b);  \
        } else { \
            r = (a); \
        } \
        r; \
    })
#endif

/** Rate limiter which evaluates as "true" once every "delta" miliseconds.
 *
 * Rate limitation is separate for each scope using this macro.
 *
 * Returns 0 or number miliseconds passed.
 */
#define QP_RATELIMIT(delta) ({ \
            static qp_militime_t g_last_time; \
            static QP_LOCK_DEFINE(g_lock); \
            qp_militime_t now_time = qp_militime_now(); \
            unsigned long delta_ms; \
            unsigned long ret = 0; \
            \
            delta_ms = now_time - g_last_time; \
            if (unlikely(delta_ms > delta)) { \
                QP_LOCK(g_lock); \
                delta_ms = now_time - g_last_time; \
                if (likely(delta_ms > delta)) { \
                    g_last_time = now_time; \
                    ret = delta_ms; \
                } \
                QP_UNLOCK(g_lock); \
            } \
            ret; \
        })

/** Count calls to this location. */
#define QP_PRINT_RATELIMIT(str, ...) do { \
        static unsigned long long g_cnt = 0; \
        static unsigned long long g_last_cnt; \
        static QP_LOCK_DEFINE(g_lock); \
        int delta_ms; \
        QP_LOCK(g_lock); \
        ++g_cnt; \
        delta_ms = QP_RATELIMIT(QP_RATELIMIT_INTERVAL); \
        if (unlikely(delta_ms)) {\
            unsigned long long rate = ((g_cnt - g_last_cnt) * 1000000); \
            unsigned long long cnt = g_cnt; \
            unsigned long rate_mod; \
            g_last_cnt = g_cnt; \
            QP_UNLOCK(g_lock); \
            do_div(rate, delta_ms); \
            rate_mod = do_div(rate, 1000); \
            QP_PRINT_LOC("cnt=%llu rate=%llu.%03lu/s: " str, \
                    cnt, rate, rate_mod, \
                    ## __VA_ARGS__); \
        } else { \
            QP_UNLOCK(g_lock); \
        } \
    } while (0)

#define QP_DEFINE_PER_CPU(type, name) DEFINE_PER_CPU(type, name)
#define QP_PER_CPU_VAR(name) __get_cpu_var(name)

#define QP_RATELIMIT_PERCPU(delta) ({ \
            static QP_DEFINE_PER_CPU(qp_militime_t, g_last_time); \
            qp_militime_t now_time = qp_militime_now(); \
            unsigned long delta_ms; \
            delta_ms = now_time - QP_PER_CPU_VAR(g_last_time); \
            if (unlikely(delta_ms > delta)) { \
                QP_PER_CPU_VAR(g_last_time) = now_time; \
            } else { \
                delta_ms = 0; \
            } \
            delta_ms; \
        })

/** Count calls to this location on a per-cpu basis. */
#define QP_PRINT_RATELIMIT_PERCPU(str, ...) do { \
        static QP_DEFINE_PER_CPU(unsigned long long, g_cnt); \
        static QP_DEFINE_PER_CPU(unsigned long long, g_last_cnt); \
        int delta_ms; \
        ++QP_PER_CPU_VAR(g_cnt); \
        delta_ms = QP_RATELIMIT_PERCPU(QP_RATELIMIT_INTERVAL); \
        if (unlikely(delta_ms)) {\
            unsigned long long rate = ((QP_PER_CPU_VAR(g_cnt) - QP_PER_CPU_VAR(g_last_cnt)) * 1000000); \
            unsigned long long cnt = QP_PER_CPU_VAR(g_cnt); \
            unsigned long rate_mod; \
            QP_PER_CPU_VAR(g_last_cnt) = QP_PER_CPU_VAR(g_cnt); \
            do_div(rate, delta_ms); \
            rate_mod = do_div(rate, 1000); \
            QP_PRINT_LOC("cpu=%d cnt=%llu rate=%llu.%03lu/s: " str, \
                    smp_processor_id(), cnt, rate, rate_mod, \
                    ## __VA_ARGS__); \
        } \
    } while (0)

/* Count calls to this location: */
#define QP_PRINT_HIST_RATELIMIT(expr, maxval, str, ...) do { \
        static unsigned long long cnt[maxval]; \
        static unsigned long long last_cnt[maxval]; \
        static qp_militime_t last_time[maxval]; \
        unsigned int curval = expr; \
        qp_militime_t now_time = qp_militime_now(); \
        qp_militime_t delta_ms = now_time - last_time[curval]; \
        ++cnt[curval]; \
        if (unlikely(delta_ms > QP_RATELIMIT_INTERVAL)) { \
            unsigned long long rate = ((cnt[curval] - last_cnt[curval]) * 1000000); \
            unsigned long rate_mod; \
            do_div(rate, delta_ms); \
            rate_mod = do_div(rate, 1000); \
            QP_PRINT_LOC("cnt=%llu rate=%llu.%03lu/s: " str, \
                    cnt[curval], rate, rate_mod, \
                    ## __VA_ARGS__); \
            last_time[curval] = now_time; \
            last_cnt[curval] = cnt[curval]; \
        } \
    } while (0)

#define QP_TRACE_RATELIMIT() QP_PRINT_RATELIMIT("trace" QP_NL)

#define QP__BOOLFUNC_FMT(expr) \
            ((!!expr()) ? " "#expr : "")

#ifdef in_nmi
    #define QP_PREEMPT_COUNT_FMT__nmi "%s"
    #define QP_PREEMPT_COUNT_ARGS__nmi , QP__BOOLFUNC_FMT(in_nmi)
#else
    #define QP_PREEMPT_COUNT_FMT__nmi
    #define QP_PREEMPT_COUNT_ARGS__nmi
#endif

#ifdef in_serving_softirq
    #define QP_PREEMPT_COUNT_FMT__serving_softirq "%s"
    #define QP_PREEMPT_COUNT_ARGS__serving_softirq , QP__BOOLFUNC_FMT(in_serving_softirq)
#else
    #define QP_PREEMPT_COUNT_FMT__serving_softirq
    #define QP_PREEMPT_COUNT_ARGS__serving_softirq
#endif

#define QP_PREEMPT_COUNT_FMT "0x%08x%s%s%s%s%s" \
                    QP_PREEMPT_COUNT_FMT__nmi \
                    QP_PREEMPT_COUNT_FMT__serving_softirq \

#define QP_PREEMPT_COUNT_ARGS \
                    preempt_count(), \
                    QP__BOOLFUNC_FMT(irqs_disabled), \
                    QP__BOOLFUNC_FMT(in_atomic), \
                    QP__BOOLFUNC_FMT(in_interrupt), \
                    QP__BOOLFUNC_FMT(in_irq), \
                    QP__BOOLFUNC_FMT(in_softirq) \
                    QP_PREEMPT_COUNT_ARGS__nmi \
                    QP_PREEMPT_COUNT_ARGS__serving_softirq \


#define QP_DUMP_KERNEL_CONTEXT() \
            QP_PRINT_LOC("preempt_count=" QP_PREEMPT_COUNT_FMT "\n", \
                    QP_PREEMPT_COUNT_ARGS);

/* Stack dumping. */
#if defined(__KERNEL__)
    /* Kernel */
    #define QP_DUMP_STACK() dump_stack()
#else
    /* GLIBC */
    #define QP_DUMP_STACK() do { \
            int i, btlen; \
            void *bt[20]; \
            char **btsym; \
            btlen = backtrace(bt, 20); \
            btsym = backtrace_symbols(bt, btlen); \
            for (i = 0; i < btlen; ++i) { \
                QP_PRINT("[%d]: %s" QP_NL, i, btsym[i]); \
            } \
            free(btsym); \
        } while (0)
#endif

#define QP_DUMP_SYMBOL(ptr) do { \
        void *fakebt = (void*)(ptr); \
        char **names; \
        names = backtrace_symbols(&fakebt, 1); \
        if (names) { \
            QP_PRINT_LOC(#ptr "=%s" QP_NL, names[0]); \
            free(names); \
        } else { \
            QP_PRINT_LOC(#ptr "=%p (no symbol)" QP_NL, (void*)(ptr)); \
        } \
    } while (0)

#define QP_DUMP_STACK_RATELIMIT() do { \
        if (QP_RATELIMIT(5000)) { \
            QP_DUMP_STACK(); \
        } \
    } while (0)

/* Micro-profiling. */
#define QP_PROFILE_REGION_BEGIN() \
        static unsigned long long qp_profile_g_usage = 0, qp_profile_g_last_usage = 0; \
        static unsigned long long qp_profile_g_count = 0, qp_profile_g_last_count = 0; \
        static unsigned long long qp_profile_g_inst_max = 0; \
        static QP_LOCK_DEFINE(qp_profile_lock); \
        qp_nanotime_t qp_profile_begin_ns = qp_nanotime_now();

#define QP_PROFILE_REGION_END(str) do { \
        unsigned int delta_ms; \
        qp_nanotime_t qp_profile_end_ns = qp_nanotime_now(); \
        QP_LOCK(qp_profile_lock); \
        qp_profile_g_usage += (qp_profile_end_ns - qp_profile_begin_ns); \
        ++qp_profile_g_count; \
        if (qp_profile_end_ns - qp_profile_begin_ns > qp_profile_g_inst_max) { \
            qp_profile_g_inst_max = qp_profile_end_ns - qp_profile_begin_ns; \
        } \
        delta_ms = QP_RATELIMIT(QP_RATELIMIT_INTERVAL); \
        if (unlikely(delta_ms)) { \
            unsigned long long total_usage = qp_profile_g_usage; \
            unsigned long long total_count = qp_profile_g_count; \
            unsigned long long delta_usage = total_usage - qp_profile_g_last_usage; \
            unsigned long long delta_count = total_count - qp_profile_g_last_count; \
            unsigned long long call_rate, usage_per_sec, instavg, longavg; \
            unsigned long long inst_max = qp_profile_g_inst_max; \
            qp_profile_g_last_count = total_count; \
            qp_profile_g_last_usage = total_usage; \
            qp_profile_g_inst_max = 0; \
            QP_UNLOCK(qp_profile_lock); \
            call_rate = 1000 * delta_count; do_div(call_rate, delta_ms); \
            usage_per_sec = delta_usage; do_div(usage_per_sec, delta_ms); \
            instavg = delta_usage; do_div(instavg, delta_count); \
            longavg = total_usage; do_div(longavg, total_count); \
            do_div(total_usage, 1000000); \
            QP_PRINT_LOC("calls=%llu %llu/sec usage=%llums" \
                    " %lluus/sec" \
                    " inst_avg_dur=%lluns" \
                    " long_avg_dur=%lluns" \
                    " instmax=%lluns " \
                    str QP_NL, \
                    total_count, call_rate, total_usage, \
                    usage_per_sec, instavg, longavg, inst_max); \
        } else { \
            QP_UNLOCK(qp_profile_lock); \
        } \
    } while (0)

#define QP_DUMP_VAR_FMT_VAL(var, fmt, val) QP_PRINT_LOC(#var "=" fmt QP_NL, (val))

#define QP_DUMP_VAR_FMT(fmt, var) QP_PRINT_LOC(#var "=" fmt QP_NL, (var))

#ifdef QP_PROJECT_LINUX_KERNEL
#define QP_DUMP_VAR_PTR(var) QP_DUMP_VAR_FMT_VAL(var, "%px", (void*)var)
#else
#define QP_DUMP_VAR_PTR(var) QP_DUMP_VAR_FMT_VAL(var, "%p", (void*)var)
#endif

#define QP_DUMP_VAR_INT(var) QP_DUMP_VAR_FMT_VAL(var, "%d", (int)var)
#define QP_DUMP_VAR_HEX16(var) QP_DUMP_VAR_FMT_VAL(var, "0x%04x", (u16)var)
#define QP_DUMP_VAR_HEX32(var) QP_DUMP_VAR_FMT_VAL(var, "0x%08x", (u32)var)
#define QP_DUMP_VAR_HEX64(var) QP_DUMP_VAR_FMT_VAL(var, "0x%016llx", (u64)var)
#define QP_DUMP_VAR_HEX(var) QP_DUMP_VAR_FMT_VAL(var, "0x%08x", (u32)var)

#define QP_DUMP_VAR(var) do { \
        typeof(var) val = (var); \
        if (__builtin_types_compatible_p(typeof(val), uint64_t)) { \
            QP_DUMP_VAR_FMT_VAL(var, "%lld", *((uint64_t*)(&(val)))); \
        } else if (__builtin_types_compatible_p(typeof(val), bool)) { \
            QP_DUMP_VAR_FMT_VAL(var, "%s", ((bool)(val)) ? "true" : "false"); \
        } else if (__builtin_types_compatible_p(typeof(val), int)) { \
            QP_DUMP_VAR_FMT_VAL(var, "%d", (int)(val)); \
        } else if (__builtin_types_compatible_p(typeof(val), unsigned int)) { \
            QP_DUMP_VAR_FMT_VAL(var, "%u", (int)(val)); \
        } else if (__builtin_types_compatible_p(typeof(val), void*)) { \
            QP_DUMP_VAR_FMT_VAL(var, "%px", (void*)(uintptr_t)(val)); \
        } else { \
            QP_PRINT_LOC("WTF is " #var "?" QP_NL); \
        } \
    } while (0)

/** Dump raw hex inline: no EOL, just space separator every 8 bytes */
#define QP_DUMP_HEX_BYTES(buf, len) do { \
        unsigned int idx; \
        for (idx = 0; idx < len; ++idx) { \
            QP_PRINT(QP_CONT "%s%02x", (idx && (idx % 8) == 0) ? " " : "", (int)((unsigned char*)buf)[idx]); \
        } \
    } while (0)

/** Dump a hex buffer nicely with a header and up to 16 bytes per line */
#define QP_DUMP_HEX_BUFFER(buf, len) do { \
        unsigned int idx; \
        QP_PRINT_LOC("DUMP %u bytes from %p:", (unsigned int)(len), (buf)); \
        for (idx = 0; idx < (unsigned int)(len); ++idx) { \
            if (idx % 16 == 0) { \
                QP_PRINT(QP_CONT "\nDUMP %p:", ((unsigned char*)(buf)) + idx); \
            } \
            QP_PRINT(QP_CONT "%s%02x", ((idx % 8) == 0) ? "  " : " ", (int)((unsigned char*)(buf))[idx]); \
        } \
        QP_PRINT(QP_CONT "\n"); \
    } while (0)

/** Dump struct msghdr and iov pointers
 *
 * This includes all fields in struct msghdr and each struct iovec but not the
 * actual contents of the iovec buffers.
 */
#define QP_DUMP_MSGHDR(msg) do { \
        unsigned int idx; \
        QP_PRINT_LOC("(msg)=%p flags=0x%x" \
                " name=%p namelen=%d" \
                " iov=%p iovlen=%d" \
                " control=%p controllen=%d\n", \
                (msg), (msg)->msg_flags, \
                (msg)->msg_name, (int)(msg)->msg_namelen, \
                (msg)->msg_iov, (int)(msg)->msg_iovlen, \
                (msg)->msg_control, (int)(msg)->msg_controllen); \
        for (idx = 0; idx < (msg)->msg_iovlen; ++idx) { \
            QP_PRINT_LOC("(msg)=%p iov[%d]: base=%p len=%u\n", \
                    (msg), idx, \
                    (msg)->msg_iov[idx].iov_base, \
                    (unsigned int)(msg)->msg_iov[idx].iov_len); \
        } \
    } while (0)

#define QP_MAC_FMT "%02hhx:%02hhx:%02hhx:%02hhx:%02hhx:%02hhx"
#define QP_MAC_ARG(x) \
        ((uint8_t *)(x))[0], ((uint8_t *)(x))[1], ((uint8_t *)(x))[2], \
        ((uint8_t *)(x))[3], ((uint8_t *)(x))[4], ((uint8_t *)(x))[5]

#define QP_DUMP_MAC(h) QP_PRINT_LOC(QP_MAC_FMT "\n", QP_MAC_ARG(h));

#define QP_IPV4_FMT "%hhu.%hhu.%hhu.%hhu"
#define QP_IPV4_ARG(x) \
        ((uint8_t *)(x))[0], ((uint8_t *)(x))[1], ((uint8_t *)(x))[2], ((uint8_t *)(x))[3]

#define QP_DUMP_IPV4_ADDR(h) QP_PRINT_LOC(QP_IPV4_FMT "\n", QP_IPV4_ARG(h));

#define QP_IPV6_FMT "%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx:%02hhx%02hhx"
#define QP_IPV6_ARG(x) \
        ((uint8_t *)(x))[ 0], ((uint8_t *)(x))[ 1], ((uint8_t *)(x))[ 2], ((uint8_t *)(x))[ 3], \
        ((uint8_t *)(x))[ 4], ((uint8_t *)(x))[ 5], ((uint8_t *)(x))[ 6], ((uint8_t *)(x))[ 7], \
        ((uint8_t *)(x))[ 8], ((uint8_t *)(x))[ 9], ((uint8_t *)(x))[10], ((uint8_t *)(x))[11], \
        ((uint8_t *)(x))[12], ((uint8_t *)(x))[13], ((uint8_t *)(x))[14], ((uint8_t *)(x))[15]

#define QP_DUMP_IPV6_ADDR(h) QP_PRINT_LOC(QP_IPV6_FMT "\n", QP_IPV6_ARG(h));

#define QP_POLL_EVENT_FMT "%x%s%s%s%s%s%s"
#define QP_POLL_EVENT_ARG(x) \
        ((int)(x)), \
        ((x) & POLLIN) ? " POLLIN" : "", \
        ((x) & POLLOUT) ? " POLLOUT" : "", \
        ((x) & POLLERR) ? " POLLERR" : "", \
        ((x) & POLLHUP) ? " POLLHUP" : "", \
        ((x) & POLLPRI) ? " POLLPRI" : "", \
        ((x) & POLLNVAL) ? " POLLNVAL" : ""

#define QP_ADDRFAM_TO_STRING(x) ({ \
        const char *str = "*unknown*"; \
        switch ((x)) { \
            case AF_UNIX: str = "AF_UNIX"; break; \
            case AF_INET: str = "AF_INET"; break; \
            case AF_INET6: str = "AF_INET6"; break; \
            case AF_NETLINK: str = "AF_NETLINK"; break; \
            case AF_PACKET: str = "AF_PACKET"; break; \
        } \
        str; \
    })

#define QP_FLAGSTR_ARG_PREFIX(x, prefix, flag) (((x) & prefix ## flag) ? " "#flag : "")

#define QP_ADDRFAM_FMT "%d%s"
#define QP_ADDRFAM_ARG(x) (int)(x), QP_ADDRFAM_TO_STRING(x)

#define QP_ETH_HDR_FMT "ethhdr=%p proto=%04hx dst_mac=" QP_MAC_FMT " src_mac=" QP_MAC_FMT
#define QP_ETH_HDR_ARG(h) \
            (h), ntohs(((struct ethhdr*)(h))->h_proto), \
            QP_MAC_ARG(((struct ethhdr*)(h))->h_dest), \
            QP_MAC_ARG(((struct ethhdr*)(h))->h_source)
#define QP_DUMP_ETH_HDR(h) QP_PRINT_LOC(QP_ETH_HDR_FMT "\n", QP_ETH_HDR_ARG(h));

#define QP_ARP_HDR_PFX_FMT "arphdr=%p htype=%02hx ptype=%02hx hlen=%hhu plen=%hhu oper=%02hx"
#define QP_ARP_HDR_PFX_ARG(h) (h), ntohs((h)->ar_hrd), ntohs((h)->ar_pro), (h)->ar_hln, (h)->ar_pln, ntohs((h)->ar_op)

#define QP_DUMP_ARP_HDR(h) do { \
        if ((h)->ar_hln == 6 && (h)->ar_pln == 4) { \
            QP_PRINT_LOC(QP_ARP_HDR_PFX_FMT \
                    " sha=" QP_MAC_FMT " spa=" QP_IPV4_FMT \
                    " tha=" QP_MAC_FMT " tpa=" QP_IPV4_FMT "\n", \
                    QP_ARP_HDR_PFX_ARG(h), \
                    QP_MAC_ARG((u8*)(h) + sizeof(struct arphdr)), \
                    QP_IPV4_ARG((u8*)(h) + sizeof(struct arphdr) + 6), \
                    QP_MAC_ARG((u8*)(h) + sizeof(struct arphdr) + 10), \
                    QP_IPV4_ARG((u8*)(h) + sizeof(struct arphdr) + 16)); \
        } else { \
            QP_PRINT_LOC(QP_ARP_HDR_PFX_FMT " other\n", QP_ARP_HDR_PFX_ARG(h)); \
        } \
    } while(0)

#define QP_DUMP_IPV4_HDR(h) QP_PRINT_LOC(\
            "iphdr=%p version=%u protocol=%hx headerlen=%u tot_len=%hu" \
            " check=%02hx" \
            " saddr=" QP_IPV4_FMT \
            " daddr=" QP_IPV4_FMT \
            "\n", \
            (h), (h)->version, ntohs((h)->protocol), \
            (h)->ihl, ntohs((h)->tot_len), \
            (h)->check, \
            QP_IPV4_ARG(&(h)->saddr), \
            QP_IPV4_ARG(&(h)->daddr))

#define QP_IPV6_HDR_FMT \
            "iphdr=%p" \
            " version=%u" \
            " priority=%u" \
            " payload_len=%hu" \
            " nexthdr=0x%hhx" \
            " hop_limit=%hhd" \
            " saddr=" QP_IPV6_FMT \
            " daddr=" QP_IPV6_FMT
#define QP_IPV6_HDR_ARG(h) \
            (h), \
            (h)->version, \
            (h)->priority, \
            ntohs((h)->payload_len), \
            (h)->nexthdr, \
            (h)->hop_limit, \
            QP_IPV6_ARG(&(h)->saddr), \
            QP_IPV6_ARG(&(h)->daddr)
#define QP_DUMP_IPV6_HDR(h) QP_PRINT_LOC(QP_IPV6_HDR_FMT QP_NL, QP_IPV6_HDR_ARG(h))

#define QP_DUMP_IPVX_HDR(h) do { \
        if (((struct iphdr*)(h))->version == 4) { \
            QP_DUMP_IPV4_HDR((struct iphdr*)(h)); \
        } else if (((struct iphdr*)(h))->version == 6) { \
            QP_DUMP_IPV6_HDR((struct ipv6hdr*)(h)); \
        } else { \
            QP_PRINT_LOC("no-IP header\n") ;\
        } \
    } while(0)

#define QP_UDP_HDR_FMT \
            "udphdr=%p sport=%hu dport=%hu len=%hu csum=%hu"

#define QP_UDP_HDR_ARG(h) \
            (h), \
            ntohs((h)->source), ntohs((h)->dest), \
            ntohl((h)->len), ntohl((h)->check)

#define QP_DUMP_UDP_HDR(h) QP_PRINT_LOC(QP_UDP_HDR_FMT QP_NL, QP_UDP_HDR_ARG(h))

#define QP_TCP_HDR_FMT \
            "tcphdr=%p sport=%hu dport=%hu" \
            " seq=%u ack=%u doff=%u" \
            " flags=%04hx%s%s%s%s%s%s" \
            " win=%u csum=%04hx urg=%hu"

#define QP_TCP_HDR_ARG(h) \
            (h), ntohs((h)->source), ntohs((h)->dest), \
            ntohl((h)->seq), ntohl((h)->ack_seq), \
            (h)->doff, \
            ((unsigned short*)(h))[6], \
            (h)->syn ? " SYN" : "", \
            (h)->fin ? " FIN" : "", \
            (h)->rst ? " RST" : "", \
            (h)->ack ? " ACK" : "", \
            (h)->psh ? " PSH" : "", \
            (h)->urg ? " URG" : "", \
            (h)->window, ntohs((h)->check), ntohs((h)->urg)

#define QP_DUMP_TCP_HDR(h) QP_PRINT_LOC(QP_TCP_HDR_FMT QP_NL, QP_TCP_HDR_ARG(h))

#define QP__VALUE_MASK_ARG(val, mask) \
            ((val) & (mask) ? " "#mask : "")

/* Try to print addressing information from struct sock *sk */
#define QP_DUMP_LINUX_SOCK_ADDR(sk) do { \
        if ((sk)->sk_family == AF_INET) { \
            if (!sk_fullsock(sk)) { \
                QP_PRINT_LOC("sk=%px TCPv4 minisock sk_state=%d" \
                        " saddr=%pI4:%hu daddr=%pI4:%hu\n", \
                        (sk), (sk)->sk_state, \
                        &(sk)->sk_rcv_saddr, (sk)->sk_num, \
                        &(sk)->sk_daddr, ntohs((sk)->sk_dport)); \
            } else if ((sk)->sk_protocol == IPPROTO_TCP && (sk)->sk_state == TCP_LISTEN) { \
                QP_PRINT_LOC("sk=%px TCPv4 LISTEN %pI4:%hu\n", \
                        (sk), &(sk)->sk_rcv_saddr, (sk)->sk_num); \
            } else { \
                QP_PRINT_LOC("sk=%px IPv4 sk_state=%d sk_protocol=%04hx sk_type=%04hx" \
                        " saddr=%pI4:%hu daddr=%pI4:%hu\n", \
                        (sk), (sk)->sk_state, \
                        (sk)->sk_protocol, (sk)->sk_type, \
                        &(sk)->sk_rcv_saddr, (sk)->sk_num, \
                        &(sk)->sk_daddr, ntohs((sk)->sk_dport)); \
            } \
        } else if ((sk)->sk_family == AF_INET6) { \
            if (!sk_fullsock(sk)) { \
                QP_PRINT_LOC("sk=%px TCPv6 minisock sk_state=%d" \
                        " saddr=%pI6:%hu daddr=%pI6:%hu\n", \
                        (sk), (sk)->sk_state, \
                        &(sk)->sk_v6_rcv_saddr, (sk)->sk_num, \
                        &(sk)->sk_v6_daddr, ntohs((sk)->sk_dport)); \
            } else if ((sk)->sk_protocol == IPPROTO_TCP && (sk)->sk_state == TCP_LISTEN) { \
                QP_PRINT_LOC("sk=%px TCPv6 LISTEN %pI6:%hu\n", \
                        (sk), &(sk)->sk_v6_rcv_saddr, (sk)->sk_num); \
            } else { \
                QP_PRINT_LOC("sk=%px IPv6 sk_state=%d sk_protocol=%04hx sk_type=%04hx" \
                        " saddr=%pI6:%hu daddr=%pI6:%hu\n", \
                        (sk), (sk)->sk_state, \
                        (sk)->sk_protocol, (sk)->sk_type, \
                        &(sk)->sk_v6_rcv_saddr, (sk)->sk_num, \
                        &(sk)->sk_v6_daddr, ntohs((sk)->sk_dport)); \
            } \
        } else if (sk_fullsock(sk)) { \
            /* TCP minisocks do not have sk_protocol sk_type */ \
            QP_PRINT_LOC("sk=%px family=%04hx sk_state=%d" \
                    " sk_protocol=%04hx sk_type=%04hx sk_num=%hu\n", \
                    (sk), (sk)->sk_family, (sk)->sk_state, \
                    (sk)->sk_protocol, (sk)->sk_type, (sk)->sk_num); \
        } else { \
            /* minisocks should be IPv4 or IPv6 */ \
            QP_PRINT_LOC("sk=%px family=%04hx sk_state=%d unexpected minisock\n", \
                    (sk), (sk)->sk_family, (sk)->sk_state); \
        } \
    } while (0)

#define QP_DUMP_SOCKADDR_LL(a) do { \
        unsigned int addr_index; \
        QP_PRINT_LOC( \
                "sockaddr_ll=%p family=%04hx protocol=%04hx ifindex=%d" \
                " hatype=%hu pkttype=%hhu halen=%hhu addr", \
                (a), (a)->sll_family, ntohs((a)->sll_protocol), (a)->sll_ifindex, \
                (a)->sll_hatype, (a)->sll_pkttype, (a)->sll_halen); \
        for (addr_index = 0; addr_index < (a)->sll_halen && addr_index < 8; ++addr_index) { \
                QP_PRINT("%c%02hhx", addr_index ? ':' : '=', (a)->sll_addr[addr_index]); \
        } \
        QP_PRINT("\n"); \
    } while (0)

#define QP_DUMP_SOCKADDR_IN(a) \
        QP_PRINT_LOC( \
                "sockaddr_in=%p family=%04hx addr=" QP_IPV4_FMT " port=%hu\n", \
                (a), (a)->sin_family, QP_IPV4_ARG(&(a)->sin_addr.s_addr), ntohs((a)->sin_port))

#define QP_DUMP_SOCKADDR_IN6(a) \
        QP_PRINT_LOC( \
                "sockaddr_in6=%p family=%04hx addr=" QP_IPV6_FMT " port=%hu\n" \
                " flowinfo=%08hx scope_id=%u\n", \
                (a), (a)->sin6_family, QP_IPV6_ARG(&(a)->sin6_addr.s6_addr16), ntohs((a)->sin6_port), \
                (a)->sin6_flowinfo, (a)->sin6_scope_id)

#define QP_DUMP_SOCKADDR(a) do { \
        if (((struct sockaddr*)(a))->sa_family == PF_PACKET) { \
                QP_DUMP_SOCKADDR_LL((struct sockaddr_ll*)(a)); \
        } else if (((struct sockaddr*)(a))->sa_family == AF_INET) { \
                QP_DUMP_SOCKADDR_IN((struct sockaddr_in*)(a)); \
        } else if (((struct sockaddr*)(a))->sa_family == AF_INET6) { \
                QP_DUMP_SOCKADDR_IN6((struct sockaddr_in6*)(a)); \
        } else { \
                QP_PRINT_LOC("sockaddr=%p family=%04hx OTHER\n", (a), ((struct sockaddr*)(a))->sa_family); \
        } \
    } while (0)

#define QP_DUMP_NLMSGHDR(m) QP_PRINT_LOC( \
        "nlmsghdr=%p len=%u type=0x%02hx flags=0x%02hx seq=%d pid=%d\n", \
        (m), (m)->nlmsg_len, \
        (m)->nlmsg_type, (m)->nlmsg_flags, \
        (m)->nlmsg_seq, (m)->nlmsg_pid)

#define QP_DUMP_NLMSG_RTATTR(m, delta) do { \
        QP_DUMP_NLMSGHDR(m); \
        struct rtattr *a = (struct rtattr *)(((u8*)(m)) + (delta)); \
        int alen = (m)->nlmsg_len - (delta); \
        while (RTA_OK(a, alen)) { \
            QP_PRINT_LOC("type=0x%04x len=%d buf=", a->rta_type, a->rta_len); \
            QP_DUMP_HEX_BYTES(a + 1, a->rta_len); \
            QP_PRINT("\n"); \
            a = RTA_NEXT(a, alen); \
        } \
    } while (0)

#define QP_GETSOCKOPT_INT(fd, level, optname) ({ \
        int optval = 0; \
        socklen_t optlen = sizeof(optval); \
        if (getsockopt((fd), (level), (optname), &optval, &optlen)) { \
            optval = -errno; \
        } \
        optval; \
    })

#define QP_DUMP_SOCKOPT_INT(fd, level, optname) do { \
        int optval = QP_GETSOCKOPT_INT(fd, level, optname); \
        if (optval < 0) { \
            QP_PRINT_LOC("failed getsockopt(%d, "#level", "#optname"): errno=%d\n", (fd), -optval); \
        } else { \
            QP_PRINT_LOC("sockopt(%d, "#level", "#optname") = %d\n", (fd), optval); \
        } \
    } while (0)

#define QP_DUMP_SOCKOPT(fd) \
        QP_PRINT_LOC("fd=%d SO_TYPE=%d SO_PRIORITY=%d" \
                " SO_RCVBUF=%d SO_SNDBUF=%d" \
                " SO_RCVLOWAT=%d SO_SNDLOWAT=%d" \
                "%s\n", fd, \
                QP_GETSOCKOPT_INT(fd, SOL_SOCKET, SO_TYPE), \
                QP_GETSOCKOPT_INT(fd, SOL_SOCKET, SO_PRIORITY), \
                QP_GETSOCKOPT_INT(fd, SOL_SOCKET, SO_RCVBUF), \
                QP_GETSOCKOPT_INT(fd, SOL_SOCKET, SO_SNDBUF), \
                QP_GETSOCKOPT_INT(fd, SOL_SOCKET, SO_RCVLOWAT), \
                QP_GETSOCKOPT_INT(fd, SOL_SOCKET, SO_SNDLOWAT), \
                (fcntl(fd, F_GETFL, NULL) & O_NONBLOCK) ? " O_NONBLOCK" : "")

#define QP_UNOPTIMIZED __attribute__((__optimize__(0)))

/** Run a system command (like system(2)) and print output through QP_PRINT
 *
 * This is allows easy capturing of output inside projects that log somewhere
 * other than stdout/stderr. It is based on popen and fgets.
 */
#define QP_RUN_SYSTEM(cmd) ({ \
        FILE *fp; \
        char buf[1024]; \
        char *fgets_result; \
        int ret; \
        fp = popen(cmd, "r"); \
        QP_PRINT_LOC("RUN: %s" QP_NL, cmd); \
        while (true) {\
            fgets_result = fgets(buf, sizeof(buf), fp); \
            if (fgets_result != NULL) { \
                QP_PRINT("%s", buf); \
            } else { \
                ret = ferror(fp); \
                if (ret) { \
                    QP_PRINT_LOC("ferror result: %d" QP_NL, ret); \
                } \
                break; \
            } \
        } \
        ret = pclose(fp); \
        if (WIFEXITED(ret)) { \
            QP_PRINT_LOC("exit status %d" QP_NL, WEXITSTATUS(ret)); \
        } else if (WIFSIGNALED(ret)) { \
            QP_PRINT_LOC("exit signal %d" QP_NL, WTERMSIG(ret)); \
        } else { \
            /* Something other than WIFEXITED WIFXSIGNALED should only happen \
             * when explicitly requested by the parent. */ \
            QP_PRINT_LOC("unexpected wait status 0x%x" QP_NL, ret); \
        } \
        ret; \
    })

#endif // QP_HEADER_INCLUDED
