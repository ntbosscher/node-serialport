cmd_Release/bindings.node := c++ -bundle -framework CoreFoundation -framework IOKit -undefined dynamic_lookup -Wl,-no_pie -Wl,-search_paths_first -mmacosx-version-min=10.9 -arch x86_64 -L./Release -stdlib=libc++  -o Release/bindings.node Release/obj.target/bindings/src/serialport.o Release/obj.target/bindings/src/serialport_unix.o Release/obj.target/bindings/src/poller.o Release/obj.target/bindings/src/darwin_list.o 