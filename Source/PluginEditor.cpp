#include "PluginEditor.h"
#include <cmath>

namespace
{
const juce::Colour black  = juce::Colour::fromRGB (13,15,17);
const juce::Colour charcoal = juce::Colour::fromRGB (31,33,35);
const juce::Colour gold = juce::Colour::fromRGB (166,132,79);
const juce::Colour lightGold = juce::Colour::fromRGB (211,185,130);
const juce::Colour deepGold = juce::Colour::fromRGB (86,67,40);
const juce::Colour amber = juce::Colour::fromRGB (225,148,63);
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
    const float cx = x + w * .5f, cy = y + h * .45f;
    const float r = juce::jmin (w*.355f, h*.347f);
    if (r < 4.f) return;
    const auto ring = juce::Rectangle<float>(cx-r*1.04f,cy-r*1.04f,2.08f*r,2.08f*r);
    g.setColour(juce::Colours::black.withAlpha(.86f));
    g.fillEllipse(ring.translated(0,2.f));
    g.setColour(juce::Colour::fromRGB(77,71,61));
    g.drawEllipse(ring,1.0f);
    // Very fine scale marks engraved into the metal rather than cartoon tick blocks.
    for (int i=0;i<=20;++i)
    {
        const float a=start+(end-start)*i/20.f;
        const float sn=std::sin(a), cs=-std::cos(a);
        const float outer=r*1.34f, inner=r*(i%5==0?1.18f:1.23f);
        g.setColour(i%5==0?juce::Colour::fromRGB(196,177,136):juce::Colour::fromRGB(105,103,94));
        g.drawLine(cx+sn*inner,cy+cs*inner,cx+sn*outer,cy+cs*outer,i%5==0?1.15f:.7f);
    }
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(101,83,56),cx-r,cy-r,
                   juce::Colour::fromRGB(25,27,27),cx+r,cy+r,false));
    g.fillEllipse(cx-r,cy-r,2*r,2*r);
    g.setColour(juce::Colour::fromRGB(136,111,70));
    g.drawEllipse(cx-r,cy-r,2*r,2*r,1.5f);
    const float ir=r*.74f;
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(183,161,115),cx-ir,cy-ir,
                   juce::Colour::fromRGB(78,65,46),cx+ir,cy+ir,false));
    g.fillEllipse(cx-ir,cy-ir,2*ir,2*ir);
    // Hairline turning marks in an actual milled knob face.
    g.setColour(juce::Colour::fromRGB(211,189,145).withAlpha(.22f));
    g.drawEllipse(cx-ir*.9f,cy-ir*.9f,ir*1.8f,ir*1.8f,.8f);
    g.setColour(juce::Colour::fromRGB(47,43,34));
    g.drawEllipse(cx-ir*.93f,cy-ir*.93f,ir*1.86f,ir*1.86f,1.f);
    const float a=start+(end-start)*juce::jlimit(0.f,1.f,pos);
    const float sn=std::sin(a), cs=-std::cos(a);
    g.setColour(juce::Colour::fromRGB(22,21,20));
    g.drawLine(cx+sn*ir*.25f,cy+cs*ir*.25f,cx+sn*ir*.9f,cy+cs*ir*.9f,2.7f);
    g.setColour(juce::Colour::fromRGB(238,224,192));
    g.drawLine(cx+sn*ir*.45f,cy+cs*ir*.45f,cx+sn*ir*.85f,cy+cs*ir*.85f,1.35f);
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
    g.fillRoundedRectangle (cx-16,py-8,32,17,2);
    g.setGradientFill (juce::ColourGradient(lightGold,cx-16,py-8,deepGold,cx+16,py+8,false));
    g.fillRoundedRectangle (cx-16,py-8,32,16,2);
    g.setColour (juce::Colour::fromRGB(84,55,27));
    g.drawLine (cx-12,py,cx+12,py,1.5f);
    g.setColour (lightGold.withAlpha(.7f));
    g.drawRoundedRectangle (cx-16,py-8,32,16,2,1);
}

