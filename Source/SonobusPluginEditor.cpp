// SPDX-License-Identifier: GPLv3-or-later WITH Appstore-exception
// Copyright (C) 2020 Jesse Chappell
// Customizado para Master Musica 

#include "SonobusPluginProcessor.h"
#include "SonobusPluginEditor.h"

#include "BeatToggleGrid.h"

#include "PeersContainerView.h"
#include "WaveformTransportComponent.h"
#include "RandomSentenceGenerator.h"
#include "SonoUtility.h"
#include "SonobusTypes.h"
#include "ChannelGroupsView.h"
#include "MonitorDelayView.h"
#include "ChatView.h"
#include "SoundboardView.h"
#include "AutoUpdater.h"
#include "LatencyMatchView.h"
#include "SuggestNewGroupView.h"
#include "SonoCallOutBox.h"
#include <sstream>

#if JUCE_ANDROID
#include "juce_core/native/juce_BasicNativeHeaders.h"
#include "juce_core/juce_core.h"
#include "juce_core/native/juce_JNIHelpers_android.h"
#endif

enum {
    PeriodicUpdateTimerId = 0,
    CheckForNewVersionTimerId
};

enum {
    nameTextColourId = 0x1002830,
    selectedColourId = 0x1002840,
    separatorColourId = 0x1002850,
};

enum {
    PeerLayoutRadioGroupId = 1
};

#define SONOBUS_SCHEME "studiomaster"

using namespace SonoAudio;


class SonobusAudioProcessorEditor::PatchMatrixView : public Component, public BeatToggleGridDelegate
{
public:
    PatchMatrixView(SonobusAudioProcessor& p) : Component(), processor(p) {
        
        grid = std::make_unique<BeatToggleGrid>();
        grid->setDelegate(this);
        
        addAndMakeVisible(grid.get());
        
        updateGrid();
    }
    virtual ~PatchMatrixView() {
    }
    
    bool beatToggleGridPressed(BeatToggleGrid* grid, int index, const MouseEvent & event) override { 
        int count = processor.getNumberRemotePeers();
        if (count == 0) return false;
        
        int src = index/count;
        int dest = index % count;        
        bool currval = processor.getPatchMatrixValue(src, dest);
        valonpress = currval;
        
        processor.setPatchMatrixValue(src, dest, !currval);

        updateGrid();

        return true;
    }
    
    bool beatToggleGridMoved(BeatToggleGrid* grid, int fromIndex, int toIndex, const MouseEvent & event) override {
        int count = processor.getNumberRemotePeers();
        if (count == 0) return false;
        
        int tosrc = toIndex/count;
        int todest = toIndex % count;        
        bool currval = processor.getPatchMatrixValue(tosrc, todest);
        
        processor.setPatchMatrixValue(tosrc, todest, !valonpress);
        
        updateGrid();
        return true;
    }
    
    bool beatToggleGridReleased(BeatToggleGrid* grid, int index, const MouseEvent & event) override {
        return true;
    }
    
    void updateGrid()
    {
        int count = processor.getNumberRemotePeers();
        if ( count*count != grid->getItems()) {
            updateGridLayout();
        }
        
        for (int i=0; i < count; ++i) {
            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                
                grid->setSelected(processor.getPatchMatrixValue(i, j), item);
                if (i == j) {
                    grid->setAccented(processor.getPatchMatrixValue(i, j), item);                    
                }
            }            
        }
        
        grid->refreshGrid(false);
        repaint();
    }
    
    Rectangle<int> getPreferredBounds() const
    {
        int count = processor.getNumberRemotePeers();
        int labwidth = 30;
        int labheight= 18;
        int buttwidth = 60;
        int buttheight = 44;

        Rectangle<int> prefsize;
        prefsize.setX(0);
        prefsize.setY(0);
        prefsize.setWidth(count * buttwidth + labwidth);
        prefsize.setHeight(count * buttheight + labheight);

        return prefsize;
    }

    
    void updateGridLayout() {

        int count = processor.getNumberRemotePeers();
        
        
        topbox.items.clear();
        topbox.flexDirection = FlexBox::Direction::row;    
        leftbox.items.clear();
        leftbox.flexDirection = FlexBox::Direction::column;    

        while (leftLabels.size() < count) {
            Label * leftlab = new Label("lab", String::formatted("%d", leftLabels.size() + 1));
            leftlab->setJustificationType(Justification::centred);
            leftLabels.add(leftlab);

            Label * toplab = new Label("lab", String::formatted("%d", topLabels.size() + 1));
            toplab->setJustificationType(Justification::centred);
            topLabels.add(toplab);

            addAndMakeVisible( leftLabels.getUnchecked(leftLabels.size()-1));
            addAndMakeVisible( topLabels.getUnchecked(topLabels.size()-1));
        }
        
        for (int i=0; i < leftLabels.size(); ++i) {
            leftLabels.getUnchecked(i)->setVisible(i < count);
            topLabels.getUnchecked(i)->setVisible(i < count);
        }
        
        int labwidth = 30;
        int labheight= 18;
        int buttheight = 36;
        
        topbox.items.add(FlexItem(labwidth, labheight));

        grid->setItems(count * count);
        grid->setSegments(count);
        
        for (int i=0; i < count; ++i) {
            grid->setSegmentSize(count, i);
        }
        
        grid->refreshGrid(true);
        
        for (int i=0; i < count; ++i) {

            for (int j=0; j < count; ++j) {
                int item = i*count + j;
                grid->setLabel(String::formatted("%d>%d", i+1, j+1), item);                
            }
            
            topbox.items.add(FlexItem(20, labheight, *topLabels.getUnchecked(i)).withMargin(2).withFlex(1));
            leftbox.items.add(FlexItem(20, labheight, *leftLabels.getUnchecked(i)).withMargin(2).withFlex(1));            
        }
        

        middlebox.items.clear();
        middlebox.flexDirection = FlexBox::Direction::row;    
        middlebox.items.add(FlexItem(labwidth, labheight, leftbox).withMargin(2).withFlex(0));
        middlebox.items.add(FlexItem(labwidth, buttheight, *grid).withMargin(2).withFlex(1));
        

        
        mainbox.items.clear();
        mainbox.flexDirection = FlexBox::Direction::column;    
        mainbox.items.add(FlexItem(labwidth*2, labheight, topbox).withMargin(2).withFlex(0).withMaxHeight(30));
        mainbox.items.add(FlexItem(labwidth*2, buttheight, middlebox).withMargin(2).withFlex(1));
        
        resized();
    }
    
    void paint (Graphics& g) override
    {

    }

    void resized() override
    {
        Component::resized();
        
        mainbox.performLayout(getLocalBounds());    
    }
    
    std::unique_ptr<BeatToggleGrid> grid;
    OwnedArray<Label> leftLabels;
    OwnedArray<Label> topLabels;

    FlexBox mainbox;
    FlexBox middlebox;
    FlexBox leftbox;
    FlexBox topbox;

    
    bool valonpress = false;
    
    SonobusAudioProcessor & processor;
};

// ==============================================================================
// INJEÇÃO DE CORES MASTER MUSICA: Textos e Labels
// ==============================================================================
static void configLabel(Label *label, bool val)
{
    if (val) {
        label->setFont(12);
        // Master Musica Off-White
        label->setColour(Label::textColourId, Colour(0x90fcfdfc));
        label->setJustificationType(Justification::centred);        
    }
    else {
        label->setFont(14);
        // Master Musica Off-White
        label->setColour(Label::textColourId, Colour(0x90fcfdfc));
        label->setJustificationType(Justification::centredLeft);
    }
}

static void configServerLabel(Label *label)
{
    label->setFont(14);
    // Master Musica Off-White
    label->setColour(Label::textColourId, Colour(0x90fcfdfc));
    label->setJustificationType(Justification::centredRight);        
}
// ==============================================================================


void SonobusAudioProcessorEditor::configEditor(TextEditor *editor, bool passwd)
{
    editor->addListener(this);
    if (passwd)  {
        editor->setIndents(8, 6);        
    } else {
        editor->setIndents(8, 8);
    }
}


//==============================================================================
void SonobusAudioProcessorEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    
    // MODIFICAÇÃO MASTER MUSICA: Fundo Azul Escuro oficial (#253262)
    g.fillAll (juce::Colour (0xff253262));
}

void SonobusAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

    Component::resized();
 
#if JUCE_IOS || JUCE_ANDROID
    int narrowthresh = 480; //520;
    int rnarrowthresh = 360; //520;
#else
    int narrowthresh = 690 ; // 644; // 588; //520;
    int rnarrowthresh = 380; // 588; //520;
