
Running the server : 

g++ -std=c++17 -DDEBUG -o server server.cpp marshaller.cpp account_store.cpp handlers.cpp -I.

Use the -DDEBUG flag for debug messages printed to the terminal, remove for no output.

./server <port number> amo|alo

amo or alo depending on the semantics