TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = ext daemon app
OTHER_FILES += \
    README.md \
    rpm/pebble.spec \
    rpm/pebble.yaml