#endif


    if (mChatView->isVisible()) {
        narrowthresh += mChatView->getWidth();
    }

    if (mSoundboardView->isVisible()) {
       narrowthresh += mSoundboardView->getWidth();
    }

    bool nownarrow = getWidth() < narrowthresh;
    if (nownarrow != isNarrow) {
        isNarrow = nownarrow;
        mPeerContainer->setNarrowMode(isNarrow);
        mInputChannelsContainer->setNarrowMode(isNarrow);
        updateLayout();
    }

    bool nowrnarrow = getWidth() < rnarrowthresh;
    if (nowrnarrow != isReallyNarrow) {
        isReallyNarrow = nowrnarrow;
        updateLayout();
    }

   
    
    DBG("RESIZED to " << getWidth() << " " << getHeight());
    
    auto mainBounds = getLocalBounds();
    const auto menuHeight = getLookAndFeel().getDefaultMenuBarHeight();

    processor.setLastPluginBounds(mainBounds); // just relative
    

    if (mMenuBar) {
        auto menuBounds = mainBounds.removeFromTop(menuHeight);
        mMenuBar->setBounds(menuBounds);
    }

    int chatwidth = processor.getLastChatWidth();

    mChatOverlay = mainBounds.getWidth() - chatwidth < 340;

    mIgnoreResize = true; // important!
 
    mChatView->setBounds(getLocalBounds().removeFromRight(chatwidth));

    if (mChatView->isVisible() || mAboutToShowChat) {
        if (!isNarrow || !mChatOverlay) {
            // take it off
            mChatView->setBounds(mainBounds.removeFromRight(chatwidth));
        }
    }

    int soundboardwidth = processor.getLastSoundboardWidth();
    mSoundboardView->setBounds(getLocalBounds().removeFromRight(soundboardwidth));

    if (mSoundboardView->isVisible() || mAboutToShowSoundboard) {
        if (!isNarrow) {
            mSoundboardView->setBounds(mainBounds.removeFromRight(soundboardwidth));
        }
    }

    mIgnoreResize = false;


    mTopLevelContainer->setBounds(getLocalBounds());

    mainBox.performLayout(mainBounds);


    mChatEdgeResizer->setBounds(mChatView->getLocalBounds().withWidth(5));

    mSoundboardEdgeResizer->setBounds(mSoundboardView->getLocalBounds().withWidth(5));


    int inchantargwidth = mMainViewport->getWidth() - 10;

    if (mInputChannelsContainer->getEstimatedWidth() != inchantargwidth && mInputChannelsContainer->isVisible()) {
        mInputChannelsContainer->setEstimatedWidth(inchantargwidth);
        mInputChannelsContainer->updateLayout(false);
    }

    Rectangle<int> peersminbounds = mPeerContainer->getMinimumContentBounds();
    Rectangle<int> inmixminbounds = mInputChannelsContainer->getMinimumContentBounds();

    Rectangle<int> inmixactualbounds = Rectangle<int>(0,0,0,0);

    if (mInputChannelsContainer->isVisible()) {
        inmixactualbounds = Rectangle<int>(0, 0,
                                           std::max(inmixminbounds.getWidth(), inchantargwidth),
                                           inmixminbounds.getHeight() + 5);

        mInputChannelsContainer->setBounds(inmixactualbounds);
    }

    int vgap = inmixactualbounds.getHeight() > 0 ?  6 : 0;

    mPeerContainer->setBounds(Rectangle<int>(0, inmixactualbounds.getBottom() + vgap, std::max(peersminbounds.getWidth(), mMainViewport->getWidth() - 10), std::max(peersminbounds.getHeight() + 5, mMainViewport->getHeight() - inmixactualbounds.getHeight() - vgap)));

    Rectangle<int> totbounds = mPeerContainer->getBounds().getUnion(inmixactualbounds);
    //totbounds.setHeight(totbounds.getHeight());


    auto viewpos = mMainViewport->getViewPosition();

    mMainContainer->setBounds(totbounds);

    mMainViewport->setViewPosition(viewpos);


#if JUCE_IOS || JUCE_ANDROID
    mSetupAudioButton->setSize(150, 1);
#else
    mSetupAudioButton->setSize(150, 50);    
#endif
    
    mSetupAudioButton->setCentrePosition(mMainViewport->getX() + 0.5*mMainViewport->getWidth(), mMainViewport->getY() + inmixactualbounds.getHeight() + 45);
    
    mMainMessageLabel->setBounds(mMainViewport->getX() + 10, mSetupAudioButton->getBottom() + 10, mMainViewport->getRight() - mMainViewport->getX() - 20, jmin(120, mMainViewport->getBottom() - (mSetupAudioButton->getBottom() + 10)));
    
    auto metbgbounds = Rectangle<int>(mMetEnableButton->getX(), mMetEnableButton->getY(), mMetConfigButton->getRight() - mMetEnableButton->getX(),  mMetEnableButton->getHeight()).expanded(2, 2);
    mMetButtonBg->setRectangle (metbgbounds.toFloat());

    // MODIFICAÇÃO MASTER MUSICA: Fundo da área do metrônomo (#96938b com 60% opacidade)
    mMetButtonBg->setFill(juce::Colour(0x6096938b));

    //auto grouptextbounds = Rectangle<int>(mMainPeerLabel->getX(), mMainGroupImage->getY(), mMainUserLabel->getRight() - mMainPeerLabel->getX(),  mMainGroupImage->getHeight()).expanded(2, 2);
    //auto grouptextbounds = Rectangle<int>(mMainPeerLabel->getX(), mMainGroupImage->getY(), mMainUserLabel->getRight() - mMainPeerLabel->getX(),  mMainUserLabel->getBottom() - mMainGroupImage->getY());
    auto grouptextbounds = Rectangle<int>(mMainPeerLabel->getX(), mPeerLayoutFullButton->getY(), mMainUserLabel->getRight() - mMainPeerLabel->getX(),  mPeerLayoutFullButton->getHeight());
    mMainLinkButton->setBounds(grouptextbounds);

    auto triwidth = 12;
    auto triheight = 8;
    //auto linkarrowrect = Rectangle<int>(mMainLinkButton->getRight() - triwidth - 4, mMainLinkButton->getBottom() - mMainLinkButton->getHeight()/2 - triheight + 2, triwidth, triheight);
    auto linkarrowrect = Rectangle<int>(mMainLinkButton->getRight() - triwidth - 4, mMainLinkButton->getBottom() - triheight - 4, triwidth, triheight);
    mMainLinkArrow->setTransformToFit(linkarrowrect.toFloat(), RectanglePlacement::stretchToFit);


    const auto precwidth = 20;
    auto peerrecbounds = Rectangle<int>(mMainLinkButton->getRight() - precwidth - 4, mMainLinkButton->getY() + mMainLinkButton->getHeight()/2 - precwidth/2, precwidth,  precwidth);
    mPeerRecImage->setTransformToFit(peerrecbounds.toFloat(), RectanglePlacement::fillDestination);


    mDragDropBg->setRectangle (getLocalBounds().toFloat());


    auto filebgbounds = Rectangle<int>(mPlayButton->getX(), mWaveformThumbnail->getY(), 
                                       mDismissTransportButton->getRight() - mPlayButton->getX(),  
                                       mDismissTransportButton->getBottom() - mWaveformThumbnail->getY()).expanded(4, 6);
    mFileAreaBg->setRectangle (filebgbounds.toFloat());
    
    // MODIFICAÇÃO MASTER MUSICA: Fundo da área do player de arquivo (#96938b com 60% opacidade)
    mFileAreaBg->setFill(juce::Colour(0x6096938b));
    
    // connect component stuff
    if (mConnectView) {
        mConnectView->setBounds(getLocalBounds());
    }

    mConnectionTimeLabel->setBounds(mConnectButton->getBounds().removeFromBottom(16));
    
    if (mRecordingButton) {
        mFileRecordingLabel->setBounds(mRecordingButton->getBounds().removeFromBottom(14).translated(0, 1));
    }

    mDrySlider->setMouseDragSensitivity(jmax(128, mDrySlider->getWidth()));
    mOutGainSlider->setMouseDragSensitivity(jmax(128, mOutGainSlider->getWidth()));
    //mInGainSlider->setMouseDragSensitivity(jmax(128, mInGainSlider->getWidth()));

    mDryLabel->setBounds(mDrySlider->getBounds().removeFromTop(17).removeFromLeft(mDrySlider->getWidth() - mDrySlider->getTextBoxWidth() + 3).translated(4, -2));
    //mInGainLabel->setBounds(mInGainSlider->getBounds().removeFromTop(14).removeFromLeft(mInGainSlider->getWidth() - mInGainSlider->getTextBoxWidth() + 3).translated(4, 0));
    mOutGainLabel->setBounds(mOutGainSlider->getBounds().removeFromTop(17).removeFromLeft(mOutGainSlider->getWidth() - mOutGainSlider->getTextBoxWidth() + 3).translated(4, -2));


    
    Component* dw = this; 
    
    if (auto * callout = dynamic_cast<CallOutBox*>(effectsCalloutBox.get())) {
        callout->updatePosition(dw->getLocalArea(nullptr, mEffectsButton->getScreenBounds()), dw->getLocalBounds());
    }


    updateSliderSnap();

}


