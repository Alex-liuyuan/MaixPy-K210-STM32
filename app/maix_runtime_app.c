#include "maix_runtime_app.h"

#include <rtthread.h>
#ifdef RT_USING_FINSH
#include <finsh.h>
#endif

#include <stddef.h>
#include <string.h>

#define MAIX_PRODUCT_NAME "MaixPy Nano"
#define MAIX_PRODUCT_VERSION "0.1.0"

static maix_runtime_profile_t g_profile;
static maix_runtime_state_t g_state;

static const char *maix_cap_state_name(maix_cap_state_t state)
{
    switch (state)
    {
    case MAIX_CAP_AVAILABLE:
        return "available";
    case MAIX_CAP_EXPERIMENTAL:
        return "experimental";
    case MAIX_CAP_PLANNED:
        return "planned";
    case MAIX_CAP_UNAVAILABLE:
    default:
        return "unavailable";
    }
}

static void maix_runtime_load_defaults(maix_runtime_profile_t *profile)
{
    memset(profile, 0, sizeof(*profile));

    profile->product_name = MAIX_PRODUCT_NAME;
    profile->product_version = MAIX_PRODUCT_VERSION;
    profile->board_name = "unknown";
    profile->runtime_name = "unknown";
    profile->console_name = "unknown";
    profile->cpu_arch = "unknown";
    profile->os_name = "RT-Thread Nano";
    profile->led = MAIX_CAP_UNAVAILABLE;
#ifdef RT_USING_HEAP
    profile->heap = MAIX_CAP_AVAILABLE;
#else
    profile->heap = MAIX_CAP_UNAVAILABLE;
#endif
#ifdef RT_USING_FINSH
    profile->shell = MAIX_CAP_AVAILABLE;
#else
    profile->shell = MAIX_CAP_UNAVAILABLE;
#endif
#ifdef RT_USING_DEVICE
    profile->device_framework = MAIX_CAP_AVAILABLE;
#else
    profile->device_framework = MAIX_CAP_UNAVAILABLE;
#endif
    profile->storage = MAIX_CAP_PLANNED;
    profile->python_vm = MAIX_CAP_PLANNED;
    profile->model_runtime = MAIX_CAP_PLANNED;
    profile->camera = MAIX_CAP_PLANNED;
    profile->display = MAIX_CAP_PLANNED;
}

static void maix_runtime_refresh(void)
{
    maix_runtime_load_defaults(&g_profile);
    maix_board_fill_profile(&g_profile);

    g_state.uptime_ms = rt_tick_get() * (1000 / RT_TICK_PER_SECOND);
    g_state.heartbeat_count = 0;
    g_state.heap_total = 0;
    g_state.heap_used = 0;
    g_state.heap_peak = 0;
}

static void maix_runtime_print_profile(void)
{
    rt_kprintf("\n[BOOT] %s %s\n", g_profile.product_name, g_profile.product_version);
    rt_kprintf("[BOOT] Board: %s\n", g_profile.board_name);
    rt_kprintf("[BOOT] Runtime: %s\n", g_profile.runtime_name);
    rt_kprintf("[BOOT] Console: %s\n", g_profile.console_name);
    rt_kprintf("[BOOT] CPU: %s\n", g_profile.cpu_arch);
    rt_kprintf("[BOOT] OS: %s\n", g_profile.os_name);

    rt_kprintf("[CAP] led=%s heap=%s shell=%s device=%s\n",
               maix_cap_state_name(g_profile.led),
               maix_cap_state_name(g_profile.heap),
               maix_cap_state_name(g_profile.shell),
               maix_cap_state_name(g_profile.device_framework));
    rt_kprintf("[CAP] storage=%s python_vm=%s model_runtime=%s\n",
               maix_cap_state_name(g_profile.storage),
               maix_cap_state_name(g_profile.python_vm),
               maix_cap_state_name(g_profile.model_runtime));
    rt_kprintf("[CAP] camera=%s display=%s\n",
               maix_cap_state_name(g_profile.camera),
               maix_cap_state_name(g_profile.display));

 #ifdef RT_USING_HEAP
    rt_kprintf("[MEM] heap enabled\n");
 #else
    rt_kprintf("[MEM] heap disabled\n");
 #endif

    rt_kprintf("[NEXT] integrate MicroPython VM, VFS and model loader on RT-Thread Nano\n");
}

static void maix_runtime_print_state(void)
{
    g_state.uptime_ms = rt_tick_get() * (1000 / RT_TICK_PER_SECOND);
    rt_kprintf("[STATE] uptime_ms=%lu heartbeat=%lu\n",
               (unsigned long)g_state.uptime_ms,
               g_state.heartbeat_count);
}

int maix_runtime_main(void)
{
    maix_board_app_init();
    maix_runtime_refresh();
    maix_runtime_print_profile();

    while (1)
    {
        maix_board_heartbeat(g_state.heartbeat_count);
        g_state.uptime_ms = rt_tick_get() * (1000 / RT_TICK_PER_SECOND);
        rt_kprintf("[APP] heartbeat=%lu uptime_ms=%lu\n",
                   g_state.heartbeat_count,
                   (unsigned long)g_state.uptime_ms);
        g_state.heartbeat_count++;
        rt_thread_mdelay(1000);
    }
}

#ifdef RT_USING_FINSH
static void maix_info(void)
{
    maix_runtime_refresh();
    maix_runtime_print_profile();
}
MSH_CMD_EXPORT(maix_info, print MaixPy Nano runtime profile);

static void maix_state(void)
{
    maix_runtime_print_state();
}
MSH_CMD_EXPORT(maix_state, print MaixPy Nano runtime state);
#endif
