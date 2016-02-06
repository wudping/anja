Anja med J -- A sample player for Jack
======================================

<img src="https://raw.githubusercontent.com/milasudril/anja/master/doc/work-in-progress-a.png" alt="A screenshot illustrating the UI" width="600">

<img src="https://raw.githubusercontent.com/milasudril/anja/master/doc/work-in-progress-b.png" alt="A screenshot illustrating the UI" width="600">

<img src="https://raw.githubusercontent.com/milasudril/anja/master/doc/work-in-progress-c.png" alt="A screenshot illustrating the UI" width="600">

This project aims at building a live performance sample player/recorder for
Jack. In contrast to Hydrogen, this will feature a virtual keyboard display as
its main UI. Pre-recorded wave files can be associated to different keyboard
keys, and the virtual keyboard display will show labels for mapped keys, making
it easier to find the right sample during a live session. There will also be
a recording mode, so it is possible to record and playback.

Dependencies
------------
* libgtk-3-dev
* g++
* libjack-jackd2-dev
* libsndfile-dev
* https://github.com/milasudril/gabi/archive/5.77.tar.gz

Build instructions (ubuntu)
---------------------------
* $ sudo apt-get install libgtk-3-dev g++ libjack-jackd2-dev libsndfile-dev
* Download and configure gabi
* $ wand

