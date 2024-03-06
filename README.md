# Effective Communication assignment

This repository contains code for the effective communication assignment. Please find further details below.

An UWP application written in C# to interface the microcontroller can be found [here](https://github.com/Captainfl4me/effective-communication_uwp).

## Goal & features

This C++ code is written with MBed 6 and aim to target a STM Nucleo F411RE. The program should be able to communicate both ways using JSON messages through a Serial bus with a computer (see [list of commands](#serial-commands)).

### JSON

To be able to serialize and deserialize JSON message and handle them properly in cpp I wrote a custom parser. It is composed of two parts, first a Lexer creates an intermediate representation of the input string by tokenising it. Then a parser creates a nested structure with custom class as values from the list of tokens.

The Lexer can handle string input break into multiple pieces. For that the Lexer output the last token generated successfully allowing the user to merge the new chunk with the remaining chars left and loop over until all chars have been tokenised. In this scenario, tokens must be stored out of the loop scope so that each time the lexer is run last and new tokens can be concatenated.

### Logger

To have a fluent flow of output, a Logger class is also provided. This class act as a singleton and so can be call from everywhere. The principle is really straight forward, the user can add a new log of different level of importance to a stack. And a dedicated thread loop to empty the stack, so it always displays messages in order and without any stream race. Messages are also formated before being outputted in the output stream so that they consistent and easily readable.

The logger can be initialized with different level of details that will impact which output will be actually print and which will be discarded without modifying the source code directly. For example, flags can be used to compile RELEASE and DEBUG version of the code with different level of logging without any modification of your code between the two binaries.

Flag|level
--|--
DEBUG|0
INFO|1
WARNING|2
ERROR|3
RELEASE|4

> The RELEASE flag is the higher level of logs allowing you to output directly to the output stream without any formatting. This is the filter level when using the RELEASE build flag of this repos.

## SERIAL commands

### Inputs

There is multiple serial commands available describe here:

JSON|Description
--|--
`{"mode":0,"on":true}`|Mode 0 is a on/off mode for the built-in LED and expects a boolean "on" key to set the led state.
`{"mode":1,"v":0.5}`| Mode 1 is a PWM mode for the built-in led allowing you to control the brightness of it by ranging from 0.0 to 1.0. This mode expects a float "v" key.
`{"mode":2,"d":1.0}`| Mode 2 is a blink mode for the built-in led allowing you to control the blinking frequency by settings the delay between on-off state in seconds. This mode expects a float "d" key. The delay is rounded with a milliseconds precision.
`{"req":0}`| Request current status to microcontroller. Expected response should be with this format `{"status":{"mode":0,"led":1}}`. Where "led" is the current led power and mode is the last mode updated.

### Reponse

After an input, the microcontroller should respond back. If the input expects a response then it will be provide else it would be an empty object. The response could also contain a "err" field with the error message if something wrong happens.

