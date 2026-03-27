#include "maix_runtime_python.h"

#include <rtthread.h>

#if defined(MAIX_HAS_MICROPYTHON) && MAIX_HAS_MICROPYTHON
#include "maix_mpy_port.h"
#endif

#include "maix_runtime_fs.h"

static const char *g_selected_script = "/maixapp/apps/sysu_aiotos_demo/main.py";

static int maix_runtime_python_run_file(const char *path, maix_runtime_state_t *state, const char *tag)
{
#if defined(MAIX_HAS_MICROPYTHON) && MAIX_HAS_MICROPYTHON
    if (!maix_runtime_fs_path_exists(path))
    {
        rt_kprintf("[PY] %s skipped: %s missing\n", tag, path);
        return 1;
    }

    if (maix_mpy_exec_file(path) != 0)
    {
        rt_kprintf("[PY] %s failed: %s\n", path, maix_mpy_last_error());
        if (state != RT_NULL)
        {
            state->boot_status = MAIX_BOOT_FAILED;
        }
        return -1;
    }

    rt_kprintf("[PY] executed %s\n", path);
    return 0;
#else
    (void)path;
    (void)state;
    (void)tag;
    return -1;
#endif
}

void maix_runtime_python_init(maix_runtime_profile_t *profile, maix_runtime_state_t *state)
{
    const char *app_main_path = maix_runtime_fs_auto_start_script();
    const char *main_path = "/main.py";

    if (maix_runtime_fs_path_exists(app_main_path))
    {
        g_selected_script = app_main_path;
    }
    else
    {
        g_selected_script = main_path;
    }

    if (profile != RT_NULL)
    {
#if defined(MAIX_HAS_MICROPYTHON) && MAIX_HAS_MICROPYTHON
        profile->python_vm = MAIX_CAP_EXPERIMENTAL;
#else
        profile->python_vm = MAIX_CAP_PLANNED;
#endif
    }

    if (state != RT_NULL)
    {
#if defined(MAIX_HAS_MICROPYTHON) && MAIX_HAS_MICROPYTHON
        state->python_backend = "micropython";
#else
        state->python_backend = "stub";
#endif
        state->active_script = g_selected_script;
    }
}

void maix_runtime_python_run_boot(maix_runtime_state_t *state)
{
    const char *boot_path = "/boot.py";
    const char *main_path = "/main.py";
    const char *app_yaml_path = "/maixapp/apps/sysu_aiotos_demo/app.yaml";
    const char *app_main_path = maix_runtime_fs_auto_start_script();

    rt_kprintf("[PY] backend=%s boot.py=%s main.py=%s\n",
               state != RT_NULL && state->python_backend ? state->python_backend : "unknown",
               maix_runtime_fs_path_exists(boot_path) ? "present" : "missing",
               maix_runtime_fs_path_exists(main_path) ? "present" : "missing");
    rt_kprintf("[APPFS] app_id=%s app.yaml=%s app_main=%s\n",
               maix_runtime_fs_auto_start_app_id(),
               maix_runtime_fs_path_exists(app_yaml_path) ? "present" : "missing",
               maix_runtime_fs_path_exists(app_main_path) ? "present" : "missing");

#if defined(MAIX_HAS_MICROPYTHON) && MAIX_HAS_MICROPYTHON
    if (maix_mpy_init() != 0)
    {
        rt_kprintf("[PY] init failed: %s\n", maix_mpy_last_error());
        if (state != RT_NULL)
        {
            state->boot_status = MAIX_BOOT_FAILED;
        }
        return;
    }

    if (state != RT_NULL)
    {
        state->heap_total = (uint32_t)maix_mpy_heap_size();
    }

    rt_kprintf("[PY] MicroPython VM initialized, heap=%lu bytes\n",
               (unsigned long)maix_mpy_heap_size());

    if (!maix_runtime_fs_path_exists(boot_path))
    {
        rt_kprintf("[PY] boot sequence failed: /boot.py missing\n");
        if (state != RT_NULL)
        {
            state->boot_status = MAIX_BOOT_FAILED;
        }
        return;
    }

    if (maix_runtime_python_run_file(boot_path, state, "boot.py") != 0)
    {
        return;
    }

    if (maix_runtime_fs_path_exists(app_main_path))
    {
        g_selected_script = app_main_path;
        if (state != RT_NULL)
        {
            state->active_script = app_main_path;
        }
        if (maix_runtime_python_run_file(app_main_path, state, "app") != 0)
        {
            return;
        }
    }
    else if (maix_runtime_fs_path_exists(main_path))
    {
        g_selected_script = main_path;
        if (state != RT_NULL)
        {
            state->active_script = main_path;
        }
        if (maix_runtime_python_run_file(main_path, state, "main") != 0)
        {
            return;
        }
    }
    else
    {
        rt_kprintf("[PY] no runnable application script found\n");
    }

    if (state != RT_NULL)
    {
        state->boot_status = MAIX_BOOT_OK;
    }
#else
    rt_kprintf("[PY] boot sequence blocked: firmware is built without MicroPython source integration\n");
    if (state != RT_NULL)
    {
        state->boot_status = MAIX_BOOT_BLOCKED;
    }
#endif
}

void maix_runtime_python_print_status(void)
{
    const char *boot_path = "/boot.py";
    const char *main_path = "/main.py";
    const char *app_yaml_path = "/maixapp/apps/sysu_aiotos_demo/app.yaml";

#if defined(MAIX_HAS_MICROPYTHON) && MAIX_HAS_MICROPYTHON
    rt_kprintf("[PY] backend=micropython status=integrated heap=%lu last_error=%s selected=%s\n",
               (unsigned long)maix_mpy_heap_size(),
               maix_mpy_last_error(),
               g_selected_script);
#else
    rt_kprintf("[PY] backend=stub status=blocked reason=no-micropython-source selected=%s\n",
               g_selected_script);
#endif
    rt_kprintf("[PY] boot.py=%s main.py=%s app.yaml=%s app_main=%s\n",
               maix_runtime_fs_path_exists(boot_path) ? "present" : "missing",
               maix_runtime_fs_path_exists(main_path) ? "present" : "missing",
               maix_runtime_fs_path_exists(app_yaml_path) ? "present" : "missing",
               maix_runtime_fs_path_exists(maix_runtime_fs_auto_start_script()) ? "present" : "missing");
}

const char *maix_runtime_python_selected_script(void)
{
    return g_selected_script;
}
