# TODO

## 1. Features to be implemented

1. IPv6 support
2. Implement --peer option
3. Implement other interfaces' operations

## 2. Known bugs

1. Random packets seem to be lost under congestion
2. lwip.d is not created automatically
3. If a translator register is set, this error raises when booting the system:
    file_name_lookup /dev/eth1: Computer bought the farm
4. First active translator set after booting crashes
5. Calling setsockopt() chrases the server
6. Sometimes the translator fails to start

## 3. Others

1. Add comments
2. Indent to GNU coding standards
3. Found and fix memory leaks
4. Check for IP address passed to bind() to be actually assigned to an interface
5. Tune TCP

## 4. Extra features to be implemented (not in the proposal)

1. Build LwIP as a library
2. PPPoS support
3. PPPoE support

