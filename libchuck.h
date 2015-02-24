//
//  libchuck.h
//  libchuck
//
//  Created by Spencer Salazar on 2/24/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#define LIBCHUCK_FUNC_DECL extern "C"

struct chuck_inst;

LIBCHUCK_FUNC_DECL chuck_inst *libchuck_create();
LIBCHUCK_FUNC_DECL void libchuck_destroy(chuck_inst *);

LIBCHUCK_FUNC_DECL void libchuck_vm_start(chuck_inst *);
LIBCHUCK_FUNC_DECL void libchuck_vm_stop(chuck_inst *);

LIBCHUCK_FUNC_DECL void libchuck_add_shred(chuck_inst *, const char *code);
LIBCHUCK_FUNC_DECL void libchuck_replace_shred(chuck_inst *, int shred_id, const char *code);
LIBCHUCK_FUNC_DECL void libchuck_remove_shred(chuck_inst *, int shred_id);

