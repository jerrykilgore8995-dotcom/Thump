#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour black  = juce::Colour::fromRGB (16,16,17);
const juce::Colour charcoal = juce::Colour::fromRGB (29,29,30);
const juce::Colour gold = juce::Colour::fromRGB (214,170,80);
const juce::Colour lightGold = juce::Colour::fromRGB (251,212,120);
const juce::Colour deepGold = juce::Colour::fromRGB (116,70,26);
const juce::Colour amber = juce::Colour::fromRGB (255,175,55);
}

ThumpLookAndFeel::ThumpLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, lightGold);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ComboBox::textColourId, lightGold);
    setColour (juce::ComboBox::backgroundColourId, charcoal);
    setColour (juce::ComboBox::outlineColourId, gold);
    setColour (juce::PopupMenu::backgroundColourId, charcoal);
    setColour (juce::PopupMenu::textColourId, lightGold);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, deepGold);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
}

void ThumpLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h,
                                        float pos, float start, float end, juce::Slider&)
{
    const float cx = (float)x + w*.5f, cy = (float)y + h*.46f;
    const float radius = juce::jmin ((float)w*.42f,(float)h*.38f);
    if (radius < 3) return;
    // Surrounding machined dial ticks, end stops and screws; every control is live JUCE.
    for (int i=0; i<=20; ++i)
    {
        float a = start + (end-start)*i/20.f;
        const float dx=std::sin(a), dy=-std::cos(a);
        g.setColour (i%5==0 ? lightGold.withAlpha(.95f) : gold.withAlpha(.55f));
        g.drawLine (cx+dx*radius*1.15f,cy+dy*radius*1.15f,
                    cx+dx*radius*(i%5==0 ? 1.31f : 1.24f),
                    cy+dy*radius*(i%5==0 ? 1.31f : 1.24f), i%5==0 ? 1.8f : .8f);
    }
    g.setColour (juce::Colours::black.withAlpha (.7f));
    g.fillEllipse (cx-radius+2,cy-radius+5,2*radius+2,2*radius+2);
    g.setGradientFill (juce::ColourGradient (juce::Colour::fromRGB(255,228,155),cx-radius,cy-radius,
                                             juce::Colour::fromRGB(118,70,28),cx+radius,cy+radius,false));
    g.fillEllipse (cx-radius,cy-radius,2*radius,2*radius);
    g.setColour (lightGold); g.drawEllipse(cx-radius,cy-radius,2*radius,2*radius,1.3f);
    g.setColour (juce::Colour::fromRGB(59,36,17));
    g.drawEllipse(cx-radius*0.83f,cy-radius*0.83f,1.66f*radius,1.66f*radius,1.6f);
    const float a=start + (end-start)*juce::jlimit(0.f,1.f,pos);
    g.setColour (juce::Colours::black.withAlpha (.85f));
    g.drawLine(cx,cy,cx+std::sin(a)*radius*.78f,cy-std::cos(a)*radius*.78f,3.7f);
    g.setColour (juce::Colour::fromRGB(255,242,186));
    g.drawLine(cx,cy,cx+std::sin(a)*radius*.74f,cy-std::cos(a)*radius*.74f,1.4f);
}

void ThumpLookAndFeel::drawLinearSlider (juce::Graphics& g, int x,int y,int w,int h,float sliderPos,
                                       float,float,juce::Slider::SliderStyle,juce::Slider&)
{
    const float cx = x + w*.5f;
    g.setColour (juce::Colour::fromRGB(3,3,4));
    g.fillRoundedRectangle (cx-5,y+2,10,(float)h-5,3);
    g.setColour (deepGold);
    g.drawRoundedRectangle (cx-5,y+2,10,(float)h-5,3,1);
    const float py = juce::jlimit ((float)y+5, (float)(y+h)-5, sliderPos);
    g.setColour (juce::Colours::black.withAlpha (.75f));
    g.fillRoundedRectangle (cx-22,py-11,46,23,3);
    g.setGradientFill (juce::ColourGradient(lightGold,cx-22,py-9,deepGold,cx+22,py+10,false));
    g.fillRoundedRectangle (cx-21,py-10,42,20,3);
    g.setColour (juce::Colour::fromRGB(84,55,27));
    g.drawLine (cx-16,py,cx+16,py,2.4f);
    g.setColour (lightGold.withAlpha(.7f));
    g.drawRoundedRectangle (cx-21,py-10,42,20,3,1);
}

