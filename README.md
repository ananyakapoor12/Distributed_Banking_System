
Running the server : 

g++ -std=c++17 -DDEBUG -I$(brew --prefix sqlitecpp)/include -o server server.cpp marshaller.cpp account_store.cpp handlers.cpp -L$(brew --prefix sqlitecpp)/lib -lSQLiteCpp -lsqlite3 -lpthread -ldl

Use the -DDEBUG flag for debug messages printed to the terminal, remove for no output.

./server <port number> amo|alo

amo or alo depending on the semantics