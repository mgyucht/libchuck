//
//  libchuck.m
//  libchuck
//
//  Created by Spencer Salazar on 2/24/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#include "libchuck.h"
#include "chuck_vm.h"
#include "chuck_compile.h"
#include "chuck_globals.h"
#include "util_thread.h"

struct chuck_inst
{
    chuck_options m_options;
    
    Chuck_VM *m_vm;
    Chuck_Compiler *m_compiler;
    XThread m_vm_thread;
};

static void *vm_cb(void *)
{
    // boost priority
    if( Chuck_VM::our_priority != 0x7fffffff )
    {
        // try
        if( !Chuck_VM::set_priority( Chuck_VM::our_priority, g_vm ) )
        {
            // error
            // libchuck TODO: log error
            // fprintf( stderr, "[chuck]: %s\n", g_vm->last_error() );
            return FALSE;
        }
    }
    
    // run the vm
    g_vm->run();
    
    // detach
    all_detach();
    
    // log
    // libchuck TODO: log error
    EM_log( CK_LOG_SEVERE, "VM callback process ending..." );
    
    // free vm
    //g_vm = NULL; SAFE_DELETE( g_vm );
    //SAFE_DELETE( g_vm );
    // free the compiler
    //SAFE_DELETE( compiler );
    
    return NULL;
}

LIBCHUCK_FUNC_DECL chuck_inst *libchuck_create(chuck_options *options)
{
    chuck_inst *ck = new chuck_inst;
    
    if(ck == NULL) return NULL;
    
    ck->m_options = *options;
    
    return ck;
}

LIBCHUCK_FUNC_DECL void libchuck_destroy(chuck_inst *ck)
{
    if(ck != NULL)
        delete ck;
}

LIBCHUCK_FUNC_DECL int libchuck_vm_start(chuck_inst *ck)
{
    if( ck->m_vm == NULL )
    {
        // log
        EM_log( CK_LOG_INFO, "allocating VM..." );
        t_CKBOOL enable_audio = TRUE;
        t_CKBOOL vm_halt = FALSE;
        t_CKUINT srate = ck->m_options.sample_rate;
        t_CKUINT buffer_size = ck->m_options.buffer_size;
        t_CKUINT num_buffers = 8;
        t_CKUINT dac = 0;
        t_CKUINT adc = 0;
        t_CKBOOL set_priority = FALSE;
        t_CKBOOL block = FALSE;
        t_CKUINT output_channels = ck->m_options.num_channels;
        t_CKUINT input_channels = ck->m_options.num_channels;
        
        // set watchdog
        g_do_watchdog = FALSE;
        
        // allocate the vm - needs the type system
        ck->m_vm = g_vm = new Chuck_VM;
        
        if( !ck->m_vm->initialize( enable_audio, vm_halt, srate, buffer_size,
                                   num_buffers, dac, adc, output_channels,
                                   input_channels, block ) )
        {
            fprintf( stderr, "[chuck]: %s\n", ck->m_vm->last_error() );
            // pop
            EM_poplog();
            return FALSE;
        }
        
        // log
        EM_log( CK_LOG_INFO, "allocating compiler..." );
        
        // allocate the compiler
        g_compiler = ck->m_compiler = new Chuck_Compiler;
        
        std::list<std::string> library_paths;
        std::list<std::string> named_chugins;
//        std::list<std::string> library_paths = vm_options.library_paths;
//        std::list<std::string> named_chugins = vm_options.named_chugins;
//        // normalize paths
//        for(std::list<std::string>::iterator i = library_paths.begin();
//            i != library_paths.end(); i++)
//            *i = expand_filepath(*i);
//        for(std::list<std::string>::iterator j = named_chugins.begin();
//            j != named_chugins.end(); j++)
//            *j = expand_filepath(*j);
        
        // initialize the compiler
        ck->m_compiler->initialize( ck->m_vm, library_paths, named_chugins );
        // enable dump
        ck->m_compiler->emitter->dump = FALSE;
        // set auto depend
        ck->m_compiler->set_auto_depend( FALSE );
        
        // vm synthesis subsystem - needs the type system
        if( !ck->m_vm->initialize_synthesis() )
        {
            fprintf( stderr, "[chuck]: %s\n", ck->m_vm->last_error() );
            // pop
            EM_poplog();
            return FALSE;
        }
        
//        for(list<t_CKBOOL (*)(Chuck_Env *)>::iterator i = vm_options.query_funcs.begin(); i != vm_options.query_funcs.end(); i++)
//            (*i)( ck->m_compiler->env );
        
        // reset the parser
        reset_parse();
        
        Chuck_VM_Code * code = NULL;
        Chuck_VM_Shred * shred = NULL;
        
        // whether or not chug should be enabled (added 1.3.0.0)
        EM_log( CK_LOG_SEVERE, "pre-loading ChucK libs..." );
        EM_pushlog();
        
        // iterate over list of ck files that the compiler found
        for( std::list<std::string>::iterator fck = ck->m_compiler->m_cklibs_to_preload.begin();
            fck != ck->m_compiler->m_cklibs_to_preload.end(); fck++)
        {
            // the filename
            std::string filename = *fck;
            
            // log
            EM_log( CK_LOG_SEVERE, "preloading '%s'...", filename.c_str() );
            // push indent
            EM_pushlog();
            
            // SPENCERTODO: what to do for full path
            std::string full_path = filename;
            
            // parse, type-check, and emit
            if( ck->m_compiler->go( filename, NULL, NULL, full_path ) )
            {
                // TODO: how to compilation handle?
                //return 1;
                
                // get the code
                code = ck->m_compiler->output();
                // name it - TODO?
                // code->name += string(argv[i]);
                
                // spork it
                shred = ck->m_vm->spork( code, NULL );
            }
            
            // pop indent
            EM_poplog();
        }
        
        // clear the list of chuck files to preload
        ck->m_compiler->m_cklibs_to_preload.clear();
        
        // pop log
        EM_poplog();
        
        // load user namespace
        ck->m_compiler->env->load_user_namespace();
        
        // start the vm handler threads
        ck->m_vm_thread.start(vm_cb);
    }
    
    return TRUE;
}

LIBCHUCK_FUNC_DECL int libchuck_vm_stop(chuck_inst *)
{
    return 0;
}

LIBCHUCK_FUNC_DECL int libchuck_add_shred(chuck_inst *, const char *code)
{
    return 0;
}

LIBCHUCK_FUNC_DECL int libchuck_replace_shred(chuck_inst *, int shred_id, const char *code)
{
    return 0;
}

LIBCHUCK_FUNC_DECL int libchuck_remove_shred(chuck_inst *, int shred_id)
{
    return 0;
}

