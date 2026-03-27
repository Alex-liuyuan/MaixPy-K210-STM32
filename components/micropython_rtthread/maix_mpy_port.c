#include "maix_mpy_port.h"
#include "maix_runtime_fs.h"

#include <rtthread.h>

#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#ifdef RT_USING_DFS
#include <sys/stat.h>
#include <unistd.h>
#endif

#include "py/builtin.h"
#include "py/compile.h"
#include "py/gc.h"
#include "py/lexer.h"
#include "py/mperrno.h"
#include "py/nlr.h"
#include "py/objlist.h"
#include "py/qstr.h"
#include "py/runtime.h"
#include "py/stackctrl.h"
#include "py/stream.h"

#define MAIX_MPY_HEAP_BYTES (32 * 1024)

static unsigned char g_heap[MAIX_MPY_HEAP_BYTES];
static size_t g_heap_size;
static void *g_stack_top;
static int g_initialized;
static char g_last_error[96] = "not-initialized";

typedef struct _maix_file_obj_t
{
    mp_obj_base_t base;
    qstr path_qstr;
    const maix_bundle_file_t *file;
    int fd;
    bool uses_posix;
    size_t offset;
    bool closed;
} maix_file_obj_t;

static const maix_bundle_file_t *maix_mpy_find_bundle_exact(const char *path)
{
    char absolute_path[256];

    if (path == RT_NULL || path[0] == '\0')
    {
        return RT_NULL;
    }

    if (path[0] == '/')
    {
        return maix_runtime_fs_find(path);
    }

    if (snprintf(absolute_path, sizeof(absolute_path), "/%s", path) >= (int)sizeof(absolute_path))
    {
        return RT_NULL;
    }

    return maix_runtime_fs_find(absolute_path);
}

static const maix_bundle_file_t *maix_mpy_find_bundle_file(const char *path)
{
    const maix_bundle_file_t *file;
    char candidate[256];

    file = maix_mpy_find_bundle_exact(path);
    if (file != RT_NULL)
    {
        return file;
    }

    if (path == RT_NULL || path[0] == '\0')
    {
        return RT_NULL;
    }

    if (strstr(path, ".py") == RT_NULL)
    {
        if (path[0] == '/')
        {
            if (snprintf(candidate, sizeof(candidate), "%s.py", path) < (int)sizeof(candidate))
            {
                file = maix_runtime_fs_find(candidate);
                if (file != RT_NULL)
                {
                    return file;
                }
            }
        }
        else if (snprintf(candidate, sizeof(candidate), "/%s.py", path) < (int)sizeof(candidate))
        {
            file = maix_runtime_fs_find(candidate);
            if (file != RT_NULL)
            {
                return file;
            }
        }
    }

    if (path[0] == '/')
    {
        if (snprintf(candidate, sizeof(candidate), "%s/__init__.py", path) < (int)sizeof(candidate))
        {
            return maix_runtime_fs_find(candidate);
        }
    }
    else if (snprintf(candidate, sizeof(candidate), "/%s/__init__.py", path) < (int)sizeof(candidate))
    {
        return maix_runtime_fs_find(candidate);
    }

    return RT_NULL;
}

static bool maix_mpy_bundle_has_dir(const char *path)
{
    size_t count;
    size_t i;
    size_t path_len;
    char prefix[256];
    const maix_bundle_file_t *files;

    if (path == RT_NULL || path[0] == '\0')
    {
        return false;
    }

    if (path[0] == '/')
    {
        if (snprintf(prefix, sizeof(prefix), "%s/", path) >= (int)sizeof(prefix))
        {
            return false;
        }
    }
    else if (snprintf(prefix, sizeof(prefix), "/%s/", path) >= (int)sizeof(prefix))
    {
        return false;
    }

    files = maix_runtime_fs_manifest(&count);
    path_len = strlen(prefix);
    for (i = 0; i < count; i++)
    {
        if (strncmp(files[i].path, prefix, path_len) == 0)
        {
            return true;
        }
    }

    return false;
}

