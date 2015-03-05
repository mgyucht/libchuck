//
//  libchuck.h
//  libchuck
//
//  Created by Spencer Salazar on 2/24/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#ifndef LIBCHUCK_H
#define LIBCHUCK_H


#ifdef __cplusplus
#define LIBCHUCK_FUNC_DECL extern "C"
#else
#define LIBCHUCK_FUNC_DECL
#endif // __cplusplus


typedef struct chuck_inst chuck_inst;

typedef struct chuck_options
{
    int num_channels;
    int sample_rate;
    int buffer_size;
    bool slave;
} chuck_options;

typedef struct chuck_result
{
    enum {
        OK,
        ERR_COMPILE,
        ERR_NOSHRED,
        ERR_BADINPUT,
        ERR_FILE,
    } type;
    
    int shred_id; // only valid if OK
} chuck_result;

LIBCHUCK_FUNC_DECL chuck_inst *libchuck_create(chuck_options *options);
LIBCHUCK_FUNC_DECL void libchuck_destroy(chuck_inst *);

LIBCHUCK_FUNC_DECL int libchuck_vm_start(chuck_inst *);
LIBCHUCK_FUNC_DECL int libchuck_vm_stop(chuck_inst *);

LIBCHUCK_FUNC_DECL chuck_result libchuck_add_shred(chuck_inst *, const char *filepath, const char *code);
LIBCHUCK_FUNC_DECL chuck_result libchuck_replace_shred(chuck_inst *, int shred_id, const char *filepath,  const char *code);
LIBCHUCK_FUNC_DECL chuck_result libchuck_remove_shred(chuck_inst *, int shred_id);

LIBCHUCK_FUNC_DECL int libchuck_slave_process(chuck_inst *, float *input, float *output, int numFrames);

LIBCHUCK_FUNC_DECL const char *libchuck_last_error_string(chuck_inst *);


#endif // LIBCHUCK_H