void ThumpLookAndFeel::drawToggleButton (juce::Graphics& g,juce::ToggleButton& button,bool over,bool down)
{
    const auto b = button.getLocalBounds().toFloat().reduced(2);
    const bool on = button.getToggleState();
    g.setColour (juce::Colours::black); g.fillRoundedRectangle(b,5);
    g.setColour (gold); g.drawRoundedRectangle(b,5,1.2f);
    auto inner=b.reduced(6,5);
    g.setGradientFill(juce::ColourGradient(on?juce::Colour::fromRGB(255,220,132):juce::Colour::fromRGB(66,64,62),
                     inner.getX(),inner.getY(),on?juce::Colour::fromRGB(152,84,20):juce::Colour::fromRGB(33,33,33),
                     inner.getRight(),inner.getBottom(),false));
    g.fillRoundedRectangle(inner,3);
    g.setColour(on ? juce::Colours::black : gold);
    g.setFont(juce::Font(juce::FontOptions(juce::jlimit(10.f,15.f,b.getHeight()*.36f),juce::Font::bold)));
    g.drawFittedText(button.getButtonText(),inner.toNearestInt(),juce::Justification::centred,1);
    if (over || down) { g.setColour(juce::Colours::white.withAlpha(.15f));g.drawRoundedRectangle(inner,3,2); }
}

void ThumpLookAndFeel::drawComboBox (juce::Graphics& g,int w,int h,bool,
                                   int,int,int,int,juce::ComboBox&)
{
    auto r=juce::Rectangle<float>(0,0,(float)w,(float)h).reduced(.5f);
    g.setGradientFill (juce::ColourGradient(juce::Colour::fromRGB(46,42,31),0,0,black,0,(float)h,false));
    g.fillRoundedRectangle(r,5);g.setColour(gold);g.drawRoundedRectangle(r,5,1.3f);
    juce::Path p; p.addTriangle(w-26.f,h*.41f,w-14.f,h*.41f,w-20.f,h*.62f);
    g.setColour(lightGold);g.fillPath(p);
}
juce::Font ThumpLookAndFeel::getComboBoxFont(juce::ComboBox&)
{ return juce::Font(juce::FontOptions(16.f)); }

CompressorModeSwitch::CompressorModeSwitch (juce::AudioProcessorValueTreeState& tree,
                                            const juce::String& id)
{
    parameter = tree.getParameter (id);
    rawValue = tree.getRawParameterValue (id);
    // A custom Component does not implement the JUCE SettableTooltipClient interface.
}
void CompressorModeSwitch::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced(1);
    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (b, 7);
    g.setColour (gold);
    g.drawRoundedRectangle (b, 7, 1.2f);
    const int active = juce::jlimit (0,2, rawValue ? juce::roundToInt(rawValue->load()) : 1);
    const float slotW = b.getWidth()/3.0f;
    for (int i=0;i<3;++i)
    {
        const float cx = b.getX()+slotW*(i+0.5f);
        auto slot = juce::Rectangle<float>(cx-slotW*.4f,b.getY()+4,slotW*.8f,b.getHeight()-9);
        g.setGradientFill (juce::ColourGradient(i==active ? deepGold : charcoal,
            slot.getX(),slot.getY(),black,slot.getX(),slot.getBottom(),false));
        g.fillRoundedRectangle(slot,4);
        if (i==active)
        {
            g.setGradientFill (juce::ColourGradient(lightGold,cx-10,b.getY()+10,
                            deepGold,cx+10,b.getY()+25,false));
            g.fillRoundedRectangle(cx-11,b.getY()+7,22,24,5); // gold toggle detent
            g.setColour (juce::Colour::fromRGB(255,240,185));
            g.drawLine(cx-6,b.getY()+14,cx+6,b.getY()+14,1.5f);
        }
        g.setColour (i==active ? lightGold : gold.withAlpha(.65f));
        g.setFont(juce::Font(juce::FontOptions(11.5f,juce::Font::bold)));
        g.drawText (i==0?"FET":i==1?"VCA":"OPTO", (int)slot.getX(),
                    (int)(b.getBottom()-20),(int)slot.getWidth(),16,
                    juce::Justification::centred,false);
    }
}
void CompressorModeSwitch::mouseDown (const juce::MouseEvent& e)
{
    if (parameter == nullptr) return;
    const int slot = juce::jlimit (0,2, (int)(3.0f*e.position.x/(float)getWidth()));
    parameter->beginChangeGesture();
    parameter->setValueNotifyingHost (parameter->convertTo0to1 ((float)slot));
    parameter->endChangeGesture();
    repaint();
}