#ifdef RT_USING_DFS
static const char *maix_mpy_normalize_path(const char *path, char *buf, size_t buf_size)
{
    if (path == RT_NULL || path[0] == '\0')
    {
        return RT_NULL;
    }

    if (path[0] == '/')
    {
        return path;
    }

    if (snprintf(buf, buf_size, "/%s", path) >= (int)buf_size)
    {
        return RT_NULL;
    }

    return buf;
}

static mp_import_stat_t maix_mpy_import_stat_posix(const char *path)
{
    char normalized[256];
    const char *candidate = maix_mpy_normalize_path(path, normalized, sizeof(normalized));
    struct stat st;

    if (candidate == RT_NULL)
    {
        return MP_IMPORT_STAT_NO_EXIST;
    }

    if (stat(candidate, &st) == 0)
    {
#ifdef S_ISDIR
        return S_ISDIR(st.st_mode) ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
#else
        return ((st.st_mode & S_IFMT) == S_IFDIR) ? MP_IMPORT_STAT_DIR : MP_IMPORT_STAT_FILE;
#endif
    }

    return MP_IMPORT_STAT_NO_EXIST;
}

static int maix_mpy_open_posix_path(const char *path, int flags)
{
    char normalized[256];
    const char *candidate = maix_mpy_normalize_path(path, normalized, sizeof(normalized));

    if (candidate == RT_NULL)
    {
        errno = ENAMETOOLONG;
        return -1;
    }

    return open(candidate, flags, 0644);
}
#endif

static void maix_file_check_open(const maix_file_obj_t *self)
{
    if (self->closed || (!self->uses_posix && self->file == RT_NULL))
    {
        mp_raise_ValueError("I/O operation on closed file");
    }
}

static void maix_file_print(const mp_print_t *print, mp_obj_t self_in, mp_print_kind_t kind)
{
    maix_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

    (void)kind;
    if (self->closed)
    {
        mp_printf(print, "<file closed>");
        return;
    }

    if (self->uses_posix)
    {
        mp_printf(print, "<file %s>", qstr_str(self->path_qstr));
    }
    else
    {
        mp_printf(print, "<romfile %s>", self->file->path);
    }
}

static mp_uint_t maix_file_read(mp_obj_t self_in, void *buf, mp_uint_t size, int *errcode)
{
    maix_file_obj_t *self = MP_OBJ_TO_PTR(self_in);
    size_t remaining;
    size_t read_size;

    maix_file_check_open(self);

    if (self->uses_posix)
    {
#ifdef RT_USING_DFS
        int res = read(self->fd, buf, size);

        if (res < 0)
        {
            *errcode = errno;
            return MP_STREAM_ERROR;
        }

        return (mp_uint_t)res;
#else
        *errcode = MP_ENOENT;
        return MP_STREAM_ERROR;
#endif
    }

    remaining = self->file->size - self->offset;
    read_size = size;
    if (read_size > remaining)
    {
        read_size = remaining;
    }

    memcpy(buf, self->file->data + self->offset, read_size);
    self->offset += read_size;
    (void)errcode;
    return (mp_uint_t)read_size;
}

static mp_uint_t maix_file_write(mp_obj_t self_in, const void *buf, mp_uint_t size, int *errcode)
{
    maix_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

    maix_file_check_open(self);

    if (self->uses_posix)
    {
#ifdef RT_USING_DFS
        int res = write(self->fd, buf, size);

        if (res < 0)
        {
            *errcode = errno;
            return MP_STREAM_ERROR;
        }

        return (mp_uint_t)res;
#else
        *errcode = MP_ENOENT;
        return MP_STREAM_ERROR;
#endif
    }

    (void)buf;
    (void)size;
    *errcode = MP_EROFS;
    return MP_STREAM_ERROR;
}