void ThumpLookAndFeel::drawToggleButton (juce::Graphics& g,juce::ToggleButton& button,bool over,bool down)
{
    const auto b=button.getLocalBounds().toFloat().reduced(2.f);
    const bool on=button.getToggleState();
    g.setColour(juce::Colour::fromRGB(5,7,8)); g.fillRoundedRectangle(b,3.f);
    g.setColour(juce::Colour::fromRGB(90,85,72)); g.drawRoundedRectangle(b,3.f,1.f);
    const auto inner=b.reduced(4.f);
    g.setGradientFill(juce::ColourGradient(on?juce::Colour::fromRGB(138,114,71):juce::Colour::fromRGB(46,48,48),
                      inner.getX(),inner.getY(),on?juce::Colour::fromRGB(77,62,37):juce::Colour::fromRGB(24,26,27),
                      inner.getX(),inner.getBottom(),false));
    g.fillRoundedRectangle(inner,2.f);
    if (on) { g.setColour(amber);g.fillEllipse(inner.getX()+5,inner.getCentreY()-2.5f,5.f,5.f); }
    g.setColour(on ? juce::Colour::fromRGB(241,223,183) : juce::Colour::fromRGB(151,151,146));
    g.setFont(juce::Font(juce::FontOptions(juce::jlimit(10.f,13.f,b.getHeight()*.34f),juce::Font::bold)));
    g.drawFittedText(button.getButtonText(),inner.toNearestInt(),juce::Justification::centred,1);
    if(over||down){g.setColour(juce::Colours::white.withAlpha(.07f));g.drawRoundedRectangle(inner,2.f,1.f);}
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
    setResizeLimits (1200,645,2000,1075);
    setSize (1520,817); // Fits a 21-inch 1080p monitor, including the DAW window chrome.
    presetMenu.setLookAndFeel(&vintage);
    presetMenu.addItem("Init / Manual",1);
    for (int i=0;i<8;++i) presetMenu.addItem(thump::factoryPresets()[(size_t)i].name,i+2);
    const int selected=processor.getSelectedPreset();
    presetMenu.setSelectedId(selected>=0?selected+2:1,juce::dontSendNotification);
    presetMenu.onChange=[this] { const int v=presetMenu.getSelectedId(); if (v>=2) processor.applyFactoryPreset(v-2); };
    presetMenu.setTooltip("Rock and metal bass starting points");
    addAndMakeVisible(presetMenu);
    positions.push_back({&presetMenu,{1080,42,450,35}});

    // GLOBAL SECTION: input/output have no wasted, decorative VU meters.
    addKnob("input","INPUT dB",105,173,72);
    addKnob("xover","CROSSOVER Hz",310,173,72);
    addKnob("lowMix","LOW BLEND",555,173,72);
    addKnob("highMix","HIGH BLEND",795,173,72);
    addSwitch("mono","MONO / CENTER",1022,157,146);
    addKnob("output","OUTPUT dB",1477,173,72);

    // LOW  // a spacious, independent instrument strip.
    addSwitch("lowSatOn","SAT ON",62,294,115);
    addKnob("lowDrive","DRIVE",119,407,75);
    addKnob("lowSatMix","WET / DRY",119,579,70);
    addEQ("low40G","low40Q","40 Hz",246,356);
    addEQ("low120G","low120Q","120 Hz",320,356);
    addEQ("low220G","low220Q","220 Hz",394,356);
    addSwitch("lowCompOn","COMP ON",468,295,105);
    addMode("lowMode",580,295,179,40);
    addKnob("lowThresh","THRESHOLD",519,514,72);
    addKnob("lowMakeup","MAKEUP",704,514,72);
    addKnob("lowAttack","FET ATTACK",520,615,63);
    lowFetAttack=sliders.back().get();
    addKnob("lowRelease","FET RELEASE",704,615,63);
    lowFetRelease=sliders.back().get();

    // HIGH // separate grind and high-band compression, equally sized GR meter.
    addSwitch("highSatOn","SAT ON",850,294,115);
    addKnob("highDrive","DISTORTION",906,407,75);
    addKnob("highSatMix","WET / DRY",906,579,70);
    addEQ("high1500G","high1500Q","1.5 kHz",1050,356);
    addEQ("high5000G","high5000Q","5 kHz",1143,356);
    addSwitch("highCompOn","COMP ON",1228,295,105);
    addMode("highMode",1340,295,182,40);
    addKnob("highThresh","THRESHOLD",1277,514,72);
    addKnob("highMakeup","MAKEUP",1475,514,72);
    addKnob("highAttack","FET ATTACK",1277,615,63);
    highFetAttack=sliders.back().get();
    addKnob("highRelease","FET RELEASE",1475,615,63);
    highFetRelease=sliders.back().get();

    // Last-stage clean peak management with a large readable GR display.
    addSwitch("limiterOn","LIMIT ON",66,755,110);
    addKnob("limiterInput","INPUT",330,789,70);
    addKnob("limiterCeiling","CEILING",522,789,70);
    addKnob("limiterRelease","RELEASE",716,789,70);
    addKnob("limiterMix","WET / DRY",906,789,70);
    startTimerHz(25);
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
    addAndMakeVisible(*ptr);positions.push_back({ptr,{x-28,yTop,56,168}});
    sliderBindings.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,idG,*ptr));
    sliders.push_back(std::move(slider));
    addTag(label,x-42,yTop-31,84,20,13);
    auto q=std::make_unique<juce::Slider>();auto* qp=q.get();
    qp->setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    qp->setRotaryParameters(juce::degreesToRadians(225.f),juce::degreesToRadians(495.f),true);
    qp->setTextBoxStyle(juce::Slider::TextBoxBelow,false,65,16);
    qp->setDoubleClickReturnValue (true, 1.0);
    qp->setTooltip (juce::String(label) + " curve width / Q trim: 1.00 = stock, <1 broadens, >1 narrows");
    addAndMakeVisible(*qp);positions.push_back({qp,{x-26,590,52,70}});
    sliderBindings.push_back(std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.apvts,idQ,*qp));
    sliders.push_back(std::move(q));
    addTag("Q",x-15,572,30,17,11);
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
    const float sx=getWidth()/1600.f, sy=getHeight()/860.f;
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
void ThumpAudioProcessorEditor::drawPanel(juce::Graphics& g,juce::Rectangle<float> r,
                                         const juce::String& title,bool rim)
{
    g.setColour(juce::Colour::fromRGB(4,6,7));
    g.fillRoundedRectangle(r.translated(1.5f,2.f),5.f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(34,36,38),r.getX(),r.getY(),
                        juce::Colour::fromRGB(20,22,23),r.getX(),r.getBottom(),false));
    g.fillRoundedRectangle(r,5.f);
    g.setColour(rim?juce::Colour::fromRGB(114,92,60):juce::Colour::fromRGB(62,61,58));
    g.drawRoundedRectangle(r.reduced(.5f),5.f,rim?1.5f:1.f);
    if (title.isNotEmpty())
    {
        g.setColour(juce::Colour::fromRGB(188,177,153));
        g.setFont(juce::Font(juce::FontOptions(14.f,juce::Font::bold)));
        g.drawFittedText(title,juce::Rectangle<int>((int)r.getX()+13,(int)r.getY()+9,
                         (int)r.getWidth()-25,20),juce::Justification::centredLeft,1);
        g.setColour(juce::Colour::fromRGB(69,67,62));
        g.drawLine(r.getX()+13,r.getY()+34,r.getRight()-13,r.getY()+34,.8f);
    }
}
void ThumpAudioProcessorEditor::drawVU(juce::Graphics& g,juce::Rectangle<float> r,
                                      const juce::String& text,float reductionDb,bool)
{
    // Real moving-coil-style GR meter: one prominent display on EACH comp and limiter.
    const auto surround=r.expanded(6.f);
    g.setColour(juce::Colours::black);
    g.fillRoundedRectangle(surround,4.f);
    g.setColour(juce::Colour::fromRGB(105,86,58));
    g.drawRoundedRectangle(surround,4.f,1.4f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(224,197,145),r.getX(),r.getY(),
                       juce::Colour::fromRGB(148,124,82),r.getX(),r.getBottom(),false));
    g.fillRoundedRectangle(r,2.5f);
    // Damped/aged lens edge, no fake lightbulb neon or oversized graphics.
    g.setColour(juce::Colour::fromRGB(63,50,32));
    g.drawRoundedRectangle(r.reduced(.6f),2.5f,1.f);
    const float x0=r.getX()+18.f, x1=r.getRight()-18.f;
    const float scaleY=r.getY()+25.f;
    const float vals[5] = {20.f,12.f,6.f,3.f,0.f};
    g.setFont(juce::Font(juce::FontOptions(10.5f,juce::Font::bold)));
    for(int i=0;i<5;++i)
    {
        const float x=juce::jmap((float)i,0.f,4.f,x0,x1);
        g.setColour(juce::Colour::fromRGB(61,51,36));
        g.drawLine(x,scaleY+12,x,scaleY+18,1.f);
        g.drawText(juce::String((int)vals[i]),(int)(x-16), (int)scaleY-8,32,15,juce::Justification::centred,false);
    }
    const float t=std::sqrt(juce::jlimit(0.f,1.f,reductionDb/20.f));
    const float nx=juce::jmap(t,x1,x0);
    const auto origin=juce::Point<float>(r.getCentreX(),r.getBottom()+14.f);
    g.saveState();
    g.reduceClipRegion(r.toNearestInt());
    g.setColour(juce::Colour::fromRGB(71,31,24));
    g.drawLine(origin.x,origin.y,nx,scaleY+18.f,1.8f);
    g.restoreState();
    g.setColour(juce::Colour::fromRGB(247,237,205));
    g.setFont(juce::Font(juce::FontOptions(12.5f,juce::Font::bold)));
    g.drawText(text,r.withTrimmedTop(r.getHeight()-25).toNearestInt(),juce::Justification::centred,false);
}
void ThumpAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour::fromRGB(8,9,10));
    g.addTransform(juce::AffineTransform::scale(getWidth()/1600.f,getHeight()/860.f));
    // Realistic industrial 19-inch faceplate: anodized black aluminium and narrow brass edges.
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(105,83,52),0,0,
                        juce::Colour::fromRGB(43,38,31),1600,860,false));
    g.fillRoundedRectangle(5,5,1590,850,9.f);
    g.setColour(juce::Colour::fromRGB(8,10,12));
    g.fillRoundedRectangle(13,13,1574,834,6.f);
    g.setGradientFill(juce::ColourGradient(juce::Colour::fromRGB(37,40,42),0,15,
                       juce::Colour::fromRGB(20,22,24),0,844,false));
    g.fillRoundedRectangle(17,17,1566,826,5.f);
    // A very restrained brushed finish. No shiny cartoony borders.
    for(int y=20;y<842;y+=3)
    {
        g.setColour(juce::Colours::white.withAlpha((y%9==2)?.018f:.008f));
        g.drawHorizontalLine(y,25.f,1575.f);
    }
    for(auto pt:{juce::Point<float>{25,26},{1573,26},{25,835},{1573,835}}) drawScrew(g,pt.x,pt.y);
    g.setColour(juce::Colour::fromRGB(18,20,21));g.fillRoundedRectangle(38,29,1524,65,3.f);
    g.setColour(juce::Colour::fromRGB(98,79,54));g.drawRoundedRectangle(38,29,1524,65,3.f,1.f);
    g.setColour(juce::Colour::fromRGB(224,192,132));
    g.setFont(juce::Font(juce::FontOptions(43.f,juce::Font::bold)));
    g.drawText("THUMP",juce::Rectangle<int>(61,36,264,44),juce::Justification::centredLeft);
    g.setFont(juce::Font(juce::FontOptions(11.5f,juce::Font::bold)));
    g.setColour(juce::Colour::fromRGB(173,166,145));
    g.drawText("ANALOG-STYLE DUAL-BAND BASS PROCESSOR",juce::Rectangle<int>(343,52,535,17),juce::Justification::centredLeft);
    g.drawText("PRESET",juce::Rectangle<int>(1011,52,58,18),juce::Justification::centredRight);
    drawPanel(g,{36,114,1528,108},"");
    drawPanel(g,{36,246,742,436},"",true);
    drawPanel(g,{822,246,742,436},"",true);
    // Subtle inner equipment regions that don't steal space from controls.
    drawPanel(g,{50,283,135,386},"");
    drawPanel(g,{197,283,239,386},"");
    drawPanel(g,{448,283,316,386},"");
    drawPanel(g,{838,283,135,386},"");
    drawPanel(g,{985,283,231,386},"");
    drawPanel(g,{1228,283,322,386},"");
    auto minor=[&](juce::String label,juce::Rectangle<int> box)
    {
        g.setColour(juce::Colour::fromRGB(176,168,148));
        g.setFont(juce::Font(juce::FontOptions(12.5f,juce::Font::bold)));
        g.drawFittedText(label,box,juce::Justification::centred,1);
    };
    minor("LOW   //   SATURATION",{44,260,147,19});
    minor("LOW EQ",{200,260,228,19});
    minor("COMPRESSOR",{451,260,305,19});
    minor("HIGH   //   DRIVE",{832,260,148,19});
    minor("HIGH EQ",{986,260,227,19});
    minor("COMPRESSOR",{1232,260,312,19});
    drawVU(g,{477,351,258,91},"LOW  •  GR",processor.lowReduction.load(),true);
    drawVU(g,{1258,351,267,91},"HIGH  •  GR",processor.highReduction.load(),true);
    g.setColour(juce::Colour::fromRGB(85,75,61));
    g.drawLine(799,255,799,674,1.f);
    drawPanel(g,{36,707,1528,128},"",true);
    minor("GLUE LIMITER",{51,712,151,18});
    drawVU(g,{1184,737,338,77},"LIMITER  •  GR",processor.limitReduction.load(),true);
    for(const auto& tag:tags)
    {
        g.setColour(juce::Colour::fromRGB(204,195,170));
        g.setFont(juce::Font(juce::FontOptions(tag.size,juce::Font::bold)));
        g.drawFittedText(tag.text,tag.rect.toNearestInt(),juce::Justification::centred,1);
    }
}
