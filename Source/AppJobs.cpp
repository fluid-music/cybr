/*
  ==============================================================================

    AppJobs.cpp
    Created: 19 Nov 2019 9:13:17pm
    Author:  Charles Holbrow

  ==============================================================================
*/

#include "AppJobs.h"

using namespace juce;

void AppJobs::play(CybrEdit& cybrEdit) {
    CybrEdit* newCybrEdit = copyCybrEditForPlayback(cybrEdit);
    te::Edit& newEdit = newCybrEdit->getEdit();
    
    newEdit.getTransport().play(false);
    
    add(newCybrEdit);
    Timer::callAfterDelay((int)ceil(newEdit.getLength() * 1000.), [this, newCybrEdit]() {
        remove(newCybrEdit);
    });
}

bool AppJobs::add(CybrEdit* cybrEdit) {
    if (playingEdits.contains(cybrEdit)) return false;

    playingEdits.add(cybrEdit);
    sendChangeMessage();
    return true;
}

bool AppJobs::remove(const CybrEdit* cybrEdit) {
    if (!playingEdits.contains(cybrEdit)) return false;

    playingEdits.removeObject(cybrEdit);
    sendChangeMessage();
    return true;
}

void AppJobs::setRunForever(bool newFlag) {
    bool changed = (newFlag != runForever);
    runForever = newFlag;
    if (changed) sendChangeMessage();
}
