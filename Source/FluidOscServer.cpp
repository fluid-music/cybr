/*
  ==============================================================================

    FluidOscServer.cpp
    Created: 18 Nov 2019 5:50:15pm
    Author:  Charles Holbrow

  ==============================================================================
*/

#include "FluidOscServer.h"
#include "plugin_report.h"

using namespace juce;

// If the first argument is a float or double return true, and place the
// retrieved value in result. Otherwise, return false.
inline bool getFloatOrDouble(const cybr::OSCArgument& arg, double& result) {
    if (arg.isFloat32()) { result = (double)arg.getFloat32(); return true; }
    else if (arg.isFloat64()) { result = arg.getFloat64(); return true; }
    else return false;
}

//==============================================================================
FluidOscServer::FluidOscServer() {
    addListener (this);
}

void FluidOscServer::oscBundleReceived(const cybr::OSCBundle &bundle){
    SelectedObjects obj;
    handleOscBundle(bundle, obj);
}

void FluidOscServer::oscMessageReceived(const cybr::OSCMessage &message){
    handleOscMessage(message);
}

void FluidOscServer::constructReply(cybr::OSCMessage &reply, int error, String message){
    reply.addInt32(error);
    reply.addString(message);
    std::cout<<message<<std::endl;
}

void FluidOscServer::constructReply(cybr::OSCMessage &reply, String message){
    reply.addString(message);
    std::cout<<message<<std::endl;
}

SelectedObjects FluidOscServer::getSelectedObjects() {
    return { selectedTrack, selectedClip, selectedPlugin };
}

cybr::OSCBundle FluidOscServer::handleOscBundle(const cybr::OSCBundle &bundle, SelectedObjects parentSelection) {
    SelectedObjects currBundle = parentSelection;
    cybr::OSCBundle reply;
    for (const auto& element: bundle) {
        if (element.isMessage()) {
            // allow messages to update the current selection ("currBundle")
            cybr::OSCMessage replyMessage = handleOscMessage(element.getMessage());
            currBundle.audioTrack = selectedTrack;
            currBundle.clip = selectedClip;
            currBundle.plugin = selectedPlugin;
            reply.addElement(replyMessage);
        } else if (element.isBundle()) {
            // After processing a bundle, selection will reset to "currBundle"
            cybr::OSCBundle replyBundle = handleOscBundle(element.getBundle(), currBundle);
            reply.addElement(replyBundle);
        }
    }

    selectedTrack = parentSelection.audioTrack;
    selectedClip = parentSelection.clip;
    selectedPlugin = parentSelection.plugin;
    return reply;
}

cybr::OSCMessage FluidOscServer::handleOscMessage (const cybr::OSCMessage& message) {
    const cybr::OSCAddressPattern msgAddressPattern = message.getAddressPattern();

    if (msgAddressPattern.matches({"/test"}) || msgAddressPattern.matches({"/print"})) {
        printOscMessage(message);
        return message;
    }
    if (msgAddressPattern.matches({"/file/activate"})) return activateEditFile(message);
    if (msgAddressPattern.matches({"/audiofile/report"})) return getAudioFileReport(message);

    if (!activeCybrEdit) {
        File file = File::getCurrentWorkingDirectory().getChildFile("empty.tracktionedit");
        activateEditFile(file, true);
    }

    if (msgAddressPattern.matches({"/midiclip/insert/note/"})) return insertMidiNote(message);
    if (msgAddressPattern.matches({"/midiclip/insert/note/beats"})) return insertMidiNoteBeats(message);
    if (msgAddressPattern.matches({"/midiclip/select"})) return selectMidiClip(message);
    if (msgAddressPattern.matches({"/midiclip/clear"})) return clearMidiClip(message);
    if (msgAddressPattern.matches({"/plugin/select"})) return selectPlugin(message);
    if (msgAddressPattern.matches({"/plugin/vst2/select"})) return selectVst2PluginById(message);
    if (msgAddressPattern.matches({"/plugin/param/set"})) return setPluginParam(message);
    if (msgAddressPattern.matches({"/plugin/param/set/at"})) return setPluginParamAt(message);
    if (msgAddressPattern.matches({"/plugin/sidechain/input/set" })) return setPluginSideChainInput(message);
    if (msgAddressPattern.matches({"/plugin/save"})) return savePluginPreset(message);
    if (msgAddressPattern.matches({"/plugin/load/trkpreset"})) return loadPluginTrkpreset(message);
    if (msgAddressPattern.matches({"/plugin/load/vst2preset"})) return loadVst2PresetInSelectedPlugin(message);
    if (msgAddressPattern.matches({"/plugin/load"})) return loadPluginPreset(message);
    if (msgAddressPattern.matches({"/plugin/report"})) return getPluginReport(message);
    if (msgAddressPattern.matches({"/plugin/param/report"})) return getPluginParameterReport(message);
    if (msgAddressPattern.matches({"/plugin/params/report"})) return getPluginParametersReport(message);
    if (msgAddressPattern.toString().startsWith("/plugin/sampler")) return handleSamplerMessage(message);
    if (msgAddressPattern.matches({"/audiotrack/select"})) return selectAudioTrack(message);
    if (msgAddressPattern.matches({"/audiotrack/select/return"})) return selectReturnTrack(message);
    if (msgAddressPattern.matches({"/audiotrack/select/submix"})) return selectSubmixTrack(message);
    if (msgAddressPattern.matches({"/audiotrack/set/db"})) return setTrackGain(message);
    if (msgAddressPattern.matches({"/audiotrack/set/pan"})) return setTrackPan(message);
    if (msgAddressPattern.matches({"/audiotrack/set/width"})) return setTrackWidth(message);
    if (msgAddressPattern.matches({"/audiotrack/send/set/db"})) return ensureSend(message);
    if (msgAddressPattern.matches({"/audiotrack/remove/clips"})) return removeAudioTrackClips(message);
    if (msgAddressPattern.matches({"/audiotrack/remove/automation"})) return removeAudioTrackAutomation(message);
    if (msgAddressPattern.matches({"/audiotrack/insert/wav"})) return insertWaveSample(message);
    if (msgAddressPattern.matches({"/audiotrack/mute"})) return muteTrack(true);
    if (msgAddressPattern.matches({"/audiotrack/unmute"})) return muteTrack(false);
    if (msgAddressPattern.matches({"/audiotrack/region/render"})) return renderRegion(message);
    if (msgAddressPattern.matches({"/file/save"})) return saveActiveEdit(message);
    if (msgAddressPattern.matches({"/cd"})) return changeWorkingDirectory(message);
    if (msgAddressPattern.toString().startsWith({"/transport"})) return handleTransportMessage(message);
    if (msgAddressPattern.matches({"/clip/render"})) return renderClip(message);
    if (msgAddressPattern.matches({"/clip/length/set/seconds"})) return setClipLengthSeconds(message);
    if (msgAddressPattern.matches({"/clip/length/set/beats"})) return setClipLengthBeats(message);
    if (msgAddressPattern.matches({"/clip/select"})) return selectClip(message);
    if (msgAddressPattern.matches({"/clip/trim/seconds"})) return trimClipBySeconds(message);
    if (msgAddressPattern.matches({"/clip/source/offset/seconds"})) return offsetClipSourceInSeconds(message);
    if (msgAddressPattern.matches({"/audioclip/set/db"})) return setClipDb(message);
    if (msgAddressPattern.matches({"/audioclip/set/pitch"})) return setClipPitch(message);
    if (msgAddressPattern.matches({"/audioclip/set/stretch-mode"})) return setClipStretchMode(message);
    if (msgAddressPattern.matches({"/audioclip/set/speed-ratio"})) return setClipSpeedRatio(message);
    if (msgAddressPattern.matches({"/audioclip/reverse"})) return reverseAudioClip(true);
    if (msgAddressPattern.matches({"/audioclip/unreverse"})) return reverseAudioClip(false);
    if (msgAddressPattern.matches({"/audioclip/fade/seconds"})) return audioClipFadeInOutSeconds(message);
    if (msgAddressPattern.matches({"/tempo/set/"})) return setTempo(message);
    if (msgAddressPattern.matches({"/content/clear"})) return clearContent(message);
    if (msgAddressPattern.matches({"/tempo/set/timesig"})) return setTimeSignatureAt(message);

    printOscMessage(message);
    cybr::OSCMessage error("/error");
    constructReply(error, 1, "Unhandled Message: " + message.getAddressPattern().toString());
    return error;
}

