//
//  libchuck.m
//  libchuck
//
//  Created by Spencer Salazar on 2/24/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#include "libchuck.h"

struct chuck_inst
{
    
};

LIBCHUCK_FUNC_DECL chuck_inst *libchuck_create()
{
    chuck_inst *ck = new chuck_inst;
    
    return ck;
}

LIBCHUCK_FUNC_DECL void libchuck_destroy(chuck_inst *ck)
{
    delete ck;
}

LIBCHUCK_FUNC_DECL void libchuck_vm_start(chuck_inst *)
{
    
}

LIBCHUCK_FUNC_DECL void libchuck_vm_stop(chuck_inst *)
{
    
}

LIBCHUCK_FUNC_DECL void libchuck_add_shred(chuck_inst *, const char *code)
{
    
}

LIBCHUCK_FUNC_DECL void libchuck_replace_shred(chuck_inst *, int shred_id, const char *code)
{
    
}

LIBCHUCK_FUNC_DECL void libchuck_remove_shred(chuck_inst *, int shred_id)
{
    
}

