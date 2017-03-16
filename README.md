# Castalia

Castalia is a simulator for Wireless Sensor Networks (WSN), Body Area Networks (BAN) and generally networks of low-power embedded devices. It is based on the OMNeT++ platform and can be used by researchers and developers who want to test their distributed algorithms and/or protocols in realistic wireless channel and radio models, with a realistic node behaviour especially relating to access of the radio. Castalia can also be used to evaluate different platform characteristics for specific applications, since it is highly parametric, and can simulate a wide range of platforms. The main features of Castalia are:

* Advanced **channel model** based on empirically measured data
   * Model defines a map of path loss, not simply connections between nodes
   * Complex model for temporal variation of path loss
   * Fully supports mobility of the nodes
   * Interference is handled as received signal strength, not as separate feature

* Advanced **radio model** based on real radios for low-power communication
   * Probability of reception based on SINR, packet size, modulation type. PSK FSK supported, custom modulation allowed by    defining SNR-BER curve
   * Multiple TX power levels with individual node variations allowed
   * States with different power consumption and delays switching between them
   * Flexible carrier sensing (polling-based and interrupt-based)

* Extended **sensing** modelling provisions
   * Highly flexible physical process model
   * Sensing device noise, bias, and power consumption
   * Node clock drift, CPU power consumption.

* MAC and routing protocols available
* Designed for adaptation and expansion.

Concerning the last bullet, Castalia was designed right from the beginning so that the users can easily implement/import their algorithms and protocols into Castalia while making use of the features the simulator is providing. Proper modularization and a configurable, automated build procedure help towards this end. The modularity, reliability, and speed of Castalia is partly enabled by OMNeT++, an excellent framework to build event-driven simulators [OMNeT++ link].


***

### What Castalia is not

Castalia is not sensor platform-specific. Castalia is meant to provide a generic reliable and realistic framework for the first order validation of an algorithm before moving to implementation on a specific sensor platform. Castalia is not useful if one would like to test code compiled for a specific sensor node platform. For such usage there are other simulators/emulators available (e.g., Avrora).

## Forum
It is important to study the manual carefully. It is structured in a way to gradually familiarise you with the basic concepts. You will be running simulations along the way so you can have a hands-on experience. For any question you can visit the forum at https://groups.google.com/forum/#!forum/castalia-simulator

Search for existing questions before posting your own.
