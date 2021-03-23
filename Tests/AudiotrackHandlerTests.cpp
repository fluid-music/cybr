//
//  MidiclipHandlerTests.cpp
//  cybr - ConsoleApp
//
//  Created by Zhi Wei Gan on 3/9/21.
//
#ifdef RUN_TESTS

#include "Tests.h"
#include "../Source/AppJobs.h"

void AudiotrackHandlerTest::runTest() {
    AppJobs testAppJobs;
    cybr::OSCMessage createEdit("/file/activate");
    createEdit.addString("test.tracktionedit");
    testAppJobs.fluidOscServer.activateEditFile(createEdit);
    
    //---------------------------------------------------------------------------------
    beginTest("create");
    cybr::OSCMessage createTrack("/audiotrack/select");
    createTrack.addString("testTrack");
    cybr::OSCMessage reply = testAppJobs.fluidOscServer.selectAudioTrack(createTrack);
    cybr::OSCMessage expected("/audiotrack/select/reply");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
    
    //---------------------------------------------------------------------------------
    beginTest("select");
    reply = testAppJobs.fluidOscServer.selectAudioTrack(createTrack);
    expected = cybr::OSCMessage("/audiotrack/select/reply");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
}

#endif
