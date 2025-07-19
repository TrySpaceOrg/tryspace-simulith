# TrySpace Simulith
Distributed middleware for coordinating time and communication across processes.

## Overview
Simulith enables distributed simulations to synchronize time across all participating processes and route messages via simulated physical interfaces.
[Zeromq](https://zeromq.org/) is wrapped into a library specific for simulation platform support.

## Server
A centralized server is responsible for communicating to the various nodes who register.
This server has the ability to pause, resume, and run for a set period of time.
Note that Simulith time is separate from wall clock time enabling the processes it connects to to run faster or slower than real time.

## Interfaces
...
