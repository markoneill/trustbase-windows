# Trustbase for Windows

[TOC]

## Structure Overview

Trustbase for Windows has two main subgroups, the **Kernel Driver** and the **Policy Engine**. The **Kernel Driver** monitors connections, identifies TLS connections, and parses out the relevant information. The certificate chain and other relevant meta-data are passed to the **Policy Engine** for evaluation. The Policy Engine runs a pluggable list of validation protocols asynchronously to validate or reject the certificate. The Policy Engine then instructs the Kernel Driver to either allow or sever the connection.

## Kernel Driver Structure

The Kernel Driver uses the 'Windows Filtering Platform' to identify TLS connections, extract certificates and metadata, and block/allow TLS connections.

### Driver Setup

The Driver is mainly initialized in 'Driver.c'. The `DriverEntry` function is called when the driver is loaded. The driver must register a device `WDFDEVICE`, which will be a instance of the driver. We do that in `THInitDriverAndDevice`. There we also create a symbolic link so our driver can be 'read' and 'written' to as a file by the Policy Engine. Also IO queues, Message queues, and Work Items are all set up here, which we will explain later.
 
### Callout Layers

The function `THRegisterCallouts` in 'Driver.c' is where we prepare the Windows Filtering Platform's engine. The Windows Filtering Platform allows us to insert into different network layers and inspect, modify, allow, or disallow the connection. The network pipeline is divided in layers and sublayers. We add filters to the layers we want to inspect. When the filters find a match, they trigger our callout. We are using the 'Classify', 'Notify', and 'Flow Delete' callout types. The registered callout functions are all defined in the 'TrustHubCallout.c' file.

A 'Notify' callout is called whenever there are changes to a callout, such as filter addition and filter deletion.

A 'Flow Delete' callout is called as a data flow is stopped, and provides a chance to clean up our context information.

A 'Classify' callout is where our callouts can inspect data, and allow or deny the data flow.

We have filters at the ALE (Application Layer Enforcement) Layer and the Stream Layer. Officially, the layer identifiers are `FWPM_LAYER_ALE_FLOW_ESTABLISHED_V` and `FWPM_LAYER_STREAM_V4`. The ALE layer provides us with information on the program that initialized the data stream, and the Stream layer lets us inspect the data, to parse TLS handshakes.

A data flow has an associated "Context" where we can store our own metadata and state with each specific flow. The structure of our context information can be seen in 'ConnectionContext.h'.

We allocate our context in the ALE classify callout, and free it after our stream flow delete. The context is shared between layers because it is associated with the data flow.

### Handshake Parsing

The file 'HandshakeHandler.c' holds our parsing code, which will search the stream data for a TLS handshake, and extract out the Client Hello, Server Hello, and certificate. It will copy each of these into an array of bytes into the flow's context.

The main controlling function is `updateState`, which is called by the stream callout whenever more data comes in. This function will return `RA_NEED_MORE` when a data packet it receives doesn't contain all of the data it needs. The `RA_NEED_MORE` will cause the stream callout to only be called again by the 'Windows Filtering Platform' when the buffer has been extended with at least the number of bytes needed for the next portion.

The handshake handling will return `RA_CONTINUE` if it should continue parsing the next packet, for example after the Client Hello is processed, but before the Server Hello has been received.

The handshake handler will return `RA_NOT_INTERESTED` when the stream is not relevant (i.e. not TLS), or we are finished with the stream.

The handshake handler will return `RA_WAIT` after grabbing the certificate. Then the driver will hand a query to the Policy Engine to validate the stream. The driver will receive a response back from Policy Engine. This is necessary before we can choose to allow or deny the stream.

### Message and Response Queues

There are two internal storage queues used, the `THResponses` queue, and the `THOutputQueue`. Empty objects are added to the `THResponses` queue in `sendCertificate` in 'TrustHubCallout.c'. These objects wait for an response from the Policy Engine. 

The `THOutputQueue` stores queries we want to send out to the policy engine the next time we handle a read event.

When a response comes back, it will be added into the queued object in the `THResponses`, and the stream callout will be able to lookup how to handle the stream.

### Communication

Kernel Driver <-> Policy Engine communication sends `IRP_MJ_WRITE` and `IRP_MJ_READ` requests to the driver's symbolic link. To the Policy Engine, it looks like a simple read and write. Each read/write event is tied to a WDF I/O Queue. Each WDF I/O Queue is enabled and disabled so our callbacks only handle the requests when we have content. 