ThumpAudioProcessorEditor::ThumpAudioProcessorEditor (ThumpAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setLookAndFeel (&vintage);
    setResizable(true,true);
    setResizeLimits (1160,665,2300,1350);
    setSize (1640,950);
    presetMenu.setLookAndFeel(&vintage);
    presetMenu.addItem("Init / Manual",1);
    for (int i=0;i<8;++i) presetMenu.addItem (thump::factoryPresets()[(size_t)i].name,i+2);
    const int selected = processor.getSelectedPreset();
    presetMenu.setSelectedId (selected>=0 ? selected+2 : 1,juce::dontSendNotification);
    presetMenu.onChange = [this]
    {
        const int selectedId = presetMenu.getSelectedId();
        if (selectedId>=2) processor.applyFactoryPreset (selectedId-2);
    };
    presetMenu.setTooltip("Eight original factory chains, drawn from documented rock / metal techniques");
    addAndMakeVisible (presetMenu);
    positions.push_back ({&presetMenu,{334,89,450,37}});

    addKnob("input","INPUT",109,226,90);
    addKnob("output","OUTPUT",1540,226,90);
    addKnob("xover","CROSSOVER Hz",811,226,68);
    addSwitch("mono","MONO",1407,201,94);

    addSwitch("lowSatOn","ON / OFF",77,364,96);
    addKnob("lowDrive","DRIVE",126,482,95);
    addKnob("lowSatMix","MIX",126,623,86);
    addEQ("low40G","low40Q","40 Hz",247);
    addEQ("low120G","low120Q","120 Hz",329);
    addEQ("low220G","low220Q","220 Hz",411);
    addSwitch("lowCompOn","COMP ON",593,342,112);
    addMode("lowMode",503,395,148,49);
    addKnob("lowThresh","THRESHOLD",510,499,66);
    addKnob("lowMakeup","MAKEUP",638,499,66);
    addKnob("lowAttack","FET ATTACK ms",514,664,63);
    lowFetAttack = sliders.back().get();
    addKnob("lowRelease","FET RELEASE ms",639,664,63);
    lowFetRelease = sliders.back().get();
    addKnob("lowMix","LOW MIX",805,470,90);
    addKnob("highMix","HIGH MIX",805,636,90);

    addSwitch("highSatOn","ON / OFF",916,364,98);
    addKnob("highDrive","DRIVE",965,483,95);
    addKnob("highSatMix","MIX",965,623,86);
    addEQ("high1500G","high1500Q","1.5 kHz",1144);
    addEQ("high5000G","high5000Q","5 kHz",1237);
    addSwitch("highCompOn","COMP ON",1490,342,107);
    addMode("highMode",1383,395,149,49);
    addKnob("highThresh","THRESHOLD",1385,499,66);
    addKnob("highMakeup","MAKEUP",1535,499,66);
    addKnob("highAttack","FET ATTACK ms",1387,664,63);
    highFetAttack = sliders.back().get();
    addKnob("highRelease","FET RELEASE ms",1530,664,63);
    highFetRelease = sliders.back().get();

    addSwitch("limiterOn","LIMIT ON",171,814,112);
    addKnob("limiterInput","DRIVE",408,837,82);
    addKnob("limiterCeiling","CEILING",593,837,82);
    addKnob("limiterRelease","RELEASE",779,837,82);
    addKnob("limiterMix","MIX",962,837,82);
    startTimerHz (25);
}
ThumpAudioProcessorEditor::~ThumpAudioProcessorEditor()
{
    stopTimer(); presetMenu.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}
