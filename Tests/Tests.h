#ifdef RUN_TESTS
#include "../JuceLibraryCode/JuceHeader.h"
#include "../Source/FluidOscServer.h"
#include <assert.h>

class FluidOscServerTest :
public FluidOscServer,
public juce::UnitTest
{
public:
    FluidOscServerTest() : UnitTest("FluidOscServer tests") {}
    void runTest() override;
    static bool equalMessage (cybr::OSCMessage a, cybr::OSCMessage b);
    
};

class MidiclipHandlerTest :
public FluidOscServer,
public juce::UnitTest
{
public:
    MidiclipHandlerTest() : UnitTest ("midiclip handler") {}
    void runTest() override;
};

class AudiotrackHandlerTest :
public FluidOscServer,
public juce::UnitTest
{
public:
    AudiotrackHandlerTest() : UnitTest ("audiotrack handler") {}
    void runTest() override;
};

#endif
