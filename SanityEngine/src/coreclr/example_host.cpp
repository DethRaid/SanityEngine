#include <string>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// https://github.com/dotnet/coreclr/blob/master/src/coreclr/hosts/inc/coreclrhost.h
#include "coreclrhost.hpp"

#define MANAGED_ASSEMBLY "ManagedLibrary.dll"

// Define OS-specific items like the CoreCLR library's name and path elements
#include <D3D12MemAlloc.h>
#include <Windows.h>

#include "core_clr_host.hpp"
#define FS_SEPARATOR "\\"
#define PATH_DELIMITER ";"
#define CORECLR_FILE_NAME "coreclr.dll"

// Function pointer types for the managed call and callback
typedef int (*report_callback_ptr)(int progress);
typedef char* (*doWork_ptr)(const char* jobName, int iterations, int dataSize, double* data, report_callback_ptr callbackFunction);

void BuildTpaList(const char* directory, const char* extension, std::string& tpaList);
int ReportProgressCallback(int progress);

int main(int argc, char* argv[]) {
    // Get the current executable's directory
    // This sample assumes that both CoreCLR and the
    // managed assembly to be loaded are next to this host
    // so we need to get the current path in order to locate those.
    char runtimePath[MAX_PATH];
    GetFullPathNameA(argv[0], MAX_PATH, runtimePath, NULL);
    char* last_slash = strrchr(runtimePath, FS_SEPARATOR[0]);
    if(last_slash != NULL)
        *last_slash = 0;

    // Construct the CoreCLR path
    // For this sample, we know CoreCLR's path. For other hosts,
    // it may be necessary to probe for coreclr.dll/libcoreclr.so
    std::string coreClrPath(runtimePath);
    coreClrPath.append(FS_SEPARATOR);
    coreClrPath.append(CORECLR_FILE_NAME);

    // Construct the managed library path
    std::string managedLibraryPath(runtimePath);
    managedLibraryPath.append(FS_SEPARATOR);
    managedLibraryPath.append(MANAGED_ASSEMBLY);

    //
    // STEP 1: Load CoreCLR (coreclr.dll/libcoreclr.so)
    //
    // <Snippet1>
    HMODULE coreClr = LoadLibraryExA(coreClrPath.c_str(), NULL, 0);
    // </Snippet1>
    if(coreClr == NULL) {
        printf("ERROR: Failed to load CoreCLR from %s\n", coreClrPath.c_str());
        return -1;
    } else {
        printf("Loaded CoreCLR from %s\n", coreClrPath.c_str());
    }

    //
    // STEP 2: Get CoreCLR hosting functions
    //
    // <Snippet2>
    coreclr_initialize_ptr initializeCoreClr = reinterpret_cast<coreclr_initialize_ptr>(GetProcAddress(coreClr, "coreclr_initialize"));
    coreclr_create_delegate_ptr createManagedDelegate = (coreclr_create_delegate_ptr) GetProcAddress(coreClr, "coreclr_create_delegate");
    coreclr_shutdown_ptr shutdownCoreClr = (coreclr_shutdown_ptr) GetProcAddress(coreClr, "coreclr_shutdown");
    // </Snippet2>

    if(initializeCoreClr == NULL) {
        printf("coreclr_initialize not found");
        return -1;
    }

    if(createManagedDelegate == NULL) {
        printf("coreclr_create_delegate not found");
        return -1;
    }

    if(shutdownCoreClr == NULL) {
        printf("coreclr_shutdown not found");
        return -1;
    }

    //
    // STEP 3: Construct properties used when starting the runtime
    //

    // Construct the trusted platform assemblies (TPA) list
    // This is the list of assemblies that .NET Core can load as
    // trusted system assemblies.
    // For this host (as with most), assemblies next to CoreCLR will
    // be included in the TPA list
    std::string tpaList;
    BuildTpaList(runtimePath, ".dll", tpaList);

    // <Snippet3>
    // Define CoreCLR properties
    // Other properties related to assembly loading are common here,
    // but for this simple sample, TRUSTED_PLATFORM_ASSEMBLIES is all
    // that is needed. Check hosting documentation for other common properties.
    const char* propertyKeys[] = {
        "TRUSTED_PLATFORM_ASSEMBLIES" // Trusted assemblies
    };

    const char* propertyValues[] = {tpaList.c_str()};
    // </Snippet3>

    //
    // STEP 4: Start the CoreCLR runtime
    //

    // <Snippet4>
    void* hostHandle;
    unsigned int domainId;

    // This function both starts the .NET Core runtime and creates
    // the default (and only) AppDomain
    int hr = initializeCoreClr(runtimePath,                          // App base path
                               "SampleHost",                         // AppDomain friendly name
                               sizeof(propertyKeys) / sizeof(char*), // Property count
                               propertyKeys,                         // Property names
                               propertyValues,                       // Property values
                               &hostHandle,                          // Host handle
                               &domainId);                           // AppDomain ID
    // </Snippet4>

    if(hr >= 0) {
        printf("CoreCLR started\n");
    } else {
        printf("coreclr_initialize failed - status: 0x%08x\n", hr);
        return -1;
    }

    //
    // STEP 5: Create delegate to managed code and invoke it
    //

    // <Snippet5>
    doWork_ptr managedDelegate;

    // The assembly name passed in the third parameter is a managed assembly name
    // as described at https://docs.microsoft.com/dotnet/framework/app-domains/assembly-names
    hr = createManagedDelegate(hostHandle,
                               domainId,
                               "ManagedLibrary, Version=1.0.0.0",
                               "ManagedLibrary.ManagedWorker",
                               "DoWork",
                               (void**) &managedDelegate);
    // </Snippet5>

    if(hr >= 0) {
        printf("Managed delegate created\n");
    } else {
        printf("coreclr_create_delegate failed - status: 0x%08x\n", hr);
        return -1;
    }

    // Create sample data for the double[] argument of the managed method to be called
    double data[4];
    data[0] = 0;
    data[1] = 0.25;
    data[2] = 0.5;
    data[3] = 0.75;

    // Invoke the managed delegate and write the returned string to the console
    char* ret = managedDelegate("Test job", 5, sizeof(data) / sizeof(double), data, ReportProgressCallback);

    printf("Managed code returned: %s\n", ret);

    // Strings returned to native code must be freed by the native code
    CoTaskMemFree(ret);

    //
    // STEP 6: Shutdown CoreCLR
    //

    // <Snippet6>
    hr = shutdownCoreClr(hostHandle, domainId);
    // </Snippet6>

    if(hr >= 0) {
        printf("CoreCLR successfully shutdown\n");
    } else {
        printf("coreclr_shutdown failed - status: 0x%08x\n", hr);
    }

    return 0;
}

