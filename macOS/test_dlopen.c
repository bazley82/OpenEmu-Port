#include <stdio.h>
#include <dlfcn.h>
#include <objc/runtime.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <path_to_bundle_binary>\n", argv[0]);
        return 1;
    }

    printf("Attempting to dlopen: %s\n", argv[1]);
    void *handle = dlopen(argv[1], RTLD_NOW);
    if (!handle) {
        printf("FAILED: %s\n", dlerror());
        return 1;
    }
    printf("SUCCESS: Loaded bundle.\n");

    // Check for Classes
    Class viceClass = objc_getClass("VICEGameCore");
    if (viceClass) {
        printf("SUCCESS: Found class 'VICEGameCore'\n");
    } else {
        printf("FAILED: Could not find class 'VICEGameCore'\n");
    }

    Class controllerClass = objc_getClass("OEGameCoreController");
    if (controllerClass) {
        printf("SUCCESS: Found class 'OEGameCoreController'\n");
    } else {
        printf("FAILED: Could not find class 'OEGameCoreController' (Might need to link OpenEmuBase?)\n");
    }
    
    // Check if VICEGameCore is a subclass of OEGameCore
    /*
    if (viceClass && controllerClass) {
        // ... runtime check
    }
    */

    // dlclose(handle); // Keep leak to avoid objc issues for now
    return 0;
}