The Read Queue needs to be enabled after we parse the certificate, but the stream callout runs at too high of a runtime level (the IRQL is dispatch level) to enable the queue. If we were to try to enable the I/O queue at this level it would result in deadlock. To fix this we use a Work Item which will run at a lower runtime level, running the function `THReadyRead`. Also in 'TrustHubCommunication.c' are functions `ThIoRead` and `ThIoWrite` which handle the read and write request associated with the I/O queues.

## Policy Engine Structure

The Policy Engine gets information from a configuration file, then loads the associated plugins and polls each of them when it recieves a query. Each query responds with a decision or times out.

### Initialization and Configuration

Our entry point is in the 'PolicyEngine.cpp' file. The `main` function first loads the plugin and configuration information from a configuration file using `context.loadConfig`.

In order to load the configuration, we use libconfig++. The official version of libconfig++ would not build, so we have a fixed version in our repo. the Policy Engine project uses it by having a "Reference" in visual studio, along with adding it's source directory as an include directory in:

> Policy Engine Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories

Loading the configuration will also create all of the plugins specified in the configuration file ('trustbase.cfg'). Then, the `main` function continues by initializing the query queue, starts each plugin loop as a thread, and starts the decider loop as a thread as well.

### Plugins

A plugin is a DLL that exports the specific functions query, initialize, and finalize, as defined in the file 'trustbase_plugin.h'. The Policy Engine, in the plugin function `init` uses LoadLibraryEx to load the DLL into memory, and uses GetProcAddress to find the symbols. Only the query function is required to be exported.

There are two main plugin types, synchronous and asynchronous. A synchronous plugin can just return one of the defined values in 'trustbase_plugin.h' like `PLUGIN_RESPONSE_...` .

An asynchronous plugin will ignore the return from the query function, and expects the asynchronous plugin to call the `async_callback`. The initialize function will give the plugin it's id, and the ` async_callback` function pointer.

### Communication and Query Queue

The Communications class receives the query from the Kernel Driver, parses it, and creates a 'Query' object that will be shared among all plugins.

The query is then enqueued in the 'QueryQueue'. This has a queue for each plugin, along with a list to save references to queries for asynchronous response.

### Decider

The decider is in 'PolicyEngine.cpp' in function `decider_loop`. Like the plugins, this is running is a separate looping thread. The decider loop waits for all plugins to respond, then decides on a response based on the plugin responses.

Plugins can be of category 'Needed', 'Congress', or 'Ignored'.  Ignored plugins have no weight on the final response. Needed plugins must respond valid for a final response of valid.

Congressed plugins form a fraction, and if the fraction of valid responses is not above a predefined threshold, then the response will be invalid.


## Development Notes

### Development Set-Up

All of Trustbase on Windows is contained in a single Visual Studio solution.

We  are using Visual Studio 2015.
The Windows SDK version 10.0.x.x must be installed.

#### Policy Engine

The Policy Engine can be tested on it's own if `COMMUNICATIONS_DEBUG_MODE` in communications.h is set to true, and `COMMUNICATIONS_DEBUG_QUERY` is the path to a binary file containing a query message formatted as a message from the Kernel Driver.

The Policy Engine also currently expects the "trustbase.cfg" file to be in the same folder as the executable. 

The Policy Engine also has a hardcoded path for the debug query in 'communications.h'.

A compiled version of Openssl 64 bit with applink enabled needs to be included. This has already been built and is located in the "Tools.zip". This should be unzipped and placed in the Trustbase root folder. This is located in our private Google Drive at:

  Internet - Security Joint Research - TrustBase - Windows

The tools.zip also include other libs used by various plugins.

The Policy Engine expects libcrypto-1_1-x64.dll to be located in 1 of 3 places: 
  1. 'C:\WINDOWS\SYSTEM32\libcrypto-1_1-x64.dll'
  2. In an folder in an environment path location
  3. In the same folder as the exe
This file is located in the tools zip at \tools\openssl-64\bin or \tools\openssl-64-debug\bin 

#### Plugins

Plugins must be compiled to a 'DLL'. 
Plugins must export the functions 'query,' 'initialize,' and 'finalize'. 

See 'SamplePlugin1' or 'SamplePlugin2' for examples.

To run the Native Plugins, each plugin must be built.

Some plugins require other files to work. Here are the details:

