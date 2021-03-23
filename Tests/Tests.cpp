//
//  Tests.cpp
//  cybr - ConsoleApp
//
//  Created by Zhi Wei Gan on 3/9/21.
//
#ifdef RUN_TESTS
#include <stdio.h>
#include "Tests.h"

bool FluidOscServerTest::equalMessage (cybr::OSCMessage a, cybr::OSCMessage b) {
    bool address = a.getAddressPattern() == b.getAddressPattern();
    bool numargs = a.size() == b.size();
    bool arguments = true;
    for (int i = 0; i < a.size() && i < b.size(); i++) {
        if (a[i].getType() != b[i].getType()) arguments = false;
        if (a[i].isBlob() && a[i].getBlob() != b[i].getBlob()) arguments = false;
        if (a[i].isInt32() && a[i].getInt32() != b[i].getInt32()) arguments = false;
        if (a[i].isColour() && a[i].getColour().toInt32() != b[i].getColour().toInt32()) arguments = false;
        if (a[i].isString() && a[i].getString() != b[i].getString()) arguments = false;
        if (a[i].isFloat32() && a[i].getFloat32() != b[i].getFloat32()) arguments = false;
        if (a[i].isFloat64() && a[i].getFloat64() != b[i].getFloat64()) arguments = false;
    }
    return address && numargs && arguments;
}


void FluidOscServerTest::runTest() {
    MidiclipHandlerTest midiclipHandlerTest;
    AudiotrackHandlerTest audiotrackHandlerTest;
    
    UnitTest* test[] = {&midiclipHandlerTest, &midiclipHandlerTest};
    
    juce::UnitTestRunner runner;
    
    runner.runTests(juce::Array<UnitTest*>(test, 2));
}

#endif
