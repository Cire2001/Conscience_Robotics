#pragma once
#include <wx/wx.h>

class MainFrame : public wxFrame
{
public:
    MainFrame(const wxString& title);

private:
    wxButton* m_captureButton;
    wxButton* m_snapshotButton;
    wxButton* m_xProfileButton;

    wxStaticText* m_captureText;
    wxStaticText* m_snapshotText;
    wxStaticText* m_xProfileText;
};