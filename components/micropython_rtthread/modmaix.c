#include <string.h>

#include "maix_runtime_fs.h"
#include "maix_runtime_model.h"
#include "maix_runtime_python.h"
#include "py/obj.h"
#include "py/objstr.h"
#include "py/runtime.h"

STATIC mp_obj_t maix_app_app_id(void)
{
    const char *app_id = maix_runtime_fs_auto_start_app_id();
    return mp_obj_new_str(app_id, strlen(app_id));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_app_app_id_obj, maix_app_app_id);

STATIC mp_obj_t maix_app_script_path(void)
{
    const char *script_path = maix_runtime_python_selected_script();
    return mp_obj_new_str(script_path, strlen(script_path));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_app_script_path_obj, maix_app_script_path);

STATIC mp_obj_t maix_app_model_path(void)
{
    const char *model_path = maix_runtime_model_active_path();

    if (model_path == NULL)
    {
        return mp_const_none;
    }

    return mp_obj_new_str(model_path, strlen(model_path));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_app_model_path_obj, maix_app_model_path);

STATIC mp_obj_t maix_app_storage_backend(void)
{
    const char *backend = maix_runtime_fs_storage_backend();
    return mp_obj_new_str(backend, strlen(backend));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_app_storage_backend_obj, maix_app_storage_backend);

STATIC mp_obj_t maix_model_exists(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    return mp_obj_new_bool(info != NULL && info->present);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_exists_obj, maix_model_exists);

STATIC mp_obj_t maix_model_path(void)
{
    const char *model_path = maix_runtime_model_active_path();

    if (model_path == NULL)
    {
        return mp_const_none;
    }

    return mp_obj_new_str(model_path, strlen(model_path));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_path_obj, maix_model_path);

STATIC mp_obj_t maix_model_format(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    const char *format = (info != NULL && info->format != NULL) ? info->format : "none";
    return mp_obj_new_str(format, strlen(format));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_format_obj, maix_model_format);

STATIC mp_obj_t maix_model_labels_path(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    const char *labels_path = (info != NULL) ? info->labels_path : NULL;

    if (labels_path == NULL)
    {
        return mp_const_none;
    }

    return mp_obj_new_str(labels_path, strlen(labels_path));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_labels_path_obj, maix_model_labels_path);

STATIC mp_obj_t maix_model_size(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    mp_uint_t value = 0;

    if (info != NULL)
    {
        value = (mp_uint_t)info->size_bytes;
    }

    return mp_obj_new_int_from_uint(value);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_size_obj, maix_model_size);

STATIC mp_obj_t maix_model_backend(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    const char *backend = (info != NULL && info->path_backend != NULL) ? info->path_backend : "missing";
    return mp_obj_new_str(backend, strlen(backend));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_backend_obj, maix_model_backend);

STATIC mp_obj_t maix_model_info_dict(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    mp_obj_t items[8];

    if (info == NULL)
    {
        items[0] = mp_const_none;
        items[1] = mp_obj_new_str("none", 4);
        items[2] = mp_const_none;
        items[3] = mp_obj_new_int(0);
        items[4] = mp_obj_new_str("missing", 7);
        items[5] = mp_obj_new_str("missing", 7);
        items[6] = mp_const_false;
        items[7] = mp_const_false;
        return mp_obj_new_tuple(8, items);
    }

    items[0] = info->path ? mp_obj_new_str(info->path, strlen(info->path)) : mp_const_none;
    items[1] = mp_obj_new_str(info->format ? info->format : "none",
                              strlen(info->format ? info->format : "none"));
    items[2] = info->labels_path ? mp_obj_new_str(info->labels_path, strlen(info->labels_path)) : mp_const_none;
    items[3] = mp_obj_new_int_from_uint((mp_uint_t)info->size_bytes);
    items[4] = mp_obj_new_str(info->path_backend ? info->path_backend : "missing",
                              strlen(info->path_backend ? info->path_backend : "missing"));
    items[5] = mp_obj_new_str(info->signature ? info->signature : "missing",
                              strlen(info->signature ? info->signature : "missing"));
    items[6] = mp_obj_new_bool(info->present);
    items[7] = mp_obj_new_bool(info->readable);
    return mp_obj_new_tuple(8, items);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(maix_model_info_dict_obj, maix_model_info_dict);

typedef struct _maix_nn_obj_t
{
    mp_obj_base_t base;
    mp_obj_t requested_path;
} maix_nn_obj_t;

STATIC mp_obj_t maix_nn_build_info_dict(void)
{
    const maix_runtime_model_info_t *info = maix_runtime_model_info();
    const maix_runtime_model_session_info_t *session = maix_runtime_model_session_info();
    mp_obj_t dict = mp_obj_new_dict(0);

    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_path),
                      session->path ? mp_obj_new_str(session->path, strlen(session->path)) : mp_const_none);
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_format),
                      mp_obj_new_str(session->format ? session->format : "none",
                                     strlen(session->format ? session->format : "none")));
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_backend),
                      mp_obj_new_str(session->runtime_backend ? session->runtime_backend : "none",
                                     strlen(session->runtime_backend ? session->runtime_backend : "none")));
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_loaded),
                      mp_obj_new_bool(session->loaded));
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_last_error),
                      mp_obj_new_str(session->last_error ? session->last_error : "unknown",
                                     strlen(session->last_error ? session->last_error : "unknown")));
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_labels_path),
                      info && info->labels_path ? mp_obj_new_str(info->labels_path, strlen(info->labels_path)) : mp_const_none);
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_input_size),
                      mp_obj_new_int_from_uint((mp_uint_t)session->last_input_size));
    mp_obj_dict_store(dict,
                      MP_OBJ_NEW_QSTR(MP_QSTR_output_size),
                      mp_obj_new_int_from_uint((mp_uint_t)session->last_output_size));
    return dict;
}

