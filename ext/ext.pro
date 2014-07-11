# Such a project structure allowed for integrating Log4Qt via git subtree
# Log4Qt folder is pulled right from the remote repository
TEMPLATE = subdirs
SUBDIRS = Log4Qt

lib.files += $$OUT_PWD/../../ext/Log4Qt/*.s*
lib.path = /usr/share/$$TARGET/lib

INSTALLS += lib