void SonobusAudioProcessorEditor::updateLayout()
{
    int minKnobWidth = 50;
    int minSliderWidth = 50;
    int minPannerWidth = 40;
    int minitemheight = 36;
    int knobitemheight = 80;
    int minpassheight = 30;
    int setitemheight = 36;
    int minButtonWidth = 90;
    int inmeterwidth = 22 ;
    int outmeterwidth = 22 ;
    int iconheight = 18; // 24;
    int iconwidth = iconheight;
    int knoblabelheight = 18;
    int toolwidth = 44;
    int mintoolwidth = 38;
#if JUCE_IOS || JUCE_ANDROID
    // make the button heights a bit more for touchscreen purposes
    iconheight = 22; // 24;
    minitemheight = 44;
    knobitemheight = 90;
    minpassheight = 38;
#endif

    // adjust max meterwidth if channel count > 2
    if (processor.getActiveSendChannelCount() > 2 && processor.getSendChannels() == 0) {
        inmeterwidth = 5 * processor.getActiveSendChannelCount();
    }
    if (processor.getMainBusNumOutputChannels() > 2) {
        outmeterwidth = 5 * processor.getMainBusNumOutputChannels();
    }

    int mutew = 52;
    int inmixw = 74;
    int choicew = inmixw + mutew + 3;

    inGainBox.items.clear();
    inGainBox.flexDirection = FlexBox::Direction::row;
    //inGainBox.items.add(FlexItem(minKnobWidth, minitemheight, *mInGainSlider).withMargin(0).withFlex(1));
    //inGainBox.items.add(FlexItem(choicew, minitemheight, *mSendChannelsLabel).withMargin(0).withFlex(0.5));
    inGainBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1));
    inGainBox.items.add(FlexItem(choicew, minitemheight, *mSendChannelsChoice).withMargin(0).withFlex(1).withMaxWidth(160));
    inGainBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1));

    dryBox.items.clear();
    dryBox.flexDirection = FlexBox::Direction::row;
    dryBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    dryBox.items.add(FlexItem(minKnobWidth, minitemheight, *mDrySlider).withMargin(0).withFlex(1));

    
    outBox.items.clear();
    outBox.flexDirection = FlexBox::Direction::column;
    outBox.items.add(FlexItem(minKnobWidth, minitemheight, *mOutGainSlider).withMargin(0).withFlex(1)); //.withAlignSelf(FlexItem::AlignSelf::center));

    inMeterBox.items.clear();
    inMeterBox.flexDirection = FlexBox::Direction::column;
    inMeterBox.items.add(FlexItem(inmeterwidth, minitemheight, *inputMeter).withMargin(0).withFlex(1).withMaxWidth(inmeterwidth).withAlignSelf(FlexItem::AlignSelf::center));

    mainMeterBox.items.clear();
    mainMeterBox.flexDirection = FlexBox::Direction::column;
    mainMeterBox.items.add(FlexItem(outmeterwidth, minitemheight, *outputMeter).withMargin(0).withFlex(1).withMaxWidth(outmeterwidth).withAlignSelf(FlexItem::AlignSelf::center));
    
    inputPannerBox.items.clear();
    inputPannerBox.flexDirection = FlexBox::Direction::row;
    inputPannerBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.2));
    inputPannerBox.items.add(FlexItem(inmixw, minitemheight, *mInMixerButton).withMargin(0).withFlex(1).withMaxWidth(160 - mutew - 3));
    inputPannerBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
    inputPannerBox.items.add(FlexItem(mutew, minitemheight, *mInMuteButton).withMargin(0).withFlex(0.0));
    //inputPannerBox.items.add(FlexItem(mutew, minitemheight, *mInEffectsButton).withMargin(0).withFlex(0));
    inputPannerBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.2));

    inputLeftBox.items.clear();
    inputLeftBox.flexDirection = FlexBox::Direction::column;
    inputLeftBox.items.add(FlexItem(choicew + 4, minitemheight, inGainBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    inputLeftBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    inputLeftBox.items.add(FlexItem(mutew+inmixw + 10, minitemheight, inputPannerBox).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

    inputButtonBox.items.clear();
    inputButtonBox.flexDirection = FlexBox::Direction::row;
    inputButtonBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0.1));
    inputButtonBox.items.add(FlexItem(mutew, minitemheight, *mInSoloButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    inputButtonBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.2));
    //inputButtonBox.items.add(FlexItem(mutew, minitemheight, *mMonDelayButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    inputButtonBox.items.add(FlexItem(3, 4));
    inputButtonBox.items.add(FlexItem(toolwidth, minitemheight, *mSoundboardButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    inputButtonBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
    inputButtonBox.items.add(FlexItem(toolwidth, minitemheight, *mChatButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    inputButtonBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));

    inputRightBox.items.clear();
    inputRightBox.flexDirection = FlexBox::Direction::column;
    inputRightBox.items.add(FlexItem(minSliderWidth, minitemheight, dryBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    inputRightBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
    inputRightBox.items.add(FlexItem(2*mutew + 10, minitemheight, inputButtonBox).withMargin(0).withFlex(1)); //.withMaxWidth(maxPannerWidth));

    inputMainBox.items.clear();
    inputMainBox.flexDirection = FlexBox::Direction::row;
    inputMainBox.items.add(FlexItem(jmax(choicew + 4, 2*mutew + 16), minitemheight + minitemheight + 4, inputLeftBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    inputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    inputMainBox.items.add(FlexItem(inmeterwidth, minitemheight, inMeterBox).withMargin(0).withFlex(0));
    inputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    inputMainBox.items.add(FlexItem(jmax(minSliderWidth, 2*mutew + 16), minitemheight + minitemheight +4, inputRightBox).withMargin(0).withFlex(1)) ; //.withMaxWidth(isNarrow ? 160 : 120));

    int minmainw = jmax(minPannerWidth, 2*mutew + 8) + inmeterwidth + jmax(minPannerWidth, 2*mutew + 8);
    
    outputMainBox.items.clear();
    outputMainBox.flexDirection = FlexBox::Direction::row;
    outputMainBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(toolwidth, minitemheight, *mBufferMinButton).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(toolwidth, minitemheight, *mEffectsButton).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(minSliderWidth, minitemheight, outBox).withMargin(0).withFlex(1)); //.withMaxWidth(isNarrow ? 160 : 120));
    outputMainBox.items.add(FlexItem(4, 6).withMargin(0).withFlex(0));
    outputMainBox.items.add(FlexItem(outmeterwidth, minitemheight, mainMeterBox).withMargin(0).withFlex(0));
    if (isNarrow) {
        outputMainBox.items.add(FlexItem(21, 6).withMargin(0).withFlex(0));
    } else {
        outputMainBox.items.add(FlexItem(16, 6).withMargin(0).withFlex(0));        
    }

    
    paramsBox.items.clear();
    paramsBox.flexDirection = FlexBox::Direction::column;
    paramsBox.items.add(FlexItem(minmainw, minitemheight + minitemheight + 4, inputMainBox).withMargin(2).withFlex(0));



    mainGroupBox.items.clear();
    mainGroupBox.flexDirection = FlexBox::Direction::row;
    //mainGroupBox.items.add(FlexItem(24, minitemheight/2, *mMainPeerLabel).withMargin(0).withFlex(0));
    mainGroupBox.items.add(FlexItem(iconwidth, iconheight, *mMainGroupImage).withMargin(0).withFlex(0));
    mainGroupBox.items.add(FlexItem(2, 4));
    mainGroupBox.items.add(FlexItem(minButtonWidth, minitemheight/2 - 0, *mMainGroupLabel).withMargin(0).withFlex(1));


    mainUserBox.items.clear();
    mainUserBox.flexDirection = FlexBox::Direction::row;
    //mainUserBox.items.add(FlexItem(24, 4));
    mainUserBox.items.add(FlexItem(iconwidth, iconheight, *mMainPersonImage).withMargin(0).withFlex(0));
    mainUserBox.items.add(FlexItem(2, 4));
    mainUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2 - 0, *mMainUserLabel).withMargin(0).withFlex(1));

    mainGroupUserBox.items.clear();
    mainGroupUserBox.flexDirection = FlexBox::Direction::column;
    //mainGroupUserBox.items.add(FlexItem(2, 2));
    mainGroupUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2 - 4, mainGroupBox).withMargin(0).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(2, 2));
    mainGroupUserBox.items.add(FlexItem(minButtonWidth, minitemheight/2 - 4, mainUserBox).withMargin(0).withFlex(0));
    mainGroupUserBox.items.add(FlexItem(2, 2));

    mainGroupLayoutBox.items.clear();
    mainGroupLayoutBox.flexDirection = FlexBox::Direction::row;
    mainGroupLayoutBox.items.add(FlexItem(1, 4));
    mainGroupLayoutBox.items.add(FlexItem(toolwidth, minitemheight, *mPeerLayoutMinimalButton).withMargin(0).withFlex(0));
    mainGroupLayoutBox.items.add(FlexItem(toolwidth, minitemheight, *mPeerLayoutFullButton).withMargin(0).withFlex(0));
    //mainGroupLayoutBox.items.add(FlexItem(3, 4));
    //mainGroupLayoutBox.items.add(FlexItem(toolwidth, minitemheight, *mChatButton).withMargin(0).withFlex(0) ); //.withMaxWidth(maxPannerWidth));
    mainGroupLayoutBox.items.add(FlexItem(4, 4));
    mainGroupLayoutBox.items.add(FlexItem(24, minitemheight, *mMainPeerLabel).withMargin(0).withFlex(0));
    mainGroupLayoutBox.items.add(FlexItem(minButtonWidth, minitemheight - 5, mainGroupUserBox).withMargin(0).withFlex(1));
    if (processor.isConnectedToServer() && processor.getCurrentJoinedGroup().isNotEmpty() && !isReallyNarrow) {
        mainGroupLayoutBox.items.add(FlexItem(3, 4).withMargin(1).withFlex(0.0));
        mainGroupLayoutBox.items.add(FlexItem(toolwidth, minitemheight, *mVideoButton).withMargin(0).withFlex(0));
        mVideoButton->setVisible(true);
    }
    else {
        mVideoButton->setVisible(false);
    }



    connectBox.items.clear();
    connectBox.flexDirection = FlexBox::Direction::column;
    connectBox.items.add(FlexItem(4, 2));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight, titleBox).withMargin(0).withFlex(1));
    connectBox.items.add(FlexItem(4, 4));
    connectBox.items.add(FlexItem(minButtonWidth, minitemheight, mainGroupLayoutBox).withMargin(0).withFlex(1));
    connectBox.items.add(FlexItem(4, 2));

    titleBox.items.clear();
    titleBox.flexDirection = FlexBox::Direction::row;
    titleBox.items.add(FlexItem(2, 20).withMargin(1).withFlex(0));
    titleBox.items.add(FlexItem(minitemheight - 8, minitemheight - 12, *mSettingsButton).withMargin(1).withMaxWidth(44).withFlex(0));
    titleBox.items.add(FlexItem(86, 20, *mTitleLabel).withMargin(1).withFlex(0));
    if (iaaConnected) {
        titleBox.items.add(FlexItem(4, 4).withMargin(1).withFlex(0.0));
        titleBox.items.add(FlexItem(minitemheight, minitemheight, *mIAAHostButton).withMargin(0).withFlex(0));
    }
    titleBox.items.add(FlexItem(4, 4).withMargin(1).withFlex(0.0));
    titleBox.items.add(FlexItem(minButtonWidth, minitemheight, *mConnectButton).withMargin(0).withFlex(1));
    if (processor.isConnectedToServer() && processor.getCurrentJoinedGroup().isNotEmpty()) {
        titleBox.items.add(FlexItem(3, 4).withMargin(1).withFlex(0.0));
        titleBox.items.add(FlexItem(toolwidth, minitemheight, *mAltConnectButton).withMargin(0).withFlex(0));
        mAltConnectButton->setVisible(true);
    }
    else {
        mAltConnectButton->setVisible(false);
    }

    
    middleBox.items.clear();
    int midboxminheight =  minitemheight * 2 + 8 ; //(knobitemheight + 4);
    int midboxminwidth = 296;
    
    if (isNarrow) {
        middleBox.flexDirection = FlexBox::Direction::column;
        middleBox.items.add(FlexItem(150, minitemheight*2 + 8, connectBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(5, 2));
        middleBox.items.add(FlexItem(100, minitemheight + minitemheight + 6, paramsBox).withMargin(2).withFlex(0));
        middleBox.items.add(FlexItem(6, 2));

        //midboxminheight = (minitemheight*1.5 + minitemheight * 2 + 20);
        midboxminwidth = 190;

        midboxminheight = 0;
        for (auto & item : middleBox.items) {
            midboxminheight += item.minHeight;
        }

    } else {
        middleBox.flexDirection = FlexBox::Direction::row;        
        middleBox.items.add(FlexItem(280, minitemheight*2+8 , connectBox).withMargin(2).withFlex(1.5).withMaxWidth(470));
        middleBox.items.add(FlexItem(1, 1).withMaxWidth(3).withFlex(0.5));
        middleBox.items.add(FlexItem(minKnobWidth*4 + inmeterwidth*2, minitemheight + minitemheight, paramsBox).withMargin(2).withFlex(4));

        midboxminwidth = 0;
        for (auto & item : middleBox.items) {
            midboxminwidth += item.minWidth;
        }

    }



    toolbarTextBox.items.clear();
    toolbarTextBox.flexDirection = FlexBox::Direction::column;
    toolbarTextBox.items.add(FlexItem(40, minitemheight/2, *mMainStatusLabel).withMargin(0).withFlex(1));


    metVolBox.items.clear();
    metVolBox.flexDirection = FlexBox::Direction::column;
    metVolBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mMetLevelSliderLabel).withMargin(0).withFlex(0));
    metVolBox.items.add(FlexItem(minKnobWidth, minitemheight, *mMetLevelSlider).withMargin(0).withFlex(1));

    metTempoBox.items.clear();
    metTempoBox.flexDirection = FlexBox::Direction::column;
    metTempoBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mMetTempoSliderLabel).withMargin(0).withFlex(0));
    metTempoBox.items.add(FlexItem(minKnobWidth, minitemheight, *mMetTempoSlider).withMargin(0).withFlex(1));

    metSendSyncBox.items.clear();
    metSendSyncBox.flexDirection = FlexBox::Direction::row;
    if (!JUCEApplicationBase::isStandaloneApp()) {
        metSendSyncBox.items.add(FlexItem(40, minitemheight, *mMetSyncButton).withMargin(0).withFlex(1));
        metSendSyncBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0));
    }
    metSendSyncBox.items.add(FlexItem(40, minitemheight, *mMetSyncFileButton).withMargin(0).withFlex(1));


    metSendBox.items.clear();
    metSendBox.flexDirection = FlexBox::Direction::column;
    metSendBox.items.add(FlexItem(80, minitemheight, metSendSyncBox).withMargin(0).withFlex(0));
    metSendBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(1));
    metSendBox.items.add(FlexItem(60, minitemheight, *mMetSendButton).withMargin(0).withFlex(0));

    metBox.items.clear();
    metBox.flexDirection = FlexBox::Direction::row;
    metBox.items.add(FlexItem(minKnobWidth, minitemheight, metTempoBox).withMargin(0).withFlex(1));
    metBox.items.add(FlexItem(minKnobWidth, minitemheight, metVolBox).withMargin(0).withFlex(1));
    metBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
    metBox.items.add(FlexItem(80, minitemheight, metSendBox).withMargin(2).withFlex(1));

    
    // effects

    reverbSizeBox.items.clear();
    reverbSizeBox.flexDirection = FlexBox::Direction::column;
    reverbSizeBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbSizeLabel).withMargin(0).withFlex(0));
    reverbSizeBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbSizeSlider).withMargin(0).withFlex(1));

    reverbLevelBox.items.clear();
    reverbLevelBox.flexDirection = FlexBox::Direction::column;
    reverbLevelBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbLevelLabel).withMargin(0).withFlex(0));
    reverbLevelBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbLevelSlider).withMargin(0).withFlex(1));

    reverbDampBox.items.clear();
    reverbDampBox.flexDirection = FlexBox::Direction::column;
    reverbDampBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbDampingLabel).withMargin(0).withFlex(0));
    reverbDampBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbDampingSlider).withMargin(0).withFlex(1));

    reverbPreDelayBox.items.clear();
    reverbPreDelayBox.flexDirection = FlexBox::Direction::column;
    reverbPreDelayBox.items.add(FlexItem(minKnobWidth, knoblabelheight, *mReverbPreDelayLabel).withMargin(0).withFlex(0));
    reverbPreDelayBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbPreDelaySlider).withMargin(0).withFlex(1));

    
    reverbCheckBox.items.clear();
    reverbCheckBox.flexDirection = FlexBox::Direction::row;
    reverbCheckBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
    reverbCheckBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbEnabledButton).withMargin(0).withFlex(0));
    reverbCheckBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbTitleLabel).withMargin(0).withFlex(2));
    reverbCheckBox.items.add(FlexItem(minKnobWidth, minitemheight, *mReverbModelChoice).withMargin(0).withFlex(1));
    reverbCheckBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));


    
    reverbKnobBox.items.clear();
    reverbKnobBox.flexDirection = FlexBox::Direction::row;
    reverbKnobBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbPreDelayBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbLevelBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbSizeBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(minKnobWidth, minitemheight, reverbDampBox).withMargin(0).withFlex(1));
    reverbKnobBox.items.add(FlexItem(5, 5).withMargin(0).withFlex(0));

    reverbBox.items.clear();
    reverbBox.flexDirection = FlexBox::Direction::column;
    reverbBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
    reverbBox.items.add(FlexItem(100, minitemheight, reverbCheckBox).withMargin(0).withFlex(0));
    reverbBox.items.add(FlexItem(6, 5).withMargin(0).withFlex(0));
    reverbBox.items.add(FlexItem(100, knoblabelheight + minitemheight, reverbKnobBox).withMargin(0).withFlex(1));

    
    effectsBox.items.clear();
    effectsBox.flexDirection = FlexBox::Direction::column;
    effectsBox.items.add(FlexItem(minKnobWidth, knoblabelheight + minitemheight + 10, reverbBox).withMargin(0).withFlex(1));
    
    
    toolbarBox.items.clear();
    toolbarBox.flexDirection = FlexBox::Direction::row;
    toolbarBox.items.add(FlexItem(7, 6).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(toolwidth, minitemheight, *mMainMuteButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1).withMaxWidth(8));
    toolbarBox.items.add(FlexItem(toolwidth, minitemheight, *mMainRecvMuteButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1).withMaxWidth(8));
    toolbarBox.items.add(FlexItem(toolwidth, minitemheight, *mMainPushToTalkButton).withMargin(0).withFlex(0));
    toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1));
    toolbarBox.items.add(FlexItem(toolwidth, minitemheight, *mMetEnableButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight+2).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(36, minitemheight, *mMetConfigButton).withMargin(0).withFlex(0).withMaxHeight(minitemheight+2).withAlignSelf(FlexItem::AlignSelf::center));
    toolbarBox.items.add(FlexItem(2, 5).withMargin(0).withFlex(0.1));

   
