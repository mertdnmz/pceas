# Pceas

This is the software project associated with a M.Sc.(Tech.) thesis submitted to the University of Turku.

We implement two SMC protocols: 
$P_{CEPS}$ (Circuit Evaluation with Passive Security) 
and 
$P_{CEAS}$ (Circuit Evaluation with Active Security). 
For $P_{CEAS}$, we also implement circuit randomization, which is a common technique for efficiency improvement. 

## Dependencies

The program is implemented with C++ and uses C++11 features. 

### FLINT: Fast Library for Number Theory

We used FLINT Library\parencite{ref112} (version 2.5.2) for number theoretic operations. In particular, we used 
\begin{itemize}
	\item \code{fmpz} and \code{fmpz\_vec} classes for operations involving integers
	\item \code{fmpz\_mod\_poly} class for operations involving polynomials over $\mathbb{Z}/p\mathbb{Z}$
\end{itemize}

We also rely on FLINT Library for random number generation. FLINT internally uses a linear congruential generator. When players want to generate random numbers as they run the protocol, they use their \code{partyID} and the randomness they get from \code{std::random\_device} as seeds. Hence, players pick different randoms in consecutive runs\footnote{It might be useful to change this behaviour for debugging. See \code{NO\_RANDOM} in \code{Pceas.h}.}.

We implemented symmetric bivariate polynomials over $\mathbb{Z}/p\mathbb{Z}$ (class \code{SymmBivariatePoly}) on top of FLINT's \code{fmpz\_mod\_poly} class.

### Boost C++ Libraries

We used \code{/tokenizer.hpp} and \code{/algorithm/string.hpp} classes from Boost Library\parencite{ref113}, for parsing the options file.

## Building

We built and run the project on a machine running 64-bit Ubuntu (16.04). Linux GCC toolchain (5.4.0 20160609 Ubuntu 5.4.0-6ubuntu1~16.04.4) was used to build the project. Compiler option \code{-std=c++0x} (ISO C++ 11 Language Standard), and linker flags \code{-lmpfr -lgmp -pthread} are required. One needs FLINT \parencite{ref112} and its dependencies installed on the system. Refer to FLINT's documentation for these installations.

## Installing

## Running

An options file template and sample options files (including those used for the runs mentioned in Chapter \ref{ChapterImplement} and Chapter \ref{ChapterExtendAndRun}) can be found along with the source code, under \code{options} folder. In order to run the simulator, make sure that the \code{options} folder (containing the options file \code{opt}) is in the same directory with the binary : \\
\code{
~/.../Release\$ ls\\
options Pceas\\
~/.../Release\$ ./Pceas
}

## Known Problems
There is a bug that effects a specific case. When all conditions below are satisfied : 
1. protocol = CEAS_with_circuit_randomization
2. TEST CASES enabled and one or more parties defined as corrupt
3. circuit has mult gate which has input that is output of a mult gate [for ex.  @x1*x1*x2  -  NOTE : @x1*x1+x2*x2 IS OK ]

a wrong result is computed.
