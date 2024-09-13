# Implementation of Olaf-Neighbourhood Protocol for Tutorial 7

- Code server in C++
- Code client in C++

We need to come to some consensus on what parameters to use.

Use a makefile for compiling.

Feel free to modify this code to implement you're vulnerabilties. As far as I am aware we will need to code the frontend of our applications and agree on some methodology of communication.

```bash
sudo apt-get install libboost-all-dev && sudo apt-get install libssl-dev && sudo apt-get install zlib1g-dev

git clone https://github.com/zaphoyd/websocketpp

cd websocketpp

mkdir build

cd build

cmake ..

sudo make install

cd ..

cd ..

git clone https://github.com/nlohmann/json

cd json

mkdir build

cd build

cmake ..

sudo make install
```
# Running the client-server

To compile the client-server use the following commands

```bash

make client

./client

make server

./server

```

To establish connection between client and server, run in client ```connect ws://localhost:9002```