#if JUCE_IOS || JUCE_ANDROID
#else
    if (processor.getCurrentJoinedGroup().isEmpty() && processor.getNumberRemotePeers() > 1) {
        toolbarBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0.2));
        toolbarBox.items.add(FlexItem(60, minitemheight, *mPatchbayButton).withMargin(2).withFlex(0.5).withMaxWidth(120));
    }
#endif

    if (mRecordingButton) {

        transportVBox.items.clear();
        transportVBox.flexDirection = FlexBox::Direction::column;

        transportBox.items.clear();
        transportBox.flexDirection = FlexBox::Direction::row;

        transportWaveBox.items.clear();
        transportWaveBox.flexDirection = FlexBox::Direction::row;

        transportWaveBox.items.add(FlexItem(isNarrow ? 11 : 5, 6).withMargin(0).withFlex(0));
        transportWaveBox.items.add(FlexItem(100, minitemheight, *mWaveformThumbnail).withMargin(0).withFlex(1));
        transportWaveBox.items.add(FlexItem(isNarrow ? 17 : 3, 6).withMargin(0).withFlex(0));

        transportBox.items.add(FlexItem(9, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(mintoolwidth, minitemheight, *mPlayButton).withMargin(0).withFlex(1).withMaxWidth(toolwidth));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(mintoolwidth, minitemheight, *mSkipBackButton).withMargin(0).withFlex(1).withMaxWidth(toolwidth));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(mintoolwidth, minitemheight, *mLoopButton).withMargin(0).withFlex(1).withMaxWidth(toolwidth));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(40, minitemheight, *mFileMenuButton).withMargin(0).withFlex(0));
        
        if ( ! isNarrow) {
            transportBox.items.add(FlexItem(3, 6, transportWaveBox).withMargin(0).withFlex(1));  

            transportVBox.items.add(FlexItem(3, minitemheight, transportBox).withMargin(0).withFlex(0));              
        }
        else {
            transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0.1));

            transportVBox.items.add(FlexItem(3, minitemheight, transportWaveBox).withMargin(0).withFlex(0));  
            transportVBox.items.add(FlexItem(3, 3).withMargin(0).withFlex(0));
            transportVBox.items.add(FlexItem(3, minitemheight, transportBox).withMargin(0).withFlex(0));  
        }
            
        transportBox.items.add(FlexItem(minKnobWidth, minitemheight, *mPlaybackSlider).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(mintoolwidth, minitemheight, *mFileSendAudioButton).withMargin(0).withFlex(1).withMaxWidth(toolwidth));
        transportBox.items.add(FlexItem(3, 6).withMargin(0).withFlex(0));
        transportBox.items.add(FlexItem(mintoolwidth, minitemheight, *mDismissTransportButton).withMargin(0).withFlex(1).withMaxWidth(toolwidth));
