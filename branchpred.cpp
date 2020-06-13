#include <cstdio>
#include <cstring>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <cstdlib>
#include <map>
#include "pin.H"
#include "instlib.H"
using std::iostream;
using std::ostringstream;
using std::cerr;
using std::ofstream;
using std::ios;
using std::string;
using std::endl;

#define LIMIT 200

ofstream OutFile;

/* ===================================================================== */
/* Global Variables */
/* ===================================================================== */
class COUNTER
{
  public:
    UINT64 _branch;
    UINT64 _taken;

    COUNTER() : _branch(0), _taken(0)  {}

    UINT64 Total()
    {
        return _branch;
    }
};

std::map<ADDRINT, COUNTER> counterMap;

// This function is executed at each branch
static VOID AtBranch(ADDRINT ip, ADDRINT target, BOOL taken)
{
    counterMap[ip]._branch ++;
    if (taken)
	   counterMap[ip]._taken ++; 
}

// Pin calls this function every time a new instruction is encountered
VOID Instruction(INS ins, VOID *v)
{
    if (INS_IsBranch(ins) && INS_HasFallThrough(ins)) {
        INS_InsertCall(ins, IPOINT_BEFORE, (AFUNPTR)AtBranch, IARG_INST_PTR, IARG_BRANCH_TARGET_ADDR, IARG_BRANCH_TAKEN, IARG_END);
    }
}

KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool",
    "o", "branchpred.out", "specify output file name");

// This function is called when the application exits
VOID Fini(INT32 code, VOID *v)
{
    // Write to a file since cout and cerr maybe closed by the application
    OutFile.setf(ios::showbase);
    // Output results
    for (std::map<ADDRINT, COUNTER>::iterator it=counterMap.begin(); it!=counterMap.end(); ++it) {
	if (it->second._branch != LIMIT) continue;
        OutFile << it->first << " => branch count: " << it->second._branch << " => taken count: " << it->second._taken << '\n';
    }
    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool predicts the outcome of conditional branches executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();

    OutFile.open(KnobOutputFile.Value().c_str());

    cerr <<  "===============================================" << endl;
    cerr <<  "This application is instrumented by branchpred" << endl;
    if (!KnobOutputFile.Value().empty())
    {
        cerr << "See file " << KnobOutputFile.Value() << " for analysis results" << endl;
    }
    cerr <<  "===============================================" << endl;

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);
    
    // Start the program, never returns
    PIN_StartProgram();
    
    return 0;
}
