# Pceas

This is the software project associated with a M.Sc.(Tech.) thesis submitted to the University of Turku.

We implement two SMC protocols: 
P_CEPS (Circuit Evaluation with Passive Security) 
and 
P_CEAS (Circuit Evaluation with Active Security). 
For P_CEAS, we also implement circuit randomization, which is a common technique for efficiency improvement. 

## Dependencies

The program is implemented with C++ and uses C++11 features. 

### FLINT: Fast Library for Number Theory

We used FLINT Library (version 2.5.2) for number theoretic operations. In particular, we used 

	fmpz and fmpz_vec classes for operations involving integers
	fmpz_mod_poly class for operations involving polynomials over Z/pZ

We also rely on FLINT Library for random number generation. FLINT internally uses a linear congruential generator. When players want to generate random numbers as they run the protocol, they use their partyID and the randomness they get from std::random_device as seeds. Hence, players pick different randoms in consecutive runs. (See NO_RANDOM in Pceas.h to change this behaviour.)

We implemented symmetric bivariate polynomials over Z/pZ (class SymmBivariatePoly) on top of FLINT's fmpz_mod_poly class.

### Boost C++ Libraries

We used tokenizer.hpp and algorithm/string.hpp classes from Boost Library, for parsing the options file.

## Building

We built and run the project on a machine running 64-bit Ubuntu (16.04). Linux GCC toolchain (5.4.0 20160609 Ubuntu 5.4.0-6ubuntu1~16.04.4) was used to build the project. Compiler option -std=c++0x (ISO C++ 11 Language Standard), and linker flags -lmpfr -lgmp -pthread are required. One needs FLINT and its dependencies installed on the system. Refer to FLINT's documentation for these installations.

## Installing

## Running

An options file template and sample options files (including those used for the runs mentioned in the thesis text) can be found along with the source code, under options folder. In order to run the simulator, make sure that the options folder (containing the options file 'opt') is in the same directory with the binary : 

~/.../Release\$ ls

options Pceas

~/.../Release\$ ./Pceas

## Known Problems
There is a bug that effects a specific case. When all conditions below are satisfied : 
1. protocol = CEAS_with_circuit_randomization
2. TEST CASES enabled and one or more parties defined as corrupt
3. circuit has MULT gate which has input that is output of a MULT gate [for ex.  @x1*x1*x2  -  NOTE : @x1*x1+x2*x2 IS OK ]

a wrong result is computed.