static mp_uint_t maix_file_ioctl(mp_obj_t self_in, mp_uint_t request, uintptr_t arg, int *errcode)
{
    maix_file_obj_t *self = MP_OBJ_TO_PTR(self_in);

    maix_file_check_open(self);

    switch (request)
    {
    case MP_STREAM_SEEK:
    {
        struct mp_stream_seek_t *seek = (struct mp_stream_seek_t *)arg;

        if (self->uses_posix)
        {
#ifdef RT_USING_DFS
            off_t next = lseek(self->fd, seek->offset, seek->whence);

            if (next < 0)
            {
                *errcode = errno;
                return MP_STREAM_ERROR;
            }

            seek->offset = next;
            return 0;
#else
            *errcode = MP_ENOENT;
            return MP_STREAM_ERROR;
#endif
        }

        mp_off_t base = 0;
        mp_off_t next = 0;

        if (seek->whence == 0)
        {
            base = 0;
        }
        else if (seek->whence == 1)
        {
            base = (mp_off_t)self->offset;
        }
        else if (seek->whence == 2)
        {
            base = (mp_off_t)self->file->size;
        }
        else
        {
            *errcode = MP_EINVAL;
            return MP_STREAM_ERROR;
        }

        next = base + seek->offset;
        if (next < 0 || (size_t)next > self->file->size)
        {
            *errcode = MP_EINVAL;
            return MP_STREAM_ERROR;
        }

        self->offset = (size_t)next;
        seek->offset = next;
        return 0;
    }
    case MP_STREAM_FLUSH:
        return 0;
    case MP_STREAM_CLOSE:
#ifdef RT_USING_DFS
        if (self->uses_posix)
        {
            close(self->fd);
            self->fd = -1;
        }
#endif
        self->closed = true;
        return 0;
    case MP_STREAM_GET_FILENO:
        if (self->uses_posix)
        {
            return (mp_uint_t)self->fd;
        }
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    default:
        *errcode = MP_EINVAL;
        return MP_STREAM_ERROR;
    }
}

static mp_obj_t maix_file___exit__(size_t n_args, const mp_obj_t *args)
{
    (void)n_args;
    return mp_stream_close(args[0]);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(maix_file___exit___obj, 4, 4, maix_file___exit__);

static const mp_rom_map_elem_t maix_file_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_read), MP_ROM_PTR(&mp_stream_read_obj) },
    { MP_ROM_QSTR(MP_QSTR_readinto), MP_ROM_PTR(&mp_stream_readinto_obj) },
    { MP_ROM_QSTR(MP_QSTR_readline), MP_ROM_PTR(&mp_stream_unbuffered_readline_obj) },
    { MP_ROM_QSTR(MP_QSTR_readlines), MP_ROM_PTR(&mp_stream_unbuffered_readlines_obj) },
    { MP_ROM_QSTR(MP_QSTR_write), MP_ROM_PTR(&mp_stream_write_obj) },
    { MP_ROM_QSTR(MP_QSTR_seek), MP_ROM_PTR(&mp_stream_seek_obj) },
    { MP_ROM_QSTR(MP_QSTR_tell), MP_ROM_PTR(&mp_stream_tell_obj) },
    { MP_ROM_QSTR(MP_QSTR_flush), MP_ROM_PTR(&mp_stream_flush_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&mp_stream_close_obj) },
    { MP_ROM_QSTR(MP_QSTR___enter__), MP_ROM_PTR(&mp_identity_obj) },
    { MP_ROM_QSTR(MP_QSTR___exit__), MP_ROM_PTR(&maix_file___exit___obj) },
};
STATIC MP_DEFINE_CONST_DICT(maix_file_locals_dict, maix_file_locals_dict_table);

static const mp_stream_p_t maix_fileio_stream_p = {
    .read = maix_file_read,
    .write = maix_file_write,
    .ioctl = maix_file_ioctl,
};

static const mp_obj_type_t maix_fileio_type = {
    { &mp_type_type },
    .name = MP_QSTR_FileIO,
    .print = maix_file_print,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &maix_fileio_stream_p,
    .locals_dict = (mp_obj_dict_t *)&maix_file_locals_dict,
};

static const mp_stream_p_t maix_textio_stream_p = {
    .read = maix_file_read,
    .write = maix_file_write,
    .ioctl = maix_file_ioctl,
    .is_text = true,
};

static const mp_obj_type_t maix_textio_type = {
    { &mp_type_type },
    .name = MP_QSTR_TextIOWrapper,
    .print = maix_file_print,
    .getiter = mp_identity_getiter,
    .iternext = mp_stream_unbuffered_iter,
    .protocol = &maix_textio_stream_p,
    .locals_dict = (mp_obj_dict_t *)&maix_file_locals_dict,
};

