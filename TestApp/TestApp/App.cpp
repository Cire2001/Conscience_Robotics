#include "App.h"
#include "MainFrame.h"

wxIMPLEMENT_APP(App);

bool App::OnInit()
{
    MainFrame* mainFrame = new MainFrame("Application Cam�ra");
    mainFrame->Show();
    return true;
}