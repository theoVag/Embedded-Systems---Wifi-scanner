# Embedded-System---Wifi-scanner
Application for scanning nearby wifi ssids
Program written in c.It scans for nearby wifi ssids (uses a script only for zsun) and saves in file each ssid followed by the current timestamp.
The program takes time between two scans as a parameter. Program uses two threads one for scanning and one for saving in files.
It is based on producer -consumer logic. Program tries to reduce cpu usage (by sleeping all threads and the main thread).
It was tested under openWRT install in Zsun Wifi Card Reader Memory Extender Wireless.
It terminates safely after pressing ctrl+c.
Makefile and script must be changed in order to run in different operating systems
