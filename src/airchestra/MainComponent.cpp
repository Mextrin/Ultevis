#include "MainComponent.h"

namespace airchestra
{
MainComponent::MainComponent(EventLogger& eventLogger)
    : logger(eventLogger),
      uiRenderer(viewState, logger),
      imguiLayer(uiRenderer, logger)
{
    addAndMakeVisible(imguiLayer);
    setSize(1280, 800);
}

void MainComponent::resized()
{
    imguiLayer.setBounds(getLocalBounds());
}
}
