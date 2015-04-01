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
#include "digiio_rtaudio.h"
#include "chuck_otf.h"
#include "util_string.h"

#include <stdio.h>
#include <unistd.h>
#include <string>
#include <vector>
#include <list>

using namespace std;

// in seconds
const int REPLY_TIMEOUT_MAX = 0.007; // a bit longer than a 256 sample buffer size


bool path_charIsSeparator(char c)
{
    return c == '/';
}

const char *path_getLastComponent(const char *path)
{
    const char *secondToLast = path;
    const char *last = path;
    
    for(int i = 0; path[i] != '\0'; i++)
    {
        if(path_charIsSeparator(path[i]))
        {
            last = path+i;
            secondToLast = last;
        }
    }
    
    if(last[1] == '\0') return secondToLast+1;
    else return last+1;
}

chuck_result init_result() { chuck_result result; memset(&result, 0, sizeof(chuck_result)); return result; }
chuck_result result_ok(int shred_id) { chuck_result result = init_result(); result.type = chuck_result::OK; result.shred_id = shred_id; return result; }
chuck_result err_file() { chuck_result result = init_result(); result.type = chuck_result::ERR_FILE; return result; }
chuck_result err_compile() { chuck_result result = init_result(); result.type = chuck_result::ERR_COMPILE; return result; }
chuck_result err_timeout() { chuck_result result = init_result(); result.type = chuck_result::ERR_TIMEOUT; return result; }


struct chuck_inst
{
    chuck_options m_options;
    
    Chuck_VM *m_vm;
    Chuck_Compiler *m_compiler;
    string m_last_err;
    
    list<t_CKBOOL (*)(Chuck_Env *)> query_funcs;
};

LIBCHUCK_FUNC_DECL void libchuck_options_reset(chuck_options *options)
{
    assert(options != NULL);
    
    memset(options, 0, sizeof(chuck_options));
    
    options->num_channels = 2;
    options->sample_rate = SAMPLING_RATE_DEFAULT;
    options->buffer_size = BUFFER_SIZE_DEFAULT;
    options->adaptive_buffer_size = 0;
    options->slave = false;
}

LIBCHUCK_FUNC_DECL chuck_inst *libchuck_create(chuck_options *options)
{
    chuck_inst *ck = new chuck_inst;
    
    if(ck == NULL) return NULL;
    
    ck->m_vm = NULL;
    ck->m_compiler = NULL;
    ck->m_options = *options;
    
    EM_setlog(CK_LOG_INFO);
    
    return ck;
}

LIBCHUCK_FUNC_DECL void libchuck_destroy(chuck_inst *ck)
{
    if(ck != NULL)
        delete ck;
}

LIBCHUCK_FUNC_DECL void libchuck_add_module(chuck_inst *ck, void *query_func)
{
    // do nothing
//    ck->query_funcs.push_back((t_CKBOOL (*)(Chuck_Env *)) query_func);
}

LIBCHUCK_FUNC_DECL int libchuck_vm_start(chuck_inst *ck)
{
    if( ck->m_vm == NULL )
    {
        // log
        EM_log( CK_LOG_INFO, "allocating VM..." );
        t_CKBOOL enable_audio = ck->m_options.slave ? FALSE : TRUE;
        t_CKBOOL vm_halt = FALSE;
        t_CKUINT srate = ck->m_options.sample_rate;
        t_CKUINT buffer_size = ck->m_options.buffer_size;
        t_CKUINT num_buffers = 8;
        t_CKUINT dac = 0;
        t_CKUINT adc = 0;
//        t_CKBOOL set_priority = FALSE;
        t_CKBOOL block = FALSE;
        t_CKBOOL force_srate = FALSE;
        t_CKUINT adaptive = ck->m_options.adaptive_buffer_size;
        t_CKBOOL slave = ck->m_options.slave;
        t_CKUINT output_channels = ck->m_options.num_channels;
        t_CKUINT input_channels = ck->m_options.num_channels;
        
        // set watchdog
        g_do_watchdog = FALSE;
        
        // allocate the vm - needs the type system
        ck->m_vm = g_vm = new Chuck_VM;
        
        Digitalio::m_init = FALSE;
        
        if( !ck->m_vm->initialize( enable_audio, vm_halt, srate, buffer_size,
                                   num_buffers, dac, adc, output_channels,
                                   input_channels, block, adaptive, force_srate, slave ) )
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
//        std::string chugin_path = g_default_chugin_path;
//        // parse the colon list into STL list (added 1.3.0.0)
//        parse_path_list( chugin_path, library_paths );
        std::list<std::string> named_chugins;
        // normalize paths
        for(std::list<std::string>::iterator i = library_paths.begin();
            i != library_paths.end(); i++)
            *i = expand_filepath(*i);
        for(std::list<std::string>::iterator j = named_chugins.begin();
            j != named_chugins.end(); j++)
            *j = expand_filepath(*j);
        
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
        
        for(list<t_CKBOOL (*)(Chuck_Env *)>::iterator i = ck->query_funcs.begin(); i != ck->query_funcs.end(); i++)
            (*i)( ck->m_compiler->env );
        
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
        
        ck->m_vm->run( (t_CKBOOL) FALSE );
    }
    
    return TRUE;
}

