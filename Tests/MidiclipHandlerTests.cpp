//
//  MidiclipHandlerTests.cpp
//  cybr - ConsoleApp
//
//  Created by Zhi Wei Gan on 3/9/21.
//
#ifdef RUN_TESTS
#include "Tests.h"
#include "../Source/AppJobs.h"

void MidiclipHandlerTest::runTest() {
    AppJobs testAppJobs;
    cybr::OSCMessage createEdit("/file/activate");
    createEdit.addString("test.tracktionedit");
    testAppJobs.fluidOscServer.activateEditFile(createEdit);
    
    cybr::OSCMessage createTrack("/audiotrack/select");
    createTrack.addString("testTrack");
    testAppJobs.fluidOscServer.selectAudioTrack(createTrack);
    
    //---------------------------------------------------------------------------------
    beginTest("create");
    cybr::OSCMessage createMidiClip("/midiclip/select");
    createMidiClip.addString("testMidiclip");
    cybr::OSCMessage reply = testAppJobs.fluidOscServer.selectMidiClip(createMidiClip);
    cybr::OSCMessage expected("/midiclip/select/reply");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
    
    //---------------------------------------------------------------------------------
    beginTest("select");
    reply = testAppJobs.fluidOscServer.selectMidiClip(createMidiClip);
    expected = cybr::OSCMessage("/midiclip/select/reply");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
    
    //---------------------------------------------------------------------------------
    beginTest("insert/note");
    cybr::OSCMessage createNote("/midiclip/insert/note");
    createNote.addInt32(60);
    createNote.addFloat32(1.0);
    createNote.addFloat32(1.0);
    createNote.addInt32(64);
    createNote.addInt32(2);
    reply = testAppJobs.fluidOscServer.insertMidiNote(createNote);
    expected = cybr::OSCMessage("/midiclip/insert/note/reply");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
    
    //---------------------------------------------------------------------------------
    beginTest("insert/note/beats");
    cybr::OSCMessage createNoteBeats("/midiclip/insert/note/beats");
    createNoteBeats.addInt32(60);
    createNoteBeats.addFloat32(1.0);
    createNoteBeats.addFloat32(1.0);
    createNoteBeats.addInt32(64);
    createNoteBeats.addInt32(2);
    reply = testAppJobs.fluidOscServer.insertMidiNoteBeats(createNoteBeats);
    expected = cybr::OSCMessage("/midiclip/insert/note/beats/reply");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
    
    //---------------------------------------------------------------------------------
    beginTest("clear");
    cybr::OSCMessage clear("/midiclip/clear");
    reply = testAppJobs.fluidOscServer.clearMidiClip(clear);
    expected = cybr::OSCMessage("/midiclip/clear");
    expected.addInt32(0);
    expect(FluidOscServerTest::equalMessage(reply, expected));
    
}

#endif
