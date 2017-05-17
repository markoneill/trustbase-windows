# Trusthub for Windows

[TOC]


## Structure Overview

Trusthub for Windows is build of two main subgroups, the **Kernel Driver** and the **Policy Engine**. The **Kernel Driver** monitors connections, looking for a TLS connection, and parses out the relevant information, including the certificate, passing it to the **Policy Engine** for approval. It will then either allow or sever the connection, based on the response.

## Kernel Driver Structure

The Kernel Driver uses the 'Windows Filtering Platform' inorder to get all of the relevant information.

### Driver Setup

### Callout Layers

### Handshake Parsing

### Message and Response Queues

### Communication

## Policy Engine Structure

The Policy Engine gets information from a configuration file, then loads the associated plugins and polls each of them when it recieves a query, responding with a decision.

### Initialization and Configuration

### Plugins

### Communication and Query Queue

### Decider


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
  1. On the _Host_  have 'TrustHubWin' selected as the active Project
  2. Start the Kernel Degubber
  4. On the _Host_ you may have to pause and continue after starting TrustHubWin, inorder to apply breakpoints
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