static void maix_mpy_set_error(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsnprintf(g_last_error, sizeof(g_last_error), fmt, args);
    va_end(args);
}

int maix_mpy_init(void)
{
    rt_thread_t thread;

    if (g_initialized)
    {
        return 0;
    }

    thread = rt_thread_self();
    g_heap_size = sizeof(g_heap);

    mp_stack_ctrl_init();
    if (thread != RT_NULL && thread->stack_addr != RT_NULL && thread->stack_size > 1024)
    {
        g_stack_top = (char *)thread->stack_addr + thread->stack_size;
        mp_stack_set_top(g_stack_top);
        mp_stack_set_limit(thread->stack_size - 512);
    }
    else
    {
        g_stack_top = RT_NULL;
        mp_stack_set_limit(3072);
    }

    gc_init(g_heap, g_heap + g_heap_size);
    mp_init();

    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_path), 0);
    mp_obj_list_append(mp_sys_path, MP_OBJ_NEW_QSTR(MP_QSTR_));
    mp_obj_list_init(MP_OBJ_TO_PTR(mp_sys_argv), 0);

    g_initialized = 1;
    maix_mpy_set_error("ok");
    return 0;
}

void maix_mpy_deinit(void)
{
    if (!g_initialized)
    {
        return;
    }

    mp_deinit();
    g_heap_size = 0;
    g_stack_top = RT_NULL;
    g_initialized = 0;
    maix_mpy_set_error("deinitialized");
}

int maix_mpy_exec(const char *virtual_path, const char *source, size_t size)
{
    qstr source_name;
    nlr_buf_t nlr;

    if (virtual_path == RT_NULL || source == RT_NULL)
    {
        maix_mpy_set_error("invalid script input");
        return -1;
    }

    if (!g_initialized && maix_mpy_init() != 0)
    {
        return -1;
    }

    source_name = qstr_from_str(virtual_path);
    if (nlr_push(&nlr) == 0)
    {
        mp_lexer_t *lex = mp_lexer_new_from_str_len(source_name, source, size, 0);
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
        maix_mpy_set_error("ok");
        return 0;
    }

    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    maix_mpy_set_error("python exception in %s", virtual_path);
    return -1;
}

int maix_mpy_exec_file(const char *virtual_path)
{
    qstr source_name;
    nlr_buf_t nlr;

    if (virtual_path == RT_NULL || virtual_path[0] == '\0')
    {
        maix_mpy_set_error("invalid script path");
        return -1;
    }

    if (!g_initialized && maix_mpy_init() != 0)
    {
        return -1;
    }

    source_name = qstr_from_str(virtual_path);
    if (nlr_push(&nlr) == 0)
    {
        mp_lexer_t *lex = mp_lexer_new_from_file(virtual_path);
        mp_parse_tree_t parse_tree = mp_parse(lex, MP_PARSE_FILE_INPUT);
        mp_obj_t module_fun = mp_compile(&parse_tree, source_name, MP_EMIT_OPT_NONE, true);
        mp_call_function_0(module_fun);
        nlr_pop();
        maix_mpy_set_error("ok");
        return 0;
    }

    mp_obj_print_exception(&mp_plat_print, (mp_obj_t)nlr.ret_val);
    maix_mpy_set_error("python exception in %s", virtual_path);
    return -1;
}

const char *maix_mpy_last_error(void)
{
    return g_last_error;
}

size_t maix_mpy_heap_size(void)
{
    return g_heap_size;
}

void gc_collect(void)
{
    void *stack_ptr;

    gc_collect_start();
    if (g_stack_top != RT_NULL && (uintptr_t)g_stack_top > (uintptr_t)&stack_ptr)
    {
        gc_collect_root(&stack_ptr,
                        ((uintptr_t)g_stack_top - (uintptr_t)&stack_ptr) / sizeof(uintptr_t));
    }
    gc_collect_end();
}