// Win32 directory search for .dll files
// <Snippet7>
void BuildTpaList(const char* directory, const char* extension, std::string& tpaList) {
    // This will add all files with a .dll extension to the TPA list.
    // This will include unmanaged assemblies (coreclr.dll, for example) that don't
    // belong on the TPA list. In a real host, only managed assemblies that the host
    // expects to load should be included. Having extra unmanaged assemblies doesn't
    // cause anything to fail, though, so this function just enumerates all dll's in
    // order to keep this sample concise.
    std::string searchPath(directory);
    searchPath.append(FS_SEPARATOR);
    searchPath.append("*");
    searchPath.append(extension);

    WIN32_FIND_DATAA findData;
    HANDLE fileHandle = FindFirstFileA(searchPath.c_str(), &findData);

    if(fileHandle != INVALID_HANDLE_VALUE) {
        do {
            // Append the assembly to the list
            tpaList.append(directory);
            tpaList.append(FS_SEPARATOR);
            tpaList.append(findData.cFileName);
            tpaList.append(PATH_DELIMITER);

            // Note that the CLR does not guarantee which assembly will be loaded if an assembly
            // is in the TPA list multiple times (perhaps from different paths or perhaps with different NI/NI.dll
            // extensions. Therefore, a real host should probably add items to the list in priority order and only
            // add a file if it's not already present on the list.
            //
            // For this simple sample, though, and because we're only loading TPA assemblies from a single path,
            // and have no native images, we can ignore that complication.
        } while(FindNextFileA(fileHandle, &findData));
        FindClose(fileHandle);
    }
}
// </Snippet7>

// Callback function passed to managed code to facilitate calling back into native code with status
int ReportProgressCallback(int progress) {
    // Just print the progress parameter to the console and return -progress
    printf("Received status from managed code: %d\n", progress);
    return -progress;
}