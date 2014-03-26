# Buzztrax

## quick info
Please turn you browser to http://www.buzztrax.org to learn what this project
is about. Buzztrax is free software and distributed under the LGPL.

## build status
We are running continuous integration on drone.io

[![Build Status](https://drone.io/github.com/Buzztrax/buzztrax/status.png)](https://drone.io/github.com/Buzztrax/buzztrax/latest)

## intro
Buzztrax is a music composer similar to tracker applications. It is roughly
modelled after the windows only, closed source application called Buzz.

## requirements
* clutter
* clutter-gtk
* glib2
* gtk+3
* gstreamer
* gst-plugins-base
* gst-plugins-good
* gst-plugins-bad
* gst-buzztrax (from buzztrax git, use same version as this module)
* gsf
* libxml2

Regarding gstreamer - the newer the better.

## supported packages
* check (http://check.sf.net - for unit tests)
* gudev (for interaction controller)

## building from git
To build use autogen.sh instead of configure. This accept the same options like
configure. Later one can use autoregen.sh to rerun the bootstrapping.
To build from git one needs to have gtk-doc and cvs (for autopoint from gettext)
installed.

## directories
* design : little test sources, not needed for releases, not built during make
* docs : design ideas and API reference
* po : gettext i18n catalogs
* src : the project sources
  * ui
    * cmd : buzztrax tool for the command line
    * edit : buzztrax editor (gtk based ui)
  * lib
    * core : logic, e.g. connections framework, file i/o, ...
    * ic : interaction controller
* tests : unit tests (same directory structure as src)

## installing locally
Use following options for ./autogen.sh or ./configure

    --prefix=$HOME/buzztrax
    --with-gconf-source=xml::/home/ensonic/.gconf/
    --with-desktop-dir=/home/ensonic/.gnome2/vfolders/

when installing the package to e.g. $HOME/buzztrax remember to set these
environment variables:

    # libraries:
    export LD_LIBRARY_PATH=$HOME/buzztrax/lib:$LD_LIBRARY_PATH
    # online help: (as root)
    export OMF_DIR="$OMF_DIR:$HOME/buzztrax/share/omf"
    scrollkeeper-update
    # devhelp:
    export DEVHELP_SEARCH_PATH="$DEVHELP_SEARCH_PATH:$HOME/buzztrax/share/gtk-doc/html"
    # pkg-config:
    export PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$HOME/buzztrax/lib/pkgconfig"
    # gstreamer
    export GST_PLUGIN_PATH_1.0="$HOME/buzztrax/lib/gstreamer-1.0"
    # mime-database & icon-themes:
    export XDG_DATA_DIRS="$XDG_DATA_DIRS:$HOME/buzztrax/share"
    update-mime-database $HOME/buzztrax/share/mime/

## installing in /usr/local
The default value for the --prefix configure option is /usr/local. Also in that
case the variables mentioned in the last example need to be exported.

## running the apps

    cd $HOME/buzztrax/bin
    ./buzztrax-cmd --command=info --input-file=../share/buzztrax/songs/melo1.xml
    ./buzztrax-cmd --command=play --input-file=../share/buzztrax/songs/melo1.xml
    ./buzztrax-cmd --command=encode --input-file=../share/buzztrax/songs/melo1.xml --output-file=./melo1.ogg
    ./buzztrax-edit --command=load --input-file=../share/buzztrax/songs/melo1.xml

## running unit tests
run all the tests:

    make check

select tests to run:

    BT_CHECKS="test_bt_edit_app*" make bt_edit.check

