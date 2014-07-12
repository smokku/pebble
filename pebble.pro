TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = ext daemon app
OTHER_FILES += \
    rpm/pebble.spec \
    rpm/pebble.yaml
