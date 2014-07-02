pebbled
=======

Unofficial Pebble watch support for SailfishOS/Jolla


More information about Pebble:
http://getpebble.com



Building
--------

You need SailfishOS development environment: https://sailfishos.org/develop.html

Open the project and build RPM.



Running
-------

Deploy as a RPM package to the device.

You need to pair your Pebble in Settings -> System settings -> Bluetooth
Application will try to connect to any paired device which name starts with "Pebble"


Development
-----------

For now this application is still under developement. By default you will be shown
development UI options, which you can utilize to test the connection.

Incoming voice calls are shown in Pebble. You can test this by calling to your device.
Pressing hangup button in Pebble should end the call.

Incoming messages are also sent to Pebble. Send SMS or instant message to your device to test it.


!!! NO WARRANTY OF ANY KIND !!!



References
----------

* http://pebbledev.org/wiki/Protocol
* https://github.com/Hexxeh/libpebble/