STATIC void maix_nn_raise_last_error(void)
{
    nlr_raise(mp_obj_new_exception_msg(&mp_type_RuntimeError, maix_runtime_model_last_error()));
}

STATIC mp_obj_t maix_nn_make_new(const mp_obj_type_t *type,
                                 size_t n_args,
                                 size_t n_kw,
                                 const mp_obj_t *all_args)
{
    static const mp_arg_t allowed_args[] = {
        { MP_QSTR_path, MP_ARG_OBJ, {.u_rom_obj = MP_ROM_PTR(&mp_const_none_obj)} },
        { MP_QSTR_dual_buff, MP_ARG_BOOL, {.u_bool = false} },
    };
    mp_arg_val_t arg_vals[MP_ARRAY_SIZE(allowed_args)];
    maix_nn_obj_t *self = m_new_obj(maix_nn_obj_t);
    const char *path = NULL;

    mp_arg_parse_all_kw_array(n_args, n_kw, all_args, MP_ARRAY_SIZE(allowed_args), allowed_args, arg_vals);
    self->base.type = type;
    self->requested_path = arg_vals[0].u_obj;

    if (self->requested_path != mp_const_none)
    {
        path = mp_obj_str_get_str(self->requested_path);
    }

    if (path != NULL || maix_runtime_model_active_path() != NULL)
    {
        (void)maix_runtime_model_load(path);
    }

    (void)arg_vals[1].u_bool;
    return MP_OBJ_FROM_PTR(self);
}

