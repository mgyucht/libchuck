//
//  LibChuckObjc.mm
//  libchuck
//
//  Created by Miles Yucht on 3/12/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#import "LibChuckObjc.h"
#import "libchuck.h"

@interface LibChuckObjc ()

@property (nonatomic) chuck_inst * instance;
+ (ChuckResult)toChuckResult:(chuck_result)cResult;

@end

@implementation LibChuckObjc

+ (instancetype)create:(ChuckOptions)options {
    LibChuckObjc *chuck = [[LibChuckObjc alloc] init];
    chuck_options cOptions;
    cOptions.num_channels = options.numChannels;
    cOptions.sample_rate = options.sampleRate;
    cOptions.buffer_size = options.bufferSize;
    cOptions.slave = options.isSlave;
    chuck.instance = libchuck_create(&cOptions);
    return chuck;
}

- (void)destroy {
    libchuck_destroy(_instance);
}

- (NSInteger)startVM {
    return libchuck_vm_start(_instance);
}

- (NSInteger)stopVM {
    return libchuck_vm_stop(_instance);
}

+ (ChuckResult)toChuckResult:(chuck_result)cResult {
    ChuckResult result;
    switch (cResult.type) {
        case chuck_result::OK:
            result.type = LC_OK;
            result.shredId = cResult.shred_id;
            break;
        case chuck_result::ERR_COMPILE:
            result.type = LC_ERR_COMPILE;
            break;
        case chuck_result::ERR_NOSHRED:
            result.type = LC_ERR_NOSHRED;
            break;
        case chuck_result::ERR_FILE:
            result.type = LC_ERR_FILE;
            break;
        case chuck_result::ERR_BADINPUT:
            result.type = LC_ERR_BADINPUT;
            break;
        default:
            break;
    };
    return result;
}

- (ChuckResult)addShred:(NSString *)filePath code:(NSString *)code {
    const char *cFilePath = [filePath cStringUsingEncoding:NSUTF8StringEncoding];
    const char *cCode = [code cStringUsingEncoding:NSUTF8StringEncoding];
    chuck_result cResult = libchuck_add_shred(_instance, cFilePath, cCode);
    return [LibChuckObjc toChuckResult:cResult];
}

- (ChuckResult)replaceShred:(NSInteger)shredId pathToShred:(NSString *)filePath code:(NSString *)code {
    int cShredId = shredId;
    const char *cFilePath = [filePath cStringUsingEncoding:NSUTF8StringEncoding];
    const char *cCode = [code cStringUsingEncoding:NSUTF8StringEncoding];
    chuck_result cResult = libchuck_replace_shred(_instance, cShredId, cFilePath, cCode);
    return [LibChuckObjc toChuckResult:cResult];
}

- (ChuckResult)removeShred:(NSInteger)shredId {
    int cShredId = shredId;
    chuck_result cResult = libchuck_remove_shred(_instance, cShredId);
    return [LibChuckObjc toChuckResult:cResult];
}

- (NSInteger)slaveProcess:(float *)input output:(float *)output numFrames:(NSInteger)numFrames {
    int cNumFrames = numFrames;
    int cResult = libchuck_slave_process(_instance, input, output, cNumFrames);
    return cResult;
}

- (NSString *)lastErrorString {
    const char *cResult = libchuck_last_error_string(_instance);
    return [NSString stringWithUTF8String:cResult];
}

@end
