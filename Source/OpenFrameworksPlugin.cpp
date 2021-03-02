/*
  ==============================================================================

    OpenFrameworksPlugin.cpp
    Created: 29 May 2019 7:49:52pm
    Author:  Charles

  ==============================================================================
*/

#include "OpenFrameworksPlugin.h"

using namespace juce;

OpenFrameworksPlugin::OpenFrameworksPlugin(te::PluginCreationInfo info) : te::Plugin(info)
{
    semitones = addParam("semitones up", TRANS("Semitones"),
        { -getMaximumSemitones(), getMaximumSemitones() },
        // valueToString
        [](float value) { return std::abs(value) < 0.01f ? "(" + TRANS("Original pitch") + ")"
                                                            : te::getSemitonesAsString(value); },
        // stringToValue
        [](const String& s) { return jlimit(-OpenFrameworksPlugin::getMaximumSemitones(),
                                            OpenFrameworksPlugin::getMaximumSemitones(),
                                            s.getFloatValue()); });


    semitonesValue.referTo(state, te::IDs::semitonesUp, getUndoManager());
    semitones->attachToCurrentValue(semitonesValue);
}

OpenFrameworksPlugin::~OpenFrameworksPlugin()
{
    notifyListenersOfDeletion();

    semitones->detachFromCurrentValue();
}

ValueTree OpenFrameworksPlugin::create()
{
    ValueTree v (te::IDs::PLUGIN);
    v.setProperty (te::IDs::type, xmlTypeName, nullptr);
    return v;
}

const char* OpenFrameworksPlugin::xmlTypeName = "openframeworks";

// Called from a "mixer" thread. (There can be multiple "mixer" threads)
void OpenFrameworksPlugin::applyToBuffer(const te::PluginRenderContext& fc)
{
    if (fc.bufferForMidiMessages != nullptr) {
        fc.bufferForMidiMessages->addToNoteNumbers(roundToInt(semitones->getCurrentValue()));
        for (auto& msg : *(fc.bufferForMidiMessages)) {
            std::cout
                << "Got midi message: "
                << fc.editTime
                << " juce::Time seconds: " << Time::getMillisecondCounterHiRes() * 0.001
                << msg.getTimeStamp() // This is not really meaningful. It's the time stamp within the block, which I believe is arbitrary. We should really figure out how to playhead->getEditTime (or whatever it is) this value
                << " - "
                << msg.getDescription()
                << std::endl;
        }
    }
}

String OpenFrameworksPlugin::getSelectableDescription()
{
    return TRANS("MIDI Modifier Plugin");
}


void OpenFrameworksPlugin::restorePluginStateFromValueTree(const juce::ValueTree& v)
{
    CachedValue<float>* cvsFloat[] = { &semitonesValue, nullptr };
    te::copyPropertiesToNullTerminatedCachedValues(v, cvsFloat);
}

