// Render the ACTUAL JUCE THUMP editor to a PNG without a visible DAW.
#include "../Source/PluginProcessor.h"
#include "../Source/PluginEditor.h"
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_graphics/juce_graphics.h>
#include <iostream>
#include <memory>

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    ThumpAudioProcessor processor;
    processor.prepareToPlay (44100.0, 512);

    auto* editor = dynamic_cast<ThumpAudioProcessorEditor*> (processor.createEditor());
    if (! editor) { std::cerr << "Editor unavailable\n"; return 1; }
    std::unique_ptr<ThumpAudioProcessorEditor> gui (editor);
    gui->setSize (1600, 1050);
    gui->setVisible (true);

    const auto area = gui->getLocalBounds();
    auto snapshot = gui->createComponentSnapshot (area, true, 1.0f);

    if (! snapshot.isValid()) { std::cerr << "Snapshot invalid\n"; return 2; }
    const auto file = juce::File::getCurrentWorkingDirectory().getChildFile ("THUMP-v0.6-current-GUI.png");
    auto stream = file.createOutputStream();
    if (! stream) { std::cerr << "Failed to open screenshot\n"; return 3; }
    juce::PNGImageFormat png;
    if (! png.writeImageToStream (snapshot, *stream))
    {
       std::cerr << "PNG encode failure\n"; return 4;
    }
    stream->flush();
    std::cout << "ACTUAL JUCE SCREENSHOT: " << file.getFullPathName()
              << " " << snapshot.getWidth() << "x" << snapshot.getHeight() << "\n";
    return 0;
}
