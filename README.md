#  NSH SF Reflector

This code acts as a simple service function.  It accepts traffic from a service function forwarder, much like nsh-traffic-injector, decapsulates it, decrements the service index and returns it.  It uses GRE+NSH encapsulation.

### Test Topology

### Requirements
The following packages are required to build:

    gcc
    make
    libpthread-stubs0-dev
    
    apt-get -y update
    apt-get -y --fix-missing install make gcc libpthread-stubs0-dev


### Build
To build the code run make from the top directory:

    make

### Run
To run the code, please see the usage information. For example:

    root@4cbcdbb40941:~/code# ./nsh-sf-reflector -h
    Usage: ./nsh-sf-reflector

    Options:
    -d -----------------> Turn debug on
    -h -----------------> This help message

    root@4cbcdbb40941:~/code#

Traffic is returned with service index decremented from the service function forwarder it was recieved from.