STATIC mp_obj_t maix_nn_load(size_t n_args, const mp_obj_t *args)
{
    maix_nn_obj_t *self = MP_OBJ_TO_PTR(args[0]);
    const char *path = NULL;

    if (n_args > 1 && args[1] != mp_const_none)
    {
        self->requested_path = args[1];
    }

    if (self->requested_path != mp_const_none)
    {
        path = mp_obj_str_get_str(self->requested_path);
    }

    return mp_obj_new_bool(maix_runtime_model_load(path) == 0);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(maix_nn_load_obj, 1, 2, maix_nn_load);

STATIC mp_obj_t maix_nn_loaded(mp_obj_t self_in)
{
    (void)self_in;
    return mp_obj_new_bool(maix_runtime_model_is_loaded());
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(maix_nn_loaded_obj, maix_nn_loaded);

STATIC mp_obj_t maix_nn_last_error(mp_obj_t self_in)
{
    (void)self_in;
    return mp_obj_new_str(maix_runtime_model_last_error(), strlen(maix_runtime_model_last_error()));
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(maix_nn_last_error_obj, maix_nn_last_error);

STATIC mp_obj_t maix_nn_info(mp_obj_t self_in)
{
    (void)self_in;
    return maix_nn_build_info_dict();
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(maix_nn_info_obj, maix_nn_info);

STATIC mp_obj_t maix_nn_forward(size_t n_args, const mp_obj_t *args)
{
    mp_buffer_info_t input_buf;
    unsigned char output_buf[512];
    size_t output_size = 0;

    (void)args[0];
    if (!maix_runtime_model_is_loaded())
    {
        maix_nn_raise_last_error();
    }

    mp_get_buffer_raise(args[1], &input_buf, MP_BUFFER_READ);
    if (maix_runtime_model_forward((const unsigned char *)input_buf.buf,
                                   (size_t)input_buf.len,
                                   output_buf,
                                   sizeof(output_buf),
                                   &output_size) != 0)
    {
        maix_nn_raise_last_error();
    }

    return mp_obj_new_bytes(output_buf, output_size);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_VAR_BETWEEN(maix_nn_forward_obj, 2, 3, maix_nn_forward);

STATIC mp_obj_t maix_nn_close(mp_obj_t self_in)
{
    (void)self_in;
    maix_runtime_model_unload();
    return mp_const_none;
}
STATIC MP_DEFINE_CONST_FUN_OBJ_1(maix_nn_close_obj, maix_nn_close);

STATIC const mp_rom_map_elem_t maix_nn_locals_dict_table[] = {
    { MP_ROM_QSTR(MP_QSTR_load), MP_ROM_PTR(&maix_nn_load_obj) },
    { MP_ROM_QSTR(MP_QSTR_loaded), MP_ROM_PTR(&maix_nn_loaded_obj) },
    { MP_ROM_QSTR(MP_QSTR_last_error), MP_ROM_PTR(&maix_nn_last_error_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&maix_nn_info_obj) },
    { MP_ROM_QSTR(MP_QSTR_forward), MP_ROM_PTR(&maix_nn_forward_obj) },
    { MP_ROM_QSTR(MP_QSTR_close), MP_ROM_PTR(&maix_nn_close_obj) },
};
STATIC MP_DEFINE_CONST_DICT(maix_nn_locals_dict, maix_nn_locals_dict_table);

STATIC const mp_obj_type_t maix_nn_type = {
    { &mp_type_type },
    .name = MP_QSTR_NN,
    .make_new = maix_nn_make_new,
    .locals_dict = (mp_obj_dict_t *)&maix_nn_locals_dict,
};

STATIC const mp_rom_map_elem_t maix_nn_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_nn) },
    { MP_ROM_QSTR(MP_QSTR_NN), MP_ROM_PTR(&maix_nn_type) },
};
STATIC MP_DEFINE_CONST_DICT(maix_nn_module_globals, maix_nn_module_globals_table);

STATIC const mp_obj_module_t mp_module_maix_nn = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&maix_nn_module_globals,
};

STATIC const mp_rom_map_elem_t maix_app_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_app) },
    { MP_ROM_QSTR(MP_QSTR_app_id), MP_ROM_PTR(&maix_app_app_id_obj) },
    { MP_ROM_QSTR(MP_QSTR_script_path), MP_ROM_PTR(&maix_app_script_path_obj) },
    { MP_ROM_QSTR(MP_QSTR_model_path), MP_ROM_PTR(&maix_app_model_path_obj) },
    { MP_ROM_QSTR(MP_QSTR_storage_backend), MP_ROM_PTR(&maix_app_storage_backend_obj) },
};
STATIC MP_DEFINE_CONST_DICT(maix_app_module_globals, maix_app_module_globals_table);

STATIC const mp_obj_module_t mp_module_maix_app = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&maix_app_module_globals,
};

STATIC const mp_rom_map_elem_t maix_model_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_model) },
    { MP_ROM_QSTR(MP_QSTR_exists), MP_ROM_PTR(&maix_model_exists_obj) },
    { MP_ROM_QSTR(MP_QSTR_path), MP_ROM_PTR(&maix_model_path_obj) },
    { MP_ROM_QSTR(MP_QSTR_format), MP_ROM_PTR(&maix_model_format_obj) },
    { MP_ROM_QSTR(MP_QSTR_labels_path), MP_ROM_PTR(&maix_model_labels_path_obj) },
    { MP_ROM_QSTR(MP_QSTR_size), MP_ROM_PTR(&maix_model_size_obj) },
    { MP_ROM_QSTR(MP_QSTR_backend), MP_ROM_PTR(&maix_model_backend_obj) },
    { MP_ROM_QSTR(MP_QSTR_info), MP_ROM_PTR(&maix_model_info_dict_obj) },
};
STATIC MP_DEFINE_CONST_DICT(maix_model_module_globals, maix_model_module_globals_table);

STATIC const mp_obj_module_t mp_module_maix_model = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&maix_model_module_globals,
};

STATIC const mp_rom_map_elem_t maix_module_globals_table[] = {
    { MP_ROM_QSTR(MP_QSTR___name__), MP_ROM_QSTR(MP_QSTR_maix) },
    { MP_ROM_QSTR(MP_QSTR_app), MP_ROM_PTR(&mp_module_maix_app) },
    { MP_ROM_QSTR(MP_QSTR_model), MP_ROM_PTR(&mp_module_maix_model) },
    { MP_ROM_QSTR(MP_QSTR_nn), MP_ROM_PTR(&mp_module_maix_nn) },
};
STATIC MP_DEFINE_CONST_DICT(maix_module_globals, maix_module_globals_table);

const mp_obj_module_t mp_module_maix = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t *)&maix_module_globals,
};