void ThumpAudioProcessorEditor::addTag (juce::String t,float x,float y,float w,float h,float s)
{ tags.push_back ({t,{x,y,w,h},s}); }
void ThumpAudioProcessorEditor::addKnob (const char* id,const char* label,float cx,float cy,float d)
{
    auto slider=std::make_unique<juce::Slider>();
    auto* ptr=slider.get();
    ptr->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    ptr->setRotaryParameters(juce::degreesToRadians(225.f),juce::degreesToRadians(495.f),true);
    ptr->setTextBoxStyle(juce::Slider::TextBoxBelow,false,78,17);
    ptr->setDoubleClickReturnValue (true,processor.apvts.getParameter(id)->convertFrom0to1 (processor.apvts.getParameter(id)->getDefaultValue()));
    ptr->setTooltip(label);
    addAndMakeVisible(*ptr);
    positions.push_back({ptr,{cx-d*.5f,cy-d*.5f,d,d+20}});
    addTag(label,cx-62,cy-d*.5f-24,124,18,12);
    sliderBindings.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,id,*ptr));
    sliders.push_back(std::move(slider));
}
void ThumpAudioProcessorEditor::addEQ(const char* idG,const char* idQ,const char* label,float x,float yTop)
{
    auto slider=std::make_unique<juce::Slider>();
    auto* ptr=slider.get();ptr->setSliderStyle(juce::Slider::LinearVertical);
    ptr->setTextBoxStyle(juce::Slider::TextBoxBelow,false,64,17);
    ptr->setDoubleClickReturnValue(true,0);
    ptr->setTooltip(juce::String(label)+" +/-12 dB");
    addAndMakeVisible(*ptr);positions.push_back({ptr,{x-28,yTop,56,173}});
    sliderBindings.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,idG,*ptr));
    sliders.push_back(std::move(slider));
    addTag(label,x-42,yTop-31,84,20,13);
    auto q=std::make_unique<juce::Slider>();auto* qp=q.get();
    qp->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qp->setRotaryParameters(juce::degreesToRadians(225.f),juce::degreesToRadians(495.f),true);
    qp->setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,16);
    qp->setDoubleClickReturnValue (true, 1.0);
    qp->setTooltip (juce::String(label) + " curve width / Q trim: 1.00 = stock, <1 broadens, >1 narrows");
    addAndMakeVisible(*qp);positions.push_back({qp,{x-26,641,52,70}});
    sliderBindings.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,idQ,*qp));
    sliders.push_back(std::move(q));
    addTag("Q",x-15,621,30,17,12);
}
void ThumpAudioProcessorEditor::addSwitch(const char* id,const char* label,float x,float y,float w)
{
    auto sw=std::make_unique<juce::ToggleButton>(label);
    auto* ptr=sw.get();addAndMakeVisible(*ptr);
    positions.push_back({ptr,{x,y,w,36}});
    switchBindings.push_back(std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.apvts,id,*ptr));
    switches.push_back(std::move(sw));
}
void ThumpAudioProcessorEditor::addMode (const char* id, float x, float y, float w, float h)
{
    auto mode = std::make_unique<CompressorModeSwitch> (processor.apvts, id);
    auto* ptr = mode.get();
    addAndMakeVisible (*ptr);
    positions.push_back ({ptr,{x,y,w,h}});
    modes.push_back (std::move (mode));
}
void ThumpAudioProcessorEditor::timerCallback()
{
    const bool lowFet  = processor.apvts.getRawParameterValue("lowMode")->load() < .5f;
    const bool highFet = processor.apvts.getRawParameterValue("highMode")->load() < .5f;
    for (auto* slider : {lowFetAttack, lowFetRelease}) if (slider) slider->setEnabled (lowFet);
    for (auto* slider : {highFetAttack, highFetRelease}) if (slider) slider->setEnabled (highFet);
    for (auto& m : modes) m->repaint();
    repaint();
}
void ThumpAudioProcessorEditor::resized()
{
    const float sx=getWidth()/1640.f, sy=getHeight()/950.f;
    for(const auto& p:positions) p.component->setBounds (juce::Rectangle<int>(
        juce::roundToInt(p.rect.getX()*sx),juce::roundToInt(p.rect.getY()*sy),
        juce::roundToInt(p.rect.getWidth()*sx),juce::roundToInt(p.rect.getHeight()*sy)));
}
void ThumpAudioProcessorEditor::drawScrew(juce::Graphics& g,float x,float y)
{
    g.setColour(juce::Colours::black);g.fillEllipse(x-12,y-10,22,22);
    g.setGradientFill(juce::ColourGradient(lightGold,x-9,y-9,deepGold,x+9,y+9,false));
    g.fillEllipse(x-9,y-9,18,18);
    g.setColour(black);g.drawLine(x-5,y+4,x+6,y-5,2);
}
void ThumpAudioProcessorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> r,const juce::String& title,bool rim)
{
    g.setColour(juce::Colours::black.withAlpha(.45f));g.fillRoundedRectangle(r.translated(3,4),12);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(48,39,25),r.getX(),r.getY(),
       juce::Colour::fromRGB(11,12,13),r.getRight(),r.getBottom(),false));
    g.fillRoundedRectangle(r,12);
    g.setColour(rim ? lightGold : gold.withAlpha(.72f));g.drawRoundedRectangle(r.reduced(2),10,2);
    if (title.isNotEmpty())
    {
        auto cap=juce::Rectangle<float>(r.getCentreX()-juce::jmin(r.getWidth()*.42f,200.f),r.getY()-11,
                         juce::jmin(r.getWidth()*.84f,400.f),28);
        g.setColour(black);g.fillRoundedRectangle(cap,10);
        g.setColour(lightGold);g.drawRoundedRectangle(cap,10,1.2f);
        g.setFont(juce::Font(juce::FontOptions(15.f,juce::Font::bold)));
        g.drawFittedText(title,cap.toNearestInt(),juce::Justification::centred,1);
    }
}
void ThumpAudioProcessorEditor::drawVU(juce::Graphics& g,juce::Rectangle<float> r,
                                       const juce::String& text,float levelDb,bool reduction)
{
    g.setColour(juce::Colours::black);g.fillRoundedRectangle(r.expanded(5),6);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(254,223,144),r.getX(),r.getY(),
                     juce::Colour::fromRGB(173,109,41),r.getX(),r.getBottom(),false));
    g.fillRoundedRectangle(r,5);g.setColour(gold);g.drawRoundedRectangle(r,5,2);
    const float start=r.getX()+18,end=r.getRight()-18;
    for(int i=0;i<11;++i)
    {
        const float xx=juce::jmap((float)i,0.f,10.f,start,end);
        g.setColour(i>8?juce::Colours::darkred:black.withAlpha(.75f));
        g.drawLine(xx,r.getY()+19,xx,r.getY()+i%2*5+29,1);
    }
    const float scaled = reduction?juce::jlimit(0.f,1.f,levelDb/24.f)
                         :juce::jlimit(0.f,1.f,(levelDb+40.f)/44.f);
    const float xx=juce::jmap(scaled,start,end);
    g.setColour(juce::Colour::fromRGB(98,30,21));g.drawLine(r.getCentreX(),r.getBottom()-10,xx,r.getY()+28,2);
    g.setColour(juce::Colour::fromRGB(35,24,14));
    g.setFont(juce::Font(juce::FontOptions(15.f,juce::Font::bold)));
    g.drawFittedText(text,juce::Rectangle<int>((int)r.getX(),(int)r.getBottom()-33,(int)r.getWidth(),25),
                     juce::Justification::centred,1);
}
void ThumpAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(black);
    g.addTransform(juce::AffineTransform::scale(getWidth()/1640.f,getHeight()/950.f));
    // 19-inch machined gold-rim black faceplate.
    g.setGradientFill(juce::ColourGradient(lightGold,0,0,deepGold,1640,950,false));
    g.fillRoundedRectangle(7,7,1626,936,17);
    g.setColour(black);g.fillRoundedRectangle(20,19,1600,910,12);
    for (auto pt: {juce::Point<float>{24,24},{1616,24},{24,925},{1616,925}}) drawScrew(g,pt.x,pt.y);
    // Header and ID: permanent engraved lettering (not an overlay image).
    g.setColour(juce::Colour::fromRGB(52,41,27));g.fillRoundedRectangle(36,30,1568,107,13);
    g.setColour(gold);g.drawRoundedRectangle(36,30,1568,107,13,1.4f);
    g.setFont(juce::Font(juce::FontOptions(60.f,juce::Font::bold)));
    g.setColour(lightGold);g.drawFittedText("T H U M P",juce::Rectangle<int>(650,37,370,60),juce::Justification::centred,1);
    g.setFont(juce::Font(juce::FontOptions(12.f,juce::Font::bold)));
    g.drawFittedText("ULTIMATE BASS MIXING TOOL",juce::Rectangle<int>(650,96,370,19),juce::Justification::centred,1);
    g.drawFittedText("ANALOG-STYLE  •  DUAL BAND  •  FET / VCA / OPTO",juce::Rectangle<int>(55,48,530,24),juce::Justification::centredLeft,1);
    g.drawFittedText("FACTORY PRESETS",juce::Rectangle<int>(178,95,154,22),juce::Justification::centredRight,1);

    drawPanel(g,{34,159,1570,120},"INPUT / CROSSOVER / OUTPUT",false);
    drawVU(g,{225,186,270,75},"INPUT",processor.inMeterDb.load());
    drawVU(g,{1128,186,235,75},"OUTPUT",processor.outMeterDb.load());
    g.setColour(gold.withAlpha(.75f));
    g.drawLine(532,235,695,235,1.6f);g.drawLine(939,235,1090,235,1.6f);
    g.drawLine(696,219,720,197,1.5f);g.drawLine(696,251,720,272,1.5f);
    g.setFont(juce::Font(juce::FontOptions(12.f)));
    g.drawFittedText("LOW   /   HIGH",juce::Rectangle<int>(712,171,190,20),juce::Justification::centred,1);

    drawPanel(g,{31,313,678,405},"LOW — THUMP & BOOM",true);
    drawPanel(g,{721,313,154,405},"BLEND",true);
    drawPanel(g,{886,313,725,405},"HIGH — GRIND & DEFINITION",true);
    drawPanel(g,{47,386,150,319},"SATURATION");
    drawPanel(g,{200,386,242,319},"3-BAND EQ");
    drawPanel(g,{451,386,247,319},"3-MODE SUB COMP");
    drawPanel(g,{894,386,144,319},"TS DRIVE");
    drawPanel(g,{1046,386,250,319},"2-BAND EQ");
    drawPanel(g,{1304,386,299,319},"3-MODE HIGH COMP");
    drawVU(g,{475,560,190,44},"LOW GR",processor.lowReduction.load(),true);
    drawVU(g,{1330,560,244,44},"HIGH GR",processor.highReduction.load(),true);
    g.setColour(gold);
    g.setFont(juce::Font(juce::FontOptions(13.f)));
    g.drawFittedText("FET 4:1  /  VCA 4:1  /  OPTO 3:1",juce::Rectangle<int>(463,386,244,15),juce::Justification::centred,1);
    g.drawFittedText("FET 4:1  /  VCA 4:1  /  OPTO 3:1",juce::Rectangle<int>(1324,386,270,15),juce::Justification::centred,1);
    g.drawFittedText("LOW",juce::Rectangle<int>(762,375,90,20),juce::Justification::centred,1);
    g.drawFittedText("HIGH",juce::Rectangle<int>(762,544,90,20),juce::Justification::centred,1);
    drawPanel(g,{31,750,1578,174},"TRANSPARENT LOOKAHEAD GLUE LIMITER",true);
    drawVU(g,{1156,806,380,89},"LIMITER GAIN REDUCTION",processor.limitReduction.load(),true);
    g.setColour(gold.withAlpha(.7f));
    g.setFont(juce::Font(juce::FontOptions(11.f)));
    g.drawFittedText("2x OVERSAMPLING  •  PHASE-COHERENT LR4  •  1ms LOOKAHEAD",juce::Rectangle<int>(425,901,700,17),juce::Justification::centred,1);
    for(const auto& tag:tags)
    {
        g.setColour(lightGold);
        g.setFont(juce::Font(juce::FontOptions(tag.size,juce::Font::bold)));
        g.drawFittedText(tag.text,tag.rect.toNearestInt(),juce::Justification::centred,1);
    }
}