#if JUCE_IOS || JUCE_ANDROID
        transportBox.items.add(FlexItem(6, 6).withMargin(1).withFlex(0));
#else
        transportBox.items.add(FlexItem(14, 6).withMargin(1).withFlex(0));        
#endif

        toolbarBox.items.add(FlexItem(toolwidth, minitemheight, *mRecordingButton).withMargin(0).withFlex(0));
        toolbarBox.items.add(FlexItem(2, 6).withMargin(0).withFlex(0.1).withMaxWidth(6));
        toolbarBox.items.add(FlexItem(toolwidth, minitemheight, *mFileBrowseButton).withMargin(0).withFlex(0));
    }

    if (!isNarrow) {
        toolbarBox.items.add(FlexItem(1, 5).withMargin(0).withFlex(0.1));
        toolbarBox.items.add(FlexItem(120, minitemheight, outputMainBox).withMargin(0).withFlex(1).withMaxWidth(390));
        toolbarBox.items.add(FlexItem(6, 6).withMargin(0).withFlex(0));
    }
    else {    
        toolbarBox.items.add(FlexItem(14, 6).withMargin(0).withFlex(0));
    }
    

    
    int minheight = 0;
    
    mainBox.items.clear();
    mainBox.flexDirection = FlexBox::Direction::column;    
    mainBox.items.add(FlexItem(midboxminwidth, midboxminheight, middleBox).withMargin(3).withFlex(0)); minheight += midboxminheight;
    mainBox.items.add(FlexItem(120, 50, *mMainViewport).withMargin(3).withFlex(3)); minheight += 50 + 6;
    mainBox.items.add(FlexItem(10, 2).withFlex(0)); minheight += 2;

    if (!mCurrentAudioFile.isEmpty()) {
        mainBox.items.add(FlexItem(4, 5).withMargin(0).withFlex(0)); minheight += 5;
        if (isNarrow) {
            mainBox.items.add(FlexItem(100, 2*minitemheight + 3, transportVBox).withMargin(2).withFlex(0)); minheight += 2*minitemheight + 3;
        }
        else {
            mainBox.items.add(FlexItem(100, minitemheight, transportVBox).withMargin(2).withFlex(0)); minheight += minitemheight;
        }
        mainBox.items.add(FlexItem(4, 11).withMargin(0).withFlex(0)); minheight += 11;
    }
    
    if (isNarrow) {
        mainBox.items.add(FlexItem(100, minitemheight + 4, outputMainBox).withMargin(0).withFlex(0)); 
        mainBox.items.add(FlexItem(4, 4).withMargin(0).withFlex(0));
        minheight += minitemheight + 8;
    }
    mainBox.items.add(FlexItem(100, minitemheight + 4, toolbarBox).withMargin(0).withFlex(0)); minheight += minitemheight + 10;
    mainBox.items.add(FlexItem(10, 6).withFlex(0));
        

}

void SonobusAudioProcessorEditor::showPopTip(const String & message, int timeoutMs, Component * target, int maxwidth)
{
    if (!popTip) {
        popTip = std::make_unique<BubbleMessageComponent>();
    }
    
    popTip->setAllowedPlacement(BubbleMessageComponent::BubblePlacement::above | BubbleMessageComponent::BubblePlacement::below);
    
    if (target) {
        if (auto * parent = target->findParentComponentOfClass<AudioProcessorEditor>()) {
            parent->addChildComponent (popTip.get());
            parent->toFront(false);
        }
        else {
            addChildComponent(popTip.get());            
        }
    }
    else 
    {
        addChildComponent(popTip.get());
    }
    
    
    AttributedString text(message);
    text.setJustification (Justification::centred);
    text.setColour (findColour (TextButton::textColourOffId));
    text.setFont(Font(12));
    if (target) {
        popTip->showAt(target, text, timeoutMs);
    }
    else {
        Rectangle<int> topbox(getWidth()/2 - maxwidth/2, 0, maxwidth, 2);
        popTip->showAt(topbox, text, timeoutMs);
    }

    popTip->toFront(false);

    //Timer::callAfterDelay(200, [message]() {
    //    AccessibilityHandler::postAnnouncement(message, AccessibilityHandler::AnnouncementPriority::high);
    //});
}


void SonobusAudioProcessorEditor::showFilePopupMenu(Component * source)
{
    Array<GenericItemChooserItem> items;
    items.add(GenericItemChooserItem(TRANS("Trim to New")));
#if JUCE_IOS || JUCE_ANDROID
    items.add(GenericItemChooserItem(TRANS("Share File")));    
#else
    items.add(GenericItemChooserItem(TRANS("Reveal File")));
#endif
    
    Component* dw = this; 
    
    
    Rectangle<int> bounds =  dw->getLocalArea(nullptr, source->getScreenBounds());
    
    GenericItemChooser::launchPopupChooser(items, bounds, dw, this, 1000);
}