mp_lexer_t *mp_lexer_new_from_file(const char *filename)
{
    const maix_bundle_file_t *file = maix_mpy_find_bundle_file(filename);

    if (file != RT_NULL)
    {
        return mp_lexer_new_from_str_len(qstr_from_str(file->path), (const char *)file->data, file->size, 0);
    }

#ifdef RT_USING_DFS
    {
        int fd = maix_mpy_open_posix_path(filename, O_RDONLY);
        struct stat st;
        size_t alloc_len;
        char *buf;
        size_t offset = 0;

        if (fd < 0)
        {
            mp_raise_OSError(errno);
        }

        if (fstat(fd, &st) != 0)
        {
            int err = errno;

            close(fd);
            mp_raise_OSError(err);
        }

        alloc_len = st.st_size > 0 ? (size_t)st.st_size : 1;
        buf = m_new(char, alloc_len);
        while (offset < (size_t)st.st_size)
        {
            int res = read(fd, buf + offset, (size_t)st.st_size - offset);

            if (res < 0)
            {
                int err = errno;

                close(fd);
                m_del(char, buf, alloc_len);
                mp_raise_OSError(err);
            }
            if (res == 0)
            {
                break;
            }
            offset += (size_t)res;
        }
        close(fd);
        return mp_lexer_new_from_str_len(qstr_from_str(filename), buf, offset, alloc_len);
    }
#else
    mp_raise_OSError(MP_ENOENT);
#endif
}

mp_import_stat_t mp_import_stat(const char *path)
{
    if (maix_mpy_find_bundle_file(path) != RT_NULL)
    {
        return MP_IMPORT_STAT_FILE;
    }

    if (maix_mpy_bundle_has_dir(path))
    {
        return MP_IMPORT_STAT_DIR;
    }

#ifdef RT_USING_DFS
    {
        char candidate[256];
        mp_import_stat_t stat;

        stat = maix_mpy_import_stat_posix(path);
        if (stat != MP_IMPORT_STAT_NO_EXIST)
        {
            return stat;
        }

        if (path != RT_NULL && path[0] != '\0' && strstr(path, ".py") == RT_NULL)
        {
            if (path[0] == '/')
            {
                if (snprintf(candidate, sizeof(candidate), "%s.py", path) < (int)sizeof(candidate))
                {
                    stat = maix_mpy_import_stat_posix(candidate);
                    if (stat == MP_IMPORT_STAT_FILE)
                    {
                        return stat;
                    }
                }
                if (snprintf(candidate, sizeof(candidate), "%s/__init__.py", path) < (int)sizeof(candidate))
                {
                    stat = maix_mpy_import_stat_posix(candidate);
                    if (stat == MP_IMPORT_STAT_FILE)
                    {
                        return MP_IMPORT_STAT_DIR;
                    }
                }
            }
            else
            {
                if (snprintf(candidate, sizeof(candidate), "/%s.py", path) < (int)sizeof(candidate))
                {
                    stat = maix_mpy_import_stat_posix(candidate);
                    if (stat == MP_IMPORT_STAT_FILE)
                    {
                        return stat;
                    }
                }
                if (snprintf(candidate, sizeof(candidate), "/%s/__init__.py", path) < (int)sizeof(candidate))
                {
                    stat = maix_mpy_import_stat_posix(candidate);
                    if (stat == MP_IMPORT_STAT_FILE)
                    {
                        return MP_IMPORT_STAT_DIR;
                    }
                }
            }
        }
    }
#endif

    return MP_IMPORT_STAT_NO_EXIST;
}

