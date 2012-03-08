Logic Analyser on XTAG2
.......................

:Stable release:  unreleased

:Status:  first idea

:Maintainer:  https://github.com/henkmuller

:Description:  A logic analyser that runs on XTAG2


Key Features
============

* None

To Do
=====

* Implement sampling software
* Copy USB buffering structure from proj_xtag2
* Develop host application to load software into an XTAG2 usign USB bootloader.

Firmware Overview
=================

The application in this repository runs a program on an XTAG2 to
make it act like a logic analyser sampling at rates up to 50 MHz
on two wires, or 16 MHz on 7 wires. The USB output format is
compatible with sigrok.

Known Issues
============

* None

Required Repositories
================

* xcommon git\@github.com:xcore/xcommon.git

Support
=======

Raise an issue.