void SonobusAudioProcessorEditor::genericItemChooserSelected(GenericItemChooser *comp, int index)
{
    int choosertag = comp->getTag();

    if (choosertag == 1000) {
        if (index == 0) {
            // trim to new
            trimCurrentAudioFile(false);
        }
        else if (index == 1) {
#if JUCE_IOS || JUCE_ANDROID
            // share
            Array<URL> urlarray;
            SafePointer<SonobusAudioProcessorEditor> safeThis(this);
            urlarray.add(mCurrentAudioFile);
            mScopedShareBox = ContentSharer::shareFilesScoped(urlarray, [safeThis](bool result, const String& msg){ DBG("url share returned " << (int)result << " : " << msg);
                safeThis->mScopedShareBox = {};
            });
#else
            // reveal
            if (mCurrentAudioFile.getFileName().isNotEmpty()) {
                mCurrentAudioFile.getLocalFile().revealToUser();
                return; // needed in case we are already gone (weird windows issue)
            }
#endif

        }
    }
    
    if (CallOutBox* const cb = comp->findParentComponentOfClass<CallOutBox>()) {
        cb->dismiss();
    }

}


void SonobusAudioProcessorEditor::changeListenerCallback (ChangeBroadcaster* source)
{
    if (source == mWaveformThumbnail.get()) {
        loadAudioFromURL(URL (mWaveformThumbnail->getLastDroppedFile()));
    } else if (source == &(processor.getTransportSource())) {
        updateTransportState();
    }
}

class SonobusAudioProcessorEditor::TrimFileJob : public ThreadPoolJob
{
public:
    TrimFileJob(SonobusAudioProcessorEditor * parent_, const String & file_, double startPos_, double lenSecs_, bool replace_)
    : ThreadPoolJob("TrimFilesJob"), parent(parent_), file(file_), startPos(startPos_), lenSecs(lenSecs_), replaceExisting(replace_) {}
    
    JobStatus runJob ()
    {
        // AppState * app = AppState::getInstance();
        DBG("Starting trim file job");
        
        File sourcefile = File(file);
        File outputfile = sourcefile.getParentDirectory().getNonexistentChildFile(sourcefile.getFileNameWithoutExtension() + "-trim", sourcefile.getFileExtension());
        std::unique_ptr<AudioFormatReader> reader;
        reader.reset(parent->processor.getFormatManager().createReaderFor (sourcefile));
        
        bool success = false;
        
        if (reader != nullptr) {
            
            String pathname = outputfile.getFullPathName();

            std::unique_ptr<AudioFormat> audioFormat;
            int qualindex = 0;

            if (outputfile.getFileExtension().toLowerCase() == ".wav") {
                audioFormat = std::make_unique<WavAudioFormat>();
            }
            else if (outputfile.getFileExtension().toLowerCase() == ".ogg") {
                audioFormat = std::make_unique<OggVorbisAudioFormat>();
                qualindex = 8; // 256k
            }
            else {
                // default to flac
                audioFormat = std::make_unique<FlacAudioFormat>();
                if (outputfile.getFileExtension().toLowerCase() != ".flac") {
                    // force name to end in flac
                    outputfile = outputfile.getParentDirectory().getNonexistentChildFile(outputfile.getFileNameWithoutExtension(), ".flac");
                }
            }

            std::unique_ptr<FileOutputStream> fos (outputfile.createOutputStream());
            
            if (fos != nullptr) {
                // Now create a writer object that writes to our output stream...

                std::unique_ptr<AudioFormatWriter> writer;
                writer.reset(audioFormat->createWriterFor (fos.get(), reader->sampleRate, reader->numChannels, 16, {}, qualindex));
                
                if (writer != nullptr)
                {
                    fos.release(); // (passes responsibility for deleting the stream to the writer object that is now using it)
                    
                    writer->writeFromAudioReader(*reader, startPos * writer->getSampleRate(), lenSecs * writer->getSampleRate());
                
                    writer->flush();
                    success = true;
                }
                
                DBG("Finished trimming file JOB to: " << pathname);
            }
        }
        else {
            DBG("Error trimming file JOB to: " << file);
        }
        
        if (success && replaceExisting) {
            outputfile.moveFileTo(sourcefile);
            DBG("Moved " << outputfile.getFullPathName() << " to " << sourcefile.getFullPathName());
            
            // remove any meta file
            File metafname = sourcefile.getParentDirectory().getChildFile("." + sourcefile.getFileName() + ".json");
            metafname.deleteFile();
            
        }

        if (success) {
            parent->trimFinished(replaceExisting ? sourcefile.getFullPathName() : outputfile.getFullPathName());
        }
        
        
        return ThreadPoolJob::jobHasFinished;
    }
    
    SonobusAudioProcessorEditor * parent;
    String file;
    
    double startPos;
    double lenSecs;
    bool replaceExisting;
};

void SonobusAudioProcessorEditor::trimFinished(const String & trimmedFile)
{
    // load it up!
    mCurrentAudioFile = URL(File(trimmedFile));
    mReloadFile  = true;
    //mTrimDone = true;
    triggerAsyncUpdate();

}

void SonobusAudioProcessorEditor::trimCurrentAudioFile(bool replaceExisting)
{
    if (mCurrentAudioFile.getFileName().isNotEmpty()) {
        String selfile = mCurrentAudioFile.getLocalFile().getFullPathName();
        double startpos, looplen;
        mWaveformThumbnail->getLoopRangeSec(startpos, looplen);
        if (looplen < processor.getTransportSource().getLengthInSeconds()) {
            trimAudioFile(selfile, startpos, looplen, replaceExisting);
        }
    }

}

void SonobusAudioProcessorEditor::trimAudioFile(const String & fname, double startPos, double lenSecs, bool replaceExisting)
{

    mWorkPool->addJob(new TrimFileJob(this, fname, startPos, lenSecs, replaceExisting), true);

}