mp_obj_t mp_builtin_open(size_t n_args, const mp_obj_t *args, mp_map_t *kwargs)
{
    static const mp_arg_t open_args[] = {
        { MP_QSTR_file, MP_ARG_OBJ | MP_ARG_REQUIRED, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_mode, MP_ARG_OBJ, {.u_obj = MP_OBJ_NEW_QSTR(MP_QSTR_r)} },
        { MP_QSTR_buffering, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_encoding, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
    };

    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(open_args)];
    const char *path;
    const char *mode;
    const maix_bundle_file_t *file;
    maix_file_obj_t *stream_obj;
    bool binary = false;
    int flags = O_RDONLY;

    mp_arg_parse_all(n_args, args, kwargs, MP_ARRAY_SIZE(open_args), open_args, arg_vals);
    path = mp_obj_str_get_str(arg_vals[0].u_obj);
    mode = mp_obj_str_get_str(arg_vals[1].u_obj);

    while (*mode != '\0')
    {
        switch (*mode)
        {
        case 'r':
            flags = O_RDONLY;
            break;
        case 'b':
        case 't':
            break;
        case 'w':
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;
        case 'a':
            flags = O_WRONLY | O_CREAT | O_APPEND;
            break;
        case '+':
            if ((flags & O_APPEND) == O_APPEND)
            {
                flags = O_RDWR | O_CREAT | O_APPEND;
            }
            else if ((flags & O_TRUNC) == O_TRUNC)
            {
                flags = O_RDWR | O_CREAT | O_TRUNC;
            }
            else
            {
                flags = O_RDWR;
            }
            break;
        default:
            mp_raise_ValueError("unsupported mode");
        }

        if (*mode == 'b')
        {
            binary = true;
        }
        mode++;
    }

    file = maix_mpy_find_bundle_file(path);
    if (file != RT_NULL)
    {
        if ((flags & (O_WRONLY | O_RDWR | O_APPEND | O_TRUNC)) != 0)
        {
            mp_raise_OSError(MP_EROFS);
        }

        stream_obj = m_new_obj(maix_file_obj_t);
        stream_obj->base.type = binary ? &maix_fileio_type : &maix_textio_type;
        stream_obj->path_qstr = qstr_from_str(file->path);
        stream_obj->file = file;
        stream_obj->fd = -1;
        stream_obj->uses_posix = false;
        stream_obj->offset = 0;
        stream_obj->closed = false;
        return MP_OBJ_FROM_PTR(stream_obj);
    }

#ifdef RT_USING_DFS
    {
        int fd = maix_mpy_open_posix_path(path, flags);

        if (fd < 0)
        {
            mp_raise_OSError(errno);
        }

        stream_obj = m_new_obj(maix_file_obj_t);
        stream_obj->base.type = binary ? &maix_fileio_type : &maix_textio_type;
        stream_obj->path_qstr = qstr_from_str(path);
        stream_obj->file = RT_NULL;
        stream_obj->fd = fd;
        stream_obj->uses_posix = true;
        stream_obj->offset = 0;
        stream_obj->closed = false;
        return MP_OBJ_FROM_PTR(stream_obj);
    }
#else
    mp_raise_OSError(MP_ENOENT);
#endif
}
MP_DEFINE_CONST_FUN_OBJ_KW(mp_builtin_open_obj, 1, mp_builtin_open);

void nlr_jump_fail(void *val)
{
    (void)val;
    maix_mpy_set_error("uncaught nlr");
    rt_kprintf("[PY] fatal: uncaught non-local return\r\n");
    while (1)
    {
    }
}

void NORETURN __fatal_error(const char *msg)
{
    maix_mpy_set_error("fatal: %s", msg ? msg : "unknown");
    rt_kprintf("[PY] fatal: %s\r\n", msg ? msg : "unknown");
    while (1)
    {
    }
}

#ifndef NDEBUG
void MP_WEAK __assert_func(const char *file, int line, const char *func, const char *expr)
{
    rt_kprintf("Assertion '%s' failed, file %s, line %d, func %s\r\n",
               expr, file, line, func);
    __fatal_error("assert");
}
#endif

mp_uint_t mp_hal_ticks_ms(void)
{
    return rt_tick_get() * (1000 / RT_TICK_PER_SECOND);
}

void mp_hal_set_interrupt_char(char c)
{
    (void)c;
}

int mp_hal_stdin_rx_chr(void)
{
    for (;;)
    {
        rt_thread_mdelay(1000);
    }
}

void mp_hal_stdout_tx_strn(const char *str, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        rt_kprintf("%c", str[i]);
    }
}

void mp_hal_stdout_tx_strn_cooked(const char *str, size_t len)
{
    size_t i;

    for (i = 0; i < len; i++)
    {
        if (str[i] == '\n')
        {
            rt_kprintf("\r\n");
        }
        else
        {
            rt_kprintf("%c", str[i]);
        }
    }
}
