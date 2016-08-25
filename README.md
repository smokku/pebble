pebbled
=======
[![Gitter](https://badges.gitter.im/Join Chat.svg)](https://gitter.im/smokku/pebble?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
[![Flattr this](https://button.flattr.com/flattr-badge-large.png)](https://flattr.com/submit/auto?fid=8n22qr&url=https%3A%2F%2Fgithub.com%2Fsmokku%2Fpebble)

Unofficial Pebble watch support for SailfishOS/Jolla


More information about Pebble:
http://getpebble.com



Features
--------
* Voice Calls notification and control
* Conversations notifications forwarding
* Missed calls notifications
* New email notifications
* Twitter and Mitakuuluu notifications
* MPRIS compatible media player support
* Set "silent" profile when watch is connected
* Phone to Watch time synchronization
* Transliterate strings to plain ASCII
* daemon management app
* "org.pebbled" DBus interface
* PebbleKit JS application partial support
 (including Pebble object, XMLHTTPRequest, localStorage, geolocation)
* Pebble AppStore preliminary support
* Firmware check and upgrade



Building
--------

You need SailfishOS development environment: https://sailfishos.org/develop.html

Open the project and build RPM.



Running
-------

Deploy as a RPM package to the device.

You need to pair your Pebble in Settings -> System settings -> Bluetooth

Application will try to connect to any paired device which name starts with "Pebble"



References
----------

* http://pebbledev.org/wiki/Protocol
* https://github.com/Hexxeh/libpebble/
* http://developer.getpebble.com/2/guides/javascript-guide.html
* http://specifications.freedesktop.org/mpris-spec/latest/
