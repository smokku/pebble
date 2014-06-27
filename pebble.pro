TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = lib daemon app
OTHER_FILES += \
    rpm/pebble.spec \
    rpm/pebble.yaml
