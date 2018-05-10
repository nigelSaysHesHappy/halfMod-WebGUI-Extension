# halfMod-WebGUI-Extension
Adds a Web Server with Control Panel to halfMod

# Installation
Copy the repo into your halfMod installation then run the following commands from the terminal:
```sh
cd src/extensions/webgui
chmod +x build.sh
./build.sh
cd ../../plugins/
./compile.sh --install webgui_console.cpp
```

# Web Server
The web server files are hosted in `./halfMod/plugins/webgui/www/`. This is the default index directory of the web server.  
The web server has support for most protocols.

# BTML
BTML is a file format that aims to replace PHP, any file with this extension is treated as a bash script. It is executed and whatever the script outputs is what is served to the requesting client.

# Special thanks
Special thanks to [MulverineX](https://github.com/MulverineX) for developing the Panel frontend.
