# Trusthub for Windows

[TOC]


## Structure Overview

Trusthub for Windows is built of two main subgroups, the **Kernel Driver** and the **Policy Engine**. The **Kernel Driver** monitors connections, identifies TLS connections, and parses out the relevant information. The certificate chain and other relevant meta-data are passed to the **Policy Engine** for evaluation. The Policy Engine runs a pluggable list of validation protocols asynchronously to validate or reject the certificate. The Policy Engine then instructs the Kernel Driver to either allow or sever the connection.

## Kernel Driver Structure

The Kernel Driver uses the 'Windows Filtering Platform' in order to get all of the relevant information.

### Driver Setup

The Driver is initialized mainly in 'Driver.c'. The `DriverEntry` function is called when the driver is inserted. The driver must register a device, which will be a instance of the driver. We do that in `THInitDriverAndDevice`. There we also create a symbolic link so our driver can be 'read' and 'written' to as a file by the Policy Engine. Also IO queues, Message queues, and Work Items are all set up here, which we will explain later.

### Callout Layers

The function `THRegisterCallouts` in 'Driver.c' is where we prepare the Windows Filtering Platform's engine. The Windows Filtering Platform allows us to insert into different network layers and inspect, modify, allow, or disallow the connection. Each layer callouts are added on a sublayer, the callouts are added, and the filters are added. Each layer has a 'Classify', 'Notify', and 'Flow Delete' callout. The registered callback functions are all in the 'TrustHubCallout.c' file.

A 'Notify' callout is called for events that have to do with the callout, such as filter addition and filter deletion.

A 'Flow Delete' callout is called when a when a data flow is stopped, and provides a chance to clean up.

A 'Classify' callout is where our callouts can inspect data, and allow or deny the data flow.

We have filters at the ALE (Application Layer Enforcement) Layer and the Stream Layer. Officially, the layer identifiers are `FWPM_LAYER_ALE_FLOW_ESTABLISHED_V` and `FWPM_LAYER_STREAM_V4`. The ALE layer provides us with information on the program that initialized the data stream, and the Stream layer lets us inspect the data, to parse TLS handshakes.

A data flow can have an associated "Context" used to associate our own metadata and state with each specific flow. The structure of our context information can be seen in 'ConnectionContext.h'.

We allocate our context in the ALE classify callout, and free it after our stream flow delete. The context is shared between layers, associated with the data flow.

### Handshake Parsing

The file 'HandshakeHandler.c' holds our parsing code, which will search the stream data for a TLS handshake, and extract out the Client Hello, Server Hello, and certificate. It will copy each of these into an array of bytes into the flow's context.

The main controlling function is `updateState`, which is called by the stream callout whenever more data comes in. This function will return `RA_NEED_MORE` when a data packet it receives doesn't contain all of the data it needs, and the . `RA_NEED_MORE` will cause the stream callout to only be called again when the buffer has been extended with at least the number of bytes needed for the next portion.

The handshake handling will return `RA_CONTINUE` if it just wants to continue to the next packet, for example after the Client Hello is processed.

The handshake handler will return `RA_NOT_INTERESTED` when the stream is not relevant, or we are finished with the stream.

The handshake handler will return `RA_WAIT` after grabbing the certificate, to let the driver know it needs to hand a query to the Policy Engine to validate the stream before we can choose to allow or deny the stream.

### Message and Response Queues

There are two internal storage queues used, the `THResponses` queue, and the `THOutputQueue`. Responses are added to their queue in `send Certificate` in 'TrustHubCallout.c', waiting for a response from the Policy Engine. When a response comes back, it will be added into the queued item, and the stream callout will be able to lookup how to handle the stream.

The output queue will contain the query we want to send out to the policy engine when we handle a read event.

### Communication

Communication works by handling `IRP_MJ_WRITE` and `IRP_MJ_READ` requests to the driver's symbolic link. To the Policy Engine, it looks like a simple read and write. Each event is tied to a WDF I/O queue. The Queues are enabled and disabled so our callbacks only handle the requests when we have content to deal with. 

