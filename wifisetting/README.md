# wlan0 up - Operation not possible due to RF-kill

## reason

   wifi is turn off

## solution

```shell
#list blocking device
rfkill list

#0: phy0: Wireless LAN
#	Soft blocked: no
#	Hard blocked: no

#unblock device
rfkill unblock 0
```