For Native Plugins:

  (for the whitelist plugin)
   
      This plugin requires "sqlite3.dll"
      The Policy Engine expects sqlite3.dll to be located in 1 of 3 places: 
        1. 'C:\WINDOWS\SYSTEM32\sqlite3.dll'
        2. In an folder in an environment path location
        3. In the same folder as the exe
      This file is located in the tools zip at \tools\sqlite3 or \tools\sqlite3 

      a folder called "whitelist" is required to be in the PolicyEngine project folder. Inside this folder include a whitelist file (any file with a .pem extension)

  (for the certificate pinning plugin)
      pinned_certs.db is required to be in the PolicyEngine project folder

  (for the cipher suite plugin)
      a folder called "plugin-config" required to be in the PolicyEngine project folder. Inside this folder include a cipher_suite.cfg.

For Python Plugins: use the folder "PythonPluginFiles". It should have the following files:

    (required for python)
    trusthub_python.py

    (for test plugin)
    test_plugin.py

    (for crlset revocation plugin)
    crlset_revocation.py
    crlset.dll

#### Python Addon

Python 2.7.x 64bit needs to be installed. We are using 2.7.13. Python should be installed at c:\Python27

Use pip to install the following libs:

  (Required for all plugins) pyOpenSSL 

  (Required for Notory Plugins) PySocks

  (Required for Notory Plugins) win_inet_pton

  (Required for Notory Plugins) twisted 

  (Required for Notory Plugins) service_identity

Python_addon project needs to be built so the python plugins can work.

#### Kernel Driver

The Kernel Driver development setup is the most complex. It involves two Windows machines, one as the _Host_, and one as the _Target_. The _Target_ will actually run the driver, and the _Host_ is used for development and controlling the debugger.

1. Machine Setup
  - Set up two separate machines with Windows 10
    - Machines must be on the same local network
    - Machines must have activated and updated versions of Windows
      `slmgr /skms kms.byu.edu`
      `slmgr /ato`
    - We haven't successfully used a Virtual Machine set up
2. Development Tools
  - On the _Host_ machine install Visual Studio Community edition
    <https://beta.visualstudio.com/vs/community/>
  - Install Windows Driver Kit 10 (WDK) latest version on _Host_ and _Target_
    <https://developer.microsoft.com/en-us/windows/hardware/windows-driver-kit>
  - Install Windows 10 SDK on _Host_ and _Target_
    <https://developer.microsoft.com/en-us/windows/downloads/windows-10-sdk>
3. Provision Target Machine
  - ​<https://msdn.microsoft.com/en-us/library/windows/hardware/dn745909(v=vs.85).aspx>
  - Verify Network Connection by pinging by hostname