The Read Queue needs to be enabled after we parse the certificate, but the stream callout runs at too high of a runtime level (the IRQL is dispatch level) to enable the queue. If we were to try to enable the I/O queue at this level it would result in deadlock. To fix this we use a Work Item which will run at a lower runtime level, running the function `THReadyRead`. Also in 'TrustHubCommunication.c' are functions `ThIoRead` and `ThIoWrite` which handle the read and write request associated with the I/O queues.

## Policy Engine Structure

The Policy Engine gets information from a configuration file, then loads the associated plugins and polls each of them when it recieves a query, responding with a decision.

### Initialization and Configuration

Our entry point is in the 'PolicyEngine.cpp' file. The first thing the `main` function does is load the plugin and configuration information from a configuration file using `context.loadConfig`.

In order to load the configuration, we use libconfig++. The official version of libconfig++ would not build, so we have a fixed version in our repo. the Policy Engine project uses it by having a "Reference" in visual studio,  along with adding it's source directory as an include directory in:

> Policy Engine Properties -> Configuration Properties -> C/C++ -> General -> Additional Include Directories

Loading the configuration will also create all of the plugins specified in the configuration file ('trusthub.cfg'). Then, the `main` function continues by initializing the query queue, starts each plugin loop as a thread, and starts the decider loop as a thread as well.

### Plugins

A plugin is a DLL that exports the specific functions query, initialize, and finalize, as defined in the file 'trusthub_plugin.h'. The Policy Engine, in the plugin function `init` uses LoadLibraryEx to load the DLL into memory, and uses GetProcAddress to find the symbols. Only query is required to be exported.

There are two main plugin types, synchronous and asynchronous. A synchronous plugin can just return one of the defined values in 'trusthub_plugin.h' like `PLUGIN_RESPONSE_...` .

An asynchronous plugin will ignore the return from the query function, and expects the asynchronous plugin to call the `async_callback`. The initialize function will give the plugin it's id, and the ` async_callback` function pointer.

### Communication and Query Queue

The communications receives the query from the Kernel Driver, and parses it, creating a 'Query' object that will be shared among all plugins.

The query is then enqueued in the 'QueryQueue'. This is actually had a queue for each plugin, along with a list to save references to queries for asynchronous response.

### Decider

The decider is in 'PolicyEngine.cpp' in function `decider_loop`. Like the plugins, this is running is a separate looping thread. The decider loop waits for all plugins to respond, then decides on a response based on the plugin responses.

Plugins can be of category 'Needed', 'Congress', or 'Ignored'.  Ignored plugins have no weight on the final response. Needed plugins must respond valid for a final response of valid.

Congressed plugins form a fraction, and if the fraction of valid responses is not above a predefined threshold, then the response will be invalid.


## Development Notes

### Development Set-Up

All of Trusthub on Windows is contained in a single Visual Studio solution.

#### Policy Engine

The Policy Engine can be tested on it's own if `COMMUNICATIONS_DEBUG_MODE` in communications.h is set to true, and `COMMUNICATIONS_DEBUG_QUERY` is the path to a binary file containing a query message formatted as a message from the Kernel Driver.

#### Plugins

Plugins should be built as a 'DLL' that exports a the functions 'query,' 'initialize,' and 'finalize'. See 'SamplePlugin1' or 'SamplePlugin2' for examples.

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

### Future Development

#### Development TODO

##### Kernel Driver

- [ ] Add shim to Outbound IP layer, parse destination IP and port
- [ ] Unload driver properly
- [ ] Remove connections from the queue to be validated if the flow is deleted

##### Policy Engine

- [ ] Python Addon Integration
- [ ] Proxy/Inserting Certificates into Root Store

#### Testing TODO

##### Kernel Driver

- [ ] Test Large Client Hello
- [ ] Large number of synchronous connections (Chrome)


##### Policy Engine

- [ ] Valgrind for leaks