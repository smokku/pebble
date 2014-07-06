TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = daemon app
OTHER_FILES += \
    rpm/pebble.spec \
    rpm/pebble.yaml
