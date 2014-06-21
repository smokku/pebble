TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = lib daemon app
OTHER_FILES += \
    rpm/waterwatch.spec \
    rpm/waterwatch.yaml
