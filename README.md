# Pr3ProQ

PreProQ (**Pre-Pro**cessor for **Q**CIRs) was first introduced by [Peyrer et al](https://resolver.obvsg.at/urn:nbn:at:at-ubl:1-72739) in their corresponding work. Pr3ProQ is a re-implementation of PreProQ removing many restrictions of the original. This includes but is not limited to

* Lighter internal datastructure to support bigger circuits
* Optimized parser
* Avoiding multiple iteration of gates/variables by applying the techniques simultaneously
* New optional technique: DAG-ification (eliminate duplicate gates by exploiting the DAG structure of QCIR)

This project is available and commonly updated at [https://github.com/cynonymia/pr3proq](https://github.com/cynonymia/pr3proq). Stable versions and major releases can in addition be found at [www.doi.org/10.5281/zenodo.20303639](www.doi.org/10.5281/zenodo.20303639)


## Setup

To build pr3proq, execute the following commands:

```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release .. 
make -j
```

Or, alternatively, for the debug version granting access to full verbose output:
```
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug .. 
make -j
```
