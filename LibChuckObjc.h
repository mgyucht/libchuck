//
//  LibChuckObjc.h
//  libchuck
//
//  Created by Miles Yucht on 3/12/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#import <Foundation/Foundation.h>

typedef struct {
    NSInteger numChannels;
    NSInteger sampleRate;
    NSInteger bufferSize;
    BOOL isSlave;
} ChuckOptions;

typedef NS_ENUM(NSUInteger, ChuckResultType) {
    LC_OK,
    LC_ERR_COMPILE,
    LC_ERR_NOSHRED,
    LC_ERR_BADINPUT,
    LC_ERR_FILE
};

typedef struct {
    ChuckResultType type;
    NSInteger shredId; // only valid if OK
} ChuckResult;

@interface LibChuckObjc : NSObject

+ (instancetype)instance;
+ (void)create:(ChuckOptions)options;
+ (void)destroy;

+ (NSInteger)startVM;
+ (NSInteger)stopVM;

+ (ChuckResult)addShred:(NSURL *)filePath code:(NSString *)code;
+ (ChuckResult)replaceShred:(NSInteger)shredId pathToShred:(NSURL *)filePath code:(NSString *)code;
+ (ChuckResult)removeShred:(NSInteger)shredId;

+ (NSInteger)slaveProcess:(float *)input output:(float *)output numFrames:(NSInteger)numFrames;
+ (NSString *)lastErrorString;

@end
