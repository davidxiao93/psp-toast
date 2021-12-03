PSP Toast

I got annoyed that I was getting no on-screen indication when the volume or the brightness was being changed.
This plugin fixes this by making a little toast when either volume or brightness changes.

Features
- On screen notification when volume changes
- On screen notification when brightness changes
- On screen notification when wifi is enabled and disabled
- On screen notification when hold is enabled and disabled
- Screen brightness levels has increased range
    - The default brightness levels are weird on the PSP-3000 (44, 60, 72 and 84). I've changed it to 20, 40, 60, 80 and 100
- Disabled long press Screen button behaviour
- Overrode Note button behaviour to take a screenshot
    - Screenshots are saved into ms0:/PICTURE/
- Disabled long press Note button behaviour

Caveats:
- I've only tested this on my PSP-3004. I have not tested this on any other PSP model.
- I've only tested this on 6.61 Pro-C Infinity. I have not tested this on other firmwares.
- I've only tested this in VSH (vsh.txt) and in PSP games (game.txt). I have not tested this in a PS1 game (pops.txt)

Why is it called Toast
- See https://developer.android.com/guide/topics/ui/notifiers/toasts for more info about toasts :P

Installation
- Download from the release folder toast.prx or build from source (instructions below)
- Install as you would for any other plugin

Building from source
- I've only attempted this on Windows. YMMV on non-Windows OSes.
- Install MinimalistPSPSDK from https://darksectordds.github.io/html/MinimalistPSPSDK/index.html
- Add pspsdk/bin to your PATH
- Run "make" in this directory


Credits
- This plugin is heavily based off Brightness-Control as it gave me the code to change brightness
	https://github.com/PSP-Archive/Brightness-Control
- Thus plugin is heavily based off PSP-HUD to get the method of displaying text on screen
	https://github.com/ErikPshat/PSP-HUD