bool SonobusAudioProcessorEditor::setupLocalisation(const String & overrideLang)
{
    String displang = SystemStats::getDisplayLanguage();
    String lang = SystemStats::getDisplayLanguage();

    String origslang = lang.initialSectionNotContaining("_").initialSectionNotContaining("-").toLowerCase();

    // currently ignore the system default language if it's one of our
    // non-vetted translations
    if (origslang == "nl"
        || origslang == "ja") {
        // force default to english
        displang = lang = "en-us";
    }

    if (overrideLang.isNotEmpty()) {
        displang = lang = overrideLang;
    }

    LocalisedStrings::setCurrentMappings(nullptr);

    bool retval = false;

    int retbytes = 0;
    int retfbytes = 0;
    String region = SystemStats::getUserRegion();

    String sflang = lang.initialSectionNotContaining("_").toLowerCase().replace("-", "");
    String slang = lang.initialSectionNotContaining("_").initialSectionNotContaining("-").toLowerCase();

    String resname = String("localized_") + slang + String("_txt");
    String resfname = String("localized_") + sflang + String("_txt");
    String resfullfilename = String("localized_") + lang.toLowerCase() + String(".txt");
    String resfilename = String("localized_") + slang + String(".txt");

    const char * rawdata = BinaryData::getNamedResource(resname.toRawUTF8(), retbytes);
    const char * rawdataf = BinaryData::getNamedResource(resfname.toRawUTF8(), retfbytes);

    File   userfilename;
    if (JUCEApplication::isStandaloneApp() && mSettingsFolder.getFullPathName().isNotEmpty()) {
        userfilename = mSettingsFolder.getChildFile(resfullfilename);
        if (!userfilename.existsAsFile()) {
            userfilename = mSettingsFolder.getChildFile(resfilename);
        }
    }


    if (userfilename.existsAsFile()) {
        DBG("Found user localization file for language: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resfname);
        LocalisedStrings * lstrings = new LocalisedStrings(userfilename.loadFileAsString(), true);

        LocalisedStrings::setCurrentMappings(lstrings);
        retval = true;
    }
    else if (rawdataf) {
        DBG("Found fulldisp localization for language: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resfname);
        LocalisedStrings * lstrings = new LocalisedStrings(String::createStringFromData(rawdataf, retfbytes), true);

        LocalisedStrings::setCurrentMappings(lstrings);
        retval = true;
    }
    else if (rawdata) {
        DBG("Found localization for language: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resname);
        LocalisedStrings * lstrings = new LocalisedStrings(String::createStringFromData(rawdata, retbytes), true);

        LocalisedStrings::setCurrentMappings(lstrings);
        retval = true;
    }
    else if (lang.startsWith("en")) {
        // special case, since untranslated is english
        retval = true;
    }
    else {
        DBG("Couldn't find mapping for lang: " << lang << "  region: " << region << " displang: " <<  displang <<  "  - resname: " << resname);
        retval = false;
    }

#ifdef JUCE_ANDROID
   // Font::setFallbackFontName("Droid Sans Fallback");
#endif

    if (retval) {
        mActiveLanguageCode = displang.toStdString();
    } else {
        mActiveLanguageCode = "en-us"; // indicates we are using english
    }

    DBG("Setup localization: active lang code: " << mActiveLanguageCode);
    
    return retval;
}


#pragma mark - ApplicationCommandTarget

enum
{
    MenuFileIndex = 0,
    MenuConnectIndex,
    MenuGroupIndex,
    MenuTransportIndex,
    MenuViewIndex,
    MenuHelpIndex
};


void SonobusAudioProcessorEditor::getCommandInfo (CommandID cmdID, ApplicationCommandInfo& info) {
    bool useKeybindings = !processor.getDisableKeyboardShortcuts();
    String name;

    switch (cmdID) {
        case SonobusCommands::MuteAllInput:
            info.setInfo (TRANS("Mute All Input"),
                          TRANS("Toggle Mute all input"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('m', ModifierKeys::noModifiers);
                info.addDefaultKeypress('m', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::MuteAllPeers:
            info.setInfo (TRANS("Mute All Users"),
                          TRANS("Toggle Mute all users"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('u', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::TogglePlayPause:
            info.setInfo (TRANS("Play/Pause"),
                          TRANS("Toggle file playback"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress (' ', ModifierKeys::noModifiers);
                info.addDefaultKeypress ('p', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::ToggleLoop:
            info.setInfo (TRANS("Loop"),
                          TRANS("Toggle file looping"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('l', ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::SkipBack:
            info.setInfo (TRANS("Return To Start"),
                          TRANS("Return to start of file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('0', ModifierKeys::noModifiers);
                info.addDefaultKeypress ('0', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::TrimSelectionToNewFile:
            info.setInfo (TRANS("Trim to New"),
                          TRANS("Trim file from selection to new file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('t', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::CloseFile:
            info.setInfo (TRANS("Close Audio File"),
                          TRANS("Close audio file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('w', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::OpenFile:
            info.setInfo (TRANS("Open Audio File..."),
                          TRANS("Open Audio file"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('o', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::ShareFile:
            info.setInfo (TRANS("Share Audio File"),
                          TRANS("Share audio file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            break;
        case SonobusCommands::RevealFile:
            info.setInfo (TRANS("Reveal Audio File"),
                          TRANS("Reveal audio file"),
                          TRANS("Popup"), 0);
            info.setActive(mCurrentAudioFile.getFileName().isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('e', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::LoadSetupFile:
            info.setInfo (TRANS("Load Setup..."),
                          TRANS("Load Setup file"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('l', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::SaveSetupFile:
            info.setInfo (TRANS("Save Setup..."),
                          TRANS("Save Setup file"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('s', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::ChatToggle:
            info.setInfo (TRANS("Show/Hide Chat"),
                          TRANS("Show or hide chat area"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('y', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::SoundboardToggle:
            info.setInfo (TRANS("Show/Hide Soundboard"),
                          TRANS("Show or hide soundboard panel"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('g', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::StopAllSoundboardPlayback:
            info.setInfo (TRANS("Stop All Soundboard Playback"),
                          TRANS("Stop All Soundboard Playback"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('k', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::ToggleAllMonitorDelay:
            info.setInfo (TRANS("Enable/Disable Monitor Delay"),
                          TRANS("Enable/Disable Monitor Delay"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('b', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::Connect:
            info.setInfo (TRANS("Connect"),
                          TRANS("Connect"),
                          TRANS("Popup"), 0);
            info.setActive(!currConnected || currGroup.isEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('n', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::Disconnect:
            info.setInfo (TRANS("Disconnect"),
                          TRANS("Disconnect"),
                          TRANS("Popup"), 0);
            info.setActive(currConnected && currGroup.isNotEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('d', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::ShowOptions:
            info.setInfo (TRANS("Show Options"),
                          TRANS("Show Options"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress (',', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::RecordToggle:
            info.setInfo (TRANS("Record"),
                          TRANS("Toggle Record"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress ('r', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::CheckForNewVersion:
            info.setInfo (TRANS("Check For New Version"),
                          TRANS("Check for New Version"),
                          TRANS("Popup"), 0);
            info.setActive(true);
            break;
        case SonobusCommands::ToggleFullInfoView:
            info.setInfo(TRANS("Toggle Full Info View"),
                TRANS("Toggle Full Info View"),
                TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress('i', ModifierKeys::commandModifier);
            }
            break;
        case SonobusCommands::ShowFileMenu:
            info.setInfo(TRANS("Show File Menu"),
                TRANS("Show File Menu"),
                TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress('f', ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::ShowConnectMenu:
            info.setInfo(TRANS("Show Connect Menu"),
                TRANS("Show Connect Menu"),
                TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress('c', ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::ShowGroupMenu:
            info.setInfo(TRANS("Show Group Menu"),
                TRANS("Show Group Menu"),
                TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress('g', ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::ShowViewMenu:
            info.setInfo(TRANS("Show View Menu"),
                TRANS("Show View Menu"),
                TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress('v', ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::ShowTransportMenu:
            info.setInfo(TRANS("Show Transport Menu"),
                TRANS("Show Transport Menu"),
                TRANS("Popup"), 0);
            info.setActive(true);
            if (useKeybindings) {
                info.addDefaultKeypress('t', ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::CopyGroupLink:
#if JUCE_IOS || JUCE_ANDROID
            name = TRANS("Share Group Link");
#else
            name = TRANS("Copy Group Link");
#endif

            info.setInfo (name, name,
                          TRANS("Popup"), 0);
            info.setActive(currConnected && !currGroup.isEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('c', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::GroupLatencyMatch:
            info.setInfo (TRANS("Group Latency Match..."),
                          TRANS("Group Latency Match..."),
                          TRANS("Popup"), 0);
            info.setActive(currConnected && !currGroup.isEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('l', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::VDONinjaVideoLink:
            info.setInfo (TRANS("VDO.Ninja Video Link..."),
                          TRANS("VDO.Ninja Video Link..."),
                          TRANS("Popup"), 0);
            info.setActive(currConnected && !currGroup.isEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('v', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::SuggestNewGroup:
            info.setInfo (TRANS("Suggest New Group..."),
                          TRANS("Suggest New Group..."),
                          TRANS("Popup"), 0);
            info.setActive(currConnected && !currGroup.isEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('s', ModifierKeys::commandModifier | ModifierKeys::altModifier);
            }
            break;
        case SonobusCommands::ResetAllJitterBuffers:
            info.setInfo (TRANS("Reset All Jitter Buffers"),
                          TRANS("Reset All Jitter Buffers"),
                          TRANS("Popup"), 0);
            info.setActive(currConnected && !currGroup.isEmpty());
            if (useKeybindings) {
                info.addDefaultKeypress ('j', ModifierKeys::commandModifier);
            }
            break;

    }
}

void SonobusAudioProcessorEditor::getAllCommands (Array<CommandID>& cmds) {
    cmds.add(SonobusCommands::MuteAllInput);
    cmds.add(SonobusCommands::MuteAllPeers);
    cmds.add(SonobusCommands::TogglePlayPause);
    cmds.add(SonobusCommands::ToggleLoop);
    cmds.add(SonobusCommands::TrimSelectionToNewFile);
    cmds.add(SonobusCommands::CloseFile);
    cmds.add(SonobusCommands::ShareFile);
    cmds.add(SonobusCommands::RevealFile);
    cmds.add(SonobusCommands::Connect);
    cmds.add(SonobusCommands::Disconnect);
    cmds.add(SonobusCommands::ShowOptions);
    cmds.add(SonobusCommands::OpenFile);
    cmds.add(SonobusCommands::RecordToggle);
    cmds.add(SonobusCommands::CheckForNewVersion);
    cmds.add(SonobusCommands::LoadSetupFile);
    cmds.add(SonobusCommands::SaveSetupFile);
    cmds.add(SonobusCommands::ChatToggle);
    cmds.add(SonobusCommands::SoundboardToggle);
    cmds.add(SonobusCommands::SkipBack);
    cmds.add(SonobusCommands::ShowFileMenu);
    cmds.add(SonobusCommands::ShowTransportMenu);
    cmds.add(SonobusCommands::ShowViewMenu);
    cmds.add(SonobusCommands::ShowGroupMenu);
    cmds.add(SonobusCommands::ShowConnectMenu);
    cmds.add(SonobusCommands::ToggleFullInfoView);
    cmds.add(SonobusCommands::StopAllSoundboardPlayback);
    cmds.add(SonobusCommands::ToggleAllMonitorDelay);
    cmds.add(SonobusCommands::CopyGroupLink);
    cmds.add(SonobusCommands::GroupLatencyMatch);
    cmds.add(SonobusCommands::VDONinjaVideoLink);
    cmds.add(SonobusCommands::SuggestNewGroup);
    cmds.add(SonobusCommands::ResetAllJitterBuffers);

}

bool SonobusAudioProcessorEditor::perform (const InvocationInfo& info) {
    bool ret = true;
    
    switch (info.commandID) {
        case SonobusCommands::MuteAllInput:
            DBG("got mute toggle!");
            mMainMuteButton->setToggleState(!mMainMuteButton->getToggleState(), sendNotification);
            break;
        case SonobusCommands::MuteAllPeers:
            DBG("got mute peers toggle!");
            mMainRecvMuteButton->setToggleState(!mMainRecvMuteButton->getToggleState(), sendNotification);
            break;
        case SonobusCommands::TogglePlayPause:
            DBG("got play pause!");
            if (mPlayButton->isVisible()) {
                mPlayButton->setToggleState(!mPlayButton->getToggleState(), sendNotification);
            }
            break;
        case SonobusCommands::StopAllSoundboardPlayback:
            if (mSoundboardView) {
                mSoundboardView->stopAllSamples();
            }
            break;
        case SonobusCommands::ToggleAllMonitorDelay:
            if (getInputChannelGroupsView()) {
                getInputChannelGroupsView()->toggleAllMonitorDelay();
            }
            break;
        case SonobusCommands::ToggleFullInfoView:

            buttonClicked(processor.getPeerDisplayMode() == SonobusAudioProcessor::PeerDisplayModeMinimal ?
                          mPeerLayoutFullButton.get() : mPeerLayoutMinimalButton.get());
            break;
        case SonobusCommands::SkipBack:
            buttonClicked(mSkipBackButton.get());
            break;
        case SonobusCommands::ShowFileMenu:
            if (mMenuBar) {
                mMenuBar->showMenu(MenuFileIndex);
            }
            break;
        case SonobusCommands::ShowTransportMenu:
            if (mMenuBar) {
                mMenuBar->showMenu(MenuTransportIndex);
            }
            break;
        case SonobusCommands::ShowConnectMenu:
            if (mMenuBar) {
                mMenuBar->showMenu(MenuConnectIndex);
            }
            break;
        case SonobusCommands::ShowGroupMenu:
            if (mMenuBar) {
                mMenuBar->showMenu(MenuGroupIndex);
            }
            break;
        case SonobusCommands::ShowViewMenu:
            if (mMenuBar) {
                mMenuBar->showMenu(MenuViewIndex);
            }
            break;
        case SonobusCommands::ToggleLoop:
            DBG("got loop toggle!");
            if (mLoopButton->isVisible()) {
                mLoopButton->setToggleState(!mLoopButton->getToggleState(), sendNotification);
            }
            break;
        case SonobusCommands::TrimSelectionToNewFile:
            DBG("Got trim!");
            trimCurrentAudioFile(false);
            break;
        case SonobusCommands::CloseFile:
            DBG("got close file!");
            if (mDismissTransportButton->isVisible()) {
                buttonClicked(mDismissTransportButton.get());
            }

            break;
        case SonobusCommands::ShareFile:
            DBG("got share file!");

            break;
        case SonobusCommands::RevealFile:
            DBG("got reveal file!");
            if (mCurrentAudioFile.getFileName().isNotEmpty()) {
                mCurrentAudioFile.getLocalFile().revealToUser();
            }
            else {
                if (mCurrOpenDir.getFullPathName().isEmpty()) {
                    mCurrOpenDir = processor.getDefaultRecordingDirectory().getLocalFile();
                }
                mCurrOpenDir.revealToUser();
            }
            break;
        case SonobusCommands::OpenFile:
            DBG("got open file!");
            openFileBrowser();

            break;
        case SonobusCommands::LoadSetupFile:
            DBG("got load setup file!");
            showLoadSettingsPreset();

            break;
        case SonobusCommands::ChatToggle:
            showChatPanel(!mChatView->isVisible());
            resized();
            break;
        case SonobusCommands::SoundboardToggle:
            showSoundboardPanel(!mSoundboardView->isVisible());
            resized();
            break;
        case SonobusCommands::SaveSetupFile:
            DBG("got save setup file!");
            showSaveSettingsPreset();

            break;
        case SonobusCommands::Connect:
            DBG("got connect!");
            if (!currConnected || currGroup.isEmpty()) {
                buttonClicked(mConnectButton.get());
            }
            break;
        case SonobusCommands::Disconnect:
            DBG("got disconnect!");
            
            if (currConnected && currGroup.isNotEmpty()) {
                buttonClicked(mConnectButton.get());
            }

            break;
        case SonobusCommands::ShowOptions:
            DBG("got show options!");
            buttonClicked(mSettingsButton.get());

            break;
        case SonobusCommands::RecordToggle:
            DBG("got record toggle!");
            buttonClicked(mRecordingButton.get());

            break;
        case SonobusCommands::CheckForNewVersion:   
            LatestVersionCheckerAndUpdater::getInstance()->checkForNewVersion (true); 
            break;

        case SonobusCommands::CopyGroupLink:
            copyGroupLink();
            break;
        case SonobusCommands::GroupLatencyMatch:
            showLatencyMatchView(true);
            break;
        case SonobusCommands::VDONinjaVideoLink:
            showVDONinjaView(true, mVideoButton->isShowing());
            break;
        case SonobusCommands::SuggestNewGroup:
            showSuggestGroupView(true);
            break;
        case SonobusCommands::ResetAllJitterBuffers:
            resetJitterBufferForAll();
            break;

        default:
            ret = false;
    }
    
    return ret;
}

void SonobusAudioProcessorEditor::populateRecentSetupsMenu(PopupMenu & popup)
{
    popup.clear();

    auto callback = [this](File file) {
        this->loadSettingsFromFile(file);
    };

    if (getRecentSetupFiles && getRecentSetupFiles()) {
        auto recents = getRecentSetupFiles();

        // load the recents in reverse order

        for (int i=recents->size()-1; i >= 0; --i) {
            const auto & fname = recents->getReference(i);
            File file(fname);
            if (file.existsAsFile()) {
                popup.addItem( file.getFileNameWithoutExtension() , [file, callback]() { callback(file); });
            }
            else {
                // remove it! (we can do this because we are traversing it from the end)
                recents->remove(i);
            }
        }

    }

}



#pragma MenuBarModel



StringArray SonobusAudioProcessorEditor::SonobusMenuBarModel::getMenuBarNames()
{
    return StringArray(TRANS("File"),
                       TRANS("Connect"),
                       TRANS("Group"),
                       TRANS("Transport"),
                       TRANS("View")
                       );
    //TRANS("Help"));
}

PopupMenu SonobusAudioProcessorEditor::SonobusMenuBarModel::getMenuForIndex (int topLevelMenuIndex, const String& /*menuName*/)
{
    PopupMenu retval;
    PopupMenu recents;

    switch (topLevelMenuIndex) {
        case MenuFileIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::OpenFile);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::CloseFile);
#if JUCE_IOS || JUCE_ANDROID
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ShareFile);
#else
            retval.addCommandItem (&parent.commandManager, SonobusCommands::RevealFile);            
#endif
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::TrimSelectionToNewFile);

            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::LoadSetupFile);
            parent.populateRecentSetupsMenu(recents);
            retval.addSubMenu(TRANS("Load Recent Setup"), recents, recents.getNumItems() > 0);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::SaveSetupFile);

            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::CheckForNewVersion);

#if (JUCE_WINDOWS || JUCE_LINUX)
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ShowOptions);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, StandardApplicationCommandIDs::quit);
#endif
            break;
        case MenuConnectIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::Connect);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::Disconnect);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::MuteAllInput);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::MuteAllPeers);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ToggleAllMonitorDelay);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ResetAllJitterBuffers);
            break;
        case MenuGroupIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::CopyGroupLink);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::GroupLatencyMatch);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::VDONinjaVideoLink);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::SuggestNewGroup);
            break;
        case MenuTransportIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::TogglePlayPause);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::SkipBack);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ToggleLoop);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::RecordToggle);
            retval.addSeparator();
            retval.addCommandItem (&parent.commandManager, SonobusCommands::StopAllSoundboardPlayback);
            break;
        case MenuViewIndex:
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ChatToggle);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::SoundboardToggle);
            retval.addCommandItem (&parent.commandManager, SonobusCommands::ToggleFullInfoView);
            break;

        case MenuHelpIndex:
            break;
    }
    
    return retval;
}

void SonobusAudioProcessorEditor::SonobusMenuBarModel::menuItemSelected (int menuItemID, int topLevelMenuIndex)
{
#if JUCE_MAC
    if (topLevelMenuIndex == -1) {
        switch (menuItemID) {
            case 1:
                // about
                break;
            case 2:
                parent.commandManager.invokeDirectly(SonobusCommands::ShowOptions, true);
                break;
        }
    }
#endif
}