4. Set up Deployment
  - Download the Repo
  - Verify the build platform is x64
  - TrustHubWin Properties->Driver Install->Deployment
    Check 'Do Not Install'
    Check 'Remove previous driver versions'
    Choose the correct Target Device Name
    (If you can't, you need to re-provision the target machine)
5. Build, Deploy, and Run
  - Build the 'TrustHubWin' Project
  - Deploy the 'TrustHubWin' Project
  - On the _Target_ Machine, there should now be a `C:\DriverTest\Drivers` Folder
  - Right Click on the .ini file, and select 'install'
  - If you want to use a a local debug viewer, start it as administrator
    Make sure kernel messeges is checked
    <https://technet.microsoft.com/en-us/sysinternals/debugview.aspx>
  - If you want to use the Kernel Debugger:
    - On the _Host_  have 'TrustHubWin' selected as the active Project
    - Start the Kernel Degubber
    - On the _Host_ you may have to pause and continue after starting TrustHubWin, inorder to apply breakpoints
  - On the _Target_ launch a administrator cmd console, and run `net start TrustHubWin`
  - Now the Kernel Driver is inserted and listening, start the Policy Engine or UserspaceTest as Admin

#### Deploying TrustBase on a computer without debug tools

I have compiled the June 22nd verison of Trustbase into a zip, save it on our private Google Drive at TrustBase\Windows. Below are the current instructions required to run TrustBase on a computer (without Visual Studio or any debug tools). 

1. Install Python 2.7 64 bit *In SNAPSHOT zip*
2. Use pip to install pyOpenSSL (some plugins might require other python lib)
    2.1. open command prompt and type: "cd c:\python27\scripts"
    2.2. type "pip install pyOpenSSL"
For Release version:
3. Install Visual Studio 2015 Redistributable (x64) *In SNAPSHOT zip*
For Debug version: 
4. Install Visual Studio 2015
5. Follow instructions for Driver Installation (below)
6. Follow instructions for Policy Engine Installaion (below)

## Driver Installation (Without debug tools)

  1. Build The TrustBaseWin Project. I believe the files from the build are sufficient. Although, if the TrustBaseWin project is "Deploy"-ed onto a target machine, then the build will also include a ceritificate.

  2. On the machine you want to the TrustBase Traffic Interceptor, you either have to provision the machine (long term effect, see `Kernel Driver development`) or you have turn on the machine with driver signing enforcement off. This will only last until the computer is restarted. The instructions the following:

      1. Go to Windows Settings -> Update and Security -> Recovery -> Advanced Startup. This will restart your machine.
      2. Troubleshoot -> Advanced options -> Startup Settings
      3. press f7 to disable driver signature enforcement

  3. Once the machine you want to install the TrustBase Traffic Interceptor is ready, right click on the "TrustBaseWin.inf" and click install. 

  4. To run the TrustBase Traffic Interceptor, open an `adminstrator command prompt window` and type "net start TrustBaseWin"

  5. To stop the TrustBase Traffic Interceptor, open an `adminstrator command prompt window` and type "net stop TrustBaseWin" Do note that, after it is stopped, you will need to restart the computer in order to start the Interceptor again (bug for now). You will then have to disabling driver signature enforcement again. 

## Policy Engine Installation (Without debug tools)
  1. After building the Policy Engine, Python Addon, and all the Plugins you may want to use, copy the entire bin folder (x64/debug) to a folder named whatever you want it to be named.
  In addition to the files from the bin, include the following:

    (required)
    trustbase.cfg - A copy of this file is located in the PolicyEngine project folder.

    (required)
    libcrypto-1_1-x64.dll - A copy of this file is located in the tools zip at \tools\openssl-64\bin or \tools\openssl-64-debug\bin (alternatively this .dll could be added to the PATH)

    (for the whitelist plugin)
    sqlite3.dll - This file is located in the tools zip at \tools\sqlite3 (alternatively this .dll could be added to the PATH)
    a folder called "whitelist". Inside this folder include a whitelist file (any file with a .pem extension)

    (for the certificate pinning plugin)
    pinned_certs.db - A copy of this file is located in the PolicyEngine project folder
 
    (for the cipher suite plugin)
    a folder called "plugin-config". Inside this folder include a cipher_suite.cfg. A copy of this file is located in the PolicyEngine project folder


  2. Go up one folder from the bin folder create above, create a folder called "PythonPluginFiles". This folder can be copied from the repo. It should have the following files:

    (required for python)
    trusthub_python.py

    (for test plugin)
    test_plugin.py

    (for crlset revocation plugin)
    crlset_revocation.py
    crlset.dll
      The crlset.dll is created from the crlset.go, crlset.c, and crlset.h. A compiled verison is in our private Google Drive at TrustBase\Windows
  3. Go up two folders from the bin (and PythonPluginFiles) folder, and create a file called tools. This should be contents of the tools zip located on our private Google Drive

  4. The PolicyEngine can be run, by opening a `adminstrator command prompt window`, and running "PolicyEngine.exe"
### Future Development

#### Development TODO

##### Kernel Driver

- [ ] Pass IP and Port to Policy Engine  
- [ ] Unload driver properly
- [ ] Remove connections from the queue to be validated if the flow is deleted

##### Policy Engine

- [x] Python Addon Integration                          
- [x?] Python Addons use multiple threads 
- [x] Proxy/Inserting Certificates into Root Store
- [] Certs from GoDaddy (4 chain length, maybe the problem) are not trusted by evaluate function in UnbrakeableCrypto. So any time we see one, the user sees "do you want to add this to the root store" (I still see this. Ask if it has been fixed!!)                                                                                           
#### Testing TODO    

##### Kernel Driver

- [ ] Test Large Client Hello
- [ ] Large number of synchronous connections (Chrome) -- (Are they saying they just want us to test this? Well we can test and then see if this is a problem or not)
- [ ] While Kernel Driver is active, and no policy engine is running, don't crash windows (I think it had to do with a buffer overflow).
- [ ] Be able to start, stop, then start "net start TrustHubWin" ---- Replace any instance of "TrustHubWin" with "TrustBaseWin""
##### Policy Engine		

- [ ] Valgrind for leaks