cybr::OSCMessage FluidOscServer::setTimeSignatureAt(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/tempo/set/timesig/reply");

    if (!activeCybrEdit) {
        constructReply(reply, 1, "Cannot set time signature: No active edit");
        return reply;
    }

    if (message.size() < 2 || !message[0].isInt32() || !message[1].isInt32()) {
        constructReply(reply, 1, "Cannot set time signature: Invalid Arguments");
        return reply;
    }

    int upper = message[0].getInt32();
    int lower = message[1].getInt32();
    double timeSeconds = 0;

    te::Edit& edit = activeCybrEdit->getEdit();

    // The behavior of message[2] is overloaded. If it is a float or a double,
    // treat it as a value in seconds. If int, treat it as a measure number.
    if (message.size() > 2) {
        if (getFloatOrDouble(message[2], timeSeconds)) {}
        else if (message[2].isInt32()) {
            int measureNumber = jmax(1, message[2].getInt32()); // index from 1
            timeSeconds = edit.tempoSequence.barsBeatsToTime({measureNumber - 1, 0.0 });
        }
    }

    te::TimeSigSetting::Ptr timeSignature = edit.tempoSequence.insertTimeSig(timeSeconds);
    timeSignature->numerator = upper;
    timeSignature->denominator = lower;
    if (upper % 3 == 0) timeSignature->triplets = true;
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::selectSubmixTrack(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/select/submix/reply");
    if (!activeCybrEdit) {
        constructReply(reply, 1, "Cannot select submix track: No active edit");
        return reply;
    }

    if (!message.size() || !message[0].isString()) {
        constructReply(reply, 1, "Cannot select submix track: Invalid submix name");
        return reply;
    }

    String submixName = message[0].getString();
    String parentName = String();

    if (message.size() >= 2) {
        if (!message[1].isString()) {
            constructReply(reply, 1, "Cannot select submix track: Invalid parent submix name");
            return reply;
        }
        parentName = message[1].getString();
    }

    selectedTrack = getOrCreateSubmixByName(activeCybrEdit->getEdit(), submixName, parentName);
    constructReply(reply, 0, "Submix selected: " + selectedTrack->getName());
    return reply;
}

cybr::OSCMessage FluidOscServer::clearContent(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/content/clear/reply");
    if (!activeCybrEdit) {
        constructReply(reply, 1, "Cannot clear content: No active edit");
        return reply;
    }

    for (auto track : te::getClipTracks(activeCybrEdit->getEdit())) {
        removeAllClipsFromTrack(*track);
        removeAllPluginAutomationFromTrack(*track);
    }

    for (auto track : te::getTracksOfType<te::FolderTrack>(activeCybrEdit->getEdit(), true)) {
        removeAllPluginAutomationFromTrack(*track);
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::removeAudioTrackClips(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/remove/clips/reply");
    if (!selectedTrack) {
        const String errorString = "Cannot remove audio track clips: no track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (auto* clipTrack = dynamic_cast<te::ClipTrack*>(selectedTrack)) {
        removeAllClipsFromTrack(*clipTrack);
    } else {
        const String result = "Not removing clips, because selected track is not a ClipTrack";
        constructReply(reply, 0, result);
        return reply;
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::removeAudioTrackAutomation(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/remove/automation/reply");

    if (!selectedTrack) {
        String errorString = "Cannot remove audio track automation: no track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }
    removeAllPluginAutomationFromTrack(*selectedTrack);

    reply.addInt32(0);
    return reply;
}


cybr::OSCMessage FluidOscServer::setClipDb(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audioclip/set/db/reply");

    if (!selectedClip) {
        String errorString = "Cannot set audio clip gain: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto* audioClip = dynamic_cast<te::AudioClipBase*>(selectedClip);
    if (!audioClip) {
        String errorString = "Cannot set audio clip gain: selected clip is not an audio clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isFloat32()) {
        String errorString = "Cannot set audio clip gain: missing db float argument";
        constructReply(reply, 1, errorString);
        return reply;
    }

    double dBFS = message[0].getFloat32();
    if (dBFS > 40) {
        String errorString = "Cannot set audio clip gain: " + String(dBFS) + "db is dangerously loud";
        constructReply(reply, 1, errorString);
        return reply;
    }

    audioClip->setGainDB(dBFS);
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setClipPitch(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audioclip/set/pitch/reply");

    if (!selectedClip) {
        String errorString = "Cannot set audio clip pitch: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto* audioClip = dynamic_cast<te::AudioClipBase*>(selectedClip);
    if (!audioClip) {
        String errorString = "Cannot set audio clip pitch: selected clip is not an audio clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isFloat32()) {
        String errorString = "Cannot set audio clip pitch: missing semitones float argument";
        constructReply(reply, 1, errorString);
        return reply;
    }

    float semitones = message[0].getFloat32();
    if (semitones != 0) {
        audioClip->setTimeStretchMode(te::TimeStretcher::Mode::soundtouchBetter);
    }
    audioClip->setPitchChange(semitones);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setClipSpeedRatio(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audioclip/set/speed-ratio/reply");

    if (!selectedClip) {
        String errorString = "Cannot set audio clip speed ratio: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto* audioClip = dynamic_cast<te::AudioClipBase*>(selectedClip);
    if (!audioClip) {
        String errorString = "Cannot set audio clip speed ratio: selected clip is not an audio clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    float ratio = message[0].getFloat32();
    audioClip->setSpeedRatio((double)ratio);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setClipStretchMode(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audioclip/set/stretch-mode/reply");

    if (!selectedClip) {
        String errorString = "Cannot set audio clip stretch mode: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto* audioClip = dynamic_cast<te::AudioClipBase*>(selectedClip);
    if (!audioClip) {
        String errorString = "Cannot set audio clip stretch mode: selected clip is not an audio clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    // check mode is valid
    int mode = message[0].getInt32();

    if (mode > 9 || mode < 0){
        String errorString = "Cannot set audio clip stretch mode: selected mode is out of bounds";
        constructReply(reply, 1, errorString);
        return reply;
    }

    audioClip->setTimeStretchMode((te::TimeStretcher::Mode)mode);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::audioClipFadeInOutSeconds(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audioclip/fade/seconds/reply");
    if (!selectedClip) {
        String errorString = "Cannot setup audio clip fade in/out: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto* audioClip = dynamic_cast<te::WaveAudioClip*>(selectedClip);
    if (!audioClip) {
        String errorString = "Cannot setup audio clip fade in/out: selected clip is not an audio clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    // All args optional
    // 0 - fade In  TIME
    // 1 - fade Out TIME

    if (message.size() >= 1 && message[0].isFloat32()) {
        double fadeInTime = message[0].getFloat32();
        audioClip->setFadeIn(fadeInTime);
        if (message.size() >= 3 && message[2].isFloat32()) {
            // TODO: Set type?
        }
    }

    if (message.size() >= 2 && message[1].isFloat32()) {
        double fadeOutTime = message[1].getFloat32();
        audioClip->setFadeOut(fadeOutTime);
        if (message.size() >= 4 && message[3].isFloat32()) {
            // TODO Set fade type?
        }
    }
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::reverseAudioClip(bool reverse) {
    cybr::OSCMessage reply("/audioclip/reverse/reply");
    if (!selectedClip) {
        String errorString = "Cannot update clip reverse status: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto* audioClip = dynamic_cast<te::WaveAudioClip*>(selectedClip);
    if (!audioClip) {
        String errorString = "Cannot update clip reverse status: selected clip is not an audio clip";
        constructReply(reply, 1, errorString);
        return reply;
    }
    if (reverse == audioClip->getIsReversed()){
        reply.addInt32(0);
        return reply;
    }

    // All the durations below are measured in seconds (after the speedRatio has
    // been applied).
    //
    // The algorithm for reversing is the same, weather we are reversing, or
    // "unreversing." Annoyingly, the clip->getSourceLength method returns 0
    // if the clip is already reversed, so the if(reverse) block is used to
    // ensure that getSourceLength is only called when the clip is in an
    // unreversed state.
    //
    // The algorithm works when the clip's speedRatio is adjusted. However, I
    // have not tested it with more complicated time streching, such as when
    // clip effects are used to automate playback speed. Preliminary tests
    // suggest that in Tracktion Waveform, clip effects are deactivated when the
    // clip is reversed.

    te::ClipPosition pos = audioClip->getPosition();
    double speedRatio = audioClip->getSpeedRatio();
    double length = pos.getLength();
    double startInSource = pos.getOffset(); // after stretch

    if (reverse) {
        double sourceLength = audioClip->getSourceLength() / speedRatio;
        // If the length of the clip exceeds the source length, reversing will
        // result in an incorrect "window" into the file, because tracktion allows
        // audio clips to exceed the source length, but does not allow negative
        // offsets.
        double maxValidLength = sourceLength - startInSource;
        jassert(maxValidLength > 0);
        if (length > maxValidLength && maxValidLength > 0) {
            std::cout << "pre-reverse trim: " << maxValidLength - length << " " << audioClip->getOriginalFile().getFileName() << std::endl;
            audioClip->setLength(maxValidLength, false);
        }

        double tailSize = sourceLength - (startInSource + length);
        audioClip->setIsReversed(reverse);
        audioClip->setOffset(tailSize);
    } else {
        audioClip->setIsReversed(reverse);
        double sourceLength = audioClip->getSourceLength() / speedRatio;
        double tailSize = sourceLength - (startInSource + length);
        audioClip->setOffset(tailSize);
    }

    // Also switch fade in/out
    double fadeInTime = audioClip->getFadeIn();
    auto fadeInType = audioClip->getFadeInType();
    audioClip->setFadeIn(audioClip->getFadeOut());
    audioClip->setFadeInType(audioClip->getFadeOutType());
    audioClip->setFadeOut(fadeInTime);
    audioClip->setFadeOutType(fadeInType);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::offsetClipSourceInSeconds(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/clip/source/offset/seconds/reply");
    if (!selectedClip) {
        String errorString = "Cannot set clip source offset: no clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isFloat32()) {
        String errorString = "Cannot set clip source offset: missing offset value";
        constructReply(reply, 1, errorString);
        return reply;
    }
    double newOffset = message[0].getFloat32();
    selectedClip->setOffset(newOffset);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::trimClipBySeconds(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/clip/trim/seconds/reply");
    if (!selectedClip) {
        String errorString = "Cannot trim clip: No clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    double startTrim = (message.size() >= 1 && message[0].isFloat32()) ? message[0].getFloat32() : 0;
    double endTrim =   (message.size() >= 2 && message[1].isFloat32()) ? message[1].getFloat32() : 0;
    te::EditTimeRange currentRange = selectedClip->getEditTimeRange();
    te::EditTimeRange newRange {currentRange.start + startTrim, currentRange.end - endTrim};

    if (newRange.getLength() <= 0) {
        String errorString = "Cannot trim clip: duration of trimmed clip would be sub-zero";
        constructReply(reply, 1, errorString);
        return reply;
    }
    // verify: does this work when the new start is after the old end, but before the new end????
    selectedClip->setStart(newRange.start, true, false);
    selectedClip->setEnd(newRange.end, true);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setClipLengthBeats(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/clip/length/set/beats/reply");
    if (!selectedClip) {
        String errorString = "Cannot set clip length: No clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    double durationInBeats = 0;
    if (!message.size() || !getFloatOrDouble(message[0], durationInBeats)) {
        String errorString = "Cannot set clip length: First argument must be a float duration";
        constructReply(reply, 1, errorString);
        return reply;
    }

    bool trimStart = false;
    if (message.size() >= 2 && message[1].isString()) {
        if (message[1].getString().toLowerCase().startsWithChar('s')) {
            trimStart = true;
        }
    }

    if (durationInBeats <= 0) {
        String errorString = "Cannot set clip length: duration argument must be greater than 0";
        constructReply(reply, 1, errorString);
        return reply;
    }

    te::EditTimeRange currentRange = selectedClip->getEditTimeRange();
    double startBeat = selectedClip->getStartBeat();
    double endBeat = selectedClip->getEndBeat();

    if (trimStart) {
        double newStartBeat = endBeat - durationInBeats;
        double newStartSeconds = selectedClip->edit.tempoSequence.beatsToTime(newStartBeat);
        selectedClip->setStart(newStartSeconds, true, false);
    } else {
        double newEndBeat = startBeat + durationInBeats;
        double newEndSeconds = selectedClip->edit.tempoSequence.beatsToTime(newEndBeat);
        double newDuration = newEndSeconds - currentRange.start;

         if (newDuration > selectedClip->getMaximumLength()) {
            newEndSeconds = currentRange.start + selectedClip->getMaximumLength();
            newEndBeat = selectedClip->edit.tempoSequence.timeToBeats(newEndSeconds);
        }
        selectedClip->setEnd(newEndSeconds, true);
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setClipLengthSeconds(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/clip/length/set/seconds/reply");
    if (!selectedClip) {
        String errorString = "Cannot set clip length: No clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    double durationSeconds = 0;
    if (!message.size() || !getFloatOrDouble(message[0], durationSeconds)) {
        String errorString = "Cannot set clip length: First argument must be a double or float duration";
        constructReply(reply, 1, errorString);
        return reply;
    }

    bool trimStart = false;
    if (message.size() >= 2 && message[1].isString()) {
        if (message[1].getString().toLowerCase().startsWithChar('s')) {
            trimStart = true;
        }
    }

    if (durationSeconds <= 0) {
        String errorString = "Cannot set clip length: duration argument must be greater than 0";
        constructReply(reply, 1, errorString);
        return reply;
    }

    te::EditTimeRange currentRange = selectedClip->getEditTimeRange();
    double startSeconds = selectedClip->getPosition().getStart();
    double endSeconds = selectedClip->getPosition().getEnd();

    if (trimStart) {
        double newStartSeconds = endSeconds - durationSeconds;
        selectedClip->setStart(newStartSeconds, true, false);
    } else {
        double newEndSeconds = startSeconds + durationSeconds;
        double newDuration = newEndSeconds - currentRange.start;

         if (newDuration > selectedClip->getMaximumLength()) {
            newEndSeconds = currentRange.start + selectedClip->getMaximumLength();
        }
        selectedClip->setEnd(newEndSeconds, true);
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::selectClip(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/clip/select/reply");
    if (!selectedTrack) {
        String errorString = "Cannot select clip: No audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }
    if (!message.size() || !message[0].isString()) {
        String errorString = "Cannot select clip: First argument must be clip name string";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String clipName = message[0].getString();

    if (auto* clipTrack = dynamic_cast<te::ClipTrack*>(selectedTrack)) {
        for (auto clip : clipTrack->getClips()) {
            if (clip->getName().equalsIgnoreCase(clipName)) {
                selectedClip = clip;
                String replyString = "Selected " + clip->getName()
                + " (" + clip->state.getType().toString() + ") on "
                + clipTrack->getName();
                constructReply(reply, 0, replyString);
                return reply;
            }
        }
    }

    String errorString = "Cannot select clip: \"" + clipName
    + "\" not found on track " + selectedTrack->getName();
    constructReply(reply, 1, errorString);
    return reply;
}

cybr::OSCMessage FluidOscServer::renderRegion(const cybr::OSCMessage& message) {
    // Args
    // 0 - (string, required) output filename
    // 1 - (float, optional) start in seconds
    // 2 - (float, optional) duration in seconds
    // If both 1 and 2 are floats, render this time range. Otherwise,
    // render the edit loop region.
    cybr::OSCMessage reply("/audiotrack/region/render/reply");
    if (!selectedTrack) {
        String errorString = "Cannot render track region: No selected track";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (message.size() < 1 || !message[0].isString()) {
        String errorString = "Cannot render track region: Missing filename";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String filename = message[0].getString();
    File outputFile = selectedTrack->edit.filePathResolver(filename);

    te::TransportControl& transport = selectedTrack->edit.getTransport();
    te::EditTimeRange range = transport.getLoopRange();

    {
        double startSeconds = 0;
        double durationSeconds = 0;
        if (message.size() >= 3 && getFloatOrDouble(message[1], startSeconds) && getFloatOrDouble(message[2], durationSeconds)) {
            range.start = startSeconds;
            range.end = startSeconds + durationSeconds;
        }
    }

    renderTrackRegion(outputFile, *selectedTrack, range);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::renderClip(const cybr::OSCMessage &message) {
    // Args
    // 0 - (string, required) output filename
    // 1 - (float, optional) tail in seconds
    cybr::OSCMessage reply("/clip/render/reply");
    if (message.size() < 1 || !message[0].isString()) {
        String errorString = "Cannot render track region: Missing filename";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String filename = message[0].getString();
    File outputFile = selectedTrack->edit.filePathResolver(filename);

    if (!selectedClip) {
        String errorString = "Cannot render selected clip: No clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    te::Track* track = selectedClip->getTrack();
    if (!track) {
        jassert(false);
        String errorString = "Cannot render clip region: Failed to get clip's track";
        constructReply(reply, 1, errorString);
        return reply;
    }

    double tail = (message.size() >= 2 && message[1].isFloat32()) ? message[1].getFloat32() : 0;
    te::EditTimeRange range = selectedClip->getEditTimeRange();
    range.end += tail;

    renderTrackRegion(outputFile, *track, range);
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::saveActiveEdit(const cybr::OSCMessage &message) {
    cybr::OSCMessage reply("/file/save/reply");
    if (!activeCybrEdit) {
        String errorString = "Cannot save active edit: No active edit";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String filename = (message.size() && message[0].isString())
        ? message[0].getString()
        : String();

    // If the first argument is string it is a filename
    File file = (filename.isNotEmpty() && filename != "")
        ? File::getCurrentWorkingDirectory().getChildFile(message[0].getString())
        : activeCybrEdit->getEdit().editFileRetriever();

    // By default use relative file paths. If the second arg begins with 'a', use absolute paths
    auto mode = SamplePathMode::decide;

    if (message.size() >= 2 && message[1].isString()) {
        String arg1 = message[1].getString();
        if (arg1.startsWith("a")) mode = SamplePathMode::absolute;
        else if (arg1.startsWith("r")) mode = SamplePathMode::relative;
        else if (arg1.startsWith("d")) mode = SamplePathMode::decide;
        else std::cout << "Save - unknown SamplePathMode: " << arg1 << std::endl;
    }

    activeCybrEdit->saveActiveEdit(file, mode);
    reply.addInt32(0);
    reply.addString(file.getFullPathName());
    return reply;
}

cybr::OSCMessage FluidOscServer::activateEditFile(File file, bool forceEmptyEdit) {
    cybr::OSCMessage reply("/file/activate/reply");
    if (forceEmptyEdit || !file.existsAsFile()) {
        std::cout << "Creating new edit: " << file.getFullPathName() << std::endl;
        activeCybrEdit = std::make_unique<CybrEdit>(createEmptyEdit(file, te::Engine::getInstance(), te::Edit::forEditing));
        // This is a little hacky, but I want the engine to stop putting
        // "Track 1" in everything. Note that there may be other places that
        // cybr calls createEmptyEdit, and it is not guaranteed that all of them
        // remove "Track 1" (even if they probably should)
        activeCybrEdit->removeTracksNamed("Track 1");
        if (!file.existsAsFile()) activeCybrEdit->saveActiveEdit(file);
    } else {
        std::cout << "Loading edit: " << file.getFullPathName() << std::endl;
        activeCybrEdit = std::make_unique<CybrEdit>(createEdit(file, te::Engine::getInstance(), te::Edit::forEditing));
    }
    constructReply(reply, 0, "Active: " + file.getFileName());
    return reply;
}

cybr::OSCMessage FluidOscServer::activateEditFile(const cybr::OSCMessage &message) {
    cybr::OSCMessage reply("/file/activate/reply");
    if (!message.size() || !message[0].isString()) {
        String errorString = "ERROR: /file/activate missing message argument";
        constructReply(reply, 1, errorString);
        return reply;
    }

    File file = File::getCurrentWorkingDirectory().getChildFile(message[0].getString());
    if (!file.hasFileExtension(".tracktionedit")) {
        std::cout << "WARNING: /file/activate argument does not have .tracktionedit extention: " << file.getFileName() << std::endl;
    }

    bool forceEmptyEdit = (message.size() >= 2 && message[1].isInt32()) ? message[1].getInt32() : false;
    return activateEditFile(file, forceEmptyEdit);
}

cybr::OSCMessage FluidOscServer::changeWorkingDirectory(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/cd/reply");
    if (!message.size() || !message[0].isString()) {
        String errorString = "ERROR: request to change directory without a path string";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String path = message[0].getString();
    File newWorkingDir = File::getCurrentWorkingDirectory().getChildFile(path);

    if (!newWorkingDir.isDirectory() || !newWorkingDir.setAsCurrentWorkingDirectory()) {
        String errorString = "ERROR: Cannot change directory: " + newWorkingDir.getFullPathName();
        constructReply(reply, 1, errorString);
        return reply;
    } else {
        String replyString = "Current Working Directory: " + newWorkingDir.getFullPathName();
        constructReply(reply, 0, replyString);
        return reply;
    }
}

cybr::OSCMessage FluidOscServer::selectAudioTrack(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/select/reply");

    if (!activeCybrEdit) {
        String errorString = "Cannot select track: no active edit";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isString()){
        String errorString = "Cannot select audio track: no track name provided";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String submixName = String();

    if (message.size() >= 2) {
        if (!message[1].isString()) {
            String errorString = "Cannot select audio track: invalid parent track name string";
            constructReply(reply, 1, errorString);
            return reply;
        }
        submixName = message[1].getString();
    }

    String trackName = message[0].getString();
    selectedTrack = getOrCreateAudioTrackByName(activeCybrEdit->getEdit(), trackName, submixName);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::selectReturnTrack(const cybr::OSCMessage &message) {
    cybr::OSCMessage reply("/audiotrack/select/return/reply");
    if (!activeCybrEdit) {
        String errorString = "Cannot select return track: no active edit";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isString()) {
        String errorString = "Cannot select return track: no name provided";
        constructReply(reply, 1, errorString);
        return reply;
    }

    // cybr identifies busses by a name string. Ensure that:
    //     - a track with the specified name exists
    //     - a bus with the specified name exists
    //     - the track has a "receive" plugin, adding the receive if needed

    te::Edit& edit = activeCybrEdit->getEdit();
    String busName = message[0].getString();
    int busIndex = ensureBus(edit, busName);

    if (busIndex == -1) {
        String errorString = "Cannot select return track: no available busses";
        constructReply(reply, 1, errorString);
        return reply;
    }

    selectedTrack = getOrCreateAudioTrackByName(activeCybrEdit->getEdit(), busName);
    jassert(selectedTrack); // I believe this will always return a track

    // Look through plugins on the track, see if it already has an AuxReturnPlugin
    te::AuxReturnPlugin* returnPlugin = nullptr;
    for (te::Plugin* checkPlugin : selectedTrack->pluginList) {
        if (auto foundPlugin = dynamic_cast<te::AuxReturnPlugin*>(checkPlugin)) {
            if (foundPlugin->busNumber == busIndex) {
                String replyString = "Skip insert aux return plugin. Edit already has " + busName + " return";
                constructReply(reply, 0, replyString);
                returnPlugin = foundPlugin;
                break;
            } else {
                String replyString = "Note: An unexpected auxreturn plugin was found while selecting return track (an additional one may be created)";
                constructReply(reply, 0, replyString);
                returnPlugin = foundPlugin;
            }
        }
    }

    // If no return plugin was found on the track insert a new one before all
    // other plugins on the track
    if (!returnPlugin) {
        te::Plugin::Ptr plugin = selectedTrack->edit.getPluginCache().createNewPlugin("auxreturn", PluginDescription());
        if (auto foundPlugin = dynamic_cast<te::AuxReturnPlugin*>(plugin.get())) {
            returnPlugin = foundPlugin;
            returnPlugin->busNumber = busIndex;
            selectedTrack->pluginList.insertPlugin(foundPlugin, 0, nullptr);

            String replyString = "Insert auxreturn plugin with busNumber: " + String(busIndex);
            constructReply(reply, 0, replyString);
        }
    }
    return reply;
}

/**
 * Verify that a send to a particular bus exists. If it does not, add it to
 * the end of the plugin chain.
 */
cybr::OSCMessage FluidOscServer::ensureSend(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/send/set/db/reply");
    if (!selectedTrack) {
        String errorString = "Cannot ensure send: no audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isString()) {
        String errorString = "Cannot ensure send: message needs name";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String busName = message[0].getString();
    float gainLevel = 0;
    String position = "post-gain";

    if (message.size() >= 2 && message[1].isFloat32()) {
        gainLevel = message[1].getFloat32();
    }

    if (message.size() >= 3 && message[2].isString()) {
        position = message[2].getString();
    }

    // cybr identifies busses by a name
    int busIndex = ensureBus(selectedTrack->edit, busName);

    if (busIndex == -1) {
        String errorString = "Cannot create send: no available busses";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!position.equalsIgnoreCase("post-gain")) {
        String errorString = "Cannot ensure send: currently only post-gain sends are supported";
        constructReply(reply, 1, errorString);
        return reply;
    }

    // Look through plugins on the track, see if it already has an AuxSendPlugin
    te::AuxSendPlugin* sendPlugin = nullptr;
    bool foundVolume = false;
    for (te::Plugin* checkPlugin : selectedTrack->pluginList) {
        if (!foundVolume) {
            if (auto foundPlugin = dynamic_cast<te::VolumeAndPanPlugin*>(checkPlugin))
                foundVolume = true;
        }
        if (auto foundPlugin = dynamic_cast<te::AuxSendPlugin*>(checkPlugin)) {
            if (foundPlugin->busNumber == busIndex) {
                String replyString = "Skip insert aux send plugin. Edit already has " + busName + " send";
                constructReply(reply, 0, replyString);
                sendPlugin = foundPlugin;
                break;
            }
        }
    }

    if (!sendPlugin) {
        te::Plugin::Ptr plugin = selectedTrack->edit.getPluginCache().createNewPlugin("auxsend", PluginDescription());
        if (auto foundPlugin = dynamic_cast<te::AuxSendPlugin*>(plugin.get())) {
            sendPlugin = foundPlugin;
            sendPlugin->busNumber = busIndex;
            selectedTrack->pluginList.insertPlugin(foundPlugin, -1, nullptr);
            String replyString = "Insert auxsend plugin with busNumber: " + String(busIndex);
            constructReply(reply, 0, replyString);
        }
    }

    if (sendPlugin) {
        sendPlugin->setGainDb(gainLevel);
    }
    return reply;
}

cybr::OSCMessage FluidOscServer::selectPlugin(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/select/reply");
    if (message.size() > 3 ||
        message.size() < 2 ||
        !message[0].isString() ||
        !message[1].isInt32() ){
        String errorString = "selectPlugin failed. Incorrect arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }
    String pluginName = message[0].getString();
    int index = message[1].getInt32();
    String pluginFormat = String();

    if (message.size() > 2 && message[2].isString())
        pluginFormat = message[2].getString();
    if (!selectedTrack){
        String errorString = "Cannot select plugin: No audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }
    selectedPlugin = getOrCreatePluginByName(*selectedTrack, pluginName, pluginFormat, index);
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::selectVst2PluginById(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/vst2/select/reply");
    if (!selectedTrack) {
        String errorString = "Cannot select vst2 plugin by ID: No audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (message.size() < 2 || !message[0].isInt32() || !message[1].isInt32()) {
        String errorString = "Cannot select vst2 plugin by ID: Incorrect arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto pluginUid = message[0].getInt32();
    auto positionId = message[1].getInt32(); // nth plugin of type

    for (PluginDescription pluginDesc : selectedTrack->edit.engine.getPluginManager().knownPluginList.getTypes()) {
        if (pluginDesc.pluginFormatName == "VST") {
            if (pluginDesc.uid == pluginUid) {
                selectedPlugin = getOrCreatePluginByName(*selectedTrack, pluginDesc.name, pluginDesc.pluginFormatName, positionId);
                reply.addInt32(0);
                return reply;
            }
        }
    }

    constructReply(reply, 1, "Cannot selecte VST2 plugin by ID: plugin not found");
    return reply;
}

cybr::OSCMessage FluidOscServer::setPluginParam(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/param/set");
    if (message.size() > 3 ||
        !message[0].isString() ||
        !message[1].isFloat32() ||
        !message[2].isString()) {
        String errorString = "Setting parameter failed. Incorrect arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!selectedPlugin) {
        String errorString = "Setting plugin parameter failed: No selected plugin";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String paramName = message[0].getString();
    float paramValue = message[1].getFloat32();
    bool isNormalized = message[2].getString() == "normalized";
    if(isNormalized){
        if (paramValue > 1 || paramValue < 0){
            String errorString = "Setting parameter " + paramName
            + " failed. Normalized value has to be between 0 and 1.";
            constructReply(reply, 1, errorString);
            return reply;
        }
    }

    if (auto rack = dynamic_cast<te::RackInstance*>(selectedPlugin)) {
        auto rackType = selectedTrack->edit.getRackList().getRackTypeForID(rack->rackTypeID);
        for (auto macro : rackType->macroParameterList.getMacroParameters()) {
            if (macro->macroName == paramName) { // CAUTION: this is case sensitive, while below is insensitive
                macro->setParameter(paramValue, juce::NotificationType::sendNotificationSync);
                constructReply(reply, 0, "Set " + macro->macroName + " to " + macro->getCurrentValueAsString());
                return reply;
            }
        }
    }

    // Iterate over the parameter list in reverse. This is a slightly hacky way
    // to dodge the "Dry Level" and "Wet Level" parameters that tracktion adds
    // to all plugins. Some external plugins may have their own Dry/Wet level
    // params. Because the tracktion versions always come first, we only find
    // them if the plugin does not provide its own version.
    for (int i = selectedPlugin->getNumAutomatableParameters() - 1; i >= 0; i--) {
        te::AutomatableParameter::Ptr param = selectedPlugin->getAutomatableParameter(i);
        if (param->paramName.equalsIgnoreCase(paramName)) {

            param->parameterChangeGestureBegin(); // BEGIN GESTURE

            if (selectedPlugin->getPluginType() == "vst" && param->getCurrentValue() == paramValue) {
                // An interesting edge-case was causing a bug. To reproduce:
                // 1) assume the parameter is initially set to 0.5
                // 2) load a vst preset which sets the parameter to 1.0
                // 3) instantly set the parameter back to 0.5
                // In this case, the value tree has not yet updated, and it "looks like"
                // the parameter is already set to 0.5, causing it not to be updated.
                // To address this, we use a hacky trick to force the parameter to udpate.
                float dummy = (param->valueRange.end == paramValue) ? paramValue - 0.00001 : paramValue + 0.00001;
                if (isNormalized) param->setNormalisedParameter(dummy, NotificationType::sendNotification);
                else param->setParameter(dummy, NotificationType::sendNotification);
            }

            if (isNormalized) param->setNormalisedParameter(paramValue, NotificationType::sendNotification);
            else param->setParameter(paramValue, NotificationType::sendNotificationSync);

            param->parameterChangeGestureEnd(); // END GESTURE

            String replyString = "set " + paramName
            + " to " + String(message[1].getFloat32())
            + " explicitvalue: "
            + param->valueToString(param->getCurrentExplicitValue());

            constructReply(reply, 0, replyString);
            return reply;
        }
    }

    return reply;
}

cybr::OSCMessage FluidOscServer::setPluginParamAt(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/param/set/at/reply");
    double changeSeconds = 0;

    if (message.size() > 5 ||
        !message[0].isString() ||
        !message[1].isFloat32() ||
        !getFloatOrDouble(message[2], changeSeconds) ||
        !message[3].isFloat32() ||
        !message[4].isString() ) {
        String errorString = "Setting parameter failed. Incorrect arguments. (sfffs or sfdfs, expected).";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!selectedPlugin) {
        String errorString = "Setting plugin parameter failed: No selected plugin";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String paramName = message[0].getString();
    float paramValue = message[1].getFloat32();
    bool isNormalized = message[4].getString() == "normalized";
    if (isNormalized){
        if (paramValue > 1 || paramValue < 0) {
            std::cout << "Setting parameter " + paramName + ": normalized value clamped" << std::endl;
        }

        if (paramValue > 1) paramValue = 1;
        else if ( paramValue < 0) paramValue = 0;
    }

    
    if (changeSeconds < 0) {
        String errorString = "Setting parameter " + paramName
        + " failed. Time has to be a positive number.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    float curveValue = message[3].getFloat32();
    if (curveValue > 1 || curveValue < -1) {
        std::cout << "Setting parameter " + paramName + "curve value clamped" << std::endl;
        if (curveValue < -1) curveValue = -1;
        else if (curveValue > 1) curveValue = 1;
    }

    te::AutomatableParameter::Ptr foundParam;
    if (auto rack = dynamic_cast<te::RackInstance*>(selectedPlugin)) {
        auto rackType = selectedTrack->edit.getRackList().getRackTypeForID(rack->rackTypeID);
        for (auto macro : rackType->macroParameterList.getMacroParameters()) {
            if (macro->macroName == paramName) { // CAUTION: this is case sensitive, while below is insensitive
                foundParam = macro;
                break;
            }
        }
    }

    // Iterate over the parameter list in reverse. This is a slightly hacky way
    // to dodge the "Dry Level" and "Wet Level" parameters that tracktion adds
    // to all plugins. Some external plugins may have their own Dry/Wet level
    // params. Because the tracktion versions always come first, we only find
    // them if the plugin does not provide its own version.
    if (!foundParam) {
        for (int i = selectedPlugin->getNumAutomatableParameters() - 1; i >= 0; i--) {
            te::AutomatableParameter::Ptr param = selectedPlugin->getAutomatableParameter(i);
            if (param->paramName.equalsIgnoreCase(paramName)) {
                foundParam = param;
                break;
            }
        }
    }

    if (foundParam) {
        if (isNormalized) paramValue = foundParam->valueRange.convertFrom0to1(paramValue);
        foundParam->getCurve().addPoint(changeSeconds, paramValue, curveValue);
        String replyString = "set " + paramName
        + " to " + String(message[1].getFloat32()) + " explicit value: " + foundParam->valueToString(paramValue)
        + " at " + String(changeSeconds) + " seconds(s).";
        constructReply(reply, 0, replyString);
        return reply;
    }

    constructReply(reply, 1, "Failed to find param named: " + paramName);
    return reply;
}

cybr::OSCMessage FluidOscServer::setTrackWidth(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/set/width/reply");

    if (!selectedTrack) {
        constructReply(reply, 1, "Cannot set track width: No audiotrack selected");
        return reply;
    }

    if (!message.size() || !message[0].isFloat32()) {
        constructReply(reply, 1, "Cannot set track width: missing float argument");
        return reply;
    }

    bool isAutomation = false;
    float curveValue = 0;
    double timeSeconds = 0;
    if (message.size() >= 2) {
        isAutomation = true;
        if (!getFloatOrDouble(message[1], timeSeconds)) {
            constructReply(reply, 1, "Cannot set track width automation point: time value must be a float or double");
            return reply;
        }
        if (message.size() >= 3 && message[2].isFloat32()) {
            curveValue = message[2].getFloat32();
        }
    }

    float paramValue = message[0].getFloat32() * 0.5 + 0.5;
    auto plugin = getOrCreatePluginByName(*selectedTrack, "cybr-width", "tracktion");
    auto rack = dynamic_cast<te::RackInstance*>(plugin);
    jassert(rack);

    if (isAutomation) {
        for (auto macro : selectedTrack->macroParameterList.getMacroParameters()) {
            if (macro->macroName == "width automation") {
                // Charles: I'm a little confused about the "macro" data type.
                // It seems like it is a regular pointer, while setParamAutomationPoint
                // accepts a juce style smart pointer. Does this mean that it
                // might automatically cast to a smart points, causeing it to be
                // erroneously freed when the smart pointer gets deleted?
                paramValue = macro->valueRange.convertFrom0to1(paramValue);
                macro->getCurve().addPoint(timeSeconds, paramValue, curveValue);
            }
        }
    } else  {
        for (auto macro : selectedTrack->macroParameterList.getMacroParameters()) {
            if (macro->macroName == "width") {
                macro->setParameter(paramValue, juce::NotificationType::sendNotificationSync);
            }
        }
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setPluginSideChainInput(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/sidechain/input/set/reply");
    if (!selectedPlugin) {
        String errorString = "Cannot set plugin side chain input: No selected plugin";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!selectedPlugin->canSidechain()) {
        String errorString = "Cannot set plugin side chain input: Selected plugin cannot side chain";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isString()) {
        String errorString = "Cannot set plugin side chain input: Missing input-track-name arg";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String inputTrackname = message[0].getString();
    te::AudioTrack* inputTrack = getOrCreateAudioTrackByName(selectedPlugin->edit, inputTrackname);
    selectedPlugin->setSidechainSourceID(inputTrack->itemID);
    std::cout << "Side chain input: " << inputTrack->getName() << std::endl;

    if (auto compressor = dynamic_cast<te::CompressorPlugin*>(selectedPlugin)) {
        std::cout << "NOTE: when enabling a side chain in put on the internal compressor plugin, the side chain will be enabled by default. " << std::endl;
        compressor->useSidechainTrigger = true;
    }

    selectedPlugin->guessSidechainRouting();

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::getPluginReport(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/report/reply");

    if (!selectedPlugin) {
        String errorString = "Cannot get plugin report: No selected plugin";
        constructReply(reply, 1, errorString);
        return reply;
    }

    DynamicObject::Ptr object = getPluginReportObject(selectedPlugin);

    reply.addInt32(0);
    reply.addString("Retrieved JSON report about plugin");
    reply.addString(JSON::toString(object.get(), true));

    return reply;
}

cybr::OSCMessage FluidOscServer::getPluginParametersReport(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/params/report/reply");

    if (!selectedPlugin) {
        String errorString = "Cannot get plugin parameter report: No selected plugin";
        constructReply(reply, 1, errorString);
        return reply;
    }

    int steps = (message.size() && message[0].isInt32()) ? message[0].getInt32() : 0;
    auto array = getAllParametersReport(selectedPlugin, steps);

    // Create JSON of the results
    String jsonString = JSON::toString(array, true);
    reply.addInt32(0);
    reply.addString("Retrieved JSON report about plugin parameters");
    reply.addString(jsonString);

    return reply;
}

cybr::OSCMessage FluidOscServer::getPluginParameterReport(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/param/report/reply");

    if (!selectedPlugin) {
        constructReply(reply, 1, "Cannot get plugin single parameter report: No selected plugin");
        return reply;
    }

    if (!message.size() || !message[0].isString()) {
        constructReply(reply, 1, "Cannot get plugin single parameter report: Missing param name");
        return reply;
    }

    String paramName = message[0].getString();
    int steps = (message.size() >= 2 && message[1].isInt32()) ? message[1].getInt32() : 0;

    for (int i = selectedPlugin->getNumAutomatableParameters() - 1; i >= 0; i--) {
        auto param = selectedPlugin->getAutomatableParameter(i);
        if (param->paramName.equalsIgnoreCase(paramName)) {
            juce::var report = getSingleParameterReport(param.get(), steps);
            String jsonString = JSON::toString(report, true);
            reply.addInt32(0);
            reply.addString("Retrieved JSON report about single plugin parameter");
            reply.addString(jsonString);
            return reply;
        }
    }

    constructReply(reply, 1, "Cannot get plugin single parameter report: \"" + paramName + "\" Parameter not found");
    return reply;
}

cybr::OSCMessage FluidOscServer::savePluginPreset(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/save/reply");
    if (!selectedPlugin) {
        String errorString = "Cannot save plugin preset: No selected plugin";
        constructReply(reply, 1, errorString);
        return reply;
    }
    if (message.size() < 1 || !message[0].isString()) {
        String errorString = "Cannot save plugin preset: First argument must be a name string";
        constructReply(reply, 1, errorString);
        return reply;
    }
    saveTracktionPreset(selectedPlugin, message[0].getString());
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::loadPluginTrkpreset(const cybr::OSCMessage &message) {
    cybr::OSCMessage reply("/plugin/load/trkpreset/reply");
    if (!selectedTrack) {
        String errorString = "Cannot load plugin preset: No audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isBlob()) {
        String errorString = "Cannot load trkpreset data: Mising blob";
        constructReply(reply, 1, errorString);
        return reply;
    }

    MemoryBlock blob = message[0].getBlob();
    String string = String::createStringFromData(blob.getData(), (int)blob.getSize());
    std::unique_ptr<XmlElement> xml = parseXML(string);
    if (!xml) {
        String errorString = "Cannot load trkpreset data: XML parser error";
        constructReply(reply, 1, errorString);
        return reply;
    }

    ValueTree v = ValueTree::fromXml(*xml.get());
    if (!v.isValid()) {
        String errorString = "Cannot load trkpreset data: Invalid ValueTree";
        constructReply(reply, 1, errorString);
        return reply;
    }

    loadTracktionPreset(*selectedTrack, v);
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::loadPluginPreset(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/plugin/load/reply");
    if (!selectedTrack) {
        String errorString = "Cannot load plugin preset: No audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (message.size() < 1 || !message[0].isString()) {
        String errorString = "Cannot load plugin preset: Message has no preset name";
        constructReply(reply, 1, errorString);
        return reply;
    }

    String filename = message[0].getString();
    if (!filename.endsWithIgnoreCase(".trkpreset")) filename.append(".trkpreset", 10);

    File editDirectory = selectedTrack->edit.editFileRetriever().getParentDirectory();
    File file = editDirectory.getChildFile(filename);

    // First check if the file is an absolute file, OR was found relative to
    // the edit file directory.
    if (file.existsAsFile()) {
        filename = file.getFullPathName(); // Found it!
    } else {
        // Look in the Cybr Search Path.
        file = CybrSearchPath(CYBR_PRESET).find(filename);
        if (file != File()) filename = file.getFullPathName(); // Found it!
        else std::cout << "Warning: preset file not found: " << filename << std::endl;
    }

    ValueTree v = loadXmlFile(file);

    if (!v.isValid()) {
        String errorString = "Cannot load plugin preset: Failed to load and parse file";
        constructReply(reply, 1, errorString);
        return reply;
    }

    loadTracktionPreset(*selectedTrack, v);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::loadVst2PresetInSelectedPlugin(const cybr::OSCMessage& message) {
    // Set state from the binary contents of a .fxp or .fxb file. This can be loaded
    cybr::OSCMessage reply("/plugin/load/vst2preset/reply");
    if (!selectedPlugin) {
        constructReply(reply, 1, "Cannot set vst2 plugin state: No plugin selected");
        return reply;
    }

    if (message.size() < 1) {
        constructReply(reply, 1, "Cannot set vst2 fxb/fxp preset: to few arguments");
        return reply;
    }

    if (!message[0].isBlob() && !message[0].isString()) {
        constructReply(reply, 1, "Cannot set vst2 fxb/fxp preset: invalid argument");
        return reply;
    }

    MemoryBlock stateBlob;
    MemoryOutputStream outStream;
    if (message[0].isString()) {
        if (!Base64::convertFromBase64(outStream, message[0].getString())) {
            constructReply(reply, 1, "Cannot set vst2 fxb/fxp preset: invalid base64 string");
            return reply;
        };
        stateBlob.append(outStream.getData(), outStream.getDataSize());
    } else {
        stateBlob = message[0].getBlob();
    }

    if (auto externalPlugin = dynamic_cast<te::ExternalPlugin*>(selectedPlugin)) {
        if (externalPlugin->isVST()) {

            juce::AudioPluginInstance* jucePlugin = externalPlugin->getAudioPluginInstance();

            jucePlugin->suspendProcessing(true);
            jucePlugin->setCurrentProgramStateInformation(stateBlob.getData(), (int)stateBlob.getSize());
            jucePlugin->suspendProcessing(false);

            externalPlugin->flushPluginStateToValueTree();
            constructReply(reply, 0, "Loaded vst state");
            return reply;
        }
    }

    constructReply(reply, 1, "Cannot set vst2 plugin state: selected plugin is not an external VST2 plugin");
    return reply;
}

cybr::OSCMessage FluidOscServer::selectMidiClip(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/midiclip/select/reply");
    if (!selectedTrack){
        String errorString = "Cannot load plugin preset: No audio track selected";
        constructReply(reply, 1, errorString);
        return reply;
    }
    if (!message.size() || !message[0].isString()){
        String errorString = "Cannot select Midi Clip: Missing arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (auto* clipTrack = dynamic_cast<te::ClipTrack*>(selectedTrack)) {
        String clipName = message[0].getString();
        selectedClip = getOrCreateMidiClipByName(*clipTrack, clipName);

        double startSeconds = 0;
        double durationSeconds = 0;
        // Clip startBeats
        if (message.size() >= 2 && getFloatOrDouble(message[1], startSeconds)) {
            selectedClip->setStart(startSeconds, false, true);
        }
        // Clip length
        if (message.size() >= 3 && getFloatOrDouble(message[2], durationSeconds)) {
            double endTime = startSeconds + durationSeconds;
            selectedClip->setEnd(endTime, true);
        }
    } else {
        String errorString = "Cannot select Midi Clip: selected track is not a ClipTrack.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::clearMidiClip(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/midiclip/clear");
    if (!selectedClip) {
        String errorString = "Cannot clear midi clip: No clip selected";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto selectedMidiClip = dynamic_cast<te::MidiClip*>(selectedClip);
    if (!selectedMidiClip) {
        String errorString = "Cannot clear midi clip: selected clip is not a midi clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto exisiting = selectedMidiClip->state.getChildWithName(te::IDs::SEQUENCE);
    if (!exisiting.isValid()) selectedMidiClip->clearTakes();
    selectedMidiClip->getSequence().clear(nullptr); // is this needed?

    // If the clip is currently playing, we probably want to send an all notes
    // off message, which looks roughly like this
    if (auto track = dynamic_cast<te::AudioTrack*>(selectedMidiClip->getTrack()))
        for (int i = 1; i <= 16; ++i)
            track->injectLiveMidiMessage(MidiMessage::allNotesOff(i),
                                         te::MidiMessageArray::notMPE);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::insertMidiNoteBeats(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/midiclip/insert/note/beats/reply");
    if(!selectedClip){
        String errorString = "Cannot insert midi note: No clip selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }
    if (message.size() < 3) {
        String errorString = "Cannot insert midi note (beats): Not enough arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto selectedMidiClip = dynamic_cast<te::MidiClip*>(selectedClip);
    if (!selectedMidiClip) {
        String errorString = "Cannot insertMidiNote: selected clip is not a midi clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    for (const auto& arg : message) {
        if (!arg.isInt32() && !arg.isFloat32()){
            String errorString = "Cannot insertMidiNote: arguments have incorrect type";
            constructReply(reply, 1, errorString);
            return reply;
        }
    }
    double startBeat = 0;
    double lengthInBeats = 1;
    int noteNumber = 0;
    int velocity = 64;
    int colorIndex = 0;

    if (message[0].isInt32()) noteNumber = message[0].getInt32();
    else if (message[0].isFloat32()) noteNumber = (int)(message[0].getFloat32());

    if (message[1].isFloat32()) startBeat = message[1].getFloat32() * 4.0;
    else if (message[1].isInt32()) startBeat = message[1].getInt32() * 4;

    if (message[2].isFloat32()) lengthInBeats = message[2].getFloat32() * 4.0;
    else if (message[2].isInt32()) lengthInBeats = (int)(message[2].getInt32()) * 4;

    if (message.size() >= 4) {
        if (message[3].isInt32()) velocity = message[3].getInt32();
        else if (message[3].isFloat32()) velocity = message[3].getFloat32();
    }

    if (message.size() >= 5) {
        if (message[4].isInt32()) colorIndex = message[4].getInt32();
        else if (message[4].isFloat32()) colorIndex = (int)(message[4].getFloat32());
    }

    te::MidiList& notes = selectedMidiClip->getSequence();
    notes.addNote(noteNumber, startBeat, lengthInBeats, velocity, colorIndex, nullptr);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::insertMidiNote(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/midiclip/insert/note/reply");
    if (!selectedClip) {
        String errorString = "Cannot insert midi note: No clip selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }
    if (message.size() < 3) {
        String errorString = "Cannot insert midi note: Not enough arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    auto selectedMidiClip = dynamic_cast<te::MidiClip*>(selectedClip);
    if (!selectedMidiClip) {
        String errorString = "Cannot insertMidiNote: selected clip is not a midi clip";
        constructReply(reply, 1, errorString);
        return reply;
    }

    int noteNumber = 0;           // message[0]
    double startSeconds = 0.0;    // message[1] (relative to start of clip)
    double durationSeconds = 1.0; // message[2]
    int velocity = 64;            // message[3]
    int colorIndex = 0;           // message[4]

    if (message[0].isInt32()) noteNumber = message[0].getInt32();
    else if (message[0].isFloat32()) noteNumber = (int)(message[0].getFloat32());
    else {
        constructReply(reply, 1, "Cannot insertMidiNoteSeconds: arg1 must be an int or float");
        return reply;
    }
    if (!getFloatOrDouble(message[1], startSeconds)) {
        constructReply(reply, 1, "Cannot insertMidiNoteSeconds: arg2 must be a float or double");
        return reply;
    }
    if (!getFloatOrDouble(message[2], durationSeconds)) {
        constructReply(reply, 1, "Cannot insertMidiNoteSeconds: arg3 must be a float or double");
        return reply;
    }
    if (message.size() >= 4) {
        if (message[3].isInt32()) velocity = message[3].getInt32();
        else if (message[3].isFloat32()) velocity = (int)message[3].getFloat32();
        else if (message[3].isFloat64()) velocity = (int)message[3].getFloat64();
    }

    if (message.size() >= 5) {
        if (message[4].isInt32()) colorIndex = message[4].getInt32();
        else if (message[4].isFloat32()) colorIndex = (int)message[4].getFloat32();
        else if (message[4].isFloat64()) colorIndex = (int)message[4].getFloat64();
    }
    double clipStartBeat = selectedMidiClip->getStartBeat();
    double startBeat = selectedMidiClip->getBeatOfRelativeTime(startSeconds) - clipStartBeat;
    double endBeat = selectedMidiClip->getBeatOfRelativeTime(startSeconds + durationSeconds) - clipStartBeat;
    double lengthInBeats = endBeat - startBeat;

    te::MidiList& notes = selectedMidiClip->getSequence();
    notes.addNote(noteNumber, startBeat, lengthInBeats, velocity, colorIndex, nullptr);

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::insertWaveSample(const cybr::OSCMessage& message){
    cybr::OSCMessage reply("/audiotrack/insert/wav/reply");
    if(!selectedTrack){
        String errorString = "Cannot insert wave sample: Must select Audio Track before inserting";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if(message.size() < 3){
        String errorString = "Cannot insert wave file: only received " + String(message.size()) + "arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message[0].isString()) {
        String errorString = "Cannot insert wave file: first argument must be a clipName string";
        constructReply(reply, 1, errorString);
        return reply;
    }
    String clipName = message[0].getString();

    if (!message[1].isString()) {
        String errorString = "Cannot insert wave file: second argument must be a filepath string";
        constructReply(reply, 1, errorString);
        return reply;
    }
    String filePath = message[1].getString();

    if (!message[2].isFloat64() && !message[2].isFloat32() && !message[2].isInt32()) {
        String errorString = "Cannot insert wave file: third argument must be a start time in whole notes (int or float)";
        constructReply(reply, 1, errorString);
        return reply;
    }

    double startSeconds = 0;
    if (getFloatOrDouble(message[2], startSeconds)) {}
    else if (message[2].isInt32()) { startSeconds = (double)message[2].getInt32(); }
    else { jassertfalse; } // should never happen

    // The default filePathResolver checks for an absolute file, then looks
    // in the relative to the edit file directory (using edit.editFileRetriever)
    File file = selectedTrack->edit.filePathResolver(filePath);
    // First check if the file is an absolute file, OR was found relative to
    // the edit file directory.
    if (file.existsAsFile()) {
        filePath = file.getFullPathName(); // Found it!
    } else {
        // Look in the sample search path.
        file = CybrSearchPath(CYBR_SAMPLE).find(filePath);
        if (file != File()) filePath = file.getFullPathName(); // Found it!
        else std::cout << "Cannot insert wave file: File not found: " << filePath << std::endl;
    }

    te::AudioFile audiofile(selectedTrack->edit.engine, file);
    if(!audiofile.isValid() || audiofile.isNull()){
        String errorString = "Cannot insert wave file: Must be valid audio file.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (auto* audioTrack = dynamic_cast<te::AudioTrack*>(selectedTrack)) {
        te::EditTimeRange timeRange = te::EditTimeRange(startSeconds, startSeconds+audiofile.getLength());
        te::ClipPosition pos;
        pos.time = timeRange;
        te::WaveAudioClip::Ptr c = audioTrack->insertWaveClip(clipName, file, pos, false);
        c->setFadeInType(te::AudioFadeCurve::convex);
        c->setFadeOutType(te::AudioFadeCurve::convex);
        selectedClip = c.get();
    } else {
        String errorString = "Cannot insert wave file: Selected track is not an AudioTrack";
        constructReply(reply, 1, errorString);
        return reply;
    }

    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setTrackGain(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/set/db/reply");
    if (!selectedTrack) {
        String errorString = "Cannot set track gain: No track selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isFloat32()) {
        String errorString = "Cannot set track gain: Missing arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    float gainDb = message[0].getFloat32();
    bool isAutomation = false;
    float curveValue = 0;
    double timeSeconds = 0;
    if (message.size() >= 2) {
        isAutomation = true;
        if (!getFloatOrDouble(message[1], timeSeconds)) {
            constructReply(reply, 1, "Cannot set track gain automation point: time value must be a float or double");
            return reply;
        }
        if (message.size() >= 3 && message[2].isFloat32()) {
            curveValue = message[2].getFloat32();
        }
    }

    if (isAutomation) {
        getOrCreatePluginByName(*selectedTrack, "volume", "tracktion", 1);
        auto plugin = getOrCreatePluginByName(*selectedTrack, "volume", "tracktion", 0);
        if (auto volumePlugin = dynamic_cast<te::VolumeAndPanPlugin*>(plugin)) {
            float paramValue = te::decibelsToVolumeFaderPosition(gainDb);
            volumePlugin->volParam->getCurve().addPoint(timeSeconds, paramValue, curveValue);
            reply.addInt32(0);
            return reply;
        }
    } else if (auto volumePlugin = selectedTrack->pluginList.getPluginsOfType<te::VolumeAndPanPlugin>().getLast()) {
        volumePlugin->setVolumeDb(gainDb);
        reply.addInt32(0);
        return reply;
    }

    String errorString = "Cannot set track gain: Track is missing volume plugin or cybr rack";
    constructReply(reply, 1, errorString);
    return reply;
}

cybr::OSCMessage FluidOscServer::setTrackPan(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiotrack/set/pan/reply");
    if (!selectedTrack) {
        String errorString = "Cannot set track pan: No track selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    if (!message.size() || !message[0].isFloat32()) {
        String errorString = "Cannot set track pan: Missing arguments.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    float panValue = message[0].getFloat32();
    bool isAutomation = false;
    float curveValue = 0;
    double timeSeconds = 0;
    if (message.size() >= 2) {
        isAutomation = true;
        if (!getFloatOrDouble(message[1], timeSeconds)) {
            constructReply(reply, 1, "Cannot set track pan automation point: time value must be a float");
            return reply;
        }
        if (message.size() >= 3 && message[2].isFloat32()) {
            curveValue = message[2].getFloat32();
        }
    }

    if (isAutomation) {
        ensureWidthRack(*selectedTrack);
        for (auto macro : selectedTrack->macroParameterList.getMacroParameters()) {
            if (macro->macroName == "pan automation") {
                macro->getCurve().addPoint(timeSeconds, panValue * 0.5 + 0.5, curveValue);
                reply.addInt32(0);
                return reply;
            }
        }
    } else if (auto volumePlugin = selectedTrack->pluginList.getPluginsOfType<te::VolumeAndPanPlugin>().getLast()) {
        volumePlugin->setPan(panValue);
        reply.addInt32(0);
        return reply;
    }

    String errorString = "Cannot set track gain: Track is missing volume plugin or cybr rack.";
    constructReply(reply, 1, errorString);
    return reply;
}

cybr::OSCMessage FluidOscServer::muteTrack(bool mute) {
    cybr::OSCMessage reply("/audiotrack/mute/reply");
    if (!selectedTrack) {
        String errorString = "Cannot " + String((mute ? "mute" : "unmute")) + ": No track selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    selectedTrack->setMute(mute);
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::setTempo(const cybr::OSCMessage &message) {
    cybr::OSCMessage reply("/tempo/set/reply");
    if (!message.size() || !message[0].isFloat32()) {
        String errorString = "Cannot set tempo: missing tempo argument";
        constructReply(reply, 1, errorString);
        return reply;
    }

    float bpm = message[0].getFloat32();
    te::TempoSetting* tempo = activeCybrEdit->getEdit().tempoSequence.getTempo(0);
    tempo->setBpm(bpm);
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::handleSamplerMessage(const cybr::OSCMessage &message) {
    cybr::OSCMessage reply("/plugin/sampler/reply");
    if (!selectedPlugin) {
        String errorString = "Cannot update sampler. No plugin selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    te::SamplerPlugin* sampler = dynamic_cast<te::SamplerPlugin*>(selectedPlugin);
    if (!sampler) {
        String errorString = "Cannot update sampler. No sampler selected.";
        constructReply(reply, 1, errorString);
        return reply;
    }

    const cybr::OSCAddressPattern pattern = message.getAddressPattern();
    if (pattern.matches({"/plugin/sampler/add"})) {
        // Required:
        // 0 - (string) name
        // 1 - (string) filepath
        // 2 - (int) note number
        // Optional:
        // 3 - gain (float, default = 0)
        // 4 - pan (float, default = 0)
        // 5 - oneShot (int, default = 0/false)
        //
        // The filepath can be
        // 1) relative relative to the .tracktionedit file
        // 2) relativeto the server's working directory
        // 3) absolute
        // If the file is not found, it will still be added, but it will not play.
        if (message.size() < 3) {
            String errorString = "Cannot add sampler sound: Not enough arguments";
            constructReply(reply, 1, errorString);
            return reply;
        }

        if (!message[0].isString() || !message[1].isString() || ! message[2].isInt32()) {
            String errorString = "Cannot add sampler sound: incorrect arguments";
            constructReply(reply, 1, errorString);
            return reply;
        }

        int index = sampler->getNumSounds();
        String soundName = message[0].getString();
        String filePath = message[1].getString();
        int noteNumber = message[2].getInt32();
        double gain = (message.size() >= 4 && message[3].isFloat32()) ? message[3].getFloat32() : 0;
        double pan = (message.size() >= 5 && message[4].isFloat32()) ? message[4].getFloat32() : 0;
        double oneShot = (message.size() >= 6 && message[5].isInt32()) ? message[5].getInt32() : 0;

        File editDirectory = selectedPlugin->edit.editFileRetriever().getParentDirectory();
        File file = editDirectory.getChildFile(filePath);

        // First check if the file is an absolute file, OR was found relative to
        // the edit file directory.
        if (file.existsAsFile()) {
            filePath = file.getFullPathName(); // Found it!
        } else {
            // Look in the sample search path.
            file = CybrSearchPath(CYBR_SAMPLE).find(filePath);
            if (file != File()) filePath = file.getFullPathName(); // Found it!
            else std::cout << "Warning: sampler trying to add sampler sound, but file not found: " << filePath << std::endl;
        }

        sampler->addSound(filePath, soundName, 0, 0, gain);
        sampler->setSoundGains(index, gain, pan);
        sampler->setSoundParams(index, noteNumber, noteNumber, noteNumber);
        sampler->setSoundOpenEnded(index, oneShot);
    } else if (pattern.matches({"/plugin/sampler/clear-all"})) {
        sampler->state.removeAllChildren(nullptr);
    }
    reply.addInt32(0);
    return reply;
}

cybr::OSCMessage FluidOscServer::handleTransportMessage(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/transport/reply");
    if (!activeCybrEdit) {
        String errorString = "Cannot update transport: No active edit";
        constructReply(reply, 1, errorString);
        return reply;
    }
    te::TransportControl& transport = activeCybrEdit->getEdit().getTransport();

    const cybr::OSCAddressPattern pattern = message.getAddressPattern();
    if (pattern.matches({"/transport/play"})) {
        std::cout << "Play!" << std::endl;
        transport.play(false);
    } else if (pattern.matches({"/transport/stop"})) {
        std::cout << "Stop!" << std::endl;
        transport.stop(false, false);
    } else if (pattern.matches({"/transport/to/seconds"})) {
        if (message.size() < 1 || !message[0].isFloat32()){
            String errorString = "Cannot update transport: Incorrect arguments";
            constructReply(reply, 1, errorString);
            return reply;
        }
        transport.setCurrentPosition(message[0].getFloat32());
    } else if (pattern.matches({"/transport/to"})) {
        if (message.size() < 1 || (!message[0].isFloat32() && !message[0].isFloat64() && !message[0].isInt32())){
            String errorString = "Cannot update transport: Incorrect arguments";
            constructReply(reply, 1, errorString);
            return reply;
        }

        // The behavior of message[2] is overloaded. If it is a float or a double,
        // treat it as a value in seconds. If int, treat it as a measure number.
        double timeSeconds = 0;
        if (getFloatOrDouble(message[0], timeSeconds)) {}
        else if (message[0].isInt32()) {
            int measureNumber = jmax(1, message[0].getInt32()); // index from 1
            timeSeconds = activeCybrEdit->getEdit().tempoSequence.barsBeatsToTime({measureNumber - 1, 0.0 });
        }

        transport.setCurrentPosition(timeSeconds);
    } else if (pattern.matches({"/transport/loop"})) {
        double startSeconds = 0;
        double durationSeconds = 0;
        if (message.size() < 2 || !getFloatOrDouble(message[0], startSeconds) || !getFloatOrDouble(message[1], durationSeconds)) {
            String errorString = "/transport/loop failed - requires loop start and duration";
            constructReply(reply, 1, errorString);
            return reply;
        }

        if (durationSeconds == 0) {
            // To disable looping specify duration of 0
            std::cout << "Looping disabled!" << std::endl;
            transport.looping.setValue(false, nullptr);
            reply.addInt32(0);
            return reply;
        }

        double endSeconds = startSeconds + durationSeconds;

        te::EditTimeRange range = transport.getLoopRange();
        if (range != te::EditTimeRange(startSeconds, endSeconds)) {
            if (range.getStart() != startSeconds) transport.setLoopIn(startSeconds);
            if (range.getEnd() != endSeconds) transport.setLoopOut(endSeconds);
            std::cout << "Looping start|length: " << startSeconds << "|" << endSeconds << std::endl;
        }

        // If looping was previously disabled, setting looping to true seems to
        // move the playhead to the start of the loop. This surprised me, but is
        // okay for now. It is probably not the ideal behavior. Setting the loop
        // point should probably never change playback (currently it probably
        // only changes the playback iff we are not already looping, but if we
        // are looping a different region, playback will be unaffected).
        transport.looping.setValue(true, nullptr);
    }
    reply.addInt32(0);
    return reply;
};

cybr::OSCMessage FluidOscServer::getAudioFileReport(const cybr::OSCMessage& message) {
    cybr::OSCMessage reply("/audiofile/report/reply");

    if (!message.size()) {
        constructReply(reply, 1,  "Cannot get audio file report: missing argument");
        return reply;
    }

    if (!message[0].isString()) {
        constructReply(reply, 1, "Cannot get audio file report: first argument must be a filename string");
        return reply;
    }

    {
        const String filePath = message[0].getString();
        File file;
        // The default filePathResolver checks for an absolute file, then looks
        // in the relative to the edit file directory (using edit.editFileRetriever)
        if (File::isAbsolutePath(filePath))   file = File(filePath);
        if (file == File() && activeCybrEdit) file = activeCybrEdit->getEdit().filePathResolver(filePath);
        if (file == File())                   file = CybrSearchPath(CYBR_SAMPLE).find(filePath);
        if (file == File()) {
            constructReply(reply, 1, "Cannot get audio file report: file not found");
            return reply;
        }

        juce::DynamicObject::Ptr report = new DynamicObject();
        std::unique_ptr<juce::AudioFormatReader> reader;
        reader.reset(te::Engine::getInstance()
            .getAudioFileFormatManager()
            .readFormatManager
            .createReaderFor(file));
        if (reader)
        {
            report->setProperty("format", reader->getFormatName());
            report->setProperty("lengthSeconds", reader->lengthInSamples / reader->sampleRate );
            report->setProperty("lengthSamples", reader->lengthInSamples);
            report->setProperty("sampleRate", reader->sampleRate);
            report->setProperty("absolutePath", file.getFullPathName());
            report->setProperty("givenPath", filePath);
            report->setProperty("numChannels", int(reader->numChannels));
            report->setProperty("bitDepth", int(reader->bitsPerSample));
            constructReply(reply, 0, JSON::toString(report.get(), true));
            return reply;
        } else {
            constructReply(reply, 1, "Cannot get audio file report: failed to read file");
            return reply;
        }
    }

    constructReply(reply, 1, "Cannot get audio file report: unknown error");
    return reply;
}
