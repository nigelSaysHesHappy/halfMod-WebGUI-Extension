#g++ -std=c++11 -I ../../include/ -o nighttpsock.o -c nighttpsock.cpp -fPIC
#g++ -std=c++11 -I ../../include/ -o webgui_server.o -c webgui_server.cpp -fPIC
#g++ -std=c++11 -I ../../include/ -o ../../../halfMod/plugins/webgui.hmo nighttpsock.o webgui_server.o -shared webgui.cpp -lpthread -fPIC

/usr/bin/clang++-3.8 -std=c++11 -stdlib=libc++ -I ../../include -o ../../../halfMod/extensions/webgui.hmo ../../o/halfmod.o ../../o/str_tok.o sha1.o base64.o nighttpsock.cpp webgui_server.cpp -shared webgui.cpp -fPIC
#g++ -std=c++11 -I ../../include/ -o ../../../halfMod/plugins/webgui.hmo ../../o/halfmod.o ../../o/str_tok.o nighttpsock.cpp webgui_server.cpp -shared webgui.cpp -fPIC
