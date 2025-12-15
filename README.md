# FoxChat - Terminal-Only Chatting Client & Server

## ***[Download <- Latest Windows Version (client)](windows version)***

## ***[Download <- Latest Linux Version (client)](windows version)***

## _There are no account sign ups!_

FoxChat has no account system, instead it uses IP’s and a random session id for each client.

**USE AN VPN, IF YOU DON’T YOU ARE PUTTING YOUR SECURITY AT RISK AS YOUR IP IS BEING EXPOSED!**

Ex:

10.54.10.2_92012 > {client ip}_{session id}

_! The session id for your or others clients won’t be the same every time !_

## _There is no chat history!_

FoxChat does not officially support chat history, all the history is stored on the client side in the ram for ease of use and to understand where the conversation was when you joined. This does not mean someone can’t just stay logged in and log messages! Don’t do anything illegal… this was made as a little project and because I found no appealing terminal-only chat software.

## _Servers are customizable and lightweight!_

Servers are customizable in the means that you can color and format, messages, join alerts, and errors. You can also set message cool downs (on by default) and character limits on the messages.

## ***Disclaimer:***

By downloading, using, or referencing FoxChat, you acknowledge that the creator is **not responsible or liable** for any illegal activity, misuse, or damages resulting from the use of this software. Use this software at your own risk.

## One more time, always use a VPN to stay safe.

Definition of VPN (from [wikiedia.org](https://en.wikipedia.org/wiki/Virtual_private_network)): A **virtual private network** (**VPN**) is an [overlay network](https://en.wikipedia.org/wiki/Overlay_network) that uses [network virtualization](https://en.wikipedia.org/wiki/Network_virtualization) to extend a [private network](https://en.wikipedia.org/wiki/Private_network) across a public network, such as the [Internet](https://en.wikipedia.org/wiki/Internet), via the use of [encryption](https://en.wikipedia.org/wiki/Encryption) and [tunneling protocols](https://en.wikipedia.org/wiki/Tunneling_protocol).[[1]](https://en.wikipedia.org/wiki/Virtual_private_network#cite_note-NIST-1) In a VPN, a tunneling protocol is used to transfer network messages from one [network host](https://en.wikipedia.org/wiki/Host_(network)) to another.

# Server Setup

To start the server, you got two options.

### 1. Build it yourself (Linux, Windows) RECOMMENDED

### ***Linux:***

1. Open a terminal window.
2. CD to a directory where you want the server.
3. Run command

```null
git clone https://github.com/Jacob-Meyers/FoxChat-FOSS;cd FoxChat-FOSS
```
4. Install ***clang ***(lookup how for your distro)
5. Run command

```null
clang++ server.cpp -o server -Iinclude
```

### ***Windows:***

1. Download and install ***[MSYS2](https://www.msys2.org/)***
2. Follow instructions on how to use MSYS2 installer in the same link below the download
3. Start ***'MSYS2 MINGW64'*** from the start menu.
4. In the console run the command (Shift+insert to paste btw)

```null
pacman -S mingw-w64-ucrt-x86_64-gcc
```
5. Locate the installation of the compiler executable (e.g., 'C:\msys64\ucrt64\bin').
6. Search for **'Environment Variables'** in the start menu and select **"Edit the system environment variables"**.
7. Click the **'Environment Variables'** button.
8. Under 'System variables', select the 'path' variable and click **"Edit"**.
9. Click **'New'** and add the path to the 'bin' directory from your MSYS2 installation (e.g., 'C:\msys64\ucrt64\bin').
10. Click **'OK'** on all windows to save the changes.
11. Now, go to the folder you want to host the server at, right click and select **'Open in Terminal'** and run the command

```null
git clone https://github.com/Jacob-Meyers/FoxChat-FOSS;cd FoxChat-FOSS;g++ server.cpp -o server.exe -Iinclude -lws2_32git
```
12. If all went right and you got no errors, start the server by starting ***'server.exe'*** of course!

### 2. Download latest release (Windows)

1. Download and extract the latest windows precompiled version from the releases page.
2. Start server.exe

# Server Customization

To customize the server open the ***'serverconfig.json'*** file and you can tweak these values anyway you like.

### Server Variables API

As you might see there is little brackets like these {} in the strings of some of the values. These are replaced with values from the server.

Available Variables:

- ***{clientip_id}*** > the combined ip and session id of the client (ex. 10.54.10.2_92012), used for private/targeted messages like _'message_prefix_self'_ or _'welcome_message'_
- ***{clientip_raw}*** > just the ip of the client (ex. 10.54.10.2_92012), also made for private/targeted messages
- ***{custom_port}*** > the optional custom port set also in the server config. (not recommended to change port as a custom client would only be able to join on ports other than 9020).

      --_Color Variables--_

- ***{tColor_reset}*** > resets the text color
- ***{tColor_red}*** > makes the text color red
- ***{tColor_yellow}*** > makes the text color red
- ***{tColor_green}*** > makes the text color red
- ***{tColor_blue}*** > makes the text color red
- ***{tColor_purple}*** > makes the text color red
- ***{tColor_gray}*** > makes the text color red
