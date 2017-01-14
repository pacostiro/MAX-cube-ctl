# MAX-cube-ctl

This is an attempt to implement the protocol of the eQ3 / ELV MAX! Heating system and use it into a simple management Linux application.

src/maxproto folder contains the implementation of the protocol and all the utility functions.

src/maxctl contains the management application that communicates with MAX! cube.

MAX.conf is a configuration file that can be used to configure the devices.

Features:

    - Discover MAX!Cube in LAN using Multicast/Broadcast

    - Retreive information about devices and configuration (Cube and Radio thermostat supported for now)
    
    - Set weekly program by using a configuration file (MAX.conf or custom in the same location as the executable).
    
    - Set Eco/Comfort temperatures.
    
    - Set working mode for devices (Auto/Eco/Comfort).
    
    - Configuration settings possible per one device or all devices (Only configuration updates are sent for minimal radio activity).

    - Logging periodically valve position, temperature set and actual. 

This protocol partial descriptions are available on the internet.

https://github.com/Bouni/max-cube-protocol