LIBCHUCK_FUNC_DECL int libchuck_vm_stop(chuck_inst *ck)
{
    if( ck->m_vm )
    {
        EM_log( CK_LOG_SYSTEM, "stopping chuck virtual machine..." );
        // get vm
        Chuck_VM * the_vm = ck->m_vm;
        // flag the global one
        ck->m_vm = g_vm = NULL;
        // if not NULL
        if( the_vm )
        {
            // stop
            the_vm->stop();
            
            // wait a bit
//            usleep( 100000 );
            
            // detach
            all_detach();
            
            SAFE_DELETE( the_vm );
        }
        
        SAFE_DELETE( ck->m_compiler );
        g_compiler = ck->m_compiler = NULL;
    }
    
    return TRUE;
}

LIBCHUCK_FUNC_DECL chuck_result libchuck_add_shred(chuck_inst *ck, const char *filepath, const char *code)
{
    const char *name = path_getLastComponent(filepath);
    FILE *file = NULL;
    
    if(code == NULL)
    {
        file = fopen(filepath, "r");
        if(file == NULL)
            return err_file();
    }
    
    chuck_result result;
    
    if(ck->m_compiler->go(name, file, code, filepath))
    {
        // allocate the VM message struct
        Chuck_Msg * msg = new Chuck_Msg;
        unsigned int shred_id = 0;
        
        // fill in the VM message
        msg->code = ck->m_compiler->output();
        
        msg->code->name = name;
        msg->type = MSG_ADD;
        msg->reply = ( ck_msg_func )1;
        msg->args = new vector<string>();
        
        // execute
        ck->m_vm->queue_msg( msg, 1 );
        
        // check results
        Chuck_Msg * reply = NULL;
        int usleep_dur = 100;
        int num_tries = REPLY_TIMEOUT_MAX/(usleep_dur/1000000.0);
        for(int i = 0; i < num_tries && (reply = ck->m_vm->get_reply()) == NULL; i++) { usleep(100); }
        
        if(reply == NULL)
        {
            result = err_timeout();
        }
        else
        {
            if(reply->type == MSG_ADD)
                shred_id = (unsigned int) reply->replyA;
            result = result_ok(shred_id);
        }
        
        //        delete msg;
    }
    else
    {
        ck->m_last_err = string(EM_lasterror());
        result = err_compile();
    }
    
    if(file) { fclose(file); file = NULL; }
    
    return result;
}

LIBCHUCK_FUNC_DECL chuck_result libchuck_replace_shred(chuck_inst *ck, int shred_id, const char *filepath, const char *code)
{
    const char *name = path_getLastComponent(filepath);
    FILE *file = NULL;
    
    if(code == NULL)
    {
        file = fopen(filepath, "r");
        if(file == NULL)
            return err_file();
    }
    
    chuck_result result;
    
    if(ck->m_compiler->go(name, file, code, filepath))
    {
        // allocate the VM message struct
        Chuck_Msg * msg = new Chuck_Msg;
        unsigned int shred_id = 0;
        
        // fill in the VM message
        msg->code = ck->m_compiler->output();
        
        msg->code->name = name;
        msg->type = MSG_REPLACE;
        msg->param = shred_id;
        msg->reply = ( ck_msg_func )1;
        msg->args = new vector<string>();
        
        // execute
        ck->m_vm->queue_msg( msg, 1 );
        
        // check results
        Chuck_Msg * reply = NULL;
        int usleep_dur = 100;
        int num_tries = REPLY_TIMEOUT_MAX/(usleep_dur/1000000.0);
        for(int i = 0; i < num_tries && (reply = ck->m_vm->get_reply()) == NULL; i++) { usleep(100); }
        
        if(reply == NULL)
        {
            result = err_timeout();
        }
        else
        {
            if(reply->type == MSG_REPLACE)
                shred_id = (unsigned int) reply->replyA;
            result = result_ok(shred_id);
        }
        
        //        delete msg;
    }
    else
    {
        ck->m_last_err = string(EM_lasterror());
        result = err_compile();
    }
    
    if(file) { fclose(file); file = NULL; }
    
    return result;
}

LIBCHUCK_FUNC_DECL chuck_result libchuck_remove_shred(chuck_inst *ck, int shred_id)
{
    chuck_result result;
    
    Chuck_Msg * msg = new Chuck_Msg;
    
    msg->type = MSG_REMOVE;
    msg->param = shred_id;
    msg->reply = ( ck_msg_func )1;
    
    ck->m_vm->queue_msg( msg, 1 );
    
    // check results
    Chuck_Msg * reply = NULL;
    int usleep_dur = 100;
    int num_tries = REPLY_TIMEOUT_MAX/(usleep_dur/1000000.0);
    for(int i = 0; i < num_tries && (reply = ck->m_vm->get_reply()) == NULL; i++) { usleep(100); }
    
    if(reply == NULL)
    {
        result = err_timeout();
    }
    else
    {
        if(reply->type == MSG_ADD)
            shred_id = (unsigned int) reply->replyA;
        result = result_ok(shred_id);
    }
    
//    delete msg;
    
    return result;
}

LIBCHUCK_FUNC_DECL int libchuck_slave_process(chuck_inst *ck, float *input, float *output, int numFrames)
{
    return Digitalio::cb2(output, input, numFrames, 0, 0, ck->m_vm);
}

LIBCHUCK_FUNC_DECL const char *libchuck_last_error_string(chuck_inst *ck)
{
    return ck->m_last_err.c_str();
}

