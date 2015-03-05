//
//  AppDelegate.m
//  LibChucKTest
//
//  Created by Spencer Salazar on 2/27/15.
//  Copyright (c) 2015 Spencer Salazar. All rights reserved.
//

#import "AppDelegate.h"

#include "libchuck.h"


@interface AppDelegate ()

@end

@implementation AppDelegate


- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions {
    // Override point for customization after application launch.
    
    chuck_options options;
    options.sample_rate = 44100;
#if defined(TARGET_IPHONE_SIMULATOR)
    options.buffer_size = 512;
#elif defined(TARGET_OS_IPHONE)
    options.buffer_size = 512;
#endif
    options.num_channels = 2;
    options.slave = false;
    
    chuck_inst *ck = libchuck_create(&options);
    
    libchuck_vm_start(ck);
    
    libchuck_add_shred(ck, "", "SawOsc s => dac; 2::second => now;");
    
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application {
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application {
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application {
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application {
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application {
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end
