# MPTCP

MultiPath-TCP


## Project distribution:
<https://docs.google.com/document/d/1-85Sw4KSLfA_8vCNIyLAU57E78Kjvl-Kvprio2yNVKg/pub>

## Overview
> TCP is widely used in the Internet today. Most all traffic nowadays is sent over some variant of the TCP protocol. Connections are limited by the bandwidth of the link, but the fact that many different paths may exist between 2 endpoints could be leveraged to increase this bandwidth. Essentially, if a host has more than one interface associated with it, and the client and server are both mptcp capable, they could distribute the load across multiple flows. For instance, if you want to watch a video on YouTube from your phone, MPTCP could make it possible to connect to WiFi and 4G/LTE data simultaneously to reduce buffering time. In this assignment, you will be sending a file to a remote server across multiple subflow paths.

> For this project you will be implementing MPTCP in C. To implement this in the Linux kernel would be very complicated ( 10,000+ lines ) so you will implement the client end to communicate with a server we have set up through a provided library of socket API wrapper functions. The purpose of this assignment is to explore how data ( reliably ) travels across the network and must adhere to its restrictions.

## Installation
Makefile is included, simply run `make`.

`make clean` for uninstallation.

## Usage:
```shell
./mptcp [ -n num_interfaces ] [ -h hostname ] [ -p port ] [ -f filename ]

    [ -n ] : number of connections ( paths ) you would like to havewith the server

    [ -h ] : hostname of the server where to connect

    [ -p ] : port number on the server where to start the initial connection

    [ -f ] : name of the file to transfer to the server
```
